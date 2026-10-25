#!/usr/bin/env python3
"""Execute the production linked ARM RGR2 inverse, with AAPCS and guards.

This validates ARM926 instructions and exact retained pixels. It is neither
calculator execution nor a timing benchmark. No private resources are needed.
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


def color_words(count, start=0):
    return b''.join(struct.pack('<H', (start + i) & 65535) for i in range(count))


def filter_words(data, offset, count, stride, mode):
    """Independent forward oracle; the game ships only the inverse."""
    for i in range(count):
        at = offset + i * stride
        rgb, = struct.unpack_from('<H', data, at)
        green = rgb // 32 % 64
        red = (rgb // 2048 - green // 2) % 32
        blue = (rgb % 32 - green // 2) % 32
        if mode == 2:
            red = red * 2 if red < 16 else (32 - red) * 2 - 1
            blue = blue * 2 if blue < 16 else (32 - blue) * 2 - 1
        struct.pack_into('<H', data, at, green + red * 64 + blue * 2048)


def filtered(raw, format_, mode):
    result = bytearray(raw)
    if format_ < 2:
        # Dense alpha is a separate plane, not an interleaved triple.
        filter_words(result, 0, len(raw) // (3 if format_ else 2), 2, mode)
    elif format_ < 4:
        filter_words(result, 0, 256, 2, mode)
    else:
        at = 0
        while at < len(raw):
            spans, = struct.unpack_from('<H', raw, at)
            at += 2
            for _ in range(spans):
                _, count, alpha = struct.unpack_from('<HHB', raw, at)
                at += 5
                filter_words(result, at, count, 2 + alpha, mode)
                at += count * (2 + alpha)
    return bytes(result)


def group(slices, mode):
    original, encoded, table = bytearray(), bytearray(), bytearray()
    for format_, raw in slices:
        table.extend(struct.pack('<II', len(original), format_ << 24 | len(raw)))
        original.extend(raw)
        encoded.extend(filtered(raw, format_, mode))
    return bytes(original), bytes(encoded), bytes(table)


def sparse(rows, alpha, single_pixel_spans=False):
    result = bytearray()
    for row in range(rows):
        spans = 320 if single_pixel_spans else 1
        result.extend(struct.pack('<H', spans))
        for span in range(spans):
            x, count = (span, 1) if single_pixel_spans else (0, 320)
            result.extend(struct.pack('<HHB', x, count, alpha))
            for pixel in range(count):
                value = row * 320 + x + pixel
                result.extend(struct.pack('<H', value & 65535))
                if alpha:
                    result.append(value * 37 & 255)
    return bytes(result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('elf', type=Path)
    args = parser.parse_args()
    elf = args.elf.read_bytes()
    symbol = b'_ZN5spire22restore_retained_groupEPhjPKhjj'
    segments, (address, size) = linked_helper(elf, symbol)
    emulator = Uc(UC_ARCH_ARM, UC_MODE_ARM | UC_MODE_LITTLE_ENDIAN)
    emulator.ctl_set_cpu_model(UC_CPU_ARM_926)
    low = min(a for a, _, _ in segments) & ~4095
    high = (max(a + n for a, n, _ in segments) + 4095) & ~4095
    if high - low > 16 * 1024 * 1024:
        raise ValueError('Production runtime exceeds the fixed 16 MiB mapping')
    emulator.mem_map(low, high - low)
    for start, _, payload in segments:
        emulator.mem_write(start, payload)
    stack, stop, destination, source = 0x20000000, 0x30000000, 0x40000000, 0x41000000
    for start, length in ((stack, 65536), (stop, 4096),
                          (destination, 1024 * 1024), (source, 1024 * 1024)):
        emulator.mem_map(start, length)
    saved = {reg: 0xA5010000 + i * 0x101 for i, reg in enumerate((
        UC_ARM_REG_R4, UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7,
        UC_ARM_REG_R8, UC_ARM_REG_R9, UC_ARM_REG_R10, UC_ARM_REG_R11))}
    sp, vectors, rejected = stack + 65536 - 64, 0, 0
    records = []

    def execute(label, raw, encoded, table, mode, expected=True, declared=None,
                count=None, null_data=False, null_table=False, untouched=False):
        nonlocal vectors, rejected
        guarded = b'\xcd' * 32 + encoded + b'\xcd' * 32
        table_guarded = b'\xa7' * 32 + table + b'\xa7' * 32
        emulator.mem_write(destination, guarded)
        emulator.mem_write(source, table_guarded)
        emulator.mem_write(sp, struct.pack('<I', mode))
        emulator.reg_write(UC_ARM_REG_CPSR, 0x10)
        for reg, value in saved.items():
            emulator.reg_write(reg, value)
        parameters = (0 if null_data else destination + 32,
            len(raw) if declared is None else declared,
            0 if null_table else source + 32,
            len(table) // 8 if count is None else count)
        for reg, value in zip((UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3), parameters):
            emulator.reg_write(reg, value)
        emulator.reg_write(UC_ARM_REG_SP, sp)
        emulator.reg_write(UC_ARM_REG_LR, stop)
        emulator.emu_start(address, stop, timeout=30000000, count=200000000)
        if emulator.reg_read(UC_ARM_REG_PC) != stop or emulator.reg_read(UC_ARM_REG_SP) != sp:
            raise AssertionError(f'{label}: inverse did not return with its original stack')
        if bool(emulator.reg_read(UC_ARM_REG_R0)) != expected:
            raise AssertionError(f'{label}: linked inverse returned the wrong result')
        if any(emulator.reg_read(reg) != value for reg, value in saved.items()):
            raise AssertionError(f'{label}: inverse clobbered a callee-saved register')
        output = bytes(emulator.mem_read(destination, len(guarded)))
        if output[:32] != guarded[:32] or output[-32:] != guarded[-32:]:
            raise AssertionError(f'{label}: inverse wrote outside the guarded output')
        if bytes(emulator.mem_read(source, len(table_guarded))) != table_guarded:
            raise AssertionError(f'{label}: inverse changed the slice table')
        if expected and output[32:-32] != raw:
            raise AssertionError(f'{label}: linked inverse changed RGB565/alpha/palette/index/span bytes')
        if untouched and output != guarded:
            raise AssertionError(f'{label}: rejected metadata changed the output')
        vectors += 1
        rejected += not expected

    all_rgb = color_words(65536)
    dense_alpha = all_rgb + bytes((i * 37 & 255 for i in range(65536)))
    fixtures = [
        ('all-rgb565', [(0, all_rgb)]),
        ('dense-alpha-plane-all-rgb565', [(1, dense_alpha)]),
        ('maximum-512-kib', [(0, all_rgb * 4)]),
        ('palette-index-all-rgb565', [(2, color_words(256, i * 256) + bytes(range(251))) for i in range(256)]),
        ('palette-index-alpha-all-rgb565', [(3, color_words(256, i * 256) + bytes(range(250))) for i in range(256)]),
        ('sparse-240-rows-opaque', [(4, sparse(240, 0))]),
        ('sparse-240-rows-alpha', [(4, sparse(240, 1))]),
        ('sparse-320-spans', [(4, sparse(1, 1, True))]),
        ('sparse-empty-rows', [(4, b'\0' * 480)]),
        ('mixed-odd-offsets', [(1, b'\xff\xff\x6d'), (0, b'\x1f\x78'),
            (2, color_words(256) + b'\x07'), (3, color_words(256, 65500) + b'\0\xff'),
            (4, b'\0\0' + sparse(1, 1))]),
    ]
    for mode in (1, 2):
        for label, slices in fixtures:
            raw, encoded, table = group(slices, mode)
            execute(label, raw, encoded, table, mode)
            records.append(dict(label=label, mode=mode, raw_bytes=len(raw), slices=len(slices),
                                raw_sha256=hashlib.sha256(raw).hexdigest()))

        raw, encoded, table = group([(0, b'\x1f\xf8'), (1, b'\0\x07\x6d')], mode)
        for label, options in (
                ('null-data', dict(null_data=True)), ('null-table', dict(null_table=True)),
                ('zero-bytes', dict(declared=0)), ('above-512-kib', dict(declared=524289)),
                ('zero-count', dict(count=0)), ('oversized-count', dict(count=3)),
                ('huge-count', dict(count=0xffffffff)), ('incomplete-table', dict(count=1))):
            execute(label, raw, encoded, table, mode, False, untouched=True, **options)
        for label, offset, value in (
                ('first-offset-gap', 0, 1), ('second-offset-overlap', 8, 1),
                ('second-offset-gap', 8, 3), ('offset-overflow', 8, 0xffffffff),
                ('empty-slice', 4, 0), ('invalid-format', 4, 5 << 24 | 2),
                ('slice-overrun', 12, 1 << 24 | 4), ('uncovered-tail', 12, 1 << 24 | 2),
                ('length-overflow', 12, 1 << 24 | 0xffffff)):
            invalid = bytearray(table)
            struct.pack_into('<I', invalid, offset, value)
            execute(label, raw, encoded, bytes(invalid), mode, False, untouched=True)

        # Valid tables with malformed individual slices must fail safely. A
        # sparse failure may already have transformed an earlier span; the
        # caller discards the failed decoded group, so only bounds are promised.
        for label, format_, payload in (
                ('odd-rgb-bytes', 0, b'\0' * 3), ('partial-dense-alpha', 1, b'\0' * 4),
                ('palette-without-indices', 2, b'\0' * 512),
                ('palette-alpha-with-odd-index-plane', 3, b'\0' * 513),
                ('sparse-241-rows', 4, b'\0' * 482), ('partial-row', 4, b'\0' * 3),
                ('too-many-spans', 4, struct.pack('<H', 321)),
                ('partial-span', 4, b'\x01\0\0\0'),
                ('zero-span-pixels', 4, struct.pack('<HHHB', 1, 0, 0, 0)),
                ('invalid-span-alpha', 4, struct.pack('<HHHBH', 1, 0, 1, 2, 0)),
                ('span-x-overflow', 4, struct.pack('<HHHBH', 1, 321, 1, 0, 0)),
                ('span-end-overflow', 4, struct.pack('<HHHBH', 1, 320, 1, 0, 0)),
                ('truncated-span-colors', 4, struct.pack('<HHHBH', 1, 0, 2, 0, 0)),
                ('overlapping-spans', 4, struct.pack('<HHHBHHHBH', 2, 0, 1, 0, 0, 0, 1, 0, 0))):
            invalid = struct.pack('<II', 0, format_ << 24 | len(payload))
            execute(label, payload, payload, invalid, mode, False)

    raw, encoded, table = group([(0, b'\x1f\xf8')], 1)
    for mode in (0, 3, 0xffffffff):
        execute('invalid-mode', raw, encoded, table, mode, False, untouched=True)
    execute('valid-after-rejections', raw, encoded, table, 1)
    print(json.dumps(dict(elf=str(args.elf), elf_sha256=hashlib.sha256(elf).hexdigest(),
        inverse_address=address, inverse_bytes=size, vectors=vectors, rejected_vectors=rejected,
        fixtures=records, cpu='ARM926', output='RGB565, alpha, palette indices and sparse geometry exact',
        guards='preserved', slice_table='unchanged', callee_saved_registers='r4-r11 preserved', stack='restored',
        limitation='Linked instruction/ABI validation, not calculator execution or performance')))


if __name__ == '__main__':
    main()
