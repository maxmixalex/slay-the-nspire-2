#!/usr/bin/env python3
"""Build optional original-language text/font packs over one private UI39 art set.

The stable card, string, image and clip IDs stay unchanged. Original localized
tables are read from the owner's pinned PCK; missing keys follow the original
English fallback. Port-only text also falls back to the reviewed English text.
Every edition is independent and may be installed with the one common file.
"""
from __future__ import annotations

import argparse
from collections import Counter
import json
import math
from pathlib import Path
import re
import struct
import unicodedata

from PIL import Image, ImageDraw

import content_tiles as tiles
import split_content as split
from repack_retained import auxiliary_count
from replace_original_glyphs import Face, visible_alphabet, styled_alphabet, text_format_control

ROOT = Path(__file__).resolve().parents[1]
VERSIONS = (39, 40)
COMMON_SIZES = (5, 6, 8, 10, 12)
LABEL_SIZES = (8, 10, 12, 14, 16, 18, 20)
REGISTRY_NAMES = 'English 简体中文 繁體中文 Deutsch Español (Latinoamérica) Français Bahasa Indonesia Italiano 日本語 한국어 Polski Português (Brasil) Русский Español (España) ภาษาไทย Türkçe'
PORT_CREDITS = 'Nspire port zyf722 (with Codex) Nspire 移植 zyf722（基于 Codex） zyf722（基於 Codex）'
FONT_NAMES = {
    'latin': ('kreon_regular.ttf', 'kreon_bold.ttf', 'bitter_medium_italic.ttf', 'spectral_bold.ttf'),
    'zhs': ('NotoSansMonoCJKsc-Regular.otf', 'SourceHanSerifSC-Bold.otf', 'SourceHanSerifSC-Medium.otf', 'SourceHanSerifSC-Bold.otf'),
    'zht': ('NotoSansMonoCJKtc-Regular.otf', 'SourceHanSerifTC-Bold.otf', 'SourceHanSerifTC-Medium.otf', 'SourceHanSerifTC-Bold.otf'),
    'jpn': ('NotoSansCJKjp-Regular.otf', 'NotoSansCJKjp-Bold.otf', 'NotoSansCJKjp-Medium.otf', 'NotoSansCJKjp-Bold.otf'),
    'kor': ('GyeonggiCheonnyeonBatangBold.ttf',)*4,
    'rus': ('FiraSansExtraCondensed-Regular.ttf', 'FiraSansExtraCondensed-Bold.ttf', 'FiraSansExtraCondensed-Italic.ttf', 'FiraSansExtraCondensed-Bold.ttf'),
    'pol': ('FiraSansExtraCondensed-Regular.ttf', 'FiraSansExtraCondensed-Bold.ttf', 'FiraSansExtraCondensed-Italic.ttf', 'FiraSansExtraCondensed-Bold.ttf'),
    'tha': ('CSChatThaiUI.ttf',)*4,
}


def tables_for(pack, language):
    from localize_content import read_tables
    english = read_tables(pack, 'eng')
    selected = read_tables(pack, language)
    tiles.require(bool(selected), 'Original language has no localization tables: '+language)
    merged = {name: dict(table) for name, table in english.items()}
    for name, table in selected.items(): merged.setdefault(name, {}).update(table)
    missing = [name+'.'+key for name, table in english.items() for key in table
               if key not in selected.get(name, {})]
    return merged, english, missing


def stable_index():
    result = {}
    for name in ('world_text_index.inc', 'native_ui_text_index.inc', 'locale_text_index.inc',
                 'ui7_text_index.inc', 'profile_text_index.inc', 'run_modes_text_index.inc',
                 'auxiliary_text_index.inc', 'selection_text_index.inc'):
        for key, slot in re.findall(r'\{"([^\"]+)",\s*(\d+)\}', (ROOT/'src'/name).read_text()):
            # Keep the original field when a source alias occupies the same ID.
            if key == 'settings_ui.SCREENSHAKE_HEADER': continue
            result[int(slot)] = key
    result.update({8103:'settings_ui.TAB_GENERAL',8104:'settings_ui.TAB_INPUT',8105:'vfx.POWER_WEARS_OFF'})
    return result


