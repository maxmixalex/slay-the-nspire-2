"""Bound source mesh textures without changing a single retained texel.

The mesh keeps its logical UV width and height. Only the linear storage of a
dense texture is paged into native images, each at most256x240. Most long,
thin textures need one reshaped image; the two larger pinned textures need a
second page. Existing geometry, emitter curves and source frame clocks are
copied byte-for-byte. Version3 adds one continuation image ID per layer.
"""
from __future__ import annotations

import argparse
import array
import copy
import hashlib
import json
import math
from pathlib import Path
import shutil
import struct

import numpy as np
from PIL import Image

import content_tiles as tiles
from lz4_block import decode, encode

PAGE_WIDTH, PAGE_HEIGHT = 256, 240
PAGE_PIXELS = PAGE_WIDTH*PAGE_HEIGHT
PARAMETER_COUNT, HEADER_WORDS, FRAME_WORDS = 116, 12, 5
OLD_LAYER_WORDS, LAYER_WORDS, VERSION = 6, 7, 3


def decoded(row, folder):
    data=(folder/row['file']).read_bytes()
    tiles.require(hashlib.sha256(data).hexdigest()==row['sha256'], 'Vantom page input checksum changed')
    if row['format']==5:
        tiles.require(data[0]==row['decoded_format'], 'Vantom page input codec changed')
        data=decode(data[5:],struct.unpack_from('<I',data,1)[0])
    else:tiles.require(row['format']==row['decoded_format'], 'Vantom page input format changed')
    tiles.require(hashlib.sha256(data).hexdigest()==row['raw_sha256'], 'Vantom page input pixels changed')
    return data


def store(folder, name, width, height, fmt, raw):
    tiles.require(0<width<=320 and 0<height<=240, 'Vantom page exceeds native image bounds')
    packed=struct.pack('<BI',fmt,len(raw))+encode(raw)
    payload,stored=(packed,5) if len(packed)<len(raw) else (raw,fmt)
    filename=name+'.bin';tiles.atomic_bytes(folder/filename,payload)
    return dict(file=filename,width=width,height=height,format=stored,decoded_format=fmt,
                bytes=len(payload),sha256=tiles.sha256(payload),raw_sha256=tiles.sha256(raw),
                preserve_payload=True)


def words_image(folder,name,words):
    tiles.require(len(words)<=65535, 'Paged Vantom parameter table exceeds word bound')
    width=256 if len(words)<=256*240 else 320
    height=math.ceil(len(words)/width)
    raw=struct.pack('<'+'H'*(width*height),*(list(words)+[0]*(width*height-len(words))))
    return store(folder,name,width,height,0,raw)


