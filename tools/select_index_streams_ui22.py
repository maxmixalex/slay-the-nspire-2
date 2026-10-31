#!/usr/bin/env python3
"""Choose smaller existing TGI2 index streams for exact-color NSP2 resources.

Only plain index groups are eligible. Tile pixels, group boundaries, frame
geometry, clocks, cache costs, retained slices, fonts and text remain exact.
The original stream is always a candidate; encoded size alone is not a frame
performance claim. Use the release ARM/whole-frame gates before adoption.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import lzma
from pathlib import Path
import struct
import time

import numpy as np

import coalesce_tile_resources as coalesce
import content_tiles as t
from tile_prediction import index_transform

PROFILES = ((2, 0, 0), (3, 1, 0), (4, 0, 0), (4, 0, 1))
ENCODER = b'ui22-existing-tgi2-v1-nice273-depth512'


def choose_index(original, dictionary, tile_count, cache, *, minimum_percent=0):
    header = list(t.GROUP.unpack_from(original))
    t.require(header[0] == b'TGR1' and header[1] == 2 and header[7] == 0 and
              header[4] == len(original)-32, 'Expected one complete plain index group')
    raw = t.decode_lzma2(original[32:], header[3], dictionary)
    transformed = index_transform(raw, header[6], 3, tile_count=tile_count)
    t.require(index_transform(transformed, header[6], 3, True, tile_count) == raw,
              'TGI2 forward/inverse changed an index or geometry byte')
    source_profile = coalesce.source_lzma_profile(original[32:])
    profiles = tuple(dict.fromkeys((source_profile, *PROFILES)))
    key = t.sha256(ENCODER + struct.pack('<II', dictionary, tile_count) +
                   bytes(x for profile in profiles for x in profile) + original)
    path = cache/(key+'.bin')
    best = original
    if path.is_file():
        candidate = path.read_bytes()
        ch = list(t.GROUP.unpack_from(candidate))
        shape = header.copy();shape[0], shape[4], shape[7] = ch[0], ch[4], ch[7]
        t.require(ch == shape and ch[0] in (b'TGR1', b'TGI2') and
                  ch[7] == (1 if ch[0] == b'TGI2' else 0) and ch[4] == len(candidate)-32,
                  'Cached index header changed its bounds or identity')
        decoded = t.decode_lzma2(candidate[32:], header[3], dictionary)
        if ch[0] == b'TGI2':
            decoded = index_transform(decoded, header[6], 3, True, tile_count)
        t.require(decoded == raw, 'Cached index stream changed original bytes')
        if len(candidate) < len(best):
            best = candidate
    else:
        for lc, lp, pb in profiles:
            encoded = lzma.compress(transformed, format=lzma.FORMAT_RAW, filters=[dict(
                id=lzma.FILTER_LZMA2, preset=9, dict_size=dictionary,
                nice_len=273, depth=512, lc=lc, lp=lp, pb=pb)])
            if len(encoded)+32 >= len(best):
                continue
            t.require(t.decode_lzma2(encoded, len(transformed), dictionary) == transformed,
                      'Encoded TGI2 bytes differ')
            ch = header.copy();ch[0], ch[4], ch[7] = b'TGI2', len(encoded), 1
            best = t.GROUP.pack(*ch)+encoded
        t.atomic_bytes(path, best)
    physical = best+bytes((len(original)-len(best)) & 1)
    if len(physical) >= len(original) or (len(original)-len(physical))*100 < len(original)*minimum_percent:
        best = physical = original
    return best, physical, dict(group=header[2], records=header[6], raw_bytes=header[3],
        source_bytes=len(original), candidate_bytes=len(best), physical_bytes=len(physical),
        saved_bytes=len(original)-len(physical), source_profile=source_profile,
        candidate_profile=coalesce.source_lzma_profile(best[32:]),
        predicted=best[:4] == b'TGI2', source_sha256=t.sha256(original),
        candidate_sha256=t.sha256(best), raw_sha256=t.sha256(raw))


def convert_bytes(data, cache, *, workers=2, groups=None, minimum_percent=0, align_suffix=True):
    t.require(1 <= workers <= 2 and 0 <= minimum_percent <= 100,
              'Use at most two workers and a bounded minimum saving')
    source = coalesce.view(data)
    t.require(41 <= source.version <= 45, 'TGI2 requires an existing NSP2/41..45 pack')
    old = t.TileReader(source);cache = Path(cache);cache.mkdir(parents=True, exist_ok=True)
    wanted = set(range(old.tile_groups, len(old.groups))) if groups is None else set(groups)
    t.require(all(old.tile_groups <= g < len(old.groups) for g in wanted),
              'Only existing global index-group IDs may be selected')
    eligible = sorted(g for g in wanted if not old.predictors[g])
    originals = {g:bytes(data[at-32:at+size]) for g,(at,size,*_) in enumerate(old.groups)}
    plan = {g:(raw,raw,raw) for g,raw in originals.items()}

    def one(group):
        payload, physical, receipt = choose_index(originals[group], old.dictionary,
            old.tile_count, cache, minimum_percent=minimum_percent)
        return group, payload, physical, receipt

    rows = []
    with ThreadPoolExecutor(max_workers=workers) as pool:
        # Tasks carry only IDs and compressed bytes. Each worker decodes one
        # bounded index group; no all-catalog raw job list is materialized.
        for group,payload,physical,row in pool.map(one, eligible):
            plan[group] = originals[group], payload, physical;rows.append(row)
            if len(rows) % 64 == 0 or len(rows) == len(eligible):
                print(json.dumps(dict(stage='select_tgi2', completed=len(rows), total=len(eligible),
                    saved_bytes=sum(r['saved_bytes'] for r in rows))), flush=True)
    changed = [g for g in eligible if plan[g][0] != plan[g][1]]
    saved = sum(len(a)-len(c) for a,_b,c in plan.values());padding = 0
    if align_suffix and changed:
        padding = saved % 4096
        group = changed[-1];a,b,c = plan[group];plan[group] = a,b,c+bytes(padding)
    output = coalesce.rewrite(data, plan, version=source.version)[0] if changed else data
    # No pixel stream may change while indexes are being selected.
    for group in range(old.tile_groups):
        t.require(plan[group][0] == plan[group][1] == plan[group][2], 'Pixel stream changed')
    proof = coalesce.verify(data, output, plan, np.arange(old.tile_count, dtype='<u4'),
                            None, None, pixel_groups=False)
    return output, dict(complete=True, runtime_format_added=False, resident_bytes_added=0,
        original_stream_is_fallback=True, all_tile_streams_byte_identical=True,
        frames_and_group_working_sets_unchanged=True, index_groups_considered=len(eligible),
        index_groups_changed=len(changed), source_profiles_preserved_for_untouched_groups=True,
        suffix_alignment_padding=padding, minimum_saving_percent=minimum_percent,
        groups=rows, verification=proof)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--workers', type=int, default=2)
    parser.add_argument('--groups', nargs='+', type=int)
    parser.add_argument('--minimum-saving-percent', type=float, default=0)
    args = parser.parse_args()
    source=t.private_path(args.input, 'Private source');target=t.private_path(args.output, 'Private output')
    work=t.private_path(args.work, 'Private cache')
    t.require(source != target, 'Never overwrite the source pack')
    t.HOST_CACHE_GROUPS=32;started=time.monotonic()
    output, proof = convert_bytes(source.read_bytes(), work/'encoded', workers=args.workers,
        groups=args.groups, minimum_percent=args.minimum_saving_percent)
    t.atomic_bytes(target, output)
    proof.update(input=str(source), output=str(target), elapsed_seconds=time.monotonic()-started,
                 cache_groups=32, workers=args.workers)
    t.atomic_bytes(target.with_suffix('.json'), json.dumps(proof, indent=2).encode()+b'\n')
    print(json.dumps({k:v for k,v in proof.items() if k != 'groups'}), flush=True)


if __name__ == '__main__':
    main()
