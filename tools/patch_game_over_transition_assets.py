#!/usr/bin/env python3
"""Append the original Game Over transition mask; v30 -> v31, no old rebake."""
import argparse
import json
from pathlib import Path
import struct
import numpy as np
import content_tiles as tiles
from lz4_block import encode as lz4_encode
from patch_frontend_assets import patch_pack
from pck import Pack
from world_scene_assets import load_scene_texture

BASE_AUX,VERSION=2049,31
SOURCE='images/ui/transitions/game_over_transition.png'
WIDTH,HEIGHT,DENOMINATOR=320,240,510


def sample_mask(image):
    """Exact source linear-filter sample at each display-pixel center.

    The 2560x1200 texture is stretched by the fullscreen ColorRect. At 320x240,
    source centers are (8*x+3.5, 5*y+2): equal horizontal weights and an exact
    vertical texel. Store the unrounded sum of both red samples in a 16-bit
    scalar image; the runtime shader uses /510, not RGB565 color conversion.
    """
    tiles.require(image.size==(2560,1200),'Pinned Game Over transition dimensions changed')
    red=np.asarray(image.convert('RGB'),dtype=np.uint16)[:,:,0]
    samples=red[2::5,3::8]+red[2::5,4::8]
    tiles.require(samples.shape==(HEIGHT,WIDTH) and int(samples.max())<=DENOMINATOR,'Invalid projected transition mask')
    return samples.astype('<u2').tobytes()


def build_mask(pack,cache):
    material=pack.read('materials/transitions/game_over_transition_mat.tres').decode()
    shader=pack.read('shaders/texture_transition.gdshader').decode()
    for expected in ('shader_parameter/threshold = 0.0','shader_parameter/additionalFade = 0.0','shader_parameter/alpha = 0.9'):
        tiles.require(expected in material,'Pinned transition material changed: '+expected)
    for expected in ('1.0 - texture(transitionTex, UV).r','mix(-0.1, 1.1, threshold)','step(falloff, remap)'):
        tiles.require(expected in shader,'Pinned transition shader changed: '+expected)
    raw=sample_mask(load_scene_texture(pack,SOURCE))
    payload=struct.pack('<BI',0,len(raw))+lz4_encode(raw)
    key='game-over-transition-linear-red';file=key+'.bin'
    tiles.atomic_bytes(cache/file,payload)
    return dict(key=key,file=file,width=WIDTH,height=HEIGHT,format=5,decoded_format=0,
        bytes=len(payload),sha256=tiles.sha256(payload),raw_sha256=tiles.sha256(raw),
        auxiliary=BASE_AUX,source=SOURCE,source_size=[2560,1200],
        scalar_encoding='linear-red-u9-in-u16-le',scalar_denominator=DENOMINATOR)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for key in ('pck','input-eng','input-zhs','cache-dir','output-dir'):
        parser.add_argument('--'+key,required=True,type=Path)
    args=parser.parse_args();cache=tiles.private_path(args.cache_dir,'Transition cache');cache.mkdir(parents=True,exist_ok=True)
    output=tiles.private_path(args.output_dir,'Transition output');pack=Pack(tiles.private_path(args.pck,'Pinned PCK'))
    try:row=build_mask(pack,cache)
    finally:pack.close()
    manifest=dict(schema=1,reference='v0.111.0/41cef1ea',output_version=VERSION,images=[row],clips=[],
        localized_text={'eng':{},'zhs':{}},auxiliary_count=BASE_AUX+1,animation_count=1765,runtime_added_pixel_buffers=0)
    tiles.atomic_bytes(cache/'game-over-transition-assets.json',json.dumps(manifest,indent=2).encode()+b'\n')
    receipts={}
    for language in ('eng','zhs'):
        source=tiles.private_path(getattr(args,'input_'+language),'Previous private pack');target=output/f'ui9-{language}.nsp'
        tiles.require(source.resolve()!=target.resolve(),'Transition append must preserve the v30 input')
        data,receipt=patch_pack(source.read_bytes(),manifest,cache,base_aux=BASE_AUX,auxiliary=BASE_AUX+1,
            input_version=30,version=VERSION,texts=(),base_clips=1765,base_strings=8704)
        receipt.update(mask_scalar_denominator=DENOMINATOR,mask_decoded_bytes=WIDTH*HEIGHT*2,
                       runtime_added_pixel_buffers=0)
        tiles.atomic_bytes(target,data);receipts[language]=receipt
    tiles.atomic_bytes(output/'game-over-transition-conversion.json',json.dumps(receipts,indent=2).encode()+b'\n')
    print(json.dumps(dict(version=VERSION,auxiliary=BASE_AUX+1,clips=1765,languages=receipts)),flush=True)


if __name__=='__main__':main()
