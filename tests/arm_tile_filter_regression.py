#!/usr/bin/env python3
"""Execute the actual linked in-place ARM TGR2 inverse, AAPCS and guards.

This is instruction/ABI evidence, not a calculator storage or FPS benchmark.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from arm_division_regression import linked_helper
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM, UC_MODE_LITTLE_ENDIAN
from unicorn.arm_const import (UC_CPU_ARM_926, UC_ARM_REG_R0, UC_ARM_REG_R1,
    UC_ARM_REG_R2, UC_ARM_REG_R3, UC_ARM_REG_R4, UC_ARM_REG_R5,
    UC_ARM_REG_R6, UC_ARM_REG_R7, UC_ARM_REG_R8, UC_ARM_REG_R9,
    UC_ARM_REG_R10, UC_ARM_REG_R11, UC_ARM_REG_SP, UC_ARM_REG_LR,
    UC_ARM_REG_PC, UC_ARM_REG_CPSR)


def fixture(count, mode):
    original, filtered = bytearray(count * 770), bytearray(count * 770)
    for tile in range(count):
        width, height = tile % 16 + 1, tile // 16 % 16 + 1
        original[tile * 770:tile * 770 + 2] = bytes((width, height))
        filtered[tile * 2:tile * 2 + 2] = bytes((width, height))
        for pixel in range(256):
            rgb = (tile * 256 + pixel) & 65535
            green = (rgb >> 5) & 63
            red = ((rgb >> 11) - (green >> 1)) & 31
            blue = ((rgb & 31) - (green >> 1)) & 31
            if mode == 2:
                red = red * 2 if red < 16 else (32 - red) * 2 - 1
                blue = blue * 2 if blue < 16 else (32 - blue) * 2 - 1
            struct.pack_into('<H', original, tile * 770 + 2 + pixel * 2, rgb)
            struct.pack_into('<H', filtered, count * 2 + tile * 512 + pixel * 2,
                             green | (red << 6) | (blue << 11))
            alpha = (tile * 256 + pixel) * 13 & 255
            original[tile * 770 + 514 + pixel] = alpha
            filtered[count * 514 + tile * 256 + pixel] = alpha
    return bytes(original), bytes(filtered)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('elf', type=Path)
    args = parser.parse_args()
    data = args.elf.read_bytes()
    # Content uses the in-place inverse. The linker may discard the unused
    # out-of-place convenience helper, so testing that symbol misses the real
    # production path and fails after otherwise harmless compiler changes.
    segments, (address, size) = linked_helper(data, b'_ZN5spire28restore_tile_filter_in_placeEPhjjjS0_j')
    emulator = Uc(UC_ARCH_ARM, UC_MODE_ARM | UC_MODE_LITTLE_ENDIAN)
    emulator.ctl_set_cpu_model(UC_CPU_ARM_926)
    low = min(a for a, _, _ in segments) & ~4095
    high = (max(a + n for a, n, _ in segments) + 4095) & ~4095
    if high - low > 24 * 1024 * 1024:
        raise ValueError('Production runtime exceeds the fixed 24 MiB budget')
    emulator.mem_map(low, high - low)
    for start, _, payload in segments:
        emulator.mem_write(start, payload)
    stack, stop, destination, scratch = 0x20000000, 0x30000000, 0x40000000, 0x41000000
    for start, length in ((stack, 65536), (stop, 4096), (destination, 1024 * 1024), (scratch, 1024 * 1024)):
        emulator.mem_map(start, length)
    saved = {reg: 0xA5010000 + i * 0x101 for i, reg in enumerate((
        UC_ARM_REG_R4, UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7,
        UC_ARM_REG_R8, UC_ARM_REG_R9, UC_ARM_REG_R10, UC_ARM_REG_R11))}
    sp, vectors = stack + 65536 - 64, 0
    records = []

    def execute(raw, filtered, count, mode, expected=True, declared=None, *,
                capacity=None, overlap=False, misalign=False):
        nonlocal vectors
        # 770-byte tile records naturally alternate between 0 and 2 mod 4;
        # also exercise a whole group starting at 2 mod 4.
        prefix = 34 if misalign else 32
        guard = b'\xcd' * prefix + filtered + b'\xcd' * 32
        planes = max(count, 1)*258
        scratch_guard = b'\xa9' * (planes+64)
        emulator.mem_write(destination, guard)
        emulator.mem_write(scratch, scratch_guard)
        target = destination+prefix
        emulator.mem_write(sp, struct.pack('<II', target+2 if overlap else scratch+32,
                                          planes if capacity is None else capacity))
        emulator.reg_write(UC_ARM_REG_CPSR, 0x10)
        for reg, value in saved.items():emulator.reg_write(reg, value)
        for reg, value in zip((UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3),
                              (target, len(filtered) if declared is None else declared, count, mode)):
            emulator.reg_write(reg, value)
        emulator.reg_write(UC_ARM_REG_SP, sp);emulator.reg_write(UC_ARM_REG_LR, stop)
        emulator.emu_start(address, stop, timeout=30000000, count=200000000)
        if emulator.reg_read(UC_ARM_REG_PC) != stop or emulator.reg_read(UC_ARM_REG_SP) != sp:
            raise AssertionError('Inverse did not return within its instruction/time bound')
        if bool(emulator.reg_read(UC_ARM_REG_R0)) != expected:
            raise AssertionError('Linked inverse returned the wrong result')
        if any(emulator.reg_read(reg) != value for reg, value in saved.items()):
            raise AssertionError('Inverse clobbered a callee-saved register')
        output = bytes(emulator.mem_read(destination, len(guard)))
        if output[:prefix] != guard[:prefix] or output[prefix+len(raw):] != guard[prefix+len(raw):]:
            raise AssertionError('Inverse wrote outside the guarded output')
        scratch_after = bytes(emulator.mem_read(scratch, len(scratch_guard)))
        if scratch_after[:32] != scratch_guard[:32] or scratch_after[-32:] != scratch_guard[-32:]:
            raise AssertionError('Inverse wrote outside the guarded scratch')
        if expected and output[prefix:prefix+len(raw)] != raw:
            raise AssertionError('Linked inverse changed RGB565/alpha/dimensions')
        vectors += 1

    for count in (1, 17, 256, 680, 1360):
        for mode in (1, 2):
            raw, filtered = fixture(count, mode)
            execute(raw, filtered, count, mode)
            execute(raw, filtered, count, mode, misalign=True)
            records.append(dict(tiles=count, mode=mode, raw_bytes=len(raw), raw_sha256=hashlib.sha256(raw).hexdigest()))
    raw, filtered = fixture(1, 1)
    for count, mode, length in ((0, 1, 770), (1361, 1, 770), (1, 0, 770), (1, 3, 770), (1, 1, 769)):
        execute(raw, filtered, count, mode, False, length)
    execute(raw, b'\0' + filtered[1:], 1, 1, False)
    execute(raw, filtered[:1] + b'\x11' + filtered[2:], 1, 1, False)
    execute(raw, filtered, 1, 1, False, capacity=257)
    execute(raw, filtered, 1, 1, False, overlap=True)
    execute(raw, filtered, 1, 1)
    print(json.dumps(dict(elf=str(args.elf), elf_sha256=hashlib.sha256(data).hexdigest(),
        inverse_address=address, inverse_bytes=size, vectors=vectors, fixtures=records,
        cpu='ARM926', output='all RGB565 values, alpha and dimensions exact',
        guards='output and scratch preserved', alignment='0 and 2 mod 4 exact',
        callee_saved_registers='r4-r11 preserved', stack='restored',
        limitation='Linked instruction/ABI validation, not calculator execution or performance')))


if __name__ == '__main__':
    main()
