#!/usr/bin/env python3
"""Check the linked Ndless memory primitives on ARM926, optionally A/B them.

Use the production ELF, not a reimplementation or specially compiled kernel.
Byte bounds/alignment, guards, exact return values and AAPCS are checked.
Instruction totals describe these calls only, never calculator FPS or cycles.
"""
import argparse
import hashlib
import json
from pathlib import Path

from arm_division_regression import linked_helper
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM, UC_MODE_LITTLE_ENDIAN
from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_WRITE
from unicorn.arm_const import (UC_CPU_ARM_926, UC_ARM_REG_R0, UC_ARM_REG_R1,
    UC_ARM_REG_R2, UC_ARM_REG_R3, UC_ARM_REG_R4, UC_ARM_REG_R5,
    UC_ARM_REG_R6, UC_ARM_REG_R7, UC_ARM_REG_R8, UC_ARM_REG_R9,
    UC_ARM_REG_R10, UC_ARM_REG_R11, UC_ARM_REG_SP, UC_ARM_REG_LR,
    UC_ARM_REG_PC, UC_ARM_REG_CPSR)


class Runtime:
    destination, source = 0x40000000, 0x41000000
    arena_size = 256 * 1024

    def __init__(self, path):
        self.data = path.read_bytes()
        segments, _ = linked_helper(self.data, b'memcpy')
        self.symbols = {name: linked_helper(self.data, name.encode())[1]
                        for name in ('memcpy', 'memset', 'memmove')}
        # The final launcher can garbage-collect an unused libc primitive.
        # Report that fact; the same production object may also be linked as
        # a standalone component ELF to exercise its otherwise unused memcmp.
        try:
            self.symbols['memcmp'] = linked_helper(self.data, b'memcmp')[1]
        except ValueError:
            pass
        self.emulator = emulator = Uc(UC_ARCH_ARM, UC_MODE_ARM | UC_MODE_LITTLE_ENDIAN)
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
                                (self.destination, self.arena_size), (self.source, self.arena_size)):
            emulator.mem_map(address, length)
        self.saved = {reg: 0xA5010000 + i * 0x101 for i, reg in enumerate((
            UC_ARM_REG_R4, UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7,
            UC_ARM_REG_R8, UC_ARM_REG_R9, UC_ARM_REG_R10, UC_ARM_REG_R11))}
        self.sp = self.stack + 65536 - 64
        self.readable, self.writable = [], []
        self.vectors, self.instructions = 0, 0
        emulator.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, self.access)

    def access(self, emulator, access, address, size, value, context):
        if size in (2, 4, 8) and address % min(size, 4):
            raise AssertionError(f'Unaligned {size}-byte access at {address:#x}')
        if any(start <= address < start + self.arena_size for start in (self.destination, self.source)):
            allowed = self.writable if access == UC_MEM_WRITE else self.readable
            if not any(start <= address and address + size <= end for start, end in allowed):
                raise AssertionError(f'Out-of-contract memory access: {address:#x}+{size}')

    def count(self, emulator, address, size, context):
        if size != 4:
            raise AssertionError('Expected ordinary ARM instructions')
        self.instructions += 1

    def call(self, name, first, second, length, readable, writable, count=False):
        emulator = self.emulator
        self.readable, self.writable = readable, writable
        emulator.reg_write(UC_ARM_REG_CPSR, 0x10)
        for register, value in self.saved.items():
            emulator.reg_write(register, value)
        for register, value in zip((UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3),
                                    (first, second & 0xffffffff, length, 0xabcddcba)):
            emulator.reg_write(register, value)
        emulator.reg_write(UC_ARM_REG_SP, self.sp)
        emulator.reg_write(UC_ARM_REG_LR, self.stop)
        self.instructions = 0
        hook = emulator.hook_add(UC_HOOK_CODE, self.count) if count else None
        if count:
            # Unicorn 2.0 can reuse blocks translated before a code hook was
            # installed. Retranslate so a zero count cannot look like a gain.
            emulator.ctl_flush_tb()
        try:
            emulator.emu_start(self.symbols[name][0], self.stop, timeout=30000000, count=8000000)
        finally:
            if hook is not None:
                emulator.hook_del(hook)
        if emulator.reg_read(UC_ARM_REG_PC) != self.stop or emulator.reg_read(UC_ARM_REG_SP) != self.sp:
            raise AssertionError(f'{name} did not return with its original stack')
        if any(emulator.reg_read(register) != value for register, value in self.saved.items()):
            raise AssertionError(f'{name} clobbered a callee-saved register')
        if count and not self.instructions:
            raise AssertionError('Instruction callback did not run')
        self.vectors += 1
        return emulator.reg_read(UC_ARM_REG_R0)

    def memory_case(self, name, length, destination_offset, source_offset=0, value=0, mismatch=None, count=False):
        emulator = self.emulator
        guard = bytes([0xcd]) * (length + 80)
        payload = bytes((i * 37 + (i >> 3) * 11 + 17) & 255 for i in range(length))
        destination, source = self.destination + 32 + destination_offset, self.source + 32 + source_offset
        emulator.mem_write(self.destination, guard)
        emulator.mem_write(self.source, guard)
        emulator.mem_write(source, payload)
        readable, writable = [(source, source + length)], []
        expected = bytearray(guard)
        if name == 'memcpy':
            expected[32 + destination_offset:32 + destination_offset + length] = payload
            writable = [(destination, destination + length)]
            result = self.call(name, destination, source, length, readable, writable, count)
            assert result == destination
        elif name == 'memset':
            expected[32 + destination_offset:32 + destination_offset + length] = bytes([value & 255]) * length
            writable = [(destination, destination + length)]
            result = self.call(name, destination, value, length, [], writable, count)
            assert result == destination
        else:
            altered = bytearray(payload)
            if mismatch is not None:
                altered[mismatch] ^= 255
            emulator.mem_write(destination, bytes(altered))
            expected[32 + destination_offset:32 + destination_offset + length] = altered
            readable.append((destination, destination + length))
            result = self.call(name, destination, source, length, readable, [], count)
            difference = 0 if mismatch is None else altered[mismatch] - payload[mismatch]
            assert result == (difference & 0xffffffff)
        if bytes(emulator.mem_read(self.destination, len(guard))) != expected:
            raise AssertionError(f'{name} changed a destination guard or produced incorrect bytes')
        original = bytearray(guard)
        original[32 + source_offset:32 + source_offset + length] = payload
        if bytes(emulator.mem_read(self.source, len(guard))) != original:
            raise AssertionError(f'{name} modified its source')
        return self.instructions

    def move_case(self, length, offset, alignment):
        data = bytes((i * 53 + 9) & 255 for i in range(length + 128))
        first = 48 + alignment
        second = first + offset
        self.emulator.mem_write(self.destination, data)
        expected = bytearray(data)
        expected[second:second + length] = data[first:first + length]
        source, destination = self.destination + first, self.destination + second
        result = self.call('memmove', destination, source, length,
                           [(source, source + length)], [(destination, destination + length)])
        if result != destination or bytes(self.emulator.mem_read(self.destination, len(data))) != expected:
            raise AssertionError('memmove overlap/guard mismatch')


