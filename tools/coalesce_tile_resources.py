#!/usr/bin/env python3
"""Coalesce exact NSP2/41 tiles without adding any frame's active groups.

This changes offline references, not the pixel codec. Every mapped tile has
identical dimensions, RGB565 words (including hidden RGB), alpha and padding.
Its destination group already occurs in every active frame which used it.
Existing TGR1/TGR2/TGI2 decoders, group IDs, raw sizes and cache budgets remain
unchanged. No cross-group pointer table or extra runtime buffer is introduced.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
from collections import defaultdict
import hashlib
import json
import lzma
from pathlib import Path
import struct
import time

import numpy as np

import content_tiles as t
from compact_tile_resources import grouped_slices, SLICE
from recompress_resources import Relocation
from repack_retained import auxiliary_count
import split_content as split
import storage_parts as storage
from tile_filter import filter_tiles, restore_tiles
from tile_prediction import index_transform

ENCODER = b'UI18-exact-tile-coalescing-v2-preserve-lzma2-context-nice273-depth512'


def view(data):
    return t.PackView(data, auxiliary_count(data), versions=tuple(range(20, 43)))


def active_tiles(geometry, width, height):
    """Return the exact tile mask used by rebuild_tile_frame, with bounds."""
    columns = (width + t.SIDE - 1) // t.SIDE
    result = np.zeros(columns * ((height + t.SIDE - 1) // t.SIDE), dtype=np.bool_)
    at = 0
    for y in range(height):
        t.require(at + 2 <= len(geometry), 'Truncated frame row')
        count, = struct.unpack_from('<H', geometry, at); at += 2
        t.require(count <= width, 'Too many sparse spans')
        previous = 0
        for _ in range(count):
            t.require(at + 5 <= len(geometry), 'Truncated frame span')
            x, length, mode = struct.unpack_from('<HHB', geometry, at); at += 5
            t.require(length > 0 and mode <= 1 and previous <= x and x + length <= width,
                      'Invalid frame span')
            previous = x + length
            begin = (y // t.SIDE) * columns + x // t.SIDE
            end = (y // t.SIDE) * columns + (previous - 1) // t.SIDE + 1
            result[begin:end] = True
    t.require(at == len(geometry), 'Trailing frame geometry')
    return result


def index_inventory(source, reader):
    """Decode indices once; keep only image-backed records for IO constraints."""
    shapes = {}
    for off, width, height, fmt, _length in source.images:
        if fmt != 6:
            continue
        global_at, local, at, length = t.DESCRIPTOR.unpack_from(source.data, off)
        t.require(global_at == source.base_end and local < reader.index_groups,
                  'Invalid image tile descriptor')
        key = local, at
        shape = width, height, length
        t.require(key not in shapes or shapes[key] == shape, 'Alias changes tile dimensions')
        shapes[key] = shape
    raws, frames = {}, []
    referenced = np.zeros(reader.tile_count, dtype=np.bool_)
    for local in range(reader.index_groups):
        raw, starts = reader.group(reader.tile_groups + local)
        raws[local] = raw
        for at, length in starts.items():
            _size, index_bytes, geometry_bytes = t.FRAME.unpack_from(raw, at)
            ids = np.frombuffer(raw, dtype='<u4', count=index_bytes // 4, offset=at + 12)
            referenced[ids] = True
            if (local, at) not in shapes:
                continue
            width, height, expected = shapes[local, at]
            t.require(expected == length and len(ids) == ((width + 15) // 16) * ((height + 15) // 16),
                      'Image frame dimensions differ from indices')
            geometry = raw[at + 12 + index_bytes:at + length]
            t.require(len(geometry) == geometry_bytes, 'Image geometry length differs')
            active = ids[active_tiles(geometry, width, height)]
            frames.append((local, at, active.copy()))
    t.require(len(frames) == len(shapes), 'Image descriptor names an incomplete record')
    return raws, frames, referenced


def exact_tile_inventory(source, reader, path):
    """Build a private checked spool. Hashes locate matches; bytes prove them."""
    path.parent.mkdir(parents=True, exist_ok=True)
    raw_tiles = np.memmap(path, dtype='u1', mode='w+', shape=(reader.tile_count, t.TILE_BYTES))
    canonical = np.empty(reader.tile_count, dtype='<u4')
    seen = defaultdict(list)
    for group in range(reader.tile_groups):
        raw = reader.group(group)[0]
        start = group * reader.per_group
        records = np.frombuffer(raw, dtype='u1').reshape(-1, t.TILE_BYTES)
        raw_tiles[start:start + len(records)] = records
        for local, record in enumerate(records):
            tile = start + local
            key = hashlib.blake2b(record, digest_size=20).digest()
            original = tile
            for candidate in seen[key]:
                if np.array_equal(raw_tiles[candidate], record):
                    original = candidate
                    break
            if original == tile:
                seen[key].append(tile)
            canonical[tile] = original
        if (group + 1) % 256 == 0 or group + 1 == reader.tile_groups:
            print(json.dumps(dict(stage='exact_tile_inventory', completed=group + 1,
                                  groups=reader.tile_groups, distinct_records=len(seen))), flush=True)
    raw_tiles.flush()
    return raw_tiles, canonical


def constrained_mapping(canonical, frames, per_group):
    """Map a duplicate only to a group present in all its active users.

    The mapping is applied once. Destination slots remain occupied even if
    their own original references chose another equal slot. Thus no indirect
    lookup, cycle or relaxed group constraint can reach the runtime.
    """
    canonical = np.asarray(canonical, dtype='<u4')
    count = len(canonical)
    t.require(count > 0 and per_group > 0 and np.all(canonical < count), 'Invalid canonical tile map')
    t.require(np.all(canonical[canonical] == canonical), 'Canonical map contains a chain')
    clusters = defaultdict(list)
    for tile, original in enumerate(canonical):
        clusters[int(original)].append(tile)
    duplicate = np.zeros(count, dtype=np.bool_)
    for items in clusters.values():
        if len(items) > 1:
            duplicate[items] = True
    allowed = {}
    active_count = 0
    for _local, _at, active in frames:
        t.require(np.all(active < count), 'Active tile outside map')
        groups = set(map(int, active // per_group))
        for tile in np.unique(active[duplicate[active]]):
            tile = int(tile)
            if tile not in allowed:
                allowed[tile] = groups.copy(); active_count += 1
            else:
                allowed[tile].intersection_update(groups)
    mapping = np.arange(count, dtype='<u4')
    for items in clusters.values():
        if len(items) == 1:
            continue
        representatives = {}
        for tile in items:
            representatives.setdefault(tile // per_group, tile)
        for tile in items:
            groups = allowed.get(tile)
            if groups is None:
                mapping[tile] = items[0]
            else:
                candidates = [representatives[group] for group in groups if group in representatives]
                t.require(candidates, 'A live tile lost its original group')
                mapping[tile] = min(candidates)
    before = after = reduced = 0
    for _local, _at, active in frames:
        old = set(map(int, active // per_group))
        new = set(map(int, mapping[active] // per_group))
        t.require(new <= old, 'Coalescing adds a visible frame group')
        before += len(old); after += len(new); reduced += len(new) < len(old)
    return mapping, dict(active_duplicate_slots=active_count,
                        active_frames=len(frames), frame_group_occurrences_before=before,
                        frame_group_occurrences_after=after, frames_with_fewer_groups=reduced,
                        no_active_frame_adds_a_group=True)


def mapped_index(raw, count, tile_count, mapping):
    result = bytearray(raw)
    for at in t.index_records(raw, count, tile_count):
        _size, index_bytes, _geometry = t.FRAME.unpack_from(raw, at)
        old = np.frombuffer(raw, dtype='<u4', count=index_bytes // 4, offset=at + 12)
        result[at + 12:at + 12 + index_bytes] = mapping[old].tobytes()
    return bytes(result)


def source_lzma_profile(encoded):
    """Retain the established stream's literal context, without a sweep."""
    at=0
    while at<len(encoded):
        control=encoded[at];at+=1
        if control==0:
            return 4,0,1  # An all-literal old stream has no LZMA properties.
        if control in (1,2):
            t.require(at+2<=len(encoded),'Truncated literal LZMA2 chunk')
            size=(encoded[at]<<8|encoded[at+1])+1;at+=2
        else:
            t.require(control>=0x80 and at+4<=len(encoded),'Invalid LZMA2 control')
            size=(encoded[at+2]<<8|encoded[at+3])+1;at+=4
            if control>=0xc0:
                t.require(at<len(encoded),'Missing LZMA2 properties')
                prop=encoded[at];lc=prop%9;remaining=prop//9;lp=remaining%5;pb=remaining//5
                t.require(lc+lp<=4 and pb<=4,'Unsupported original literal context')
                return lc,lp,pb
        t.require(size<=len(encoded)-at,'LZMA2 chunk crosses stream boundary');at+=size
    raise ValueError('LZMA2 stream lacks end marker')


