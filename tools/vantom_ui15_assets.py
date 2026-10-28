"""Factor Vantom's driven skeleton into continuous LCD transform layers.

Only the owner's pinned PCK and reviewed Spine runtime supply the geometry.
Every authored sample, conditional track and draw-order position is retained.
Five independently sampled bone scales prove each visible mesh is either a
root-scale child, a rigid translated tail, or independent of ScalingBone.
This avoids a Cartesian product of animation frames and shrink phases.

The small per-clip parameter tables use losslessly encoded RGB565 payloads as
bounded word arrays. They are never painted. Layer images reuse the old whole
frame image slots, after checking that no unrelated model references them.
Only excess unique layers grow the image directory. Layer pixels are shared
across tail phases; no resident animation table or pixel pool is required.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import time

import numpy as np
from PIL import Image

import content_tiles as tiles
from lz4_block import encode as lz4_encode
from patch_feedback_assets import write_image
from pck import Pack
from retile_appended_resources import view
from world_enemy_assets import SpineSampler, _tracks, bone_drives, FLIP_Y, _scene_matrix, _bone_matrix
from world_enemy_profile import plan as enemy_plan
from world_raster import raster

BASE_AUXILIARY, PARAMETER_COUNT, AUXILIARY_COUNT = 5288, 116, 5404
HEADER_WORDS, FRAME_WORDS, LAYER_WORDS, EMITTER_WORDS, MAX_LAYERS = 12, 5, 6, 14, 64
MAGIC, VERSION = 0x5654, 2
DISSOLVE_SLOTS = frozenset(('tail', 'tail_hook', 'megablade'))
EMITTER_NODES = ('SprayBoneNode', 'DeathSpraySlotNode', 'DeathSprayBackSlotNode', 'DeathExplosionSlotNode')
SCALES = (1., .875, .75, .625, .5)
# Float32 Spine transforms accumulate at a roughly4000-unit source extent.
# This is under0.002 LCD pixels even at the maximum retained sampling density.
GEOMETRY_TOLERANCE = .03
LCD_GEOMETRY_TOLERANCE = .20
LCD_DENSITY = 4/21  # Original1680-wide4:3 viewport projected to320LCD pixels.
MESH_MAGIC, MESH_HEADER_WORDS = 0x564D, 16
MAX_MESH_VERTICES, MAX_MESH_TRIANGLES = 114, 112
SOURCE_FILES = ('tools/vantom_ui15_assets.py', 'tools/world_enemy_assets.py',
                'tools/world_enemy_profile.py', 'tools/SpineSampleAdapter.cs',
                'tools/world_raster.py', 'tools/vantom_vfx_ui15_assets.py',
                'tools/vantom_ui15_texture_pages.py', 'config/enemy-visuals-0.17.json')
ROOT = Path(__file__).resolve().parents[1]


def catalog():
    plan = enemy_plan()
    model = next(row for row in plan['models'] if row['model'] == 'Vantom')
    jobs = [row for row in plan['jobs'] if row['model'] == 'Vantom']
    full = [row for row in jobs if not row['visual_state']['shrink']]
    tiles.require(len(jobs) == 232 and len(full) == PARAMETER_COUNT,
                  'Pinned Vantom clip catalog changed')
    return model, full, jobs


def reusable_image_slots(source, jobs):
    """Prove that replacing Vantom's frame directory cannot change another model."""
    clips = {job['id'] for job in jobs}
    slots = {image for clip_id in clips
             for image in range(source.clips[clip_id][0],
                                source.clips[clip_id][0]+source.clips[clip_id][1])}
    tiles.require(bool(slots), 'Vantom has no retained frame slots to replace')
    for clip_id, clip in enumerate(source.clips):
        if clip_id not in clips:
            tiles.require(not slots.intersection(range(clip[0], clip[0]+clip[1])),
                          'Vantom frame slot is also referenced by clip '+str(clip_id))
    tiles.require(not slots.intersection(card[0] for card in source.cards),
                  'Vantom frame slot is also a card portrait')
    tiles.require(not slots.intersection(source.aux),
                  'Vantom frame slot is also an auxiliary image')
    return sorted(slots)


def layer_image_slot(reusable, first_parameter, ordinal):
    image = (reusable[ordinal] if ordinal < len(reusable) else
             first_parameter+PARAMETER_COUNT+ordinal-len(reusable))
    tiles.require(0 <= image < 65535, 'Vantom layer exceeds the image directory bound')
    return image


def primary_transform(model):
    primary, color = np.eye(3), np.ones(4)
    chain = model.get('transform_chain', [model['transform']])
    for transform in chain:
        primary = primary @ FLIP_Y @ _scene_matrix(transform) @ FLIP_Y
        color *= transform.get('modulate', [1, 1, 1, 1])
    color *= chain[-1].get('self_modulate', [1, 1, 1, 1])
    tiles.require(not any('skeleton' in row for row in model['layers']),
                  'Vantom child skeleton requires explicit draw-order factoring')
    return primary, color


