"""Lossless, bounded NSP2/14 -> NSP2/15 sparse-animation repacker.

The input is an existing privately built pack, never an original game archive.
Every selected format-4 byte is recovered from exact RGB565/alpha tiles and its
original scanline geometry before the new file is published. See
docs/content-tiles-0.17.md for the on-disk contract and validation boundaries.
"""
from __future__ import annotations

import argparse
from collections import OrderedDict, deque
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
import hashlib
import json
import lzma
import mmap
import os
from pathlib import Path
import shutil
import struct
from tempfile import TemporaryDirectory, NamedTemporaryFile
import time

import numpy as np
from tile_filter import restore_tiles

try:
    import lz4.block as _lz4
except ImportError:
    _lz4 = None
from lz4_block import decode as _portable_lz4_decode


ROOT = Path(__file__).resolve().parents[1]
REFERENCE = 0x41cef1ea
FILE_LIMIT = 256 * 1024 * 1024
DIRECTORY_LIMIT = 2 * 1024 * 1024
IMAGE_LIMIT = 320 * 240 * 8 + 2 * 240
ENCODED_LIMIT = IMAGE_LIMIT + IMAGE_LIMIT // 255 + 32
GROUP_LIMIT = 512 * 1024
GLYPH_GROUP_VERSION = 32
GLYPH_GROUP_FLAG = 0x8000
GLYPH_GROUP_LIMIT = 64 * 1024
STRING_GROUP_VERSION = 33
STRING_GROUP_FLAG = 0x80000000
STRING_GROUP_LIMIT = 32 * 1024
SIDE = 16
TILE_BYTES = 770
MAX_TILES_PER_GROUP = GROUP_LIMIT // TILE_BYTES
MAX_TILES = 65535 * 300
INDEX_FRAMES = 64
HOST_CACHE_GROUPS = 128
SPOOL_LIMIT = 2 * 1024 * 1024 * 1024
STRING_LIMIT = 16384
HEADER = struct.Struct('<4s9I')
IMAGE = struct.Struct('<IHHII')
GLYPH = struct.Struct('<IIHBBHH')
CLIP = struct.Struct('<IHHhhI')
GLOBAL = struct.Struct('<4s15I')
GROUP = struct.Struct('<4s7I')
DESCRIPTOR = struct.Struct('<4I')
FRAME = struct.Struct('<3I')


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def fnv(data):
    value = 2166136261
    for byte in data:
        value = ((value ^ byte) * 16777619) & 0xffffffff
    return value


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':')).encode()


def atomic_bytes(path, data):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with NamedTemporaryFile(dir=path.parent, prefix=path.name + '.', delete=False) as handle:
        temporary = Path(handle.name)
        try:
            handle.write(data)
            handle.flush()
            os.fsync(handle.fileno())
        except BaseException:
            temporary.unlink(missing_ok=True)
            raise
    try:
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def private_path(path, label):
    path = Path(path).expanduser().resolve()
    require(path != ROOT and ROOT not in path.parents,
            label + ' must be outside the public source repository')
    return path


def protect_paths(input_path, output_path, work_directory, receipt_path=None):
    """The encoder owns every filename in its cache namespace, never packs."""
    cache_root = (Path(work_directory).resolve() / 'encoded').resolve()
    protected = [('Input', Path(input_path)), ('Output', Path(output_path))]
    if receipt_path is not None:
        protected.append(('Receipt', Path(receipt_path)))
    require(len({path.resolve() for _label, path in protected}) == len(protected),
            'Input, output and receipt must differ')
    for label, path in protected:
        # Protect both the target and the directory entry. Replacing a final
        # symlink inside the cache would still change the user's input name,
        # even when its resolved target is safely outside that namespace.
        for candidate in (path.resolve(), path.parent.resolve() / path.name):
            require(candidate != cache_root and cache_root not in candidate.parents,
                    label + ' overlaps the writable encoded-cache namespace')


def emit(progress, stage, **values):
    if progress:
        progress(dict(stage=stage, **values))


def validate_sparse(raw, width, height, *, pixels=False):
    """Return original geometry and optionally exact dense host pixel planes."""
    require(0 < len(raw) <= IMAGE_LIMIT, 'Invalid sparse image byte count')
    geometry = bytearray()
    colors = np.zeros((height, width), dtype='<u2') if pixels else None
    alpha = np.zeros((height, width), dtype='u1') if pixels else None
    at = 0
    for y in range(height):
        require(at + 2 <= len(raw), 'Truncated sparse row')
        count, = struct.unpack_from('<H', raw, at)
        require(count <= width, 'Excessive sparse span count')
        geometry.extend(raw[at:at + 2])
        at += 2
        previous = 0
        for _ in range(count):
            require(at + 5 <= len(raw), 'Truncated sparse span')
            x, size, mode = struct.unpack_from('<HHB', raw, at)
            require(mode in (0, 1) and size > 0 and previous <= x and x + size <= width,
                    'Invalid sparse span geometry')
            previous = x + size
            geometry.extend(raw[at:at + 5])
            at += 5
            byte_count = size * (2 + mode)
            require(at + byte_count <= len(raw), 'Truncated sparse pixels')
            if pixels:
                if mode:
                    row = np.frombuffer(raw, dtype='u1', count=byte_count, offset=at).reshape(size, 3)
                    colors[y, x:previous] = row[:, 0].astype('u2') | (row[:, 1].astype('u2') << 8)
                    alpha[y, x:previous] = row[:, 2]
                else:
                    colors[y, x:previous] = np.frombuffer(raw, dtype='<u2', count=size, offset=at)
                    alpha[y, x:previous] = 255
            at += byte_count
    require(at == len(raw), 'Trailing sparse image bytes')
    return bytes(geometry), colors, alpha


def validate_raw(raw, width, height, fmt):
    require(0 < len(raw) <= IMAGE_LIMIT, 'Decoded image outside limit')
    area = width * height
    if fmt == 4:
        validate_sparse(raw, width, height)
    else:
        require(fmt in (0, 1, 2, 3), 'Unsupported decoded image format')
        expected = (area * 2, area * 3, 512 + area, 512 + area * 2)[fmt]
        require(len(raw) == expected, 'Invalid decoded image size')


