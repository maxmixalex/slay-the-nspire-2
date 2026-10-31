#!/usr/bin/env python3
"""Extend a private NSP2/6 pack with seven audited colorless cards and eight relics.

The input installation uses the pinned release metadata. Existing image/animation payloads
and glyph coverage are retained; only the additional original art is converted.
No original game files or installable fonts are copied into public source.
"""
from __future__ import annotations
from conversion_context import file_limit
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import struct
from PIL import Image, ImageDraw, ImageFont
from build_content import ROOT, choose_font, description, fnv, image_data, prepared, texture
from build_expansion import rle
from build_silent import ALL_CARDS as BASE_CARDS
from lz4_block import encode as compress
from pck import Pack
from reference_inventory import verify_release

CARDS = ['IMPATIENCE', 'MASTER_OF_STRATEGY', 'SHOCKWAVE', 'SCRAWL', 'PURITY', 'HAND_OF_GREED', 'PANIC_BUTTON']
RELICS = [('HornCleat','horn_cleat'), ('CaptainsWheel','captains_wheel'),
          ('MercuryHourglass','mercury_hourglass'), ('ArtOfWar','art_of_war'),
          ('RegalPillow','regal_pillow'), ('MealTicket','meal_ticket'),
          ('OldCoin','old_coin'), ('CentennialPuzzle','centennial_puzzle')]
MAX_PACK = 6 * 1024 * 1024

def localized(key: str, text: str, upgraded: bool) -> str:
    text = description(text.replace('{Gold:diff()}', '{p}').replace('{Turns:diff()}', '{p}'), upgraded)
    if key not in ('IMPATIENCE', 'HAND_OF_GREED'):
        text += '\n[gold]消耗。[/gold]'
    if key == 'PURITY' or (key == 'SCRAWL' and upgraded):
        text += '\n[gold]保留。[/gold]'
    return text

