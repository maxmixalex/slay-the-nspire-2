#!/usr/bin/env python3
"""Split immutable artwork by concern and compress locale addressing pages.

NSM1 maps the existing NSC1 bytes onto eight concern files. NSZ1 is a random
access LZ4 wrapper around an unchanged NSL1/NSL2 file. These storage adapters
change no image, animation, glyph, string, directory ID, or game-save format.
All original data and output packs must remain outside the public repository.
"""
from __future__ import annotations
import argparse
from collections import Counter,defaultdict
import ctypes
import ctypes.util
import hashlib
import json
from pathlib import Path
import re
import struct

import numpy as np
from lz4_block import decode as decode_lz4,encode as encode_lz4
from ui12_resources import view
import content_tiles as tiles
import split_content as split

BLOCK=4096
MAX_BLOCKS=4096
MAX_SPANS=8192
NAMES=('interface','cards','characters','enemies','bosses','rooms','effects','shared')
PART_FILES=tuple('nSpire2.'+name+'.dat.tns' for name in NAMES)
U32=struct.Struct('<I')
LITERAL=0x80000000
MASK=0x7fffffff

def compression_function():
    try:
        import lz4.block
        return lambda raw:lz4.block.compress(raw,mode='high_compression',compression=12,store_size=False)
    except ImportError:
        name=ctypes.util.find_library('lz4')
        if not name:return encode_lz4
        lib=ctypes.CDLL(name)
        lib.LZ4_compress_HC.argtypes=(ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int,ctypes.c_int,ctypes.c_int)
        lib.LZ4_compress_HC.restype=ctypes.c_int
        output=ctypes.create_string_buffer(BLOCK+256)
        def compress(raw):
            n=lib.LZ4_compress_HC(raw,output,len(raw),len(output),12)
            if n<=0:raise ValueError('LZ4 encoding failed')
            return output.raw[:n]
        return compress

def compress_blocks(data):
    tiles.require(0<len(data)<=BLOCK*MAX_BLOCKS,'Locale exceeds independent-page bound')
    encode=compression_function();rows=[];payload=bytearray();blocks=(len(data)+BLOCK-1)//BLOCK
    begin=32+4*(blocks+1);compressed=0
    for at in range(0,len(data),BLOCK):
        raw=data[at:at+BLOCK];candidate=encode(raw)
        # The descriptor table costs four bytes per page. Avoid decoding for
        # a trivial saving: the literal branch also coalesces adjacent pages.
        smaller=len(candidate)+16<len(raw)
        rows.append(begin+len(payload)|(0 if smaller else LITERAL))
        payload.extend(candidate if smaller else raw);compressed+=smaller
    rows.append(begin+len(payload))
    output=struct.pack('<4s7I',b'NSZ1',1,tiles.REFERENCE,begin+len(payload),len(data),BLOCK,blocks,begin)
    output+=struct.pack('<'+'I'*len(rows),*rows)+payload
    tiles.require(expand_blocks(output)==data,'Independent locale storage roundtrip differs')
    return bytes(output),dict(input_bytes=len(data),output_bytes=len(output),saved_bytes=len(data)-len(output),
        blocks=blocks,compressed_blocks=compressed,literal_blocks=blocks-compressed,
        block_bytes=BLOCK,logical_sha256=tiles.sha256(data),physical_sha256=tiles.sha256(output))

def expand_blocks(data):
    tiles.require(len(data)>=32,'Truncated NSZ1 header')
    tag,version,reference,physical,total,block,blocks,begin=struct.unpack_from('<4s7I',data)
    tiles.require((tag,version,reference,physical,block)==(b'NSZ1',1,tiles.REFERENCE,len(data),BLOCK) and
        0<total<=BLOCK*MAX_BLOCKS and blocks==(total+BLOCK-1)//BLOCK and
        begin==32+4*(blocks+1)<len(data),'Invalid NSZ1 header')
    offsets=struct.unpack_from('<'+'I'*(blocks+1),data,32)
    tiles.require(offsets[0]&MASK==begin and offsets[-1]==len(data),'Invalid NSZ1 addressing')
    output=bytearray()
    for i in range(blocks):
        first,last=offsets[i]&MASK,offsets[i+1]&MASK;raw=min(BLOCK,total-i*BLOCK)
        literal=bool(offsets[i]&LITERAL)
        tiles.require(begin<=first<last<=len(data) and
                      (last-first==raw if literal else last-first<raw),'Invalid NSZ1 page bounds')
        output.extend(data[first:last] if literal else decode_lz4(data[first:last],raw))
    tiles.require(len(output)==total,'NSZ1 output length differs')
    return bytes(output)