class PackView:
    """Checked mmap-compatible directory view; never copy the complete pack."""
    def __init__(self, data, auxiliary_count, *, versions=(14,), profile=None):
        require(40 <= len(data) <= FILE_LIMIT, 'Pack outside file limit')
        fields = HEADER.unpack_from(data)
        magic, self.version, reference, size, ni, ng, ns, nc, nk, checksum = fields
        require(magic == b'NSP2' and self.version in versions and reference == REFERENCE,
                'Expected pinned NSP2 pack version')
        require(size == len(data), 'Pack size mismatch')
        require(0 < ni <= 65535 and 0 < ng <= 32768 and 0 < ns <= 65535 and
                0 < nc <= 65535 and 0 < nk <= 65535 and 0 < auxiliary_count <= 65535,
                'Invalid directory counts')
        if profile is not None:
            require((nk, ns, nc, auxiliary_count) == tuple(profile), 'Incompatible catalog profile')
        self.data = data
        self.counts = (ni, ng, ns, nc, nk, auxiliary_count)
        self.glyph_at = 40 + ni * IMAGE.size
        self.string_at = self.glyph_at + ng * GLYPH.size
        self.clip_at = self.string_at + ns * 8
        self.card_at = self.clip_at + nc * CLIP.size
        self.aux_at = self.card_at + nk * 16
        self.base_end = self.aux_at + auxiliary_count * 4
        require(self.base_end <= min(len(data), DIRECTORY_LIMIT), 'Directory outside limit')
        self.images = [IMAGE.unpack_from(data, 40 + i * 16) for i in range(ni)]
        for offset, w, h, fmt, length in self.images:
            require(0 < w <= 320 and 0 < h <= 240 and 0 <= fmt <= (7 if self.version >= 20 else 6 if self.version >= 15 else 5),
                    'Invalid image dimensions or format')
            require(offset % 2 == 0 and 0 < length <= ENCODED_LIMIT, 'Invalid image range')
            self.block(offset, length)
        self.glyphs = [GLYPH.unpack_from(data, self.glyph_at + i * 16) for i in range(ng)]
        self.retained_cache = OrderedDict()
        previous_glyph = -1
        for glyph_id, (cp, offset, px, w, h, advance, encoded) in enumerate(self.glyphs):
            require(cp <= 0x10ffff and 0 < px <= (255 if self.version >= 24 else 32) and
                    (px & (31 if self.version >= 28 else 63)) in ((4, 5, 6, 8, 10, 12, 14, 16, 20) if self.version >= 33 else (5, 6, 8, 10, 12, 14, 16, 20) if self.version >= 28 else (8, 10, 12, 16, 20)) and
                    0 < w <= 40 and 0 < h <= 40 and advance <= 40,
                    'Invalid glyph geometry')
            glyph_key = (px << 24) | cp
            require(glyph_key > previous_glyph, 'Glyph keys must be strictly increasing')
            previous_glyph = glyph_key
            self.glyph_payload(glyph_id)
        self.strings = [struct.unpack_from('<II', data, self.string_at + i * 8) for i in range(ns)]
        for string_id in range(len(self.strings)):
            payload = self.string_payload(string_id)
            require(payload[-1] == 0 and b'\0' not in payload[:-1], 'Invalid string terminator')
            payload[:-1].decode('utf-8')
        self.clips = [CLIP.unpack_from(data, self.clip_at + i * 16) for i in range(nc)]
        for first, count, ms, px, py, reserved in self.clips:
            require(0 < count <= 256 and first + count <= ni and 0 < ms <= 1000 and
                    -640 <= px <= 640 and -480 <= py <= 480 and
                    (0 <= reserved < 1000000 if self.version >= 29 else reserved == 0), 'Invalid clip')
        self.cards = [struct.unpack_from('<4I', data, self.card_at + i * 16) for i in range(nk)]
        self.aux = list(struct.unpack_from('<' + 'I' * auxiliary_count, data, self.aux_at))
        require(all(row[0] < ni and all(value < ns for value in row[1:]) for row in self.cards)
                and all(image < ni for image in self.aux), 'Invalid directory reference')

    def block(self, offset, size):
        require(offset >= self.base_end and 0 <= size <= len(self.data) - offset,
                'Payload outside pack')
        return self.data[offset:offset + size]

    def glyph_payload(self, glyph_id):
        """Return exact old raw/RLE glyph bytes through either directory form."""
        _cp, offset, _px, width, height, _advance, encoded = self.glyphs[glyph_id]
        grouped = encoded & GLYPH_GROUP_FLAG
        require(not grouped or self.version >= GLYPH_GROUP_VERSION,
                'Grouped glyph requires NSP2/32')
        encoded &= ~GLYPH_GROUP_FLAG
        count = encoded or width * height
        if not grouped:
            payload = self.block(offset, count)
        else:
            relative, group_size, raw_at, raw_bytes = struct.unpack('<iIII', self.block(offset, 16))
            group_at = offset + relative
            require(GROUP.size < group_size <= ENCODED_LIMIT and raw_bytes == count,
                    'Invalid grouped glyph descriptor')
            require(group_at >= self.base_end and group_size <= len(self.data) - group_at,
                    'Grouped glyph payload outside pack')
            tag, raw_size, stored_size, prop, items, a, b, c = GROUP.unpack_from(self.data, group_at)
            require(tag == b'RGR1' and 0 < raw_size <= GLYPH_GROUP_LIMIT and
                    stored_size == group_size - GROUP.size and prop == 14 and
                    0 < items <= 32768 and not (a or b or c) and
                    raw_at <= raw_size and raw_bytes <= raw_size - raw_at,
                    'Invalid grouped glyph header/slice')
            cache_key = ('glyph', group_at, group_size)
            if cache_key in self.retained_cache:
                self.retained_cache.move_to_end(cache_key)
                raw = self.retained_cache[cache_key]
            else:
                raw = decode_lzma2(self.data[group_at + GROUP.size:group_at + group_size], raw_size)
                if len(self.retained_cache) >= HOST_CACHE_GROUPS:
                    self.retained_cache.popitem(last=False)
                self.retained_cache[cache_key] = raw
            payload = raw[raw_at:raw_at + raw_bytes]
        if encoded:
            require(encoded % 2 == 0 and encoded <= 2 * width * height and
                    all(payload[::2]) and sum(payload[::2]) == width * height,
                    'Invalid glyph run encoding')
        return payload

    def glyph_alpha(self, glyph_id):
        """Independent unpacked alpha plane for converter readback proofs."""
        payload = self.glyph_payload(glyph_id)
        if not self.glyphs[glyph_id][6] & ~GLYPH_GROUP_FLAG:
            return bytes(payload)
        return b''.join(bytes((alpha,)) * count for count, alpha in zip(payload[::2], payload[1::2]))

    def string_length(self, string_id):
        return self.strings[string_id][1] & ~STRING_GROUP_FLAG

    def string_payload(self, string_id):
        """Return exact UTF-8 bytes including the original terminal NUL."""
        offset, stored = self.strings[string_id]
        grouped, length = stored & STRING_GROUP_FLAG, stored & ~STRING_GROUP_FLAG
        require(length < STRING_LIMIT, 'String exceeds runtime byte limit')
        require(not grouped or self.version >= STRING_GROUP_VERSION,
                'Grouped string requires NSP2/33')
        require(not grouped or string_id != 6110, 'Locale marker must remain ungrouped')
        if not grouped:
            return self.block(offset, length + 1)
        relative, group_size, raw_at, raw_bytes = struct.unpack('<iIII', self.block(offset, 16))
        group_at = offset + relative
        require(GROUP.size < group_size <= ENCODED_LIMIT and raw_bytes == length + 1 and
                group_at >= self.base_end and group_size <= len(self.data) - group_at and
                (group_at + group_size <= offset or offset + 16 <= group_at),
                'Invalid grouped string descriptor')
        tag, raw_size, stored_size, prop, items, a, b, c = GROUP.unpack_from(self.data, group_at)
        require(tag == b'RGR1' and 0 < raw_size <= STRING_GROUP_LIMIT and
                stored_size == group_size - GROUP.size and prop == 14 and
                0 < items <= 16384 and not (a or b or c) and
                raw_at <= raw_size and raw_bytes <= raw_size - raw_at,
                'Invalid grouped string header/slice')
        cache_key = ('string', group_at, group_size)
        if cache_key in self.retained_cache:
            self.retained_cache.move_to_end(cache_key)
            raw = self.retained_cache[cache_key]
        else:
            raw = decode_lzma2(self.data[group_at + GROUP.size:group_at + group_size], raw_size)
            if len(self.retained_cache) >= HOST_CACHE_GROUPS:
                self.retained_cache.popitem(last=False)
            self.retained_cache[cache_key] = raw
        return raw[raw_at:raw_at + raw_bytes]

    def image_raw(self, image_id):
        offset, width, height, fmt, length = self.images[image_id]
        payload = self.block(offset, length)
        if fmt == 7:
            require(self.version >= 20 and length == 16, 'Invalid retained descriptor')
            relative, group_size, raw_at, shape = struct.unpack('<iIII', payload)
            group_at, fmt, size = offset + relative, shape >> 24, shape & 0xffffff
            require(fmt <= 4 and 0 < size <= IMAGE_LIMIT, 'Invalid retained image shape')
            if group_at in self.retained_cache:
                self.retained_cache.move_to_end(group_at)
                raw = self.retained_cache[group_at]
            else:
                require(GROUP.size < group_size <= ENCODED_LIMIT, 'Invalid retained group size')
                group = self.block(group_at, group_size)
                tag, raw_size, encoded_size, prop, _items, _a, _b, _c = GROUP.unpack_from(group)
                require(tag == b'RGR1' and 0 < raw_size <= GROUP_LIMIT and
                        encoded_size == group_size - GROUP.size and prop == 14,
                        'Invalid retained group header')
                raw = decode_lzma2(group[GROUP.size:], raw_size)
                if len(self.retained_cache) == HOST_CACHE_GROUPS:
                    self.retained_cache.popitem(last=False)
                self.retained_cache[group_at] = raw
            require(raw_at <= len(raw) and size <= len(raw) - raw_at, 'Retained slice outside group')
            payload = raw[raw_at:raw_at + size]
        if fmt == 5:
            require(len(payload) >= 6, 'Truncated LZ4 image wrapper')
            fmt, size = struct.unpack_from('<BI', payload)
            require(fmt <= 4 and 0 < size <= IMAGE_LIMIT, 'Invalid LZ4 image wrapper')
            try:
                payload = (_lz4.decompress(payload[5:], uncompressed_size=size) if _lz4
                           else _portable_lz4_decode(payload[5:], size))
            except Exception as error:
                raise ValueError('Invalid LZ4 image block') from error
            require(len(payload) == size, 'LZ4 image length mismatch')
        validate_raw(payload, width, height, fmt)
        return width, height, fmt, payload

    def selected_images(self, first_clip):
        require(0 <= first_clip < len(self.clips), 'First tiled clip outside catalog')
        selected = {image for first, count, *_ in self.clips[first_clip:]
                    for image in range(first, first + count)}
        protected = {image for first, count, *_ in self.clips[:first_clip]
                     for image in range(first, first + count)}
        protected.update(row[0] for row in self.cards)
        protected.update(self.aux)
        selected.difference_update(protected)
        retained_ranges = {(off, length) for i, (off, _w, _h, _fmt, length) in enumerate(self.images)
                           if i not in selected}
        selected = {i for i in selected if (self.images[i][0], self.images[i][4]) not in retained_ranges}
        require(bool(selected), 'No independent animation images selected')
        return sorted(selected)