def raw_samples(sampler, model, job, scales=SCALES):
    from build_world import job_state
    times = [i*job['duration']/job['frames'] for i in range(job['frames'])]
    context = job_state(job)
    setup, tracks, track = _tracks(model, job['clip'], context)
    if job['clip'] == 'die':
        events = next(row for row in model['clips'] if row['name'] == 'die')['events']
        dissolve = [row['time'] for row in events if row['name'] == 'dissolve_tail']
        tiles.require(len(dissolve) == 1, 'Vantom die must have its original dissolve event')
        # NVantomVfx.DissolveTail appends this callback after the one-second
        # shader tween. It changes track1 while track0 continues dying.
        tracks = [row for row in tracks if row['track'] != 1]
        tracks.extend([dict(track=1, clip='_tracks/charge_up_1', loop=False, queued=True, delay=dissolve[0]+1.),
                       dict(track=1, clip='_tracks/charged_1', loop=True, queued=True)])
    result = []
    for scale in scales:
        # Hold the actual ScalingBone driver at this independently requested
        # value. Its tween begins after this clip; no geometry is interpolated
        # before the reference runtime has sampled it.
        state = dict(context, shrink_from=scale, shrink_started=1000000.)
        controls = bone_drives(model, job['clip'], times, state)
        result.append(sampler.sample(model, job['clip'], times, skins=job['skins'],
            setup_tracks=setup, tracks=tracks, track=track, bone_drives=controls,
            loop=any(s['clip'] == job['clip'] and s['loop'] for s in model['states'])))
    return result


def classify_frame(frames, scales=SCALES, *, allow_mesh=False):
    """Prove rigid layers; explicitly retain actual non-rigid source meshes."""
    normal, half = frames[0]['draw'], frames[-1]['draw']
    tiles.require(all(len(f['draw']) == len(normal) for f in frames), 'ScalingBone changes visible mesh topology')
    kinds, tail, maximum, vertices = [], None, 0., 0
    fields = ('slot', 'index', 'attachment', 'region', 'page',
              'premultiplied_alpha', 'uvs', 'triangles', 'tint', 'dark', 'blend')
    for index, source in enumerate(normal):
        for frame in frames[1:]:
            tiles.require(all(source[key] == frame['draw'][index][key] for key in fields),
                          'ScalingBone changes topology or material: '+source['slot'])
        points = [np.asarray(frame['draw'][index]['vertices'], dtype=float).reshape(-1, 2) for frame in frames]
        tiles.require(all(p.shape == points[0].shape and len(p) for p in points), 'Empty or changing Vantom mesh')
        root_error = max(float(np.max(np.abs(p-points[0]*scale))) for p, scale in zip(points, scales))
        static_error = max(float(np.max(np.abs(p-points[0]))) for p in points)
        delta = np.mean(points[-1]-points[0], axis=0)/(1-scales[-1])
        rigid_error = max(float(np.max(np.abs(p-(points[0]+(1-scale)*delta)))) for p, scale in zip(points, scales))
        if root_error <= GEOMETRY_TOLERANCE:kind,error = 0,root_error
        elif static_error <= GEOMETRY_TOLERANCE:kind,error = 2,static_error
        elif rigid_error <= GEOMETRY_TOLERANCE and (tail is None or np.max(np.abs(delta-tail)) <= GEOMETRY_TOLERANCE):
            kind,error = 1,rigid_error;tail=delta
        else:
            tiles.require(allow_mesh, 'Non-rigid ScalingBone dependency: '+source['slot'])
            kind,error = 3,0.
        kinds.append(kind);maximum=max(maximum,error);vertices+=len(points[0])
    return kinds, np.zeros(2) if tail is None else tail, maximum, vertices


def projected_vertices(frame, index, primary, density):
    points = np.asarray(frame['draw'][index]['vertices'], dtype=float).reshape(-1, 2)
    points = (primary @ np.column_stack((points, np.ones(len(points)))).T).T[:, :2]*density
    points[:, 1] *= -1
    return points