def encode(raw, original, reader, group, cache):
    """One established profile; no parameter search or new pixel transform."""
    header = list(t.GROUP.unpack_from(original))
    count = header[6]
    if group < reader.tile_groups:
        mode = reader.filters[group]
        transformed = filter_tiles(raw, mode) if mode else raw
    else:
        mode = 0
        transformed = index_transform(raw, count, 3, tile_count=reader.tile_count)
        header[0], header[7] = b'TGI2', 1
    lc,lp,pb=source_lzma_profile(original[32:])
    key = t.sha256(ENCODER + struct.pack('<6I', group < reader.tile_groups, mode, count,lc,lp,pb) + transformed)
    path = cache / (key + '.lzma2')
    if path.is_file():
        payload = path.read_bytes()
    else:
        payload = lzma.compress(transformed, format=lzma.FORMAT_RAW, filters=[dict(
            id=lzma.FILTER_LZMA2, preset=9, dict_size=t.GROUP_LIMIT,
            nice_len=273, depth=512, lc=lc, lp=lp, pb=pb)])
        t.atomic_bytes(path, payload)
    decoded = t.decode_lzma2(payload, len(transformed))
    t.require(decoded == transformed, 'Cached/new encoded stream differs')
    if group < reader.tile_groups and mode:
        t.require(restore_tiles(decoded, mode) == raw, 'Tile filter lost a pixel bit')
    if group >= reader.tile_groups:
        t.require(index_transform(decoded, count, 3, True, reader.tile_count) == raw,
                  'Index inverse differs')
    header[4], header[5] = len(payload), 0
    candidate = t.GROUP.pack(*header) + payload
    return candidate, candidate + bytes((len(original) - len(candidate)) & 1)


