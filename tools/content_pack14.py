"""Strict private NSP2/10 reader and append-only NSP2/11 encoder."""
import struct
from build_content import fnv
LIMIT=8*1024*1024
PROFILES={10:(68,269,105),11:(114,407,185)}

class PackData:
    """Bounded directory reader/writer shared by conversion regression tests."""
    def __init__(self,data:bytes):
        if not 40<=len(data)<=LIMIT:raise ValueError('Pack size outside resident limit')
        data=bytes(data)
        magic,self.version,commit,n,ni,ng,ns,nc,nk,digest=struct.unpack_from('<4s9I',data)
        if (magic!=b'NSP2' or self.version not in (10,11) or commit!=0x41cef1ea or
            n!=len(data) or digest!=fnv(data[40:])):raise ValueError('Expected intact NSP2/10 or NSP2/11')
        expected_cards,expected_strings,nu=PROFILES[self.version]
        if ni>8192 or ng>8192 or ns!=expected_strings or nc!=17 or nk!=expected_cards:
            raise ValueError('Incompatible directory profile')
        clips_at=40+ni*16+ng*16+ns*8;cards_at=clips_at+nc*16;aux_at=cards_at+nk*16
        end=aux_at+nu*4
        if end>n:raise ValueError('Truncated directories')
        def block(off:int,count:int)->bytes:
            if off<end or count<0 or off>n-count:raise ValueError('Payload outside pack')
            return data[off:off+count]
        self.images=[]
        for i in range(ni):
            off,w,h,fmt,count=struct.unpack_from('<IHHII',data,40+i*16)
            if not 0<w<=320 or not 0<h<=240 or fmt>5 or off%2:raise ValueError('Invalid image')
            self.images.append((w,h,fmt,block(off,count)))
        self.glyphs={}
        for i in range(ng):
            cp,off,px,w,h,advance,enc=struct.unpack_from('<IIHBBHH',data,40+ni*16+i*16)
            if cp>0x10ffff or not 0<w<=40 or not 0<h<=40 or advance>40 or px not in (8,10,12,16,20):raise ValueError('Invalid glyph')
            if (cp,px) in self.glyphs:raise ValueError('Duplicate glyph')
            raw=block(off,enc or w*h)
            if enc and (enc%2 or not all(raw[::2]) or sum(raw[::2])!=w*h):raise ValueError('Invalid glyph runs')
            self.glyphs[cp,px]=(w,h,advance,enc,raw)
        self.strings=[]
        for i in range(ns):
            off,count=struct.unpack_from('<II',data,40+ni*16+ng*16+i*8);raw=block(off,count+1)
            if raw[-1] or b'\0' in raw[:-1]:raise ValueError('Invalid text termination')
            self.strings.append(raw[:-1].decode('utf-8'))
        self.clips=data[clips_at:cards_at]
        self.cards=[struct.unpack_from('<4I',data,cards_at+i*16) for i in range(nk)]
        self.aux=list(struct.unpack_from('<'+'I'*nu,data,aux_at))
        if any(r[0]>=ni or any(t>=ns for t in r[1:]) for r in self.cards) or any(i>=ni for i in self.aux):raise ValueError('Invalid directory reference')
        for i in range(nc):
            first,count,ms,px,py,reserved=struct.unpack_from('<IHHhhI',self.clips,i*16)
            if not 0<count<=256 or first+count>ni or not 0<ms<=1000 or reserved:raise ValueError('Invalid clip')
    def encode(self)->bytes:
        ni,ng,ns,nk,nu=len(self.images),len(self.glyphs),len(self.strings),len(self.cards),len(self.aux)
        if ni>8192 or ng>8192 or ns!=407 or nk!=114 or nu!=185:raise ValueError('Output profile exceeded')
        start=40+ni*16+ng*16+ns*8+17*16+nk*16+nu*4
        payload=bytearray();unique={}
        def add(raw:bytes,align:int=1)->int:
            key=(align,raw)
            if key in unique:return unique[key]
            while (start+len(payload))%align:payload.append(0)
            off=start+len(payload);unique[key]=off;payload.extend(raw);return off
        images=b''.join(struct.pack('<IHHII',add(raw,2),w,h,fmt,len(raw)) for w,h,fmt,raw in self.images)
        glyphs=b''.join(struct.pack('<IIHBBHH',cp,add(raw),px,w,h,advance,enc) for (cp,px),(w,h,advance,enc,raw) in sorted(self.glyphs.items(),key=lambda p:(p[0][1],p[0][0])))
        strings=b''.join(struct.pack('<II',add(t.encode()+b'\0'),len(t.encode())) for t in self.strings)
        body=images+glyphs+strings+self.clips+b''.join(struct.pack('<4I',*r) for r in self.cards)+struct.pack('<'+'I'*nu,*self.aux)+payload
        output=struct.pack('<4s9I',b'NSP2',11,0x41cef1ea,40+len(body),ni,ng,ns,17,nk,fnv(body))+body
        if len(output)>LIMIT:raise ValueError(f'Pack exceeds resident buffer: {len(output)}')
        return output
