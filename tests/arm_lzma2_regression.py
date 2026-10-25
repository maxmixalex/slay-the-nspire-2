#!/usr/bin/env python3
"""Execute the production linked decoder as ARM926 instructions.

Uses the optional Unicorn validation dependency, as arm_division_regression.py
does. This checks actual linked C/C++ decoding, guards and AAPCS registers. It is
not a calculator launch, storage-I/O benchmark or hardware performance result.
"""
import argparse
import hashlib
import json
import lzma
from pathlib import Path
import struct

from arm_division_regression import linked_helper
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM, UC_MODE_LITTLE_ENDIAN
from unicorn.arm_const import (UC_CPU_ARM_926, UC_ARM_REG_R0, UC_ARM_REG_R1,
    UC_ARM_REG_R2, UC_ARM_REG_R3, UC_ARM_REG_R4, UC_ARM_REG_R5,
    UC_ARM_REG_R6, UC_ARM_REG_R7, UC_ARM_REG_R8, UC_ARM_REG_R9,
    UC_ARM_REG_R10, UC_ARM_REG_R11, UC_ARM_REG_SP, UC_ARM_REG_LR,
    UC_ARM_REG_PC, UC_ARM_REG_CPSR)


def fixtures():
    yield 'one', b'Z', b'\x01\x00\x00Z\x00'
    # The offline adaptive encoder changes literal/position contexts, not the
    # runtime format or fixed probability arena. Exercise each added context
    # in the actual linked ARM decoder, including lc+lp == 4's largest arena.
    raw = bytes((i % 31) * 5 for i in range(4097))
    for lc, lp, pb in ((3, 0, 0), (4, 0, 1), (3, 1, 1)):
        packed = lzma.compress(raw, format=lzma.FORMAT_RAW,
            filters=[dict(id=lzma.FILTER_LZMA2, dict_size=512 * 1024,
                lc=lc, lp=lp, pb=pb, preset=9, nice_len=273, depth=512)])
        yield f'contexts-{lc}-{lp}-{pb}', raw, packed
    for name, raw in (
            ('ramp', bytes(i % 251 for i in range(4097))),
            ('maximum', bytes(512 * 1024))):
        packed = lzma.compress(raw, format=lzma.FORMAT_RAW,
            filters=[dict(id=lzma.FILTER_LZMA2, dict_size=512 * 1024)])
        yield name, raw, packed
    # Independent raw chunks exercise dictionary-reset/no-reset controls and
    # the largest accepted output without depending on compressor choices.
    raw = bytes(((i // 97) ^ (17 * i)) & 255 for i in range(512 * 1024))
    packed = bytearray()
    for at in range(0, len(raw), 65536):
        packed.extend(bytes((1 if at == 0 else 2, 255, 255)))
        packed.extend(raw[at:at + 65536])
    packed.append(0)
    yield 'maximum-raw-chunks', raw, bytes(packed)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('elf', type=Path)
    args = parser.parse_args()
    data = args.elf.read_bytes()
    segments, (address, size) = linked_helper(data,
        b'_ZN5spire18decode_lzma2_blockEPhjjPKhjj')
    emulator = Uc(UC_ARCH_ARM, UC_MODE_ARM | UC_MODE_LITTLE_ENDIAN)
    emulator.ctl_set_cpu_model(UC_CPU_ARM_926)
    low = min(a for a, _, _ in segments) & ~4095
    high = (max(a + n for a, n, _ in segments) + 4095) & ~4095
    if high - low > 24 * 1024 * 1024:
        raise ValueError('Production runtime exceeds its fixed 24 MiB budget')
    emulator.mem_map(low, high - low)
    for start, _, payload in segments:
        emulator.mem_write(start, payload)
    stack, stop, destination, source = 0x20000000, 0x30000000, 0x40000000, 0x41000000
    emulator.mem_map(stack, 65536)
    emulator.mem_map(stop, 4096)
    emulator.mem_map(destination, 1024 * 1024)
    emulator.mem_map(source, 1024 * 1024)
    saved = {reg: 0xA5010000 + i * 0x101 for i, reg in enumerate((
        UC_ARM_REG_R4, UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7,
        UC_ARM_REG_R8, UC_ARM_REG_R9, UC_ARM_REG_R10, UC_ARM_REG_R11))}
    sp = stack + 65536 - 64
    count = 0
    records = []

    def decode(raw, packed, expected, capacity, property, success):
        nonlocal count
        guard = b'\xcd' * (len(raw) + 64)
        emulator.mem_write(destination, guard)
        emulator.mem_write(source, packed)
        emulator.mem_write(sp, struct.pack('<II', len(packed), property))
        emulator.reg_write(UC_ARM_REG_CPSR, 0x10)
        for reg, value in saved.items():
            emulator.reg_write(reg, value)
        for reg, value in zip((UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2,
                UC_ARM_REG_R3), (destination + 32, capacity, expected, source)):
            emulator.reg_write(reg, value)
        emulator.reg_write(UC_ARM_REG_SP, sp)
        emulator.reg_write(UC_ARM_REG_LR, stop)
        emulator.emu_start(address, stop, timeout=30000000, count=200000000)
        if emulator.reg_read(UC_ARM_REG_PC) != stop or emulator.reg_read(UC_ARM_REG_SP) != sp:
            raise AssertionError('Decoder did not return within its instruction/time bound')
        if bool(emulator.reg_read(UC_ARM_REG_R0)) != success:
            raise AssertionError('Linked ARM decoder returned the wrong result')
        if any(emulator.reg_read(reg) != value for reg, value in saved.items()):
            raise AssertionError('Decoder clobbered a callee-saved register')
        output = bytes(emulator.mem_read(destination, len(guard)))
        if output[:32] != guard[:32] or output[len(raw) + 32:] != guard[len(raw) + 32:]:
            raise AssertionError('Decoder wrote outside the guarded output')
        if expected < len(raw) and output[expected + 32] != 0xcd:
            raise AssertionError('Decoder wrote beyond its declared output')
        if success and output[32:32 + len(raw)] != raw:
            raise AssertionError('ARM output differs from independent original bytes')
        count += 1

    for name, raw, packed in fixtures():
        decode(raw, packed, len(raw), len(raw), 14, True)
        decode(raw, packed[:-1], len(raw), len(raw), 14, False)
        decode(raw, packed + b'\0', len(raw), len(raw), 14, False)
        decode(raw, packed, len(raw), len(raw), 16, False)
        decode(raw, packed, len(raw), len(raw) - 1, 14, False)
        # A failed stream must not leave the shared fixed arena occupied.
        decode(raw, packed, len(raw), len(raw), 14, True)
        records.append(dict(name=name, raw_bytes=len(raw), encoded_bytes=len(packed),
            raw_sha256=hashlib.sha256(raw).hexdigest()))
    print(json.dumps(dict(elf=str(args.elf), elf_sha256=hashlib.sha256(data).hexdigest(),
        decoder_address=address, decoder_bytes=size, vectors=count, fixtures=records,
        cpu='ARM926', output='exact original synthetic bytes',
        guards='preserved', callee_saved_registers='r4-r11 preserved', stack='restored',
        limitation='Linked instruction/ABI validation, not calculator execution or performance')))


if __name__ == '__main__':
    main()