def localized_texts(source, pack, language):
    from locale_cards import card_strings
    from locale_profile import bindings_and_keys
    from localize_content import format_catalog
    from auxiliary_ui8_assets import credits_text, TEXTS as AUX_TEXTS
    from game_over_ui8_assets import TEXTS as GAME_OVER_TEXTS, PORT_TEXTS
    tables, english, missing = tables_for(pack, language)
    texts = [source.string_payload(i)[:-1].decode('utf-8') for i in range(len(source.strings))]
    index, bound, fallback = stable_index(), set(), []
    def get(key):
        table, name = key.split('.', 1)
        value = tables.get(table, {}).get(name)
        return value if isinstance(value, str) else None
    for slot, key in index.items():
        value = get(key)
        if value is not None: texts[slot] = value; bound.add(slot)
    for row, variants in zip(source.cards, card_strings(tables, language)):
        for slot, text in zip(row[1:], variants): texts[slot] = text; bound.add(slot)
    bindings, _ = bindings_and_keys()
    potion_values = json.loads((ROOT/'config/locale-potion-values.json').read_text())['potions']
    for slot, row in enumerate(bindings, 7168):
        if row.get('reference'):
            value = get(row['reference'])
            tiles.require(value is not None, 'Unbound original UI key: '+row['reference'])
            if row['reference'].startswith('potions.') and row['reference'].endswith('.description'):
                model = row['reference'].split('.')[1]
                value = format_catalog(value, potion_values[model], tables, language)
            if row.get('amount_placeholder'): value=value.replace('{Amount:diff()}','{n}').replace('{Amount}','{n}')
            texts[slot]=row.get('prefix','')+value+row.get('suffix','')
        else:
            texts[slot]=row['source'] if language=='zhs' else row['eng']
            if language not in ('eng','zhs'): fallback.append(slot)
        bound.add(slot)
    for slot, key in AUX_TEXTS:
        if key=='auxiliary.CREDITS': texts[slot]=credits_text(tables['credits']);bound.add(slot)
        # Removed offline services must not keep their old articles or glyphs.
        elif key.startswith('patch_notes.') or 'FEEDBACK' in key: texts[slot]='';bound.add(slot)
    for slot, key in GAME_OVER_TEXTS:
        if key.startswith('game_over_ui.'):
            texts[slot]=PORT_TEXTS.get(language,PORT_TEXTS['eng'])[key.split('.',1)[1]]
            if language not in PORT_TEXTS: fallback.append(slot)
        else:
            badge, tier = key.split('.')[1:]
            prefix='' if tier=='default' else tier
            texts[slot]=tables['badges'][badge+'.'+(prefix+'Title' if prefix else 'title')]+'\n'+tables['badges'][badge+'.'+(prefix+'Description' if prefix else 'description')]
        bound.add(slot)
    # Legacy slots are still addressable by historical save readers. A unique
    # source-table match safely translates them without guessing an ID.
    inverse = {}
    for table, values in english.items():
        for key, value in values.items():
            if isinstance(value,str): inverse.setdefault(value,set()).add(table+'.'+key)
    for slot, text in enumerate(texts):
        if slot in bound or not text: continue
        candidates={get(key) for key in inverse.get(text,())};candidates.discard(None)
        if len(candidates)==1: texts[slot]=next(iter(candidates));bound.add(slot)
        elif language!='eng': fallback.append(slot)
    texts[split.MARKER]=language
    return texts, dict(language=language,original_bound_slots=len(bound),
        english_fallback_missing_keys=missing,port_or_legacy_fallback_slots=sorted(set(fallback)),
        removed_patch_notes_and_feedback=True)


def cluster_texts(texts):
    """Bake combining graphemes once, so ARM never splits a Thai vowel stack.

    PUA identifiers are private pack-local display glyphs, never gameplay IDs.
    They preserve fully shaped graphemes without a resident shaping engine.
    """
    import regex
    clusters, values = {}, {}
    def replace(match):
        value=unicodedata.normalize('NFC',match[0])
        # Thai SARA AM is a spacing vowel (Lo), but its upper component must
        # still be shaped with the preceding consonant, just like tone marks.
        if len(value)==1 or not any(unicodedata.category(c).startswith('M') or c=='\u0e33' for c in value): return value
        if value not in clusters:
            cp=0xf0000+len(clusters);tiles.require(cp<0xffffd,'Too many shaped graphemes')
            clusters[value]=cp;values[cp]=value
        return chr(clusters[value])
    return [regex.sub(r'\X',replace,value) for value in texts], values


