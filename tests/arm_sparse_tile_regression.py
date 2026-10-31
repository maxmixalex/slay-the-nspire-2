#!/usr/bin/env python3
"""Exercise the production ARM926 sparse inverse, with optional instruction A/B.

Uses the function in the linked launcher ELF. Exact pixels, hidden RGB,
overlap boundaries, output guards and AAPCS are checked independently.
Instruction counts concern this decoder only, not calculator FPS or cycles.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from arm_division_regression import linked_helper
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM, UC_MODE_LITTLE_ENDIAN, UC_HOOK_CODE
from unicorn.arm_const import (UC_CPU_ARM_926, UC_ARM_REG_R0, UC_ARM_REG_R1,
    UC_ARM_REG_R2, UC_ARM_REG_R3, UC_ARM_REG_R4, UC_ARM_REG_R5,
    UC_ARM_REG_R6, UC_ARM_REG_R7, UC_ARM_REG_R8, UC_ARM_REG_R9,
    UC_ARM_REG_R10, UC_ARM_REG_R11, UC_ARM_REG_SP, UC_ARM_REG_LR,
    UC_ARM_REG_PC, UC_ARM_REG_CPSR)


def fixture(items, mode, density):
    original = bytearray(items * 770)
    header, masks = bytearray(items * 2), bytearray(items * 32)
    colors, alpha = bytearray(), bytearray()
    for tile in range(items):
        width, height = tile % 16 + 1, tile // 16 % 16 + 1
        original[tile * 770:tile * 770 + 2] = bytes((width, height))
        header[tile * 2:tile * 2 + 2] = bytes((width, height))
        count = (256 if tile % 7 == 0 else 64) if density == 'mixed' else density
        for pixel in range(256):
            # Permutation exercises all occupancy bytes, including pixels
            # beyond the visible rectangle and nonzero RGB with zero alpha.
            if (pixel * 73 + tile * 11) % 256 >= count:
                continue
            rgb = 1 + (tile * 257 + pixel * 31) % 65535
            opacity = (tile * 17 + pixel * 13) % 256 if pixel % 5 else 0
            struct.pack_into('<H', original, tile * 770 + 2 + pixel * 2, rgb)
            original[tile * 770 + 514 + pixel] = opacity
            green = (rgb >> 5) & 63
            red, blue = ((rgb >> 11) - (green >> 1)) & 31, ((rgb & 31) - (green >> 1)) & 31
            if mode == 2:
                red = red * 2 if red < 16 else (32 - red) * 2 - 1
                blue = blue * 2 if blue < 16 else (32 - blue) * 2 - 1
            colors.extend(struct.pack('<H', green | red << 6 | blue << 11))
            alpha.append(opacity)
            masks[tile * 32 + pixel // 8] |= 1 << (pixel & 7)
    return bytes(original), bytes(header + masks + colors + alpha)


class Runtime:
    destination, scratch = 0x40000000, 0x41000000

    def __init__(self, path):
        self.data = path.read_bytes()
        symbol = b'_ZN5spire29restore_sparse_tiles_in_placeEPhjjjS0_j'
        segments, (self.address, self.size) = linked_helper(self.data, symbol)
        emulator = self.emulator = Uc(UC_ARCH_ARM, UC_MODE_ARM | UC_MODE_LITTLE_ENDIAN)
        emulator.ctl_set_cpu_model(UC_CPU_ARM_926)
        low = min(a for a, _, _ in segments) & ~4095
        high = (max(a + n for a, n, _ in segments) + 4095) & ~4095
        if high - low > 24 * 1024 * 1024:
            raise ValueError('Linked runtime exceeds the 24 MiB mapping bound')
        emulator.mem_map(low, high - low)
        for start, _, payload in segments:
            emulator.mem_write(start, payload)
        self.stack, self.stop = 0x20000000, 0x30000000
        for address, length in ((self.stack, 65536), (self.stop, 4096),
                                (self.destination, 2 * 1024 * 1024), (self.scratch, 1024 * 1024)):
            emulator.mem_map(address, length)
        self.saved = {reg: 0xA5010000 + i * 0x101 for i, reg in enumerate((
            UC_ARM_REG_R4, UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7,
            UC_ARM_REG_R8, UC_ARM_REG_R9, UC_ARM_REG_R10, UC_ARM_REG_R11))}
        self.sp = self.stack + 65536 - 64
        self.instructions = self.vectors = 0

    def count(self, emulator, address, size, context):
        if size != 4:
            raise AssertionError('Expected ordinary ARM instructions')
        self.instructions += 1

    def execute(self, raw, encoded, items, mode, *, count=False, expected=True,
                declared=None, capacity=None, overlap=False):
        emulator = self.emulator
        pixels = max(0, (len(encoded) - items * 34) // 3)
        planes = items * 34 + pixels
        target, scratch = self.destination + 32, self.scratch + 32
        before = b'\xb7' * 32 + encoded + b'\xb7' * (len(raw) - len(encoded) + 32)
        scratch_guard = b'\xa9' * (planes + 64)
        emulator.mem_write(self.destination, before)
        emulator.mem_write(self.scratch, scratch_guard)
        emulator.reg_write(UC_ARM_REG_CPSR, 0x10)
        for reg, value in self.saved.items():
            emulator.reg_write(reg, value)
        for reg, value in zip((UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3),
                             (target, len(encoded) if declared is None else declared, items, mode)):
            emulator.reg_write(reg, value)
        emulator.mem_write(self.sp, struct.pack('<II', target + 1 if overlap else scratch,
                                              planes if capacity is None else capacity))
        emulator.reg_write(UC_ARM_REG_SP, self.sp)
        emulator.reg_write(UC_ARM_REG_LR, self.stop)
        self.instructions = 0
        hook = emulator.hook_add(UC_HOOK_CODE, self.count) if count else None
        if count:
            emulator.ctl_flush_tb()
        try:
            emulator.emu_start(self.address, self.stop, timeout=30000000, count=100000000)
        finally:
            if hook is not None:
                emulator.hook_del(hook)
        assert emulator.reg_read(UC_ARM_REG_PC) == self.stop
        assert emulator.reg_read(UC_ARM_REG_SP) == self.sp
        assert all(emulator.reg_read(reg) == value for reg, value in self.saved.items())
        assert bool(emulator.reg_read(UC_ARM_REG_R0)) == expected
        after = bytes(emulator.mem_read(self.destination, len(before)))
        assert after[:32] == before[:32] and after[-32:] == before[-32:]
        scratch_after = bytes(emulator.mem_read(self.scratch, len(scratch_guard)))
        assert scratch_after[:32] == scratch_guard[:32] and scratch_after[-32:] == scratch_guard[-32:]
        if expected:
            assert after[32:-32] == raw
        if count:
            assert self.instructions
        self.vectors += 1
        return self.instructions


def validate(path):
    runtime = Runtime(path)
    for items in (1, 17, 680, 1360):
        for mode in (1, 2):
            for density in (0, 64, 224):
                raw, encoded = fixture(items, mode, density)
                runtime.execute(raw, encoded, items, mode)
    for mode in (1, 2):
        raw, encoded = fixture(1360, mode, 'mixed')
        runtime.execute(raw, encoded, 1360, mode)
    raw, encoded = fixture(1, 1, 0)
    for mode in (0, 3):
        runtime.execute(raw, encoded, 1, mode, expected=False)
    runtime.execute(raw, encoded, 1, 1, expected=False, declared=len(encoded)-1)
    runtime.execute(raw, encoded, 1, 1, expected=False, capacity=33)
    runtime.execute(raw, encoded, 1, 1, expected=False, overlap=True)
    runtime.execute(raw, encoded[:2]+b'\x01'+encoded[3:], 1, 1, expected=False)
    instructions = {}
    for mode in (1, 2):
        for density in (0, 16, 64, 128, 224, 'mixed'):
            raw, encoded = fixture(16, mode, density)
            instructions[f'mode{mode}_density{density}'] = runtime.execute(raw, encoded, 16, mode, count=True)
    return dict(elf=str(path), elf_sha256=hashlib.sha256(runtime.data).hexdigest(),
                inverse_address=runtime.address, inverse_bytes=runtime.size,
                vectors=runtime.vectors, instructions=instructions,
                exact='RGB565 including hidden colors, alpha, dimensions and record padding',
                guards='output and scratch preserved', abi='r4-r11 and stack preserved')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('elf', type=Path)
    parser.add_argument('--baseline', type=Path)
    args = parser.parse_args()
    result = dict(candidate=validate(args.elf), cpu='ARM926',
                  limitation='Linked decoder instruction counts, not calculator FPS or cycles')
    if args.baseline:
        result['baseline'] = validate(args.baseline)
        result['instruction_change_percent'] = {
            key: (value / result['baseline']['instructions'][key] - 1) * 100
            for key, value in result['candidate']['instructions'].items()}
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
