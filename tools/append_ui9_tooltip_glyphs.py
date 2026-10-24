#!/usr/bin/env python3
"""Append missing original tooltip/title glyphs to a private v31..33 pack.

All existing glyph bitmaps, art, strings, model indices and clip registrations
stay byte-exact. This does not bake sprites or change the resource version.
"""
import argparse
import json
from pathlib import Path
import content_tiles as tiles
from patch_ui7_assets import append_glyphs
from replace_original_glyphs import Face, tooltip_small_bold_alphabet, character_title_display_alphabet, text_format_control
from repack_retained import auxiliary_count
import split_content


def verify_append(before, after, auxiliary, version):
    """Check all old keys/pixels/text and opaque art/group bytes after relocation."""
    source=tiles.PackView(before,auxiliary,versions=(version,))
    target=tiles.PackView(after,auxiliary,versions=(version,))
    tiles.TileReader(source);tiles.TileReader(target)
    tiles.require(source.cards==target.cards and source.clips==target.clips and source.aux==target.aux,
        'Glyph append changed model or clip indices')
    by_key={(row[0],row[2]):i for i,row in enumerate(target.glyphs)}
    for i,old in enumerate(source.glyphs):
        at=by_key.get((old[0],old[2]));tiles.require(at is not None,'Glyph append lost an old key')
        new=target.glyphs[at]
        tiles.require(new[2:]==old[2:] and source.glyph_payload(i)==target.glyph_payload(at) and
            source.glyph_alpha(i)==target.glyph_alpha(at),'Glyph append changed an old glyph')
    tiles.require(len(source.strings)==len(target.strings),'Glyph append changed the string directory')
    for i in range(len(source.strings)):
        tiles.require(source.strings[i][1]==target.strings[i][1] and source.string_payload(i)==target.string_payload(i),
            'Glyph append changed an old string or encoding')
    language=source.string_payload(split_content.MARKER)[:-1].decode('ascii')
    old,new=split_content.layout(before,language),split_content.layout(after,language)
    tiles.require(old['artwork']==new['artwork'] and
        before[old['prefix']:old['prefix']+old['artwork']]==after[new['prefix']:new['prefix']+new['artwork']],
        'Glyph append changed an artwork or compressed animation byte')
    delta=(len(target.glyphs)-len(source.glyphs))*tiles.GLYPH.size
    old_suffix=old['prefix']+old['artwork']
    tiles.require(after[old_suffix+delta:len(before)+delta]==before[old_suffix:],
        'Glyph append changed old glyph/string group streams or relative descriptors')
    return dict(old_glyphs_verified=len(source.glyphs),strings_verified=len(source.strings),
        exact_old_glyph_keys_metrics_alpha=True,exact_old_string_bytes_and_indices=True,
        exact_old_compressed_groups_and_descriptors=True,exact_artwork_and_animations=True,
        resident_cache_growth_bytes=0)


def missing_glyphs(source,face,display=None):
    known={(row[0],row[2]) for row in source.glyphs};rows=[]
    for cp in sorted(tooltip_small_bold_alphabet(source)):
        if cp<32 or text_format_control(cp) or (cp,101) in known:continue
        tiles.require(cp in face.coverage,'Original explicit-bold font misses a tooltip title')
        rows.append(face.glyph(cp,5,5))
    if display:
        for cp in sorted(character_title_display_alphabet(source)):
            if cp<32 or text_format_control(cp):continue
            tiles.require(cp in display.coverage,'Original display font misses a character title')
            for px in (10,12,14):
                if (cp,px|32) not in known:rows.append(display.glyph(cp,px,4))
    return rows


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',required=True,type=Path)
    parser.add_argument('--explicit-bold-font',required=True,type=Path)
    parser.add_argument('--display-font',type=Path)
    parser.add_argument('--output',required=True,type=Path)
    parser.add_argument('--receipt',required=True,type=Path)
    args=parser.parse_args()
    source_path=tiles.private_path(args.input,'Previous private pack')
    target_path=tiles.private_path(args.output,'Updated private pack')
    receipt_path=tiles.private_path(args.receipt,'Private font receipt')
    tiles.require(source_path.resolve()!=target_path.resolve(),'Glyph update must preserve its input')
    data=source_path.read_bytes();auxiliary=auxiliary_count(data)
    tiles.require(auxiliary in (2050,2051),'Unexpected final tooltip catalog')
    source=tiles.PackView(data,auxiliary,versions=(31,32,33),profile=(558,8704,1765,auxiliary))
    face=Face(tiles.private_path(args.explicit_bold_font,'Original explicit-bold font'))
    display=Face(tiles.private_path(args.display_font,'Original display font')) if args.display_font else None
    rows=missing_glyphs(source,face,display);out,count=append_glyphs(data,auxiliary,1765,rows,version=source.version)
    target=tiles.PackView(out,auxiliary,versions=(source.version,),profile=(558,8704,1765,auxiliary))
    tiles.require(len(target.glyphs)<=32768,'Tooltip glyphs exceed the runtime directory bound')
    proof=verify_append(data,out,auxiliary,source.version)
    tiles.require(not missing_glyphs(target,face,display),'A required source title glyph is still absent')
    language=source.string_payload(6110)[:-1].decode('ascii')
    receipt=dict(schema=2,reference='v0.111.0/41cef1ea',version=source.version,language=language,
        input_bytes=len(data),output_bytes=len(out),input_sha256=tiles.sha256(data),output_sha256=tiles.sha256(out),
        previous_glyphs=len(source.glyphs),glyphs=len(target.glyphs),added_glyphs=count,
        added_glyph_keys=[dict(codepoint=row[0],pixels=row[1]&31,face=(row[1]>>6)|((row[1]&32)>>3)) for row in rows],font=face.metadata(),display_font=display.metadata() if display else None,
        existing_glyph_bitmaps_exact=True,common_art_exact=True,existing_strings_exact=True,
        model_and_image_indices_exact=True,animation_registration_exact=True,verification=proof)
    tiles.atomic_bytes(target_path,bytes(out));tiles.atomic_bytes(receipt_path,json.dumps(receipt,indent=2).encode()+b'\n')
    print(json.dumps(receipt),flush=True)


if __name__=='__main__':main()