def sampled_geometry(sampler, model, job, primary, density):
    """Refine only a numerical scale axis, never animation-frame bitmaps.

    Source poses retain the original physics solve. Independent midpoint poses
    verify the actual fixed-point interpolation before a segment is accepted.
    Fully rigid meshes keep the stricter source-coordinate proof above.
    """
    projection=max(1.,LCD_DENSITY/density)
    limit=LCD_GEOMETRY_TOLERANCE/projection
    source = dict(zip((1024, 896, 768, 640, 512), raw_samples(sampler, model, job)))
    classes = [classify_frame([source[q][i] for q in (1024,896,768,640,512)], allow_mesh=True)
               for i in range(job['frames'])]
    deforming = [(frame,index) for frame,(kinds,*_) in enumerate(classes) for index,kind in enumerate(kinds) if kind == 3]
    mesh_points, emitter_cache = {}, {}
    def emitters(q,frame):
        key=q,frame
        if key not in emitter_cache:emitter_cache[key]=emitter_samples(model,source[q][frame],primary,density)
        return emitter_cache[key]
    def points(q, frame, index):
        key=q,frame,index
        if key not in mesh_points:mesh_points[key]=projected_vertices(source[q][frame],index,primary,density)
        return mesh_points[key]
    intervals, maximum = [(512,768),(768,1024)], 0.
    accepted=[]
    while intervals:
        low, high = intervals.pop();middle=(low+high)//2
        # Q10 is the actual runtime domain. A one-unit interval has no
        # representable interior; both original endpoint poses are retained.
        if high-low==1:
            accepted.append((low,high));continue
        tiles.require(high-low>=2, 'Invalid Vantom scale interval')
        if middle not in source:
            source[middle]=raw_samples(sampler,model,job,(middle/1024,))[0]
            if len(source)%16==0:print(json.dumps(dict(event='vantom_scale_refinement',clip=job['id'],source_scales=len(source))),flush=True)
        error=0.
        for frame,(kinds,tail,previous_error,vertices) in enumerate(classes):
            for index,kind in enumerate(kinds):
                if kind==3:continue
                normal=np.asarray(source[1024][frame]['draw'][index]['vertices']).reshape(-1,2)
                actual=np.asarray(source[middle][frame]['draw'][index]['vertices']).reshape(-1,2)
                expected=normal*(middle/1024) if kind==0 else normal+(1-middle/1024)*(tail if kind==1 else 0)
                if np.max(np.abs(actual-expected))>GEOMETRY_TOLERANCE:
                    kinds[index]=3;deforming.append((frame,index))
        for frame,index in deforming:
            original=source[1024][frame]['draw'][index]
            changed=source[middle][frame]['draw'][index]
            for key in ('slot','attachment','uvs','triangles','tint','dark','blend'):
                tiles.require(changed[key]==original[key], 'Vantom scale changes mesh topology/material')
            left=np.rint(points(low,frame,index)*64)
            right=np.rint(points(high,frame,index)*64)
            interpolated=np.rint((left+right)/2)/64
            error=max(error,float(np.max(np.abs(interpolated-points(middle,frame,index)))))
        for frame in range(job['frames']):
            for lo,mid,hi in zip(emitters(low,frame),emitters(middle,frame),emitters(high,frame)):
                tiles.require(lo['depth']==mid['depth']==hi['depth'] and lo['flags']==mid['flags']==hi['flags'],
                              'Vantom scale changes particle draw order')
                actual=np.asarray(mid['matrix'],dtype=float)
                interpolation=np.rint((np.asarray(lo['matrix'],dtype=float)+np.asarray(hi['matrix'],dtype=float))/2)
                residual=np.abs(actual-interpolation)
                error=max(error,float(np.max(residual[4:])/4096+np.max(residual[:4])*4000/1048576))
        if error <= limit:
            accepted.append((low,high));maximum=max(maximum,error)
        else:
            tiles.require(len(source)<=513, 'Vantom nonlinear scale exceeds the Q10 source domain')
            intervals.extend(((low,middle),(middle,high)))
    # Every mesh may use a coarser subset than the worst nonlinear appendage.
    # This avoids repeating rigid-like whip vertices at all head-physics knots.
    keys=sorted(source)
    def knots(frame,index):
        def part(low,high):
            inside=[q for q in keys if low<q<high]
            if not inside:return [low,high]
            left=np.rint(points(low,frame,index)*64);right=np.rint(points(high,frame,index)*64)
            error=max(float(np.max(np.abs(np.rint((left*(high-q)+right*(q-low))/(high-low))/64-points(q,frame,index)))) for q in inside)
            if error<=limit:return [low,high]
            middle=(low+high)//2
            tiles.require(middle in source,'Vantom adaptive interval has no original midpoint')
            return part(low,middle)[:-1]+part(middle,high)
        return part(512,1024)
    selected={key:knots(*key) for key in deforming}
    final_error=0.
    for (frame,index),grid in selected.items():
        for low,high in zip(grid,grid[1:]):
            for q in keys:
                if low<q<high:
                    left=np.rint(points(low,frame,index)*64);right=np.rint(points(high,frame,index)*64)
                    error=float(np.max(np.abs(np.rint((left*(high-q)+right*(q-low))/(high-low))/64-points(q,frame,index))))
                    final_error=max(final_error,error)
    tiles.require(final_error<=limit, 'Vantom fixed-point interpolation failed independent source validation')
    emitter_keys=sorted(set(q for pair in accepted for q in pair))
    return source,classes,selected,emitter_keys,dict(maximum_lcd_interpolation_error=final_error*projection,
        maximum_lcd_emitter_error=maximum*projection,
        independently_sampled_scales=len(source),maximum_mesh_scale_keys=max(map(len,selected.values()),default=0),
        nonrigid_mesh_frames=len(selected))