class TileSpool:
    """Global exact deduplication with disk bytes as the collision oracle."""
    def __init__(self, path, limit=SPOOL_LIMIT):
        self.writer = Path(path).open('wb', buffering=0)
        self.reader = Path(path).open('rb', buffering=0)
        self.limit = limit
        self.keys = {}
        self.count = self.occurrences = self.duplicates = self.collisions = 0

    @staticmethod
    def key(record):
        return record[:2] + hashlib.sha256(record).digest()

    def add(self, record):
        require(len(record) == TILE_BYTES, 'Invalid tile record length')
        self.occurrences += 1
        key = self.key(record)
        candidate = self.keys.get(key)
        bucket = candidate if isinstance(candidate, list) else (() if candidate is None else (candidate,))
        for tile_id in bucket:
            self.reader.seek(tile_id * TILE_BYTES)
            if self.reader.read(TILE_BYTES) == record:
                self.duplicates += 1
                return tile_id
            self.collisions += 1
        require(self.count < MAX_TILES and (self.count + 1) * TILE_BYTES <= self.limit,
                'Unique tile spool exceeds configured bound')
        tile_id = self.count
        self.count += 1
        self.writer.write(record)
        if candidate is None:
            self.keys[key] = tile_id
        elif isinstance(candidate, list):
            candidate.append(tile_id)
        else:
            self.keys[key] = [candidate, tile_id]
        return tile_id

    def close(self):
        self.writer.close()
        self.reader.close()
        self.keys.clear()


@dataclass
class Plan:
    frames: list
    mapping: dict
    retained: list
    tiles: int
    tile_path: Path
    index_path: Path
    tile_ranges: list
    index_ranges: list
    stats: dict