def build(game: Path, source: Path, output: Path, font_path: Path) -> dict:
    verify_release(game)
    data = source.read_bytes()
    if not 40 <= len(data) <= file_limit(MAX_PACK):
        raise ValueError('Input pack is outside the resident size bound')
    magic, version, commit, size, ni, ng, ns, nc, nk, digest = struct.unpack_from('<4s9I', data)
    if magic != b'NSP2' or version not in (6, 7) or commit != 0x41CEF1EA or size != len(data):
        raise ValueError('Expected an intact pinned NSP2/6 or NSP2/7 pack')
    nu = 74 if version == 6 else 83
    if nk != (51 if version == 6 else 58) or nc != 17 or ni > 8192 or ng > 8192 or ns > 2048:
        raise ValueError('Incompatible directory counts')
    clip_start = 40 + ni*16 + ng*16 + ns*8
    card_start = clip_start + nc*16
    end = card_start + nk*16 + nu*4
    if end > len(data):
        raise ValueError('Truncated directories')
    def block(offset: int, length: int) -> bytes:
        if offset < end or length < 0 or offset > len(data)-length:
            raise ValueError('Record outside payload')
        return data[offset:offset+length]
    images = []
    for i in range(ni):
        off,w,h,fmt,n = struct.unpack_from('<IHHII', data, 40+i*16)
        images.append((w,h,fmt,block(off,n)))
    glyphs = {}
    for i in range(ng):
        cp,off,px,w,h,advance,encoded = struct.unpack_from('<IIHBBHH', data, 40+ni*16+i*16)
        if cp > 0x10ffff or not 0 < w <= 40 or not 0 < h <= 40:
            raise ValueError('Invalid glyph record')
        glyphs[cp,px] = (w,h,advance,encoded,block(off,encoded or w*h))
    strings = []
    for i in range(ns):
        off,n = struct.unpack_from('<II', data, 40+ni*16+ng*16+i*8)
        raw = block(off,n+1)
        if raw[-1] or b'\0' in raw[:-1]:
            raise ValueError('Invalid localization text')
        strings.append(raw[:-1].decode('utf-8'))
    animations = data[clip_start:card_start]
    records = [list(struct.unpack_from('<4I',data,card_start+i*16)) for i in range(nk)]
    aux = list(struct.unpack_from('<'+'I'*nu,data,card_start+nk*16))
    def image_record(im: Image.Image) -> tuple:
        alpha = im.getchannel('A').getextrema()[0] < 255
        fmt, raw = int(alpha), image_data(im,alpha)
        encoded = struct.pack('<BI',fmt,len(raw)) + compress(raw)
        return (*im.size, 5, encoded) if len(encoded) < len(raw) else (*im.size,fmt,raw)
    pck = Pack(game/'SlayTheSpire2.pck')
    try:
        texts = json.loads(pck.read('localization/zhs/cards.json'))
        for i,key in enumerate(CARDS,51):
            im = prepared(texture(pck,'images/packed/card_portraits/colorless/'+key.lower()+'.png'),(82,60),'cover')
            text = [texts[key+'.title'], localized(key,texts[key+'.description'],False), localized(key,texts[key+'.description'],True)]
            if i < len(records):
                images[records[i][0]] = image_record(im)
                for j in range(3): strings[records[i][j+1]] = text[j]
            else:
                first = len(strings);records.append([len(images),first,first+1,first+2])
                images.append(image_record(im));strings.extend(text)
        header = (ROOT/'include/ui_assets.hpp').read_text(encoding='utf-8')
        profile = [(name,'images/relics/'+key+'.png',(25,25)) for name,key in RELICS]
        profile.append(('NoBlock','no_block_power.png',(13,13)))
        for i,(name,path,size) in enumerate(profile,74):
            if f'U_{name}={i}' not in header:
                raise ValueError('Auxiliary profile/header mismatch: '+name)
            record = image_record(prepared(texture(pck,path),size,'contain'))
            if i < len(aux): images[aux[i]] = record
            else: aux.append(len(images));images.append(record)
    finally:
        pck.close()
    chars = set(''.join(strings)) | set(chr(i) for i in range(32,127)) | {chr(cp) for cp,px in glyphs}
    for path in (ROOT/'src').iterdir():
        if path.suffix in ('.cpp','.inc'):
            chars.update(c for c in path.read_text(encoding='utf-8') if ord(c)>127)
    chars = {c for c in chars if ord(c)>=32}
    for px in (10,12,16,20):
        font = ImageFont.truetype(str(font_path),px)
        for ch in sorted(chars,key=ord):
            key = (ord(ch),px)
            if key in glyphs: continue
            advance = max(1,math.ceil(font.getlength(ch)));w=max(advance,font.getbbox(ch)[2]);h=px+5
            if w > 40 or h > 40 or advance > 40:
                raise ValueError('Font exceeds runtime bounds')
            im=Image.new('L',(w,h));ImageDraw.Draw(im).text((0,px+1),ch,font=font,fill=255,anchor='ls')
            raw=im.tobytes();encoded=rle(raw);glyphs[key]=(w,h,advance,len(encoded),encoded or raw)
    ni,ng,ns,nk,nu = len(images),len(glyphs),len(strings),len(records),len(aux)
    if ni>8192 or ng>8192 or ns>2048 or nk!=58 or nu!=83:
        raise ValueError('Output directories exceed the runtime profile')
    start=40+ni*16+ng*16+ns*8+nc*16+nk*16+nu*4
    payload=bytearray();unique={}
    def append(raw:bytes,align:int=1)->int:
        key=(align,raw)
        if key in unique:return unique[key]
        while (start+len(payload))%align:payload.append(0)
        off=start+len(payload);payload.extend(raw);unique[key]=off;return off
    image_dir=b''.join(struct.pack('<IHHII',append(raw,2),w,h,fmt,len(raw)) for w,h,fmt,raw in images)
    glyph_dir=b''.join(struct.pack('<IIHBBHH',cp,append(raw),px,w,h,advance,encoded)
                       for (cp,px),(w,h,advance,encoded,raw) in sorted(glyphs.items(),key=lambda x:(x[0][1],x[0][0])))
    string_dir=b''.join(struct.pack('<II',append(text.encode('utf-8')+b'\0'),len(text.encode('utf-8'))) for text in strings)
    body=image_dir+glyph_dir+string_dir+animations+b''.join(struct.pack('<4I',*r) for r in records)+struct.pack('<'+'I'*nu,*aux)+payload
    result=struct.pack('<4s9I',b'NSP2',7,commit,40+len(body),ni,ng,ns,nc,nk,0)+body
    if len(result)>file_limit(MAX_PACK):
        raise ValueError(f'Output exceeds the six-MiB resident buffer: {len(result):,}')
    output.parent.mkdir(parents=True,exist_ok=True)
    temp=output.with_name(output.name+'.tmp');temp.write_bytes(result);temp.replace(output)
    manifest={'format':'NSP2/7','reference':'v0.111.0 / 41cef1ea','bytes':len(result),'sha256':hashlib.sha256(result).hexdigest(),
              'cards':BASE_CARDS+CARDS,'card_records':records,'images':ni,'glyphs':ng,'strings':ns,'clips':nc,
              'auxiliary_images':aux,'retained_baseline_images':1286,'excluded':['audio','multiplayer','timeline'],
              'scope':'24 Ironclad, 24 Silent, 9 colorless, 1 token; 25 relics; 16 potions. Custom expedition remains.'}
    output.with_name('content-manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(f'NSP2/7: {len(result):,} bytes; {nk} cards; {ni} images; {ng} glyphs',flush=True)
    return manifest

if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('game',type=Path);ap.add_argument('source',type=Path)
    ap.add_argument('--output',type=Path,required=True);ap.add_argument('--font');a=ap.parse_args()
    build(a.game,a.source,a.output,choose_font(a.font))
