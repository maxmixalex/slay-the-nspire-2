#!/usr/bin/env python3
"""Measure new within-group predictors on deterministic private sample groups."""
import argparse
import json
import lzma
from pathlib import Path
import time

import numpy as np

import content_tiles as t
from ui12_resources import view
from tile_prediction import index_transform, predict_tiles, restore_tiles


def encode(raw):
    result = lzma.compress(raw, format=lzma.FORMAT_RAW, filters=[dict(
        id=lzma.FILTER_LZMA2, preset=9, dict_size=t.GROUP_LIMIT,
        nice_len=273, depth=512, lc=4, lp=0, pb=1)])
    t.require(t.decode_lzma2(result, len(raw)) == raw, 'Predictor LZMA2 roundtrip differs')
    return len(result)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--input', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--sample', type=int, default=24)
    a = p.parse_args()
    source = view(a.input.read_bytes()); reader = t.TileReader(source)
    rows = []
    for kind in (1, 2):
        groups = list(range(reader.tile_groups)) if kind == 1 else list(range(reader.tile_groups, len(reader.groups)))
        weights = np.cumsum([reader.groups[i][1] for i in groups])
        selected = sorted(set(groups[int(np.searchsorted(weights, x))]
                              for x in np.linspace(1, weights[-1], a.sample)))
        for group in selected:
            raw, _ = reader.group(group)
            before = reader.groups[group][1]
            started = time.monotonic()
            candidates = {}
            if kind == 1:
                changed = predict_tiles(raw)
                t.require(restore_tiles(changed, len(raw) // 770) == raw, 'Predictive tiles differ')
                candidates['tile_prediction'] = encode(changed)
            else:
                count = reader.groups[group][5]
                for mode in (1, 2, 3, 5, 6, 7):
                    changed = index_transform(raw, count, mode)
                    t.require(index_transform(changed, count, mode, True) == raw, 'Predictive indices differ')
                    candidates[str(mode)] = encode(changed)
            row = dict(group=group, kind=kind, old_bytes=before, candidates=candidates,
                       saved_bytes=before-min(before, *candidates.values()),
                       seconds=time.monotonic()-started)
            rows.append(row)
            print(json.dumps(row), flush=True)
    report = dict(sample_only=True, exact_roundtrips=True, groups=rows,
                  summary={str(kind):dict(old=sum(r['old_bytes'] for r in rows if r['kind']==kind),
                                         saved=sum(r['saved_bytes'] for r in rows if r['kind']==kind))
                           for kind in (1, 2)})
    t.atomic_bytes(a.output, json.dumps(report, indent=2).encode()+b'\n')
    print(json.dumps(report['summary']), flush=True)


if __name__ == '__main__':main()