def word_image(words, cache, key):
    tiles.require(len(words)<=65535 and all(-32768<=value<=65535 for value in words),'Vantom word image exceeds16-bit bounds')
    width,height=256,math.ceil(len(words)/256)
    raw=struct.pack('<'+'H'*(width*height),*(([value&65535 for value in words])+[0]*(width*height-len(words))))
    compressed=struct.pack('<BI',0,len(raw))+lz4_encode(raw)
    payload,fmt=(compressed,5) if len(compressed)<len(raw) else (raw,0)
    path=key+'.bin';tiles.atomic_bytes(cache/path,payload)
    return dict(file=path,width=width,height=height,format=fmt,decoded_format=0,bytes=len(payload),
                sha256=tiles.sha256(payload),raw_sha256=tiles.sha256(raw),raw_words=True,preserve_payload=True)


def mesh_asset(draw, positions, grid, folder, key, noise=None, projection=1.):
    """Keep the original UV topology and sample art above its LCD density."""
    vertices,uv,triangles,texture,tint,blend=draw
    tiles.require(len(uv)<=MAX_MESH_VERTICES and len(triangles)<=MAX_MESH_TRIANGLES,'Vantom mesh bound exceeded')
    texture_xy=uv*[texture.shape[1]-1,texture.shape[0]-1]
    low=np.floor(texture_xy.min(axis=0)).astype(int)-1
    high=np.ceil(texture_xy.max(axis=0)).astype(int)+1
    low=np.maximum(0,low);high=np.minimum([texture.shape[1]-1,texture.shape[0]-1],high)
    stretch=0.
    for triangle in triangles:
        mapped=texture_xy[triangle]
        basis=(mapped[1:]-mapped[0]).T
        if abs(np.linalg.det(basis))<1e-8:continue
        inverse=np.linalg.inv(basis)
        for points in positions:
            projected=(points[triangle[1:]]-points[triangle[0]]).T@inverse
            stretch=max(stretch,float(np.linalg.svd(projected,compute_uv=False)[0]))
    # Two texture samples per maximum original projected pixel retain the
    # source filtering without shipping a huge atlas or adding blurry icons.
    factor=min(1.,max(1/64,stretch*2*projection))
    extent=high-low
    width,height=np.maximum(2,np.ceil(extent*factor).astype(int)+1)
    tiles.require(width<=2048 and height<=2048,'Vantom mesh texture exceeds bounded atlas crop')
    xs=np.linspace(low[0],high[0],width);ys=np.linspace(low[1],high[1],height)
    ix,iy=np.floor(xs).astype(int),np.floor(ys).astype(int)
    jx=np.minimum(ix+1,texture.shape[1]-1);jy=np.minimum(iy+1,texture.shape[0]-1)
    fx,fy=(xs-ix)[None,:,None],(ys-iy)[:,None,None]
    tex=((texture[iy[:,None],ix[None,:]]*(1-fx)+texture[iy[:,None],jx[None,:]]*fx)*(1-fy)+
         (texture[jy[:,None],ix[None,:]]*(1-fx)+texture[jy[:,None],jx[None,:]]*fx)*fy)*tint
    picture=Image.fromarray(np.uint8(np.clip(tex*255,0,255)))
    filename=key+'.png';picture.save(folder/filename)
    mask_name=None
    if noise is not None:
        # Same atlas-coordinate UVs as the original slot material.
        nx=xs/(texture.shape[1]-1)*(noise.shape[1]-1);ny=ys/(texture.shape[0]-1)*(noise.shape[0]-1)
        ax,ay=np.floor(nx).astype(int),np.floor(ny).astype(int);bx=np.minimum(ax+1,noise.shape[1]-1);by=np.minimum(ay+1,noise.shape[0]-1)
        dx,dy=(nx-ax)[None,:],(ny-ay)[:,None]
        values=(noise[ay[:,None],ax[None,:]]*(1-dx)+noise[ay[:,None],bx[None,:]]*dx)*(1-dy)+(noise[by[:,None],ax[None,:]]*(1-dx)+noise[by[:,None],bx[None,:]]*dx)*dy
        mask=Image.new('RGBA',(int(width),int(height)),'white');mask.putalpha(Image.fromarray(np.uint8(values)))
        mask_name=key+'-mask.png';mask.save(folder/mask_name)
    mapped=(texture_xy-low)/np.maximum(1,extent)
    uv_words=np.rint(mapped*65535).astype(int).reshape(-1).tolist()
    xy=np.rint(np.asarray(positions)*64).astype(int)
    tiles.require(np.min(xy)>=-32768 and np.max(xy)<=32767,'Vantom mesh position exceeds Q6 bounds')
    words=[MESH_MAGIC,1,len(uv),len(triangles),len(grid),MESH_HEADER_WORDS,0,0,0,0,int(width),int(height),
           int(xy[:,:,0].min()),int(xy[:,:,1].min()),int(xy[:,:,0].max()),int(xy[:,:,1].max())]
    words.extend(grid);words[6]=len(words);words.extend(uv_words);words[7]=len(words)
    words.extend(triangles.reshape(-1).tolist());words[8]=len(words);words.extend(xy.reshape(-1).tolist());words[9]=len(words)
    tiles.require(len(words)<65535,'Vantom individual mesh exceeds its word chunk')
    return words,filename,mask_name


