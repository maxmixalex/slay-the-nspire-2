#!/usr/bin/env python3
"""Restore measured merchant group bounds from the same generation's references.

The ordinary/focused top actor uses the original small RGR2 groups. Other
merchant bodies use the first retained merge's at-most-one-cache-page groups.
Every reference byte and slice is proved against the current complete group;
no image, frame, clock, sample, palette or alpha value changes. The existing
NSP44 runtime and pools are unchanged. This is not a cross-game-version import.
"""
from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
from pathlib import Path
import struct
import time

import compact_retained_colors as colors
from compact_tile_resources import SLICE, grouped_slices
from complete_retained_resources_ui17 import decoded_group
import content_tiles as t
from merge_retained_resources import partition_with_relocation
from retained_filter import group_layout
import split_content as split
import storage_parts as storage
from tile_group_policy import literal_profile

ROOT = Path(__file__).resolve().parents[1]
PAGE_BYTES = 128 * 1024


@dataclass
class Restore:
    group: int
    sha256: str
    slices: tuple
    parts: tuple
    # image ID, source raw offset/shape, destination part/raw offset
    images: tuple
    payload: bytes
    receipt: dict


def merchant_selection(source, policy='cache-page'):
    profile=literal_profile(ROOT/'tools/merchant_ui14_assets.py',{'FIRST_CLIP','CLIP_KEYS'})
    small={'merchant','merchant-focused'}
    medium={'bottom','ironclad','silent','defect','regent','necrobinder'}
    keys=profile['CLIP_KEYS'];t.require(small|medium <= set(keys),'Merchant semantic profile changed')
    rows=[]
    for offset,key in enumerate(keys):
        if key not in small|medium:continue
        clip=profile['FIRST_CLIP']+offset
        t.require(clip<len(source.clips),'Merchant clip is missing: '+key)
        rows.append(dict(key='merchant.'+key,clip=clip,
                         reference='small' if key in small or policy=='original' else 'medium'))
    return rows


def inventory(source):
    catalog=colors.groups(source);by_offset={g.offset:i for i,g in enumerate(catalog)}
    images={};members={i:[] for i in range(len(catalog))}
    for image,(off,_w,_h,fmt,_size) in enumerate(source.images):
        if fmt!=7:continue
        relative,size,raw_at,shape=SLICE.unpack_from(source.data,off)
        group=by_offset[off+relative]
        t.require(catalog[group].size==size,'Retained image group length differs')
        images[image]=(group,raw_at,shape);members[group].append(image)
    return catalog,images,members


