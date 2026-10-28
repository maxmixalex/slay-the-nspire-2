#!/usr/bin/env python3
"""Sample independent complete Vantom clips concurrently, then assemble one catalog.

Each worker owns a reviewed runtime process and private input directory. The
per-clip signature/cache remains the same as the sequential converter, and all
geometry/timing assertions run unchanged. No frame, source scale, material or
particle insertion is omitted to make a worker finish early.
"""
from __future__ import annotations
import argparse
from concurrent.futures import ProcessPoolExecutor, as_completed
import json
import os
from pathlib import Path
import time

import numpy as np
import vantom_ui15_assets as v

_WORK = None


def initialize(pck, runtime, logical_pack, cache, sample_work, dotnet):
    global _WORK
    model, jobs, all_jobs = v.catalog()
    pack = v.Pack(pck)
    source = v.view(Path(logical_pack).read_bytes())
    worker = Path(cache)/('worker-'+str(os.getpid()))
    sampler = v.SpineSampler(pack, runtime=runtime, dotnet=dotnet, work_dir=worker)
    sampler.__enter__()
    _WORK = dict(model=model, jobs={row['id']:row for row in jobs},
        paired={row['key']:row for row in all_jobs if row['visual_state']['shrink']},
        source=source, pack=pack, sampler=sampler, identity=v.source_identity(pack,runtime),
        noise=np.load(Path(cache)/'vantom-tail-noise.npy'), sampled=Path(sample_work))


def sample(clip):
    data=_WORK;job=data['jobs'][clip];partner=data['paired'][job['key'].replace('Shrink=0','Shrink=1')]
    started=time.monotonic()
    result=v.sample_job(data['sampler'],data['model'],job,data['source'].clips[clip],
        data['source'].clips[partner['id']],data['sampled']/str(clip),data['identity'],data['noise'])
    return dict(event='vantom_sampled',clip=clip,frames=result['frames'],
        maximum_lcd_interpolation_error=result['maximum_lcd_interpolation_error'],
        maximum_layers=result['maximum_layers'],source_scales=result['independently_sampled_scales'],
        elapsed_seconds=time.monotonic()-started)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('pck','runtime','input','cache','fastnoise-header'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--sample-work',type=Path)
    parser.add_argument('--dotnet',default='dotnet')
    parser.add_argument('--workers',type=int,default=2)
    args=parser.parse_args()
    if not 1<=args.workers<=4:parser.error('workers must be within1..4')
    sample_work=args.sample_work or args.cache/'samples'
    from vantom_vfx_ui15_assets import tail_noise
    tail_noise(args.pck,args.fastnoise_header,args.cache)
    model,jobs,_=v.catalog()
    # Visit one actual compound clip from each family first, so a real source
    # topology or canvas issue is found before the repeated phase contexts.
    seen=set();first=[];rest=[]
    for job in jobs:
        (rest if job['clip'] in seen else first).append(job['id']);seen.add(job['clip'])
    errors=[]
    with ProcessPoolExecutor(args.workers,initializer=initialize,
          initargs=(args.pck,args.runtime,args.input,args.cache,sample_work,args.dotnet)) as pool:
        futures={pool.submit(sample,clip):clip for clip in first+rest}
        for future in as_completed(futures):
            clip=futures[future]
            try:print(json.dumps(future.result()),flush=True)
            except Exception as error:
                errors.append(dict(clip=clip,error=str(error)))
                print(json.dumps(dict(event='vantom_sample_failed',**errors[-1])),flush=True)
    if errors:raise RuntimeError('Vantom source sampling failed: '+json.dumps(errors))
    v.source_assets(args.pck,args.runtime,args.input,args.cache,dotnet=args.dotnet,
                    sample_work=sample_work,fastnoise_header=args.fastnoise_header)


if __name__=='__main__':main()
