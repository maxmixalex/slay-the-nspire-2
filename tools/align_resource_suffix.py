#!/usr/bin/env python3
"""Preserve the 4 KiB read-page phase of an unchanged resource suffix.

Tile compression can move retained artwork and language data across read-page
or concern boundaries. Add at most 4095 bytes after the last encoded tile/index
group. All encoded groups and the complete immutable suffix remain byte exact;
this changes file placement, not codecs, image quality, caches or preloading.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import time

import coalesce_tile_resources as coalesce
import compact_tile_slots as compact
import content_tiles as t
import split_content as split
import storage_parts as storage

PAGE_BYTES=4096


def make_plan(data, reference):
    source, baseline=coalesce.view(data),coalesce.view(reference)
    current, old=t.TileReader(source),t.TileReader(baseline)
    t.require(41 <= source.version <= t.MAXIMUM_CONTENT_VERSION,'Unsupported suffix alignment version')
    t.require((current.tile_count,current.per_group,current.tile_groups,current.index_groups)==
              (old.tile_count,old.per_group,old.tile_groups,old.index_groups),
              'Suffix reference has different tile identities')
    last=max(range(len(current.groups)),key=lambda g:sum(current.groups[g][:2]))
    encoded_end=sum(current.groups[last][:2])
    reference_end=max(sum(row[:2]) for row in old.groups)
    suffix=reference[reference_end:]
    # Locate by the complete reference suffix, not just the last group's end:
    # an already aligned input may contain padding between those two points.
    # This also makes the operation idempotent without adding disk metadata.
    suffix_start=len(data)-len(suffix)
    t.require(suffix and suffix_start>=encoded_end and data[suffix_start:]==suffix,
              'Reference suffix differs; only unchanged suffixes may retain page phase')
    padding=(reference_end-suffix_start)%PAGE_BYTES
    plan={}
    for group,row in enumerate(current.groups):
        at,length=row[:2];payload=data[at-32:at+length]
        plan[group]=payload,payload,payload+(bytes(padding) if group==last else b'')
    return plan,dict(page_bytes=PAGE_BYTES,padding_bytes=padding,last_group=last,
        reference_suffix_offset=reference_end,input_suffix_offset=suffix_start,
        output_suffix_offset=suffix_start+padding,reference_suffix_phase=reference_end%PAGE_BYTES,
        input_suffix_phase=suffix_start%PAGE_BYTES,output_suffix_phase=(suffix_start+padding)%PAGE_BYTES,
        immutable_suffix_bytes=len(suffix),immutable_suffix_sha256=t.sha256(suffix),
        reference_sha256=t.sha256(reference),all_encoded_groups_unchanged=True,resident_bytes_added=0)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path,required=True)
    parser.add_argument('--reference',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--work',type=Path,required=True)
    parser.add_argument('--locales',nargs='+',choices=split.LANGUAGES)
    args=parser.parse_args()
    source=t.private_path(args.input,'Private input')
    reference=t.private_path(args.reference,'Private phase reference')
    target=t.private_path(args.output,'Private output');work=t.private_path(args.work,'Private work')
    t.require(source!=target and source not in target.parents and target not in source.parents,
              'Never overwrite the source resources')
    t.require(reference!=target and reference not in target.parents and target not in reference.parents,
              'Never overwrite the phase reference')
    target.mkdir(parents=True,exist_ok=True);work.mkdir(parents=True,exist_ok=True)
    def artwork(folder):
        index=(folder/'nSpire2.dat.tns').read_bytes()
        return index,storage.join_parts(index,{n:(folder/n).read_bytes() for n in storage.PART_FILES})
    _old_index,old_common=artwork(source);reference_index,reference_common=artwork(reference)
    inputs={c:source/f'nSpire2.{c}.dat.tns' for c in (args.locales or split.LANGUAGES)}
    t.require('eng' in inputs and all(p.is_file() for p in inputs.values()),'Requested locale is missing')
    english=split.reconstruct(old_common,inputs['eng'].read_bytes(),'eng')
    previous=split.reconstruct(reference_common,(reference/'nSpire2.eng.dat.tns').read_bytes(),'eng')
    started=time.monotonic();plan,report=make_plan(english,previous)
    report.update(schema=1,version=coalesce.view(english).version,locales={})
    common=None
    for code in ('eng',*(c for c in split.LANGUAGES if c!='eng' and c in inputs)):
        original=english if code=='eng' else split.reconstruct(old_common,inputs[code].read_bytes(),code)
        version=coalesce.view(original).version
        t.require(version==report['version'],'Locales use different logical versions')
        converted,_=coalesce.rewrite(original,plan,version=version)
        proof=coalesce.verify(original,converted,plan,None,None,None,
                              pixel_groups=False,encoded_groups_only=True)
        old_reader=t.TileReader(coalesce.view(original));new_reader=t.TileReader(coalesce.view(converted))
        end=max(sum(row[:2]) for row in old_reader.groups)
        new_end=max(sum(row[:2]) for row in new_reader.groups)+report['padding_bytes']
        t.require(original[end:]==converted[new_end:], 'Padding changed the complete suffix')
        t.require(len(converted)-len(original)==report['padding_bytes'],'Unexpected padding extent')
        bounds=split.layout(converted,code);payload=converted[bounds['prefix']:bounds['prefix']+bounds['artwork']]
        if common is None:
            common,index,parts,partition=compact.partition_like(reference_index,previous,converted)
            t.atomic_bytes(target/'nSpire2.dat.tns',index)
            for name,encoded in parts.items():t.atomic_bytes(target/name,encoded)
            report['artwork']=partition
        else:t.require(common[64:]==payload,'Locales lost exact shared artwork')
        language=split.compress_language_directory(split.language_file(converted,bounds,code,bytes(32)))
        language,details=storage.compress_blocks(language)
        t.require(split.reconstruct(common,language,code)==converted,'Aligned split reconstruction differs')
        t.atomic_bytes(target/f'nSpire2.{code}.dat.tns',language)
        if code in ('eng','zhs'):t.atomic_bytes(work/(code+'-aligned'+str(version)+'.nsp'),converted)
        report['locales'][code]=dict(proof=proof,storage=details,stored_bytes=len(language))
        print(json.dumps(dict(stage='locale_complete',locale=code,**proof,stored_bytes=len(language))),flush=True)
    report.update(complete=True,elapsed_seconds=time.monotonic()-started,
        installed_artwork_bytes=sum((target/n).stat().st_size for n in ('nSpire2.dat.tns',*storage.PART_FILES)),
        installed_locale_bytes=sum((target/f'nSpire2.{c}.dat.tns').stat().st_size for c in inputs))
    t.atomic_bytes(target/'suffix-phase-receipt.json',json.dumps(report,indent=2).encode()+b'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ('locales','artwork')}),flush=True)


if __name__=='__main__':main()
