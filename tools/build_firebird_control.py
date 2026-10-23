#!/usr/bin/env python3
"""Build the command frontend against an existing, unchanged Firebird checkout.

Firebird, its gif-h dependency, Boot1 and NAND images are supplied separately.
Only generated object files and the frontend executable are written to --output.
"""
from __future__ import annotations
import argparse
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import platform
import shutil
import subprocess


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', required=True, type=Path)
    parser.add_argument('--gif-h', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--cc', default='/usr/bin/clang-18')
    parser.add_argument('--cxx', default='/usr/bin/clang++-18')
    parser.add_argument('--jobs', type=int, default=3)
    args = parser.parse_args()
    if platform.machine() not in ('x86_64', 'AMD64'):
        parser.error('This build entry point currently supports an x86_64 host')
    if args.jobs < 1:
        parser.error('--jobs must be positive')
    source = args.source.resolve(strict=True)
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    includes = output / 'include' / 'gif-h'
    includes.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(args.gif_h.resolve(strict=True) / 'gif.h', includes / 'gif.h')
    c = ('asmcode.c translate_x86_64.c armsnippets_loader.c casplus.c des.c disasm.c '
         'gdbstub.c interrupt.c lcd.c link.c mem.c misc.c mmu.c schedule.c serial.c '
         'sha256.c usb.c usblink.c os/os-linux.c').split()
    cpp = ('arm_interpreter.cpp coproc.cpp cpu.cpp debug.cpp emu.cpp flash.cpp gif.cpp '
           'thumb_interpreter.cpp usblink_queue.cpp keypad.cpp cx2.cpp usb_cx2.cpp '
           'usblink_cx2.cpp fieldparser.cpp').split()
    sources = [(source / 'core' / name, args.cc, 'c11') for name in c]
    sources += [(source / 'core' / name, args.cxx, 'c++17') for name in cpp]
    sources += [(source / 'core' / 'asmcode_x86_64.S', args.cc, None),
                (Path(__file__).with_name('firebird_control.cpp').resolve(), args.cxx, 'c++17')]
    flags = ['-O2', '-g', '-DBENCHMARK', '-DSUPPORT_LINUX', '-pthread',
             '-I' + str(source), '-I' + str(output / 'include')]

    def compile_one(item: tuple[Path, str, str | None]) -> Path:
        path, compiler, standard = item
        obj = output / (path.parent.name + '-' + path.name + '.o')
        command = [compiler, *flags]
        if standard:
            command.append('-std=' + standard)
        command.extend(['-c', str(path), '-o', str(obj)])
        print('Compiling', path.name, flush=True)
        subprocess.run(command, check=True)
        return obj

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        objects = list(pool.map(compile_one, sources))
    binary = output / 'firebird-control'
    subprocess.run([args.cxx, '-pthread', *map(str, objects), '-lz', '-o', str(binary)], check=True)
    print(f'Built {binary} ({binary.stat().st_size} bytes); upstream core unchanged', flush=True)


if __name__ == '__main__':
    main()
