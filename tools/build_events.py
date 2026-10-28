#!/usr/bin/env python3
"""Append original event layouts/text and merchant art to the owner's NSP2/9 pack.

Only the supported v0.111.0 installation is read. Old image, glyph and clip
payloads are retained; new original art is downsampled offline, never downloaded.
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
from build_content import ROOT, choose_font, fnv, image_data, prepared, texture
from build_expansion import rle
from build_tactics import compact_image
from pck import Pack
from reference_inventory import verify_release

LIMIT=6*1024*1024
EVENTS=[('ABYSSAL_BATHS','ABSTAIN','EXIT_BATHS','IMMERSE','ABSTAIN'),
        ('DENSE_VEGETATION','TRUDGE_ON','REST','TRUDGE_ON','REST'),
        ('MORPHIC_GROVE','GROUP','LONER','GROUP','LONER'),
        ('AROMA_OF_CHAOS','LET_GO','MAINTAIN_CONTROL','LET_GO','MAINTAIN_CONTROL'),
        ('DOORS_OF_LIGHT_AND_DARK','LIGHT','DARK','LIGHT','DARK')]
ART=['abyssal_baths','dense_vegetation','morphic_grove','aroma_of_chaos','doors_of_light_and_dark']

class PackData:
    """Bounded directory reader/writer shared by conversion regression tests."""
    def __init__(self,data:bytes):
        if not 40<=len(data)<=LIMIT:raise ValueError('Pack size outside resident limit')
        data=bytes(data)
        magic,self.version,commit,n,ni,ng,ns,nc,nk,digest=struct.unpack_from('<4s9I',data)
        if (magic!=b'NSP2' or self.version not in (9,10) or commit!=0x41cef1ea or
            n!=len(data)):raise ValueError('Expected NSP2/9 or NSP2/10')
        nu=95 if self.version==9 else 105
        if ni>8192 or ng>8192 or ns!=(228 if self.version==9 else 269) or nc!=17 or nk!=68:
            raise ValueError('Incompatible directory profile')
        clips_at=40+ni*16+ng*16+ns*8;cards_at=clips_at+nc*16;aux_at=cards_at+nk*16
        end=aux_at+nu*4
        if end>n:raise ValueError('Truncated directories')
        def block(off:int,count:int)->bytes:
            if off<end or count<0 or off>n-count:raise ValueError('Payload outside pack')
            return data[off:off+count]
        self.images=[]
        for i in range(ni):
            off,w,h,fmt,count=struct.unpack_from('<IHHII',data,40+i*16)
            if not 0<w<=320 or not 0<h<=240 or fmt>5 or off%2:raise ValueError('Invalid image')
            self.images.append((w,h,fmt,block(off,count)))
        self.glyphs={}
        for i in range(ng):
            cp,off,px,w,h,advance,enc=struct.unpack_from('<IIHBBHH',data,40+ni*16+i*16)
            if cp>0x10ffff or not 0<w<=40 or not 0<h<=40 or advance>40 or px not in (8,10,12,16,20):raise ValueError('Invalid glyph')
            if (cp,px) in self.glyphs:raise ValueError('Duplicate glyph')
            raw=block(off,enc or w*h)
            if enc and (enc%2 or not all(raw[::2]) or sum(raw[::2])!=w*h):raise ValueError('Invalid glyph runs')
            self.glyphs[cp,px]=(w,h,advance,enc,raw)
        self.strings=[]
        for i in range(ns):
            off,count=struct.unpack_from('<II',data,40+ni*16+ng*16+i*8);raw=block(off,count+1)
            if raw[-1] or b'\0' in raw[:-1]:raise ValueError('Invalid text termination')
            self.strings.append(raw[:-1].decode('utf-8'))
        self.clips=data[clips_at:cards_at]
        self.cards=[struct.unpack_from('<4I',data,cards_at+i*16) for i in range(nk)]
        self.aux=list(struct.unpack_from('<'+'I'*nu,data,aux_at))
        if any(r[0]>=ni or any(t>=ns for t in r[1:]) for r in self.cards) or any(i>=ni for i in self.aux):raise ValueError('Invalid directory reference')
        for i in range(nc):
            first,count,ms,px,py,reserved=struct.unpack_from('<IHHhhI',self.clips,i*16)
            if not 0<count<=256 or first+count>ni or not 0<ms<=1000 or reserved:raise ValueError('Invalid clip')
    def encode(self)->bytes:
        ni,ng,ns,nk,nu=len(self.images),len(self.glyphs),len(self.strings),len(self.cards),len(self.aux)
        if ni>8192 or ng>8192 or ns!=269 or nk!=68 or nu!=105:raise ValueError('Output profile exceeded')
        start=40+ni*16+ng*16+ns*8+17*16+nk*16+nu*4
        payload=bytearray();unique={}
        def add(raw:bytes,align:int=1)->int:
            key=(align,raw)
            if key in unique:return unique[key]
            while (start+len(payload))%align:payload.append(0)
            off=start+len(payload);unique[key]=off;payload.extend(raw);return off
        images=b''.join(struct.pack('<IHHII',add(raw,2),w,h,fmt,len(raw)) for w,h,fmt,raw in self.images)
        glyphs=b''.join(struct.pack('<IIHBBHH',cp,add(raw),px,w,h,advance,enc) for (cp,px),(w,h,advance,enc,raw) in sorted(self.glyphs.items(),key=lambda p:(p[0][1],p[0][0])))
        strings=b''.join(struct.pack('<II',add(t.encode()+b'\0'),len(t.encode())) for t in self.strings)
        body=images+glyphs+strings+self.clips+b''.join(struct.pack('<4I',*r) for r in self.cards)+struct.pack('<'+'I'*nu,*self.aux)+payload
        output=struct.pack('<4s9I',b'NSP2',10,0x41cef1ea,40+len(body),ni,ng,ns,17,nk,0)+body
        if len(output)>LIMIT:raise ValueError(f'Pack exceeds resident buffer: {len(output)}')
        return output

def build(game:Path,source:Path,output:Path,font_path:Path)->dict:
    verify_release(game);pack=PackData(source.read_bytes())
    old_images=pack.images[:1324];old_glyphs=pack.glyphs.copy();old_clips=pack.clips
    pck=Pack(game/'SlayTheSpire2.pck')
    try:
        texts=json.loads(pck.read('localization/zhs/events.json'));chars=json.loads(pck.read('localization/zhs/characters.json'))
        appended=[]
        for key,finish_a,finish_b,option_a,option_b in EVENTS:
            get=lambda part:texts[key+'.'+part]
            parts=[get('title'),get('pages.INITIAL.description'),get('pages.'+finish_a+'.description'),get('pages.'+finish_b+'.description')]
            for option in (option_a,option_b):parts += [get('pages.INITIAL.options.'+option+'.title'),get('pages.INITIAL.options.'+option+'.description')]
            appended+=parts
        appended.append(texts['AROMA_OF_CHAOS.pages.MAINTAIN_CONTROL.description'].replace('{AromaPrinciple}',chars['SILENT.aromaPrinciple']))
        appended=[s.replace('{IsMultiplayer:所有人|}','').replace('{AromaPrinciple}',chars['IRONCLAD.aromaPrinciple']) for s in appended]
        allowed={'MaxHp','Damage','Heal','Gold','HpLoss','Cards'}
        for text in appended:
            if set(re.findall(r'\{([^{}]*)\}',text))-allowed:raise ValueError('Unresolved localization expression')
        pack.strings[228:]=appended
        profile=[(f'EventFull{i}',name+'.png',(320,218),'cover') for i,name in enumerate(ART)]
        profile += [('EventButton','images/packed/common_ui/event_button.png',(178,39),'stretch'),
                    ('EventOutline','images/packed/common_ui/event_button_outline.png',(178,39),'stretch'),
                    ('ShopRemove','images/rooms/merchant_room/card_removal_00.png',(51,51),'contain'),
                    ('ShopRemoveUsed','images/rooms/merchant_room/card_removal_05.png',(51,51),'contain'),
                    ('MerchantRug','images/rooms/merchant_room/shop_rug.png',(320,180),'stretch')]
        header=(ROOT/'include/ui_assets.hpp').read_text()
        for index,(name,path,size,fit) in enumerate(profile,95):
            if f'U_{name}={index}' not in header:raise ValueError('Asset index mismatch: '+name)
            image=prepared(texture(pck,path),size,fit)
            alpha=image.getchannel('A').getextrema()[0]<255
            fmt,raw=compact_image(int(alpha),image_data(image,alpha));record=(*image.size,fmt,raw)
            if index<len(pack.aux):pack.images[pack.aux[index]]=record
            else:pack.aux.append(len(pack.images));pack.images.append(record)
    finally:pck.close()
    alphabet=set(''.join(pack.strings))|{chr(cp) for cp,px in pack.glyphs}
    for folder in ('src','include'):
        for path in (ROOT/folder).rglob('*'):
            if path.suffix in ('.cpp','.inc','.hpp'):alphabet.update(path.read_text())
    alphabet={ch for ch in alphabet if ord(ch)>=32}
    for px in (8,10,12,16,20):
        font=ImageFont.truetype(str(font_path),px)
        for ch in sorted(alphabet,key=ord):
            if (ord(ch),px) in pack.glyphs:continue
            advance=max(1,math.ceil(font.getlength(ch)));w=max(advance,font.getbbox(ch)[2]);h=px+5
            if w>40 or advance>40:raise ValueError('Font outside runtime bounds')
            image=Image.new('L',(w,h));ImageDraw.Draw(image).text((0,px+1),ch,font=font,fill=255,anchor='ls')
            raw=image.tobytes();enc=rle(raw);pack.glyphs[ord(ch),px]=(w,h,advance,len(enc),enc or raw)
    if pack.images[:1324]!=old_images or pack.clips!=old_clips or any(pack.glyphs[k]!=v for k,v in old_glyphs.items()):raise ValueError('Retained payload changed')
    data=pack.encode();PackData(data)
    output.parent.mkdir(parents=True,exist_ok=True);tmp=output.with_name(output.name+'.tmp');tmp.write_bytes(data);tmp.replace(output)
    result={'format':'NSP2/10','reference':'v0.111.0 / 41cef1ea','bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),
            'images':len(pack.images),'glyphs':len(pack.glyphs),'strings':len(pack.strings),'cards':68,'relics':30,'potions':16,
            'events':5,'clips':17,'auxiliary_images':len(pack.aux),'legacy_image_payloads_unchanged':1324,
            'remaining_resident_bytes':LIMIT-len(data),'excluded':['audio','multiplayer','timeline'],
            'scope':'Five adapted events in the custom expedition; original event pool and RNG parity are not claimed.'}
    output.with_name('content-manifest.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps(result,ensure_ascii=False),flush=True);return result

if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('game',type=Path);ap.add_argument('source',type=Path)
    ap.add_argument('--output',type=Path,required=True);ap.add_argument('--font');args=ap.parse_args()
    build(args.game,args.source,args.output,choose_font(args.font))