def image_categories(source):
    from world_ui_profile import CLIP_ROWS,AUXILIARY_ROWS,BASE_AUXILIARY
    categories=np.zeros(len(source.images),dtype='u1')
    def assign_clip(clip,category):
        if clip>=len(source.clips):return
        first,count,*_=source.clips[clip];categories[first:first+count]=category
    for clip in range(min(38,len(source.clips))):
        assign_clip(clip,3 if 4<=clip<=7 else 5 if clip in (8,9) else 2)
    boss_text=(Path(__file__).resolve().parents[1]/'src/enemy_encounter_definitions.inc').read_text()
    bosses={key.removesuffix('_BOSS').replace('_','') for key in
            re.findall(r'"([A-Z_]+)",EncounterClass::Boss',boss_text)}
    for row in CLIP_ROWS:
        model=row.get('model')
        category=4 if model and model.upper() in bosses else 3 if model else 5
        assign_clip(row['id'],category)
    for clip in range(1695,min(1703,len(source.clips))):assign_clip(clip,5)
    for clip in range(1703,len(source.clips)):assign_clip(clip,6)
    for image,*_ in source.cards:categories[image]=1
    for i,row in enumerate(AUXILIARY_ROWS,BASE_AUXILIARY):
        if i>=len(source.aux):break
        key=row['key'];image=source.aux[i]
        if key.startswith(('background','events.','ancients.','room.','treasure.','rest_actor.')):categories[image]=5
        elif key.startswith(('map.','intents.','rewards.')):categories[image]=0
    return categories

