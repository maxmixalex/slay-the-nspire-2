"""Fixed, conservative closure for unreachable UI22 legacy image consumers.

No caller-supplied deletion manifest is accepted. Both the source routing and
the actual pack's direct/typed roots must agree with the reviewed plan.
"""
from __future__ import annotations
import hashlib
import json
from pathlib import Path
import struct

import content_tiles as t

ROOT=Path(__file__).resolve().parents[1]
PLAN=json.loads((ROOT/'config/retired-assets-ui22.json').read_text())
BASE_AUX,AUXILIARY=7211,7212
BASE_IMAGES,MARKER_IMAGE=58612,58612
MAGIC,REVISION=0x4c323252,1


def marker_bytes():
    return struct.pack('<4I',MAGIC,REVISION,PLAN['plan_tag'],PLAN['retired_images'])


def checked_plan():
    row={k:v for k,v in PLAN.items() if k not in ('plan_tag','plan_sha256')}
    digest=hashlib.sha256(json.dumps(row,sort_keys=True,separators=(',',':')).encode()).digest()
    t.require(digest.hex()==PLAN['plan_sha256'] and int.from_bytes(digest[:4],'little')==PLAN['plan_tag'],
              'Retirement plan identity changed')
    ids=[row['id'] for row in PLAN['retired_clips']]
    expected={8,9,*range(166,178),*range(322,346),*range(805,841)}
    t.require(set(ids)==expected and len(ids)==74,'Retirement clip whitelist changed')
    return PLAN


def verify_runtime_routes():
    checked_plan()
    for relative,expected in PLAN['runtime_route_sources'].items():
        t.require(hashlib.sha256((ROOT/relative).read_bytes()).hexdigest()==expected,
                  'Retirement routing must be reviewed again: '+relative)
    # Also reject a newly introduced literal consumer in another translation
    # unit. Dynamic world selection remains bounded by the pinned model graph.
    names=('body/idle_loop','reactions/attack_left','reactions/hurt_left',
           'reactions/attack_right','reactions/hurt_right')
    for path in (ROOT/'src').iterdir():
        if not path.is_file() or path.name in ('world_enemy_assets.inc','world_clip_index.inc'):
            continue
        text=path.read_text(errors='replace')
        t.require(not any('"'+name+'"' in text for name in names),
                  'A retired animation gained a literal caller: '+path.name)


def retired_images(source):
    checked_plan();ids=set()
    t.require(len(source.clips)==1783 and len(source.images) in (BASE_IMAGES,BASE_IMAGES+1),
              'Retirement requires the pinned complete image/clip catalog')
    for row in PLAN['retired_clips']:
        expected=tuple(row['tuple'])
        t.require(source.clips[row['id']]==expected,'Retired clip registration changed')
        ids.update(range(expected[0],expected[0]+expected[1]))
    t.require(len(ids)==PLAN['retired_images'],'Retired image identity count changed')
    return ids


def framed_references(raw):
    t.require(len(raw)>=20 and len(raw)%2==0,'Retirement framed table is truncated')
    words=struct.unpack('<'+'H'*(len(raw)//2),raw)
    frames,used=words[2],words[8]
    t.require(words[:2]==(0x4652,1) and words[9]==3 and 0<frames<=256 and
              10+frames*2<=used<=len(words),'Retirement framed table is invalid')
    ids=set()
    for frame in range(frames):
        at,count=words[10+frame*2:12+frame*2]
        t.require(0<count<=16 and at<=used and count*3<=used-at,'Retirement frame escapes its table')
        ids.update(words[at+part*3] for part in range(count))
    return ids


def vantom_references(raw):
    t.require(len(raw)>=24 and len(raw)%2==0,'Retirement Vantom table is truncated')
    words=struct.unpack('<'+'H'*(len(raw)//2),raw)
    frames,used=words[2],words[10]
    t.require(words[:2]==(0x5654,3) and words[8:10]==(7,12) and 0<frames<=256 and
              12+frames*5<=used<=len(words),'Retirement Vantom table is invalid')
    ids=set()
    for frame in range(frames):
        at,count=words[12+frame*5:14+frame*5]
        t.require(count<=64 and at<=used and count*7<=used-at,'Retirement Vantom layer escapes its table')
        for part in range(count):
            row=words[at+part*7:at+(part+1)*7];mode=row[3]
            t.require(mode<=7,'Retirement Vantom mode is invalid')
            ids.add(row[0])
            if mode&3==3:ids.add(row[1])
            ids.update(i for i in row[5:7] if i!=65535)
    return ids


def validate_roots(source,image):
    ids=retired_images(source)
    t.require(len(source.aux) in (BASE_AUX,AUXILIARY),'Retirement requires the complete selection profile')
    direct=set(range(66))|set(source.aux[:BASE_AUX])|{row[0] for row in source.cards}
    selected={row['id'] for row in PLAN['retired_clips']}
    for clip,row in enumerate(source.clips):
        if clip not in selected:
            direct.update(range(row[0],row[0]+row[1]))
    t.require(not ids&direct,'Retired image is still a direct clip/card/auxiliary root')
    typed=set()
    for auxiliary in [*range(5409,5419),*range(5422,7206)]:
        value=source.aux[auxiliary]
        if not value:continue
        _w,_h,fmt,raw=image(value)
        t.require(fmt==0,'Retirement framed parameters must remain exact words')
        typed.update(framed_references(raw))
    for auxiliary in range(5288,5404):
        value=source.aux[auxiliary]
        t.require(value>0,'Retirement requires the original Vantom layer tables')
        _w,_h,fmt,raw=image(value)
        t.require(fmt==0,'Retirement Vantom parameters must remain exact words')
        typed.update(vantom_references(raw))
    t.require(all(0<=i<len(source.images) for i in typed) and not ids&typed,
              'Retired image is still a typed actor/mesh/mask root')
    for row in PLAN['merchant_clips']:
        t.require(source.clips[row['id']]==tuple(row['tuple']),
                  'Retirement requires every current merchant moving layer')
        first,count=source.clips[row['id']][:2]
        t.require(all(entry[1:3]==tuple(row['size']) for entry in source.images[first:first+count]),
                  'Retirement requires current merchant moving-layer dimensions')
    for row in PLAN['merchant_auxiliary']:
        t.require(source.aux[row['id']]==row['image'] and
                  source.images[row['image']][1:3]==tuple(row['size']),
                  'Retirement requires the current merchant background and fixed layers')
    return ids,dict(retired_images=len(ids),direct_roots=len(direct),typed_roots=len(typed),
                    current_merchant_clips=10,current_merchant_planes=11)


def validate_retired_profile(source,image):
    t.require(len(source.aux)==AUXILIARY and len(source.images)==BASE_IMAGES+1 and
              source.aux[BASE_AUX]==MARKER_IMAGE,'UI22 retirement marker/catalog is missing')
    w,h,fmt,raw=image(MARKER_IMAGE)
    t.require((w,h,fmt,raw)==(8,1,0,marker_bytes()),'UI22 retirement plan marker differs')
    ids,report=validate_roots(source,image)
    for image_id in sorted(ids):
        w,h,fmt,raw=image(image_id)
        t.require(fmt==4 and raw==bytes(2*h) and source.images[image_id][1:3]==(w,h),
                  'Retired image must be a same-size empty sparse record')
    report.update(complete=True,plan_sha256=PLAN['plan_sha256'],marker_image=MARKER_IMAGE,
                  old_ui20_leaves_retained=True,unknown_static_images_retained=True)
    return report
