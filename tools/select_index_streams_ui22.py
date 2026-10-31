#!/usr/bin/env python3
"""Choose smaller existing TGI2 index streams for exact-color NSP2 resources.

Only plain index groups are eligible. Tile pixels, group boundaries, frame
geometry, clocks, cache costs, retained slices, fonts and text remain exact.
The original stream is always a candidate. Preparing streams is explicit;
applying them requires a complete ARM gate bound to this input, both measured
ELFs, and every original/candidate stream. Slower candidates keep their input
bytes. Instruction counts do not replace the release whole-frame gates.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import lzma
from pathlib import Path
import struct
import time

import numpy as np

import coalesce_tile_resources as coalesce
import content_tiles as t
from tile_prediction import index_transform

PROFILES = ((2, 0, 0), (3, 1, 0), (4, 0, 0), (4, 0, 1))
ENCODER = b'ui22-existing-tgi2-v1-nice273-depth512'


def _sha(value):
    return isinstance(value,str) and len(value)==64 and all(c in '0123456789abcdef' for c in value)


def validate_arm_gate(data, reader, gate, elf_hashes):
    """Reject stale, partial, contradictory or foreign performance evidence."""
    try:
        t.require(isinstance(gate,dict) and type(gate['schema']) is int and gate['schema']==1 and gate['complete'] is True,
                  'ARM gate is incomplete or unsupported')
        for field in ('source_sha256','candidate_sha256','old_elf_sha256','new_elf_sha256'):
            t.require(_sha(gate[field]), 'Invalid ARM gate digest')
        t.require(gate['source_sha256']==t.sha256(data), 'ARM gate input pack differs')
        t.require(elf_hashes and tuple(elf_hashes)==(gate['old_elf_sha256'],gate['new_elf_sha256']),
                  'Measured ARM ELF hashes differ')
        t.require(type(gate['maximum_instruction_regression_percent']) in (int,float) and
                  gate['maximum_instruction_regression_percent']==0, 'ARM gate permits a CPU regression')
        rows=gate['rows'];t.require(isinstance(rows,list) and type(gate['total_candidates']) is int and
            len(rows)==gate['total_candidates'], 'ARM gate omits candidate results')
        measured={}
        for row in rows:
            group=row['group']
            t.require(type(group) is int and reader.tile_groups<=group<len(reader.groups) and
                      group not in measured and not reader.predictors[group], 'ARM gate group differs or repeats')
            at,size,raw,_,kind,items=reader.groups[group]
            t.require(kind==2 and row['records']==items and row['raw_bytes']==raw,
                      'ARM gate index shape differs')
            for key in ('original_group_sha256','candidate_group_sha256','raw_sha256'):
                t.require(_sha(row[key]), 'Invalid ARM group digest')
            t.require(t.sha256(data[at-32:at+size])==row['original_group_sha256'],
                      'ARM gate original group differs')
            for side in ('before','after'):
                result=row[side]
                t.require(all(type(result[k]) is int and result[k]>=0 for k in
                    ('lzma_instructions','inverse_instructions','total_instructions','stored_bytes',
                     'decoded_bytes','complete_bytes')), 'Invalid ARM decoder counters')
                t.require(result['lzma_instructions']>0 and result['total_instructions']==
                          result['lzma_instructions']+result['inverse_instructions'], 'ARM instruction total differs')
                t.require(result['decoded_bytes']==result['complete_bytes']==raw and
                          result['restored_sha256']==row['raw_sha256'], 'ARM decoded identity differs')
            t.require(row['before']['stored_bytes']==size and
                      row['after']['stored_bytes']<size and type(row['accepted']) is bool and
                      row['exact_rgb_index_geometry_guards_aapcs'] is True, 'ARM stream size or exactness differs')
            t.require(row['accepted']==(row['after']['total_instructions']<=row['before']['total_instructions']),
                      'ARM acceptance contradicts complete decoder work')
            measured[group]=row
        accepted={g for g,r in measured.items() if r['accepted']}
        rejected=set(measured)-accepted
        t.require(len(gate['accepted_groups'])==len(accepted) and set(gate['accepted_groups'])==accepted and
                  len(gate['rejected_groups'])==len(rejected) and set(gate['rejected_groups'])==rejected,
                  'ARM accepted/rejected directories differ')
        for side,field in (('before','before_instructions'),('after','after_instructions')):
            t.require(gate[field]==sum(measured[g][side]['total_instructions'] for g in accepted),
                      'ARM accepted aggregate differs')
        return measured
    except (KeyError,TypeError,IndexError) as error:
        raise ValueError('Malformed ARM gate receipt') from error


def aligned_plan(plan, align_suffix):
    result=dict(plan);changed=[g for g,(a,b,_c) in sorted(plan.items()) if a!=b]
    padding=0
    if align_suffix and changed:
        padding=sum(len(a)-len(c) for a,_b,c in plan.values())%4096
        group=changed[-1];a,b,c=result[group];result[group]=a,b,c+bytes(padding)
    return result,changed,padding


def choose_index(original, dictionary, tile_count, cache, *, minimum_percent=0):
    header = list(t.GROUP.unpack_from(original))
    t.require(header[0] == b'TGR1' and header[1] == 2 and header[7] == 0 and
              header[4] == len(original)-32, 'Expected one complete plain index group')
    raw = t.decode_lzma2(original[32:], header[3], dictionary)
    transformed = index_transform(raw, header[6], 3, tile_count=tile_count)
    t.require(index_transform(transformed, header[6], 3, True, tile_count) == raw,
              'TGI2 forward/inverse changed an index or geometry byte')
    source_profile = coalesce.source_lzma_profile(original[32:])
    profiles = tuple(dict.fromkeys((source_profile, *PROFILES)))
    key = t.sha256(ENCODER + struct.pack('<II', dictionary, tile_count) +
                   bytes(x for profile in profiles for x in profile) + original)
    path = cache/(key+'.bin')
    best = original
    if path.is_file():
        candidate = path.read_bytes()
        ch = list(t.GROUP.unpack_from(candidate))
        shape = header.copy();shape[0], shape[4], shape[7] = ch[0], ch[4], ch[7]
        t.require(ch == shape and ch[0] in (b'TGR1', b'TGI2') and
                  ch[7] == (1 if ch[0] == b'TGI2' else 0) and ch[4] == len(candidate)-32,
                  'Cached index header changed its bounds or identity')
        decoded = t.decode_lzma2(candidate[32:], header[3], dictionary)
        if ch[0] == b'TGI2':
            decoded = index_transform(decoded, header[6], 3, True, tile_count)
        t.require(decoded == raw, 'Cached index stream changed original bytes')
        if len(candidate) < len(best):
            best = candidate
    else:
        for lc, lp, pb in profiles:
            encoded = lzma.compress(transformed, format=lzma.FORMAT_RAW, filters=[dict(
                id=lzma.FILTER_LZMA2, preset=9, dict_size=dictionary,
                nice_len=273, depth=512, lc=lc, lp=lp, pb=pb)])
            if len(encoded)+32 >= len(best):
                continue
            t.require(t.decode_lzma2(encoded, len(transformed), dictionary) == transformed,
                      'Encoded TGI2 bytes differ')
            ch = header.copy();ch[0], ch[4], ch[7] = b'TGI2', len(encoded), 1
            best = t.GROUP.pack(*ch)+encoded
        t.atomic_bytes(path, best)
    physical = best+bytes((len(original)-len(best)) & 1)
    if len(physical) >= len(original) or (len(original)-len(physical))*100 < len(original)*minimum_percent:
        best = physical = original
    return best, physical, dict(group=header[2], records=header[6], raw_bytes=header[3],
        source_bytes=len(original), candidate_bytes=len(best), physical_bytes=len(physical),
        saved_bytes=len(original)-len(physical), source_profile=source_profile,
        candidate_profile=coalesce.source_lzma_profile(best[32:]),
        predicted=best[:4] == b'TGI2', source_sha256=t.sha256(original),
        candidate_sha256=t.sha256(best), raw_sha256=t.sha256(raw))


def convert_bytes(data, cache, *, workers=2, groups=None, minimum_percent=0, align_suffix=True,
                  prepare_candidates=False, arm_gate=None, elf_hashes=None):
    t.require(bool(prepare_candidates) != (arm_gate is not None),
              'Explicit candidate preparation or a measured ARM gate is required')
    t.require(1 <= workers <= 2 and 0 <= minimum_percent <= 100,
              'Use at most two workers and a bounded minimum saving')
    source = coalesce.view(data)
    t.require(41 <= source.version <= 45, 'TGI2 requires an existing NSP2/41..45 pack')
    old = t.TileReader(source);cache = Path(cache);cache.mkdir(parents=True, exist_ok=True)
    measured=None
    if arm_gate is not None:
        t.require(groups is None and minimum_percent==0 and align_suffix,
                  'ARM application must reproduce the measured candidate policy')
        measured=validate_arm_gate(data,old,arm_gate,elf_hashes)
    wanted = (set(measured) if measured is not None else
              (set(range(old.tile_groups, len(old.groups))) if groups is None else set(groups)))
    t.require(all(old.tile_groups <= g < len(old.groups) for g in wanted),
              'Only existing global index-group IDs may be selected')
    eligible = sorted(g for g in wanted if not old.predictors[g])
    originals = {g:bytes(data[at-32:at+size]) for g,(at,size,*_) in enumerate(old.groups)}
    plan = {g:(raw,raw,raw) for g,raw in originals.items()}

    def one(group):
        payload, physical, receipt = choose_index(originals[group], old.dictionary,
            old.tile_count, cache, minimum_percent=minimum_percent)
        return group, payload, physical, receipt

    rows = []
    with ThreadPoolExecutor(max_workers=workers) as pool:
        # Tasks carry only IDs and compressed bytes. Each worker decodes one
        # bounded index group; no all-catalog raw job list is materialized.
        for group,payload,physical,row in pool.map(one, eligible):
            plan[group] = originals[group], payload, physical;rows.append(row)
            if len(rows) % 64 == 0 or len(rows) == len(eligible):
                print(json.dumps(dict(stage='select_tgi2', completed=len(rows), total=len(eligible),
                    saved_bytes=sum(r['saved_bytes'] for r in rows))), flush=True)
    if measured is not None:
        for row in rows:
            gate=measured[row['group']]
            t.require(row['source_sha256']==gate['original_group_sha256'] and
                      row['candidate_sha256']==gate['candidate_group_sha256'] and
                      row['raw_sha256']==gate['raw_sha256'] and
                      row['candidate_bytes']-32==gate['after']['stored_bytes'],
                      'Chosen stream differs from its measured ARM candidate')
            row['accepted']=gate['accepted']
        candidate_plan,candidate_changes,_=aligned_plan(plan,True)
        candidate=coalesce.rewrite(data,candidate_plan,version=source.version)[0] if candidate_changes else data
        t.require(t.sha256(candidate)==arm_gate['candidate_sha256'], 'Complete measured candidate pack differs')
        del candidate,candidate_plan
        for group,gate in measured.items():
            if not gate['accepted']:
                original=originals[group];plan[group]=original,original,original
    plan,changed,padding=aligned_plan(plan,align_suffix)
    output = coalesce.rewrite(data, plan, version=source.version)[0] if changed else data
    # No pixel stream may change while indexes are being selected.
    for group in range(old.tile_groups):
        t.require(plan[group][0] == plan[group][1] == plan[group][2], 'Pixel stream changed')
    verification = coalesce.verify(data, output, plan, np.arange(old.tile_count, dtype='<u4'),
                            None, None, pixel_groups=False)
    proof=dict(complete=True, candidate_only=measured is None, release_accepted=measured is not None,
        runtime_format_added=False, resident_bytes_added=0,
        original_stream_is_fallback=True, all_tile_streams_byte_identical=True,
        frames_and_group_working_sets_unchanged=True, index_groups_considered=len(eligible),
        index_groups_changed=len(changed), source_profiles_preserved_for_untouched_groups=True,
        suffix_alignment_padding=padding, minimum_saving_percent=minimum_percent,
        groups=rows, verification=verification)
    if measured is not None:
        proof['arm_policy']=dict(source_sha256=arm_gate['source_sha256'],
            measured_candidate_sha256=arm_gate['candidate_sha256'],
            old_elf_sha256=arm_gate['old_elf_sha256'],new_elf_sha256=arm_gate['new_elf_sha256'],
            receipt_canonical_sha256=t.sha256(json.dumps(arm_gate,sort_keys=True,separators=(',',':')).encode()),
            accepted_groups=sum(r['accepted'] for r in measured.values()),
            rejected_groups=sum(not r['accepted'] for r in measured.values()),
            maximum_instruction_regression_percent=0,
            before_instructions=arm_gate['before_instructions'],after_instructions=arm_gate['after_instructions'])
    return output,proof


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--workers', type=int, default=2)
    parser.add_argument('--groups', nargs='+', type=int)
    parser.add_argument('--minimum-saving-percent', type=float, default=0)
    policy=parser.add_mutually_exclusive_group(required=True)
    policy.add_argument('--prepare-candidates', action='store_true', help='Prepare unaccepted streams for ARM measurement')
    policy.add_argument('--arm-gate', type=Path, help='Apply only the complete, input-bound zero-regression ARM results')
    parser.add_argument('--baseline-elf', type=Path)
    parser.add_argument('--candidate-elf', type=Path)
    args = parser.parse_args()
    source=t.private_path(args.input, 'Private source');target=t.private_path(args.output, 'Private output')
    work=t.private_path(args.work, 'Private cache')
    t.require(source != target, 'Never overwrite the source pack')
    gate=None;elf_hashes=None
    if args.arm_gate:
        t.require(args.baseline_elf is not None and args.candidate_elf is not None,
                  'Both measured ARM ELF files are required')
        gate=json.loads(args.arm_gate.read_text())
        elf_hashes=(t.sha256(args.baseline_elf.read_bytes()),t.sha256(args.candidate_elf.read_bytes()))
    else:
        t.require(args.baseline_elf is None and args.candidate_elf is None, 'ELF inputs require an ARM gate')
    t.HOST_CACHE_GROUPS=32;started=time.monotonic()
    output, proof = convert_bytes(source.read_bytes(), work/'encoded', workers=args.workers,
        groups=args.groups, minimum_percent=args.minimum_saving_percent,
        prepare_candidates=args.prepare_candidates, arm_gate=gate, elf_hashes=elf_hashes)
    t.atomic_bytes(target, output)
    proof.update(input=str(source), output=str(target), elapsed_seconds=time.monotonic()-started,
                 cache_groups=32, workers=args.workers)
    t.atomic_bytes(target.with_suffix('.json'), json.dumps(proof, indent=2).encode()+b'\n')
    print(json.dumps({k:v for k,v in proof.items() if k != 'groups'}), flush=True)


if __name__ == '__main__':
    main()
