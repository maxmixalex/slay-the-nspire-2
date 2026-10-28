#!/usr/bin/env python3
"""Build Firebird's upstream headless target in an isolated working copy.

With --source and --gif-source this command works without downloading anything.
It never opens, imports, copies or modifies calculator ROM/flash images.
It only builds the emulator; it does not claim that an Ndless app was tested.
"""
from __future__ import annotations
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
UPSTREAM = 'https://github.com/nspire-emus/firebird.git'


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, help='Existing extracted Firebird source directory')
    parser.add_argument('--gif-source', type=Path, help='Existing gif-h source, when missing from the Firebird archive')
    parser.add_argument('--jobs', type=int, default=min(8, os.cpu_count() or 1))
    parser.add_argument('--interpreter', action='store_true', help='Disable JIT for a portable interpreter build')
    args = parser.parse_args()
    if not 1 <= args.jobs <= 64:
        parser.error('--jobs must be between 1 and 64')
    make = shutil.which('make')
    if not make or not shutil.which('cc') or not shutil.which('c++'):
        parser.error('A C/C++ toolchain and make are required, along with zlib development files.')
    work = ROOT / 'build' / 'firebird-source'
    if work.exists():
        parser.error(f'{work} already exists. Move it aside before starting a new build; it will not be deleted automatically.')
    work.parent.mkdir(parents=True, exist_ok=True)
    if args.source:
        source = args.source.resolve()
        for relative in ('headless/Makefile', 'headless/main.cpp', 'core/emu.cpp'):
            if not (source / relative).is_file():
                parser.error(f'Not a complete Firebird source directory: missing {relative}')
        if work == source or source in work.parents:
            parser.error('The source must be outside the destination working directory.')
        shutil.copytree(source, work, ignore=shutil.ignore_patterns('.git', '*.o', 'build', 'firebird-headless'))
    else:
        git = shutil.which('git')
        if not git:
            parser.error('Install Git or pass --source with an existing checkout.')
        subprocess.run([git, 'clone', '--recursive', '--depth', '1', UPSTREAM, str(work)], check=True, timeout=180)
    if args.gif_source:
        gif = args.gif_source.resolve()
        if not (gif / 'gif.h').is_file():
            parser.error('--gif-source must contain gif.h')
        shutil.copytree(gif, work / 'core' / 'gif-h', dirs_exist_ok=True,
                        ignore=shutil.ignore_patterns('.git', '*.o', 'build'))
    if not (work / 'core' / 'gif-h' / 'gif.h').is_file():
        parser.error('Missing gif-h submodule. A GitHub source ZIP normally omits its files. Supply --gif-source or a recursive checkout.')
    command = [make, '-C', str(work / 'headless'), f'-j{args.jobs}']
    if args.interpreter:
        command.append('TRANSLATION_ENABLED=FALSE')
    subprocess.run(command, check=True)
    binary = work / 'headless' / 'firebird-headless'
    if not binary.is_file():
        raise RuntimeError('Upstream build did not produce firebird-headless.')
    subprocess.run([str(binary), '--help'], check=True)
    print(f'Built: {binary}\nThis is a build result, not an Ndless launch verification.')
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(f'Firebird build failed: {error}\nNo calculator image was accessed. '
              'For an offline build, pass --source and, when needed, --gif-source.', file=sys.stderr)
        raise SystemExit(1)
