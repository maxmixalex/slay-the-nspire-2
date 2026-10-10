#!/usr/bin/env python3
"""Add the audited 0.7 cards to a private pack, preserving existing animations.

Input: the owner's exact reference installation and an intact NSP2/3 or NSP2/4
pack. No original files or installable font are copied into the public source.
Output: NSP2/4, whose card directory separates gameplay IDs from asset offsets.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
from PIL import Image, ImageDraw, ImageFont
from build_content import ROOT, CARDS, choose_font, description, fnv, image_data, prepared, texture
from pck import Pack
from reference_inventory import verify_release

ADDITIONAL = ['BLOODLETTING', 'HEMOKINESIS', 'IMPERVIOUS', 'OFFERING', 'BLUDGEON',
              'THUNDERCLAP', 'UPPERCUT', 'BODY_SLAM', 'BATTLE_TRANCE', 'FIEND_FIRE']
EXHAUSTS = {'IMPERVIOUS', 'OFFERING', 'FIEND_FIRE'}


def upgrade(game: Path, source: Path, destination: Path, font_path: Path, *, intermediate: bool = False) -> dict:
    verify_release(game)
    data = source.read_bytes()
    if len(data) < 40:
        raise ValueError('Truncated pack')
    magic, version, commit, size, ni, ng, ns, nc, nk, digest = struct.unpack_from('<4s9I', data)
    if magic != b'NSP2' or version not in (3, 4) or commit != 0x41CEF1EA or size != len(data):
        raise ValueError('Unsupported pack identity')
    if (version == 3 and nk != 0) or (version == 4 and nk != 24):
        raise ValueError('Unsupported card directory')
    if ni > 1024 or ng > 8192 or ns > 2048 or nc != 11 or fnv(data[40:]) != digest:
        raise ValueError('Corrupt or incompatible pack')
    end = 40 + ni*16 + ng*16 + ns*8 + nc*16 + nk*16
    if end > len(data):
        raise ValueError('Truncated directories')

    def block(offset: int, length: int) -> bytes:
        if offset < end or length < 0 or offset > len(data)-length:
            raise ValueError('Payload outside pack')
        return data[offset:offset+length]

    images = []
    for i in range(ni):
        offset, w, h, fmt, length = struct.unpack_from('<IHHII', data, 40+i*16)
        images.append((w, h, fmt, block(offset, length)))
    strings = []
    for i in range(ns):
        offset, length = struct.unpack_from('<II', data, 40+ni*16+ng*16+i*8)
        raw = block(offset, length+1)
        if raw[-1]:
            raise ValueError('Unterminated string')
        strings.append(raw[:-1].decode('utf-8'))
    animation_start = 40+ni*16+ng*16+ns*8
    animations = data[animation_start:animation_start+nc*16]
    records = ([list(struct.unpack_from('<4I', data, animation_start+nc*16+i*16)) for i in range(nk)]
               if version == 4 else [[6+i, i*3, i*3+1, i*3+2] for i in range(14)])
    with_pack = Pack(game/'SlayTheSpire2.pck')
    try:
        localization = json.loads(with_pack.read('localization/zhs/cards.json'))
        for i, key in enumerate(CARDS+ADDITIONAL):
            text = [localization[key+'.title'], description(localization[key+'.description'], False),
                    description(localization[key+'.description'], True)]
            if key in EXHAUSTS:
                text[1] += '\n[gold]消耗。[/gold]'
                text[2] += '\n[gold]消耗。[/gold]'
            if i < len(records):
                for j in range(3):
                    strings[records[i][j+1]] = text[j]
            else:
                image = prepared(texture(with_pack, key.lower()+'.png'), (41, 30), 'cover')
                image_id, string_id = len(images), len(strings)
                images.append((41, 30, 0, image_data(image, False)))
                strings.extend(text)
                records.append([image_id, string_id, string_id+1, string_id+2])
    finally:
        with_pack.close()
    # Include only needed glyphs. Kept old glyph codepoints make an incremental
    # refresh deterministic and avoid losing text used by existing views.
    chars = set(''.join(strings)) | set(chr(i) for i in range(32, 127))
    for i in range(ng):
        cp = struct.unpack_from('<I', data, 40+ni*16+i*16)[0]
        if cp > 0x10ffff:
            raise ValueError('Invalid codepoint')
        chars.add(chr(cp))
    for path in (ROOT/'src').iterdir():
        if path.suffix in ('.cpp', '.inc'):
            chars.update(c for c in path.read_text(encoding='utf-8') if ord(c) > 127)
    chars = {c for c in chars if ord(c) >= 32}
    glyphs = []
    for px in (10, 12, 16, 20):
        font = ImageFont.truetype(str(font_path), px)
        for c in sorted(chars, key=ord):
            advance = max(1, math.ceil(font.getlength(c)))
            w, h = max(advance, font.getbbox(c)[2]), px+5
            if w > 40 or h > 40 or advance > 40:
                raise ValueError('Font exceeds glyph bounds')
            im = Image.new('L', (w, h))
            ImageDraw.Draw(im).text((0, px+1), c, font=font, fill=255, anchor='ls')
            glyphs.append((ord(c), px, w, h, advance, im.tobytes()))
    ni, ng, ns, nk = len(images), len(glyphs), len(strings), len(records)
    if ni > 1024 or ng > 8192 or ns > 2048:
        raise ValueError('Directory exceeds runtime bounds')
    start = 40+ni*16+ng*16+ns*8+nc*16+nk*16
    payload = bytearray()

    def append(raw: bytes, align: int = 1) -> int:
        while (start+len(payload)) % align:
            payload.append(0)
        offset = start+len(payload)
        payload.extend(raw)
        return offset

    image_dir = b''.join(struct.pack('<IHHII', append(raw, 2), w, h, fmt, len(raw))
                         for w, h, fmt, raw in images)
    glyph_dir = b''.join(struct.pack('<IIHBBHH', cp, append(raw), px, w, h, advance, 0)
                         for cp, px, w, h, advance, raw in glyphs)
    string_dir = bytearray()
    for text in strings:
        raw = text.encode('utf-8')
        string_dir.extend(struct.pack('<II', append(raw+b'\0'), len(raw)))
    card_dir = b''.join(struct.pack('<4I', *r) for r in records)
    body = image_dir+glyph_dir+bytes(string_dir)+animations+card_dir+bytes(payload)
    output = struct.pack('<4s9I', b'NSP2', 4, commit, 40+len(body), ni, ng, ns, nc, nk, fnv(body))+body
    if len(output) > (32 if intermediate else 6)*1024*1024:
        raise ValueError(f'Content exceeds 6 MiB: {len(output)} bytes')
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_name(destination.name+'.tmp')
    temporary.write_bytes(output)
    temporary.replace(destination)
    manifest = {'format': 'NSP2/4', 'reference_commit': '41cef1ea',
                'content_sha256': hashlib.sha256(output).hexdigest(), 'bytes': len(output),
                'cards': CARDS+ADDITIONAL, 'card_records': records,
                'images': ni, 'glyphs': ng, 'strings': ns, 'clips': nc,
                'scope': '24 modeled Ironclad cards; legacy expedition, not original run parity.',
                'conversion': 'Base images and animation clips retained; additional card art read from pinned PCK.'}
    destination.with_name('content-manifest.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(f'NSP2/4: {nk} cards; {len(output):,} bytes; {ni} images; {ng} glyphs')
    return manifest


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game', type=Path)
    parser.add_argument('source', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--font')
    args = parser.parse_args()
    upgrade(args.game, args.source, args.output, choose_font(args.font))


if __name__ == '__main__':
    main()
