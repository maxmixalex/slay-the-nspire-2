#!/usr/bin/env python3
"""Extend the owner's NSP2/5 pack with the pinned Silent subset and original UI.

NSP2/6 keeps the old card/clip IDs. New cards, auxiliary assets and six clips are
appended. Existing sprites, scene images and animation payloads are preserved.
Only a verified local reference installation is accepted. No source assets or
installable font are included in the public source tree.
"""
from __future__ import annotations
from conversion_context import file_limit
import argparse, hashlib, json, math, struct
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageOps
from build_content import ROOT, CARDS, atlas, choose_font, description, fnv, image_data, prepared, texture
from extend_cards import ADDITIONAL
from ui_profile import UI_ASSETS, COLORLESS_CARDS
from reference_inventory import verify_release
from build_expansion import rle
from pck import Pack
from animation import Skeleton, load_textures, geometry, pose, raster, bake
from sparse_image import encode as sparse_encode
from lz4_block import encode as lz4_encode, decode as lz4_decode
from card_banner import material_tint

SILENT_CARDS=['STRIKE_SILENT','DEFEND_SILENT','NEUTRALIZE','SURVIVOR','ACROBATICS',
 'BACKFLIP','DAGGER_THROW','DEADLY_POISON','POISONED_STAB','CLOAK_AND_DAGGER',
 'BLADE_DANCE','DAGGER_SPRAY','DASH','LEG_SWEEP','FOOTWORK','NOXIOUS_FUMES',
 'BOUNCING_FLASK','PREPARED','SLICE','DEFLECT','SUCKER_PUNCH','ADRENALINE','ACCURACY','LEADING_STRIKE','SHIV']
ALL_CARDS=CARDS+ADDITIONAL+COLORLESS_CARDS+SILENT_CARDS
EXHAUSTS={'BLADE_DANCE','ADRENALINE','SHIV'}
NEW_UI=[
 ('SilentBackground','#silent',(320,240),'cover'),
 ('SilentPortrait','char_select_silent.png',(36,40),'contain'),
 ('SnakeRing','ring_of_the_snake.png',(25,25),'contain'),
 ('SilentEnergy','@card/energy_silent',(26,26),'contain'),
 ('Poison','poison_power.png',(13,13),'contain'),
 ('NoxiousFumes','noxious_fumes_power.png',(13,13),'contain'),
 ('Accuracy','accuracy_power.png',(13,13),'contain'),
 *[(color.title()+kind.title(),f'!frame:{color}:{kind}',(80,112),'stretch')
   for color in ('red','green','colorless') for kind in ('attack','skill','power')],
 ('SilentOrb','#orb',(42,42),'contain'),
 ('SilentMapMarker','@map/icons/map_marker_silent',(23,23),'contain')]
ALL_UI=UI_ASSETS+NEW_UI
NEW_CLIPS=[('silent','idle_loop',86,160,24),('silent','attack',86,160,24),
 ('silent','hurt',86,160,24),('silent','cast',86,160,24),('silent','shiv',86,160,24),
 ('restsite_silent','overgrowth_loop',80,108,16)]


def localized(text: str,key: str,upgraded: bool) -> str:
    text=text.replace('{TargetType:choose(AllEnemies):对所有敌人|}','')
    for variable in ('WeakPower','PoisonPower','DexterityPower','PoisonPerTurn','AccuracyPower'):
        text=text.replace('{'+variable+':diff()}','{p}')
    if key in ('CLOAK_AND_DAGGER','BLADE_DANCE'):
        text=text.replace('{Cards:diff()}','{s}')
    text=text.replace('{Shivs:diff()}','{s}').replace('{Repeat:diff()}','{r}')
    # The shared formatter deliberately rejects expressions not in its own scope.
    sentinel_s,sentinel_r='NSPCREATIONVALUE','NSPREPEATVALUE'
    text=text.replace('{s}',sentinel_s).replace('{r}',sentinel_r)
    text=description(text,upgraded).replace(sentinel_s,'{s}').replace(sentinel_r,'{r}')
    if key in EXHAUSTS:text+='\n[gold]消耗。[/gold]'
    return text


