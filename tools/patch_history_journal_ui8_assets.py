#!/usr/bin/env python3
"""Append four source history sprite-font icons to v30 without rebaking v29."""
import argparse
import json
from pathlib import Path
import content_tiles as tiles
from history_journal_ui8_assets import BASE_AUX,COUNT,VERSION,build_images
from patch_frontend_assets import patch_pack
from pck import Pack


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pck',required=True,type=Path)
    parser.add_argument('--input-eng',required=True,type=Path)
    parser.add_argument('--input-zhs',required=True,type=Path)
    parser.add_argument('--cache-dir',required=True,type=Path)
    parser.add_argument('--output-dir',required=True,type=Path)
    args=parser.parse_args();cache=tiles.private_path(args.cache_dir,'History icon cache');cache.mkdir(parents=True,exist_ok=True)
    output=tiles.private_path(args.output_dir,'History output')
    pack=Pack(tiles.private_path(args.pck,'Original pinned PCK'))
    try:rows=build_images(pack,cache)
    finally:pack.close()
    tiles.require(len(rows)==COUNT and all(row['auxiliary']==BASE_AUX+i for i,row in enumerate(rows)),'History icon sequence changed')
    manifest=dict(schema=1,reference='v0.111.0/41cef1ea',output_version=VERSION,images=rows,clips=[],
        localized_text={'eng':{},'zhs':{}},auxiliary_count=BASE_AUX+COUNT,animation_count=1765,runtime_added_pixel_buffers=0)
    tiles.atomic_bytes(cache/'history-journal-assets.json',json.dumps(manifest,indent=2).encode()+b'\n')
    receipts={}
    for language in ('eng','zhs'):
        source=tiles.private_path(getattr(args,'input_'+language),'Previous private pack');target=output/f'ui9-{language}.nsp'
        tiles.require(source.resolve()!=target.resolve(),'Journal icon append must preserve its v29 input')
        data,receipt=patch_pack(source.read_bytes(),manifest,cache,base_aux=BASE_AUX,auxiliary=BASE_AUX+COUNT,
            input_version=29,version=VERSION,texts=(),base_clips=1765,base_strings=8704)
        tiles.atomic_bytes(target,data);receipts[language]=receipt
    tiles.atomic_bytes(output/'history-journal-conversion.json',json.dumps(receipts,indent=2).encode()+b'\n')
    print(json.dumps(dict(version=VERSION,auxiliary=BASE_AUX+COUNT,clips=1765,languages=receipts)),flush=True)


if __name__=='__main__':main()