def make_plan(source, work, *, workers=2):
    t.require(source.version<=41,'Coalesce before NSP42 slot compaction')
    work = Path(work); cache = work / 'encoded'; cache.mkdir(parents=True, exist_ok=True)
    reader = t.TileReader(source)
    indices, frames, referenced = index_inventory(source, reader)
    raw_tiles, canonical = exact_tile_inventory(source, reader, work / 'original-tiles.raw')
    mapping, report = constrained_mapping(canonical, frames, reader.per_group)
    for tile in np.flatnonzero(mapping != np.arange(reader.tile_count)):
        t.require(np.array_equal(raw_tiles[tile], raw_tiles[mapping[tile]]), 'Mapped tiles differ')
    keep = np.zeros(reader.tile_count, dtype=np.bool_); keep[mapping[referenced]] = True
    changed = int(np.count_nonzero(mapping != np.arange(reader.tile_count)))
    cleared = int(np.count_nonzero(referenced & ~keep))
    report.update(tile_count=reader.tile_count, mapped_slots=changed,
                  newly_unreferenced_slots=cleared, original_referenced_slots=int(referenced.sum()))
    print(json.dumps(dict(stage='reference_plan', **report)), flush=True)

    def convert(group):
        at, length, _raw_size, _crc, _kind, count = reader.groups[group]
        original = bytes(source.data[at - 32:at + length])
        if group < reader.tile_groups:
            first = group * reader.per_group
            records = np.array(raw_tiles[first:first + count], copy=True)
            old_raw = records.tobytes()
            unused = ~keep[first:first + count]
            records[unused] = 0; records[unused, :2] = 1
            raw = records.tobytes()
        else:
            old_raw = indices[group - reader.tile_groups]
            raw = mapped_index(old_raw, count, reader.tile_count, mapping)
        if raw == old_raw:
            return group, original, original, original
        payload, physical = encode(raw, original, reader, group, cache)
        # Pixel bytes in unreferenced slots can stay when zeroing costs more.
        # Changed indices, however, must be encoded even when locally larger.
        if group < reader.tile_groups and len(physical) >= len(original):
            return group, original, original, original
        return group, original, payload, physical

    plan = {}
    with ThreadPoolExecutor(max_workers=workers) as pool:
        for number, (group, original, payload, physical) in enumerate(pool.map(convert, range(len(reader.groups))), 1):
            plan[group] = original, payload, physical
            if number % 128 == 0 or number == len(reader.groups):
                print(json.dumps(dict(stage='encode_coalesced_tiles', completed=number,
                    groups=len(reader.groups), net_bytes_saved=sum(len(a)-len(c) for a,_b,c in plan.values()))), flush=True)
    report.update(encoded_tile_groups=sum(plan[g][0] != plan[g][1] for g in range(reader.tile_groups)),
                  encoded_index_groups=sum(plan[g][0] != plan[g][1] for g in range(reader.tile_groups, len(reader.groups))),
                  aligned_bytes_saved=sum(len(a)-len(c) for a,_b,c in plan.values()),
                  resident_bytes_added=0, decoder_changed=False, group_raw_sizes_unchanged=True)
    t.require(report['aligned_bytes_saved'] > 0, 'Complete coalescing plan does not reduce storage')
    t.atomic_bytes(work / 'mapping.u32', mapping.tobytes())
    return plan, mapping, keep, raw_tiles, report