def face_set(font_dir, language):
    names=FONT_NAMES.get(language,FONT_NAMES['latin'])
    loaded={}
    def face(name):
        if name not in loaded: loaded[name]=Face(font_dir/name)
        return loaded[name]
    regular,bold,italic,display=map(face,names)
    # Cross-script language names and the original multinational credits need
    # real glyphs from their source fonts, even in the English edition.
    fallback=[face(name) for name in ('NotoSansMonoCJKsc-Regular.otf','NotoSansMonoCJKtc-Regular.otf',
        'NotoSansCJKjp-Regular.otf','GyeonggiCheonnyeonBatangBold.ttf','FiraSansExtraCondensed-Regular.ttf','CSChatThaiUI.ttf')]
    return [regular,bold,italic,bold,display,bold,italic],fallback


def glyph_for(font, cp, text, px, style):
    face=font.fonts[px]
    missing_space=len(text)==1 and unicodedata.category(text)=='Zs' and cp not in font.coverage
    measured=' ' if missing_space else text
    advance=max(1,math.floor(face.getlength(measured)*(0.5 if missing_space and cp==0x202f else 1)+0.5))
    box=face.getbbox(measured);width=advance if missing_space else max(advance,box[2]);height=px+5
    tiles.require(0<width<=40 and advance<=40,'Glyph outside LCD bounds')
    image=Image.new('L',(width,height))
    if not missing_space:ImageDraw.Draw(image).text((0,px+1),text,font=face,fill=255,anchor='ls')
    raw=image.tobytes();encoded=bytearray();at=0
    while at<len(raw):
        end=at+1
        while end<len(raw) and raw[end]==raw[at] and end-at<255: end+=1
        encoded.extend((end-at,raw[at]));at=end
    payload=bytes(encoded) if len(encoded)<len(raw) else raw
    band=px|((style&3)<<6)|((style&4)<<3)
    return cp,band,width,height,advance,len(payload) if payload!=raw else 0,payload


def build_glyphs(source,texts,clusters,font_dir,language):
    fonts,fallback=face_set(font_dir,language)
    required=styled_alphabet(texts)
    alphabet=set().union(*required,visible_alphabet(REGISTRY_NAMES+PORT_CREDITS),range(32,127))
    alphabet.update(map(ord,'←→↑↓‹›×…−·✓★'))
    title=set(range(32,127))
    for slot,key in stable_index().items():
        if key.endswith(('.title','.header','.name')) or key.startswith(('gameplay_ui.','main_menu_ui.','settings_ui.')):
            title.update(visible_alphabet(texts[slot]))
    for row in source.cards:title.update(visible_alphabet(texts[row[1]]))
    rows={};missing=set();spaces=set()
    def add(cp,px,style):
        if text_format_control(cp) or cp in (0xe000,0xe001):return
        text=clusters.get(cp,chr(cp));codepoints=set(map(ord,text))
        chosen=next((font for font in [fonts[style],*fallback] if codepoints<=font.coverage),None)
        if chosen is None and len(text)==1 and unicodedata.category(text)=='Zs':chosen=fonts[style];spaces.add(cp)
        if chosen is None:missing.add(cp);return
        rows[(cp,px,style)]=glyph_for(chosen,cp,text,px,style)
    for cp in sorted(alphabet):
        for px in COMMON_SIZES:add(cp,px,0)
    for cp in sorted(title|visible_alphabet(REGISTRY_NAMES+PORT_CREDITS)):
        for px in LABEL_SIZES:add(cp,px,0)
    for cp in range(32,127):
        for px in (4,7,9):add(cp,px,0)
    for style in (1,2,3):
        for cp in sorted(required[style]|set(range(32,127))):
            for px in COMMON_SIZES:add(cp,px,style)
    for style in (4,5,6):
        selected=title if style==4 else title|required[1] if style==5 else required[2]|set(range(32,127))
        for cp in sorted(selected):
            for px in (8,10,12,14,16,20) if style==4 else COMMON_SIZES:add(cp,px,style)
    # Keep old rare scene sizes when still reachable, but never carry their
    # obsolete patch-note-only alphabet or payload into the new edition.
    for cp,_off,band,_w,_h,_advance,_encoded in source.glyphs:
        if cp not in alphabet:continue
        px=band&31;style=((band>>6)&3)|((band>>3)&4)
        if style<7 and (cp,px,style) not in rows:add(cp,px,style)
    tiles.require(not missing,'Original font chain lacks visible characters: '+repr(sorted(missing)))
    result=sorted(rows.values(),key=lambda row:(row[1],row[0]))
    tiles.require(len(result)<=65535,'Locale glyph count exceeds runtime bound')
    return result,dict(glyphs=len(result),codepoints=len(alphabet),shaped_graphemes=len(clusters),
        missing_visible_codepoints=[],space_metrics_from_regular_space=sorted(spaces),advance='nearest integer; no per-character ceil tracking',
        regular_sizes=list(COMMON_SIZES),font_files=sorted({f.path.name for f in [*fonts,*fallback]}))


