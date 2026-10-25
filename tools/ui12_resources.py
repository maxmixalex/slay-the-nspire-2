#!/usr/bin/env python3
"""Correct source artwork and font metrics in private UI12 locale editions.

Existing animation streams are copied without decoding. Replacement images
and seven missing source UI images use the existing codec. Latin layout keeps
1/64px advances, negative bearings and original pair kerning in private glyph
records; there is no resident font engine or public font-derived table.
"""
from __future__ import annotations
import argparse
import json
import math
from pathlib import Path
import struct
from PIL import Image, ImageDraw, ImageFont
import content_tiles as tiles
import split_content as split
from repack_retained import auxiliary_count
from build_locale_editions import face_set, rewrite
from patch_frontend_assets import patch_pack
from patch_feedback_assets import write_image, stored_image
from pck import Pack
from world_scene_assets import load_scene_texture
from random_character_ui12 import build as random_background

VERSION=40
BASE_AUX=2154
AUXILIARY=2161
PAIR_BASE=0x100000
KEY_SYMBOLS=(46,47,48,49,50,51,52,53,54,55,56,57,59,97,98,99,100,101,103,104,107,126,161,162,163,164,183)
_pairs={}


def view(data):
    return tiles.PackView(data,auxiliary_count(data),versions=(39,40))