def build_plan(source, directory, first_clip, per_group, *, progress=None, spool_limit=SPOOL_LIMIT):
    require(1 <= per_group <= MAX_TILES_PER_GROUP, 'Invalid tiles per group')
    selected = source.selected_images(first_clip)
    retained = sorted(set(range(len(source.images))) - set(selected))
    # Validate retained image formats too, once per identical directory tuple.
    validated = set()
    for image_id in retained:
        entry = source.images[image_id]
        if entry not in validated:
            source.image_raw(image_id)
            validated.add(entry)
    tile_path, index_path = directory / 'tiles.raw', directory / 'indices.raw'
    spool = TileSpool(tile_path, spool_limit)
    mapping, frames, seen, source_ranges = {}, [], {}, {}
    index_ranges = []
    group_start = group_size = group_frames = 0
    raw_frame_bytes = original_aliases = canonical_collisions = 0
    try:
        with index_path.open('wb') as indices:
            for number, image_id in enumerate(selected):
                entry = source.images[image_id]
                if entry in source_ranges:
                    mapping[image_id] = source_ranges[entry]
                    original_aliases += 1
                    continue
                width, height, fmt, raw = source.image_raw(image_id)
                require(fmt == 4, 'Selected animation must have sparse inner format 4')
                key = (width, height, len(raw), sha256(raw))
                found = None
                for previous in seen.get(key, ()):
                    if source.image_raw(frames[previous]['image'])[3] == raw:
                        found = previous
                        break
                    canonical_collisions += 1
                if found is not None:
                    mapping[image_id] = source_ranges[entry] = found
                    continue
                geometry, colors, alpha = validate_sparse(raw, width, height, pixels=True)
                tile_ids = []
                rgb_tile = np.zeros((SIDE, SIDE), dtype='<u2')
                alpha_tile = np.zeros((SIDE, SIDE), dtype='u1')
                for y in range(0, height, SIDE):
                    h = min(SIDE, height - y)
                    for x in range(0, width, SIDE):
                        w = min(SIDE, width - x)
                        rgb_tile.fill(0)
                        alpha_tile.fill(0)
                        rgb_tile[:h, :w] = colors[y:y + h, x:x + w]
                        alpha_tile[:h, :w] = alpha[y:y + h, x:x + w]
                        tile_ids.append(spool.add(bytes((w, h)) + rgb_tile.tobytes() + alpha_tile.tobytes()))
                raw_ids = np.asarray(tile_ids, dtype='<u4').tobytes()
                record = FRAME.pack(len(raw), len(raw_ids), len(geometry)) + raw_ids + geometry
                require(len(record) <= GROUP_LIMIT, 'Frame index record exceeds group bound')
                if group_frames and (group_frames == INDEX_FRAMES or group_size + len(record) > GROUP_LIMIT):
                    index_ranges.append((group_start, group_size, group_frames))
                    group_start += group_size
                    group_size = group_frames = 0
                canonical_id = len(frames)
                frames.append(dict(image=image_id, width=width, height=height, raw_bytes=len(raw),
                                   raw_sha256=key[3], index_group=len(index_ranges),
                                   record_offset=group_size, record_bytes=len(record)))
                mapping[image_id] = source_ranges[entry] = canonical_id
                seen.setdefault(key, []).append(canonical_id)
                indices.write(record)
                group_size += len(record)
                group_frames += 1
                raw_frame_bytes += len(raw)
                require(spool.count * TILE_BYTES + group_start + group_size <= spool_limit,
                        'Combined raw spools exceed configured bound')
                if len(frames) % 500 == 0:
                    emit(progress, 'tile_index', canonical_frames=len(frames),
                         processed_images=number + 1, selected_images=len(selected), unique_tiles=spool.count)
            if group_frames:
                index_ranges.append((group_start, group_size, group_frames))
        tile_ranges = [(first * TILE_BYTES, min(per_group, spool.count - first) * TILE_BYTES,
                        min(per_group, spool.count - first)) for first in range(0, spool.count, per_group)]
        stats = dict(selected_images=len(selected), retained_images=len(retained), canonical_frames=len(frames),
                     identical_source_range_aliases=original_aliases,
                     canonical_hash_collisions_compared=canonical_collisions,
                     tile_occurrences=spool.occurrences, exact_duplicate_tiles=spool.duplicates,
                     tile_hash_collisions_compared=spool.collisions, unique_tiles=spool.count,
                     original_canonical_fmt4_bytes=raw_frame_bytes,
                     tile_raw_bytes=spool.count * TILE_BYTES, index_raw_bytes=index_path.stat().st_size)
        require(set(mapping).isdisjoint(retained) and len(mapping) + len(retained) == len(source.images),
                'Image coverage partition mismatch')
        return Plan(frames, mapping, retained, spool.count, tile_path, index_path,
                    tile_ranges, index_ranges, stats)
    finally:
        spool.close()


def decode_lzma2(payload, raw_size):
    require(0 < raw_size <= GROUP_LIMIT and 0 < len(payload) + GROUP.size <= ENCODED_LIMIT,
            'LZMA2 block outside bounds')
    decoder = lzma.LZMADecompressor(format=lzma.FORMAT_RAW,
        filters=[dict(id=lzma.FILTER_LZMA2, dict_size=GROUP_LIMIT)])
    try:
        raw = decoder.decompress(payload, max_length=raw_size + 1)
    except lzma.LZMAError as error:
        raise ValueError('Invalid raw LZMA2 block') from error
    require(len(raw) == raw_size and decoder.eof and not decoder.unused_data,
            'LZMA2 output length, end marker or trailing bytes mismatch')
    return raw


