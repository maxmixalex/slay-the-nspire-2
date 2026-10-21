#!/usr/bin/env python3
"""Build private Chinese and English NSP2/17 editions from a complete v16 pack.

Every original image, compressed animation group, clip and card image index
survives exactly. Only directories, strings, glyphs and new static HUD skins
are rebuilt. No animation decoding, resampling or recompression is performed.
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
import content_tiles as tiles
from lz4_block import encode as lz4_encode

ROOT = Path(__file__).resolve().parents[1]
MARKER, EXTRA_BASE, UI_BASE, STRING_COUNT, AUX_COUNT = 6110, 6111, 7168, 8192, 1236
FONT_SIZES = (8, 10, 12, 16, 20)


def repack(data, auxiliary_count, texts, glyphs, skins=(), *, profile=None):
    """Compact live payloads without changing existing graphic bytes.

    glyphs is a sorted iterable of (codepoint, size, width, height, advance,
    encoded_length, bytes); skins are independent dense images. This function
    also accepts small synthetic catalogs for boundary tests.
    """
    source = tiles.PackView(data, auxiliary_count, versions=(16,), profile=profile)
    old = tiles.TileReader(source)
    ni, _ng, _ns, nc, nk, nu = source.counts
    ni2, ng, ns, nu2 = ni + len(skins), len(glyphs), len(texts), nu + len(skins)
    tiles.require(0 < ng <= 32768 and 0 < ns <= 16384 and ni2 <= 65535, 'Locale catalog outside bound')
    base = 40 + ni2*16 + ng*16 + ns*8 + nc*16 + nk*16 + nu2*4
    group_count = old.tile_groups + old.index_groups
    table = base + tiles.GLOBAL.size
    descriptors = table + group_count*8
    descriptor_end = descriptors + old.frame_count*tiles.DESCRIPTOR.size
    tiles.require(table + group_count*8 <= tiles.DIRECTORY_LIMIT, 'Locale directory exceeds resident bound')
    out = bytearray(descriptor_end)
    def append(raw, align=1):
        if align == 2 and len(out) & 1:
            out.append(0)
        off = len(out); out.extend(raw); return off
    old_header = list(tiles.GLOBAL.unpack_from(data, source.base_end))
    old_header[8], old_header[9] = descriptors, table
    tiles.GLOBAL.pack_into(out, base, *old_header)
    old_table = tiles.GLOBAL.unpack_from(data, source.base_end)[9]
    graphic_digest = hashlib.sha256()
    for i in range(group_count):
        off, length = struct.unpack_from('<II', data, old_table+i*8)
        raw = data[off:off+length]
        graphic_digest.update(raw)
        at = append(raw, 2)
        struct.pack_into('<II', out, table+i*8, at, length)
    for i in range(old.frame_count):
        at = old.descriptors+i*16
        global_at, group, offset, length = tiles.DESCRIPTOR.unpack_from(data, at)
        tiles.require(global_at == source.base_end, 'Invalid historical descriptor')
        tiles.DESCRIPTOR.pack_into(out, descriptors+i*16, base, group, offset, length)
    copied = {}
    for i, (off,w,h,fmt,length) in enumerate(source.images):
        if fmt == 6:
            at = descriptors + off-old.descriptors
            graphic_digest.update(data[off+4:off+length])
        else:
            key = (off,length)
            if key not in copied:
                copied[key] = append(data[off:off+length], 2)
            at = copied[key]
            graphic_digest.update(data[off:off+length])
        tiles.IMAGE.pack_into(out,40+i*16,at,w,h,fmt,length)
    for i,(w,h,fmt,raw) in enumerate(skins):
        tiles.require(0<w<=320 and 0<h<=240 and fmt in (0,1,2,3), 'Invalid new HUD image')
        tiles.validate_raw(raw,w,h,fmt)
        encoded = struct.pack('<BI',fmt,len(raw)) + lz4_encode(raw)
        stored,payload = (5,encoded) if len(encoded)<len(raw) else (fmt,raw)
        at = append(payload,2)
        tiles.IMAGE.pack_into(out,40+(ni+i)*16,at,w,h,stored,len(payload))
    glyph_at = 40+ni2*16
    for i,(cp,px,w,h,advance,encoded,raw) in enumerate(glyphs):
        at = append(raw)
        tiles.GLYPH.pack_into(out,glyph_at+i*16,cp,at,px,w,h,advance,encoded)
    string_at = glyph_at+ng*16
    for i,text in enumerate(texts):
        tiles.require(isinstance(text,str), 'Unfilled locale string')
        raw = text.encode('utf-8')
        tiles.require(len(raw)<tiles.STRING_LIMIT and b'\0' not in raw, 'Locale string exceeds bound')
        at = append(raw+b'\0')
        struct.pack_into('<II',out,string_at+i*8,at,len(raw))
    clip_at = string_at+ns*8
    out[clip_at:clip_at+nc*16] = data[source.clip_at:source.card_at]
    card_at = clip_at+nc*16
    out[card_at:card_at+nk*16] = data[source.card_at:source.aux_at]
    aux_at = card_at+nk*16
    struct.pack_into('<'+'I'*nu2,out,aux_at,*(source.aux+list(range(ni,ni2))))
    tiles.require(len(out)<=tiles.FILE_LIMIT, 'Locale pack exceeds file bound')
    tiles.HEADER.pack_into(out,0,b'NSP2',17,tiles.REFERENCE,len(out),ni2,ng,ns,nc,nk,tiles.fnv(memoryview(out)[40:]))
    target = tiles.PackView(out,nu2,versions=(17,),profile=(nk,ns,nc,nu2) if profile else None)
    new = tiles.TileReader(target)
    tiles.require(target.cards==source.cards and target.clips==source.clips and target.aux[:nu]==source.aux,
                  'Historical graphic catalog changed')
    for i in range(group_count):
        oldoff,oldlen=struct.unpack_from('<II',data,old_table+i*8)
        newoff,newlen=struct.unpack_from('<II',out,table+i*8)
        tiles.require(oldlen==newlen and data[oldoff:oldoff+oldlen]==out[newoff:newoff+newlen],
                      'Compressed animation group changed')
    for original,revised in zip(source.images,target.images):
        off,w,h,fmt,length=original;at,w2,h2,fmt2,length2=revised
        tiles.require((w,h,fmt,length)==(w2,h2,fmt2,length2),'Historical image format changed')
        skip=4 if fmt==6 else 0
        tiles.require(data[off+skip:off+length]==out[at+skip:at+length], 'Historical image bytes changed')
    for i,skin in enumerate(skins):
        tiles.require(target.image_raw(ni+i)==skin,'HUD image roundtrip mismatch')
    tiles.require((old.tile_count,old.frame_count)==(new.tile_count,new.frame_count),'Tile counts changed')
    return bytes(out),dict(input_bytes=len(data),output_bytes=len(out),images=ni2,glyphs=ng,strings=ns,
        clips=nc,cards=nk,auxiliary=nu2,input_sha256=tiles.sha256(data),output_sha256=tiles.sha256(out),
        original_graphics_sha256=graphic_digest.hexdigest(),unchanged_original_images=True,
        unchanged_compressed_animation_groups=True,unchanged_clip_and_card_image_catalog=True,
        appended_static_images=len(skins),discarded_obsolete_text_and_glyph_payload=True)


def glyph_subset(texts, font_path, language, legacy_literals=()):
    alphabet = set(''.join(texts)) | set(''.join(legacy_literals)) | set(chr(i) for i in range(32,127))
    # Runtime punctuation and input hints are independent of private locale text.
    alphabet.update('←→↑↓‹›×…−·')
    alphabet = sorted((ch for ch in alphabet if ord(ch)>=32 and (not ch.isspace() or ch==' ')),key=ord)
    glyphs=[]
    for px in FONT_SIZES:
        font=ImageFont.truetype(str(font_path),px)
        for ch in alphabet:
            advance=max(1,math.ceil(font.getlength(ch)));w=max(advance,font.getbbox(ch)[2]);h=px+5
            tiles.require(0<w<=40 and advance<=40,'Glyph outside runtime bound')
            image=Image.new('L',(w,h));ImageDraw.Draw(image).text((0,px+1),ch,font=font,fill=255,anchor='ls')
            raw=image.tobytes();encoded=bytearray();at=0
            while at<len(raw):
                end=at+1
                while end<len(raw) and raw[end]==raw[at] and end-at<255:end+=1
                encoded.extend((end-at,raw[at]));at=end
            payload=bytes(encoded) if len(encoded)<len(raw) else raw
            glyphs.append((ord(ch),px,w,h,advance,len(payload) if payload!=raw else 0,payload))
    return glyphs


def read_tables(pack, language):
    prefix='localization/'+language+'/'
    return {name[len(prefix):-5]:json.loads(pack.read(name)) for name in pack.entries
            if name.startswith(prefix) and name.endswith('.json')}


def load_skins(pack):
    from build_content import atlas,texture,prepared,image_data
    from card_banner import material_tint
    profile=json.loads((ROOT/'config/card-hud-assets.json').read_text())
    tiles.require(profile['base_auxiliary_count']==1188 and len(profile['assets'])==48,'HUD profile mismatch')
    cache={};skins=[];receipt=[]
    for row in profile['assets']:
        image=atlas(pack,row['source'],cache) if row['source'].startswith('@') else texture(pack,row['source'])
        if row.get('material'):image=material_tint(pack,image,row['material'])
        image=prepared(image,tuple(row['size']),row['fit'])
        raw=image_data(image,True);skins.append((*image.size,1,raw))
        receipt.append(dict(name=row['name'],source=row['source'],size=list(image.size),raw_sha256=tiles.sha256(raw)))
    return skins,receipt


def format_catalog(source, values, tables, language):
    """Format fixed original potion values, retaining original plural grammar."""
    values=dict(values,energyPrefix=1,IsMultiplayer=0)
    def branches(text):
        result=[];depth=0;start=0
        for at,ch in enumerate(text):
            if ch=='{':depth+=1
            elif ch=='}':depth-=1
            elif ch=='|' and depth==0:result.append(text[start:at]);start=at+1
        result.append(text[start:]);return result
    def unit(star):
        title=tables['static_hover_tips'][('STAR_COUNT' if star else 'ENERGY_COUNT')+'.title']
        return (' ' if language=='eng' else ' 点')+('[gold]'+title+'[/gold]' if star else title)
    def render(text, context=None, depth=0):
        tiles.require(depth<=12,'Catalog format depth exceeded');out=[];at=0
        while at<len(text):
            if text[at]!='{':out.append(text[at]);at+=1;continue
            end=at+1;level=1
            while end<len(text) and level:
                if text[end]=='{':level+=1
                elif text[end]=='}':level-=1
                end+=1
            tiles.require(level==0,'Unbalanced catalog template')
            expression=text[at+1:end-1];parts=expression.split(':',2);name=parts[0]
            value=context if not name else values.get(name)
            tiles.require(value is not None,'Unbound catalog variable: '+name)
            fmt=parts[1] if len(parts)>1 else ''
            if fmt in ('plural','show'):
                choices=branches(parts[2]);index=0 if (value==1 if fmt=='plural' else bool(value)) else 1
                tiles.require(index<len(choices),'Missing catalog branch')
                out.append(render(choices[index],value,depth+1))
            elif fmt in ('','diff()','inverseDiff()'):out.append(str(value))
            elif fmt in ('energyIcons()','starIcons()'):out.append(str(value)+unit(fmt=='starIcons()'))
            elif re.fullmatch(r'energyIcons\([0-9]+\)',fmt):
                preceding=''.join(out);count=fmt[12:-1]
                out.append(('' if preceding and preceding[-1] in '0123456789X' else count)+unit(False))
            else:raise ValueError('Unsupported catalog formatter: '+expression)
            at=end
        return ''.join(out)
    return render(source)


def legacy_single_player(source, aroma):
    """Keep the historical event reader's numeric tokens, fold only its mode.

    The original English and Chinese text share this conditional grammar;
    selecting its single-player branch needs no translated replacement prose.
    """
    source=source.replace('{AromaPrinciple}',aroma)
    source=re.sub(r'\{IsMultiplayer:([^{}|]*)\|([^{}]*)\}',lambda m:m[2],source)
    allowed={'MaxHp','Damage','Heal','Gold','HpLoss','Cards'}
    unknown=set(re.findall(r'\{([^{}]*)\}',source))-allowed
    tiles.require(not unknown,'Unbound legacy event variables: '+str(sorted(unknown)))
    return source


def localized_strings(source, tables, language, bindings, extra_keys):
    from locale_cards import card_strings
    from world_ui_profile import TEXT_KEYS,TEXT_BASE
    from native_ui_profile import TEXTS
    from asset_profile import EVENT_STRINGS
    from build_events import EVENTS
    potion_values=json.loads((ROOT/'config/locale-potion-values.json').read_text())['potions']
    texts=['']*STRING_COUNT
    def original(qualified):
        table,key=qualified.split('.',1)
        value=tables[table].get(key)
        tiles.require(isinstance(value,str),'Missing original localization key: '+qualified)
        return value
    for record,variants in zip(source.cards,card_strings(tables,language)):
        for index,value in zip(record[1:],variants):texts[index]=value
    texts[42]=original('monsters.NIBBIT.name');texts[43]=original('characters.IRONCLAD.title')
    def old_event(key,character='IRONCLAD'):
        return legacy_single_player(original('events.'+key),original('characters.'+character+'.aromaPrinciple'))
    for index,key in enumerate(EVENT_STRINGS,44):texts[index]=old_event(key)
    index=228
    for key,finish_a,finish_b,option_a,option_b in EVENTS:
        suffixes=['title','pages.INITIAL.description','pages.'+finish_a+'.description','pages.'+finish_b+'.description']
        for option in (option_a,option_b):suffixes.extend(['pages.INITIAL.options.'+option+'.title','pages.INITIAL.options.'+option+'.description'])
        for suffix in suffixes:
            texts[index]=old_event(key+'.'+suffix)
            index+=1
    texts[268]=old_event('AROMA_OF_CHAOS.pages.MAINTAIN_CONTROL.description','SILENT')
    for index,key in enumerate(TEXT_KEYS,TEXT_BASE):texts[index]=original(key)
    for index,(table,key) in enumerate(TEXTS,6100):texts[index]=original(table+'.'+key)
    texts[MARKER]=language
    tiles.require(len(extra_keys)<=UI_BASE-EXTRA_BASE and len(bindings)<=STRING_COUNT-UI_BASE,'Locale reserved catalog exhausted')
    for index,key in enumerate(extra_keys,EXTRA_BASE):texts[index]=original(key)
    for index,row in enumerate(bindings,UI_BASE):
        if row.get('reference'):
            value=original(row['reference'])
            if row['reference'].startswith('potions.') and row['reference'].endswith('.description'):
                model=row['reference'].split('.')[1]
                value=format_catalog(value,potion_values[model],tables,language)
            if row.get('amount_placeholder'):
                value=value.replace('{Amount:diff()}','{n}').replace('{Amount}','{n}')
            texts[index]=row.get('prefix','')+value+row.get('suffix','')
        else:texts[index]=row['source'] if language=='zhs' else row['eng']
    if language=='eng':
        residual=[i for i,t in enumerate(texts) if re.search('[\u3400-\u9fff]',t)]
        tiles.require(not residual,'English pack contains Chinese strings: '+str(residual[:20]))
    return texts


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path,required=True)
    parser.add_argument('--pck',type=Path,required=True)
    parser.add_argument('--output-dir',type=Path,required=True)
    parser.add_argument('--font',type=Path)
    parser.add_argument('--language',choices=('zhs','eng','both'),default='both')
    args=parser.parse_args()
    source_path=tiles.private_path(args.input,'Input');pck_path=tiles.private_path(args.pck,'PCK')
    destination=tiles.private_path(args.output_dir,'Output directory');destination.mkdir(parents=True,exist_ok=True)
    release=json.loads((pck_path.parent/'release_info.json').read_text(encoding='utf-8-sig'))
    tiles.require(release.get('version')=='v0.111.0' and release.get('commit')=='41cef1ea','Unpinned reference input')
    from pck import Pack
    from build_content import choose_font
    from locale_profile import bindings_and_keys,validate_generated
    validate_generated();bindings,extra_keys=bindings_and_keys()
    data=source_path.read_bytes();source=tiles.PackView(data,1188,versions=(16,),profile=(558,6110,1695,1188))
    font=choose_font(str(args.font) if args.font else None)
    pack=Pack(pck_path);editions=[]
    try:
        skins,skin_receipt=load_skins(pack)
        for language in (('zhs','eng') if args.language=='both' else (args.language,)):
            tables=read_tables(pack,language)
            texts=localized_strings(source,tables,language,bindings,extra_keys)
            glyphs=glyph_subset(texts,font,language,[r['source'] for r in bindings] if language=='zhs' else ())
            output,receipt=repack(data,1188,texts,glyphs,skins,profile=(558,6110,1695,1188))
            receipt.update(schema=1,reference='v0.111.0 / 41cef1ea',language=language,format=17,
                original_text_keys=extra_keys,ui_bindings=len(bindings),skins=skin_receipt,
                glyph_sizes=list(FONT_SIZES),font_sha256=tiles.sha256(font.read_bytes()))
            path=destination/f'nSpire2-0.17-{language}.nsp.tns'
            tiles.atomic_bytes(path,output)
            tiles.atomic_bytes(destination/f'locale-{language}-receipt.json',json.dumps(receipt,indent=2).encode()+b'\n')
            editions.append(dict(language=language,path=str(path),bytes=len(output),sha256=receipt['output_sha256'],graphics=receipt['original_graphics_sha256']))
            print(json.dumps(editions[-1]),flush=True)
    finally:pack.close()
    tiles.require(len({r['graphics'] for r in editions})<=1,'Language editions differ in original graphics')
    tiles.atomic_bytes(destination/'bilingual-receipt.json',json.dumps(dict(schema=1,editions=editions,identical_original_graphics=True),indent=2).encode()+b'\n')


if __name__=='__main__':main()
