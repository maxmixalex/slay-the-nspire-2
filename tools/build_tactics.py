#!/usr/bin/env python3
"""Append ten pinned character cards and power icons; losslessly compact NSP2.

The existing independent runtime decoder still reads raw LZ4 blocks. Only this
PC-side converter uses python-lz4's high-compression encoder. No frames, image
pixels, glyph coverage, clipping geometry or animation timing are discarded.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import lz4.block
from PIL import Image, ImageDraw, ImageFont
from build_content import ROOT, choose_font, description, fnv, image_data, prepared, texture
from build_expansion import rle
from build_shared import BASE_CARDS, CARDS as SHARED_CARDS
from lz4_block import decode
from pck import Pack
from reference_inventory import verify_release

CARDS = ['HEADBUTT', 'SWORD_BOOMERANG', 'PERFECTED_STRIKE', 'FLAME_BARRIER', 'DEMON_FORM',
         'DODGE_AND_ROLL', 'INFINITE_BLADES', 'BACKSTAB', 'ESCAPE_PLAN', 'PREDATOR']
ICONS = [('FeelNoPain','feel_no_pain'), ('DarkEmbrace','dark_embrace'),
         ('DemonForm','demon_form'), ('InfiniteBlades','infinite_blades'),
         ('FlameBarrier','flame_barrier'), ('BlockNextTurn','block_next_turn'),
         ('DrawNextTurn','draw_cards_next_turn')]
LIMIT = 6*1024*1024

def unpack_image(fmt: int, raw: bytes) -> tuple[int, bytes]:
    if fmt == 5:
        if len(raw) < 6:
            raise ValueError('Truncated LZ4 image')
        fmt, size = struct.unpack_from('<BI', raw)
        if fmt > 4 or not 0 < size <= 320*240*3:
            raise ValueError('Invalid decoded image bound')
        raw = decode(raw[5:], size)
    if not 0 <= fmt <= 4:
        raise ValueError('Unknown image format')
    return fmt, raw

def compact_image(fmt: int, raw: bytes) -> tuple[int, bytes]:
    inner, pixels = unpack_image(fmt, raw)
    block = lz4.block.compress(pixels, mode='high_compression', compression=12, store_size=False)
    # Verify with the independent Python decoder, not the encoder's library.
    if decode(block, len(pixels)) != pixels:
        raise ValueError('Lossless compression check failed')
    wrapped = struct.pack('<BI', inner, len(pixels)) + block
    options = [(fmt,raw), (inner,pixels), (5,wrapped)]
    return min(options, key=lambda pair: len(pair[1]))

def localized(key: str, text: str, upgraded: bool) -> str:
    text = text.replace('{Repeat:diff()}', 'NSPREPEATVALUE')
    for token in ('ExtraDamage', 'DamageBack'):
        text = text.replace('{'+token+':diff()}', '{p}')
    text = description(text, upgraded).replace('NSPREPEATVALUE', '{r}')
    if key == 'BACKSTAB':
        text += '\n[gold]消耗。[/gold]'
    if key == 'BACKSTAB' or (key == 'INFINITE_BLADES' and upgraded):
        text += '\n[gold]固有。[/gold]'
    return text

def build(game: Path, source: Path, output: Path, font_path: Path) -> dict:
    verify_release(game)
    data = source.read_bytes()
    if not 40 <= len(data) <= LIMIT:
        raise ValueError('Input exceeds the resident pack bound')
    magic, version, commit, size, ni, ng, ns, nc, nk, digest = struct.unpack_from('<4s9I', data)
    if (magic != b'NSP2' or version not in (7,8) or commit != 0x41cef1ea
            or size != len(data)):
        raise ValueError('Expected an intact pinned NSP2/7 or NSP2/8 pack')
    nu = 83 if version == 7 else 90
    if nk != (58 if version == 7 else 68) or nc != 17 or ni > 8192 or ng > 8192 or ns > 2048:
        raise ValueError('Incompatible directories')
    clip_start = 40+ni*16+ng*16+ns*8
    card_start = clip_start+nc*16
    end = card_start+nk*16+nu*4
    if end > len(data):
        raise ValueError('Truncated directories')
    def block(off: int, n: int) -> bytes:
        if off < end or n < 0 or off > len(data)-n:
            raise ValueError('Payload outside pack')
        return data[off:off+n]
    images = []
    original_images = []
    old_payload = new_payload = 0
    seen_offsets = set()
    for i in range(ni):
        off,w,h,fmt,n = struct.unpack_from('<IHHII', data, 40+i*16)
        if not 0 < w <= 320 or not 0 < h <= 240 or off % 2:
            raise ValueError('Invalid image geometry')
        raw = block(off,n)
        original_images.append((w,h,*unpack_image(fmt,raw)))
        compact_fmt, compressed = compact_image(fmt,raw)
        if off not in seen_offsets:
            old_payload += n; new_payload += len(compressed);seen_offsets.add(off)
        images.append((w,h,compact_fmt,compressed))
    glyphs = {}
    for i in range(ng):
        cp,off,px,w,h,advance,encoded = struct.unpack_from('<IIHBBHH',data,40+ni*16+i*16)
        if cp > 0x10ffff or not 0 < w <= 40 or not 0 < h <= 40 or advance > 40:
            raise ValueError('Invalid glyph')
        raw = block(off,encoded or w*h)
        if encoded and (encoded%2 or any(raw[j]==0 for j in range(0,encoded,2))
                        or sum(raw[::2]) != w*h):
            raise ValueError('Invalid glyph runs')
        if (cp,px) in glyphs:
            raise ValueError('Duplicate glyph')
        glyphs[cp,px] = (w,h,advance,encoded,raw)
    strings = []
    for i in range(ns):
        off,n = struct.unpack_from('<II',data,40+ni*16+ng*16+i*8)
        raw = block(off,n+1)
        if raw[-1] or b'\0' in raw[:-1]:
            raise ValueError('Invalid text')
        strings.append(raw[:-1].decode('utf-8'))
    animations = data[clip_start:card_start]
    records = [list(struct.unpack_from('<4I',data,card_start+i*16)) for i in range(nk)]
    aux = list(struct.unpack_from('<'+'I'*nu,data,card_start+nk*16))
    if any(rec[0]>=ni or any(j>=ns for j in rec[1:]) for rec in records) or any(i>=ni for i in aux):
        raise ValueError('Invalid card/UI reference')
    def encoded_image(image: Image.Image) -> tuple:
        alpha = image.getchannel('A').getextrema()[0] < 255
        fmt,raw = compact_image(int(alpha),image_data(image,alpha))
        return (*image.size,fmt,raw)
    pck = Pack(game/'SlayTheSpire2.pck')
    try:
        text = json.loads(pck.read('localization/zhs/cards.json'))
        for i,key in enumerate(CARDS,58):
            group = 'ironclad' if i < 63 else 'silent'
            image = prepared(texture(pck,'images/packed/card_portraits/'+group+'/'+key.lower()+'.png'),(82,60),'cover')
            parts = [text[key+'.title'],localized(key,text[key+'.description'],False),localized(key,text[key+'.description'],True)]
            if i < len(records):
                images[records[i][0]] = encoded_image(image)
                for j in range(3):strings[records[i][j+1]] = parts[j]
            else:
                first = len(strings);records.append([len(images),first,first+1,first+2]);strings.extend(parts)
                images.append(encoded_image(image))
        header = (ROOT/'include/ui_assets.hpp').read_text(encoding='utf-8')
        for i,(name,key) in enumerate(ICONS,83):
            if f'U_{name}={i}' not in header:
                raise ValueError('UI profile/header mismatch: '+name)
            image = encoded_image(prepared(texture(pck,key+'_power.png'),(13,13),'contain'))
            if i < len(aux):images[aux[i]] = image
            else:aux.append(len(images));images.append(image)
    finally:
        pck.close()
    # All legacy images must decode identically, even when compressed bytes differ.
    for i in range(min(1302,ni)):
        w,h,fmt,raw = images[i]
        if (w,h,*unpack_image(fmt,raw)) != original_images[i]:
            raise ValueError(f'Legacy image {i} changed')
    chars = set(''.join(strings)) | set(chr(i) for i in range(32,127)) | {chr(cp) for cp,_ in glyphs}
    for folder in ('src','include'):
        for path in (ROOT/folder).rglob('*'):
            if path.suffix in ('.cpp','.inc','.hpp'):
                chars.update(c for c in path.read_text(encoding='utf-8') if ord(c)>127)
    chars = {ch for ch in chars if ord(ch)>=32}
    for px in (10,12,16,20):
        font = ImageFont.truetype(str(font_path),px)
        for ch in sorted(chars,key=ord):
            if (ord(ch),px) in glyphs:continue
            advance=max(1,math.ceil(font.getlength(ch)));w=max(advance,font.getbbox(ch)[2]);h=px+5
            if w>40 or h>40 or advance>40:raise ValueError('Glyph exceeds runtime bounds')
            image=Image.new('L',(w,h));ImageDraw.Draw(image).text((0,px+1),ch,font=font,fill=255,anchor='ls')
            raw=image.tobytes();encoded=rle(raw)
            glyphs[ord(ch),px]=(w,h,advance,len(encoded),encoded or raw)
    ni,ng,ns,nk,nu=len(images),len(glyphs),len(strings),len(records),len(aux)
    if ni>8192 or ng>8192 or ns>2048 or nk!=68 or nu!=90:
        raise ValueError('Output directory capacity exceeded')
    start=40+ni*16+ng*16+ns*8+nc*16+nk*16+nu*4
    payload=bytearray();unique={}
    def append(raw: bytes, align: int=1) -> int:
        key=(align,raw)
        if key in unique:return unique[key]
        while (start+len(payload))%align:payload.append(0)
        off=start+len(payload);payload.extend(raw);unique[key]=off;return off
    image_dir=b''.join(struct.pack('<IHHII',append(raw,2),w,h,fmt,len(raw)) for w,h,fmt,raw in images)
    glyph_dir=b''.join(struct.pack('<IIHBBHH',cp,append(raw),px,w,h,advance,encoded)
                        for (cp,px),(w,h,advance,encoded,raw) in sorted(glyphs.items(),key=lambda pair:(pair[0][1],pair[0][0])))
    string_dir=b''.join(struct.pack('<II',append(t.encode('utf-8')+b'\0'),len(t.encode('utf-8'))) for t in strings)
    body=image_dir+glyph_dir+string_dir+animations+b''.join(struct.pack('<4I',*rec) for rec in records)+struct.pack('<'+'I'*nu,*aux)+payload
    result=struct.pack('<4s9I',b'NSP2',8,commit,40+len(body),ni,ng,ns,nc,nk,0)+body
    if len(result)>LIMIT:raise ValueError('Output exceeds six-MiB resident limit')
    manifest={'format':'NSP2/8','reference':'v0.111.0 / 41cef1ea','bytes':len(result),'sha256':hashlib.sha256(result).hexdigest(),
              'cards':BASE_CARDS+SHARED_CARDS+CARDS,'images':ni,'glyphs':ng,'strings':ns,'clips':nc,'auxiliary_count':nu,
              'legacy_images_losslessly_checked':min(1302,len(original_images)),
              'unique_image_payload_savings':old_payload-new_payload,'remaining_resident_bytes':LIMIT-len(result),
              'scope':'29 Ironclad + 29 Silent + 9 colorless + 1 generated Shiv; 25 relics; 16 potions; custom expedition.',
              'excluded':['audio','multiplayer','timeline']}
    output.parent.mkdir(parents=True,exist_ok=True)
    tmp=output.with_name(output.name+'.tmp');tmp.write_bytes(result);tmp.replace(output)
    output.with_name('content-manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(f"NSP2/8: {len(result):,} bytes; {nk} cards; {LIMIT-len(result):,} bytes headroom; {old_payload-new_payload:,} image bytes saved",flush=True)
    return manifest

if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('game',type=Path);ap.add_argument('source',type=Path)
    ap.add_argument('--output',type=Path,required=True);ap.add_argument('--font');args=ap.parse_args()
    build(args.game,args.source,args.output,choose_font(args.font))