def make_plan(source, references, selection):
    catalog,images,members=inventory(source)
    inventories={}
    for name,reference in references.items():
        t.require(len(source.images)==len(reference.images) and source.clips==reference.clips and
                  source.cards==reference.cards and source.aux==reference.aux,
                  'Reference is from a different content generation')
        inventories[name]=inventory(reference)
    chosen={};owners={}
    for row in selection:
        clip,reference=row['clip'],row['reference']
        t.require(isinstance(clip,int) and 0<=clip<len(source.clips) and reference in references,
                  'Invalid merchant reference selection')
        first,count,*_=source.clips[clip]
        matched=0
        for image in range(first,first+count):
            if image not in images:continue
            matched+=1
            group=images[image][0]
            # A shared boundary takes the tighter original reference.
            chosen[group]='small' if reference=='small' or chosen.get(group)=='small' else reference
            owners.setdefault(group,set()).add(row['key'])
        t.require(matched,'Merchant selection has no retained frames: '+row['key'])
    plan=[]
    for group,reference_name in sorted(chosen.items()):
        original=catalog[group];payload=bytes(source.block(original.offset,original.size))
        raw_size,*_=group_layout(payload,source.version)
        t.require(colors.complete_slices(original.slices,raw_size),'Merchant group has unowned raw bytes')
        reference=references[reference_name]
        ref_catalog,ref_images,ref_members=inventories[reference_name]
        t.require(all(image in ref_images for image in members[group]),'Reference changed an image codec')
        ids=sorted({ref_images[image][0] for image in members[group]})
        parts=[];shifts={};raw_parts=[];raw_at=0
        for part,ref_group in enumerate(ids):
            g=ref_catalog[ref_group];block=bytes(reference.block(g.offset,g.size))
            raw,_start,mode,_slices=group_layout(block,source.version)
            limit=64*1024 if reference_name=='small' else PAGE_BYTES
            t.require(raw<=limit and colors.complete_slices(g.slices,raw),
                      'Merchant reference exceeds its bounded complete group')
            t.require(all(image in images and images[image][0]==group for image in ref_members[ref_group]),
                      'Reference group crosses the current replacement boundary')
            # The small reference deliberately removes the new plane restore
            # cost as well as the larger group. Original RGR1 remains legal.
            t.require(reference_name!='small' or mode<8,'Small reference must use the pre-plane codec')
            shifts[ref_group]=(part,raw_at);parts.append(block)
            raw_parts.append(decoded_group(block,source.version));raw_at+=raw
        raw=decoded_group(payload,source.version)
        t.require(raw_size==raw_at and raw==b''.join(raw_parts),'Reference changes complete retained raw bytes')
        rows=[]
        for image in members[group]:
            _group,offset,shape=images[image]
            ref_group,ref_offset,ref_shape=ref_images[image];part,shift=shifts[ref_group]
            t.require(source.images[image][1:3]==reference.images[image][1:3] and
                      shape==ref_shape and offset==shift+ref_offset,
                      'Reference changes an image shape, alias or raw slice')
            rows.append((image,offset,shape,part,ref_offset))
        replacement=b''.join(block+bytes(len(block)&1) for block in parts)
        receipt=dict(group=group,selected_by=sorted(owners[group]),reference=reference_name,
            source_raw_bytes=raw_size,source_encoded_bytes=original.size,
            replacement_groups=len(parts),replacement_encoded_bytes=len(replacement),
            extra_encoded_bytes=len(replacement)-colors.aligned(original.size),
            raw_sha256=t.sha256(raw),source_stream_sha256=t.sha256(payload),
            replacement_stream_sha256=[t.sha256(block) for block in parts],
            max_replacement_raw_bytes=max(len(raw) for raw in raw_parts),
            covered_images=[row[0] for row in rows])
        plan.append(Restore(group,t.sha256(payload),original.slices,tuple(parts),tuple(rows),replacement,receipt))
    t.require(plan,'No retained merchant frames selected')
    # Preserve the established page offsets of every following font/string
    # and unrelated suffix block. Only the chosen merchant spans are changed.
    extra=sum(row.receipt['extra_encoded_bytes'] for row in plan)
    padding=(-extra)%4096
    plan[-1].payload+=bytes(padding)
    return plan,dict(selected_clips=selection,changed_groups=sum(len(p.parts)>1 or
        p.parts[0]!=bytes(source.block(catalog[p.group].offset,catalog[p.group].size)) for p in plan),
        raw_bytes_compared=sum(p.receipt['source_raw_bytes'] for p in plan),
        payload_extra_bytes=extra,alignment_padding_bytes=padding,
        results=[p.receipt for p in plan],runtime_bytes_added=0)


