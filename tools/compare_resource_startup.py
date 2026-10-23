#!/usr/bin/env python3
"""Same-input split-resource startup comparison after a saved checkpoint.

The caller builds one benchmark object and links it to each frozen core library.
File hashes warm host filesystem caches before measured fresh-process opens.
No physical-device or cold-cache performance inference is made here.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import statistics
import subprocess
from datetime import datetime, timezone
from pathlib import Path


def identity(path: Path) -> dict:
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return {'path': str(path.resolve()), 'bytes': path.stat().st_size, 'sha256': digest.hexdigest()}


def save(path: Path, report: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix('.tmp')
    temporary.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    temporary.replace(path)


def describe(values: list[float]) -> dict:
    return {'count': len(values), 'median': statistics.median(values),
            'mean': statistics.mean(values), 'minimum': min(values),
            'maximum': max(values), 'stdev': statistics.stdev(values) if len(values) > 1 else 0.0}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('baseline_executable', 'candidate_executable', 'baseline_library',
                 'candidate_library', 'driver_object', 'driver_source', 'resources', 'output'):
        parser.add_argument('--' + name.replace('_', '-'), required=True, type=Path)
    parser.add_argument('--baseline-checkpoint', required=True)
    parser.add_argument('--candidate-checkpoint', required=True)
    parser.add_argument('--runs', type=int, default=7)
    args = parser.parse_args()
    if args.runs < 3:
        parser.error('at least three measured rounds are required')
    receipt = json.loads((args.resources / 'split-receipt.json').read_text())
    artifacts = {name: identity(getattr(args, name)) for name in
                 ('baseline_executable', 'candidate_executable', 'baseline_library',
                  'candidate_library', 'driver_object', 'driver_source')}
    resources = {}
    for name, entry in [('common', receipt['common']), *receipt['languages'].items()]:
        resources[name] = identity(args.resources / entry['file'])
        assert resources[name]['bytes'] == entry['bytes']
        assert resources[name]['sha256'] == entry['sha256']
    report = {
        'schema': 1, 'started_at': datetime.now(timezone.utc).isoformat(), 'status': 'running',
        'reference': 'v0.111.0 / 41cef1ea',
        'baseline_checkpoint': args.baseline_checkpoint,
        'candidate_checkpoint': args.candidate_checkpoint,
        'execution_script': identity(Path(__file__)),
        'artifacts': artifacts, 'resources': resources,
        'split_receipt': identity(args.resources / 'split-receipt.json'),
        'resource_profiles': {locale: {key: entry[key] for key in
                              ('version', 'original_bytes', 'original_sha256', 'original_fnv')}
                              for locale, entry in receipt['languages'].items()},
        'method': 'One identical driver object linked against each frozen core library. '
                  'Same receipt-bound split files; source hashes are read before timing. Each open '
                  'runs in a fresh process, with one excluded warmup per binary and locale, '
                  'then alternating binary and locale order across measured rounds. '
                  'Elapsed time is host steady-clock wall time without startup drawing, '
                  'App initialization or first render. Counts describe requested FILE '
                  'callbacks/bytes/seeks, not physical disk transfers or CPU work.',
        'warmups': [], 'runs': [], 'summary': {},
    }
    save(args.output, report)
    counter_keys = ('file_callbacks', 'file_requested_bytes', 'file_seeks', 'file_failures')

    def invoke(which: str, locale: str, round_id: int, warmup: bool) -> None:
        command = [str(getattr(args, which + '_executable')), '--split',
                   resources['common']['path'], resources[locale]['path'], locale]
        process = subprocess.run(command, text=True, capture_output=True, timeout=90)
        result = {'binary': which, 'locale': locale, 'round': round_id,
                  'command': command, 'returncode': process.returncode,
                  'stderr': process.stderr, 'stdout': process.stdout}
        report['warmups' if warmup else 'runs'].append(result)
        save(args.output, report)
        if process.returncode:
            raise RuntimeError(f'{which} {locale} exited {process.returncode}: {process.stderr}')
        rows = [json.loads(line) for line in process.stdout.splitlines() if line]
        result['rows'] = rows
        opened, = [row for row in rows if row['operation'] == 'open']
        phases = {row['phase']: row for row in rows if row['operation'] == 'open_phase'}
        unphased, = [row for row in rows if row['operation'] == 'unphased_file_reads']
        assert opened['success'] and opened['source_kind'] == 'split'
        assert opened['scope'] == 'split_open_and_content_open'
        assert opened['pack_bytes'] == receipt['languages'][locale]['original_bytes']
        assert phases['integrity']['total'] == phases['integrity']['completed'] == opened['pack_bytes'] - 40
        assert set(phases) == {'header', 'integrity', 'directory', 'images', 'tile_index', 'glyphs', 'strings', 'ready'}
        assert all(row['completed'] == row['total'] and row['reports'] for row in phases.values())
        assert not opened['file_failures']
        for key in counter_keys:
            assert opened[key] == unphased[key] + sum(row[key] for row in phases.values())
        print(f'{"warmup" if warmup else "measured"} round={round_id} {which} {locale}: '
              f'open={opened["milliseconds"]:.3f} ms '
              f'tile_index={phases["tile_index"]["milliseconds"]:.3f} ms '
              f'tile_file_bytes={phases["tile_index"]["file_requested_bytes"]}', flush=True)
        save(args.output, report)

    try:
        for locale in ('zhs', 'eng'):
            for which in ('baseline', 'candidate'):
                invoke(which, locale, -1, True)
        for round_id in range(args.runs):
            locales = ('zhs', 'eng') if round_id % 2 == 0 else ('eng', 'zhs')
            binaries = ('baseline', 'candidate') if round_id % 2 == 0 else ('candidate', 'baseline')
            for locale in locales:
                for which in binaries:
                    invoke(which, locale, round_id, False)
        for locale in ('zhs', 'eng'):
            report['summary'][locale] = {}
            for scope in ('open', 'integrity', 'images', 'tile_index'):
                entry = {}
                for which in ('baseline', 'candidate'):
                    selected = []
                    for run in report['runs']:
                        if run['locale'] != locale or run['binary'] != which:
                            continue
                        selected.extend(row for row in run['rows'] if
                                        (scope == 'open' and row['operation'] == 'open') or
                                        (row['operation'] == 'open_phase' and row['phase'] == scope))
                    assert len(selected) == args.runs
                    counts = {tuple(row[key] for key in counter_keys) for row in selected}
                    assert len(counts) == 1, (locale, which, scope, counts)
                    entry[which] = {'milliseconds': describe([row['milliseconds'] for row in selected]),
                                    'file_counts': dict(zip(counter_keys, next(iter(counts))))}
                before = entry['baseline']['milliseconds']['median']
                after = entry['candidate']['milliseconds']['median']
                entry['median_reduction_percent'] = (before - after) * 100 / before if before else 0.0
                report['summary'][locale][scope] = entry
        # The FNV stage must remain a complete, unchanged set of file requests.
        for locale in ('zhs', 'eng'):
            integrity = report['summary'][locale]['integrity']
            assert integrity['baseline']['file_counts'] == integrity['candidate']['file_counts']
        for name, original in artifacts.items():
            assert identity(Path(original['path'])) == original, name
        for name, original in resources.items():
            assert identity(Path(original['path'])) == original, name
        report['status'] = 'passed'
    except BaseException as error:
        report['status'] = 'failed'
        report['failure'] = repr(error)
        raise
    finally:
        report['finished_at'] = datetime.now(timezone.utc).isoformat()
        save(args.output, report)


if __name__ == '__main__':
    main()
