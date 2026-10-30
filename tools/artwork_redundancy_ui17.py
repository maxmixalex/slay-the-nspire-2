#!/usr/bin/env python3
"""Account for exact artwork redundancy without modifying a resource pack.

This is a converter planning aid, not a shipped compression stage. The private
output contains the old logical stream and an exact tile spool for subsequent
bounded experiments. Hash matches are followed by actual record comparisons.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct
import time

import numpy as np

import content_tiles as t
from reclaim_unused_tiles_ui14 import reachable_tiles
from storage_parts import PART_FILES, join_parts
from split_content import reconstruct
from ui12_resources import view


def audit(folder, output):
    output.mkdir(parents=True, exist_ok=True)
    common = join_parts((folder / 'nSpire2.dat.tns').read_bytes(),
                        {name: (folder / name).read_bytes() for name in PART_FILES})
    logical = reconstruct(common, (folder / 'nSpire2.eng.dat.tns').read_bytes(), 'eng')
    t.atomic_bytes(output / 'baseline-eng.nsp', logical)
    t.atomic_bytes(output / 'baseline-common.nsc', common)
    source = view(logical)
    reader = t.TileReader(source)
    live = reachable_tiles(source)
    rows, seen, counts = [], {}, Counter()
    canonical = np.empty(reader.tile_count, dtype='<u4')
    categories = Counter()
    for offset, width, height, fmt, length in source.images:
        categories[fmt] += 1
    raw_path = output / 'baseline-tiles.raw'
    with raw_path.open('w+b', buffering=0) as spool:
        for group in range(reader.tile_groups):
            raw = reader.group(group)[0]
            start = group * reader.per_group
            before = len(seen)
            duplicates = local_duplicates = live_duplicates = 0
            for index in range(len(raw) // t.TILE_BYTES):
                tile_id = start + index
                record = raw[index * t.TILE_BYTES:(index + 1) * t.TILE_BYTES]
                digest = hashlib.blake2b(record, digest_size=20).digest()
                previous = None
                for candidate in seen.get(digest, ()):
                    spool.seek(candidate * t.TILE_BYTES)
                    if spool.read(t.TILE_BYTES) == record:
                        previous = candidate
                        break
                if previous is None:
                    seen.setdefault(digest, []).append(tile_id)
                    previous = tile_id
                else:
                    duplicates += 1
                    local_duplicates += previous >= start
                    live_duplicates += bool(live[tile_id])
                canonical[tile_id] = previous
                spool.seek(tile_id * t.TILE_BYTES)
                spool.write(record)
            _at, encoded, size, _crc, _kind, items = reader.groups[group]
            rows.append(dict(group=group, items=items, raw_bytes=size,
                             encoded_bytes=encoded + 32, filter=reader.filters[group],
                             unique_new=len(seen) - before, duplicates=duplicates,
                             same_group_duplicates=local_duplicates,
                             live_duplicates=live_duplicates,
                             live_tiles=int(np.count_nonzero(live[start:start + items]))))
            counts.update(duplicates=duplicates, same_group_duplicates=local_duplicates,
                          live_duplicates=live_duplicates, encoded_bytes=encoded + 32)
            if (group + 1) % 128 == 0:
                print(json.dumps(dict(stage='exact_tiles', completed=group + 1,
                                      groups=reader.tile_groups, **counts)), flush=True)
    t.atomic_bytes(output / 'baseline-canonical-tiles.u32', canonical.tobytes())
    retained = {}
    literals = {}
    for offset, _w, _h, fmt, length in source.images:
        if fmt == 7:
            relative, size, _raw_at, _shape = struct.unpack_from('<iIII', logical, offset)
            retained[offset + relative] = size
        elif fmt != 6:
            literals[(offset, length)] = length
    report = dict(schema=1, input_files={name: hashlib.sha256((folder / name).read_bytes()).hexdigest()
                                        for name in ('nSpire2.dat.tns', *PART_FILES, 'nSpire2.eng.dat.tns')},
                  logical_bytes=len(logical), common_bytes=len(common), images=len(source.images),
                  image_formats=dict(categories), tiles=reader.tile_count,
                  tile_groups=reader.tile_groups, index_groups=reader.index_groups,
                  raw_tile_bytes=reader.tile_count * t.TILE_BYTES,
                  unique_tiles=len(seen), live_tiles=int(np.count_nonzero(live)),
                  live_canonical_tiles=len(np.unique(canonical[live])),
                  tile_totals=dict(counts),
                  index_encoded_bytes=sum(row[1] + 32 for row in reader.groups[reader.tile_groups:]),
                  retained_encoded_bytes=sum(retained.values()),
                  literal_encoded_bytes=sum(literals.values()), groups=rows,
                  complete_exact_duplicate_comparison=True,
                  runtime_changed=False, production_output=False)
    t.atomic_bytes(output / 'artwork-redundancy.json', json.dumps(report, indent=2).encode() + b'\n')
    print(json.dumps({k: v for k, v in report.items() if k not in ('groups', 'input_files')}), flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    data = t.private_path(args.data, 'Existing private resource directory')
    output = t.private_path(args.output, 'Private analysis directory')
    started = time.monotonic()
    audit(data, output)
    print(json.dumps(dict(elapsed_seconds=time.monotonic() - started)), flush=True)


if __name__ == '__main__':
    main()