def source_assets(pck,cache):
    specs=(('pile-count','images/packed/combat_ui/pile_button_count.png',(20,20)),
      ('player-thought','images/vfx/shared_use/smoke_vfx_flat.png',(48,48)),
      ('random-character','images/packed/character_select/char_select_random.png',(64,64)),
      ('transition-ironclad','images/ui/transitions/ironclad_transition.png',(320,240)),
      ('transition-silent','images/ui/transitions/silent_transition.png',(320,240)),
      ('transition-necrobinder','images/ui/transitions/necrobinder_transition.png',(320,240)))
    pack=Pack(pck);rows=[]
    try:
        for key,path,size in specs:
            original=load_scene_texture(pack,path).convert('RGBA')
            if key=='player-thought':original=original.crop((0,0,original.width//2,original.height//2))
            image=original.resize(size,Image.Resampling.LANCZOS)
            row=write_image(cache,key,image);row.update(source=path,auxiliary=BASE_AUX+len(rows))
            image.save(cache/(key+'.png'));rows.append(row)
        image=random_background(pack,cache/'random-background.png')
        row=write_image(cache,'random-background',image);row.update(source='scenes/screens/char_select/char_select_bg_random_character.tscn',auxiliary=2160);rows.append(row)
    finally:pack.close()
    return rows


def replace_images(data,replacements):
    """Copy only referenced retained blocks; preserve all tile streams/clocks."""
    source=view(data);tiled=tiles.TileReader(source)
    end=tiled.descriptors+tiled.frame_count*16
    out=bytearray(data[:end]);copied={};slices={}
    def add(raw):
        out.extend(bytes(len(out)&1));at=len(out);out.extend(raw);return at
    def copy(offset,length):
        key=offset,length
        if key not in copied:copied[key]=add(source.block(offset,length))
        return copied[key]
    def group_slice(offset):
        if offset not in slices:
            relative,count,raw_at,shape=struct.unpack_from('<iIII',data,offset)
            target=copy(offset+relative,count);at=add(bytes(16))
            struct.pack_into('<iIII',out,at,target-at,count,raw_at,shape);slices[offset]=at
        return slices[offset]
    header=tiles.GLOBAL.unpack_from(data,source.base_end);table,cost=header[9],header[14]
    for i,(offset,length,*_) in enumerate(tiled.groups):
        at=copy(offset-32,length+32);struct.pack_into('<II',out,table+i*8,at,length+32)
    hashes={}
    for i,(offset,w,h,fmt,length) in enumerate(source.images):
        if i in replacements:
            image=replacements[i].convert('RGBA');row,payload=stored_image(image,sparse=True)
            w,h,fmt,length=image.width,image.height,row['format'],len(payload);at=add(payload)
            decoded=struct.unpack_from('<I',payload,1)[0] if fmt==5 else len(payload)
            out[cost+i]=(decoded+4095)//4096;hashes[str(i)]=row['raw_sha256']
        else:at=offset if fmt==6 else group_slice(offset) if fmt==7 else copy(offset,length)
        tiles.IMAGE.pack_into(out,40+i*16,at,w,h,fmt,length)
    for i,(cp,offset,band,w,h,advance,encoded) in enumerate(source.glyphs):
        length=(encoded&~tiles.GLYPH_GROUP_FLAG) or w*h
        at=group_slice(offset) if encoded&tiles.GLYPH_GROUP_FLAG else copy(offset,length)
        tiles.GLYPH.pack_into(out,source.glyph_at+i*16,cp,at,band,w,h,advance,encoded)
    for i,(offset,length) in enumerate(source.strings):
        at=group_slice(offset) if length&tiles.STRING_GROUP_FLAG else copy(offset,length+1)
        struct.pack_into('<II',out,source.string_at+i*8,at,length)
    struct.pack_into('<I',out,12,len(out));struct.pack_into('<I',out,36,0)
    target=view(out)
    tiles.require(target.clips==source.clips and target.cards==source.cards and target.aux==source.aux,'Changed stable model/clip IDs')
    for key,sha in hashes.items():tiles.require(tiles.sha256(target.image_raw(int(key))[3])==sha,'Replacement image differs')
    return bytes(out),dict(replaced_images=hashes,input_bytes=len(data),output_bytes=len(out),
        tile_streams_copied=len(tiled.groups),animation_clocks_unchanged=True)


def rle(raw):
    encoded=bytearray();at=0
    while at<len(raw):
        end=at+1
        while end<len(raw) and raw[end]==raw[at] and end-at<255:end+=1
        encoded.extend((end-at,raw[at]));at=end
    return (bytes(encoded),len(encoded)) if len(encoded)<len(raw) else (raw,0)


def pair_metrics(face,cache):
    key=(face.path.name,'em2048')
    if key in _pairs:return _pairs[key]
    path=cache/(face.path.stem+'-em2048-pairs.json')
    if path.exists():result=json.loads(path.read_text())
    else:
        font=ImageFont.truetype(str(face.path),2048);alphabet=[c for c in range(32,127) if c in face.coverage]
        length={c:font.getlength(chr(c),features=['-liga','-clig']) for c in alphabet}
        result=[]
        for left in alphabet:
            for right in alphabet:
                value=round(font.getlength(chr(left)+chr(right),features=['-liga','-clig'])-length[left]-length[right])
                if value:result.append((left,right,value))
        tiles.atomic_bytes(path,json.dumps(result).encode()+b'\n')
    _pairs[key]=result;return result


def precise_glyphs(source,font_dir,language,cache):
    faces,fallback=face_set(font_dir,language);rows={};bands=set();changed=0;bearings=0
    for i,(cp,_off,band,w,h,advance,encoded) in enumerate(source.glyphs):
        raw=bytes(source.glyph_payload(i));encoded&=~tiles.GLYPH_GROUP_FLAG
        px=band&31;style=((band>>6)&3)|((band>>3)&4)
        # Existing shaped Thai graphemes retain their exact source raster.
        if 32<=cp<0x530 and style<len(faces):
            face=next((f for f in [faces[style],*fallback] if cp in f.coverage),None)
            if face:
                font=face.fonts[px];char=chr(cp);q=round(font.getlength(char)*64)
                left=min(0,font.getbbox(char)[0]);tiles.require(-7<=left<=0 and 0<=q<=2560,'Metric outside UI40 range')
                advance=0x8000|((-left)<<12)|q;changed+=1
                if left:
                    width=max(math.ceil(q/64),font.getbbox(char)[2])-left
                    image=Image.new('L',(width,h));ImageDraw.Draw(image).text((-left,px+1),char,font=font,fill=255,anchor='ls')
                    raw,encoded=rle(image.tobytes());w=width;bearings+=1
                if cp<127:bands.add((px,style))
        rows[(band,cp)]=(cp,band,w,h,advance,encoded,raw)
    completed_ascii=0
    for px,style in sorted(bands):
        band=px|((style&3)<<6)|((style&4)<<3);face=faces[style];font=face.fonts[px]
        for cp in range(32,127):
            if (band,cp) in rows or cp not in face.coverage:continue
            char=chr(cp);q=round(font.getlength(char)*64);left=min(0,font.getbbox(char)[0])
            width=max(1,math.ceil(q/64),font.getbbox(char)[2])-left;height=px+5
            tiles.require(-7<=left<=0 and width<=40 and height<=40 and q<=2560,'Completed ASCII exceeds bounds')
            image=Image.new('L',(width,height));ImageDraw.Draw(image).text((-left,px+1),char,font=font,fill=255,anchor='ls')
            raw,encoded=rle(image.tobytes());rows[(band,cp)]=(cp,band,width,height,0x8000|((-left)<<12)|q,encoded,raw);completed_ascii+=1
    # These source scene families share their exact original font file. One
    # table per real face also keeps the large Traditional Chinese alphabet
    # below the fixed 65,535-entry directory bound.
    canonical=lambda style:1 if style in (3,5) else 2 if style==6 else style
    pairs=0
    for style in sorted({canonical(style) for px,style in bands}):
        px=20 # Metadata-only entries do not advertise a drawable font size.
        band=px|((style&3)<<6)|((style&4)<<3)
        for left,right,value in pair_metrics(faces[style],cache):
            tiles.require(-2048<=value<2048,'Kerning outside signed range')
            cp=PAIR_BASE+(left-32)*95+right-32
            rows[(band,cp)]=(cp,band,1,1,0x4000|(value+2048),0,b'\0');pairs+=1
    # Real TI key shapes: crop to their ink, then preserve aspect in a native
    # 8/10/12/...px label cell. Font source remains a caller-supplied input.
    key_font=font_dir/'TINspireKeysCX.ttf'
    for px in (6,8,10,12,14,16,18,20):
        font=ImageFont.truetype(str(key_font),px*3)
        for symbol in KEY_SYMBOLS:
            char=chr(symbol);box=font.getbbox(char);image=Image.new('L',(max(1,box[2]-box[0]),max(1,box[3]-box[1])))
            ImageDraw.Draw(image).text((-box[0],-box[1]),char,font=font,fill=255)
            height=px+1;width=max(1,min(38,round(image.width*height/image.height)))
            image=image.resize((width,height),Image.Resampling.LANCZOS)
            cell=Image.new('L',(width+2,px+5));cell.paste(image,(1,1))
            raw,encoded=rle(cell.tobytes());cp=0xe100+symbol
            rows[(px,cp)]=(cp,px,width+2,px+5,width+2,encoded,raw)
    result=[rows[key] for key in sorted(rows)]
    tiles.require(len(result)<=65535,'Glyph directory count exceeded')
    return result,dict(fractional_metrics=changed,restored_negative_bearings=bearings,
        kerning_pairs=pairs,kerning_em_units=2048,completed_ascii_glyphs=completed_ascii,key_font_glyphs=len(KEY_SYMBOLS)*8,glyphs=len(result))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for key in ('editions','pck','fonts','cache','output'):parser.add_argument('--'+key,type=Path,required=True)
    parser.add_argument('--replacements',type=Path,help='Private JSON image/auxiliary/clip-frame to PNG mapping')
    parser.add_argument('--art-source',type=Path,help='UI39 logical pack with corrected/re-tiled art; locale text stays from editions')
    parser.add_argument('--languages',default='all')
    args=parser.parse_args();out=tiles.private_path(args.output,'UI12 output');out.mkdir(parents=True,exist_ok=True)
    cache=tiles.private_path(args.cache,'UI12 cache');cache.mkdir(parents=True,exist_ok=True)
    original_common=(args.editions/'nSpire2.common.tns').read_bytes()
    data=args.art_source.read_bytes() if args.art_source else split.reconstruct(original_common,(args.editions/'nSpire2.eng.tns').read_bytes(),'eng');source=view(data)
    replacements={}
    if args.replacements:
        for row in json.loads(args.replacements.read_text()):
            image_id=row.get('image')
            if image_id is None:
                aux=row.get('auxiliary',row.get('aux'))
                image_id=source.aux[aux] if aux is not None else source.clips[row['clip']][0]+row['frame']
            tiles.require(image_id not in replacements,'Duplicate image replacement')
            replacements[image_id]=Image.open(row['png']).convert('RGBA')
    data,art_report=replace_images(data,replacements)
    rows=source_assets(args.pck,cache)
    data,append_report=patch_pack(data,dict(images=rows,clips=[],localized_text={'eng':{},'zhs':{}}),cache,
        base_aux=BASE_AUX,auxiliary=AUXILIARY,input_version=39,version=40,texts=(),base_clips=1783,base_strings=8704)
    art=view(data);reports={};common=None
    old_manifest=json.loads((args.editions/'locales-receipt.json').read_text())
    languages=list(split.LANGUAGES) if args.languages=='all' else args.languages.split(',')
    for language in languages:
        old=split.reconstruct(original_common,(args.editions/f'nSpire2.{language}.tns').read_bytes(),language);previous=view(old)
        texts=[previous.string_payload(i)[:-1].decode('utf-8') for i in range(len(previous.strings))]
        glyphs,font_report=precise_glyphs(previous,args.fonts,language,cache)
        localized,layout_report=rewrite(data,texts,glyphs,language);bounds=split.layout(localized,language)
        candidate=split.common_file(localized[bounds['prefix']:bounds['prefix']+bounds['artwork']])
        if common is None:common=candidate;tiles.atomic_bytes(out/'nSpire2.common.tns',common)
        tiles.require(common==candidate,'Locale artwork differs')
        lang=split.compress_language_directory(split.language_file(localized,bounds,language,common[32:64]))
        tiles.require(split.reconstruct(common,lang,language)==localized,'Split readback differs')
        tiles.atomic_bytes(out/f'nSpire2.{language}.tns',lang)
        if language in ('eng','zhs'):tiles.atomic_bytes(out/f'ui12-{language}.nsp',localized)
        reports[language]=dict(old_manifest['languages'][language],fonts=font_report,layout=layout_report,bytes=len(lang),sha256=tiles.sha256(lang),logical_bytes=len(localized))
        tiles.atomic_bytes(out/f'{language}-receipt.json',json.dumps(reports[language],ensure_ascii=False,indent=2).encode()+b'\n')
        print(json.dumps(dict(language=language,bytes=len(lang),**font_report)),flush=True)
    receipt=dict(schema=1,reference='v0.111.0 / 41cef1ea',version=40,common_bytes=len(common),languages=reports,artwork=art_report,appended_artwork=append_report)
    tiles.atomic_bytes(out/'locales-receipt.json',json.dumps(receipt,ensure_ascii=False,indent=2).encode()+b'\n')


if __name__=='__main__':main()
