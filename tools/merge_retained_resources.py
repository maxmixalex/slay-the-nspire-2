#!/usr/bin/env python3
"""Merge beneficial adjacent retained groups inside one existing concern span.

All raw image slices remain exact and the existing 512 KiB retained bound stays
unchanged. A selected pair becomes one RGR1/RGR2/RGR3 stream; descriptors retain
their image identity and receive the second group's raw offset. Tile/index
streams, IDs, fonts, strings and clocks are preserved. This offline transform
adds no runtime format, decoder, pool or integrity scan.
"""
from __future__ import annotations

import argparse
from bisect import bisect_right
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
import json
import lzma
from pathlib import Path
import struct
import time

import compact_retained_colors as colors
from compact_tile_resources import grouped_slices, SLICE
from coalesce_tile_resources import source_lzma_profile
from complete_retained_resources_ui17 import decoded_group
import content_tiles as t
from retained_filter import GROUP_LIMIT, group_layout, transform_group
import split_content as split
import storage_parts as storage

ENCODER = b'UI19-adjacent-retained-pairs-v1-exact-512KiB-lzma2-9-273-512'


@dataclass
class Pair:
    first: int
    hashes: tuple
    slices: tuple
    raw_sizes: tuple
    payload: bytes | None
    receipt: dict


def eligible_pairs(source, index):
    """Never move compressed bytes across a physical concern boundary."""
    t.require(source.version >= 43, 'Merge the complete NSP43 or later resources')
    t.require(index[:4] == b'NSM1' and len(index) >= 96, 'Expected a concern index')
    count = struct.unpack_from('<I', index, 24)[0]
    t.require(len(index) == 96 + count*16, 'Invalid concern index length')
    spans = [struct.unpack_from('<4I', index, 96+i*16) for i in range(count)]
    starts = [row[0] for row in spans]
    language = source.string_payload(split.MARKER)[:-1].decode('ascii')
    prefix = split.layout(source.data, language)['prefix']
    catalog = colors.groups(source)

    def span(group):
        at = 64 + group.offset - prefix
        i = bisect_right(starts, at)-1
        return i if i >= 0 and at+colors.aligned(group.size) <= spans[i][1] else -1

    pairs = []; at = 0
    while at+1 < len(catalog):
        left, right = catalog[at:at+2]
        a = group_layout(source.block(left.offset, left.size), source.version)[0]
        b = group_layout(source.block(right.offset, right.size), source.version)[0]
        owner = span(left)
        if owner >= 0 and owner == span(right) and a+b <= GROUP_LIMIT and \
                colors.complete_slices(left.slices, a) and colors.complete_slices(right.slices, b):
            pairs.append((at, left, right, a, b, spans[owner][3])); at += 2
        else:at += 1
    return catalog, pairs


