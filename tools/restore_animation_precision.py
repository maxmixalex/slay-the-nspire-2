#!/usr/bin/env python3
"""Recover original RGB565/alpha8 samples with explicit source equivalence.

An old full-precision resource is evidence, not permission to roll back newer
art. A frame is eligible only when its complete clip registration and image
shape still match. Every restored byte must reproduce the current frame under
one documented historical precision map. Later changed clips and unmatched
images are reported for source rebaking; they are never silently substituted.
The bounded sequential raw spool is also suitable for one final lossless tile
import, avoiding an intermediate decode/recompress cycle for every frame.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct
import time

import content_tiles as t
from quantize_animation_resources import quantize_image, PROFILES
from ui12_resources import view


def emit(event, **values):
    print(json.dumps(dict(event=event, **values)), flush=True)


def read_image(source, reader, image):
    return reader.image_raw(image) if source.images[image][3] == 6 else source.image_raw(image)


def matching_precision(original, current, preferred=1):
    """Compare full sparse geometry and every visible sample, never hashes alone."""
    if original[:3] != current[:3]:
        return None
    if original[3] == current[3]:
        return 8
    for profile in dict.fromkeys((preferred, 1, 2, 4)):
        if quantize_image(original[3], *original[:3], profile) == current[3]:
            return profile
    return None


def owners(source):
    result = [[] for _ in source.images]
    for clip, (first, count, *_rest) in enumerate(source.clips):
        for image in range(first, first + count):
            result[image].append(clip)
    return result


def changed_owners(before, after):
    return {clip for clip in range(max(len(before.clips), len(after.clips)))
            if clip >= len(before.clips) or clip >= len(after.clips) or
            before.clips[clip] != after.clips[clip]}


def write_spool(source, path):
    reader=t.TileReader(source);rows=[];digest=hashlib.sha256();offset=0
    with path.open('wb') as handle:
        for image in range(len(source.images)):
            width,height,fmt,raw=read_image(source,reader,image)
            handle.write(raw);digest.update(raw)
            rows.append(dict(image=image,width=width,height=height,format=fmt,
                offset=offset,bytes=len(raw),raw_sha256=t.sha256(raw)))
            offset+=len(raw)
            if (image+1)%2000==0:
                emit('precision_source_spool',complete=image+1,total=len(source.images),bytes=offset)
    return dict(file=str(path.resolve()),bytes=offset,sha256=digest.hexdigest(),rows=rows)


def read_spooled(handle,row):
    handle.seek(row['offset']);raw=handle.read(row['bytes'])
    t.require(len(raw)==row['bytes'] and t.sha256(raw)==row['raw_sha256'],
              'Original precision spool changed')
    return row['width'],row['height'],row['format'],raw


def audit(original,current,work,exclude_clips=(),exclude_images=()):
    source_path=work/'original-images.raw';catalog_path=work/'original-images.json'
    source_sha=t.sha256(original.data)
    catalog=json.loads(catalog_path.read_text()) if catalog_path.is_file() else None
    if not (catalog and catalog.get('source_sha256')==source_sha and
            source_path.is_file() and source_path.stat().st_size==catalog['bytes']):
        catalog=write_spool(original,source_path);catalog['source_sha256']=source_sha
        t.atomic_bytes(catalog_path,json.dumps(catalog,indent=2).encode()+b'\n')
    current_owners=owners(current);old_owners=owners(original)
    changed=changed_owners(original,current)|set(exclude_clips)
    explicit=set(exclude_images)
    planes=set(original.aux[1500:1505])
    eligible={i for i,row in enumerate(old_owners) if row}|planes
    exact=[];restore=[];unmatched=[];excluded=[];fingerprints=[];reader=t.TileReader(current)
    current_rows=[];current_digest=hashlib.sha256();current_offset=0
    with source_path.open('rb') as old, (work/'current-images.raw').open('wb') as new:
        for image in range(len(current.images)):
            actual=read_image(current,reader,image);width,height,fmt,raw=actual
            row=dict(image=image,width=width,height=height,format=fmt,offset=current_offset,
                     bytes=len(raw),raw_sha256=t.sha256(raw))
            current_rows.append(row);new.write(raw);current_digest.update(raw);current_offset+=len(raw)
            ownership=current_owners[image]
            reason=None;matched=False
            if image in explicit:reason='explicitly-rebuilt-source'
            elif any(clip in changed for clip in ownership):reason='changed-clip-registration'
            if reason:
                excluded.append(dict(image=image,reason=reason,clips=ownership))
            elif image < len(original.images):
                source=read_spooled(old,catalog['rows'][image])
                if source==actual:
                    exact.append(image);matched=True
                elif image in eligible:
                    preferred=4 if ownership and min(ownership)<38 else 2 if image in planes or \
                        any(1705<=clip<1710 for clip in ownership) else 1
                    profile=matching_precision(source,actual,preferred)
                    if profile is not None:
                        restore.append(dict(catalog['rows'][image],profile=PROFILES[profile],
                            current_sha256=row['raw_sha256'],clips=ownership,
                            complete_clip_registration_unchanged=True))
                        matched=True
                    else:
                        unmatched.append(dict(image=image,clips=ownership,reason='source-pixels-or-shape-changed'))
                else:
                    unmatched.append(dict(image=image,clips=ownership,reason='changed-static-source'))
            else:
                unmatched.append(dict(image=image,clips=ownership,reason='new-source-image'))
            # A fingerprint is triage, never proof of loss or of exactness.
            # Metadata/lookup pictures can coincidentally use these values.
            profiles=[]
            if not matched:
                for profile in (1,2,4):
                    if quantize_image(raw,width,height,fmt,profile)==raw:
                        profiles.append(PROFILES[profile])
                if profiles:fingerprints.append(dict(image=image,clips=ownership,profiles=profiles))
            if (image+1)%2000==0:
                emit('precision_equivalence',complete=image+1,total=len(current.images),
                    exact=len(exact),restore=len(restore),excluded=len(excluded),unmatched=len(unmatched))
    receipt=dict(schema=1,complete=True,original_sha256=source_sha,current_sha256=t.sha256(current.data),
        original_images=len(original.images),current_images=len(current.images),
        original_spool=dict(file=catalog['file'],bytes=catalog['bytes'],sha256=catalog['sha256']),
        current_spool=dict(file=str((work/'current-images.raw').resolve()),bytes=current_offset,
                           sha256=current_digest.hexdigest()),
        changed_clips=sorted(changed),exact_images=exact,restored_images=restore,
        excluded_images=excluded,unmatched_images=unmatched,quantized_fingerprint=fingerprints,
        restored_profile_counts=dict(Counter(row['profile'] for row in restore)),
        unresolved_is_not_accepted=True)
    t.atomic_bytes(work/'current-images.json',json.dumps(dict(source_sha256=t.sha256(current.data),
        rows=current_rows,**receipt['current_spool']),indent=2).encode()+b'\n')
    t.atomic_bytes(work/'precision-restoration.json',json.dumps(receipt,indent=2).encode()+b'\n')
    return receipt


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--original',type=Path,required=True);p.add_argument('--current',type=Path,required=True)
    p.add_argument('--work',type=Path,required=True)
    p.add_argument('--exclude-clip',type=int,action='append',default=[])
    p.add_argument('--exclude-image',type=int,action='append',default=[])
    args=p.parse_args()
    for path in (args.original,args.current,args.work):t.private_path(path,'Private exact source/restoration path')
    args.work.mkdir(parents=True,exist_ok=True);started=time.monotonic()
    result=audit(view(args.original.read_bytes()),view(args.current.read_bytes()),args.work,
                 args.exclude_clip,args.exclude_image)
    emit('precision_equivalence_complete',seconds=round(time.monotonic()-started,3),
        exact=len(result['exact_images']),restorable=len(result['restored_images']),
        excluded=len(result['excluded_images']),unmatched=len(result['unmatched_images']),
        profiles=result['restored_profile_counts'])


if __name__=='__main__':main()