class EncodedCache:
    """Content-keyed cache; imported metadata never substitutes for byte proof."""
    def __init__(self, directory, reuse=()):
        self.directory = Path(directory)
        self.directory.mkdir(parents=True, exist_ok=True)
        self.imported = {}
        for root in reuse:
            root = Path(root).resolve()
            require(root.is_dir(), 'Reuse cache is not a directory')
            count = 0
            for metadata in sorted(root.glob('*.json')):
                count += 1
                require(count <= 32768, 'Reuse cache metadata count exceeds bound')
                if metadata.stat().st_size > 16384:
                    continue
                row = json.loads(metadata.read_bytes())
                if not isinstance(row, dict) or not all(key in row for key in
                    ('raw_sha256', 'raw_bytes', 'encoded_bytes', 'identity')):
                    continue
                identity = row['identity']
                require(isinstance(identity, dict), 'Invalid cache identity')
                if (identity.get('codec'), identity.get('preset'), identity.get('dict_size')) != ('raw LZMA2', 6, GROUP_LIMIT):
                    continue
                payload = metadata.with_suffix('.lzma2')
                require(payload.is_file() and payload.resolve().parent == root,
                        'Reuse payload must be beside its metadata')
                require(0 < row['raw_bytes'] <= GROUP_LIMIT and
                        0 < row['encoded_bytes'] + GROUP.size <= ENCODED_LIMIT and
                        payload.stat().st_size == row['encoded_bytes'], 'Imported cache size outside bound')
                self.imported.setdefault((row['raw_bytes'], row['raw_sha256']), []).append((payload, row))

    def encode(self, raw):
        digest = sha256(raw)
        identity = dict(codec='raw LZMA2', preset=6, dict_size=GROUP_LIMIT,
                        raw_bytes=len(raw), raw_sha256=digest)
        key = sha256(canonical(identity))
        path = self.directory / (key + '.lzma2')
        reused = path.exists()
        if reused:
            require(0 < path.stat().st_size + GROUP.size <= ENCODED_LIMIT, 'Cached block outside bound')
        payload = path.read_bytes() if reused else None
        if payload is None:
            for imported_path, row in self.imported.get((len(raw), digest), ()):
                candidate = imported_path.read_bytes()
                require(len(candidate) == row['encoded_bytes'], 'Changed imported cache payload size')
                require(decode_lzma2(candidate, len(raw)) == raw, 'Imported cache raw bytes differ')
                payload, reused = candidate, True
                break
        if payload is None:
            payload = lzma.compress(raw, format=lzma.FORMAT_RAW,
                filters=[dict(id=lzma.FILTER_LZMA2, preset=6, dict_size=GROUP_LIMIT)])
        require(decode_lzma2(payload, len(raw)) == raw, 'Cached or compressed group byte mismatch')
        row = dict(path=str(path.resolve()), raw_bytes=len(raw), raw_sha256=digest,
                   raw_crc32=0, encoded_bytes=len(payload), encoded_sha256=sha256(payload),
                   identity=identity, cache_hit=reused)
        if not path.exists():
            atomic_bytes(path, payload)
        atomic_bytes(path.with_suffix('.json'), canonical(row) + b'\n')
        return row


def compress_plan(plan, cache, *, workers=4, progress=None):
    require(1 <= workers <= 4, 'Compression worker count must be 1..4')
    groups = []
    jobs = [(1, plan.tile_path, r) for r in plan.tile_ranges] + [(2, plan.index_path, r) for r in plan.index_ranges]
    pending = deque()
    encoded_total = 0

    def worker(kind, path, record):
        offset, length, count = record
        with path.open('rb') as handle:
            handle.seek(offset)
            raw = handle.read(length)
        require(len(raw) == length, 'Raw spool was truncated')
        return dict(cache.encode(raw), type=kind, item_count=count)

    def drain():
        nonlocal encoded_total
        groups.append(pending.popleft().result())
        encoded_total += groups[-1]['encoded_bytes'] + GROUP.size
        require(encoded_total <= FILE_LIMIT,
                'Compressed groups exceed pack limit')
        if len(groups) % 100 == 0:
            emit(progress, 'compress', complete=len(groups), total=len(jobs))

    with ThreadPoolExecutor(max_workers=workers) as executor:
        for job in jobs:
            pending.append(executor.submit(worker, *job))
            if len(pending) == 8:
                drain()
        while pending:
            drain()
    return groups


def write_pack(source, plan, groups, per_group, destination, maximum):
    """Write a real NSP2 file, including metadata and padding; legacy integrity words are unused."""
    table = source.base_end + GLOBAL.size
    prefix_end = table + len(groups) * 8
    require(prefix_end <= DIRECTORY_LIMIT, 'Extended directory exceeds resident cache')
    descriptor_base = prefix_end
    payload_start = descriptor_base + len(plan.frames) * DESCRIPTOR.size
    directory = bytearray(source.data[:source.base_end])
    directory.extend(GLOBAL.pack(b'TIL1', 1, SIDE, 14, plan.tiles, len(plan.tile_ranges),
        len(plan.index_ranges), len(plan.frames), descriptor_base, table, TILE_BYTES,
        per_group, GROUP_LIMIT, ENCODED_LIMIT, 0, 0))
    directory.extend(bytes(len(groups) * 8))
    for frame in plan.frames:
        directory.extend(DESCRIPTOR.pack(source.base_end, frame['index_group'],
                                        frame['record_offset'], frame['record_bytes']))
    require(len(directory) == payload_start, 'Descriptor layout mismatch')
    dedup = {}
    byte_counts = dict(base_directory_bytes=source.base_end, global_header_bytes=GLOBAL.size,
                      group_lookup_bytes=len(groups) * 8, descriptor_bytes=len(plan.frames) * 16,
                      retained_image_bytes=0, glyph_bytes=0, string_bytes=0, alignment_bytes=0,
                      group_header_bytes=len(groups) * GROUP.size, group_payload_bytes=0)
    cursor = payload_start

    with Path(destination).open('w+b') as out:
        out.write(directory)

        def add(offset, length, alignment, kind):
            nonlocal cursor
            payload = source.block(offset, length)
            key = (alignment, length, sha256(payload))
            for old_offset, new_offset in dedup.get(key, ()):
                if source.block(old_offset, length) == payload:
                    return new_offset
            pad = (-cursor) % alignment
            if pad:
                out.write(bytes(pad))
                cursor += pad
                byte_counts['alignment_bytes'] += pad
            new_offset = cursor
            out.write(payload)
            cursor += length
            byte_counts[kind + '_bytes'] += length
            dedup.setdefault(key, []).append((offset, new_offset))
            return new_offset

        for image_id in plan.retained:
            offset, width, height, fmt, length = source.images[image_id]
            replacement = add(offset, length, 2, 'retained_image')
            IMAGE.pack_into(directory, 40 + image_id * 16, replacement, width, height, fmt, length)
        for image_id, canonical_id in plan.mapping.items():
            _off, width, height, _fmt, _length = source.images[image_id]
            IMAGE.pack_into(directory, 40 + image_id * 16, descriptor_base + canonical_id * 16,
                            width, height, 6, 16)
        for group_id, row in enumerate(groups):
            if cursor % 2:
                out.write(b'\0')
                cursor += 1
                byte_counts['alignment_bytes'] += 1
            payload = Path(row['path']).read_bytes()
            require(len(payload) == row['encoded_bytes'], 'Encoded group size changed before pack write')
            header = GROUP.pack(b'TGR1', row['type'], group_id, row['raw_bytes'], len(payload),
                                0, row['item_count'], 0)
            struct.pack_into('<II', directory, table + group_id * 8, cursor, len(header) + len(payload))
            out.write(header)
            out.write(payload)
            cursor += len(header) + len(payload)
            byte_counts['group_payload_bytes'] += len(payload)
        for i, (cp, offset, px, w, h, advance, encoded) in enumerate(source.glyphs):
            replacement = add(offset, encoded or w * h, 1, 'glyph')
            GLYPH.pack_into(directory, source.glyph_at + i * 16, cp, replacement, px, w, h, advance, encoded)
        for i, (offset, length) in enumerate(source.strings):
            replacement = add(offset, length + 1, 1, 'string')
            struct.pack_into('<II', directory, source.string_at + i * 8, replacement, length)
        require(cursor == sum(byte_counts.values()) and out.tell() == cursor, 'Pack byte accounting mismatch')
        require(cursor <= min(maximum, FILE_LIMIT), 'Complete pack exceeds requested size limit')
        ni, ng, ns, nc, nk, _nu = source.counts
        HEADER.pack_into(directory, 0, b'NSP2', 15, REFERENCE, cursor, ni, ng, ns, nc, nk, 0)
        out.seek(0)
        out.write(directory)
        out.flush()
        os.fsync(out.fileno())
    return byte_counts


