#!/usr/bin/env python3
"""Compression-weighted bounded sample of exact local 8x8 tile dictionaries.

This is an experiment, not a pack converter or installed-size result. It
retains original group membership and all frame indices. Both the real old
payload and a same-profile control are reported, so a parameter re-encode is
not silently credited to changing tile granularity.
"""
import argparse
import json
import lzma
from pathlib import Path
import time

import numpy as np

import content_tiles as t
from ui12_resources import view
from tile_filter import filter_tiles
import subtile_dictionary as subtiles


def compress(raw):
    encoded = lzma.compress(raw, format=lzma.FORMAT_RAW, filters=[dict(
        id=lzma.FILTER_LZMA2, preset=9, dict_size=t.GROUP_LIMIT,
        nice_len=273, depth=512, lc=4, lp=0, pb=1)])
    t.require(t.decode_lzma2(encoded, len(raw)) == raw, 'Sample LZMA2 roundtrip differs')
    return (32+len(encoded)+1) & ~1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--sample', type=int, default=24)
    args = parser.parse_args()
    t.require(1 <= args.sample <= 64, 'Use a bounded sample of 1 through 64 groups')
    src = t.private_path(args.input, 'Private source'); dst = t.private_path(args.output, 'Private report')
    t.require(src != dst, 'Never overwrite original resources')
    source = view(src.read_bytes()); reader = t.TileReader(source)
    weights = np.cumsum([(reader.groups[i][1]+33) & ~1 for i in range(reader.tile_groups)])
    selected = sorted(set(int(np.searchsorted(weights, value)) for value in
                          (np.arange(args.sample)+0.5)/args.sample*weights[-1]))
    rows = []
    for group in selected:
        started = time.monotonic()
        raw, _ = reader.group(group)
        count, mode = reader.groups[group][5], reader.filters[group]
        candidate = subtiles.encode(raw, mode)
        t.require(subtiles.decode(candidate, count, mode) == raw, '8x8 dictionary changed original pixel bytes')
        actual = (reader.groups[group][1]+33) & ~1
        control = compress(filter_tiles(raw, mode) if mode else raw)
        encoded = compress(candidate) if len(candidate) <= t.GROUP_LIMIT else None
        unique = int.from_bytes(candidate[:4], 'little')
        row = dict(group=group, mode=mode, tiles=count, unique_subtiles=unique,
            existing_stored_bytes=actual, fixed_profile_16_control_bytes=control,
            candidate_raw_bytes=len(candidate), dictionary_bytes=unique*192,
            dimensions_and_four_local_ids_bytes=4+count*10,
            candidate_stored_bytes=encoded, saved_bytes=max(0,actual-encoded) if encoded else 0,
            fixed_profile_shape_saved_bytes=control-encoded if encoded else None,
            frame_index_bytes_added=0, decoded_cache_bytes_added=0,
            exact_original_tile_bytes=len(raw), seconds=time.monotonic()-started)
        rows.append(row); print(json.dumps(row), flush=True)
    total = sum(row['existing_stored_bytes'] for row in rows)
    saving = sum(row['saved_bytes'] for row in rows)
    ratios = [row['saved_bytes']/row['existing_stored_bytes'] for row in rows]
    report = dict(schema=1, diagnostic_only=True, sample='systematic cumulative physical-byte quantiles',
        input_sha256=t.sha256(source.data), groups=rows, exact_tile_bytes_compared=sum(row['exact_original_tile_bytes'] for row in rows),
        sampled_physical_bytes=total, sampled_selected_savings=saving,
        all_tile_group_physical_bytes=int(weights[-1]),
        weighted_quantile_mean_saving_fraction=sum(ratios)/len(ratios),
        estimated_all_tile_group_saving_not_measured=int(sum(ratios)/len(ratios)*int(weights[-1])),
        complete_artwork_not_converted=True, frame_indices_and_runtime_not_changed=True,
        original_independent_group_membership_preserved=True, existing_color_modes_preserved=True)
    t.atomic_bytes(dst, json.dumps(report, indent=2).encode()+b'\n')
    print(json.dumps({k:v for k,v in report.items() if k != 'groups'}), flush=True)


if __name__ == '__main__':
    main()
