#!/usr/bin/env python3
"""Select smaller existing dense tile streams only when complete ARM work falls.

The old TGR1/TGR2 stream remains the fallback. Every applied candidate restores
all 770-byte records exactly and its production ARM LZMA2 plus inverse takes no
more instructions than the original stream in the baseline launcher. This is
an offline tool; it adds no format, allocation, startup scan or runtime code.
Full-frame release measurements remain necessary after resource integration.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ProcessPoolExecutor, as_completed
import json
import lzma
import multiprocessing
from pathlib import Path
import struct
import sys
import time

import numpy as np

import coalesce_tile_resources as coalesce
import content_tiles as t
from tile_filter import filter_tiles, restore_tiles

PROFILES = ((0,1,1), (2,0,1), (2,0,2), (2,1,1), (4,0,1),
            (3,1,1), (3,0,2), (0,1,0), (3,0,0))
ENCODER = b'ui22-dense-pixels-v1-nice273-depth512'
MEASUREMENT = b'ui22-complete-arm-dense-v1'


def exact_records(original, dictionary):
    header = list(t.GROUP.unpack_from(original))
    t.require(header[0] in (b'TGR1', b'TGR2') and header[1] == 1 and
              header[3] == header[6]*770 and 0 < header[6] <= 1360 and
              header[4] == len(original)-32 and header[5] == 0 and
              header[7] in ((0,) if header[0] == b'TGR1' else (1,2)),
              'Expected one complete dense pixel group')
    decoded = t.decode_lzma2(original[32:], header[3], dictionary)
    raw = restore_tiles(decoded, header[7]) if header[7] else decoded
    records = np.frombuffer(raw,dtype='u1').reshape(-1,770)
    t.require(np.all((records[:,:2] >= 1) & (records[:,:2] <= 16)),
              'Original tile dimensions are invalid')
    return header, raw


def prepare_candidates(original, dictionary, cache):
    """Keep a size winner per existing mode; cache by exact raw bytes, not IDs."""
    header, raw = exact_records(original, dictionary)
    profiles = tuple(dict.fromkeys((coalesce.source_lzma_profile(original[32:]), *PROFILES)))
    key = t.sha256(ENCODER+struct.pack('<I',dictionary)+
                   bytes(value for profile in profiles for value in profile)+raw)
    folder = Path(cache)/'encoded'/key; folder.mkdir(parents=True,exist_ok=True)
    candidates = []
    for mode in (0,1,2):
        transformed = filter_tiles(raw,mode) if mode else raw
        t.require((restore_tiles(transformed,mode) if mode else transformed) == raw,
                  'Existing forward/inverse tile transform changed a byte')
        path = folder/f'mode{mode}.bin'; meta = folder/f'mode{mode}.json'
        if path.is_file() and meta.is_file():
            encoded = path.read_bytes(); record = json.loads(meta.read_text())
            profile = tuple(record['profile'])
            t.require(record['key'] == key and record['mode'] == mode and
                      profile in profiles and record['sha256'] == t.sha256(encoded),
                      'Cached pixel candidate identity differs')
        else:
            encoded, profile = None, None
            for lc,lp,pb in profiles:
                candidate = lzma.compress(transformed,format=lzma.FORMAT_RAW,filters=[dict(
                    id=lzma.FILTER_LZMA2,preset=9,dict_size=dictionary,
                    nice_len=273,depth=512,lc=lc,lp=lp,pb=pb)])
                if encoded is None or len(candidate) < len(encoded):
                    encoded, profile = candidate, (lc,lp,pb)
            t.atomic_bytes(path,encoded)
            t.atomic_bytes(meta,json.dumps(dict(key=key,mode=mode,profile=profile,
                sha256=t.sha256(encoded)),sort_keys=True).encode()+b'\n')
        t.require(t.decode_lzma2(encoded,len(raw),dictionary) == transformed,
                  'Cached/new dense pixel stream is not exact')
        ch = header.copy(); ch[0],ch[4],ch[7] = (b'TGR2' if mode else b'TGR1'),len(encoded),mode
        payload = t.GROUP.pack(*ch)+encoded
        physical = payload+bytes((len(original)-len(payload)) & 1)
        if len(physical) < len(original):
            candidates.append((payload,profile))
    candidates.sort(key=lambda row: (len(row[0]),row[0][28:32],row[1]))
    return raw, candidates


class ArmTiles:
    """The actual linked ARM926 synchronous decoder and in-place dense inverse."""
    def __init__(self, path):
        sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tests'))
        from arm_sparse_tile_regression import Runtime
        from arm_division_regression import linked_helper
        from unicorn import UC_HOOK_BLOCK
        from unicorn import arm_const
        self.runtime = runtime = Runtime(Path(path)); self.reg = arm_const
        self.emulator = runtime.emulator; self.elf_sha256 = t.sha256(runtime.data)
        self.symbols = {name:linked_helper(runtime.data,symbol)[1][0] for name,symbol in (
            ('lzma',b'_ZN5spire18decode_lzma2_blockEPhjjPKhjj'),
            ('dense',b'_ZN5spire28restore_tile_filter_in_placeEPhjjjS0_j'))}
        self.input = 0x42000000; self.emulator.mem_map(self.input,2*1024*1024)
        self.instructions = 0; self.emulator.hook_add(UC_HOOK_BLOCK,self.block)

    def block(self, _emulator, _address, size, _context):
        t.require(size%4 == 0,'Expected ordinary ARM instructions')
        self.instructions += size//4

    def call(self, name, arguments):
        e,r,state = self.emulator,self.reg,self.runtime
        e.reg_write(r.UC_ARM_REG_CPSR,0x10)
        for register,value in state.saved.items(): e.reg_write(register,value)
        for register,value in zip((r.UC_ARM_REG_R0,r.UC_ARM_REG_R1,r.UC_ARM_REG_R2,r.UC_ARM_REG_R3),arguments):
            e.reg_write(register,value)
        e.mem_write(state.sp,struct.pack('<II',*arguments[4:]))
        e.reg_write(r.UC_ARM_REG_SP,state.sp); e.reg_write(r.UC_ARM_REG_LR,state.stop)
        self.instructions = 0
        e.emu_start(self.symbols[name],state.stop,timeout=120000000,count=300000000)
        t.require(e.reg_read(r.UC_ARM_REG_PC) == state.stop and
                  e.reg_read(r.UC_ARM_REG_SP) == state.sp and e.reg_read(r.UC_ARM_REG_R0) == 1 and
                  all(e.reg_read(register) == value for register,value in state.saved.items()) and
                  self.instructions > 0,'Linked ARM decoder return, bound or AAPCS differs')
        return self.instructions

    def decode(self, payload, raw, mode, dictionary):
        e,state = self.emulator,self.runtime
        size,items = len(raw),len(raw)//770
        transformed = filter_tiles(raw,mode) if mode else raw
        target = state.destination+32
        e.mem_write(state.destination,b'\xb7'*(size+64)); e.mem_write(self.input,payload)
        property = 16 if dictionary == 1024*1024 else 14
        t.require(dictionary in (512*1024,1024*1024),'Unsupported linked decoder dictionary')
        lzma_work = self.call('lzma',(target,size,size,self.input,len(payload),property))
        t.require(bytes(e.mem_read(target,size)) == transformed,'Linked LZMA2 changed filtered bytes')
        inverse = 0
        if mode:
            capacity = items*258
            e.mem_write(state.scratch,b'\xa9'*(capacity+64))
            inverse = self.call('dense',(target,size,items,mode,state.scratch+32,capacity))
            t.require(bytes(e.mem_read(state.scratch,32)) == b'\xa9'*32 and
                      bytes(e.mem_read(state.scratch+32+capacity,32)) == b'\xa9'*32,
                      'Linked inverse changed a scratch guard')
        t.require(bytes(e.mem_read(target,size)) == raw and
                  bytes(e.mem_read(state.destination,32)) == b'\xb7'*32 and
                  bytes(e.mem_read(target+size,32)) == b'\xb7'*32,
                  'Linked decoder changed tile records or output guards')
        return dict(lzma_instructions=lzma_work,inverse_instructions=inverse,
            total_instructions=lzma_work+inverse,stored_bytes=len(payload),decoded_bytes=size,
            restored_sha256=t.sha256(raw),exact_records_guards_aapcs=True)


def validate_measurement(record, key, before_size, after_size, raw_size, raw_sha):
    t.require(record['schema'] == 1 and record['complete'] is True and record['key'] == key,
              'Cached ARM measurement identity differs')
    for name,size in (('before',before_size),('after',after_size)):
        row = record[name]
        t.require(all(type(row[field]) is int and row[field] >= 0 for field in
                  ('lzma_instructions','inverse_instructions','total_instructions','stored_bytes','decoded_bytes')) and
                  row['lzma_instructions'] > 0 and row['total_instructions'] ==
                  row['lzma_instructions']+row['inverse_instructions'] and
                  row['stored_bytes'] == size and row['decoded_bytes'] == raw_size and
                  row['restored_sha256'] == raw_sha and row['exact_records_guards_aapcs'] is True,
                  'Cached ARM counters or restored identity differ')
    return record['after']['total_instructions'] <= record['before']['total_instructions']


BASELINE = CANDIDATE = None


def initialize_workers(baseline, candidate):
    global BASELINE,CANDIDATE
    BASELINE,CANDIDATE = ArmTiles(baseline),ArmTiles(candidate)


def choose_group(group, original, dictionary, cache):
    raw, candidates = prepare_candidates(original,dictionary,cache)
    raw_sha, header = t.sha256(raw),t.GROUP.unpack_from(original)
    folder = Path(cache)/'measured'; folder.mkdir(exist_ok=True)
    best, measured, baseline_result = original, [], None
    for payload,profile in candidates:
        ch = t.GROUP.unpack_from(payload)
        key = t.sha256(MEASUREMENT+bytes.fromhex(BASELINE.elf_sha256+CANDIDATE.elf_sha256)+
            struct.pack('<III',dictionary,header[7],ch[7])+bytes.fromhex(raw_sha)+original[32:]+payload[32:])
        path = folder/(key+'.json')
        if path.is_file():
            proof = json.loads(path.read_text())
        else:
            if baseline_result is None:
                baseline_result = BASELINE.decode(original[32:],raw,header[7],dictionary)
            proof = dict(schema=1,complete=True,key=key,before=baseline_result,
                after=CANDIDATE.decode(payload[32:],raw,ch[7],dictionary))
            t.atomic_bytes(path,json.dumps(proof,sort_keys=True).encode()+b'\n')
        accepted = validate_measurement(proof,key,len(original)-32,len(payload)-32,len(raw),raw_sha)
        measured.append(dict(mode=ch[7],profile=profile,candidate_group_sha256=t.sha256(payload),
            candidate_bytes=len(payload),accepted=accepted,**proof))
        if accepted:
            best = payload
            break
    return group,best,dict(group=group,items=header[6],raw_bytes=len(raw),raw_sha256=raw_sha,
        original_group_sha256=t.sha256(original),chosen_group_sha256=t.sha256(best),
        source_mode=header[7],chosen_mode=t.GROUP.unpack_from(best)[7],source_bytes=len(original),
        chosen_bytes=len(best),source_profile=coalesce.source_lzma_profile(original[32:]),
        chosen_profile=coalesce.source_lzma_profile(best[32:]),measurements=measured,
        unchanged_fallback=best == original)


def convert(data, cache, baseline, candidate, *, workers=8, groups=None):
    t.require(1 <= workers <= 8,'Use between one and eight bounded encoder/ARM workers')
    source = coalesce.view(data); reader = t.TileReader(source)
    originals = {g:bytes(data[at-32:at+size]) for g,(at,size,*_) in enumerate(reader.groups)}
    wanted = set(range(reader.tile_groups)) if groups is None else set(groups)
    t.require(all(type(g) is int and 0 <= g < reader.tile_groups for g in wanted),
              'Only existing pixel group IDs may be selected')
    eligible = sorted(g for g in wanted if not reader.sparse[g])
    plan = {g:(payload,payload,payload) for g,payload in originals.items()}
    rows = []
    with ProcessPoolExecutor(max_workers=workers,mp_context=multiprocessing.get_context('spawn'),
            initializer=initialize_workers,initargs=(str(baseline),str(candidate))) as pool:
        jobs = {pool.submit(choose_group,g,originals[g],reader.dictionary,str(cache)):g for g in eligible}
        for future in as_completed(jobs):
            group,payload,row = future.result(); original = originals[group]
            physical = payload+bytes((len(original)-len(payload)) & 1)
            t.require(len(physical) <= len(original),'Pixel selection increased physical storage')
            plan[group] = original,payload,physical; rows.append(row)
            if len(rows)%16 == 0 or len(rows) == len(eligible):
                print(json.dumps(dict(stage='select_exact_arm_pixels',completed=len(rows),total=len(eligible),
                    saved_bytes=sum(r['source_bytes']-r['chosen_bytes'] for r in rows))),flush=True)
    changed = [g for g,(original,payload,_physical) in plan.items() if original != payload]
    padding = 0
    if changed:
        padding = sum(len(a)-len(c) for a,_b,c in plan.values())%4096
        group = changed[-1]; a,b,c = plan[group]; plan[group] = a,b,c+bytes(padding)
    output = coalesce.rewrite(data,plan,version=source.version)[0] if changed else data
    verification = coalesce.verify(data,output,plan,np.arange(reader.tile_count,dtype='<u4'),
                                    None,None,pixel_groups=False)
    rows.sort(key=lambda row:row['group'])
    return output,dict(complete=True,release_accepted=True,workers=workers,groups=rows,
        original_stream_is_fallback=True,maximum_complete_arm_regression_percent=0,
        group_boundaries_and_pixels_unchanged=True,resident_bytes_added=0,runtime_format_added=False,
        source_sha256=t.sha256(data),output_sha256=t.sha256(output),output_bytes=len(output),
        saved_bytes=len(data)-len(output),changed_groups=changed,suffix_alignment_padding=padding,
        baseline_elf_sha256=t.sha256(Path(baseline).read_bytes()),candidate_elf_sha256=t.sha256(Path(candidate).read_bytes()),
        verification=verification,limitation='Complete decoder ARM instructions; whole-frame and device gates are separate')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--work',type=Path,required=True)
    parser.add_argument('--baseline-elf',type=Path,required=True)
    parser.add_argument('--candidate-elf',type=Path,required=True)
    parser.add_argument('--workers',type=int,default=8)
    parser.add_argument('--groups',type=int,nargs='+')
    args = parser.parse_args()
    source = t.private_path(args.input,'Private pixel input'); target = t.private_path(args.output,'Private pixel output')
    cache = t.private_path(args.work,'Private pixel/ARM cache')
    t.require(source != target,'Never overwrite the source pixel pack')
    t.HOST_CACHE_GROUPS=16; started=time.monotonic()
    output,receipt = convert(source.read_bytes(),cache,args.baseline_elf,args.candidate_elf,
                             workers=args.workers,groups=args.groups)
    receipt['seconds'] = round(time.monotonic()-started,3)
    t.atomic_bytes(target,output); t.atomic_bytes(target.with_suffix('.json'),json.dumps(receipt,indent=2).encode()+b'\n')
    print(json.dumps({key:value for key,value in receipt.items() if key not in ('groups','verification','changed_groups')}),flush=True)


if __name__ == '__main__':
    main()
