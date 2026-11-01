"""Strict private NSP2/10..14 reader and append-only versioned encoder."""
import struct
from conversion_context import file_limit
from build_content import fnv
from character_ui_profile import CARDS,STRINGS,AUXILIARY
from all_character_ui_profile import CARDS as ALL_CARDS,STRINGS as ALL_STRINGS,AUXILIARY as ALL_AUXILIARY,CLIPS as ALL_CLIPS
from world_ui_profile import CARDS as WORLD_CARDS,STRINGS as WORLD_STRINGS,AUXILIARY as WORLD_AUXILIARY,CLIPS as WORLD_CLIPS,FILE_LIMIT,DIRECTORY_LIMIT
LIMIT=13*1024*1024
PROFILES={10:(68,269,105),11:(114,407,185),12:(CARDS,STRINGS,AUXILIARY),13:(ALL_CARDS,ALL_STRINGS,ALL_AUXILIARY),14:(WORLD_CARDS,WORLD_STRINGS,WORLD_AUXILIARY)}
CLIP_COUNTS={10:17,11:17,12:17,13:ALL_CLIPS,14:WORLD_CLIPS}
GLYPH_LIMITS={10:8192,11:8192,12:8192,13:16384,14:32768}

class PackData:
    """Bounded directory reader/writer shared by conversion regression tests."""
    def __init__(self,data:bytes):
        if not 40<=len(data)<=FILE_LIMIT:raise ValueError('Pack size outside file limit')
        data=bytes(data)
        magic,self.version,commit,n,ni,ng,ns,nc,nk,digest=struct.unpack_from('<4s9I',data)
        if (magic!=b'NSP2' or self.version not in PROFILES or commit!=0x41cef1ea or
            n!=len(data)):raise ValueError('Expected pinned NSP2/10..14')
        if self.version<14 and n>file_limit(LIMIT):raise ValueError('Historical pack size outside conversion file bound')
        expected_cards,expected_strings,nu=PROFILES[self.version]
        if ni>(65535 if self.version>=14 else 8192) or ng>GLYPH_LIMITS[self.version] or ns!=expected_strings or nc!=CLIP_COUNTS[self.version] or nk!=expected_cards:
            raise ValueError('Incompatible directory profile')
        clips_at=40+ni*16+ng*16+ns*8;cards_at=clips_at+nc*16;aux_at=cards_at+nk*16
        end=aux_at+nu*4
        if end>n or end>DIRECTORY_LIMIT:raise ValueError('Truncated or oversized directories')
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
            if not 0<count<=256 or first+count>ni or not 0<ms<=1000 or reserved or not -640<=px<=640 or not -480<=py<=480:raise ValueError('Invalid clip')
    def encode(self,version:int=11)->bytes:
        ni,ng,ns,nk,nu=len(self.images),len(self.glyphs),len(self.strings),len(self.cards),len(self.aux)
        if version not in (11,12,13,14) or ni>(65535 if version>=14 else 8192) or ng>GLYPH_LIMITS[version] or (nk,ns,nu)!=PROFILES[version] or len(self.clips)!=CLIP_COUNTS[version]*16:raise ValueError('Output profile exceeded')
        nc=CLIP_COUNTS[version]
        start=40+ni*16+ng*16+ns*8+nc*16+nk*16+nu*4
        if start>DIRECTORY_LIMIT:raise ValueError('Output directory exceeds bounded cache')
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
        output=struct.pack('<4s9I',b'NSP2',version,0x41cef1ea,40+len(body),ni,ng,ns,nc,nk,0)+body
        if len(output)>(FILE_LIMIT if version>=14 else file_limit(LIMIT)):raise ValueError(f'Pack exceeds versioned size limit: {len(output)}')
        return output