def candidate(source, entry, cache, minimum_saved, minimum_percent):
    first, left, right, a, b, owner = entry
    original = tuple(bytes(source.block(g.offset, g.size)) for g in (left, right))
    slices = left.slices + tuple((at+a, shape) for at, shape in right.slices)
    table = b''.join(struct.pack('<II', *row) for row in slices)
    key = t.sha256(ENCODER + original[0] + original[1] + table)
    path = cache/(key+'.bin')
    before = sum(colors.aligned(len(p)) for p in original)
    best = None; best_size = before; chosen = None
    if path.is_file():
        proposed = path.read_bytes()
        if proposed:
            size, _start, mode, coverage = group_layout(proposed, source.version)
            t.require(size == a+b and (not mode or coverage == slices), 'Cached merged layout differs')
            # Cache entries are proposals. The complete raw-byte comparison in
            # verify() is mandatory before any common/locale output is written.
            t.require(colors.aligned(len(proposed)) < before, 'Cached merged group is not smaller')
            best = proposed; best_size = colors.aligned(len(best)); chosen = group_layout(best, source.version)[2]
    else:
        raw = b''.join(decoded_group(p, source.version) for p in original)
        t.require(len(raw) == a+b, 'Retained pair size differs')
        profiles = {(3, 0, 2)}
        for payload in original:
            profiles.add(source_lzma_profile(payload[group_layout(payload, source.version)[1]:]))
        max_sparse = max((shape & 0xffffff for _at, shape in slices if shape >> 24 == 4), default=0)
        for mode in (0, 1, 2, 8, 9, 10):
            if mode >= 8 and 32+len(table)+max_sparse > t.ENCODED_LIMIT:continue
            transformed = transform_group(raw, slices, mode) if mode else raw
            if mode:t.require(transform_group(transformed, slices, mode, True) == raw, 'Merged transform differs')
            for lc, lp, pb in sorted(profiles):
                stream = lzma.compress(transformed, format=lzma.FORMAT_RAW, filters=[dict(
                    id=lzma.FILTER_LZMA2, preset=9, dict_size=GROUP_LIMIT,
                    nice_len=273, depth=512, lc=lc, lp=lp, pb=pb)])
                size = 32 + (len(table) if mode else 0) + len(stream)
                if size > t.ENCODED_LIMIT or colors.aligned(size) >= best_size:continue
                t.require(t.decode_lzma2(stream, a+b) == transformed, 'Merged stream differs')
                tag = b'RGR3' if mode >= 8 else b'RGR2' if mode else b'RGR1'
                best = t.GROUP.pack(tag, a+b, len(stream), 14, len(slices),
                    mode-8 if mode >= 8 else mode, len(table) if mode else 0, 0) + (table if mode else b'') + stream
                best_size = colors.aligned(size); chosen = mode
        t.atomic_bytes(path, best or b'')
    saved = before-best_size
    selected = best is not None and saved >= minimum_saved and saved*100 >= before*minimum_percent
    row = dict(first_group=first, first_images=[left.first_image,right.first_image], concern=owner,
        raw_bytes=[a,b], previous_bytes=before, candidate_bytes=best_size,
        candidate_saved_bytes=saved, selected=selected, saved_bytes=saved if selected else 0, mode=chosen)
    return Pair(first, tuple(t.sha256(p) for p in original), (left.slices,right.slices),
                (a,b), best if selected else None, row)


def make_plan(source, index, cache, *, workers=2, minimum_saved=1024, minimum_percent=3):
    cache = Path(cache); cache.mkdir(parents=True, exist_ok=True)
    catalog, entries = eligible_pairs(source, index); result = []
    with ThreadPoolExecutor(max_workers=workers) as pool:
        jobs = pool.map(lambda row:candidate(source,row,cache,minimum_saved,minimum_percent), entries)
        for pair in jobs:
            result.append(pair)
            if len(result)%40 == 0 or len(result) == len(entries):
                print(json.dumps(dict(stage='merge_retained_pairs', completed=len(result), total=len(entries),
                    selected=sum(p.payload is not None for p in result),
                    saved_bytes=sum(p.receipt['saved_bytes'] for p in result))), flush=True)
    return result, len(catalog)


def rewrite(source, plan):
    groups = colors.groups(source); replacements = []; destinations = {}
    seen = set()
    for pair in plan:
        t.require(pair.first+1 < len(groups), 'Merged group index outside catalog')
        left, right = groups[pair.first:pair.first+2]
        for i, group in enumerate((left,right)):
            original = source.block(group.offset,group.size)
            t.require(t.sha256(original) == pair.hashes[i] and group.slices == pair.slices[i],
                      'Locale retained pair differs from the shared plan')
        if pair.payload is None:continue
        t.require(not ({pair.first,pair.first+1} & seen), 'Overlapping retained pairs')
        seen.update((pair.first,pair.first+1))
        for i, group in enumerate((left,right)):
            end = group.offset+group.size
            if end & 1:
                t.require(end < len(source.data) and source.data[end] == 0, 'Missing retained alignment')
                end += 1
            payload = pair.payload+bytes(len(pair.payload)&1) if i == 0 else b''
            replacements.append((group.offset,end,payload))
            destinations[group.offset] = (left.offset,len(pair.payload),0 if i == 0 else pair.raw_sizes[0])
    replacements.sort(); relocate = colors.Relocation(replacements)
    out = bytearray(); previous = 0
    for begin, end, payload in replacements:
        t.require(begin >= previous, 'Overlapping retained replacement')
        out.extend(source.data[previous:begin]); out.extend(payload); previous = end
    out.extend(source.data[previous:])
    for i,(off,w,h,fmt,length) in enumerate(source.images):
        t.IMAGE.pack_into(out,40+i*16,relocate(off),w,h,fmt,length)
    for i,(cp,off,px,w,h,advance,encoded) in enumerate(source.glyphs):
        t.GLYPH.pack_into(out,source.glyph_at+i*16,cp,relocate(off),px,w,h,advance,encoded)
    for i,(off,length) in enumerate(source.strings):
        struct.pack_into('<II',out,source.string_at+i*8,relocate(off),length)
    header = list(t.GLOBAL.unpack_from(source.data,source.base_end)); table = header[9]
    for field in (8,9,14):header[field] = relocate(header[field])
    t.GLOBAL.pack_into(out,source.base_end,*header)
    tiled = t.TileReader(source)
    for i in range(len(tiled.groups)):
        off,length = struct.unpack_from('<II',source.data,table+i*8)
        struct.pack_into('<II',out,relocate(table+i*8),relocate(off),length)
    for i in range(tiled.frame_count):
        off = tiled.descriptors+i*16
        global_at,group,raw_at,size = t.DESCRIPTOR.unpack_from(source.data,off)
        t.DESCRIPTOR.pack_into(out,relocate(off),relocate(global_at),group,raw_at,size)
    for off,(relative,length,raw_at,shape) in grouped_slices(source).items():
        group_at = off+relative
        new_group,new_length,shift = destinations.get(group_at,(group_at,length,0))
        new_off = relocate(off)
        SLICE.pack_into(out,new_off,relocate(new_group)-new_off,new_length,raw_at+shift,shape)
    struct.pack_into('<I',out,12,len(out)); struct.pack_into('<I',out,36,0)
    mapping = {g.offset:(relocate(destinations.get(g.offset,(g.offset,g.size,0))[0]),
        destinations.get(g.offset,(g.offset,g.size,0))[1],destinations.get(g.offset,(g.offset,g.size,0))[2])
        for g in groups}
    return bytes(out), mapping


