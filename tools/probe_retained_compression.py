"""Measure lossless storage alternatives for retained NSP2/15 payloads.

This is an offline diagnostic, not a format writer. It never changes the input
pack, never emits resource bytes, and never extrapolates a sample to a full pack.
Every compressed candidate is decoded and compared with its original bytes.
The grouping model includes an explicit 32-byte group header and 16-byte item
descriptor; a runtime format and bounded reader would still need implementation.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import lzma
import mmap
import os
from pathlib import Path
from tempfile import NamedTemporaryFile
import time

from content_tiles import (GROUP_LIMIT, PackView, private_path,
                           require, sha256)


def encoded(raw):
    payload = lzma.compress(raw, format=lzma.FORMAT_RAW,
        filters=[dict(id=lzma.FILTER_LZMA2, preset=6, dict_size=GROUP_LIMIT)])
    decoder = lzma.LZMADecompressor(format=lzma.FORMAT_RAW,
        filters=[dict(id=lzma.FILTER_LZMA2, dict_size=GROUP_LIMIT)])
    restored = decoder.decompress(payload, max_length=len(raw) + 1)
    require(restored == raw and decoder.eof and not decoder.unused_data,
            'Candidate did not restore the complete original bytes')
    return len(payload)


def aligned(size):
    return (size + 1) & ~1


def publish(path, receipt):
    path.parent.mkdir(parents=True, exist_ok=True)
    with NamedTemporaryFile(dir=path.parent, prefix=path.name + '.', delete=False) as handle:
        temporary = Path(handle.name)
        try:
            handle.write((json.dumps(receipt, indent=2, sort_keys=True) + '\n').encode())
            handle.flush()
            os.fsync(handle.fileno())
        except BaseException:
            temporary.unlink(missing_ok=True)
            raise
    try:
        os.link(temporary, path)  # Preserve an earlier or concurrently published receipt.
    finally:
        temporary.unlink(missing_ok=True)


def model(items, limit):
    """Pack whole records into independently decodable, bounded groups."""
    groups = []
    chunk = []
    size = 0
    retained = 0
    retained_items = 0

    def finish():
        if not chunk:
            return
        raw = b''.join(item['raw'] for item in chunk)
        require(len(raw) <= limit, 'Group exceeds its stated raw bound')
        payload = encoded(raw)
        # Each descriptor carries group/offset/length/format; actual group
        # indexes and any new global-format header are not implemented here.
        stored = aligned(32 + payload) + len(chunk) * 16
        previous = sum(item['stored'] for item in chunk)
        groups.append(dict(items=len(chunk), raw_bytes=len(raw),
                           payload_bytes=payload, modeled_stored_bytes=stored,
                           previous_stored_bytes=previous,
                           choose_smaller_bytes=min(previous, stored)))

    for item in items:
        if len(item['raw']) > limit:
            finish()
            chunk, size = [], 0
            retained += item['stored']
            retained_items += 1
            continue
        if chunk and size + len(item['raw']) > limit:
            finish()
            chunk, size = [], 0
        chunk.append(item)
        size += len(item['raw'])
    finish()
    return dict(raw_group_bound=limit, modeled_group_count=len(groups),
                items_retained_over_raw_bound=retained_items,
                retained_over_raw_bound_bytes=retained,
                modeled_bytes=retained + sum(x['modeled_stored_bytes'] for x in groups),
                modeled_choose_smaller_bytes=retained + sum(x['choose_smaller_bytes'] for x in groups),
                maximum_group_raw_bytes=max((x['raw_bytes'] for x in groups), default=0),
                maximum_group_payload_bytes=max((x['payload_bytes'] for x in groups), default=0),
                groups=groups)


def selected(items, count):
    if not count or count >= len(items):
        return items
    if count == 1:
        return [items[len(items) // 2]]
    indexes = sorted({i * (len(items) - 1) // (count - 1) for i in range(count)})
    return [items[i] for i in indexes]


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('pack', type=Path)
    parser.add_argument('--auxiliary-count', type=int, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--sample-images', type=int, default=0,
                        help='Uniform sample of unique retained payloads; 0 means all')
    args = parser.parse_args(argv)
    require(args.sample_images >= 0, 'Sample size must be nonnegative')
    source = private_path(args.pack, 'Input resource pack')
    output = private_path(args.output, 'Diagnostic receipt')
    require(source != output, 'Output cannot overwrite the resource pack')
    require(not output.exists(), 'Use a new receipt path to preserve previous evidence')
    started = time.monotonic()
    with source.open('rb') as handle, mmap.mmap(handle.fileno(), 0, access=mmap.ACCESS_READ) as data:
        before = os.fstat(handle.fileno())
        pack = PackView(data, args.auxiliary_count, versions=(15,))
        source_hash = hashlib.sha256(data).hexdigest()
        seen = set()
        unique = []
        retained_image_count = 0
        for image, row in enumerate(pack.images):
            off, width, height, fmt, length = row
            if fmt == 6:
                continue
            retained_image_count += 1
            # Exact payload aliases share storage even if directory geometry
            # differs. image_raw below checks each representative's geometry.
            key = (off, length)
            if key in seen:
                continue
            seen.add(key)
            unique.append((image, row))
        rows = selected(unique, args.sample_images)
        items = []
        independent_bytes = 0
        original_bytes = 0
        formats = {}
        for image, row in rows:
            _off, _width, _height, _fmt, length = row
            width, height, fmt, raw = pack.image_raw(image)
            # Eight-byte proposed local wrapper: version/format/raw length,
            # rounded to the same two-byte alignment as current payloads.
            candidate = aligned(8 + encoded(raw))
            original = aligned(length)
            independent_bytes += min(original, candidate)
            original_bytes += original
            formats[str(fmt)] = formats.get(str(fmt), 0) + 1
            items.append(dict(id=image, raw=raw, stored=original, format=fmt))
        image_groups = [model(items, limit) for limit in (131072, GROUP_LIMIT)]

        # Glyphs are an independent all-glyph experiment. Preserve existing
        # raw/RLE bytes exactly; do not change font rasterization or metrics.
        glyphs = []
        seen_glyphs = set()
        for _cp, off, _px, width, height, _advance, length in pack.glyphs:
            length = length or width * height
            if (off, length) in seen_glyphs:
                continue
            seen_glyphs.add((off, length))
            # The existing writer uses alignment=1 for glyphs, unlike images.
            glyphs.append(dict(raw=pack.block(off, length), stored=length))
        glyph_groups = [model(glyphs, limit) for limit in (32768, 131072)]

        after = source.stat()
        require((before.st_dev, before.st_ino, before.st_size, before.st_mtime_ns) ==
                (after.st_dev, after.st_ino, after.st_size, after.st_mtime_ns),
                'Input path changed during the diagnostic')

        receipt = dict(schema=1, diagnostic_only=True,
            script_sha256=sha256(Path(__file__).read_bytes()),
            decoder_sha256=sha256(Path(__file__).with_name('content_tiles.py').read_bytes()),
            input_bytes=len(data), input_sha256=source_hash,
            immutable_input=True, output_contains_resource_bytes=False,
            codec=dict(name='raw LZMA2', preset=6, dictionary_bytes=GROUP_LIMIT),
            retained_images=dict(catalog_entries=retained_image_count,
                unique_payloads=len(unique), measured_payloads=len(rows),
                complete=bool(len(rows) == len(unique)),
                sample_selection='uniform over first-use order' if len(rows) != len(unique) else 'all',
                original_aligned_payload_bytes=original_bytes,
                measured_raw_bytes=sum(len(x['raw']) for x in items), inner_formats=formats,
                independent_choose_smaller_modeled_bytes=independent_bytes,
                group_models=image_groups),
            glyphs=dict(catalog_entries=len(pack.glyphs), unique_payloads=len(glyphs),
                complete=True, original_payload_bytes=sum(x['stored'] for x in glyphs),
                group_models=glyph_groups),
            limits=['No new production pack or reader is implemented.',
                'Global group lookup/header costs are not included in these payload models.',
                'This does not estimate a full pack from a sample.',
                'Host compression and byte equality do not establish target-device speed.',
                'Group decompression changes random-access and cache costs.'],
            every_candidate_roundtrip_byte_exact=True,
            elapsed_seconds=time.monotonic() - started)
        publish(output, receipt)
        print(json.dumps({k: receipt[k] for k in ('input_sha256', 'elapsed_seconds',
              'every_candidate_roundtrip_byte_exact')}), flush=True)
        print(json.dumps(dict(retained_unique=len(unique), measured=len(rows),
            original_bytes=original_bytes, individual_modeled_bytes=independent_bytes,
            group_modeled_bytes=[x['modeled_choose_smaller_bytes'] for x in image_groups],
            glyph_original_bytes=sum(x['stored'] for x in glyphs),
            glyph_modeled_bytes=[x['modeled_choose_smaller_bytes'] for x in glyph_groups])), flush=True)


if __name__ == '__main__':
    main()