def bounded_catalog(catalog, folder):
    """Transform one emitted catalog and prove exact native pixel recovery."""
    if catalog.get('parameter_version')==VERSION:return catalog
    tiles.require(catalog.get('parameter_version')==2, 'Expected source-verified Vantom version2 catalog')
    result=copy.deepcopy(catalog);base=result['first_parameter_image'];rows=result['images']
    all_rows={base+i:row for i,row in enumerate(rows)}
    all_rows.update({row['image']:row for row in result['replacement_images']})
    second={};changed=[];original_cost=sum(row['bytes'] for row in all_rows.values())
    for image,row in list(all_rows.items()):
        if row['width']<=320 and row['height']<=240:continue
        tiles.require(row.get('preserve_payload') and row['decoded_format']==1 and row.get('png'),
                      'Only dense sampled mesh textures can need linear paging')
        count=row['width']*row['height'];raw=decoded(row,folder)
        tiles.require(count<=2*PAGE_PIXELS and len(raw)==count*3, 'Vantom source texture exceeds two native pages')
        rgba=np.asarray(Image.open(folder/row['png']).convert('RGBA')).reshape(-1,4)
        tiles.require(len(rgba)==count, 'Vantom texture PNG dimensions differ from exact native pixels')
        pages=[]
        for page,start in enumerate(range(0,count,PAGE_PIXELS)):
            length=min(PAGE_PIXELS,count-start);width=PAGE_WIDTH;height=math.ceil(length/width);padded=width*height
            pixels=raw[start*2:(start+length)*2]+bytes((padded-length)*2)
            alpha=raw[2*count+start:2*count+start+length]+bytes(padded-length)
            key='paged-'+row['raw_sha256']+'-'+str(page)
            part=store(folder,key,width,height,1,pixels+alpha)
            picture=np.zeros((padded,4),dtype=np.uint8);picture[:length]=rgba[start:start+length]
            part['png']=key+'.png';Image.fromarray(picture.reshape(height,width,4)).save(folder/part['png'])
            pages.append((part,length))
        rebuilt_rgb=b''.join(decoded(part,folder)[:length*2] for part,length in pages)
        rebuilt_alpha=b''.join(decoded(part,folder)[part['width']*part['height']*2:part['width']*part['height']*2+length]
                               for part,length in pages)
        tiles.require(rebuilt_rgb+rebuilt_alpha==raw, 'Native Vantom texture paging changed an RGB565 or alpha8 value')
        if len(pages)>1:
            extra=base+len(rows);tiles.require(extra<65535,'Vantom continuation exceeds image ID bound')
            continuation=dict(pages[1][0],image=extra);rows.append(continuation);second[image]=extra
        original=dict(row);row.clear();row.update(pages[0][0])
        if 'image' in original:row['image']=original['image']
        changed.append(dict(image=image,logical_size=[original['width'],original['height']],pages=len(pages),
                            source_raw_sha256=original['raw_sha256'],exact_native_pixels=True))
    geometry_checks=0;frame_count=0
    for index,row in enumerate(list(rows[:PARAMETER_COUNT])):
        old=array.array('H',decoded(row,folder));tiles.require(old[:2].tolist()==[0x5654,2], 'Source Vantom table version differs')
        tiles.require(old[8]==OLD_LAYER_WORDS and old[9]==HEADER_WORDS, 'Source Vantom table layout differs')
        count=old[2];new=list(old[:HEADER_WORDS]);new[1]=VERSION;new[8]=LAYER_WORDS
        new.extend([0]*(count*FRAME_WORDS))
        for frame in range(count):
            at=HEADER_WORDS+frame*FRAME_WORDS;layers,n,tx,ty,em=old[at:at+FRAME_WORDS]
            new[at:at+FRAME_WORDS]=[len(new),n,tx,ty,0];frame_count+=1
            for layer in range(n):
                values=old[layers+layer*OLD_LAYER_WORDS:layers+(layer+1)*OLD_LAYER_WORDS].tolist()
                image,geometry,offset,mode,depth,mask=values
                tiles.require(mask not in second, 'Pinned dissolve mask unexpectedly needs a second texture page')
                tiles.require(image not in second or (mode&3)==3, 'Only a non-rigid mesh may use a continuation page')
                new.extend(values+[second.get(image,65535)])
                if mode&3==3:geometry_checks+=1
            new[at+4]=len(new);keys=old[em];end=em+1+keys+keys*4*14
            tiles.require(end<=old[10], 'Source emitter curve exceeds parameter table')
            new.extend(old[em:end])
        new[10]=len(new);replacement=words_image(folder,f'parameters-paged-{index:03d}',new)
        rows[index]=replacement
    result.update(parameter_version=VERSION,layer_words=LAYER_WORDS,texture_page_pixels=PAGE_PIXELS,
                  bounded_mesh_textures=True,texture_pages=changed,continuation_images=len(second),
                  original_parameter_version=2,unique_images=result['unique_images']+len(second),
                  appended_layer_images=result['appended_layer_images']+len(second),
                  directory_added_images=result['directory_added_images']+len(second),
                  parameter_bytes=sum(row['bytes'] for row in rows[:PARAMETER_COUNT]))
    result['image_bytes']=sum(row['bytes'] for row in rows[PARAMETER_COUNT:]+result['replacement_images'])
    result['texture_page_proof']=dict(native_rgb565_alpha8_exact=True,source_geometry_unchanged=True,
        source_emitters_unchanged=True,original_frames=frame_count,mesh_layers=geometry_checks,
        changed_textures=len(changed),continuation_images=len(second),
        payload_delta_bytes=result['parameter_bytes']+result['image_bytes']-original_cost)
    tiles.require(all(0<row['width']<=320 and 0<row['height']<=240 for row in rows+result['replacement_images']),
                  'Final Vantom image dimensions still exceed the native resource contract')
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();source=json.loads(args.input.read_text());args.output.mkdir(parents=True,exist_ok=True)
    for row in source['images']+source['replacement_images']:
        for field in ('file','png'):
            if field in row:
                origin=args.input.parent/row[field];target=args.output/row[field]
                if origin.resolve()!=target.resolve():shutil.copyfile(origin,target)
    result=bounded_catalog(source,args.output)
    result['source_catalog_sha256']=tiles.sha256(args.input.read_bytes())
    tiles.atomic_bytes(args.output/'vantom-assets.json',json.dumps(result,indent=2).encode()+b'\n')
    print(json.dumps(result['texture_page_proof'],sort_keys=True))


if __name__=='__main__':main()