def verify(source, converted, plan, mapping, *, decode_groups):
    target = colors.view(converted)
    t.require(source.version == target.version and source.counts == target.counts and
        source.clips == target.clips and source.cards == target.cards and source.aux == target.aux,
        'Merged catalog or clocks changed')
    a,b = t.TileReader(source),t.TileReader(target)
    t.require((a.tile_count,a.frame_count,a.per_group,len(a.groups)) ==
              (b.tile_count,b.frame_count,b.per_group,len(b.groups)), 'Tile identities changed')
    for old,new in zip(a.groups,b.groups):
        at,length = old[:2]; nt,nlength = new[:2]
        t.require(old[1:] == new[1:] and source.data[at-32:at+length] == converted[nt-32:nt+nlength],
                  'Compressed tile/index bytes changed')
    old_groups,new_groups = colors.groups(source),colors.groups(target)
    selected = [p for p in plan if p.payload is not None]
    t.require(len(new_groups) == len(old_groups)-len(selected), 'Merged group count differs')
    expected = {g.offset:bytes(source.block(g.offset,g.size)) for g in old_groups}
    for pair in selected:
        for g in old_groups[pair.first:pair.first+2]:expected[g.offset] = pair.payload
    extents = {}; raw_bytes = 0; last_at = None; last_raw = None
    for group in old_groups:
        at,size,shift = mapping[group.offset]; payload = bytes(target.block(at,size))
        t.require(payload == expected[group.offset], 'Selected merged stream differs')
        old_size = group_layout(source.block(group.offset,group.size),source.version)[0]
        new_size = group_layout(payload,target.version)[0]
        extents.setdefault(at,[]).append((shift,old_size,new_size))
        if decode_groups:
            old_raw = decoded_group(source.block(group.offset,group.size),source.version)
            if last_at != at:last_raw = decoded_group(payload,target.version); last_at = at
            t.require(last_raw[shift:shift+len(old_raw)] == old_raw, 'Complete retained raw bytes changed')
            raw_bytes += len(old_raw)
    t.require(set(extents) == {g.offset for g in new_groups}, 'Unreferenced merged group')
    for intervals in extents.values():
        end = 0
        for at,size,total in sorted(intervals):
            t.require(at == end and size <= total-at, 'Merged raw coverage overlaps or has gaps'); end += size
        t.require(end == total, 'Merged raw bytes left over')
    for old,new in zip(source.images,target.images):
        off,w,h,fmt,size = old; noff = new[0]
        t.require(old[1:] == new[1:], 'Merged image shape changed')
        if fmt == 7:
            relative,length,raw_at,shape = SLICE.unpack_from(source.data,off)
            nrelative,nlength,nraw,nshape = SLICE.unpack_from(converted,noff)
            at,expected_length,shift = mapping[off+relative]
            t.require((noff+nrelative,nlength,nraw,nshape) == (at,expected_length,raw_at+shift,shape),
                      'Merged image addresses the wrong raw slice')
        else:t.require(source.block(off,size) == target.block(noff,size), 'Literal image/frame descriptor changed')
    old_cost = t.GLOBAL.unpack_from(source.data,source.base_end)[14]
    new_cost = t.GLOBAL.unpack_from(converted,target.base_end)[14]
    t.require(source.data[old_cost:old_cost+len(source.images)] == converted[new_cost:new_cost+len(target.images)],
              'Decoded image cost changed')
    for i,(old,new) in enumerate(zip(source.glyphs,target.glyphs)):
        t.require(old[:1]+old[2:] == new[:1]+new[2:] and source.glyph_payload(i) == target.glyph_payload(i),
                  'Merged glyph changed')
    for i,(old,new) in enumerate(zip(source.strings,target.strings)):
        t.require(old[1] == new[1] and source.string_payload(i) == target.string_payload(i), 'Merged string changed')
    return dict(images=len(source.images),glyphs=len(source.glyphs),strings=len(source.strings),clips=len(source.clips),
        original_retained_groups=len(old_groups),output_retained_groups=len(new_groups),merged_pairs=len(selected),
        complete_raw_groups_compared=len(old_groups) if decode_groups else 0,complete_raw_bytes_compared=raw_bytes,
        tile_and_index_groups_unchanged=len(a.groups),all_image_slices_glyphs_strings_clocks_costs_equal=True,
        input_bytes=len(source.data),output_bytes=len(converted),saved_bytes=len(source.data)-len(converted),
        input_sha256=t.sha256(source.data),output_sha256=t.sha256(converted))


