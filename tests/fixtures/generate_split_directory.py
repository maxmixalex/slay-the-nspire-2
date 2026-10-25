#!/usr/bin/env python3
"""Regenerate synthetic NSL2 decoder fixtures; no game/resource input is used."""
from pathlib import Path
import lzma
import struct

BLOCK = 65536
DIRECTORY = 3 * BLOCK + 4096
PREFIX, ARTWORK, SUFFIX = DIRECTORY + 321, 137, 41


def first_block():
    raw = bytearray(BLOCK)
    for i in range(40, BLOCK):
        raw[i] = (i * 71 + (i >> 5)) & 255
    struct.pack_into('<4s9I', raw, 0, b'NSP2', 38, 0x41CEF1EA,
                     PREFIX + ARTWORK + SUFFIX, 1, 1, 6111, 1, 1, 0)
    struct.pack_into('<II', raw, 40 + 16 + 16 + 6110 * 8,
                     PREFIX + ARTWORK + 7, 3)
    return bytes(raw)


def emit(name, raw):
    encoded = lzma.compress(raw, format=lzma.FORMAT_RAW, filters=[dict(
        id=lzma.FILTER_LZMA2, dict_size=524288, lc=3, lp=0, pb=2,
        mode=lzma.MODE_NORMAL, nice_len=273, mf=lzma.MF_BT4, depth=512)])
    assert len(encoded) < len(raw)
    decoder = lzma.LZMADecompressor(format=lzma.FORMAT_RAW, filters=[dict(
        id=lzma.FILTER_LZMA2, dict_size=524288)])
    assert decoder.decompress(encoded) == raw and decoder.eof and not decoder.unused_data
    rows = [f'static const uint8_t {name}[]={{']
    for start in range(0, len(encoded), 16):
        rows.append('    ' + ','.join(f'0x{n:02x}' for n in encoded[start:start + 16]) + ',')
    rows.append('};')
    return '\n'.join(rows)


def main():
    text = '// Synthetic bytes only. Reproduce with tests/fixtures/generate_split_directory.py.\n'
    text += '// Raw LZMA2: dictionary 512 KiB, lc=3/lp=0/pb=2, BT4, nice=273, depth=512.\n'
    text += emit('split_directory_first', first_block()) + '\n'
    text += emit('split_directory_full', bytes(0xA5 + i % 7 for i in range(BLOCK))) + '\n'
    text += emit('split_directory_last', bytes(0xA5 + i % 7 for i in range(4096))) + '\n'
    Path(__file__).with_name('split_directory_lzma2.inc').write_text(text)


if __name__ == '__main__':
    main()