def emitter_samples(model, frame, primary, density):
    """Exact source follow-node transforms, including particle child offsets.

    Spine's Follow transform deliberately removes skeleton shear; node_matrix
    is exported by the pinned runtime before the drive pass. All four nodes
    are descendants of root_scale_adjust. Runtime applies that one proved
    scaling transform around the projected root, including particle offsets.
    """
    result = []
    depth = {name: i for i, name in enumerate(frame['draw_order'])}
    for name in EMITTER_NODES:
        host = next(row for row in model['layers'] if row['node'] == 'Visuals/'+name)
        child = next(row for row in model['layers'] if row['node'].startswith(host['node']+'/'))
        props = host['properties']
        if host['type'] == 'SpineBoneNode':
            anchor = next(row for row in frame['bones'] if row['name'] == props['bone_name'])
            at = 0 if props.get('show_behind_parent', False) else len(depth)
        else:
            anchor = next(row for row in frame['slots'] if row['slot'] == props['slot_name'])
            at = depth[props['slot_name']]+1
        matrix = FLIP_Y @ primary @ FLIP_Y @ _bone_matrix(anchor['node_matrix']) @ _scene_matrix(child['properties'])
        matrix[:2] *= density
        values = [round(matrix[0, 0]*1048576), round(matrix[0, 1]*1048576),
                  round(matrix[1, 0]*1048576), round(matrix[1, 1]*1048576),
                  round(matrix[0, 2]*4096), round(matrix[1, 2]*4096)]
        tiles.require(all(-2147483648 <= value <= 2147483647 for value in values),
                      'Vantom emitter matrix exceeds signed32 fixed point')
        result.append(dict(matrix=values, depth=at, flags=2|int(child['properties'].get('local_coords', False))))
    return result


def ordered_groups(draw, kinds, raw_frame, emitter_depths):
    """Retain this complete composite frame's actual authored draw order.

    Invisible source slots still establish particle insertion boundaries.
    Dissolve slots stay separate so their original UV noise can be sampled.
    No temporal independence between the two Spine tracks is assumed.
    """
    groups = []
    depths = {name: i for i, name in enumerate(raw_frame['draw_order'])}
    previous = -1
    for layer, kind, source in zip(draw, kinds, raw_frame['draw']):
        blend, depth = int(layer[-1]), depths[source['slot']]
        tiles.require(depth >= previous, 'Vantom meshes disagree with the original draw order')
        previous = depth
        masked = source['slot'] in DISSOLVE_SLOTS
        segment = sum(at <= depth for at in emitter_depths)
        key = (kind, blend, source['slot'] if masked or kind==3 else '', segment)
        tiles.require(blend in (0, 1), 'Vantom layer requires an unsupported compositing mode')
        if groups and not blend and not masked and kind!=3 and groups[-1][0] == key:
            groups[-1][1].append(layer)
        else:
            groups.append([key, [layer], depth, raw_frame['draw'].index(source)])
    tiles.require(len(groups) <= MAX_LAYERS, 'Vantom layer descriptor bound exceeded')
    return groups


def cropped_raster(draw, density, noise=None):
    vertices = np.concatenate([layer[0] for layer in draw])
    low, high = vertices.min(axis=0), vertices.max(axis=0)
    left, top = math.floor(low[0]*density)-2, math.floor(-high[1]*density)-2
    right, bottom = math.ceil(high[0]*density)+2, math.ceil(-low[1]*density)+2
    tiles.require(0 < right-left <= 320 and 0 < bottom-top <= 240,
                  'Vantom group exceeds its bounded clip canvas')
    # Additive groups contain one mesh; straight-alpha storage is applied
    # with the original blend at runtime, preserving overlap energy.
    material = [(vs, uv, tri, tex, tint, 0) for vs, uv, tri, tex, tint, blend in draw]
    picture = raster(material, density, [-left, -top], (right-left, bottom-top))
    crop = picture.getbbox()
    if crop is None:
        return None
    mask = None
    if noise is not None:
        texture = np.ones((*noise.shape, 4), dtype=float)
        texture[:, :, 3] = noise.astype(float)/255
        mask = raster([(vs, uv, tri, texture, np.ones(4), 0) for vs, uv, tri, *_ in draw],
                      density, [-left, -top], (right-left, bottom-top)).crop(crop)
        # Noise is stored in alpha8. RGB565 quantization must not reduce the
        # shader's smoothstep threshold to five or six bits of precision.
        alpha = mask.getchannel('A');mask = Image.new('RGBA', mask.size, 'white');mask.putalpha(alpha)
    return picture.crop(crop), left+crop[0], top+crop[1], mask


def source_identity(pack, runtime):
    digest = hashlib.sha256(Path(runtime).read_bytes())
    for name in SOURCE_FILES:
        digest.update((ROOT/name).read_bytes())
    for key, entry in sorted(pack.entries.items()):
        digest.update(key.encode());digest.update(entry.md5)
    return digest.hexdigest()


