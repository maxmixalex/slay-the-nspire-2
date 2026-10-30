#!/usr/bin/env python3
"""Verify complete ENG/ZHS resource-provider replays against saved byte oracles.

The existing locale_io_coalescing_regression executable renders the fixed
Continue/End Turn fixture. This runner compares its eager mode 2 with deferred
mode 3 or 4 on the same new packs, then compares every candidate RGB565 and
canonical State byte with the preserved UI16 output. Cross-version virtual
offsets are deliberately not compared because compression relocates payloads.
No original assets, saves, or oracle bytes are embedded in this source tool.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
FRAMES = 182
FRAME_BYTES = 320 * 240 * 2
SUFFIXES = ('.rgb565', '.states', '.hashes')
PARTS = ('interface', 'cards', 'characters', 'enemies', 'bosses', 'rooms', 'effects', 'shared')


def require(value, message):
    if not value:
        raise ValueError(message)


def identity(path):
    digest = hashlib.sha256()
    with path.open('rb') as source:
        while block := source.read(1024 * 1024):
            digest.update(block)
    return dict(bytes=path.stat().st_size, sha256=digest.hexdigest())


def compare_bytes(left, right):
    require(left.stat().st_size == right.stat().st_size,
            f'Oracle byte lengths differ: {left.name} / {right.name}')
    offset = 0
    with left.open('rb') as a, right.open('rb') as b:
        while block := a.read(1024 * 1024):
            other = b.read(len(block))
            if block != other:
                first = next(i for i, (x, y) in enumerate(zip(block, other)) if x != y)
                raise ValueError(f'Oracle bytes differ at {offset + first}: {left.name} / {right.name}')
            offset += len(block)
        require(not b.read(1), 'Unexpected oracle trailing bytes')
    return offset


def validate_frames(prefix):
    require(prefix.with_suffix('.rgb565').stat().st_size == FRAMES * FRAME_BYTES,
            f'Expected {FRAMES} complete RGB565 frames: {prefix}')
    lines = prefix.with_suffix('.hashes').read_text().splitlines()
    require(len(lines) == FRAMES, f'Expected {FRAMES} frame hashes: {prefix}')
    for i, line in enumerate(lines):
        require(re.fullmatch(str(i) + r' [0-9a-f]{16}', line), 'Frame hash sequence is incomplete')
    state_bytes = prefix.with_suffix('.states').stat().st_size
    count = consumed = 0
    with prefix.with_suffix('.states').open('rb') as source:
        while consumed < state_bytes:
            word = source.read(4)
            require(len(word) == 4, 'Truncated canonical State size')
            size, = struct.unpack('<I', word)
            consumed += 4
            require(0 < size <= state_bytes - consumed, 'Invalid canonical State extent')
            source.seek(size, 1)
            consumed += size
            count += 1
    require(count == FRAMES and consumed == state_bytes,
            f'Expected {FRAMES} complete canonical States: {prefix}')


def logical_trace(prefix):
    result = []
    last = 0
    for line in prefix.with_suffix('.log').read_text().splitlines():
        fields = line.split(',')
        require(len(fields) == 8 and fields[0].isdigit() and int(fields[0]) == last + 1,
                'Physical/logical trace row or sequence was lost')
        require(all(fields[i].isdigit() for i in (2, 5, 6, 7)), 'Malformed trace range')
        last += 1
        if fields[3] == 'virtual':
            require(fields[4] == 'NSP2', 'Unexpected logical resource tag')
            # Physical header discovery has a different number of trace rows;
            # retain stage, frame, complete virtual range and order, not its
            # incidental shared physical/logical sequence number.
            result.append(','.join(fields[1:]))
    require(result, 'No virtual resource reads were recorded')
    output = ('\n'.join(result) + '\n').encode()
    prefix.with_suffix('.logical').write_bytes(output)
    return len(result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--interpreter', type=Path, help='ELF interpreter when executable bits are unavailable')
    parser.add_argument('--candidate-mode', type=int, choices=(3, 4), required=True)
    parser.add_argument('--data', type=Path, required=True)
    parser.add_argument('--save', type=Path, required=True)
    parser.add_argument('--progress', type=Path, required=True)
    parser.add_argument('--baseline', type=Path, required=True, help='Preserved {eng,zhs}-mode-2 oracle directory')
    parser.add_argument('--earlier-english', type=Path, help='Optional still-earlier English oracle prefix')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--runtime-checkpoint', required=True)
    parser.add_argument('--timeout-per-run', type=int, default=300)
    args = parser.parse_args()
    require(re.fullmatch('[0-9a-f]{40}', args.runtime_checkpoint), 'Use the full frozen runtime checkpoint')
    require(1 <= args.timeout_per_run <= 1800, 'Invalid replay timeout')
    output = args.output.resolve()
    require(output != ROOT and ROOT not in output.parents, 'Private oracle output must remain outside the repository')
    require(not output.exists(), 'Never replace preserved oracle outputs')
    for path in (args.binary, args.save, args.progress):
        require(path.is_file(), 'Missing required replay input: ' + str(path))
    required = ('nSpire2.dat.tns', 'nSpire2.eng.dat.tns', 'nSpire2.zhs.dat.tns',
                *('nSpire2.' + part + '.dat.tns' for part in PARTS))
    require(all((args.data/name).is_file() for name in required), 'Matching bilingual index and eight parts are required')
    for language in ('eng', 'zhs'):
        validate_frames(args.baseline/(language + '-mode-2'))
    if args.earlier_english:
        validate_frames(args.earlier_english)
    output.mkdir(parents=True)
    report = dict(schema=1, complete=False, runtime_checkpoint=args.runtime_checkpoint,
        verification_script=identity(Path(__file__)), binary=identity(args.binary),
        candidate_mode=args.candidate_mode, frames_per_run=FRAMES,
        inputs=dict(save=identity(args.save), progress=identity(args.progress)),
        resources={path.name: identity(path) for path in sorted(args.data.glob('*.dat.tns'))},
        languages={})
    executable = ([str(args.interpreter.resolve())] if args.interpreter else []) + [str(args.binary.resolve())]
    for language in ('eng', 'zhs'):
        runs = {}
        for mode in (2, args.candidate_mode):
            prefix = output/(language + '-mode-' + str(mode))
            # Both nonzero modes use argv[3] for artwork and locale storage;
            # argv[2] is the unused legacy-container parameter of the driver.
            command = executable + [str(mode), str(args.data.resolve()), str(args.data.resolve()),
                str(args.save.resolve()), str(args.progress.resolve()), str(prefix), language]
            with prefix.with_suffix('.log').open('w') as log, prefix.with_suffix('.summary').open('w') as summary:
                subprocess.run(command, stdout=log, stderr=summary, check=True, timeout=args.timeout_per_run)
            validate_frames(prefix)
            requests = logical_trace(prefix)
            summary = re.findall(r'^mode=(\d+) calls=(\d+) bytes=(\d+) seeks=(\d+) opens=(\d+)$',
                                 prefix.with_suffix('.summary').read_text(), re.MULTILINE)
            require(len(summary) == 1 and int(summary[0][0]) == mode, 'Missing or ambiguous physical I/O summary')
            runs[str(mode)] = dict(command=command, logical_requests=requests,
                physical=dict(zip(('mode','calls','bytes','seeks','opens'), map(int, summary[0]))),
                files={suffix: identity(prefix.with_suffix(suffix)) for suffix in (*SUFFIXES, '.logical')})
            print(json.dumps(dict(language=language, **runs[str(mode)]['physical'], frames=FRAMES,
                                  logical_requests=requests)), flush=True)
        control = output/(language + '-mode-2')
        candidate = output/(language + '-mode-' + str(args.candidate_mode))
        baseline = args.baseline/(language + '-mode-2')
        equal_new = {suffix: compare_bytes(control.with_suffix(suffix), candidate.with_suffix(suffix))
                     for suffix in (*SUFFIXES, '.logical')}
        equal_old = {suffix: compare_bytes(candidate.with_suffix(suffix), baseline.with_suffix(suffix))
                     for suffix in SUFFIXES}
        result = dict(runs=runs, same_new_pack_equal_bytes=equal_new,
            old_ui16_equal_bytes=equal_old, baseline_files={suffix: identity(baseline.with_suffix(suffix)) for suffix in SUFFIXES})
        if language == 'eng' and args.earlier_english:
            result['earlier_english_equal_bytes'] = {
                suffix: compare_bytes(candidate.with_suffix(suffix), args.earlier_english.with_suffix(suffix))
                for suffix in SUFFIXES}
            result['earlier_english_files'] = {suffix: identity(args.earlier_english.with_suffix(suffix)) for suffix in SUFFIXES}
        report['languages'][language] = result
        print(language + ': complete frames, States and same-pack logical requests match exactly.', flush=True)
    report.update(complete=True, replayed_frames=FRAMES*4, candidate_frames_compared_to_ui16=FRAMES*2,
                  cross_version_virtual_offsets_compared=False, byte_comparison_not_hash_only=True)
    (output/'receipt.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(complete=True, candidate_mode=args.candidate_mode, replayed_frames=FRAMES*4,
                         candidate_frames_compared_to_ui16=FRAMES*2, receipt=str(output/'receipt.json'))), flush=True)


if __name__ == '__main__':
    main()
