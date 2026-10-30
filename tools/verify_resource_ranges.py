#!/usr/bin/env python3
"""Run the complete native resource oracle with bounded temporary pixel spools.

Each invocation compares an explicit contiguous image interval. The receipt
requires their union to cover the whole catalog exactly once. The original
decoded page-cost metadata bounds each interval's temporary byte budget.
"""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import time

from content_version import MAXIMUM_CONTENT_VERSION


def costs(path):
    with path.open('rb') as handle: prefix=handle.read(2*1024*1024)
    tag,version,reference,size,ni,ng,ns,nc,nk,_checksum=struct.unpack_from('<4s9I',prefix)
    if tag!=b'NSP2' or not 20<=version<=MAXIMUM_CONTENT_VERSION or reference!=0x41cef1ea or size!=path.stat().st_size:
        raise ValueError('Expected a complete compatible logical pack')
    at=40+16*ni+16*ng+8*ns+16*nc+16*nk
    for _ in range(65535):
        if at+64>len(prefix):raise ValueError('Missing bounded TIL1 header')
        if prefix[at:at+4]==b'TIL1':break
        if struct.unpack_from('<I',prefix,at)[0]>=ni:raise ValueError('Invalid auxiliary directory')
        at+=4
    else:raise ValueError('Auxiliary directory exceeds bound')
    fields=struct.unpack_from('<4s15I',prefix,at)
    start,count=fields[14:16]
    if not ni or count!=ni or start+count>len(prefix):raise ValueError('Invalid decoded-cost metadata')
    # A zero hint means unknown to the preparer; use the absolute decoded
    # picture bound. Nonzero hints were verified by the producing pipeline.
    return [max(1,value)*4096+16 if value else 614880+16 for value in prefix[start:start+count]]


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary',type=Path,required=True)
    p.add_argument('--candidate',type=Path,required=True)
    p.add_argument('--reference',type=Path,required=True)
    p.add_argument('--receipt',type=Path,required=True)
    p.add_argument('--spool-mib',type=int,default=64)
    p.add_argument('--loader',type=Path)
    p.add_argument('--preload',action='store_true')
    a=p.parse_args();limit=a.spool_mib*1024*1024
    before,after=costs(a.reference),costs(a.candidate)
    if len(before)!=len(after) or not 1024*1024<=limit<=256*1024*1024:raise ValueError('Invalid range budget or mismatched catalogs')
    bounds=[max(x,y) for x,y in zip(before,after)];ranges=[];first=0;size=0
    for i,value in enumerate(bounds):
        if size and size+value>limit:ranges.append((first,i-first,size));first=i;size=0
        size+=value
    if size:ranges.append((first,len(bounds)-first,size))
    done=0;started=time.monotonic();rows=[]
    for first,count,size in ranges:
        if first!=done or not count or size>limit:raise ValueError('Range coverage or spool budget is invalid')
        command=([str(a.loader)] if a.loader else [])+[str(a.binary.resolve()),str(a.candidate),str(a.reference),'--images',str(first),str(count)]
        if a.preload:command.append('--preload')
        print(json.dumps(dict(stage='native_resource_range',first=first,count=count,spool_upper_bytes=size)),flush=True)
        t=time.monotonic();subprocess.run(command,check=True)
        rows.append(dict(first=first,count=count,spool_upper_bytes=size,elapsed_seconds=time.monotonic()-t));done+=count
    if done!=len(bounds):raise ValueError('Native oracle omitted catalog images')
    result=dict(complete=True,images=done,incremental=a.preload,ranges=rows,spool_limit_bytes=limit,
                candidate=str(a.candidate),reference=str(a.reference),elapsed_seconds=time.monotonic()-started)
    a.receipt.parent.mkdir(parents=True,exist_ok=True);a.receipt.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k!='ranges'}),flush=True)


if __name__=='__main__':main()
