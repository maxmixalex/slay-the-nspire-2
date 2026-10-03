#!/usr/bin/env python3
"""Create a private, bounded calculator content pack from the player's STS2 install.

Only v0.111.0 / 41cef1ea is supported. No game executable is run. The converter
reads the PCK, samples Spine animation tracks, and rasterizes a glyph subset.
"""
from __future__ import annotations
import argparse
import hashlib
import io
import json
import math
from pathlib import Path
import re
import struct
from PIL import Image, ImageDraw, ImageFont, ImageOps
import numpy as np
from pck import Pack
from reference_inventory import verify_release
from setup_pose import decode_texture
from animation import Skeleton, bake, load_textures
from sparse_image import encode as encode_sparse
from asset_profile import ASSETS, CLIPS, EVENT_STRINGS

CARDS = ['STRIKE_IRONCLAD','DEFEND_IRONCLAD','BASH','ANGER','POMMEL_STRIKE','SHRUG_IT_OFF',
         'BURNING_PACT','TRUE_GRIT','TWIN_STRIKE','INFLAME','IRON_WAVE','WHIRLWIND','FEEL_NO_PAIN','DARK_EMBRACE']
GAME_COMMIT='41cef1ea'
ROOT=Path(__file__).resolve().parents[1]

def texture(pack:Pack,filename:str)->Image.Image:
    name=pack.imported(filename,'.ctex')
    data=pack.read(name)
    if b'RIFF' in data[:80]:
        return decode_texture(data)
    if data[:4]!=b'GST2' or len(data)<52:
        raise ValueError(f'Unsupported texture: {name}')
    w,h=struct.unpack_from('<HH',data,40)
    fmt=struct.unpack_from('<I',data,48)[0]
    codes={17:b'DXT1',18:b'DXT3',19:b'DXT5',22:b'DX10'}
    if fmt not in codes:
        raise ValueError(f'Unsupported GPU texture format {fmt}: {name}')
    block=8 if fmt==17 else 16
    size=((w+3)//4)*((h+3)//4)*block
    if not w or not h or len(data)<52+size:raise ValueError('Truncated texture')
    fourcc=int.from_bytes(codes[fmt],'little')
    header=struct.pack('<7I',124,0x81007,h,w,size,0,1)+bytes(44)
    header+=struct.pack('<8I',32,4,fourcc,0,0,0,0,0)+struct.pack('<5I',0x1000,0,0,0,0)
    dx10=struct.pack('<5I',98,3,0,1,0) if fmt==22 else b''
    return Image.open(io.BytesIO(b'DDS '+header+dx10+data[52:52+size])).convert('RGBA')

def image_data(im:Image.Image,alpha:bool)->bytes:
    a=np.asarray(im.convert('RGBA'),dtype=np.uint16)
    p=((a[:,:,0]>>3)<<11)|((a[:,:,1]>>2)<<5)|(a[:,:,2]>>3)
    return p.astype('<u2').tobytes()+(a[:,:,3].astype('u1').tobytes() if alpha else b'')

def description(s:str,upgraded:bool)->str:
    s=re.sub(r'\[/?[a-zA-Z_]+\]','',s)
    s=s.replace('{IfUpgraded:show:| 随机}', '' if upgraded else '随机')
    replacements={'Damage':'d','Block':'b','Cards':'n','VulnerablePower':'p','StrengthPower':'p','Power':'p'}
    for key,value in replacements.items():s=s.replace('{'+key+':diff()}','{'+value+'}')
    remaining=re.findall(r'\{[^}]*\}',s)
    if any(x not in ('{d}','{b}','{n}','{p}') for x in remaining):raise ValueError('Unsupported localization expression: '+s)
    return s

def choose_font(explicit:str|None)->Path:
    if explicit:
        p=Path(explicit)
        if not p.is_file():raise ValueError(f'Font does not exist: {p}')
        return p
    candidates=[Path('/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc'),Path('C:/Windows/Fonts/msyh.ttc'),Path('/System/Library/Fonts/PingFang.ttc')]
    for p in candidates:
        if p.is_file():return p
    raise ValueError('Use --font with a Chinese-capable TrueType/OpenType font. Font files are never copied to the output.')

def fnv(data:bytes)->int:
    h=2166136261
    for b in data:h=((h^b)*16777619)&0xffffffff
    return h

def atlas(pack:Pack,name:str,cache:dict)->Image.Image:
    if name.startswith('!'):
        skeleton,region=name[1:].split(':',1)
        key='spine/'+skeleton
        if key not in cache:cache[key]=load_textures(pack,skeleton,texture)
        return Image.fromarray(np.uint8(np.clip(cache[key][region]*255,0,255)))
    if not name.startswith('@'):
        if name not in cache:cache[name]=texture(pack,name)
        return cache[name].copy()
    path='images/atlases/ui_atlas.sprites/'+name[1:]+'.tres'
    text=pack.read(path).decode('utf-8')
    filename=re.search(r'path="res://[^\"]*/([^/\"]+\.png)"',text)[1]
    x,y,w,h=map(int,re.search(r'region = Rect2\(([^)]+)\)',text)[1].split(','))
    if filename not in cache:cache[filename]=texture(pack,filename)
    return cache[filename].crop((x,y,x+w,y+h))

def prepared(im:Image.Image,size:tuple,fit:str)->Image.Image:
    im=im.convert('RGBA')
    if fit=='trim':
        box=im.getbbox()
        if box:im=im.crop(box)
        fit='contain'
    if fit=='stretch':return im.resize(size,Image.Resampling.LANCZOS)
    if fit=='cover':return ImageOps.fit(im,size,Image.Resampling.LANCZOS)
    small=ImageOps.contain(im,size,Image.Resampling.LANCZOS)
    result=Image.new('RGBA',size);result.alpha_composite(small,((size[0]-small.width)//2,(size[1]-small.height)//2));return result

def encoded_image(im:Image.Image,alpha:bool,indexed:bool)->tuple[int,bytes]:
    if not indexed:return int(alpha),image_data(im,alpha)
    return 4,encode_sparse(im)

def main()->None:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('game',type=Path,help='Directory containing SlayTheSpire2.pck and release_info.json')
    ap.add_argument('--output',type=Path,default=ROOT/'dist')
    ap.add_argument('--font',help='Local Chinese font; only rasterized glyphs enter the pack')
    args=ap.parse_args()
    info=json.loads((args.game/'release_info.json').read_text(encoding='utf-8-sig'))
    if info.get('version')!='v0.111.0' or info.get('commit')!=GAME_COMMIT:
        raise ValueError('This rule subset supports only v0.111.0 / 41cef1ea.')
    verify_release(args.game)
    pack=Pack(args.game/'SlayTheSpire2.pck');texture_cache={};skeleton_cache={}
    cards=json.loads(pack.read('localization/zhs/cards.json'))
    monsters=json.loads(pack.read('localization/zhs/monsters.json'))
    strings=[]
    for key in CARDS:strings += [cards[key+'.title'],description(cards[key+'.description'],False),description(cards[key+'.description'],True)]
    strings += [monsters.get('NIBBIT.name','Nibbit'),'铁甲战士']
    events=json.loads(pack.read('localization/zhs/events.json'))
    for key in EVENT_STRINGS:strings.append(events[key])
    assert len(strings)==66
    print('Converting original scene layers and interface textures…',flush=True)
    background=texture(pack,'overgrowth_00.png')
    for suffix in ['01_a','02_a','03_a','04_a']:
        layer=texture(pack,'overgrowth_'+suffix+'.png')
        if layer.size!=background.size:layer=layer.resize(background.size,Image.Resampling.LANCZOS)
        background.alpha_composite(layer)
    background=background.convert('RGB')
    battle=ImageOps.fit(background,(320,130),Image.Resampling.LANCZOS,centering=(0.5,0.65))
    title=ImageOps.fit(background,(320,240),Image.Resampling.LANCZOS)
    hero,_=bake(pack,'ironclad','idle_loop',texture,frames=1,height=86,max_width=160,cache=skeleton_cache)
    enemy,_=bake(pack,'nibbit','idle_loop',texture,frames=1,height=40,max_width=110,cache=skeleton_cache)
    logo=prepared(texture(pack,'sts2_logo_static.png'),(230,67),'contain')
    blood=prepared(texture(pack,'burning_blood.png'),(16,16),'contain')
    pictures=[(battle,False,False),(title,False,False),(hero[0],True,True),(enemy[0],True,True),(logo,True,True),(blood,True,False)]
    for key in CARDS:pictures.append((prepared(texture(pack,key.lower()+'.png'),(41,30),'cover'),False,False))
    header=(ROOT/'include/assets.hpp').read_text()
    for i,(name,source,size,fit) in enumerate(ASSETS,20):
        if 'A_'+name+'='+str(i) not in header:raise ValueError('Asset index/header mismatch: '+name)
        im=prepared(atlas(pack,source,texture_cache),size,fit)
        alpha=im.getchannel('A').getextrema()[0]<255
        pictures.append((im,alpha,False))
    # Converted static images no longer need their full-resolution source atlases.
    texture_cache.clear()
    animation_records=[];animation_meta=[]
    for name,skeleton,original,fps,height,width in CLIPS:
        # Keep scale entries, but retain only the active full-resolution skeleton.
        for cached in list(skeleton_cache):
            if isinstance(cached,str) and cached!=skeleton:del skeleton_cache[cached]
        print(f'  Sampling {skeleton}/{original}: {fps} fps',flush=True)
        if skeleton not in skeleton_cache:
            sk=Skeleton(pack.read(pack.imported(skeleton+'.skel','.spskel')))
            skeleton_cache[skeleton]=(sk,load_textures(pack,skeleton,texture))
        samples=max(1,math.ceil(skeleton_cache[skeleton][0].animations[original]['duration']*fps))
        frames,meta=bake(pack,skeleton,original,texture,frames=samples,height=height,max_width=width,cache=skeleton_cache,exclude_regions=('bg','pink_highlights') if name=='MerchantBottom' else ())
        first=len(pictures);pictures.extend((im,True,True) for im in frames)
        animation_records.append((first,len(frames),meta['frame_ms'],*meta['pivot'],0))
        animation_meta.append(dict(id=name,first_image=first,**meta))
    chars=set(''.join(strings))|set(chr(c) for c in range(32,127))
    for file in [p for p in (ROOT/'src').iterdir() if p.suffix in ('.cpp', '.inc')]:chars.update(c for c in file.read_text(encoding='utf-8') if ord(c)>127)
    chars.discard('\n');chars.discard('\r');chars.update('▼·（）')
    font_path=choose_font(args.font);glyphs=[]
    for size in (10,12,16,20):
        font=ImageFont.truetype(str(font_path),size)
        for ch in sorted(chars,key=ord):
            advance=max(1,math.ceil(font.getlength(ch)));w=max(advance,font.getbbox(ch)[2]);h=size+5
            bitmap=Image.new('L',(w,h));ImageDraw.Draw(bitmap).text((0,size+1),ch,font=font,fill=255,anchor='ls')
            glyphs.append((ord(ch),size,w,h,advance,bitmap.tobytes()))
    start=40+len(pictures)*16+len(glyphs)*16+len(strings)*8+len(animation_records)*16
    payload=bytearray();images_dir=bytearray();glyph_dir=bytearray();text_dir=bytearray()
    def add(data:bytes,align:int=1)->int:
        while (start+len(payload))%align:payload.append(0)
        off=start+len(payload);payload.extend(data);return off
    for im,alpha,indexed in pictures:
        fmt,data=encoded_image(im,alpha,indexed);off=add(data,2)
        images_dir.extend(struct.pack('<IHHII',off,*im.size,fmt,len(data)))
    for cp,size,w,h,advance,data in glyphs:
        off=add(data);glyph_dir.extend(struct.pack('<IIHBBHH',cp,off,size,w,h,advance,0))
    for s in strings:
        b=s.encode('utf-8');off=add(b+b'\0');text_dir.extend(struct.pack('<II',off,len(b)))
    anim_dir=b''.join(struct.pack('<IHHhhI',*r) for r in animation_records)
    body=bytes(images_dir+glyph_dir+text_dir+anim_dir+payload)
    result=struct.pack('<4s9I',b'NSP2',3,0x41CEF1EA,40+len(body),len(pictures),len(glyphs),len(strings),len(animation_records),0,fnv(body))+body
    if len(result)>6*1024*1024:raise ValueError(f'Content exceeds the 6 MiB bound: {len(result)}')
    args.output.mkdir(parents=True,exist_ok=True)
    output=args.output/'nSpire2.nsp.tns';output.write_bytes(result)
    manifest={'format':'NSP2/3','game':info,'content_sha256':hashlib.sha256(result).hexdigest(),'bytes':len(result),
      'cards':CARDS,'images':len(pictures),'glyphs':len(glyphs),'language':'zhs','animations':animation_meta,
      'scope':'14-card subset; original UI and offline sampled original animations; custom twelve-floor branching expedition; three rich-text events; ten modeled relics; no original map/seed/save parity.'}
    (args.output/'content-manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
    pack.close();print(f'Created {output}: {len(result):,} bytes, {len(pictures)} images, {len(glyphs)} glyphs',flush=True)

if __name__=='__main__':main()
