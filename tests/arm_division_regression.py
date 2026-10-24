#!/usr/bin/env python3
"""Execute the linked Ndless ARM helper against Python's integer oracle.

Usage: python3 tests/arm_division_regression.py build/ndless/nSpire2.elf
Requires the optional Unicorn 2.1.2 emulator in the validation environment.
This checks actual ARM926 instructions and RTABI registers, not device speed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct

from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM, UC_MODE_LITTLE_ENDIAN
from unicorn.arm_const import (UC_CPU_ARM_926, UC_ARM_REG_R0, UC_ARM_REG_R1,
    UC_ARM_REG_R2, UC_ARM_REG_R3, UC_ARM_REG_R4, UC_ARM_REG_R5,
    UC_ARM_REG_R6, UC_ARM_REG_R7, UC_ARM_REG_R8, UC_ARM_REG_R9,
    UC_ARM_REG_R10, UC_ARM_REG_R11, UC_ARM_REG_R12, UC_ARM_REG_SP,
    UC_ARM_REG_LR, UC_ARM_REG_PC, UC_ARM_REG_CPSR)


def linked_helper(data, symbol=b'__aeabi_uldivmod'):
    if data[:7] != b'\x7fELF\x01\x01\x01':
        raise ValueError('Expected a little-endian ELF32 image')
    fields = struct.unpack_from('<HHIIIIIHHHHHH', data, 16)
    kind, machine, version, entry, phoff, shoff, flags, ehsize, phsize, phnum, shsize, shnum, _ = fields
    if kind != 2 or machine != 40 or version != 1 or phsize != 32 or shsize != 40:
        raise ValueError('Expected a linked ARM ELF32 image with ordinary tables')
    segments = []
    for i in range(phnum):
        ptype, offset, address, _, filesz, memsz, _, _ = struct.unpack_from('<8I', data, phoff + i * phsize)
        if ptype == 1 and memsz:
            if filesz > memsz or offset + filesz > len(data):
                raise ValueError('Invalid linked load segment')
            segments.append((address, memsz, data[offset:offset + filesz]))
    sections = [struct.unpack_from('<10I', data, shoff + i * shsize) for i in range(shnum)]
    matches = []
    for section in sections:
        if section[1] != 2:
            continue
        strings = sections[section[6]]
        names = data[strings[4]:strings[4] + strings[5]]
        if section[9] != 16:
            raise ValueError('Unexpected ELF symbol width')
        for offset in range(section[4], section[4] + section[5], 16):
            name, address, size, info, _, owner = struct.unpack_from('<IIIBBH', data, offset)
            if names[name:].split(b'\0', 1)[0] == symbol and owner:
                if info & 15 != 2 or not size or address & 3:
                    raise ValueError('Division helper is not a sized ARM function')
                matches.append((address, size))
    if len(matches) != 1 or not segments:
        raise ValueError('Need exactly one defined helper and loadable code')
    return segments, matches[0]


def vectors():
    maximum = (1 << 64) - 1
    edges = {0, 1, 2, 3, maximum, maximum - 1}
    for bit in (7, 8, 15, 16, 31, 32, 33, 62, 63):
        edges.update(((1 << bit) - 1, 1 << bit, (1 << bit) + 1))
    for numerator in sorted(edges):
        for denominator in sorted(edges):
            yield numerator, denominator
    rng = random.Random(0x41cef1ea)
    for i in range(4096):
        yield rng.getrandbits(64), rng.getrandbits(32 if i % 2 else 64)
    # Rest's long-running tick product and all original actor periods, plus
    # the treasure opening/shine frame normalization, exercise real callers.
    for ticks in (0, 1, 202, 645, 0x7fffffff, 0xfffffffe, 0xffffffff):
        for duration, frames in ((6667, 134), (8000, 160), (21333, 256), (10000, 200)):
            elapsed = ticks * 33
            yield elapsed, duration
            yield (elapsed % duration) * frames, duration
    for duration, frames in ((1133, 23), (500, 10)):
        for elapsed in (0, 33, duration - 1, duration):
            yield elapsed * frames, duration


def signed_vectors():
    minimum, maximum = -(1 << 63), (1 << 63) - 1
    edges = {minimum, minimum + 1, maximum, maximum - 1, -3, -2, -1, 0, 1, 2, 3}
    for bit in (7, 8, 15, 16, 31, 32, 33, 62):
        for offset in (-1, 0, 1):
            edges.update(((1 << bit) + offset, -((1 << bit) + offset)))
    for numerator in sorted(edges):
        for denominator in sorted(edges):
            yield numerator, denominator
    rng = random.Random(0x5349474e4544)
    for i in range(4096):
        yield rng.randrange(minimum, maximum + 1), rng.randrange(-(1 << (31 if i & 1 else 63)), 1 << (31 if i & 1 else 63))
    # Actual signed projection/raster divisors and both coordinate signs.
    for coordinate in (-3240, -1920, -960, -1, 0, 1, 960, 1920, 3240):
        for denominator in (22, 100, 255, 256, 4096, 16777216):
            yield coordinate * 65536, denominator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('elf', type=Path)
    parser.add_argument('--unsigned-only', action='store_true', help='Validate a historical ELF without the signed helper')
    args = parser.parse_args()
    data = args.elf.read_bytes()
    segments, (address, size) = linked_helper(data)
    emulator = Uc(UC_ARCH_ARM, UC_MODE_ARM | UC_MODE_LITTLE_ENDIAN)
    emulator.ctl_set_cpu_model(UC_CPU_ARM_926)
    low = min(a for a, _, _ in segments) & ~4095
    high = (max(a + n for a, n, _ in segments) + 4095) & ~4095
    if high - low > 32 * 1024 * 1024:
        raise ValueError('Linked runtime exceeds the bounded emulator mapping')
    emulator.mem_map(low, high - low)
    for start, _, payload in segments:
        emulator.mem_write(start, payload)
    stack, stop = 0x20000000, 0x30000000
    emulator.mem_map(stack, 65536)
    emulator.mem_map(stop, 4096)
    registers = [UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3,
        UC_ARM_REG_R4, UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7,
        UC_ARM_REG_R8, UC_ARM_REG_R9, UC_ARM_REG_R10, UC_ARM_REG_R11, UC_ARM_REG_R12]
    saved = {reg: 0xA5010000 + i * 0x101 for i, reg in enumerate(registers[4:12])}
    sp = stack + 65536 - 64
    records = []
    helpers = [(b'__aeabi_uldivmod', vectors(), False)]
    if not args.unsigned_only:
        helpers.append((b'__aeabi_ldivmod', signed_vectors(), True))
    for symbol, cases, signed in helpers:
        _, (entry, length) = linked_helper(data, symbol)
        count = 0
        for numerator, denominator in cases:
            # Zero divisors and MIN/-1 have defined local bit results outside
            # C++'s division domain. Ordinary quotients truncate toward zero.
            if not denominator:
                quotient, remainder = 0, numerator
            elif signed:
                quotient = abs(numerator) // abs(denominator)
                if (numerator < 0) != (denominator < 0):
                    quotient = -quotient
                remainder = numerator - quotient * denominator
            else:
                quotient, remainder = divmod(numerator, denominator)
            expected = quotient & ((1 << 64) - 1), remainder & ((1 << 64) - 1)
            emulator.reg_write(UC_ARM_REG_CPSR, 0x10)
            for reg, value in saved.items():
                emulator.reg_write(reg, value)
            for reg, value in zip(registers[:4], (numerator & 0xffffffff, (numerator >> 32) & 0xffffffff,
                    denominator & 0xffffffff, (denominator >> 32) & 0xffffffff)):
                emulator.reg_write(reg, value)
            emulator.reg_write(UC_ARM_REG_R12, 0xCAFE1234)
            emulator.reg_write(UC_ARM_REG_SP, sp)
            emulator.reg_write(UC_ARM_REG_LR, stop)
            emulator.emu_start(entry, stop, count=2048)
            actual = (emulator.reg_read(UC_ARM_REG_R0) | emulator.reg_read(UC_ARM_REG_R1) << 32,
                      emulator.reg_read(UC_ARM_REG_R2) | emulator.reg_read(UC_ARM_REG_R3) << 32)
            if actual != expected:
                raise AssertionError(f'{symbol.decode()}: {numerator:#x} / {denominator:#x}: {actual} != {expected}')
            if emulator.reg_read(UC_ARM_REG_PC) != stop or emulator.reg_read(UC_ARM_REG_SP) != sp:
                raise AssertionError('Helper did not return with its original stack')
            if any(emulator.reg_read(reg) != value for reg, value in saved.items()):
                raise AssertionError('Helper clobbered an AAPCS callee-saved register')
            count += 1
        records.append(dict(symbol=symbol.decode(), address=entry, bytes=length, vectors=count))
    print(json.dumps(dict(elf=str(args.elf), elf_sha256=hashlib.sha256(data).hexdigest(),
        helper_address=address, helper_bytes=size, vectors=sum(row['vectors'] for row in records), helpers=records, cpu='ARM926',
        quotient_and_remainder='exact Python integer oracle',
        callee_saved_registers='r4-r11 preserved', stack='restored',
        limitation='Instruction-level ABI validation, not calculator execution or performance')))


if __name__ == '__main__':
    main()
