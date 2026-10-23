"""Offline sampling of the Spine 4.2 tracks used by the supported game assets.

This is a bounded asset converter, not an on-device Spine runtime. It reads
bone/slot/deform/draw-order tracks and solves the selected IK/transform chains.
The calculator receives ordinary, pre-rendered RGB565 sprite frames.
"""
from __future__ import annotations
import copy
import math
from pathlib import Path
import json
import numpy as np
from PIL import Image
from setup_pose import Reader


def rgba(v):
    return np.array([(v >> s) & 255 for s in (24, 16, 8, 0)], dtype=float) / 255


def read_attachment(r, extra, key):
    f = r.byte(); kind = f & 7
    name = r.ref() if f & 8 else key
    o = dict(kind=kind, name=name, path=name, color=0xffffffff)
    if kind in (0, 2, 3):
        if f & 16: o['path'] = r.ref() or name
        if f & 32: o['color'] = r.uint()
        if f & 64: o['sequence'] = r.sequence()
    if kind == 0:
        o['rotation'] = r.fl() if f & 128 else 0
        o.update(zip(('x','y','sx','sy','w','h'), r.floats(6)))
    elif kind in (1, 4, 6):
        if kind == 6: o['end_slot'] = r.vi()
        o['vertices'] = r.verts(bool(f & (64 if kind == 4 else 16)))
        if kind == 4: r.floats(len(o['vertices']) // 3)
        if extra: r.uint()
    elif kind == 2:
        hull = r.vi(); vs = r.verts(bool(f & 128)); uv = r.floats(len(vs)*2)
        tris = [r.vi() for _ in range((len(vs)*2-hull-2)*3)]
        if extra:
            [r.vi() for _ in range(r.vi())]; r.floats(2)
        o.update(vertices=vs, uv=np.array(uv).reshape(-1,2), triangles=np.array(tris).reshape(-1,3))
    elif kind == 3:
        o.update(skin=r.vi(), parent=r.ref())
        if extra: r.floats(2)
    elif kind == 5:
        r.floats(3)
        if extra: r.uint()
    else: raise ValueError(f'Unsupported attachment type {kind}')
    return o


def read_skin(r, extra, default):
    if default: name='default'; count=r.vi()
    else:
        name=r.st()
        if extra:r.uint()
        for _ in range(5): [r.vi() for _ in range(r.vi())]
        count=r.vi()
    atts={}
    for _ in range(count):
        slot=r.vi()
        for _ in range(r.vi()):
            key=r.ref();atts[slot,key]=read_attachment(r,extra,key)
    return name,atts


def curve(r, channels):
    typ=r.byte()
    if typ==2:return [r.floats(4) for _ in range(channels)]
    if typ not in (0,1):raise ValueError(f'Invalid curve {typ} at {r.pos}')
    return typ


def numeric_track(r, count, channels, byte_values=False):
    frames=[]
    def values():
        return np.array([r.byte()/255 for _ in range(channels)] if byte_values else r.floats(channels))
    time=r.fl();val=values()
    for i in range(count):
        f=[time,val,0];frames.append(f)
        if i+1==count:break
        time=r.fl();val=values();f[2]=curve(r,channels)
    return frames


class Skeleton:
    def __init__(self,data):
        r=Reader(data);r.pos=8;self.version=r.st()
        if not self.version.startswith('4.2.'):raise ValueError('Expected Spine 4.2')
        self.bounds=r.floats(5);extra=r.byte()
        if extra:r.fl();r.st();r.st()
        r.strings=[r.st() for _ in range(r.vi())]
        self.bones=[]
        for i in range(r.vi()):
            name=r.st();parent=r.vi() if i else -1
            vals=r.floats(8);inherit=r.byte();skin=r.byte()
            if extra:r.uint();r.st();r.byte()
            self.bones.append(dict(name=name,parent=parent,values=np.array(vals[:7]),length=vals[7],inherit=inherit))
        self.slots=[]
        for _ in range(r.vi()):
            name=r.st();bone=r.vi();color=r.uint();dark=r.uint();att=r.ref();blend=r.vi()
            if extra:r.byte()
            self.slots.append(dict(name=name,bone=bone,color=rgba(color),attachment=att,blend=blend))
        self.iks=[]
        for _ in range(r.vi()):
            name=r.st();order=r.vi();bones=[r.vi() for _ in range(r.vi())];target=r.vi();f=r.byte()
            mix=(r.fl() if f&64 else 1) if f&32 else 0
            soft=r.fl() if f&128 else 0
            self.iks.append(dict(name=name,order=order,bones=bones,target=target,mix=mix,soft=soft,bend=1 if f&2 else -1,compress=bool(f&4),stretch=bool(f&8),uniform=bool(f&16)))
        self.transforms=[]
        for _ in range(r.vi()):
            name=r.st();order=r.vi();bones=[r.vi() for _ in range(r.vi())];target=r.vi();f=r.byte()
            offsets=np.array([r.fl() if f&bit else 0 for bit in (8,16,32,64,128)])
            local=bool(f&2);relative=bool(f&4);f=r.byte();shear=r.fl() if f&1 else 0
            mixes=np.array([r.fl() if f&bit else 0 for bit in (2,4,8,16,32,64)])
            self.transforms.append(dict(name=name,order=order,bones=bones,target=target,offset=np.append(offsets,shear),mix=mixes,local=local,relative=relative))
        if r.vi():raise ValueError('Path constraints are not supported by this asset profile')
        if r.vi():raise ValueError('Physics constraints are not supported by this asset profile')
        self.skins=[read_skin(r,extra,True)]
        for _ in range(r.vi()): self.skins.append(read_skin(r,extra,False))
        for _,atts in self.skins:
            for (slot,key),o in atts.items():
                if o['kind']==3:
                    parent=self.skins[o['skin']][1].get((slot,o['parent']))
                    if not parent or parent['kind']!=2:raise ValueError('Unsupported linked mesh')
                    for k in ('vertices','uv','triangles'):o[k]=parent[k]
                    o['kind']=2;o['deform_parent']=o['parent']
        self.events=[]
        for _ in range(r.vi()):
            name=r.st();r.vi();r.fl();r.st();audio=r.st()
            if audio:r.floats(2)
            self.events.append(bool(audio))
        self.animations={}
        for _ in range(r.vi()):
            name=r.st();self.animations[name]=self._animation(r)
        if r.pos!=len(data):raise ValueError(f'Animation reader left {len(data)-r.pos} bytes')
        self.children=[[] for _ in self.bones]
        for i,b in enumerate(self.bones):
            if b['parent']>=0:self.children[b['parent']].append(i)

    def _animation(self,r):
        r.vi();tracks=[]
        for _ in range(r.vi()):
            slot=r.vi()
            for _ in range(r.vi()):
                typ=r.byte();n=r.vi()
                if typ==0:
                    frames=[[r.fl(),r.ref(),1] for _ in range(n)]
                else:
                    r.vi();channels={1:4,2:3,3:7,4:6,5:1}[typ]
                    frames=numeric_track(r,n,channels,True)
                tracks.append(('slot',slot,typ,frames))
        for _ in range(r.vi()):
            bone=r.vi()
            for _ in range(r.vi()):
                typ=r.byte();n=r.vi()
                if typ==10:frames=[[r.fl(),r.byte(),1] for _ in range(n)]
                else:
                    r.vi();frames=numeric_track(r,n,2 if typ in (1,4,7) else 1)
                tracks.append(('bone',bone,typ,frames))
        for _ in range(r.vi()):
            index=r.vi();n=r.vi();r.vi();frames=[]
            def ik_frame():
                f=r.byte();t=r.fl();mix=(r.fl() if f&2 else 1) if f&1 else 0;soft=r.fl() if f&4 else 0
                return f,t,np.array([mix,soft]),(1 if f&8 else -1)
            flags,t,v,b=ik_frame()
            for i in range(n):
                frame=[t,v,0,b,bool(flags&16),bool(flags&32)];frames.append(frame)
                if i+1==n:break
                flags,t,v,b=ik_frame()
                frame[2]=1 if flags&64 else [r.floats(4),r.floats(4)] if flags&128 else 0
            tracks.append(('ik',index,0,frames))
        for _ in range(r.vi()):
            index=r.vi();n=r.vi();r.vi();tracks.append(('transform',index,0,numeric_track(r,n,6)))
        if r.vi():raise ValueError('Unexpected path animation')
        if r.vi():raise ValueError('Unexpected physics animation')
        for _ in range(r.vi()):
            skin=r.vi()
            for _ in range(r.vi()):
                slot=r.vi()
                for _ in range(r.vi()):
                    name=r.ref();typ=r.byte();n=r.vi();o=self.skins[skin][1][slot,name]
                    if typ==1:
                        frames=[[r.fl(),(r.uint(),r.fl()),1] for _ in range(n)]
                        tracks.append(('sequence',(skin,slot,name),0,frames));continue
                    if typ!=0:raise ValueError('Unexpected attachment animation')
                    size=sum(len(v) for v in o['vertices'])*2;r.vi();t=r.fl();frames=[]
                    for i in range(n):
                        vals=np.zeros(size);length=r.vi()
                        if length:
                            start=r.vi()
                            if start+length>size:raise ValueError('Deform outside mesh')
                            vals[start:start+length]=r.floats(length)
                        f=[t,vals,0];frames.append(f)
                        if i+1==n:break
                        t=r.fl();f[2]=curve(r,1)
                    tracks.append(('deform',(skin,slot,name),0,frames))
        order=[]
        for _ in range(r.vi()):
            t=r.fl();count=r.vi();original=0;unchanged=[];arr=[-1]*len(self.slots)
            for _ in range(count):
                slot=r.vi()
                while original!=slot:unchanged.append(original);original+=1
                offset=r.vi()
                if offset&0x80000000:offset-=0x100000000
                arr[original+offset]=original;original+=1
            unchanged.extend(range(original,len(arr)))
            for j in range(len(arr)-1,-1,-1):
                if arr[j]<0:arr[j]=unchanged.pop()
            order.append([t,arr,1])
        if order:tracks.append(('order',0,0,order))
        for _ in range(r.vi()):
            r.fl();idx=r.vi();r.vi();r.fl();r.st()
            if self.events[idx]:r.floats(2)
        return dict(tracks=tracks,duration=max((x[3][-1][0] for x in tracks),default=0))


def interpolate(frames,t,deform=False):
    if t<frames[0][0]:return None
    i=0
    while i+1<len(frames) and frames[i+1][0]<=t:i+=1
    a=frames[i]
    if i==len(frames)-1 or (isinstance(a[2],int) and a[2]==1):return a[1]
    if i==len(frames)-1:return a[1]
    b=frames[i+1];dt=b[0]-a[0]
    if dt<=0:return b[1]
    v=a[1];w=b[1];alpha=(t-a[0])/dt
    if isinstance(a[2],list):
        out=[]
        for j,c in enumerate(a[2]):
            x1,y1,x2,y2=c
            lo,hi=0.,1.
            for _ in range(16):
                u=(lo+hi)/2;nu=1-u;x=nu**3*a[0]+3*nu*nu*u*x1+3*nu*u*u*x2+u**3*b[0]
                if x<t:lo=u
                else:hi=u
            u=(lo+hi)/2;nu=1-u
            out.append(3*nu*nu*u*y1+3*nu*u*u*y2+u**3 if deform else nu**3*v[j]+3*nu*nu*u*y1+3*nu*u*u*y2+u**3*w[j])
        return v+(w-v)*out[0] if deform else np.array(out)
    return v+(w-v)*alpha


def matrix(vals):
    rot,x,y,sx,sy,shx,shy=vals
    a=math.radians(rot+shx);b=math.radians(rot+90+shy)
    return np.array([[math.cos(a)*sx,math.cos(b)*sy,x],[math.sin(a)*sx,math.sin(b)*sy,y],[0,0,1.]])


def rotation(angle):
    a=math.radians(angle);c=math.cos(a);s=math.sin(a);return np.array([[c,-s],[s,c]])


def normalize_angle(angle):return (angle+180)%360-180


def relative_world(bone,target,offset,mix):
    """Apply the Spine 4.2 relative-world constraint in our unit skeleton space.

    The target contributes an added transform, including world translation;
    its reflection changes the sign of angular offsets, not the translation.
    Reference: EsotericSoftware/spine-runtimes, 4.2,
    spine-ts/spine-core/src/TransformConstraint.ts, applyRelativeWorld.
    """
    handedness=1 if np.linalg.det(target[:2,:2])>0 else -1
    if mix[0]:
        angle=math.degrees(math.atan2(target[1,0],target[0,0]))+offset[0]*handedness
        bone[:2,:2]=rotation(normalize_angle(angle)*mix[0])@bone[:2,:2]
    destination=target@np.array([offset[1],offset[2],1])
    bone[:2,2]+=destination[:2]*mix[1:3]
    for axis in (0,1):
        bone[:2,axis]*=1+(np.linalg.norm(target[:2,axis])-1+offset[3+axis])*mix[3+axis]
    if mix[5]>0:
        tx=math.atan2(target[1,0],target[0,0]);ty=math.atan2(target[1,1],target[0,1])
        delta=normalize_angle(math.degrees(ty-tx))-90+offset[5]*handedness
        bone[:2,1]=rotation(delta*mix[5])@bone[:2,1]


def scaled_ik(values,inherit,parent,world,target,length,compress,stretch,uniform,mix):
    """One-bone IK with the authored local-space reach and scale switches.

    Reference: Spine 4.2 IkConstraint.apply1; global skeleton scale is one in
    this offline sampler. The returned values preserve both local shears.
    """
    out=values.copy();angle=-out[5]-out[0]
    if inherit==1:delta=target-world[:2,2]
    else:
        linear=parent[:2,:2].copy()
        if inherit==2:
            axis=linear[:,0].copy();amount=abs(np.linalg.det(linear))/max(0.0001,float(axis@axis))
            linear[:,1]=np.array([-axis[1],axis[0]])*amount
            angle+=math.degrees(math.atan2(axis[1],axis[0]))
        if abs(np.linalg.det(linear))<=0.0001:delta=np.zeros(2)
        else:delta=np.linalg.solve(linear,target-parent[:2,2])-out[1:3]
    angle+=math.degrees(math.atan2(delta[1],delta[0]))
    if out[3]<0:angle+=180
    out[0]+=normalize_angle(angle)*mix
    if inherit in (3,4):delta=target-world[:2,2]
    reach=length*out[3];distance=np.linalg.norm(delta)
    if reach>0.0001 and ((compress and distance<reach) or (stretch and distance>reach)):
        scale=1+(distance/reach-1)*mix
        out[3]*=scale
        if uniform:out[4]*=scale
    return out


def pose(sk,clip,t):
    values=[b['values'].copy() for b in sk.bones];inherits=[b['inherit'] for b in sk.bones]
    slots=copy.deepcopy(sk.slots);iks=copy.deepcopy(sk.iks);trans=copy.deepcopy(sk.transforms);deforms={};order=list(range(len(slots)))
    for group,index,typ,frames in sk.animations[clip]['tracks']:
        v=interpolate(frames,t,group=='deform')
        if v is None:continue
        if group=='bone':
            val=values[index];base=sk.bones[index]['values']
            if typ==10:inherits[index]=v;continue
            coords={0:[0],1:[1,2],2:[1],3:[2],4:[3,4],5:[3],6:[4],7:[5,6],8:[5],9:[6]}[typ]
            for j,c in enumerate(coords):val[c]=base[c]*v[j] if c in (3,4) else base[c]+v[j]
        elif group=='slot':
            if typ==0:slots[index]['attachment']=v
            elif typ==5:slots[index]['color'][3]=v[0]
            else:slots[index]['color'][:4 if typ in (1,3) else 3]=v[:4 if typ in (1,3) else 3]
        elif group=='ik':
            iks[index]['mix'],iks[index]['soft']=v
            a=[f for f in frames if f[0]<=t][-1];iks[index]['bend']=a[3]
            iks[index]['compress'],iks[index]['stretch']=a[4:6]
        elif group=='transform':trans[index]['mix']=v
        elif group=='deform':deforms[index]=v
        elif group=='order':order=v
        elif group=='sequence':raise ValueError('Sequence tracks require a dedicated profile')
    world=[np.eye(3) for _ in sk.bones]
    def update(i):
        local=matrix(values[i]);p=sk.bones[i]['parent'];inherit=inherits[i]
        if p<0:world[i]=local
        elif inherit==0:world[i]=world[p]@local
        elif inherit==2:
            # Spine 4.2 Bone.updateWorldTransformWith: NoRotationOrReflection.
            # Cancel parent rotation/reflection while retaining its stretch.
            par=world[p];linear=par[:2,:2];axis=linear[:,0];length2=float(axis@axis)
            if length2>0.0001:
                scale=abs(np.linalg.det(linear))/length2
                basis=np.column_stack((axis,np.array([-axis[1],axis[0]])*scale))
                angle=math.degrees(math.atan2(axis[1],axis[0]))
            else:
                basis=np.array([[0.,-linear[0,1]],[0.,linear[1,1]]])
                angle=90-math.degrees(math.atan2(linear[1,1],linear[0,1]))
            adjusted=values[i].copy();adjusted[0]-=angle
            world[i]=np.eye(3);world[i][:2,:2]=basis@matrix(adjusted)[:2,:2]
            world[i][:2,2]=(par@np.array([values[i][1],values[i][2],1]))[:2]
        elif inherit in (3,4):
            par=world[p];m=par[:2,:2];r=math.radians(values[i][0]);co,si=math.cos(r),math.sin(r)
            a=m@np.array([co,si]);a/=max(1e-8,np.linalg.norm(a));sign=-1 if np.linalg.det(m)<0 and inherit==3 else 1
            base=np.array([[a[0],-a[1]*sign],[a[1],a[0]*sign]])
            no_rot=values[i].copy();no_rot[0]=0;world[i]=np.eye(3);world[i][:2,:2]=base@matrix(no_rot)[:2,:2];world[i][:2,2]=(par@np.array([values[i][1],values[i][2],1]))[:2]
        elif inherit==1:
            world[i]=local;world[i][:2,2]=(world[p]@np.array([values[i][1],values[i][2],1]))[:2]
        else:raise ValueError(f'Unsupported bone inheritance {inherit}')
    def children(i):
        for c in sk.children[i]:update(c);children(c)
    for i in range(len(world)):update(i)
    def rotate_world(i,degrees):
        world[i][:2,:2]=rotation(degrees)@world[i][:2,:2];children(i)
    for _,kind,index in sorted([(a['order'],'ik',i) for i,a in enumerate(iks)]+[(a['order'],'transform',i) for i,a in enumerate(trans)]):
        if kind=='ik':
            a=iks[index];mix=a['mix']
            if mix<=0:continue
            target=world[a['target']][:2,2].copy();chain=a['bones']
            if len(chain)==1:
                i=chain[0]
                if a.get('compress') or a.get('stretch'):
                    p=sk.bones[i]['parent']
                    if p<0:raise ValueError('Scaled IK requires a parent bone')
                    values[i]=scaled_ik(values[i],inherits[i],world[p],world[i],target,sk.bones[i]['length'],a.get('compress',False),a.get('stretch',False),a.get('uniform',False),mix)
                    update(i);children(i)
                else:
                    d=target-world[i][:2,2];ang=math.degrees(math.atan2(d[1],d[0]));cur=math.degrees(math.atan2(world[i][1,0],world[i][0,0]));rotate_world(i,normalize_angle(ang-cur)*mix)
            elif len(chain)==2:
                p,c=chain;origin=world[p][:2,2].copy();middle=world[c][:2,2].copy();vec=target-origin
                l1=np.linalg.norm(middle-origin);l2=sk.bones[c]['length']*np.linalg.norm(world[c][:2,0]);dist=np.linalg.norm(vec)
                if l1<1e-5 or l2<1e-5 or dist<1e-5:continue
                bend=a['bend']*(-1 if np.linalg.det(world[p][:2,:2])<0 else 1)
                angle2=math.acos(np.clip((dist*dist-l1*l1-l2*l2)/(2*l1*l2),-1,1))*bend
                angle1=math.atan2(vec[1],vec[0])-math.atan2(l2*math.sin(angle2),l1+l2*math.cos(angle2))
                current=math.atan2(middle[1]-origin[1],middle[0]-origin[0]);rotate_world(p,normalize_angle(math.degrees(angle1-current))*mix)
                d=target-world[c][:2,2];current=math.degrees(math.atan2(world[c][1,0],world[c][0,0]));rotate_world(c,normalize_angle(math.degrees(math.atan2(d[1],d[0]))-current)*mix)
            else:raise ValueError('Unsupported IK chain')
        else:
            a=trans[index];target=world[a['target']];o=a['offset'];m=a['mix']
            if a['local']:
                if np.any(m):raise ValueError('Local transform needs a separate profile')
                continue
            for i in a['bones']:
                b=world[i]
                if a['relative']:
                    relative_world(b,target,o,m);children(i);continue
                if m[0]:
                    desired=math.degrees(math.atan2(target[1,0],target[0,0]))+o[0];cur=math.degrees(math.atan2(b[1,0],b[0,0]));b[:2,:2]=rotation(normalize_angle(desired-cur)*m[0])@b[:2,:2]
                dest=(target@np.array([o[1],o[2],1]))[:2];b[0,2]+=(dest[0]-b[0,2])*m[1];b[1,2]+=(dest[1]-b[1,2])*m[2]
                for axis in (0,1):
                    old=np.linalg.norm(b[:2,axis]);desired=np.linalg.norm(target[:2,axis])+o[3+axis]
                    if old>1e-8:b[:2,axis]*=1+(desired/old-1)*m[3+axis]
                if m[5]:
                    tx=math.atan2(target[1,0],target[0,0]);ty=math.atan2(target[1,1],target[0,1]);bx=math.atan2(b[1,0],b[0,0]);by=math.atan2(b[1,1],b[0,1]);d=normalize_angle(math.degrees((ty-tx)-(by-bx))+o[5]);b[:2,1]=rotation(d*m[5])@b[:2,1]
                children(i)
    return world,slots,deforms,order


def load_textures(pack,name,texture):
    raw=json.loads(pack.read(pack.imported(name+'.atlas','.spatlas')))['atlas_data']
    regions={};pages={};cur=None;page=None
    for line in raw.splitlines():
        line=line.strip()
        if not line:cur=None;continue
        if ':' not in line:
            if line.endswith('.png'):page=line;pages[page]=texture(pack,line);cur=None
            else:cur=dict(page=page);regions[line]=cur
        elif cur is not None:
            k,v=line.split(':',1);cur[k]=v
    out={}
    for key,r in regions.items():
        x,y,w,h=map(int,r['bounds'].split(','));rot=int(r.get('rotate','0'))
        im=pages[r['page']].crop((x,y,x+(h if rot in (90,270) else w),y+(w if rot in (90,270) else h)))
        if rot:im=im.rotate(rot,expand=True)
        ox,oy,ow,oh=map(int,r.get('offsets',f'0,0,{w},{h}').split(','));full=Image.new('RGBA',(ow,oh));full.paste(im,(ox,oh-oy-h));out[key]=np.asarray(full,dtype=float)/255
    return out


def cross2(a,b):return float(a[0]*b[1]-a[1]*b[0])


def polygon_triangles(polygon):
    """Triangulate a simple clipping contour, including concave contours."""
    points=[]
    for point in polygon:
        if not points or np.linalg.norm(point-points[-1])>1e-8:points.append(point)
    if len(points)>1 and np.linalg.norm(points[0]-points[-1])<=1e-8:points.pop()
    if len(points)<3:return []
    area=sum(cross2(a,b) for a,b in zip(points,points[1:]+points[:1]))
    if abs(area)<=1e-8:return []
    if area<0:points.reverse()
    result=[]
    while len(points)>3:
        for i,b in enumerate(points):
            a,c=points[i-1],points[(i+1)%len(points)];turn=cross2(b-a,c-b)
            if abs(turn)<=1e-8:
                points.pop(i);break
            if turn<0:continue
            other=[p for j,p in enumerate(points) if j not in ((i-1)%len(points),i,(i+1)%len(points))]
            if any(cross2(b-a,p-a)>=-1e-8 and cross2(c-b,p-b)>=-1e-8 and cross2(a-c,p-c)>=-1e-8 for p in other):continue
            result.append(np.array([a,b,c]));points.pop(i);break
        else:raise ValueError('Clipping attachment must be a simple polygon')
    if len(points)==3:result.append(np.array(points))
    return result


def clip_mesh(vertices,uv,triangles,polygon):
    """Intersect mesh triangles with a contour, interpolating UV at each cut.

    A concave contour is first partitioned into disjoint triangles. Clipping
    each mesh triangle against those convex pieces retains its texture map
    and permits tight sampled bounds, without painting outside the contour.
    """
    contours=polygon_triangles(polygon);out=[];indices=[]
    for tri in triangles:
        source=np.column_stack((vertices[tri],uv[tri]))
        for contour in contours:
            current=list(source)
            for a,b in zip(contour,np.roll(contour,-1,axis=0)):
                previous=current;current=[]
                if not previous:break
                last=previous[-1];before=cross2(b-a,last[:2]-a)
                for point in previous:
                    after=cross2(b-a,point[:2]-a)
                    if (after>=-1e-8)!=(before>=-1e-8):
                        fraction=before/(before-after)
                        current.append(last+(point-last)*fraction)
                    if after>=-1e-8:current.append(point)
                    last,before=point,after
            if len(current)<3:continue
            first=len(out);out.extend(current)
            for j in range(1,len(current)-1):
                if abs(cross2(current[j][:2]-current[0][:2],current[j+1][:2]-current[0][:2]))>1e-8:
                    indices.append((first,first+j,first+j+1))
    if not indices:return np.empty((0,2)),np.empty((0,2)),np.empty((0,3),dtype=int)
    out=np.asarray(out);return out[:,:2],out[:,2:],np.asarray(indices,dtype=int)


def attachment_vertices(attachment,world,bone,deformation=None):
    result=[];at=0
    for influences in attachment['vertices']:
        vertex=np.zeros(3)
        for bi,x,y,weight in influences:
            if deformation is not None:x+=deformation[at];y+=deformation[at+1]
            at+=2;vertex+=(world[bi if bi>=0 else bone]@np.array([x,y,1]))*weight
        result.append(vertex[:2])
    return np.array(result)


def geometry(sk,world,slots,deforms,order,textures,exclude_regions=(),skin=0):
    # Spine resolves each attachment in the selected skin first, then the
    # default skin. Keep the owning skin for its deform timeline as well.
    if not 0 <= skin < len(sk.skins):raise ValueError('Invalid skin index')
    draw=[];atts=sk.skins[0][1];selected=sk.skins[skin][1];clipping=None;clip_end=-1
    for si in order:
        s=slots[si];name=s['attachment'];key=(si,name)
        owner=skin if key in selected else 0;o=selected.get(key,atts.get(key))
        if o and o['kind']==6:
            if clipping is None:
                clipping=attachment_vertices(o,world,s['bone'],deforms.get((owner,si,name)))
                clip_end=o['end_slot']
        elif o and o['kind'] in (0,2) and s['color'][3]>0 and o['path'] not in exclude_regions:
            tex=textures.get(o['path'])
            if tex is None:raise ValueError('Missing region '+o['path'])
            if o['kind']==0:
                w,h=o['w'],o['h'];local=matrix([o['rotation'],o['x'],o['y'],o['sx'],o['sy'],0,0]);vs=np.array([[-w/2,-h/2,1],[-w/2,h/2,1],[w/2,h/2,1],[w/2,-h/2,1]])
                vs=(world[s['bone']]@local@vs.T).T[:,:2];uv=np.array([[0,1],[0,0],[1,0],[1,1.]]);tris=np.array([[0,1,2],[2,3,0]])
            else:
                deformation=deforms.get((owner,si,o.get('deform_parent',name)))
                vs=attachment_vertices(o,world,s['bone'],deformation);uv=o['uv'];tris=o['triangles']
            if clipping is not None:vs,uv,tris=clip_mesh(vs,uv,tris,clipping)
            if len(tris):draw.append((vs,uv,tris,tex,s['color']*rgba(o['color']),s['blend']))
        # The end slot is included in the clipped range, even when empty.
        if si==clip_end:clipping=None;clip_end=-1
    return draw


def raster(draw,scale,origin,canvas):
    width,height=canvas;out=np.zeros((height,width,4))
    for vs,uv,tris,tex,tint,blend in draw:
        xy=vs*scale+origin;xy[:,1]=origin[1]-vs[:,1]*scale;layer=np.zeros_like(out)
        for tri in tris:
            dst=xy[tri];src=uv[tri]*[tex.shape[1]-1,tex.shape[0]-1]
            x0,y0=np.maximum(0,np.floor(dst.min(axis=0)).astype(int));x1,y1=np.minimum([width-1,height-1],np.ceil(dst.max(axis=0)).astype(int))
            if x1<x0 or y1<y0:continue
            mat=np.vstack([dst.T,np.ones(3)])
            if abs(np.linalg.det(mat))<1e-6:continue
            xx,yy=np.meshgrid(np.arange(x0,x1+1)+0.5,np.arange(y0,y1+1)+0.5);weights=np.linalg.solve(mat,np.vstack([xx.ravel(),yy.ravel(),np.ones(xx.size)]));inside=(weights.min(axis=0)>=-1e-5)&(weights.max(axis=0)<=1.00001)
            sx,sy=src.T@weights;sx=np.clip(sx,0,tex.shape[1]-1);sy=np.clip(sy,0,tex.shape[0]-1)
            # Bilinear filtering is done offline, never on the calculator.
            ix=np.floor(sx).astype(int);iy=np.floor(sy).astype(int);jx=np.minimum(ix+1,tex.shape[1]-1);jy=np.minimum(iy+1,tex.shape[0]-1);fx=(sx-ix)[:,None];fy=(sy-iy)[:,None]
            sample=((tex[iy,ix]*(1-fx)+tex[iy,jx]*fx)*(1-fy)+(tex[jy,ix]*(1-fx)+tex[jy,jx]*fx)*fy)*tint
            by,bx=np.where(inside.reshape(xx.shape));layer[y0:y1+1,x0:x1+1][by,bx]=sample[inside]
        alpha=layer[:,:,3:4];old=out[:,:,3:4];new=alpha+old*(1-alpha)
        if blend==1:
            out[:,:,:3]=np.minimum(1,out[:,:,:3]+layer[:,:,:3]*alpha);out[:,:,3:4]=new
        elif blend in (2,3):
            src=layer[:,:,:3];dst=out[:,:,:3]
            mix=src*dst if blend==2 else src+dst-src*dst
            premul=src*alpha*(1-old)+dst*old*(1-alpha)+mix*alpha*old
            out[:,:,:3]=np.divide(premul,new,out=np.zeros_like(premul),where=new>0)
            out[:,:,3:4]=new
        else:
            out[:,:,:3]=np.divide(layer[:,:,:3]*alpha+out[:,:,:3]*old*(1-alpha),new,out=np.zeros_like(out[:,:,:3]),where=new>0);out[:,:,3:4]=new
    return Image.fromarray(np.uint8(np.clip(out*255,0,255)))


def bake(pack,name,clip,texture,frames=16,height=88,max_width=160,cache=None,exclude_regions=()):
    if cache is None:cache={}
    if name not in cache:
        sk=Skeleton(pack.read(pack.imported(name+'.skel','.spskel')));textures=load_textures(pack,name,texture);cache[name]=(sk,textures)
    sk,textures=cache[name]
    if clip not in sk.animations:raise ValueError(f'{name} has no {clip}: {list(sk.animations)}')
    duration=sk.animations[clip]['duration'];samples=[]
    for i in range(frames):samples.append(geometry(sk,*pose(sk,clip,i*duration/frames),textures,exclude_regions))
    allv=np.concatenate([d[0] for ds in samples for d in ds]);low=allv.min(axis=0);high=allv.max(axis=0)
    # A shared scale across a character's clips prevents attack-size pumping.
    key=(name+'-scale',tuple(exclude_regions))
    if key not in cache:cache[key]=min((height-4)/(high[1]-low[1]),(max_width-4)/(high[0]-low[0]))
    scale=cache[key];w=int(math.ceil((high[0]-low[0])*scale))+4;h=int(math.ceil((high[1]-low[1])*scale))+4
    if w>320 or h>240:raise ValueError('Animation exceeds calculator stage')
    origin=np.array([2-low[0]*scale,2+high[1]*scale]);ims=[raster(d,scale,origin,(w,h)) for d in samples]
    return ims,dict(name=name,clip=clip,source_version=sk.version,duration_ms=max(1,round(duration*1000)),frame_ms=max(1,round(duration*1000/frames)),frames=frames,pivot=[round(origin[0]),round(origin[1])],size=[w,h])

if __name__=='__main__':
    import sys
    from pck import Pack
    p=Pack(sys.argv[1])
    for name in sys.argv[2:]:
        s=Skeleton(p.read(p.imported(name+'.skel','.spskel')))
        print(name,[(k,round(v['duration'],2),len(v['tracks'])) for k,v in s.animations.items()])