def rewrite(data, plan, *, version=41):
    """Relocate every address; pixel-group references themselves remain direct."""
    source = view(data); old = t.TileReader(source)
    t.require(set(plan) == set(range(len(old.groups))), 'Plan omits/duplicates a group')
    replacements = []
    for group, (original, payload, physical) in sorted(plan.items()):
        at, length, *_ = old.groups[group]
        t.require(data[at-32:at+length] == original, 'Locale compressed artwork differs from shared plan')
        if payload != original:
            replacements.append((at-32, len(original), payload, physical))
    relocate = Relocation(replacements)
    out, prior, lengths = bytearray(), 0, {}
    for at, length, payload, physical in replacements:
        out.extend(data[prior:at]); out.extend(physical)
        prior = at + length; lengths[at] = len(payload)
    out.extend(data[prior:])
    for i, (offset, w, h, fmt, length) in enumerate(source.images):
        t.IMAGE.pack_into(out, 40+i*16, relocate(offset), w, h, fmt, length)
    for i, (cp, offset, px, w, h, advance, encoded) in enumerate(source.glyphs):
        t.GLYPH.pack_into(out, source.glyph_at+i*16, cp, relocate(offset), px, w, h, advance, encoded)
    for i, (offset, length) in enumerate(source.strings):
        struct.pack_into('<II', out, source.string_at+i*8, relocate(offset), length)
    header = list(t.GLOBAL.unpack_from(data, source.base_end)); table = header[9]
    for field in (8, 9, 14):
        header[field] = relocate(header[field])
    t.GLOBAL.pack_into(out, source.base_end, *header)
    for i in range(len(old.groups)):
        offset, length = struct.unpack_from('<II', data, table+i*8)
        struct.pack_into('<II', out, relocate(table+i*8), relocate(offset), lengths.get(offset, length))
    for i in range(old.frame_count):
        offset = old.descriptors+i*16
        global_at, group, raw_at, size = t.DESCRIPTOR.unpack_from(data, offset)
        t.DESCRIPTOR.pack_into(out, relocate(offset), relocate(global_at), group, raw_at, size)
    for offset, (relative, length, raw_at, shape) in grouped_slices(source).items():
        SLICE.pack_into(out, relocate(offset), relocate(offset+relative)-relocate(offset), length, raw_at, shape)
    t.require(version in (41,42), 'Unsupported output tile version')
    struct.pack_into('<I', out, 4, version); struct.pack_into('<I', out, 12, len(out)); struct.pack_into('<I', out, 36, 0)
    return bytes(out), relocate