def build(game: Path, source: Path, output: Path, font: Path) -> dict:
    verify_release(game); b=source.read_bytes()
    if len(b)<40:raise ValueError('Truncated pack')
    magic,version,commit,size,ni,ng,ns,nc,nk,digest=struct.unpack_from('<4s9I',b)
    if magic!=b'NSP2' or version not in (5,6) or commit!=0x41CEF1EA or size!=len(b):
        raise ValueError('Expected intact pinned NSP2/5 or NSP2/6')
    expected=(26,11,56) if version==5 else (51,17,74)
    if (nk,nc)!=expected[:2] or ni>8192 or ng>8192 or ns>2048:raise ValueError('Incompatible directories')
    cd=40+ni*16+ng*16+ns*8+nc*16; end=cd+nk*16+expected[2]*4
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
    clips=[list(struct.unpack_from('<IHHhhI',b,cd-nc*16+i*16)) for i in range(nc)]
    records=[list(struct.unpack_from('<4I',b,cd+i*16)) for i in range(nk)]
    aux=list(struct.unpack_from('<'+str(expected[2])+'I',b,cd+nk*16))
    old_glyphs={}
    for i in range(ng):
        cp,o,px,w,h,advance,encoded=struct.unpack_from('<IIHBBHH',b,40+ni*16+i*16);raw=block(o,encoded or w*h)
        if encoded:
            if encoded%2:raise ValueError('Invalid glyph runs')
            raw=b''.join(bytes([raw[j+1]])*raw[j] for j in range(0,len(raw),2))
        if len(raw)!=w*h:raise ValueError('Invalid glyph coverage')
        old_glyphs[cp,px]=(w,h,advance,raw)
    animation_meta=[];pack=Pack(game/'SlayTheSpire2.pck')
    try:
        localization=json.loads(pack.read('localization/zhs/cards.json'))
        for i,key in enumerate(SILENT_CARDS,26):
            text=[localization[key+'.title'],localized(localization[key+'.description'],key,False),localized(localization[key+'.description'],key,True)]
            image=prepared(texture(pack,'images/packed/card_portraits/'+('token/' if key=='SHIV' else 'silent/')+key.lower()+'.png'),(82,60),'cover')
            im=(*image.size,0,image_data(image,False))
            if i<len(records):
                images[records[i][0]]=im
                for j in range(3):strings[records[i][j+1]]=text[j]
            else:
                index=len(strings);records.append([len(images),index,index+1,index+2]);strings.extend(text);images.append(im)
        textures={};header=(ROOT/'include/ui_assets.hpp').read_text()
        for i,(name,src,size,fit) in enumerate(NEW_UI,56):
            if f'U_{name}={i}' not in header:raise ValueError('Auxiliary header/profile drift: '+name)
            if src=='#silent':
                sk=Skeleton(pack.read(pack.imported('characterselect_silent.skel','.spskel')))
                tex=load_textures(pack,'characterselect_silent',texture)
                layers=geometry(sk,*pose(sk,'animation',0),tex)
                image=raster(layers,640/5120,np.array([0.,0.]),(640,300))
                image=ImageOps.fit(image,(320,240),Image.Resampling.LANCZOS,centering=(0.72,0.5))
            elif src=='#orb':
                image=texture(pack,'silent_orb_layer_1.png')
                for k in range(2,6):
                    layer=texture(pack,f'silent_orb_layer_{k}.png')
                    if layer.size!=image.size:layer=layer.resize(image.size,Image.Resampling.LANCZOS)
                    image.alpha_composite(layer)
            elif src.startswith('!frame:'):
                _,color,kind=src.split(':')
                image=material_tint(pack,atlas(pack,f'@card/card_frame_{kind}_s',textures),f'materials/cards/frames/card_frame_{color}_mat.tres')
            else:image=atlas(pack,src,textures)
            image=prepared(image,size,fit);alpha=image.getchannel('A').getextrema()[0]<255
            record=(*image.size,int(alpha),image_data(image,alpha))
            if i<len(aux):images[aux[i]]=record
            else:aux.append(len(images));images.append(record)
        cache={}
        for i,(skeleton,clip,height,width,fps) in enumerate(NEW_CLIPS,11):
            print(f'Sampling {skeleton}/{clip}',flush=True)
            if skeleton not in cache:
                sk=Skeleton(pack.read(pack.imported(skeleton+'.skel','.spskel')))
                cache[skeleton]=(sk,load_textures(pack,skeleton,texture))
            samples=max(1,math.ceil(cache[skeleton][0].animations[clip]['duration']*fps))
            frames,meta=bake(pack,skeleton,clip,texture,frames=samples,height=height,max_width=width,cache=cache)
            if i<len(clips):
                first,count=clips[i][:2]
                if count!=len(frames):raise ValueError('Animation sampling profile mismatch')
                for j,image in enumerate(frames):images[first+j]=(*image.size,4,sparse_encode(image))
            else:
                first=len(images);images.extend((*im.size,4,sparse_encode(im)) for im in frames)
                clips.append([first,len(frames),meta['frame_ms'],*meta['pivot'],0])
            animation_meta.append(dict(first_image=first,**meta))
        nc=len(clips);animations=b''.join(struct.pack('<IHHhhI',*r) for r in clips)
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
    data=struct.pack('<4s9I',b'NSP2',6,commit,40+len(body),ni,ng,ns,nc,nk,0)+body
    if len(data)>file_limit(6*1024*1024):raise ValueError(f'Pack exceeds conversion file bound: {len(data):,}')
    output.parent.mkdir(parents=True,exist_ok=True);temp=output.with_name(output.name+'.tmp');temp.write_bytes(data);temp.replace(output)
    manifest={'format':'NSP2/6','reference':'v0.111.0 / 41cef1ea','bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),
              'cards':ALL_CARDS,'card_records':records,'auxiliary_images':{a[0]:i for a,i in zip(ALL_UI,aux)},
              'images':ni,'glyphs':ng,'strings':ns,'clips':nc,'camp_flame_frames':160,'camp_flame_fps':8,'glyph_rle_bytes_saved':saved,'image_lz4_bytes_saved':image_saved,
              'scope':'24 Ironclad and 24 Silent pool models, 2 colorless cards and 1 generated Shiv; 17 relics and 16 potions; custom expedition.',
              'silent_animations':animation_meta,'excluded':['audio','multiplayer','timeline']}
    output.with_name('content-manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
    print(f'NSP2/6: {len(data):,} bytes, glyph RLE saves {saved:,}, {ni} images, {nk} cards')
    return manifest


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('game',type=Path);p.add_argument('source',type=Path)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--font')
    a=p.parse_args();build(a.game,a.source,a.output,choose_font(a.font))
