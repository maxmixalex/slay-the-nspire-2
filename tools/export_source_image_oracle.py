#!/usr/bin/env python3
"""Export independently decoded original-target image bytes for the native oracle.

Outputs contain private source pixels and must stay outside the repository.
The complete default traverses every catalog image; optional ranges are marked
incomplete and cannot be mistaken for a full-catalog proof.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import content_tiles as t
from restore_animation_precision import read_image
from ui12_resources import view


def export(data: bytes, output: Path, first=0, count=None):
    source=view(data);reader=t.TileReader(source);total=len(source.images)
    count=total-first if count is None else count
    t.require(0<=first<total and 0<count<=total-first,'Invalid source image oracle range')
    temporary=output.with_suffix(output.suffix+'.partial');output.parent.mkdir(parents=True,exist_ok=True)
    size=decoded=0;digest=hashlib.sha256()
    with temporary.open('wb') as handle:
        def write(block):
            nonlocal size
            t.require(handle.write(block)==len(block),'Source image oracle short write')
            digest.update(block);size+=len(block)
        write(b'IMGR0022'+struct.pack('<5I',total,first,count,len(data),struct.unpack_from('<I',data,36)[0]))
        for image in range(first,first+count):
            width,height,fmt,raw=read_image(source,reader,image)
            t.validate_raw(raw,width,height,fmt)
            write(struct.pack('<5I',image,width,height,fmt,len(raw)));write(raw);decoded+=len(raw)
            if (image-first+1)%2000==0:
                print(json.dumps(dict(stage='independent_image_oracle',images=image-first+1,total=count,bytes=decoded)),flush=True)
        handle.flush();os.fsync(handle.fileno())
    t.require(temporary.stat().st_size==size,'Source image oracle truncated')
    verified=hashlib.sha256()
    with temporary.open('rb') as handle:
        for block in iter(lambda:handle.read(1024*1024),b''):verified.update(block)
    t.require(verified.digest()==digest.digest(),'Source image oracle changed during publication')
    os.replace(temporary,output)
    report=dict(complete=first==0 and count==total,images=count,catalog_images=total,first_image=first,
        input_sha256=t.sha256(data),oracle_sha256=digest.hexdigest(),oracle_bytes=size,decoded_image_bytes=decoded,
        independent_python_decode=True,comparison='All decoded bytes, dimensions and image formats; not hashes alone')
    t.atomic_bytes(output.with_suffix(output.suffix+'.json'),json.dumps(report,indent=2).encode()+b'\n')
    return report


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--first',type=int,default=0);parser.add_argument('--count',type=int)
    args=parser.parse_args();t.private_path(args.output,'Private image byte oracle')
    print(json.dumps(export(args.input.read_bytes(),args.output,args.first,args.count)),flush=True)


if __name__=='__main__':main()