def partition_relocated(index, source, converted, plan):
    """Map the actual replaced byte spans, including descriptors between groups.

    Two old encoded groups can now alias one new group. Mapping only semantic
    group endpoints can move intervening descriptor bytes to another concern;
    the exact replacement deltas preserve the existing physical ownership.
    """
    groups = colors.groups(source); replacements = []
    for pair in plan:
        if pair.payload is None:continue
        for i, group in enumerate(groups[pair.first:pair.first+2]):
            payload = pair.payload+bytes(len(pair.payload)&1) if i == 0 else b''
            replacements.append((group.offset,group.offset+colors.aligned(group.size),payload))
    replacements.sort(); relocate = colors.Relocation(replacements)
    language = source.string_payload(split.MARKER)[:-1].decode('ascii')
    before, after = split.layout(source.data,language),split.layout(converted,language)
    old_prefix, prefix = before['prefix'],after['prefix']
    common = split.common_file(converted[prefix:prefix+after['artwork']])
    count = struct.unpack_from('<I',index,24)[0]
    t.require(index[:4] == b'NSM1' and len(index) == 96+count*16, 'Invalid concern index')
    files = [bytearray(32) for _ in storage.NAMES]; rows = []
    for i in range(count):
        first,last,_physical,part = struct.unpack_from('<4I',index,96+i*16)
        start,end = relocate(old_prefix+first-64),relocate(old_prefix+last-64)
        t.require(prefix <= start < end <= prefix+after['artwork'] and part < len(files),
                  'Merged concern leaves its artwork')
        at = len(files[part]); files[part].extend(converted[start:end])
        rows.append((64+start-prefix,64+end-prefix,at,part))
    for i,payload in enumerate(files):
        struct.pack_into('<4s7I',payload,0,b'NSS1',1,t.REFERENCE,32,len(payload),i,0,0)
    new_index = struct.pack('<4s7I',b'NSM1',1,t.REFERENCE,96,96+count*16,len(common),count,len(files))+common[:64]
    new_index += b''.join(struct.pack('<4I',*row) for row in rows)
    parts = {name:bytes(payload) for name,payload in zip(storage.PART_FILES,files)}
    t.require(storage.join_parts(new_index,parts) == common,'Relocated concerns changed artwork bytes')
    receipt = dict(spans=count,virtual_bytes=len(common),index_bytes=len(new_index),
        installed_bytes=len(new_index)+sum(len(v) for v in parts.values()),
        original_concern_ownership_preserved=True,physical_replacement_boundaries=True,
        complete_byte_identical_roundtrip=True,
        files={n:dict(bytes=len(v),sha256=t.sha256(v)) for n,v in parts.items()})
    return common,new_index,parts,receipt


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('input','output','work'):p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--workers',type=int,default=2)
    p.add_argument('--minimum-saved',type=int,default=1024)
    p.add_argument('--minimum-percent',type=float,default=3)
    p.add_argument('--locales',nargs='+',choices=split.LANGUAGES)
    args = p.parse_args()
    t.require(1 <= args.workers <= 4 and args.minimum_saved >= 1 and 0 <= args.minimum_percent <= 100,
              'Invalid retained merge bounds')
    source = t.private_path(args.input,'Private input'); target = t.private_path(args.output,'Private output')
    work = t.private_path(args.work,'Private work')
    t.require(source != target and source not in target.parents and target not in source.parents,
              'Never overwrite source resources')
    locales = args.locales or list(split.LANGUAGES)
    t.require('eng' in locales and len(set(locales)) == len(locales) and
        all((source/f'nSpire2.{c}.dat.tns').is_file() for c in locales), 'Missing or repeated locales')
    target.mkdir(parents=True,exist_ok=True); work.mkdir(parents=True,exist_ok=True)
    index = (source/'nSpire2.dat.tns').read_bytes()
    old_common = storage.join_parts(index,{n:(source/n).read_bytes() for n in storage.PART_FILES})
    english = split.reconstruct(old_common,(source/'nSpire2.eng.dat.tns').read_bytes(),'eng')
    initial = colors.view(english); started = time.monotonic()
    plan, group_count = make_plan(initial,index,work/'encoded',workers=args.workers,
        minimum_saved=args.minimum_saved,minimum_percent=args.minimum_percent)
    report = dict(schema=1,reference='v0.111.0 / 41cef1ea',version=initial.version,
        raw_group_limit=GROUP_LIMIT,minimum_saved=args.minimum_saved,minimum_percent=args.minimum_percent,
        original_groups=group_count,eligible_pairs=len(plan),merged_pairs=sum(p.payload is not None for p in plan),
        previous_installed_bytes=sum((source/n).stat().st_size for n in ('nSpire2.dat.tns',*storage.PART_FILES,
            *(f'nSpire2.{c}.dat.tns' for c in locales))),runtime_pools_added=0,
        results=[p.receipt for p in plan],locales={})
    common = None
    for code in ('eng',*(c for c in split.LANGUAGES if c != 'eng' and c in locales)):
        before = english if code == 'eng' else split.reconstruct(old_common,(source/f'nSpire2.{code}.dat.tns').read_bytes(),code)
        old = initial if code == 'eng' else colors.view(before)
        converted,mapping = rewrite(old,plan)
        proof = verify(old,converted,plan,mapping,decode_groups=code == 'eng')
        row = split.layout(converted,code); artwork = converted[row['prefix']:row['prefix']+row['artwork']]
        if common is None:
            common,new_index,parts,partition = partition_relocated(index,initial,converted,plan)
            t.atomic_bytes(target/'nSpire2.dat.tns',new_index)
            for name,payload in parts.items():t.atomic_bytes(target/name,payload)
            report['artwork'] = partition
        else:t.require(common[64:] == artwork,'Merged locale artwork differs')
        locale = split.compress_language_directory(split.language_file(converted,row,code,bytes(32)))
        locale,compression = storage.compress_blocks(locale)
        t.require(split.reconstruct(common,locale,code) == converted,'Merged installation roundtrip differs')
        t.atomic_bytes(target/f'nSpire2.{code}.dat.tns',locale)
        if code in ('eng','zhs'):t.atomic_bytes(work/(code+'-merged.nsp'),converted)
        report['locales'][code] = dict(proof=proof,storage=compression,stored_bytes=len(locale))
        print(json.dumps(dict(stage='merged_locale_complete',locale=code,stored_bytes=len(locale),**proof)),flush=True)
    report.update(complete=True,elapsed_seconds=time.monotonic()-started,
        installed_bytes=sum((target/n).stat().st_size for n in ('nSpire2.dat.tns',*storage.PART_FILES,
            *(f'nSpire2.{c}.dat.tns' for c in locales))))
    report['saved_installed_bytes'] = report['previous_installed_bytes']-report['installed_bytes']
    t.atomic_bytes(target/'retained-merge-receipt.json',json.dumps(report,indent=2).encode()+b'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ('results','locales','artwork')}),flush=True)


if __name__ == '__main__':main()
