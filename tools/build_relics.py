#!/usr/bin/env python3
"""Complete the private 0.12 pack: five relic icons, immutable old payloads.

Accept pinned NSP2/8 or NSP2/9; retain the 68-card directory, existing images,
strings, glyphs and all seventeen clips. Only local original assets are read.
"""
from __future__ import annotations
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
from build_tactics import BASE_CARDS, SHARED_CARDS, CARDS as TACTICS_CARDS
from lz4_block import encode as compress
from pck import Pack
from reference_inventory import verify_release

RELICS = [('Kunai','kunai'), ('Shuriken','shuriken'),
          ('OrnamentalFan','ornamental_fan'), ('LetterOpener','letter_opener'),
          ('Nunchaku','nunchaku')]
MAX_PACK = 6 * 1024 * 1024

def build(game: Path, source: Path, output: Path, font_path: Path) -> dict:
    verify_release(game)
    data = source.read_bytes()
    if not 40 <= len(data) <= MAX_PACK:
        raise ValueError('Input pack is outside the resident size bound')
    magic, version, commit, size, ni, ng, ns, nc, nk, digest = struct.unpack_from('<4s9I', data)
    if magic != b'NSP2' or version not in (8, 9) or commit != 0x41CEF1EA or size != len(data) or fnv(data[40:]) != digest:
        raise ValueError('Expected an intact pinned NSP2/8 or NSP2/9 pack')
    nu = 90 if version == 8 else 95
    if nk != 68 or nc != 17 or ni > 8192 or ng > 8192 or ns > 2048:
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
        if not 0 < w <= 320 or not 0 < h <= 240 or fmt > 5 or off % 2:
            raise ValueError('Invalid image record')
        images.append((w,h,fmt,block(off,n)))
    glyphs = {}
    for i in range(ng):
        cp,off,px,w,h,advance,encoded = struct.unpack_from('<IIHBBHH', data, 40+ni*16+i*16)
        if cp > 0x10ffff or not 0 < w <= 40 or not 0 < h <= 40:
            raise ValueError('Invalid glyph record')
        if (cp,px) in glyphs or advance > 40:
            raise ValueError('Duplicate or invalid glyph record')
        raw = block(off,encoded or w*h)
        if encoded and (encoded%2 or any(raw[j]==0 for j in range(0,encoded,2)) or sum(raw[::2]) != w*h):
            raise ValueError('Invalid glyph runs')
        glyphs[cp,px] = (w,h,advance,encoded,raw)
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
        header = (ROOT/'include/ui_assets.hpp').read_text(encoding='utf-8')
        profile = [(name,'images/relics/'+key+'.png',(25,25)) for name,key in RELICS]
        for i,(name,path,size) in enumerate(profile,90):
            if f'U_{name}={i}' not in header:
                raise ValueError('Auxiliary profile/header mismatch: '+name)
            record = image_record(prepared(texture(pck,path),size,'contain'))
            if i < len(aux): images[aux[i]] = record
            else: aux.append(len(images));images.append(record)
    finally:
        pck.close()
    chars = set(''.join(strings)) | set(chr(i) for i in range(32,127)) | {chr(cp) for cp,px in glyphs}
    for folder in ('src','include'):
        for path in (ROOT/folder).rglob('*'):
            if path.suffix in ('.cpp','.inc','.hpp'):
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
    if ni>8192 or ng>8192 or ns>2048 or nk!=68 or nu!=95:
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
    result=struct.pack('<4s9I',b'NSP2',9,commit,40+len(body),ni,ng,ns,nc,nk,fnv(body))+body
    if len(result)>MAX_PACK:
        raise ValueError(f'Output exceeds the six-MiB resident buffer: {len(result):,}')
    output.parent.mkdir(parents=True,exist_ok=True)
    temp=output.with_name(output.name+'.tmp');temp.write_bytes(result);temp.replace(output)
    manifest={'format':'NSP2/9','reference':'v0.111.0 / 41cef1ea','bytes':len(result),'sha256':hashlib.sha256(result).hexdigest(),
              'cards':BASE_CARDS+SHARED_CARDS+TACTICS_CARDS,'card_records':records,'images':ni,'glyphs':ng,'strings':ns,'clips':nc,
              'auxiliary_images':aux,'retained_baseline_images':1319,'excluded':['audio','multiplayer','timeline'],
              'scope':'29 Ironclad, 29 Silent, 9 colorless, 1 token; 30 relics; 16 potions. Custom expedition remains.'}
    output.with_name('content-manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(f'NSP2/9: {len(result):,} bytes; {nk} cards; {ni} images; {ng} glyphs',flush=True)
    return manifest

if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('game',type=Path);ap.add_argument('source',type=Path)
    ap.add_argument('--output',type=Path,required=True);ap.add_argument('--font');a=ap.parse_args()
    build(a.game,a.source,a.output,choose_font(a.font))
