#!/usr/bin/env python3
"""Build the 0.8 NSP2/5 content extension from the owner's pinned installation.

Retains all existing animation pixels. Adds original UI textures/scene stills,
larger card portraits and two colorless cards. Glyph RLE is lossless, preserving
8-bit coverage. No game data or installable font belongs in the public tree.
"""
from __future__ import annotations
import argparse, hashlib, json, math, struct
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageOps
from build_content import ROOT, CARDS, atlas, choose_font, description, fnv, image_data, prepared, texture
from extend_cards import ADDITIONAL, EXHAUSTS
from ui_profile import UI_ASSETS, COLORLESS_CARDS
from reference_inventory import verify_release
from pck import Pack
from animation import Skeleton, load_textures, geometry, pose, raster
from lz4_block import encode as lz4_encode, decode as lz4_decode


def rle(raw: bytes) -> bytes:
    out=bytearray();at=0
    while at<len(raw):
        end=at+1
        while end<len(raw) and end-at<255 and raw[end]==raw[at]:end+=1
        out.extend((end-at,raw[at]));at=end
    return bytes(out) if len(out)<len(raw) else b''


def scene(pack: Pack, key: str) -> Image.Image:
    def draw(name: str):
        sk=Skeleton(pack.read(pack.imported(name+'.skel','.spskel')))
        textures=load_textures(pack,name,texture)
        return geometry(sk,*pose(sk,'animation',0),textures)
    if key=='#mainmenu':
        # Original background layers share their authored coordinate system.
        # 1920x1080 scene, converted at 2x small-screen resolution then 4:3 cropped.
        layers=draw('main_menu_bottom')+draw('main_menu_top')
        image=raster(layers,0.51*852/1920,np.array([0.,24*852/1920]),(852,480))
        return ImageOps.fit(image,(320,240),Image.Resampling.LANCZOS)
    if key=='#ironclad':
        layers=draw('characterselect_ironclad')
        # The full character-selection artwork is 5120x2400, not a standing sprite.
        image=raster(layers,640/5120,np.array([0.,0.]),(640,300))
        return ImageOps.fit(image,(320,240),Image.Resampling.LANCZOS,centering=(0.72,0.5))
    if key=='#battle':
        image=texture(pack,'overgrowth_00.png')
        for suffix in ('01_a','02_a','03_a','04_a'):
            layer=texture(pack,'overgrowth_'+suffix+'.png')
            if layer.size!=image.size:layer=layer.resize(image.size,Image.Resampling.LANCZOS)
            image.alpha_composite(layer)
        return ImageOps.fit(image,(320,218),Image.Resampling.LANCZOS,centering=(0.5,0.65))
    raise ValueError('Unknown scene profile '+key)