def rewrite(data,texts,glyphs,language):
    """Rebuild the live glyph/string tail; copy compressed art without decoding."""
    aux=auxiliary_count(data);source=tiles.PackView(data,aux,versions=VERSIONS)
    old_language=source.string_payload(split.MARKER)[:-1].decode('ascii');before=split.layout(data,old_language)
    tiled=tiles.TileReader(source);ni,ng,ns,nc,nk,_=source.counts
    tiles.require(len(texts)==ns,'String IDs must not change')
    delta=(len(glyphs)-ng)*16;art_end=before['prefix']+before['artwork']
    out=bytearray(data[:source.glyph_at]);out.extend(bytes(len(glyphs)*16));out.extend(data[source.string_at:art_end])
    for i,(off,w,h,fmt,length) in enumerate(source.images):tiles.IMAGE.pack_into(out,40+i*16,off+delta,w,h,fmt,length)
    header=list(tiles.GLOBAL.unpack_from(data,source.base_end));table=header[9]
    for i in (8,9,14):header[i]+=delta
    tiles.GLOBAL.pack_into(out,source.base_end+delta,*header)
    for i in range(tiled.tile_groups+tiled.index_groups):
        off,length=struct.unpack_from('<II',data,table+i*8);struct.pack_into('<II',out,table+delta+i*8,off+delta,length)
    for i in range(tiled.frame_count):
        at=tiled.descriptors+i*16;global_at,group,offset,length=tiles.DESCRIPTOR.unpack_from(data,at)
        tiles.DESCRIPTOR.pack_into(out,at+delta,global_at+delta,group,offset,length)
    def append(raw):
        out.extend(bytes(len(out)&1));at=len(out);out.extend(raw);return at
    frequency=Counter(''.join(texts));shared={}
    for i in sorted(range(len(glyphs)),key=lambda i:(-frequency[chr(glyphs[i][0])],glyphs[i][0],glyphs[i][1])):
        cp,band,w,h,advance,encoded,raw=glyphs[i]
        if raw not in shared:shared[raw]=append(raw)
        tiles.GLYPH.pack_into(out,source.glyph_at+i*16,cp,shared[raw],band,w,h,advance,encoded)
    strings={};string_at=source.string_at+delta
    for i,text in enumerate(texts):
        raw=text.encode('utf-8')+b'\0';tiles.require(len(raw)<=tiles.STRING_LIMIT and b'\0' not in raw[:-1],'Localized text exceeds bounds')
        if raw not in strings:strings[raw]=append(raw)
        struct.pack_into('<II',out,string_at+i*8,strings[raw],len(raw)-1)
    tiles.HEADER.pack_into(out,0,b'NSP2',source.version,tiles.REFERENCE,len(out),ni,len(glyphs),ns,nc,nk,0)
    target=tiles.PackView(out,aux,versions=VERSIONS);after=split.layout(out,language)
    tiles.require(target.cards==source.cards and target.clips==source.clips and target.aux==source.aux,'Model IDs or animation clocks changed')
    tiles.require(bytes(out[after['prefix']:after['prefix']+after['artwork']])==data[before['prefix']:art_end],'Shared artwork changed')
    tiles.require(header[8]<=tiles.DIRECTORY_LIMIT,'Resident font directory exceeds 2MiB')
    # Use the existing bounded grouping codecs; preserve v39 after their
    # standalone historic writers' v32/v33 tag has been emitted.
    import group_glyphs,group_strings
    grouped,_=group_glyphs.write(target,group_glyphs.plan(target),after)
    grouped=bytearray(grouped);struct.pack_into('<I',grouped,4,source.version)
    target=tiles.PackView(grouped,aux,versions=VERSIONS);bounds=split.layout(grouped,language)
    result,_=group_strings.write(target,group_strings.plan(target),group_strings.nonstring_end(target,bounds))
    result=bytearray(result);struct.pack_into('<I',result,4,source.version)
    final=tiles.PackView(result,aux,versions=VERSIONS)
    for i,text in enumerate(texts):tiles.require(final.string_payload(i)==text.encode('utf-8')+b'\0','Localized string roundtrip differs')
    return bytes(result),dict(old_glyphs=ng,new_glyphs=len(glyphs),directory_bytes=header[8],prefix_bytes=after['prefix'],
        artwork_bytes=before['artwork'],artwork_sha256=tiles.sha256(data[before['prefix']:art_end]),
        unchanged_artwork=True,unchanged_model_ids_and_clip_clocks=True,strings_verified=ns)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path,required=True)
    parser.add_argument('--pck',type=Path,required=True)
    parser.add_argument('--fonts',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--languages',default='eng,zhs',help='Comma-separated original codes, or all')
    parser.add_argument('--keep-logical',action='store_true')
    args=parser.parse_args()
    from pck import Pack
    language_list=list(split.LANGUAGES) if args.languages=='all' else args.languages.split(',')
    tiles.require(set(language_list)<=set(split.LANGUAGES),'Unsupported original locale')
    output=tiles.private_path(args.output,'Private locale output');output.mkdir(parents=True,exist_ok=True)
    data=tiles.private_path(args.input,'Private immutable UI39 input').read_bytes()
    source=tiles.PackView(data,auxiliary_count(data),versions=VERSIONS);pack=Pack(tiles.private_path(args.pck,'Private pinned PCK'))
    reports={};common=None
    try:
        for language in language_list:
            texts,text_report=localized_texts(source,pack,language)
            texts,clusters=cluster_texts(texts)
            glyphs,font_report=build_glyphs(source,texts,clusters,args.fonts,language)
            localized,layout_report=rewrite(data,texts,glyphs,language)
            row=split.layout(localized,language);art=localized[row['prefix']:row['prefix']+row['artwork']]
            candidate=split.common_file(art)
            if common is None:common=candidate;tiles.atomic_bytes(output/'nSpire2.common.tns',common)
            tiles.require(candidate==common,'Locale editions do not share exact artwork')
            language_file=split.compress_language_directory(split.language_file(localized,row,language,common[32:64]))
            tiles.require(split.reconstruct(common,language_file,language)==localized,'Split locale readback differs')
            tiles.atomic_bytes(output/f'nSpire2.{language}.tns',language_file)
            if args.keep_logical:tiles.atomic_bytes(output/f'ui11-{language}.nsp',localized)
            reports[language]=dict(**text_report,fonts=font_report,layout=layout_report,
                bytes=len(language_file),sha256=tiles.sha256(language_file),logical_bytes=len(localized))
            tiles.atomic_bytes(output/f'{language}-receipt.json',json.dumps(reports[language],ensure_ascii=False,indent=2).encode()+b'\n')
            print(json.dumps(dict(language=language,bytes=len(language_file),glyphs=len(glyphs),directory_bytes=layout_report['directory_bytes'])),flush=True)
    finally:pack.close()
    tiles.atomic_bytes(output/'locales-receipt.json',json.dumps(dict(schema=1,reference='v0.111.0 / 41cef1ea',
        common_bytes=len(common or b''),optional_language_installation=True,languages=reports),ensure_ascii=False,indent=2).encode()+b'\n')


if __name__=='__main__':main()