def rewrite(source, plan):
    catalog=colors.groups(source);replacements=[]
    for row in plan:
        t.require(row.group<len(catalog),'Merchant plan group is outside the catalog')
        group=catalog[row.group]
        t.require(t.sha256(source.block(group.offset,group.size))==row.sha256 and group.slices==row.slices,
                  'Locale group differs from the shared merchant plan')
        end=group.offset+colors.aligned(group.size)
        if group.size&1:t.require(end<=len(source.data) and source.data[end-1]==0,'Missing retained group alignment')
        replacements.append((group.offset,end,row.payload))
    replacements.sort();relocate=colors.Relocation(replacements)
    out=bytearray();previous=0
    for begin,end,payload in replacements:
        t.require(begin>=previous,'Merchant replacements overlap')
        out.extend(source.data[previous:begin]);out.extend(payload);previous=end
    out.extend(source.data[previous:])
    for image,(off,w,h,fmt,size) in enumerate(source.images):
        t.IMAGE.pack_into(out,40+image*16,relocate(off),w,h,fmt,size)
    for i,(cp,off,px,w,h,advance,encoded) in enumerate(source.glyphs):
        t.GLYPH.pack_into(out,source.glyph_at+i*16,cp,relocate(off),px,w,h,advance,encoded)
    for i,(off,size) in enumerate(source.strings):
        struct.pack_into('<II',out,source.string_at+i*8,relocate(off),size)
    old=t.TileReader(source);header=list(t.GLOBAL.unpack_from(source.data,source.base_end));table=header[9]
    for field in (8,9,14):header[field]=relocate(header[field])
    t.GLOBAL.pack_into(out,source.base_end,*header)
    for i in range(len(old.groups)):
        off,size=struct.unpack_from('<II',source.data,table+i*8)
        struct.pack_into('<II',out,header[9]+i*8,relocate(off),size)
    for i in range(old.frame_count):
        off=old.descriptors+i*16;global_at,local,raw_at,size=t.DESCRIPTOR.unpack_from(source.data,off)
        t.DESCRIPTOR.pack_into(out,relocate(off),relocate(global_at),local,raw_at,size)
    overrides={};selected_offsets=set()
    for row in plan:
        group=catalog[row.group];selected_offsets.add(group.offset)
        starts=[];at=relocate(group.offset)
        for block in row.parts:starts.append(at);at+=colors.aligned(len(block))
        for image,old_raw,shape,part,raw_at in row.images:
            off=source.images[image][0]
            rel,size,raw,old_shape=SLICE.unpack_from(source.data,off)
            t.require((off+rel,size,raw,old_shape)==(group.offset,group.size,old_raw,shape),
                      'Locale merchant image differs from the shared plan')
            result=(starts[part],len(row.parts[part]),raw_at,shape)
            t.require(off not in overrides or overrides[off]==result,'Merchant alias has two destinations')
            overrides[off]=result
    descriptors={}
    for off,(rel,size,raw_at,shape) in grouped_slices(source).items():
        t.require(off+rel not in selected_offsets or off in overrides,'Unplanned live merchant group slice')
        target,length,raw,shape=overrides[off] if off in overrides else (relocate(off+rel),size,raw_at,shape)
        noff=relocate(off);SLICE.pack_into(out,noff,target-noff,length,raw,shape)
        descriptors[off]=(target,length,raw,shape)
    struct.pack_into('<I',out,12,len(out));struct.pack_into('<I',out,36,0)
    return bytes(out),relocate,descriptors