def validate_tiles(raw):
    require(len(raw) % TILE_BYTES == 0, 'Truncated tile array')
    records = np.frombuffer(raw, dtype='u1').reshape(-1, TILE_BYTES)
    require(np.all((records[:, 0] >= 1) & (records[:, 0] <= SIDE)) and
            np.all((records[:, 1] >= 1) & (records[:, 1] <= SIDE)), 'Invalid tile dimensions')
    for record in records[(records[:, 0] < SIDE) | (records[:, 1] < SIDE)]:
        w, h = int(record[0]), int(record[1])
        colors, alpha = record[2:514].reshape(SIDE, SIDE * 2), record[514:].reshape(SIDE, SIDE)
        require(not np.any(colors[h:]) and not np.any(alpha[h:]) and
                not np.any(colors[:h, w * 2:]) and not np.any(alpha[:h, w:]), 'Nonzero tile edge padding')


def index_records(raw, count, tile_count):
    records = {}
    at = 0
    for _ in range(count):
        require(at + FRAME.size <= len(raw), 'Truncated frame index header')
        size, indices, geometry = FRAME.unpack_from(raw, at)
        length = FRAME.size + indices + geometry
        require(0 < size <= IMAGE_LIMIT and 0 < indices <= 1200 and indices % 4 == 0 and
                0 < geometry <= size and at + length <= len(raw), 'Invalid frame index length')
        ids = np.frombuffer(raw, dtype='<u4', count=indices // 4, offset=at + FRAME.size)
        require(np.all(ids < tile_count), 'Tile ID outside global count')
        records[at] = length
        at += length
    require(at == len(raw), 'Index item count or trailing record bytes mismatch')
    return records


class TileReader:
    """Independent host reference decoder for a completed v15 pack."""
    def __init__(self, pack):
        require(pack.version in (15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33), 'Tiled reader requires version 15 through 33')
        self.pack = pack
        require(pack.base_end + GLOBAL.size <= len(pack.data), 'Truncated tile header')
        fields = GLOBAL.unpack_from(pack.data, pack.base_end)
        (magic, version, side, prop, self.tile_count, self.tile_groups, self.index_groups,
         self.frame_count, self.descriptors, table, tile_bytes, self.per_group,
         raw_bound, encoded_bound, flags, reserved) = fields
        require((magic, version, side, prop, tile_bytes, raw_bound, encoded_bound) ==
                (b'TIL1', 1, SIDE, 14, TILE_BYTES, GROUP_LIMIT, ENCODED_LIMIT), 'Invalid tile codec header')
        require(1 <= self.per_group <= MAX_TILES_PER_GROUP and 0 < self.tile_count <= MAX_TILES and
                self.tile_groups == (self.tile_count + self.per_group - 1) // self.per_group and
                0 < self.index_groups <= self.frame_count <= len(pack.images), 'Invalid tile counts')
        group_count = self.tile_groups + self.index_groups
        self.prefix_end = table + group_count * 8
        if pack.version >= 20:
            require(flags == self.prefix_end and reserved == len(pack.images), 'Invalid decoded-cost table')
            self.prefix_end += (len(pack.images) + 1) & ~1
        require(table == pack.base_end + GLOBAL.size and self.prefix_end <= min(len(pack.data), DIRECTORY_LIMIT)
                and self.descriptors >= self.prefix_end and self.descriptors % 2 == 0,
                'Invalid extended directory placement')
        descriptor_end = self.descriptors + self.frame_count * DESCRIPTOR.size
        require(descriptor_end <= len(pack.data), 'Truncated descriptor array')
        for off, _w, _h, fmt, length in pack.images:
            if fmt == 6:
                require(length == DESCRIPTOR.size and self.descriptors <= off <= descriptor_end - DESCRIPTOR.size
                        and (off - self.descriptors) % DESCRIPTOR.size == 0, 'Invalid image descriptor array member')
            else:
                require(off >= descriptor_end and length <= len(pack.data) - off,
                        'Image invades descriptor array')
        for _cp, off, _px, w, h, _advance, enc in pack.glyphs:
            grouped = enc & GLYPH_GROUP_FLAG
            require(off >= descriptor_end and (16 if grouped else enc or w * h) <= len(pack.data) - off,
                    'Glyph invades descriptor array')
            if grouped:
                relative, count = struct.unpack_from('<iI', pack.data, off)
                require(off + relative >= descriptor_end and count <= len(pack.data) - off - relative,
                        'Glyph group invades descriptor array')
        for off, length in pack.strings:
            grouped = length & STRING_GROUP_FLAG
            require(off >= descriptor_end and (16 if grouped else length + 1) <= len(pack.data) - off,
                    'String invades descriptor array')
            if grouped:
                relative, count = struct.unpack_from('<iI', pack.data, off)
                require(off + relative >= descriptor_end and count <= len(pack.data) - off - relative,
                        'String group invades descriptor array')
        self.groups, self.filters = [], []
        previous_end = descriptor_end
        for i in range(group_count):
            off, length = struct.unpack_from('<II', pack.data, table + i * 8)
            require(off % 2 == 0 and previous_end <= off and GROUP.size < length <= ENCODED_LIMIT
                    and off + length <= len(pack.data), 'Invalid or overlapping group range')
            header = GROUP.unpack_from(pack.data, off)
            tag, kind, number, raw_size, encoded_size, crc, count, zero = header
            filtered = tag == b'TGR2'
            require(((filtered and pack.version >= 33 and kind == 1 and zero in (1, 2)) or
                     (tag == b'TGR1' and zero == 0)) and number == i and encoded_size + GROUP.size == length
                    and 0 < raw_size <= GROUP_LIMIT, 'Invalid group header')
            if i < self.tile_groups:
                expected = min(self.per_group, self.tile_count - i * self.per_group)
                require(kind == 1 and count == expected and raw_size == count * TILE_BYTES, 'Invalid tile group shape')
            else:
                require(kind == 2 and 0 < count <= INDEX_FRAMES, 'Invalid index group item count')
            self.groups.append((off + GROUP.size, encoded_size, raw_size, crc, kind, count))
            self.filters.append(zero if filtered else 0)
            previous_end = off + length
        self.cache = OrderedDict()
        self.group_reads = self.decoded_bytes = 0

    def group(self, group_id):
        require(0 <= group_id < len(self.groups), 'Invalid group ID')
        if group_id in self.cache:
            self.cache.move_to_end(group_id)
            return self.cache[group_id]
        off, encoded_size, raw_size, crc, kind, count = self.groups[group_id]
        raw = decode_lzma2(self.pack.data[off:off + encoded_size], raw_size)
        if self.filters[group_id]:
            raw = restore_tiles(raw, self.filters[group_id])
        if kind == 1:
            validate_tiles(raw)
            starts = None
        else:
            starts = index_records(raw, count, self.tile_count)
        if len(self.cache) == HOST_CACHE_GROUPS:
            self.cache.popitem(last=False)
        self.cache[group_id] = (raw, starts)
        self.group_reads += 1
        self.decoded_bytes += len(raw)
        return raw, starts

    def image_raw(self, image_id):
        offset, width, height, fmt, length = self.pack.images[image_id]
        require(fmt == 6 and length == DESCRIPTOR.size and offset >= self.descriptors and
                (offset - self.descriptors) % DESCRIPTOR.size == 0 and
                (offset - self.descriptors) // DESCRIPTOR.size < self.frame_count, 'Invalid image descriptor')
        global_at, local_group, record_at, record_size = DESCRIPTOR.unpack_from(self.pack.data, offset)
        require(global_at == self.pack.base_end and local_group < self.index_groups, 'Invalid descriptor reference')
        group, starts = self.group(self.tile_groups + local_group)
        require(starts.get(record_at) == record_size, 'Descriptor does not name a complete index record')
        raw_size, index_size, geometry_size = FRAME.unpack_from(group, record_at)
        across, down = (width + SIDE - 1) // SIDE, (height + SIDE - 1) // SIDE
        require(index_size == across * down * 4, 'Frame tile grid size mismatch')
        ids = struct.unpack_from('<' + 'I' * (index_size // 4), group, record_at + FRAME.size)
        geometry = group[record_at + FRAME.size + index_size:record_at + record_size]
        require(len(geometry) == geometry_size, 'Truncated geometry record')
        colors = np.zeros((height, width), dtype='<u2')
        alpha = np.zeros((height, width), dtype='u1')
        positions = {}
        for position, tile_id in enumerate(ids):
            positions.setdefault(tile_id // self.per_group, []).append((position, tile_id))
        needed = sorted(positions)
        cached = [value for value in needed if value in self.cache]
        for group_id in cached + [value for value in needed if value not in self.cache]:
            tiles, _ = self.group(group_id)
            for position, tile_id in positions[group_id]:
                y, x = position // across * SIDE, position % across * SIDE
                w, h = min(SIDE, width - x), min(SIDE, height - y)
                at = (tile_id % self.per_group) * TILE_BYTES
                require(at + TILE_BYTES <= len(tiles) and tiles[at:at + 2] == bytes((w, h)),
                        'Tile reference edge dimensions mismatch')
                color = np.frombuffer(tiles, dtype='<u2', count=256, offset=at + 2).reshape(SIDE, SIDE)
                a = np.frombuffer(tiles, dtype='u1', count=256, offset=at + 514).reshape(SIDE, SIDE)
                colors[y:y + h, x:x + w] = color[:h, :w]
                alpha[y:y + h, x:x + w] = a[:h, :w]
        result = bytearray()
        at = 0
        for y in range(height):
            require(at + 2 <= len(geometry), 'Truncated geometry row')
            count, = struct.unpack_from('<H', geometry, at)
            require(count <= width, 'Excessive geometry spans')
            result.extend(geometry[at:at + 2])
            at += 2
            previous = 0
            for _ in range(count):
                require(at + 5 <= len(geometry), 'Truncated geometry span')
                x, size, mode = struct.unpack_from('<HHB', geometry, at)
                require(mode in (0, 1) and size > 0 and previous <= x and x + size <= width,
                        'Invalid geometry span')
                previous = x + size
                result.extend(geometry[at:at + 5])
                at += 5
                if mode:
                    span = np.empty((size, 3), dtype='u1')
                    span[:, 0] = colors[y, x:previous] & 255
                    span[:, 1] = colors[y, x:previous] >> 8
                    span[:, 2] = alpha[y, x:previous]
                    result.extend(span.tobytes())
                else:
                    require(np.all(alpha[y, x:previous] == 255), 'Opaque span has non-opaque tile alpha')
                    result.extend(colors[y, x:previous].tobytes())
                require(len(result) <= raw_size, 'Geometry output exceeds declared size')
        require(at == len(geometry) and len(result) == raw_size, 'Geometry output byte count mismatch')
        return width, height, 4, bytes(result)


def verify_pack(source, destination, plan, *, progress=None):
    """Complete byte proof of real encoded groups, directory aliases and payloads."""
    target = PackView(destination, source.counts[-1], versions=(15,))
    require(target.counts == source.counts, 'Directory count changed')
    require(source.data[source.clip_at:source.base_end] == target.data[target.clip_at:target.base_end],
            'Clip timing, pivots, cards or auxiliary references changed')
    reader = TileReader(target)
    for group_id in range(len(reader.groups)):
        reader.group(group_id)
    for canonical_id, frame in enumerate(plan.frames):
        original = source.image_raw(frame['image'])
        require(reader.image_raw(frame['image']) == original, 'Original sparse frame bytes changed')
        if (canonical_id + 1) % 500 == 0:
            emit(progress, 'verify_sparse_bytes', complete=canonical_id + 1, total=len(plan.frames))
    for image_id, canonical_id in plan.mapping.items():
        old = source.images[image_id]
        canonical_image = plan.frames[canonical_id]['image']
        representative = source.images[canonical_image]
        require(old == representative or source.image_raw(image_id) == source.image_raw(canonical_image),
                'Animation timing alias changed')
        require(target.images[image_id] == target.images[canonical_image], 'Output timing alias changed')
    for image_id in plan.retained:
        old, new = source.images[image_id], target.images[image_id]
        require(old[1:] == new[1:] and source.block(old[0], old[4]) == target.block(new[0], new[4]),
                'Retained image bytes changed')
    for old, new in zip(source.glyphs, target.glyphs):
        require(old[:1] + old[2:] == new[:1] + new[2:] and
                source.block(old[1], old[6] or old[3] * old[4]) ==
                target.block(new[1], new[6] or new[3] * new[4]), 'Glyph bytes changed')
    for old, new in zip(source.strings, target.strings):
        require(old[1] == new[1] and source.block(old[0], old[1] + 1) == target.block(new[0], new[1] + 1),
                'String bytes changed')
    return dict(canonical_frames_verified=len(plan.frames), animation_entries_verified=len(plan.mapping),
                retained_images_verified=len(plan.retained), glyphs_verified=len(source.glyphs),
                strings_verified=len(source.strings), all_original_fmt4_bytes_equal=True,
                all_groups_crc_and_bounds_verified=True, all_tile_padding_and_edges_verified=True,
                host_reference_group_reads=reader.group_reads, host_reference_decoded_bytes=reader.decoded_bytes,
                host_reference_cache_upper_bytes=HOST_CACHE_GROUPS * GROUP_LIMIT,
                physical_device_performance_established=False)


def repack(input_path, output_path, work_directory, *, auxiliary_count, first_clip, tiles_per_group,
           profile=None, maximum=80 * 1024 * 1024, workers=4, reuse=(), progress=None, receipt_path=None):
    """Build, verify, then publish one new output; source and existing outputs survive failures."""
    input_path, output_path, work_directory = map(Path, (input_path, output_path, work_directory))
    protect_paths(input_path, output_path, work_directory, receipt_path)
    require(input_path.resolve() != output_path.resolve(), 'Input and output must differ')
    require(not output_path.exists(), 'Refusing to overwrite an existing output')
    require(40 <= maximum <= FILE_LIMIT, 'Invalid maximum output size')
    output_path.parent.mkdir(parents=True, exist_ok=True)
    work_directory.mkdir(parents=True, exist_ok=True)
    require(shutil.disk_usage(work_directory).free >= 64 * 1024 * 1024, 'Insufficient temporary disk space')
    started = time.perf_counter()
    cache = EncodedCache(work_directory / 'encoded', reuse)
    with input_path.open('rb') as handle, mmap.mmap(handle.fileno(), 0, access=mmap.ACCESS_READ) as data:
        input_stat = os.fstat(handle.fileno())
        source_hash = sha256(data)
        source = PackView(data, auxiliary_count, profile=profile)
        with TemporaryDirectory(prefix='tile-repack-', dir=work_directory) as temporary:
            temporary = Path(temporary)
            plan = build_plan(source, temporary, first_clip, tiles_per_group, progress=progress)
            groups = compress_plan(plan, cache, workers=workers, progress=progress)
            # A same-filesystem hard link publishes the completely verified
            # file atomically without a check/replace overwrite race.
            with NamedTemporaryFile(dir=output_path.parent, prefix=output_path.name + '.', delete=False) as out:
                candidate = Path(out.name)
            try:
                byte_counts = write_pack(source, plan, groups, tiles_per_group, candidate, maximum)
                with candidate.open('rb') as output, mmap.mmap(output.fileno(), 0, access=mmap.ACCESS_READ) as encoded:
                    verified = verify_pack(source, encoded, plan, progress=progress)
                    output_hash, output_bytes = sha256(encoded), len(encoded)
                current_stat = input_path.stat()
                require((current_stat.st_dev, current_stat.st_ino, current_stat.st_size) ==
                        (input_stat.st_dev, input_stat.st_ino, input_stat.st_size),
                        'Input pathname changed during conversion')
                os.link(candidate, output_path)
            finally:
                candidate.unlink(missing_ok=True)
            receipt = dict(schema=1, complete=True, format='NSP2/15 sparse tiles',
                           input_sha256=source_hash, input_bytes=len(data), output_sha256=output_hash,
                           output_bytes=output_bytes, input_version=14, output_version=15,
                           first_tiled_clip=first_clip, tile_side=SIDE, tiles_per_group=tiles_per_group,
                           tile_groups=len(plan.tile_ranges), index_groups=len(plan.index_ranges),
                           group_raw_limit=GROUP_LIMIT, lzma2_dictionary_property=14,
                           max_group_raw_bytes=max(row['raw_bytes'] for row in groups),
                           max_group_encoded_bytes=max(row['encoded_bytes'] + GROUP.size for row in groups),
                           verified_cache_hits=sum(row['cache_hit'] for row in groups),
                           compression_workers=workers, max_inflight_groups=8,
                           raw_spool_limit=SPOOL_LIMIT, statistics=plan.stats,
                           byte_accounting=byte_counts, verification=verified,
                           no_frame_removed_resampled_or_quantized=True,
                           elapsed_seconds=time.perf_counter() - started,
                           encoder_sha256=sha256(Path(__file__).read_bytes()))
    return receipt


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True, help='Private spool and verified encoded-cache directory')
    parser.add_argument('--receipt', type=Path, required=True)
    parser.add_argument('--tiles-per-group', type=int, default=680,
                        help='1..680 (default: 680); storage/access tradeoff, never a visual change')
    parser.add_argument('--max-output-bytes', type=int, default=80 * 1024 * 1024)
    parser.add_argument('--workers', type=int, choices=range(1, 5), default=4)
    parser.add_argument('--reuse-cache', type=Path, action='append', default=[],
                        help='Directory of raw LZMA2 group payloads with adjacent checked JSON receipts')
    args = parser.parse_args(argv)
    protect_paths(args.input, args.output, args.work, args.receipt)
    from world_ui_profile import CARDS, STRINGS, CLIPS, AUXILIARY, BASE_CLIPS
    args.output = private_path(args.output, 'Output pack')
    args.work = private_path(args.work, 'Work directory')
    args.receipt = private_path(args.receipt, 'Receipt')
    require(not args.receipt.exists(), 'Refusing to overwrite an existing receipt')
    protect_paths(args.input, args.output, args.work, args.receipt)
    result = repack(args.input, args.output, args.work, auxiliary_count=AUXILIARY,
                    first_clip=BASE_CLIPS, tiles_per_group=args.tiles_per_group,
                    profile=(CARDS, STRINGS, CLIPS, AUXILIARY), maximum=args.max_output_bytes,
                    workers=args.workers, reuse=args.reuse_cache, receipt_path=args.receipt,
                    progress=lambda row: print(json.dumps(row), flush=True))
    atomic_bytes(args.receipt, canonical(result) + b'\n')
    print(json.dumps(result), flush=True)


if __name__ == '__main__':
    main()