def validate(path):
    runtime = Runtime(path)
    lengths = list(range(65)) + [65, 67, 79, 95, 127, 128, 129, 255, 256, 257, 1023, 1024, 1025, 4095, 4096, 4097]
    for length in lengths:
        for destination in range(4):
            for source in range(4):
                runtime.memory_case('memcpy', length, destination, source)
                if 'memcmp' in runtime.symbols:
                    runtime.memory_case('memcmp', length, destination, source)
                if length and 'memcmp' in runtime.symbols:
                    for mismatch in sorted({0, length // 2, length - 1}):
                        runtime.memory_case('memcmp', length, destination, source, mismatch=mismatch)
            for value in (-257, -1, 0, 1, 255, 256, 0x1234):
                runtime.memory_case('memset', length, destination, value=value)
    for length in (16384, 65536, 153600):
        for destination, source in ((0, 0), (1, 1), (3, 3), (0, 1)):
            runtime.memory_case('memcpy', length, destination, source)
            if 'memcmp' in runtime.symbols:
                runtime.memory_case('memcmp', length, destination, source)
            runtime.memory_case('memset', length, destination, value=0xa5)
    for length in (0, 1, 2, 3, 4, 5, 7, 8, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127, 128, 129, 256, 257, 1024):
        for offset in (-31, -17, -4, -3, -2, -1, 0, 1, 2, 3, 4, 17, 31):
            for alignment in range(4):
                runtime.move_case(length, offset, alignment)
    instructions = {}
    for name in ('memcpy', 'memset', 'memcmp'):
        if name not in runtime.symbols:
            continue
        for length in (8, 64, 4096):
            for destination, source in ((0, 0), (1, 1), (0, 1)):
                if name == 'memset' and (destination, source) == (0, 1):
                    continue
                key = f'{name}/{length}/dst{destination}/src{source}'
                instructions[key] = runtime.memory_case(name, length, destination, source, value=0xa5, count=True)
    return dict(elf=str(path), elf_sha256=hashlib.sha256(runtime.data).hexdigest(), vectors=runtime.vectors,
                functions={name: dict(address=address, bytes=size) for name, (address, size) in runtime.symbols.items()},
                instructions=instructions, omitted_symbols=[name for name in ('memcmp',) if name not in runtime.symbols],
                bounds='every data access within the supplied byte ranges',
                alignment='no unaligned multi-byte accesses', guards='unchanged',
                abi='return values, r4-r11 and stack preserved')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('elf', type=Path)
    parser.add_argument('--baseline', type=Path)
    args = parser.parse_args()
    result = dict(candidate=validate(args.elf), cpu='ARM926',
                  limitation='Linked instruction counts and correctness, not calculator FPS or cycles')
    if args.baseline:
        result['baseline'] = validate(args.baseline)
        result['instruction_change_percent'] = {
            key: (value / result['baseline']['instructions'][key] - 1) * 100
            for key, value in result['candidate']['instructions'].items()}
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