def verify(source, output, plan, relocate, descriptors):
    target=colors.view(output);old,new=t.TileReader(source),t.TileReader(target)
    t.require(source.version==target.version and source.counts==target.counts and source.clips==target.clips and
              source.cards==target.cards and source.aux==target.aux,'Merchant catalog or clocks changed')
    t.require((old.tile_count,old.tile_groups,old.index_groups,old.per_group)==
              (new.tile_count,new.tile_groups,new.index_groups,new.per_group),'Tile slots or groups changed')
    for a,b in zip(old.groups,new.groups):
        off,n=a[:2];at,size=b[:2]
        t.require(a[1:]==b[1:] and source.data[off-32:off+n]==output[at-32:at+size],
                  'Merchant policy changed a compressed tile/index stream')
    old_groups=colors.groups(source);selected={row.group:row for row in plan}
    for group,g in enumerate(old_groups):
        at=relocate(g.offset)
        blocks=selected[group].parts if group in selected else (bytes(source.block(g.offset,g.size)),)
        for block in blocks:
            t.require(target.block(at,len(block))==block,'Restored or untouched retained stream changed')
            at+=colors.aligned(len(block))
    t.require(len(colors.groups(target))==len(old_groups)+sum(len(row.parts)-1 for row in plan),
              'Restored group count differs')
    for image,(a,b) in enumerate(zip(source.images,target.images)):
        off,w,h,fmt,size=a
        t.require(b==(relocate(off),w,h,fmt,size),'Merchant image format or dimensions changed')
        if fmt==7:
            rel,n,raw,shape=SLICE.unpack_from(output,b[0])
            t.require((b[0]+rel,n,raw,shape)==descriptors[off],'Merchant image addresses the wrong raw slice')
        elif fmt<6:t.require(source.block(off,size)==target.block(b[0],size),'Literal image changed')
        else:
            global_at,local,raw,n=t.DESCRIPTOR.unpack_from(source.data,off)
            t.require(t.DESCRIPTOR.unpack_from(output,b[0])==(relocate(global_at),local,raw,n),
                      'Tiled image descriptor changed')
    a=t.GLOBAL.unpack_from(source.data,source.base_end)[14];b=t.GLOBAL.unpack_from(output,target.base_end)[14]
    t.require(source.data[a:a+len(source.images)]==output[b:b+len(target.images)],'Image preparation costs changed')
    for i,(a,b) in enumerate(zip(source.glyphs,target.glyphs)):
        t.require(b==(a[0],relocate(a[1]),*a[2:]) and source.glyph_payload(i)==target.glyph_payload(i),'Glyph changed')
    for i,(a,b) in enumerate(zip(source.strings,target.strings)):
        t.require(b==(relocate(a[0]),a[1]) and source.string_payload(i)==target.string_payload(i),'String changed')
    return dict(images=len(source.images),glyphs=len(source.glyphs),strings=len(source.strings),clips=len(source.clips),
        input_bytes=len(source.data),output_bytes=len(output),extra_bytes=len(output)-len(source.data),
        exact_reference_raw_bytes=sum(row.receipt['source_raw_bytes'] for row in plan),
        exact_other_streams_aliases_and_catalog=True,output_sha256=t.sha256(output))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('input','output','work','small-reference','medium-reference'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--policy',choices=('cache-page','original'),default='cache-page')
    parser.add_argument('--locales',nargs='+',choices=split.LANGUAGES)
    args=parser.parse_args()
    source=t.private_path(args.input,'Private input');output=t.private_path(args.output,'Private output')
    work=t.private_path(args.work,'Private work')
    t.require(source!=output and source not in output.parents and output not in source.parents and work!=source,
              'Never overwrite the input resource set')
    locales=args.locales or list(split.LANGUAGES)
    t.require('eng' in locales and len(set(locales))==len(locales),'English and unique locales are required')
    output.mkdir(parents=True,exist_ok=True);work.mkdir(parents=True,exist_ok=True)
    index=(source/'nSpire2.dat.tns').read_bytes()
    common=storage.join_parts(index,{name:(source/name).read_bytes() for name in storage.PART_FILES})
    inputs={code:(source/f'nSpire2.{code}.dat.tns').read_bytes() for code in locales}
    english=split.reconstruct(common,inputs['eng'],'eng');initial=colors.view(english)
    references={key:colors.view(t.private_path(path,'Private matching reference').read_bytes())
                for key,path in (('small',args.small_reference),('medium',args.medium_reference))}
    started=time.monotonic();plan,report=make_plan(initial,references,merchant_selection(initial,args.policy))
    report.update(schema=1,policy=args.policy,version=initial.version,reference='v0.111.0 / 41cef1ea',locales={},
                  matching_reference_sha256={key:t.sha256(value.data) for key,value in references.items()})
    new_common=None
    for code in ('eng',*(c for c in locales if c!='eng')):
        original=initial if code=='eng' else colors.view(split.reconstruct(common,inputs[code],code))
        converted,relocate,descriptors=rewrite(original,plan)
        proof=verify(original,converted,plan,relocate,descriptors)
        bounds=split.layout(converted,code);art=converted[bounds['prefix']:bounds['prefix']+bounds['artwork']]
        if new_common is None:
            new_common,new_index,parts,details=partition_with_relocation(index,initial,converted,relocate)
            t.atomic_bytes(output/'nSpire2.dat.tns',new_index)
            for name,payload in parts.items():t.atomic_bytes(output/name,payload)
            report['artwork']=details;t.atomic_bytes(work/'eng-bounded44.nsp',converted)
        else:t.require(new_common[64:]==art,'Bounded merchant artwork differs between locales')
        language=split.compress_language_directory(split.language_file(converted,bounds,code,bytes(32)))
        language,compression=storage.compress_blocks(language)
        t.require(split.reconstruct(new_common,language,code)==converted,'Merchant locale roundtrip differs')
        t.atomic_bytes(output/f'nSpire2.{code}.dat.tns',language)
        report['locales'][code]=dict(proof=proof,stored_bytes=len(language),storage=compression)
        print(json.dumps(dict(stage='bounded_merchant_locale',locale=code,stored_bytes=len(language),**proof)),flush=True)
    names=('nSpire2.dat.tns',*storage.PART_FILES,*(f'nSpire2.{code}.dat.tns' for code in locales))
    before=sum((source/name).stat().st_size for name in names);after=sum((output/name).stat().st_size for name in names)
    report.update(complete=True,installed_bytes_before=before,installed_bytes_after=after,
                  installed_bytes_added=after-before,elapsed_seconds=time.monotonic()-started)
    t.atomic_bytes(output/'merchant-bounds-receipt.json',json.dumps(report,indent=2).encode()+b'\n')
    print(json.dumps({key:value for key,value in report.items() if key not in ('results','locales','artwork','selected_clips')}),flush=True)


if __name__=='__main__':main()
