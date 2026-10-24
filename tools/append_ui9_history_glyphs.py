#!/usr/bin/env python3
"""Append only missing original small-bold UI labels to a private v29 pack.

No old image, glyph bitmap, string or model index is regenerated. Input and
output are separate private files; version 29 and its clip registrations stay
unchanged. History uses 5px; Proceed/Skip/treasure labels use 6px. Full font
rebuilds share the same bounded alphabet helpers.
"""
import argparse
import json
from pathlib import Path
import content_tiles as tiles
from patch_ui7_assets import append_glyphs
from replace_original_glyphs import Face, history_small_bold_alphabet, control_small_bold_alphabet, text_format_control


def missing_glyphs(source, face):
    known={(row[0],row[2]) for row in source.glyphs};rows=[]
    for size,alphabet in ((5,history_small_bold_alphabet(source)),(6,control_small_bold_alphabet(source))):
        for cp in sorted(alphabet):
            if cp<32 or text_format_control(cp):continue
            tiles.require(cp in face.coverage,'Original explicit-bold font misses a source UI label')
            row=face.glyph(cp,size,5)
            if (row[0],row[1]) not in known:rows.append(row)
    return rows


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',required=True,type=Path)
    parser.add_argument('--explicit-bold-font',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    parser.add_argument('--receipt',required=True,type=Path)
    args=parser.parse_args()
    source_path=tiles.private_path(args.input,'Previous private pack')
    target_path=tiles.private_path(args.output,'Updated private pack')
    receipt_path=tiles.private_path(args.receipt,'Private font receipt')
    tiles.require(source_path.resolve()!=target_path.resolve(),'Glyph update must preserve its input')
    data=source_path.read_bytes();source=tiles.PackView(data,2045,versions=(29,),profile=(558,8704,1765,2045))
    face=Face(tiles.private_path(args.explicit_bold_font,'Original explicit-bold font'))
    rows=missing_glyphs(source,face);out,count=append_glyphs(data,2045,1765,rows,version=29)
    target=tiles.PackView(out,2045,versions=(29,),profile=(558,8704,1765,2045))
    by_key={(row[0],row[2]):row for row in target.glyphs}
    for cp,offset,px,w,h,advance,encoded in source.glyphs:
        old=(cp,offset,px,w,h,advance,encoded);new=by_key[(cp,px)]
        tiles.require(new[2:]==old[2:] and bytes(source.block(offset,encoded or w*h))==bytes(target.block(new[1],encoded or w*h)),
                      'History glyph append changed an existing glyph')
    tiles.require(not missing_glyphs(target,face),'A required history header glyph is still absent')
    marker,n=source.strings[6110];language=bytes(source.block(marker,n)).decode('ascii')
    receipt=dict(schema=1,reference='v0.111.0/41cef1ea',version=29,language=language,
        input_bytes=len(data),output_bytes=len(out),input_sha256=tiles.sha256(data),output_sha256=tiles.sha256(out),
        previous_glyphs=len(source.glyphs),glyphs=len(target.glyphs),added_glyphs=count,
        added_glyph_keys=[dict(codepoint=row[0],pixels=row[1]&31,face=5) for row in rows],font=face.metadata(),
        existing_glyph_bitmaps_exact=True,common_art_exact=True,existing_strings_exact=True,
        model_and_image_indices_exact=True,animation_registration_exact=True)
    tiles.atomic_bytes(target_path,bytes(out));tiles.atomic_bytes(receipt_path,json.dumps(receipt,indent=2).encode()+b'\n')
    print(json.dumps(receipt),flush=True)


if __name__=='__main__':main()