def sample_job(sampler, model, job, clip, paired, folder, identity, noise):
    signature = tiles.sha256((identity+json.dumps(job, sort_keys=True)+str(tuple(clip))+str(tuple(paired))).encode())
    saved = folder/'samples.json'
    if saved.is_file():
        data = json.loads(saved.read_text())
        if data.get('signature') == signature and all((folder/p).is_file() for p in data['pictures']) and all((folder/p['file']).is_file() for p in data['payloads']):
            return data
    folder.mkdir(parents=True, exist_ok=True)
    expected_ms = max(1, round(job['duration']*1000/job['frames']))
    tiles.require(clip[1] == paired[1] == job['frames'] and clip[2] == paired[2] == expected_ms,
                  'Vantom source timing differs from its retained endpoint directories')
    units = min(clip[5], paired[5])
    tiles.require(units > 0, 'Vantom requires its actual per-clip sampling density')
    density, (primary, color) = 1024/units, primary_transform(model)
    raw, classes, meshes, emitter_keys, mesh_proof = sampled_geometry(sampler, model, job, primary, density)
    records, pictures, payloads, dense, max_error, checked, max_layers, orders = [], [], [], [], 0., 0, 0, set()
    for frame_index in range(job['frames']):
        frames = [raw[q][frame_index] for q in (1024,896,768,640,512)]
        kinds, tail, error, vertices = classes[frame_index]
        tiles.require(all(frame['draw_order'] == frames[0]['draw_order'] for frame in frames),
                      'ScalingBone changes the original draw order')
        orders.add(tuple(frames[0]['draw_order']))
        max_error, checked = max(max_error, error), checked+vertices
        emitters = emitter_samples(model, frames[0], primary, density)
        draw = sampler.draw(model, frames[0], primary)
        draw = [(vs, uv, tri, tex, tint*color, blend) for vs, uv, tri, tex, tint, blend in draw]
        groups, layers = ordered_groups(draw, kinds, frames[0], [e['depth'] for e in emitters]), []
        chunks, pending_words = [], []
        for group_index, ((kind, blend, masked_slot, segment), group, depth, source_index) in enumerate(groups):
            if kind==3:
                grid=meshes[frame_index,source_index]
                points=[projected_vertices(raw[q][frame_index],source_index,primary,density) for q in grid]
                words,path,mask_name=mesh_asset(group[0],points,grid,folder,f'{frame_index:03d}-{group_index:02d}-mesh',
                                               noise if masked_slot in DISSOLVE_SLOTS else None,max(1.,LCD_DENSITY/density))
                if len(pending_words)+len(words)>60000:
                    chunks.append(pending_words);pending_words=[]
                geometry=f'{frame_index:03d}-geometry-{len(chunks):02d}.bin'
                layer=dict(file=path,geometry=geometry,offset=len(pending_words),mode=kind|(blend<<2),depth=depth)
                pending_words.extend(words);pictures.append(path);dense.append(path)
                if mask_name:layer['mask']=mask_name;pictures.append(mask_name);dense.append(mask_name)
                layers.append(layer);continue
            sampled = cropped_raster(group, density, noise if masked_slot else None)
            if sampled is None:
                continue
            picture, x, y, mask = sampled
            path = f'{frame_index:03d}-{group_index:02d}.png'
            picture.save(folder/path);pictures.append(path)
            layer = dict(file=path, x=x, y=y, mode=kind|(blend << 2), depth=depth)
            if mask is not None:
                layer['mask'] = f'{frame_index:03d}-{group_index:02d}-mask.png'
                mask.save(folder/layer['mask']);pictures.append(layer['mask'])
            layers.append(layer)
        if pending_words:chunks.append(pending_words)
        for index,words in enumerate(chunks):payloads.append(word_image(words,folder,f'{frame_index:03d}-geometry-{index:02d}'))
        max_layers = max(max_layers, len(layers))
        translated = primary[:2, :2] @ tail
        records.append(dict(layers=layers, tail_q8=[round(translated[0]*density*256),
                                                   round(-translated[1]*density*256)], emitter_keys=emitter_keys,
                            emitters=[emitter_samples(model,raw[q][frame_index],primary,density) for q in emitter_keys]))
    origin = primary @ [0, 0, 1]
    result = dict(signature=signature, clip=job['id'], key=job['key'],
        frames=job['frames'], frame_ms=clip[2], sprite_units=units,
        root_q8=[round(origin[0]*density*256), round(-origin[1]*density*256)],
        records=records, pictures=pictures, payloads=payloads, dense=dense, verified_source_vertices=checked,
        verified_scale_samples=len(SCALES), maximum_source_error=max_error,
        maximum_lcd_error=max_error*float(np.max(np.abs(primary[:2, :2])))*max(density,LCD_DENSITY),
        source_draw_orders=len(orders), maximum_layers=max_layers)
    result.update(mesh_proof)
    tiles.atomic_bytes(saved, json.dumps(result, separators=(',', ':')).encode()+b'\n')
    return result