def verify(data, output, plan, mapping, keep, original_tiles, *, pixel_groups=True, compact_slots=None):
    source, target = view(data), view(output)
    old, new = t.TileReader(source), t.TileReader(target)
    t.require(source.counts == target.counts and source.clips == target.clips and
              source.cards == target.cards and source.aux == target.aux,
              'Tile coalescing changed catalog or animation clocks')
    t.require((old.tile_count, old.tile_groups, old.index_groups, old.frame_count, old.per_group) ==
              (new.tile_count, new.tile_groups, new.index_groups, new.frame_count, new.per_group),
              'Tile coalescing changed group boundaries or IDs')
    replacements = [(old.groups[g][0]-32, len(a), b, c)
                    for g,(a,b,c) in sorted(plan.items()) if a != b]
    relocate = Relocation(replacements)
    compared_tile_bytes = compared_index_bytes = 0
    for group, (a, b) in enumerate(zip(old.groups, new.groups)):
        at, size, raw_size, _crc, kind, items = a
        nat, nsize, nraw, _ncrc, nkind, nitems = b
        shape_equal = ((raw_size, kind, items) == (nraw, nkind, nitems))
        if compact_slots is not None and group in compact_slots:
            shape_equal = (kind == nkind == 1 and nitems == max(1,len(compact_slots[group]))
                           and nraw == nitems*t.TILE_BYTES and nitems <= items and target.version >= 42)
        t.require(shape_equal and nat == relocate(at-32)+32, 'Coalescing changed group shape or relocation')
        t.require(output[nat-32:nat+nsize] == plan[group][1], 'New group differs from accepted plan')
        if group < old.tile_groups:
            if pixel_groups:
                raw = new.group(group)[0]
                first = group * old.per_group
                if compact_slots is not None:
                    previous = old.group(group)[0]
                    if group in compact_slots:
                        slots = compact_slots[group]
                        previous = (np.frombuffer(previous,dtype='u1').reshape(items,t.TILE_BYTES)[slots].tobytes()
                                    if len(slots) else b'\x01\x01'+bytes(t.TILE_BYTES-2))
                    t.require(raw == previous, 'Compacted tile lost RGB, alpha, edge or padding bytes')
                    compared_tile_bytes += len(raw)
                else:
                    records = np.frombuffer(raw, dtype='u1').reshape(items, t.TILE_BYTES)
                    selected = keep[first:first+items]
                    t.require(np.array_equal(records[selected], original_tiles[first:first+items][selected]),
                              'Referenced tile lost RGB, alpha, edge or padding bytes')
                    compared_tile_bytes += int(selected.sum()) * t.TILE_BYTES
        else:
            original = old.group(group)[0]
            current = new.group(group)[0]
            t.require(current == mapped_index(original, items, old.tile_count, mapping),
                      'Frame geometry or mapped index differs')
            if compact_slots is not None:
                capacities = np.array([g[5] for g in new.groups[:new.tile_groups]],dtype='<u4')
                for record in t.index_records(current,items,new.tile_count):
                    count = t.FRAME.unpack_from(current,record)[1]//4
                    ids = np.frombuffer(current,dtype='<u4',count=count,offset=record+12)
                    t.require(np.all(ids%new.per_group < capacities[ids//new.per_group]),
                              'Frame names a removed tile slot')
            compared_index_bytes += raw_size
    for a, b in zip(source.images, target.images):
        offset, _w, _h, fmt, length = a
        t.require(b == (relocate(offset), *a[1:]), 'Image identity, dimensions or format changed')
        if fmt < 6:
            t.require(data[offset:offset+length] == output[b[0]:b[0]+length], 'Literal image changed')
        elif fmt == 6:
            d = t.DESCRIPTOR.unpack_from(data, offset)
            t.require(t.DESCRIPTOR.unpack_from(output, b[0]) == (relocate(d[0]), *d[1:]),
                      'Image descriptor changed its frame record')
    checked = set()
    for offset, (relative, length, raw_at, shape) in grouped_slices(source).items():
        group = offset+relative
        t.require(SLICE.unpack_from(output, relocate(offset)) ==
                  (relocate(group)-relocate(offset), length, raw_at, shape), 'Grouped slice changed')
        if group not in checked:
            t.require(data[group:group+length] == output[relocate(group):relocate(group)+length],
                      'Retained image/font/text bytes changed')
            checked.add(group)
    for i,(a,b) in enumerate(zip(source.glyphs, target.glyphs)):
        t.require(b == (a[0],relocate(a[1]),*a[2:]) and source.glyph_payload(i) == target.glyph_payload(i),
                  'Glyph metrics or mask changed')
    for i,(a,b) in enumerate(zip(source.strings, target.strings)):
        t.require(b == (relocate(a[0]),a[1]) and source.string_payload(i) == target.string_payload(i),
                  'Text changed')
    old_costs = t.GLOBAL.unpack_from(data, source.base_end)[14]
    new_costs = t.GLOBAL.unpack_from(output, target.base_end)[14]
    t.require(data[old_costs:old_costs+len(source.images)] == output[new_costs:new_costs+len(target.images)],
              'Image-page costs changed')
    return dict(input_bytes=len(data), output_bytes=len(output), saved_bytes=len(data)-len(output),
                input_sha256=t.sha256(data), output_sha256=t.sha256(output),
                compared_live_tile_bytes=compared_tile_bytes, compared_index_bytes=compared_index_bytes,
                glyphs=len(source.glyphs), strings=len(source.strings), images=len(source.images),
                exact_mapped_tile_records=True, exact_geometry_and_clocks=True,
                exact_glyphs_strings_and_retained=True, shared_encoded_plan_verified=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--work', required=True, type=Path)
    parser.add_argument('--workers', default=2, type=int)
    parser.add_argument('--locales', nargs='+', choices=split.LANGUAGES)
    parser.add_argument('--extra-locales', type=Path, action='append', default=[])
    args = parser.parse_args(); t.require(1 <= args.workers <= 2, 'Use one or two encoder workers')
    source = t.private_path(args.input, 'Private input'); target = t.private_path(args.output, 'Private output')
    work = t.private_path(args.work, 'Private work')
    t.require(source != target and source not in target.parents and target not in source.parents,
              'Never overwrite original resource files')
    target.mkdir(parents=True, exist_ok=True); work.mkdir(parents=True, exist_ok=True)
    common = storage.join_parts((source/'nSpire2.dat.tns').read_bytes(),
                               {name:(source/name).read_bytes() for name in storage.PART_FILES})
    inputs = {code:source/f'nSpire2.{code}.dat.tns' for code in (args.locales or split.LANGUAGES)}
    for folder in args.extra_locales:
        folder=t.private_path(folder,'Private updated locales')
        for code in inputs:
            path=folder/f'nSpire2.{code}.dat.tns'
            if path.is_file():inputs[code]=path
    t.require('eng' in inputs and all(path.is_file() for path in inputs.values()), 'Requested locale is missing')
    started = time.monotonic()
    english = split.reconstruct(common, inputs['eng'].read_bytes(), 'eng')
    plan, mapping, keep, originals, report = make_plan(view(english), work, workers=args.workers)
    report.update(schema=1, reference='v0.111.0 / 41cef1ea', version=41, locales={})
    new_common = None
    for code in ('eng', *(c for c in split.LANGUAGES if c != 'eng' and c in inputs)):
        original = english if code == 'eng' else split.reconstruct(common, inputs[code].read_bytes(), code)
        output, _ = rewrite(original, plan)
        proof = verify(original, output, plan, mapping, keep, originals, pixel_groups=code == 'eng')
        row = split.layout(output, code); artwork = output[row['prefix']:row['prefix']+row['artwork']]
        if new_common is None:
            new_common = split.common_file(artwork)
            index, parts, partition = storage.partition(new_common, output)
            t.atomic_bytes(target/'nSpire2.dat.tns', index)
            for name,payload in parts.items(): t.atomic_bytes(target/name, payload)
            report['artwork'] = partition
        else:
            t.require(new_common[64:] == artwork, 'Locales lost shared exact artwork')
        locale = split.compress_language_directory(split.language_file(output, row, code, bytes(32)))
        locale, block_report = storage.compress_blocks(locale)
        t.require(split.reconstruct(new_common, locale, code) == output, 'Split locale reconstruction differs')
        t.atomic_bytes(target/f'nSpire2.{code}.dat.tns', locale)
        if code in ('eng','zhs'): t.atomic_bytes(work/(code+'-coalesced41.nsp'), output)
        report['locales'][code] = dict(proof=proof, storage=block_report, stored_bytes=len(locale))
        print(json.dumps(dict(stage='locale_complete', locale=code, **proof, stored_bytes=len(locale))), flush=True)
    report.update(complete=True, elapsed_seconds=time.monotonic()-started,
                  installed_artwork_bytes=sum((target/name).stat().st_size for name in ('nSpire2.dat.tns',*storage.PART_FILES)),
                  installed_locale_bytes=sum((target/f'nSpire2.{code}.dat.tns').stat().st_size for code in inputs))
    t.atomic_bytes(target/'coalesced-tiles-receipt.json', json.dumps(report,indent=2).encode()+b'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ('artwork','locales')}), flush=True)


if __name__ == '__main__': main()
