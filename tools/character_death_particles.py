"""Offline source-driven player-death particle planes.

The owner's scene supplies every emitter, texture, count, event time, lifetime,
curve and emission-point image. A fixed presentation seed replaces Godot's
unrecorded random seed. Sampling uses a bounded 60-Hz CPU integration; the
small Regent turbulence field is an explicitly recorded approximation to the
GPU field. No particle count or animation frame is dropped. Additive emission
is retained as a separate plane, never flattened over a guessed background.
"""
from __future__ import annotations

import math
import numpy as np
from PIL import Image

from build_content import texture
from world_room_shaders import curve_channel, generated_texture
from world_scene_assets import Call, SceneLoader, _node_matrix, _property, _vector


def sequence(value):
    return value.args if isinstance(value, Call) else value


def curve(loader, ref, default=1.):
    if ref is None:
        return np.full((256, 3), default, dtype=float)
    attrs, props = loader.resource(ref)
    if attrs.get('type') == 'CurveTexture':
        one = curve_channel(loader, _property(props, 'curve'), 256)
        return np.repeat(one[:, None], 3, axis=1)
    if attrs.get('type') == 'CurveXYZTexture':
        return generated_texture(loader, ref)[0]
    raise ValueError('Unreviewed death-particle curve: '+str(attrs.get('type')))


def sample_curve(values, t):
    at = min(255., max(0., t*256))
    lo = int(at);hi = min(255, lo+1)
    return values[lo]*(1-at+lo)+values[hi]*(at-lo)


def follower(frame, kind, key):
    rows = frame['slots'] if kind == 'slot' else frame['bones']
    item = next(row for row in rows if row['slot' if kind == 'slot' else 'name'] == key)
    a, b, tx, c, d, ty = item['node_matrix']
    return np.array([[a, b, tx], [c, d, ty], [0., 0., 1.]])