def build(game: Path, source: Path, output: Path, font: Path, cache_dir: Path | None=None) -> dict:
    verify_release(game)
    b=source.read_bytes()
    if len(b)<40:raise ValueError('Truncated pack')
    magic,version,commit,size,ni,ng,ns,nc,nk,digest=struct.unpack_from('<4s9I',b)
    if magic!=b'NSP2' or version not in (4,5) or commit!=0x41CEF1EA or size!=len(b) or fnv(b[40:])!=digest:
        raise ValueError('Expected intact pinned NSP2/4 or NSP2/5')
    if (version==4 and nk!=24) or (version==5 and nk!=26) or ni>8192 or ng>8192 or ns>2048 or nc!=11:
        raise ValueError('Incompatible directories')
    cd=40+ni*16+ng*16+ns*8+nc*16
    end=cd+nk*16+(len(UI_ASSETS)*4 if version==5 else 0)
    if end>len(b):raise ValueError('Truncated directories')
    def block(o,n):
        if o<end or n<0 or o>len(b)-n:raise ValueError('Payload outside pack')
        return b[o:o+n]
    images=[]
    for i in range(ni):
        o,w,h,f,n=struct.unpack_from('<IHHII',b,40+i*16);raw=block(o,n)
        if f==5:
            f,decoded=struct.unpack_from('<BI',raw);raw=lz4_decode(raw[5:],decoded)
        images.append((w,h,f,raw))
    strings=[]
    for i in range(ns):
        o,n=struct.unpack_from('<II',b,40+ni*16+ng*16+i*8);raw=block(o,n+1)
        if raw[-1]:raise ValueError('Unterminated string')
        strings.append(raw[:-1].decode('utf-8'))
    animations=b[cd-nc*16:cd]
    records=[list(struct.unpack_from('<4I',b,cd+i*16)) for i in range(nk)]
    aux=list(struct.unpack_from('<'+str(len(UI_ASSETS))+'I',b,cd+nk*16)) if version==5 else []
    old_glyphs={}
    for i in range(ng):
        cp,o,px,w,h,advance,encoded=struct.unpack_from('<IIHBBHH',b,40+ni*16+i*16)
        raw=block(o,encoded or w*h)
        if encoded:
            if encoded%2:raise ValueError('Invalid glyph runs')
            raw=b''.join(bytes([raw[j+1]])*raw[j] for j in range(0,len(raw),2))
        if len(raw)!=w*h:raise ValueError('Invalid glyph coverage')
        old_glyphs[cp,px]=(w,h,advance,raw)
    pack=Pack(game/'SlayTheSpire2.pck')
    try:
        localization=json.loads(pack.read('localization/zhs/cards.json'))
        for i,key in enumerate(CARDS+ADDITIONAL+COLORLESS_CARDS):
            parts=[localization[key+'.title'],description(localization[key+'.description'],False),description(localization[key+'.description'],True)]
            if key in EXHAUSTS:
                parts[1]+='\n[gold]消耗。[/gold]';parts[2]+='\n[gold]消耗。[/gold]'
            image=prepared(texture(pack,key.lower()+'.png'),(82,60),'cover')
            if i<len(records):
                image_id=records[i][0];images[image_id]=(82,60,0,image_data(image,False))
                for j in range(3):strings[records[i][j+1]]=parts[j]
            else:
                image_id,len_strings=len(images),len(strings)
                images.append((82,60,0,image_data(image,False)));strings.extend(parts)
                records.append([image_id,len_strings,len_strings+1,len_strings+2])
        texture_cache={}
        header=(ROOT/'include/ui_assets.hpp').read_text()
        for i,(name,src,size,fit) in enumerate(UI_ASSETS):
            if f'U_{name}={i}' not in header:raise ValueError('Auxiliary header/profile drift')
            if src=='#campflames':
                from camp_flames import bake as bake_flames
                frames=bake_flames(pack)
                if i<len(aux):
                    first=aux[i]
                    for j,im in enumerate(frames):images[first+j]=(*im.size,1,image_data(im,True))
                else:
                    aux.append(len(images));images.extend((*im.size,1,image_data(im,True)) for im in frames)
                continue
            cached=cache_dir/(name+'.png') if cache_dir else None
            # Cache is only for deterministic private scene conversion. Ordinary
            # UI textures are cheap, so always read them from the pinned PCK.
            if src.startswith('#') and cached and cached.is_file():image=Image.open(cached).convert('RGBA')
            else:
                if src.startswith('!banner:'):
                    from card_banner import material_banner
                    image=material_banner(pack,atlas(pack,'@card/card_banner',texture_cache),src.split(':')[1])
                else:
                    image=scene(pack,src) if src.startswith('#') else prepared(atlas(pack,src,texture_cache),size,fit)
                if cached and src.startswith('#'):
                    cached.parent.mkdir(parents=True,exist_ok=True);image.save(cached)
            image=prepared(image,size,fit)
            alpha=image.getchannel('A').getextrema()[0]<255
            record=(*image.size,int(alpha),image_data(image,alpha))
            if i<len(aux):images[aux[i]]=record
            else:aux.append(len(images));images.append(record)
    finally:pack.close()
    chars=set(''.join(strings))|set(chr(i) for i in range(32,127))|{chr(cp) for cp,_ in old_glyphs}
    for file in (ROOT/'src').iterdir():
        if file.suffix in ('.cpp','.inc'):chars.update(c for c in file.read_text() if ord(c)>127)
    chars={c for c in chars if ord(c)>=32}
    glyphs=[];saved=0
    for px in (10,12,16,20):
        f=ImageFont.truetype(str(font),px)
        for c in sorted(chars,key=ord):
            cp=ord(c)
            if (cp,px) in old_glyphs:w,h,advance,raw=old_glyphs[cp,px]
            else:
                advance=max(1,math.ceil(f.getlength(c)));w=max(advance,f.getbbox(c)[2]);h=px+5
                im=Image.new('L',(w,h));ImageDraw.Draw(im).text((0,px+1),c,font=f,fill=255,anchor='ls');raw=im.tobytes()
            if w>40 or h>40 or advance>40:raise ValueError('Glyph exceeds runtime bounds')
            compressed=rle(raw);saved+=len(raw)-(len(compressed) if compressed else len(raw))
            glyphs.append((cp,px,w,h,advance,len(compressed),compressed or raw))
    compressed_images=[];image_saved=0
    for w,h,fmt,raw in images:
        compressed=struct.pack('<BI',fmt,len(raw))+lz4_encode(raw)
        if len(compressed)<len(raw):
            image_saved+=len(raw)-len(compressed);compressed_images.append((w,h,5,compressed))
        else:compressed_images.append((w,h,fmt,raw))
    images=compressed_images
    ni,ng,ns,nk=len(images),len(glyphs),len(strings),len(records)
    if ni>8192 or ng>8192 or ns>2048:raise ValueError('Runtime directory capacity exceeded')
    start=40+ni*16+ng*16+ns*8+nc*16+nk*16+len(aux)*4
    payload=bytearray();unique={}
    def append(raw,align=1):
        # Exact deduplication only; no animation resampling or lossy frame changes.
        key=(align,raw)
        if key in unique:return unique[key]
        while (start+len(payload))%align:payload.append(0)
        o=start+len(payload);payload.extend(raw);unique[key]=o;return o
    idata=b''.join(struct.pack('<IHHII',append(raw,2),w,h,fmt,len(raw)) for w,h,fmt,raw in images)
    gdata=b''.join(struct.pack('<IIHBBHH',cp,append(raw),px,w,h,advance,encoded) for cp,px,w,h,advance,encoded,raw in glyphs)
    sdata=b''.join(struct.pack('<II',append(t.encode()+b'\0'),len(t.encode())) for t in strings)
    cdata=b''.join(struct.pack('<4I',*r) for r in records)
    body=idata+gdata+sdata+animations+cdata+struct.pack('<'+str(len(aux))+'I',*aux)+payload
    data=struct.pack('<4s9I',b'NSP2',5,commit,40+len(body),ni,ng,ns,nc,nk,fnv(body))+body
    if len(data)>6*1024*1024:raise ValueError(f'Pack exceeds resident buffer: {len(data):,}')
    output.parent.mkdir(parents=True,exist_ok=True);temp=output.with_name(output.name+'.tmp');temp.write_bytes(data);temp.replace(output)
    manifest={'format':'NSP2/5','reference':'v0.111.0 / 41cef1ea','bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),
              'cards':CARDS+ADDITIONAL+COLORLESS_CARDS,'card_records':records,'auxiliary_images':{a[0]:i for a,i in zip(UI_ASSETS,aux)},
              'images':ni,'glyphs':ng,'strings':ns,'clips':nc,'camp_flame_frames':160,'camp_flame_fps':8,'glyph_rle_bytes_saved':saved,'image_lz4_bytes_saved':image_saved,
              'scope':'26 modeled cards, 16 relics, 16 potions; original-derived UI; legacy expedition remains custom.'}
    output.with_name('content-manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
    print(f'NSP2/5: {len(data):,} bytes, glyph RLE saves {saved:,}, {ni} images, {nk} cards')
    return manifest

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('game',type=Path);p.add_argument('source',type=Path)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--font');p.add_argument('--scene-cache',type=Path)
    a=p.parse_args();build(a.game,a.source,a.output,choose_font(a.font),a.scene_cache)
