#!/usr/bin/env python3
"""Refresh interface glyphs in an NSP2/3 pack without resampling its images.

This is a private development conversion, not a content downloader. Image bytes,
localization strings and animation entries are preserved. No font is copied.
"""
from __future__ import annotations
import argparse
import hashlib
import math
from pathlib import Path
import struct
from PIL import Image, ImageDraw, ImageFont
from build_content import ROOT, choose_font, fnv


def refresh(source: Path, destination: Path, font_path: Path) -> tuple[int, str]:
    data = source.read_bytes()
    if len(data) < 40:
        raise ValueError('Truncated content pack')
    magic, version, commit, size, ni, ng, ns, nc, reserved, digest = struct.unpack_from('<4s9I', data)
    if magic != b'NSP2' or version != 3 or commit != 0x41CEF1EA or reserved or size != len(data):
        raise ValueError('Expected NSP2/3. For v4 use extend_cards.py with the pinned installation.')
    if ni > 1024 or ng > 8192 or ns > 2048 or nc != 11:
        raise ValueError('Unsupported directory counts')
    directory_end = 40 + ni * 16 + ng * 16 + ns * 8 + nc * 16
    if directory_end > len(data):
        raise ValueError('Truncated directories')

    def block(offset: int, count: int) -> bytes:
        if offset < directory_end or count < 0 or offset > len(data) - count:
            raise ValueError('Directory entry outside payload')
        return data[offset:offset + count]

    images = []
    for i in range(ni):
        offset, width, height, fmt, count = struct.unpack_from('<IHHII', data, 40 + i * 16)
        images.append((width, height, fmt, block(offset, count)))
    strings = []
    for i in range(ns):
        offset, count = struct.unpack_from('<II', data, 40 + ni * 16 + ng * 16 + i * 8)
        text = block(offset, count + 1)
        if not text.endswith(b'\0'):
            raise ValueError('Unterminated localization text')
        strings.append(text[:-1].decode('utf-8'))
    animations = data[directory_end - nc * 16:directory_end]
    chars = set(''.join(strings)) | set(chr(i) for i in range(32, 127))
    # Retain old glyphs too: a shared pack remains usable by version-3 viewers.
    for i in range(ng):
        cp = struct.unpack_from('<I', data, 40 + ni * 16 + i * 16)[0]
        chars.add(chr(cp))
    for path in [p for p in (ROOT / 'src').iterdir() if p.suffix in ('.cpp', '.inc')]:
        chars.update(c for c in path.read_text(encoding='utf-8') if ord(c) > 127)
    chars.difference_update('\n\r')
    glyphs = []
    for pixel_size in (10, 12, 16, 20):
        font = ImageFont.truetype(str(font_path), pixel_size)
        for c in sorted(chars, key=ord):
            advance = max(1, math.ceil(font.getlength(c)))
            width, height = max(advance, font.getbbox(c)[2]), pixel_size + 5
            if width > 40 or height > 40:
                raise ValueError('Font exceeds runtime glyph dimensions')
            image = Image.new('L', (width, height))
            ImageDraw.Draw(image).text((0, pixel_size + 1), c, font=font, fill=255, anchor='ls')
            glyphs.append((ord(c), pixel_size, width, height, advance, image.tobytes()))
    ng = len(glyphs)
    if ng > 8192:
        raise ValueError('Too many glyphs')
    start = 40 + ni * 16 + ng * 16 + ns * 8 + nc * 16
    payload = bytearray()

    def append(raw: bytes, alignment: int = 1) -> int:
        while (start + len(payload)) % alignment:
            payload.append(0)
        offset = start + len(payload)
        payload.extend(raw)
        return offset

    image_dir = b''.join(struct.pack('<IHHII', append(raw, 2), w, h, fmt, len(raw)) for w, h, fmt, raw in images)
    glyph_dir = b''.join(struct.pack('<IIHBBHH', cp, append(raw), size, w, h, advance, 0) for cp, size, w, h, advance, raw in glyphs)
    string_dir = bytearray()
    for text in strings:
        raw = text.encode('utf-8')
        string_dir.extend(struct.pack('<II', append(raw + b'\0'), len(raw)))
    body = image_dir + glyph_dir + bytes(string_dir) + animations + bytes(payload)
    output = struct.pack('<4s9I', magic, version, commit, len(body) + 40, ni, ng, ns, nc, 0, 0) + body
    if len(output) > 6 * 1024 * 1024:
        raise ValueError('Updated pack exceeds the resident buffer')
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_name(destination.name + '.tmp')
    temporary.write_bytes(output)
    temporary.replace(destination)
    return len(output), hashlib.sha256(output).hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--font')
    args = parser.parse_args()
    size, digest = refresh(args.source, args.output, choose_font(args.font))
    print(f'{args.output}: {size} bytes; SHA-256 {digest}')


if __name__ == '__main__':
    main()