def parameter_image(cache, key, sample, image_indices):
    coordinates = list(sample['root_q8'])
    for frame in sample['records']:
        coordinates.extend(frame['tail_q8'])
        for layer in frame['layers']:
            if 'geometry' not in layer:coordinates.extend((layer['x'], layer['y']))
    tiles.require(all(-32768 <= value <= 32767 for value in coordinates),
                  'Vantom signed coordinate exceeds its exact parameter range')
    words = [MAGIC, VERSION, sample['frames'], sample['frame_ms'],
             sample['sprite_units'] & 65535, sample['sprite_units'] >> 16,
             *sample['root_q8'], LAYER_WORDS, HEADER_WORDS, 0, 0]
    words.extend([0]*(sample['frames']*FRAME_WORDS))
    for frame_index, frame in enumerate(sample['records']):
        at = HEADER_WORDS+frame_index*FRAME_WORDS
        words[at:at+FRAME_WORDS] = [len(words), len(frame['layers']), *frame['tail_q8'], 0]
        for layer in frame['layers']:
            words.extend([image_indices[layer['file']],
                          image_indices[layer['geometry']] if 'geometry' in layer else layer['x'],
                          layer['offset'] if 'geometry' in layer else layer['y'], layer['mode'], layer['depth'],
                          image_indices[layer['mask']] if 'mask' in layer else 65535])
        words[at+4] = len(words)
        words.extend([len(frame['emitter_keys']),*frame['emitter_keys']])
        tiles.require(len(frame['emitters'])==len(frame['emitter_keys']), 'Vantom emitter scale table changed')
        for scaled in frame['emitters']:
            tiles.require(len(scaled)==4, 'Vantom frame must retain all four particle anchors')
            for emitter in scaled:
                for value in emitter['matrix']:
                    words.extend((value & 65535, (value >> 16) & 65535))
                words.extend((emitter['depth'], emitter['flags']))
    words[10] = len(words)
    tiles.require(len(words) <= 65535 and all(-32768 <= x <= 65535 for x in words),
                  'Vantom parameter word cannot be represented exactly')
    width, height = 256, math.ceil(len(words)/256)
    words.extend([0]*(width*height-len(words)))
    raw = struct.pack('<'+'H'*len(words), *(w & 65535 for w in words))
    compressed = struct.pack('<BI', 0, len(raw))+lz4_encode(raw)
    payload, fmt = (compressed, 5) if len(compressed) < len(raw) else (raw, 0)
    path = key+'.bin'
    tiles.atomic_bytes(cache/path, payload)
    return dict(file=path, width=width, height=height, format=fmt, decoded_format=0,
                bytes=len(payload), sha256=tiles.sha256(payload), raw_sha256=tiles.sha256(raw))


