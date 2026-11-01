#!/usr/bin/env python3
"""Exactly retune existing animation streams after color selection.

This keeps pixel formats, group membership, dictionaries, decoded bytes and
geometry unchanged. A chosen literal model never exceeds the source model's
context size. The calculator uses its existing decoder and allocations.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import time

import content_tiles as tiles
from tighten_ui14_resources import tighten


PROFILES = {
    'tile': ((0, 0, 1), (1, 0, 1), (2, 0, 1), (1, 1, 1)),
    'index': ((0, 0, 0), (1, 0, 0), (2, 0, 0)),
    'retained': ((0, 0, 1), (1, 0, 1), (2, 0, 1), (0, 1, 1), (1, 1, 1), (2, 0, 0)),
}


def convert(data, work, *, workers=2):
    return tighten(data, work, workers=workers, artwork_only=True, profiles=PROFILES,
                   preserve_suffix_phase=True, bounded_literal_contexts=True)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--workers', type=int, choices=(1, 2), default=2)
    args = parser.parse_args(argv)
    source = tiles.private_path(args.input, 'Private artwork input')
    target = tiles.private_path(args.output, 'Private artwork output')
    work = tiles.private_path(args.work, 'Private encoding cache')
    tiles.require(source != target, 'Never overwrite source artwork')
    started = time.monotonic()
    data = source.read_bytes()
    output, proof = convert(data, work, workers=args.workers)
    tiles.require(len(output) <= len(data), 'Exact retuning increased the complete resource')
    target.parent.mkdir(parents=True, exist_ok=True)
    tiles.atomic_bytes(target, output)
    report = dict(schema=1, complete=True, not_release_approved=True,
                  checkpoint_tested_separately=True,
                  source_sha256=tiles.sha256(data), output_sha256=tiles.sha256(output),
                  seconds=time.monotonic()-started, profiles=PROFILES, **proof)
    tiles.atomic_bytes(target.with_suffix('.json'), (json.dumps(report, indent=2)+'\n').encode())
    print(json.dumps(report), flush=True)


if __name__ == '__main__':
    main()