def concern_ranges(source):
    """Assign whole decoder groups using their actual image references.

    Shared compressed groups remain intact. The majority of frame references
    determines their file; a tied group goes into shared. File boundaries never
    split a requested compressed image stream.
    """
    category=image_categories(source);reader=tiles.TileReader(source)
    tile_votes=np.zeros((reader.tile_groups,len(NAMES)),dtype=np.int64)
    index_votes=[Counter() for _ in range(reader.index_groups)]
    records=defaultdict(list);retained=defaultdict(Counter);literals=defaultdict(Counter)
    for i,(at,_w,_h,fmt,size) in enumerate(source.images):
        if fmt==6:
            _global,group,record_at,record_size=tiles.DESCRIPTOR.unpack_from(source.data,at)
            records[group].append((record_at,record_size,int(category[i])))
        else:
            literals[(at,size)][int(category[i])]+=1
            if fmt==7:
                relative,length,*_=struct.unpack_from('<iIII',source.data,at)
                retained[(at+relative,length)][int(category[i])]+=1
    for group,rows in sorted(records.items()):
        raw,starts=reader.group(reader.tile_groups+group)
        for at,size,cat in rows:
            tiles.require(starts.get(at)==size,'Frame record does not cover a complete index')
            _decoded,index_bytes,_geometry=tiles.FRAME.unpack_from(raw,at)
            ids=np.frombuffer(raw,dtype='<u4',count=index_bytes//4,offset=at+tiles.FRAME.size)
            tiles.require(index_bytes%4==0 and np.all(ids<reader.tile_count),'Invalid referenced tile ID')
            tile_votes[:,cat]+=np.bincount(ids//reader.per_group,minlength=reader.tile_groups)
            index_votes[group][cat]+=len(ids)
    def choose(counts):
        pairs=sorted(((int(n),int(i)) for i,n in enumerate(counts) if n),reverse=True)
        return pairs[0][1] if pairs and (len(pairs)==1 or pairs[0][0]>pairs[1][0]) else 7
    result={}
    for i,(at,size,*_) in enumerate(reader.groups):
        votes=tile_votes[i] if i<reader.tile_groups else [index_votes[i-reader.tile_groups][j] for j in range(len(NAMES))]
        result[(at-32,size+32)]=choose(votes)
    for entries in (retained,literals):
        for key,votes in entries.items():
            result[key]=choose([votes[j] for j in range(len(NAMES))])
    return sorted((at,size,cat) for (at,size),cat in result.items())

def partition(common,logical):
    source=view(logical);language=source.string_payload(split.MARKER)[:-1].decode('ascii');bounds=split.layout(logical,language)
    prefix,end=bounds['prefix'],bounds['prefix']+bounds['artwork']
    tiles.require(split.common_file(logical[prefix:end])==common,'Logical source does not match common artwork')
    ranges=[];cursor=prefix
    for at,size,cat in concern_ranges(source):
        if at<prefix or at>=end:continue
        tiles.require(at>=cursor and at+size<=end,'Concern ranges overlap or cross artwork boundary')
        if at>cursor:ranges.append((cursor,at,7))
        ranges.append((at,at+size,cat));cursor=at+size
    if cursor<end:ranges.append((cursor,end,7))
    # Fold tiny alignment gaps into the preceding concern. They are preserved
    # exactly, and no extra mapping/OS read is needed for a padding byte.
    for i,(first,last,cat) in enumerate(ranges):
        if cat==7 and last-first<=16 and i:ranges[i]=(first,last,ranges[i-1][2])
    merged=[]
    for first,last,cat in ranges:
        if merged and merged[-1][1]==first and merged[-1][2]==cat:
            merged[-1]=(merged[-1][0],last,cat)
        else:merged.append((first,last,cat))
    tiles.require(0<len(merged)<=MAX_SPANS,'Too many concern mappings')
    files=[bytearray(32) for _ in NAMES];rows=[]
    for first,last,cat in merged:
        physical=len(files[cat]);files[cat].extend(logical[first:last])
        rows.append((64+first-prefix,64+last-prefix,physical,cat))
    for i,buf in enumerate(files):struct.pack_into('<4s7I',buf,0,b'NSS1',1,tiles.REFERENCE,32,len(buf),i,0,0)
    index=struct.pack('<4s7I',b'NSM1',1,tiles.REFERENCE,96,96+len(rows)*16,len(common),len(rows),len(files))+common[:64]
    index+=b''.join(struct.pack('<4I',*row) for row in rows)
    files={name:bytes(data) for name,data in zip(PART_FILES,files)}
    tiles.require(join_parts(index,files)==common,'Concern split changed artwork bytes')
    return index,files,dict(spans=len(rows),virtual_bytes=len(common),index_bytes=len(index),
        installed_bytes=len(index)+sum(len(data) for data in files.values()),
        complete_byte_identical_roundtrip=True,
        files={name:dict(bytes=len(data),sha256=tiles.sha256(data)) for name,data in files.items()})

def join_parts(index,parts):
    tiles.require(len(index)>=96,'Truncated NSM1 header')
    tag,version,reference,header,length,virtual,spans,count=struct.unpack_from('<4s7I',index)
    tiles.require((tag,version,reference,header,length,count)==(b'NSM1',1,tiles.REFERENCE,96,len(index),len(NAMES)) and
        0<spans<=MAX_SPANS and len(index)==96+spans*16,'Invalid NSM1 header')
    output=bytearray(index[32:96])
    tiles.require(struct.unpack_from('<4s5I',output)==
                  (b'NSC1',1,tiles.REFERENCE,64,virtual,virtual-64),
                  'Invalid embedded NSC1 header')
    for i,name in enumerate(PART_FILES):
        data=parts[name]
        tiles.require(len(data)>=32 and struct.unpack_from('<4s7I',data)==
                      (b'NSS1',1,tiles.REFERENCE,32,len(data),i,0,0),'Invalid concern file '+name)
    for i in range(spans):
        first,last,physical,part=struct.unpack_from('<4I',index,96+i*16)
        tiles.require(first==len(output)<last<=virtual and part<count,'Invalid concern range')
        data=parts[PART_FILES[part]]
        tiles.require(32<=physical<=len(data) and last-first<=len(data)-physical,'Concern range exceeds file')
        output.extend(data[physical:physical+last-first])
    tiles.require(len(output)==virtual,'Missing concern ranges')
    return bytes(output)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--input',type=Path,required=True,help='Directory containing NSC1 common and locale files')
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--extra-locales',type=Path)
    a=p.parse_args();source=tiles.private_path(a.input,'Private input');target=tiles.private_path(a.output,'Private output')
    tiles.require(source!=target,'Never overwrite the input edition');target.mkdir(parents=True,exist_ok=True)
    common=(source/'nSpire2.dat.tns').read_bytes();eng=(source/'nSpire2.eng.dat.tns').read_bytes()
    logical=split.reconstruct(common,eng,'eng');index,parts,report=partition(common,logical)
    tiles.atomic_bytes(target/'nSpire2.dat.tns',index)
    for name,data in parts.items():tiles.atomic_bytes(target/name,data)
    report['locales']={};inputs={}
    for directory in (source,a.extra_locales):
        if directory:
            for code in split.LANGUAGES:
                path=directory/f'nSpire2.{code}.dat.tns'
                if path.is_file():inputs[code]=path
    for code,path in inputs.items():
        raw=path.read_bytes();raw=expand_blocks(raw) if raw[:4]==b'NSZ1' else raw
        packed,details=compress_blocks(raw);tiles.atomic_bytes(target/path.name,packed)
        report['locales'][code]=details
        print(json.dumps(dict(stage='locale_storage',language=code,**details)),flush=True)
    report['complete']=True
    report['total_saved_bytes']=sum(r['saved_bytes'] for r in report['locales'].values())-(report['installed_bytes']-len(common))
    tiles.atomic_bytes(target/'storage-receipt.json',json.dumps(report,indent=2).encode()+b'\n')
    print(json.dumps(report,indent=2))

if __name__=='__main__':main()
