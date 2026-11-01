#!/usr/bin/env python3
"""Original Tezcatara fire/cinder source definitions and traversal order.

The source clock is independent of the 16.667s body animation. Geometry follows
its actual Spine parents; the renderer never changes the shared world sampler.
"""
from __future__ import annotations
from contextlib import contextmanager
from pathlib import Path
import json
import math
import numpy as np
from PIL import Image
import content_tiles as t
import world_scene_assets as world
import world_room_shaders as shaders
import ancient_lights_ui23 as lights

SCENE='scenes/events/background_scenes/tezcatara.tscn'
CINDER='images/vfx/fire/cinder_particle.png'
FIRE_SHADERS=tuple(p for p,v in shaders.SHADERS.items() if v[0]=='fire')


def plain(value):
    if isinstance(value, (str,int,float,bool)) or value is None:return value
    if hasattr(value,'args'):return [plain(v) for v in value.args]
    return [plain(v) for v in value]


class Effect:
    def __init__(self,record):self.record=record


class SourceRenderer(world._Renderer):
    def visit(self,node,parent_matrix,parent_size,parent_tint,parent_z,path,clips=(),bones=None):
        p=node.props
        if not world._property(p,'visible',True):
            return super().visit(node,parent_matrix,parent_size,parent_tint,parent_z,path,clips,bones)
        ref=world._property(p,'texture')
        texture=self.loader.path(ref) if ref else None
        shader,material,blend,_=self.material(node)
        kind=('light' if node.kind=='CPUParticles2D' and texture==lights.LIGHT else
              'cinder' if node.kind=='GPUParticles2D' and texture==CINDER else
              'fire' if node.kind=='Sprite2D' and shader in FIRE_SHADERS else None)
        if kind is None:return super().visit(node,parent_matrix,parent_size,parent_tint,parent_z,path,clips,bones)
        t.require(not clips and not node.children,'Effect has unsupported clipping/children: '+path)
        z=int(world._property(p,'z_index',0))+(parent_z if world._property(p,'z_as_relative',True) else 0)
        matrix=self.screen@parent_matrix@world._node_matrix(p)
        tint=parent_tint*world._vector(world._property(p,'modulate',(1,1,1,1)),4)*world._vector(world._property(p,'self_modulate',(1,1,1,1)),4)
        record=dict(kind=kind,path=path,z=z,texture=texture,matrix=matrix[:2].ravel().tolist(),
                    local_matrix=world._node_matrix(p)[:2].ravel().tolist(),tint=tint.tolist(),blend=blend)
        if kind=='light':record['light']=lights.node_record(self,node,path,parent_matrix,parent_tint,parent_z,clips)
        elif kind=='fire':
            for name,default in [('region_enabled',False),('hframes',1),('vframes',1),('flip_h',False),('flip_v',False)]:
                t.require(world._property(p,name,default)==default,'Unreviewed Tez fire UV mode: '+name)
            t.require(shader in shaders.SHADERS and t.sha256(world._property(self.loader.resource(shader)[1],'code').encode())==shaders.SHADERS[shader][1], 'Fire shader identity changed')
            image,rect=self.image_geometry(node,self.texture(ref),parent_size)
            record.update(shader=shader,rect=list(rect),size=list(image.size),
                          uniforms={k:plain(world._property(material,'shader_parameter/'+k,v)) for k,v in shaders.FIRE_DEFAULTS.items()},
                          textures={k:self.loader.path(world._property(material,'shader_parameter/'+k)) for k in shaders.FIRE_TEXTURES})
        else:
            t.require(blend==1 and world._property(p,'amount',8)==24 and world._property(p,'local_coords',False) and
                      world._property(p,'fixed_fps',30)==60 and world._property(p,'preprocess',0)==6 and
                      world._property(p,'emitting',True) and not world._property(p,'one_shot',False) and
                      world._property(p,'speed_scale',1)==1 and world._property(p,'explosiveness',0)==0 and
                      world._property(p,'randomness',0)==0 and not world._property(p,'trail_enabled',False),
                      'Cinder lifecycle or blend changed')
            attrs,process=self.loader.resource(world._property(p,'process_material'))
            t.require(attrs.get('type')=='ParticleProcessMaterial','Cinder material class changed')
            record['properties']={k:plain(world._property(p,k)) for k in p if k not in ('material','texture','process_material') and not k.startswith('metadata/')}
            record['process']={k:plain(world._property(process,k)) for k in process if not k.endswith('_curve')}
            record['curves']={}
            for name in ('scale_curve','alpha_curve','orbit_velocity_curve'):
                curve_ref=world._property(process,name)
                attrs,curve_texture=self.loader.resource(curve_ref)
                t.require(attrs.get('type')=='CurveTexture' and world._property(curve_texture,'width',256)==256,'Unexpected cinder CurveTexture')
                values=shaders.curve_channel(self.loader,world._property(curve_texture,'curve'),256)
                record['curves'][name]=np.asarray(values).ravel().tolist()
        self.commands.append((z,Effect(record),blend,()))

    def finish(self,plane='all'):
        commands=sorted(self.commands,key=lambda r:r[0])
        effect_positions=[i for i,r in enumerate(commands) if isinstance(r[1],Effect)]
        ordinary=[i for i,r in enumerate(commands) if r[1] is not None and not isinstance(r[1],Effect)]
        t.require(ordinary and effect_positions and max(ordinary)<min(effect_positions),'Tez effects interleave an unhandled body mesh')
        rows=[commands[i][1].record for i in effect_positions]
        counts={kind:sum(r['kind']==kind for r in rows) for kind in ('fire','cinder','light')}
        t.require(counts==dict(fire=237,cinder=74,light=90),'Tez effect registry changed')
        self.diagnostics['tez_effects']=rows
        self.commands=[r for r in commands if not isinstance(r[1],Effect)]
        return super().finish(plane)


@contextmanager
def source_renderer():
    original=world._Renderer
    t.require(original is SourceRenderer.__bases__[0], 'Concurrent Tez renderer specialization')
    world._Renderer=SourceRenderer
    try:yield
    finally:world._Renderer=original