def source_assets(pck, runtime, logical_pack, cache, *, dotnet='dotnet', sample_work=None, fastnoise_header=None):
    """Return replacement rows, appended parameter/excess-layer rows and proof.

    Only the first116 rows are auxiliary resources. All remaining image rows
    in ``images`` are excess unique layers. ``replacement_images`` replaces
    all old Vantom frame slots, including empty aliases for unused slots.
    Parameters reference absolute image IDs, so no duplicate directory is
    needed to preserve a contiguous layer range. The thirteen earlier UI15
    resources each append one image and one auxiliary entry.
    """
    cache = tiles.private_path(cache, 'Private Vantom resource cache')
    cache.mkdir(parents=True, exist_ok=True)
    sample_work = tiles.private_path(sample_work or cache/'sampled', 'Private Vantom sample cache')
    source = view(Path(logical_pack).read_bytes())
    model, jobs, all_jobs = catalog()
    reusable = reusable_image_slots(source, all_jobs)
    preceding_images = BASE_AUXILIARY-len(source.aux)
    tiles.require(0 <= preceding_images <= 13,
                  'Append the thirteen preceding UI15 images before the Vantom parameter rows')
    first_parameter = len(source.images)+preceding_images
    from vantom_vfx_ui15_assets import tail_noise
    tiles.require(fastnoise_header is not None, 'Supply the pinned FastNoiseLite header for the original tail dissolve')
    noise = tail_noise(pck, fastnoise_header, cache)
    pack = Pack(pck)
    rows, samples, dedup, records = [], [], {}, []
    started = time.monotonic()
    try:
        identity = source_identity(pack, runtime)
        with SpineSampler(pack, runtime=runtime, dotnet=dotnet, work_dir=cache/'adapter') as sampler:
            metadata = sampler.metadata(model)
            tiles.atomic_bytes(cache/'metadata.json', json.dumps(metadata, separators=(',', ':')).encode()+b'\n')
            paired = {row['key']: row for row in all_jobs if row['visual_state']['shrink']}
            for index, job in enumerate(jobs):
                folder = sample_work/str(job['id'])
                partner = paired[job['key'].replace('Shrink=0', 'Shrink=1')]
                sample = sample_job(sampler, model, job, source.clips[job['id']], source.clips[partner['id']],
                                    folder, identity, noise)
                samples.append(sample)
                indices = {}
                for filename in sample['pictures']:
                    picture = Image.open(folder/filename).convert('RGBA')
                    key = hashlib.sha256(struct.pack('<HH', *picture.size)+picture.tobytes()).hexdigest()
                    if key not in dedup:
                        dedup[key] = layer_image_slot(reusable, first_parameter, len(rows))
                        row = write_image(cache, 'layer-'+key, picture, sparse=filename not in sample['dense'] and not filename.endswith('-mask.png'))
                        if filename in sample['dense']:row['preserve_payload']=True
                        row['image'] = dedup[key]
                        row['png'] = 'layer-'+key+'.png'
                        picture.save(cache/row['png'])
                        rows.append(row)
                    indices[filename] = dedup[key]
                for payload in sample['payloads']:
                    key=payload['raw_sha256']
                    if key not in dedup:
                        dedup[key]=layer_image_slot(reusable,first_parameter,len(rows))
                        filename='geometry-'+key+'.bin';tiles.atomic_bytes(cache/filename,(folder/payload['file']).read_bytes())
                        rows.append(dict(payload,file=filename,image=dedup[key]))
                    indices[payload['file']]=dedup[key]
                records.append(parameter_image(cache, f'parameters-{index:03d}', sample, indices))
                print(json.dumps(dict(event='vantom_clip', complete=index+1, total=len(jobs),
                    clip=job['id'], frames=sample['frames'], unique_images=len(rows),
                    maximum_source_error=sample['maximum_source_error'],
                    maximum_layers=sample['maximum_layers'])), flush=True)
    finally:
        pack.close()
    replacements = list(rows[:len(reusable)])
    if len(rows) < len(reusable):
        empty = Image.new('RGBA', (1, 1))
        empty_row = write_image(cache, 'retired-frame-empty', empty, sparse=True)
        empty_row['png'] = 'retired-frame-empty.png';empty.save(cache/empty_row['png'])
        replacements.extend(dict(empty_row, image=image) for image in reusable[len(rows):])
    result = dict(schema=2, reference='v0.111.0 / 41cef1ea',
        base_auxiliary=BASE_AUXILIARY, auxiliary_count=AUXILIARY_COUNT,
        images=records+rows[len(reusable):], replacement_images=replacements,
        auxiliary_rows=list(range(PARAMETER_COUNT)), clips=[],
        first_parameter_image=first_parameter, parameter_image_indices='absolute',
        factored_image_ids=reusable,
        reused_image_slots=min(len(rows), len(reusable)),
        empty_retired_slots=max(0, len(reusable)-len(rows)),
        appended_layer_images=max(0, len(rows)-len(reusable)),
        directory_added_images=PARAMETER_COUNT+max(0, len(rows)-len(reusable)),
        sampled_frames=sum(s['frames'] for s in samples), retained_clips=len(all_jobs),
        verified_source_vertices=sum(s['verified_source_vertices'] for s in samples),
        verified_scale_samples=len(SCALES),
        maximum_source_error=max(s['maximum_source_error'] for s in samples),
        complete_composite_tracks=True, original_draw_orders=sum(s['source_draw_orders'] for s in samples),
        dissolve_slots=sorted(DISSOLVE_SLOTS), emitter_count=4, parameter_version=VERSION,
        maximum_lcd_error=max(s['maximum_lcd_error'] for s in samples),
        maximum_lcd_interpolation_error=max(s['maximum_lcd_interpolation_error'] for s in samples),
        maximum_lcd_emitter_error=max(s['maximum_lcd_emitter_error'] for s in samples),
        independently_sampled_scales=max(s['independently_sampled_scales'] for s in samples),
        maximum_mesh_scale_keys=max(s['maximum_mesh_scale_keys'] for s in samples),
        nonrigid_mesh_frames=sum(s['nonrigid_mesh_frames'] for s in samples),
        maximum_layers=max(s['maximum_layers'] for s in samples),
        parameter_bytes=sum(r['bytes'] for r in records),
        image_bytes=sum(r['bytes'] for r in rows), unique_images=len(rows),
        source_frame_layer_count=sum(len(f['layers']) for s in samples for f in s['records']),
        elapsed_seconds=time.monotonic()-started,
        runtime_added_pixel_buffers=0, shrink_duration_ms=750,
        intermediate_scale_bitmaps=0)
    from vantom_ui15_texture_pages import bounded_catalog
    result=bounded_catalog(result,cache)
    tiles.atomic_bytes(cache/'vantom-assets.json', json.dumps(result, indent=2).encode()+b'\n')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pck', required=True, type=Path)
    parser.add_argument('--runtime', required=True, type=Path)
    parser.add_argument('--input', required=True, type=Path)
    parser.add_argument('--cache', required=True, type=Path)
    parser.add_argument('--sample-work', type=Path)
    parser.add_argument('--dotnet', default='dotnet')
    parser.add_argument('--fastnoise-header', type=Path, required=True)
    args = parser.parse_args()
    source_assets(args.pck, args.runtime, args.input, args.cache,
                  dotnet=args.dotnet, sample_work=args.sample_work, fastnoise_header=args.fastnoise_header)


if __name__ == '__main__':
    main()