class Emitter:
    def __init__(self, pack, loader, node, parent, events, index):
        self.node, self.parent = node, parent
        self.props = node.props
        _, self.material = loader.resource(_property(node.props, 'process_material'))
        self.matrix = _node_matrix(node.props)
        self.follow = None
        if parent.kind == 'SpineSlotNode': self.follow = ('slot', _property(parent.props, 'slot_name'))
        elif parent.kind == 'SpineBoneNode': self.follow = ('bone', _property(parent.props, 'bone_name'))
        self.amount = int(_property(node.props, 'amount', 8))
        self.lifetime = float(_property(node.props, 'lifetime', 1))
        self.speed = float(_property(node.props, 'speed_scale', 1))
        self.randomness = float(_property(node.props, 'randomness', 0))
        self.explosive = float(_property(node.props, 'explosiveness', 0))
        self.starts, self.stop = events
        self.rng = np.random.default_rng(0x41cef1ea+index*31337)
        self.image = np.asarray(texture(pack, loader.path(_property(node.props, 'texture'))).convert('RGBA'), dtype=float)/255
        self.scale_curve = curve(loader, _property(self.material, 'scale_curve'))
        self.orbit_curve = curve(loader, _property(self.material, 'orbit_velocity_curve'))
        self.linear_curve = curve(loader, _property(self.material, 'linear_accel_curve'))
        self.color_curve = None
        color_ref = _property(self.material, 'color_initial_ramp')
        if color_ref is not None:self.color_curve = generated_texture(loader, color_ref)[0]
        self.align = bool(_property(self.material, 'particle_flag_align_y', False))
        self.points = None
        if int(_property(self.material, 'emission_shape', 0)) == 4:
            _, tex = loader.resource(_property(self.material, 'emission_point_texture'))
            _, image = loader.resource(_property(tex, 'image'))
            data = _property(image, 'data')
            if data['format'] != 'RGFloat':raise ValueError('Unreviewed death emission-point format')
            count = int(_property(self.material, 'emission_point_count'))
            self.points = np.frombuffer(bytes(sequence(data['data'])), dtype='<f4', count=count*2).reshape(count, 2).copy()
        attrs, material = loader.resource(_property(node.props, 'material'))
        if attrs.get('type') != 'CanvasItemMaterial' or int(_property(material, 'blend_mode', 0)) != 1:
            raise ValueError('Death particles must retain the source ADD material')
        if _property(node.props, 'local_coords', False):raise ValueError('Unreviewed local death emitter')
        self.pending=[];self.live=[];self.emitted=0
        for start in self.starts:
            if self.explosive == 1:
                births=[start]*self.amount
            else:
                interval=self.lifetime/self.speed/self.amount
                count=max(0,math.ceil((self.stop-start)/interval))
                births=[start+i*interval+self.rng.random()*interval*self.randomness for i in range(count)]
            self.pending.extend(t for t in births if t < self.stop)
        self.pending.sort()

    def value(self, name, default=0.):
        low=float(_property(self.material,name+'_min',default));high=float(_property(self.material,name+'_max',default))
        return low+(high-low)*self.rng.random()

    def spawn(self, time, frame):
        transform=(follower(frame,*self.follow) if self.follow else np.eye(3)) @ self.matrix
        origin=np.zeros(2)
        if self.points is not None:
            origin=self.points[int(self.rng.integers(len(self.points)))].copy()
            origin*=np.asarray(_vector(_property(self.material,'emission_shape_scale',(1,1,1)),3))[:2]
        world=(transform@np.array([*origin,1.]))[:2]
        direction=np.asarray(_vector(_property(self.material,'direction',(1,0,0)),3))[:2]
        angle=math.atan2(direction[1],direction[0])+(self.rng.random()*2-1)*math.radians(float(_property(self.material,'spread',45)))
        velocity=transform[:2,:2]@np.array([math.cos(angle),math.sin(angle)])*self.value('initial_velocity')
        life=self.lifetime*(1-self.rng.random()*float(_property(self.material,'lifetime_randomness',0)))
        color=np.asarray(_vector(_property(self.material,'color',(1,1,1,1)),4))
        if self.color_curve is not None:color=color*self.color_curve[int(self.rng.integers(len(self.color_curve)))]
        self.live.append(dict(birth=time,age=0.,life=life,position=world.copy(),origin=world.copy(),velocity=velocity,
            scale=self.value('scale',1.),linear=self.value('linear_accel'),radial=self.value('radial_accel'),
            orbit=self.value('orbit_velocity'),damping=self.value('damping'),color=color,
            turbulence=self.value('turbulence_influence'),transform=transform[:2,:2]))
        self.emitted+=1

    def step(self, time, frame, dt):
        while self.pending and self.pending[0] <= time+1e-8:
            self.spawn(self.pending.pop(0),frame)
        alive=[]
        gravity=np.asarray(_vector(_property(self.material,'gravity',(0,98,0)),3))[:2]
        for p in self.live:
            dtp=min(dt,max(0,time-p['birth']))*self.speed
            p['age']+=dtp
            if p['age']>=p['life']:continue
            unit=p['age']/p['life'];v=p['velocity'];length=np.linalg.norm(v)
            if length>1e-9:v+=v/length*p['linear']*float(sample_curve(self.linear_curve,unit)[0])*dtp
            radial=p['position']-p['origin'];length=np.linalg.norm(radial)
            if length>1e-9:v+=radial/length*p['radial']*dtp
            v+=gravity*dtp
            length=np.linalg.norm(v)
            if length>1e-9:v*=max(0.,length-p['damping']*dtp)/length
            if _property(self.material,'turbulence_enabled',False) and p['turbulence']:
                # Fixed curl-like field. GPU noise itself is not claimed to
                # be reproduced; source strength/influence remain explicit.
                strength=float(_property(self.material,'turbulence_noise_strength',1.))
                x,y=p['position'];v+=np.array([math.sin(y*.008+time*1.625),-math.sin(x*.008+time)])*length*strength*p['turbulence']*dtp
            p['position']+=v*dtp
            if p['orbit']:
                angle=p['orbit']*float(sample_curve(self.orbit_curve,unit)[0])*dtp*math.tau
                sn,cs=math.sin(angle),math.cos(angle);r=p['position']-p['origin']
                p['position']=p['origin']+np.array([r[0]*cs-r[1]*sn,r[0]*sn+r[1]*cs])
            alive.append(p)
        self.live=alive

    def snapshot(self):
        result=[]
        for p in self.live:
            size=sample_curve(self.scale_curve,p['age']/p['life'])[:2]*p['scale']
            if min(size)<=0:continue
            angle=math.atan2(p['velocity'][1],p['velocity'][0])-math.pi/2 if self.align else 0.
            sn,cs=math.sin(angle),math.cos(angle)
            matrix=np.array([[cs*size[0],-sn*size[1]],[sn*size[0],cs*size[1]]])
            # Matrices and positions are Godot y-down. Rasterization below
            # retains that convention, while the skeleton uses Spine y-up.
            result.append((p['position'].copy(),matrix,p['color'].copy(),self.image))
        return result


