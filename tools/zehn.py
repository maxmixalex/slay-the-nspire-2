#!/usr/bin/env python3
"""Strict ELF32 ARM -> Zehn packer for this freestanding LLVM build.

No inferred relocations: every absolute word requires an ELF relocation.
Unknown relocation kinds, TLS, constructors and undefined symbols fail closed.
Format follows the Ndless r2022 zehn.h and genzehn.cpp allocation convention.
"""
from pathlib import Path
import struct
import zlib
import argparse

def build(source:Path,target:Path)->None:
    b=source.read_bytes()
    if b[:7]!=b'\x7fELF\x01\x01\x01':raise ValueError('Expected ELF32 little-endian')
    et,machine,ver,entry,phoff,shoff,flags,ehsize,phsize,phnum,shsize,shnum,shstr=struct.unpack_from('<HH5I6H',b,16)
    if et!=2 or machine!=40 or shsize!=40:raise ValueError('Expected linked ARM executable')
    sec=[struct.unpack_from('<10I',b,shoff+i*40) for i in range(shnum)]
    names=b[sec[shstr][4]:sec[shstr][4]+sec[shstr][5]]
    def name(s):return names[s[0]:].split(b'\0',1)[0].decode()
    alloc=[s for s in sec if s[2]&2 and s[5]]
    if not alloc or min(s[3] for s in alloc)!=0:raise ValueError('Image must be linked at zero')
    for s in alloc:
        if s[2]&0x400 or name(s) in ('.init_array','.fini_array','.ctors'):raise ValueError('TLS/constructors unsupported')
    end=max(s[3]+s[5] for s in alloc if s[1]!=8)
    memory_end=max(s[3]+s[5] for s in alloc)
    if memory_end>=0x1000000 or entry>=end:raise ValueError('Invalid image size or entry')
    image=bytearray(end);covered=bytearray(end)
    for s in alloc:
        if s[1]==8:
            if s[3]<end:raise ValueError('Non-tail NOBITS section unsupported')
            continue
        if s[4]+s[5]>len(b) or any(covered[s[3]:s[3]+s[5]]):raise ValueError('Section bounds/overlap')
        image[s[3]:s[3]+s[5]]=b[s[4]:s[4]+s[5]];covered[s[3]:s[3]+s[5]]=b'\1'*s[5]
    relocs=set()
    for s in sec:
        if s[1]==2: # symtab
            for off in range(s[4],s[4]+s[5],s[9]):
                stname,value,size,info,other,section=struct.unpack_from('<IIIBBH',b,off)
                if section==0 and stname and info>>4!=2:raise ValueError('Unresolved strong symbol')
        if s[1]!=9:continue
        if not sec[s[7]][2]&2:continue
        symtab=sec[s[6]]
        for off in range(s[4],s[4]+s[5],8):
            addr,info=struct.unpack_from('<2I',b,off);kind=info&255;symbol=info>>8
            sym=struct.unpack_from('<IIIBBH',b,symtab[4]+symbol*symtab[9])
            if kind in (2,38): # R_ARM_ABS32 / TARGET1
                if sym[5]==0:raise ValueError('Absolute relocation against undefined symbol')
                if sym[5]==0xfff1:continue # Absolute linker constant, not an image pointer.
                if addr%4 or addr+4>end:raise ValueError('Unaligned/out-of-range relocation')
                value=struct.unpack_from('<I',image,addr)[0]
                if value>memory_end:raise ValueError(f'Absolute word outside image: {value:x}')
                relocs.add(addr)
            elif kind not in (0,3,10,28,29,30,42):
                raise ValueError(f'Unsupported relocation type {kind} at {addr:x}')
    extra=b'nSpire II\0zyf722\0Experimental native adaptation 0.7; bounded rules/content subset.\0'
    extra+=bytes((-len(extra))%4)
    flag_pairs=[(0,31),(2,2022),(4,1),(5,0),(6,1),(7,0),(8,0),(9,10),(10,7),(11,17),(12,1),(13,1)]
    reloc_data=struct.pack('<I',3)+b''.join(struct.pack('<I',r<<8) for r in sorted(relocs))
    flag_data=b''.join(struct.pack('<I',kind|(data<<8)) for kind,data in flag_pairs)
    prefix=32+len(reloc_data)+len(flag_data)+len(extra)
    compressed=zlib.compress(image,9)
    result=struct.pack('<8I',0x6e68655A,1,prefix+len(compressed),len(relocs)+1,len(flag_pairs),len(extra),prefix+memory_end,entry)+reloc_data+flag_data+extra+compressed
    target.write_bytes(result)
    # Independent round trip validates bytes, allocation and every rebase word.
    decoded=zlib.decompress(result[prefix:]);assert decoded==image
    for base in (0x10100000,0x11000000):
        relocated=bytearray(decoded)+bytearray(memory_end-end)
        for r in relocs:
            v=struct.unpack_from('<I',relocated,r)[0];struct.pack_into('<I',relocated,r,v+base)
        for r in relocs:assert struct.unpack_from('<I',relocated,r)[0]-base==struct.unpack_from('<I',image,r)[0]
    print(f'{target}: {len(result):,} bytes; allocation {prefix+memory_end:,}; {len(relocs)} absolute relocations; entry 0x{entry:x}')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('input',type=Path);p.add_argument('output',type=Path);a=p.parse_args();build(a.input,a.output)
