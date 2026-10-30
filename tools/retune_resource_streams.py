#!/usr/bin/env python3
"""Select smaller exact LZMA2 streams without changing resource working sets.

NSP2 tile, index and retained groups keep their complete decoder bytes, filters,
IDs, raw sizes, geometry and preparation costs. More suitable literal contexts
are selected offline after group consolidation. No runtime codec, pool, scan or
per-frame work is added. All locale outputs share the same verified artwork.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import time

import coalesce_tile_resources as coalesce
from compact_tile_slots import partition_like
import content_tiles as t
import split_content as split
import storage_parts as storage
from tighten_ui14_resources import tighten


PROFILES = {
    'tile': ((0, 1, 1), (2, 0, 1), (2, 0, 2), (2, 1, 1), (4, 0, 1), (3, 1, 1)),
    'index': ((0, 1, 0), (2, 0, 0), (3, 1, 0), (4, 0, 0), (4, 0, 1)),
    'retained': ((0, 1, 1), (2, 0, 1), (2, 0, 0), (4, 0, 1)),
    'font': ((0, 0, 0), (2, 0, 0), (3, 0, 0), (4, 0, 0), (4, 0, 1)),
    'text': ((0, 0, 0), (2, 0, 0), (3, 0, 0), (4, 0, 0)),
}


def convert(source, target, work, *, workers=2, locales=None, artwork_only=False):
    source = t.private_path(source, 'Private source')
    target = t.private_path(target, 'Private output')
    work = t.private_path(work, 'Private work')
    t.require(source != target and source not in target.parents and target not in source.parents,
              'Never overwrite source resources')
    t.require(1 <= workers <= 4, 'Use one through four bounded workers')
    codes = list(locales or split.LANGUAGES)
    t.require('eng' in codes and len(set(codes)) == len(codes), 'English is required; locales must be unique')
    inputs = {code: source / f'nSpire2.{code}.dat.tns' for code in codes}
    t.require(all(path.is_file() for path in inputs.values()), 'Requested locale is missing')
    target.mkdir(parents=True, exist_ok=True)
    work.mkdir(parents=True, exist_ok=True)
    index = (source / 'nSpire2.dat.tns').read_bytes()
    old_common = storage.join_parts(index, {name:(source/name).read_bytes() for name in storage.PART_FILES})
    english = split.reconstruct(old_common, inputs['eng'].read_bytes(), 'eng')
    started = time.monotonic()
    report = dict(schema=1, reference='v0.111.0 / 41cef1ea', version=coalesce.view(english).version,
                  runtime_memory_added=0, runtime_code_changes=0, profiles=PROFILES,
                  all_group_boundaries_and_decoder_bytes_preserved=True, locales={})
    common = None
    for code in ('eng', *(code for code in codes if code != 'eng')):
        original = english if code == 'eng' else split.reconstruct(old_common, inputs[code].read_bytes(), code)
        result, proof = tighten(original, work/'encoded', workers=workers, artwork_only=artwork_only,
                                profiles=PROFILES, preserve_suffix_phase=True)
        bounds = split.layout(result, code)
        artwork = result[bounds['prefix']:bounds['prefix']+bounds['artwork']]
        if common is None:
            common, new_index, parts, partition = partition_like(index, english, result)
            t.atomic_bytes(target/'nSpire2.dat.tns', new_index)
            for name, payload in parts.items():
                t.atomic_bytes(target/name, payload)
            report['artwork'] = partition
            t.atomic_bytes(work/'eng-retuned.nsp', result)
        else:
            t.require(common[64:] == artwork, 'Locales no longer share identical artwork')
        language = split.compress_language_directory(split.language_file(result, bounds, code, bytes(32)))
        language, details = storage.compress_blocks(language)
        t.require(split.reconstruct(common, language, code) == result, 'Retuned locale reconstruction differs')
        t.atomic_bytes(target/f'nSpire2.{code}.dat.tns', language)
        report['locales'][code] = dict(proof=proof, storage=details, stored_bytes=len(language))
        print(json.dumps(dict(stage='retuned_locale', locale=code, stored_bytes=len(language), **proof)), flush=True)
    names = ('nSpire2.dat.tns', *storage.PART_FILES, *(f'nSpire2.{code}.dat.tns' for code in codes))
    before = sum((source/name).stat().st_size for name in names)
    after = sum((target/name).stat().st_size for name in names)
    report.update(complete=True, installed_bytes_before=before, installed_bytes_after=after,
                  installed_bytes_saved=before-after, elapsed_seconds=time.monotonic()-started)
    t.atomic_bytes(target/'retuned-streams-receipt.json', json.dumps(report, indent=2).encode()+b'\n')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--workers', type=int, default=2)
    parser.add_argument('--locales', nargs='+', choices=split.LANGUAGES)
    parser.add_argument('--artwork-only', action='store_true', help='Keep previous font/text encoded streams')
    args = parser.parse_args()
    report = convert(args.input, args.output, args.work, workers=args.workers, locales=args.locales,
                     artwork_only=args.artwork_only)
    print(json.dumps({k:v for k,v in report.items() if k not in ('artwork', 'locales')}), flush=True)


if __name__ == '__main__':
    main()