def systems(pack, name, events):
    if name not in ('defect','regent'):return []
    times={}
    for event in events:times.setdefault(event['name'],[]).append(float(event['time']))
    loader=SceneLoader(pack);root=loader.instantiate('scenes/creature_visuals/'+name+'.tscn')
    nodes={}
    def walk(node,path='',parent=None):
        if parent is not None:nodes[path]=(node,parent)
        for child in node.children:walk(child,(path+'/' if path else '')+child.name,node)
    walk(root)
    if name=='defect':
        definitions=[('Visuals/SparkSlot/SparkLeftParticles',times['fire_sparks_left'],100.),
                     ('Visuals/SparkSlot/SparkRightParticles',times['fire_sparks_right'],100.)]
    else:
        stop=times['death_particles_end'][0]
        definitions=[('Visuals/'+bone+'/'+part,times[event],stop) for bone,part,event in (
            ('SpineArmBone','Particles','death_particles_start'),('SpineLegBone','Particles','death_particles_start'),
            ('SpineLegBoneL','Particles','death_particles_start'),('SpineChestBone','Particles','death_particles_start2'),
            ('SpineChestBone','ParticlesBack','death_particles_start2'))]
        definitions.append(('Visuals/Explosion',times['explode_dead'],times['explode_end'][0]))
    return [Emitter(pack,loader,*nodes[path],(starts,stop),i) for i,(path,starts,stop) in enumerate(definitions)]


def sample_particles(pack, name, events, frames):
    emitters=systems(pack,name,events);samples=[]
    if not emitters:return [],{}
    previous=0.
    for i,frame in enumerate(frames):
        time=float(frame['time']);dt=time-previous
        for emitter in emitters:emitter.step(time,frame,dt)
        if i%2==0 or i==len(frames)-1:samples.append([particle for emitter in emitters for particle in emitter.snapshot()])
        previous=time
    report=dict(seed='fixed presentation seed, separate from gameplay',physics_hz=60,
        systems=[dict(node=e.node.name,parent=e.parent.name,amount=e.amount,lifetime=e.lifetime,
            speed_scale=e.speed,emitted=e.emitted,remaining=len(e.live)) for e in emitters],
        source_events=events,turbulence='bounded CPU curl approximation of GPU field' if name=='regent' else 'none')
    return samples,report


def particle_bounds(samples):
    points=[]
    for sample in samples:
        for center,matrix,_color,image in sample:
            half=np.array([image.shape[1],image.shape[0]])/2
            corners=np.array([[-1,-1],[1,-1],[1,1],[-1,1]])*half
            corners=corners@matrix.T+center
            corners[:,1]*=-1 # same y-up layout convention as skeleton bounds
            points.extend(corners)
    return np.asarray(points) if points else np.array([[0.,0.],[1.,1.]])


def render_particles(sample,scale,origin,size):
    width,height=size;out=np.zeros((height,width,3),dtype=float)
    for center,matrix,color,texture in sample:
        matrix=matrix*scale;center=center*scale+origin
        half=np.array([texture.shape[1],texture.shape[0]])/2
        corners=(np.array([[-1,-1],[1,-1],[1,1],[-1,1]])*half)@matrix.T+center
        low=np.maximum([0,0],np.floor(corners.min(axis=0)).astype(int));high=np.minimum([width,height],np.ceil(corners.max(axis=0)).astype(int))
        if np.any(high<=low) or abs(np.linalg.det(matrix))<1e-9:continue
        xx,yy=np.meshgrid(np.arange(low[0],high[0])+.5,np.arange(low[1],high[1])+.5)
        coords=np.stack((xx-center[0],yy-center[1]),axis=-1)@np.linalg.inv(matrix).T+half-.5
        inside=(coords[:,:,0]>=-.5)&(coords[:,:,0]<texture.shape[1]-.5)&(coords[:,:,1]>=-.5)&(coords[:,:,1]<texture.shape[0]-.5)
        sx=np.clip(coords[:,:,0],0,texture.shape[1]-1);sy=np.clip(coords[:,:,1],0,texture.shape[0]-1)
        x0=sx.astype(int);y0=sy.astype(int);x1=np.minimum(texture.shape[1]-1,x0+1);y1=np.minimum(texture.shape[0]-1,y0+1)
        fx=(sx-x0)[:,:,None];fy=(sy-y0)[:,:,None]
        pixel=((texture[y0,x0]*(1-fx)+texture[y0,x1]*fx)*(1-fy)+(texture[y1,x0]*(1-fx)+texture[y1,x1]*fx)*fy)*color
        out[low[1]:high[1],low[0]:high[0]]+=pixel[:,:,:3]*pixel[:,:,3:4]*inside[:,:,None]
    rgb=np.uint8(np.clip(out*255,0,255));alpha=np.where(np.any(rgb,axis=2),255,0).astype(np.uint8)
    return Image.fromarray(np.dstack((rgb,alpha)),'RGBA')
