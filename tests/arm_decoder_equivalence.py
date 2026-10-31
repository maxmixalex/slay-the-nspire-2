#!/usr/bin/env python3
"""Compare linked ARM decoder code, referenced constants and relocation closure.

Only relocation-dependent addresses may differ. Their target symbol and target
offset must remain identical, and every referenced object/function is compared
recursively. This permits reuse of measured group instruction counts after an
unrelated label or resource-open change without pretending the ELFs are equal.
Unsupported relocation kinds, unbounded targets and changed bytes fail closed.
"""
from __future__ import annotations
import argparse
from collections import deque
import hashlib
import json
from pathlib import Path
import struct

ROOTS = (
    '_ZN5spire18decode_lzma2_blockEPhjjPKhjj',
    '_ZN5spire28restore_tile_filter_in_placeEPhjjjS0_j',
    '_ZN5spire29restore_sparse_tiles_in_placeEPhjjjS0_j',
    '_ZN5spire24restore_index_predictionEPhjjj',
    'memcpy','memmove','memset',
)
# entry.S exports these small tail-call aliases without ELF .size records.
# Bound them by their complete fixed instruction sequence plus one mandatory
# unconditional relocated branch. A changed wrapper fails instead of allowing
# a guessed range ending at the next unrelated symbol.
ASSEMBLY_WRAPPERS = {
    **{name:((), 'memcpy') for name in ('__aeabi_memcpy','__aeabi_memcpy4','__aeabi_memcpy8')},
    '__aeabi_memmove':((), 'memmove'),
    **{name:((0xe1a03001,0xe1a01002,0xe1a02003), 'memset')
       for name in ('__aeabi_memset','__aeabi_memset4','__aeabi_memset8')},
    **{name:((0xe1a02001,0xe3a01000), 'memset')
       for name in ('__aeabi_memclr','__aeabi_memclr4','__aeabi_memclr8')},
}


def require(condition, message):
    if not condition: raise ValueError(message)


def sha(data): return hashlib.sha256(data).hexdigest()


def signed(value, bits):
    return value-(1 << bits) if value & (1 << (bits-1)) else value


class Elf:
    def __init__(self, path):
        self.path,self.data = Path(path),Path(path).read_bytes()
        d=self.data
        require(d[:7] == b'\x7fELF\x01\x01\x01' and struct.unpack_from('<H',d,18)[0] == 40,
                'Expected a little-endian ELF32 ARM image')
        offset=struct.unpack_from('<I',d,32)[0]
        width,count,names=struct.unpack_from('<HHH',d,46)
        require(width == 40 and offset+width*count <= len(d),'Invalid ELF section directory')
        self.sections=[struct.unpack_from('<10I',d,offset+i*width) for i in range(count)]
        strings=self.section_bytes(names)
        self.section_names=[self.string(strings,row[0]) for row in self.sections]
        self.tables={};self.symbols={};self.objects=[];self.relocations=[]
        for index,row in enumerate(self.sections):
            if row[1] != 2:continue
            require(row[9] == 16 and row[5]%16 == 0,'Invalid symbol table')
            names=self.section_bytes(row[6]);table=[]
            for at in range(row[4],row[4]+row[5],16):
                name,value,size,info,other,section=struct.unpack_from('<IIIBBH',d,at)
                symbol=dict(name=self.string(names,name),value=value,size=size,
                            type=info&15,section=section)
                if symbol['name'] in ASSEMBLY_WRAPPERS and not size:
                    prefix,_target=ASSEMBLY_WRAPPERS[symbol['name']]
                    symbol['size'],symbol['type']=4*(len(prefix)+1),2
                    words=struct.unpack('<'+'I'*(len(prefix)+1),self.body(symbol))
                    require(words[:-1] == prefix and words[-1]>>24 == 0xea,
                            'Unbounded or changed assembly wrapper: '+symbol['name'])
                    size=symbol['size']
                table.append(symbol)
                if symbol['name'] and size and section < len(self.sections) and symbol['type'] in (0,1,2):
                    self.symbols.setdefault(symbol['name'],[]).append(symbol);self.objects.append(symbol)
            self.tables[index]=table
        for row in self.sections:
            if row[1] != 9:continue
            require(row[9] == 8 and row[5]%8 == 0 and row[6] in self.tables,'Invalid relocation table')
            for at in range(row[4],row[4]+row[5],8):
                place,info=struct.unpack_from('<II',d,at)
                symbol_index,kind=info>>8,info&255
                require(symbol_index < len(self.tables[row[6]]),'Invalid relocation symbol')
                self.relocations.append((place,kind,self.tables[row[6]][symbol_index]))

    @staticmethod
    def string(table, at):
        require(at < len(table),'String offset is outside its ELF table')
        end=table.find(b'\0',at);require(end >= 0,'Unterminated ELF string')
        return table[at:end].decode('utf-8')

    def section_bytes(self, index):
        require(index < len(self.sections),'Invalid ELF section index')
        row=self.sections[index]
        require(row[4]+row[5] <= len(self.data),'ELF section exceeds file')
        return self.data[row[4]:row[4]+row[5]]

    def symbol(self, name):
        rows=self.symbols.get(name,[])
        require(rows and len({(r['value'],r['size'],r['type'],r['section']) for r in rows}) == 1,
                'Missing or ambiguous bounded symbol: '+name)
        return rows[0]

    def resolve(self, address):
        found=[row for row in self.objects if row['value'] <= address < row['value']+row['size']]
        require(found,'Relocation points outside a bounded symbol: '+hex(address))
        return min(found,key=lambda row:(row['value'] != address,row['size'],row['name']))

    def body(self, symbol):
        row=self.sections[symbol['section']]
        at=symbol['value']-row[3]
        require(0 <= at and at+symbol['size'] <= row[5],'Symbol crosses section bounds: '+symbol['name'])
        if row[1] == 8:return bytes(symbol['size'])
        return self.data[row[4]+at:row[4]+at+symbol['size']]

    def normalized(self, name):
        symbol=self.symbol(name);raw=self.body(symbol);normalized=bytearray(raw);references=[]
        start,end=symbol['value'],symbol['value']+symbol['size']
        for place,kind,target in self.relocations:
            if not start <= place < end:continue
            at=place-start;require(at+4 <= len(raw),'Relocation crosses symbol boundary')
            word=struct.unpack_from('<I',raw,at)[0]
            if kind in (0,40):
                # NONE and V4BX do not carry another object's address. The
                # original linked instruction remains part of byte equality.
                references.append(dict(at=at,kind=kind,target=None,offset=0));continue
            if kind in (1,27,28,29):
                address=(place+8+(signed(word&0xffffff,24)<<2)) & 0xffffffff
                struct.pack_into('<I',normalized,at,word&0xff000000)
            elif kind in (2,38):
                address=word;struct.pack_into('<I',normalized,at,0)
            elif kind == 3:
                address=(place+signed(word,32)) & 0xffffffff
                struct.pack_into('<I',normalized,at,0)
            elif kind == 42:
                address=(place+signed(word&0x7fffffff,31)) & 0xffffffff
                struct.pack_into('<I',normalized,at,word&0x80000000)
            else:
                raise ValueError(f'Unsupported ARM relocation {kind} in {name}')
            bounded=(target if target['name'] in self.symbols and target['size'] and
                     target['value'] <= address < target['value']+target['size'] else self.resolve(address))
            references.append(dict(at=at,kind=kind,target=bounded['name'],offset=address-bounded['value']))
        if name in ASSEMBLY_WRAPPERS:
            prefix,target=ASSEMBLY_WRAPPERS[name]
            require(len(references) == 1 and references[0]['at'] == len(prefix)*4 and
                    references[0]['target'] == target and references[0]['offset'] == 0 and
                    references[0]['kind'] in (1,27,28,29),'Assembly tail call differs: '+name)
        return symbol,raw,bytes(normalized),sorted(references,key=lambda row:(row['at'],row['kind']))


def compare(baseline, candidate, roots=ROOTS):
    before,after=Elf(baseline),Elf(candidate)
    pending=deque(roots);checked={}
    while pending:
        name=pending.popleft()
        if name in checked:continue
        a,raw_a,code_a,links_a=before.normalized(name)
        b,raw_b,code_b,links_b=after.normalized(name)
        require((a['size'],a['type']) == (b['size'],b['type']), 'Bounded symbol shape differs: '+name)
        require(links_a == links_b,'Linked call/data relocation semantics differ: '+name)
        require(code_a == code_b,'Non-relocation linked bytes differ: '+name)
        checked[name]=dict(kind='code' if a['type']==2 else 'data',bytes=len(raw_a),
            baseline_address=a['value'],candidate_address=b['value'],
            baseline_raw_sha256=sha(raw_a),candidate_raw_sha256=sha(raw_b),
            relocation_normalized_sha256=sha(code_a),references=links_a)
        pending.extend(row['target'] for row in links_a if row['target'] is not None)
    return dict(complete=True,baseline_elf=str(baseline),candidate_elf=str(candidate),
        baseline_elf_sha256=sha(before.data),candidate_elf_sha256=sha(after.data),
        roots=list(roots),symbols=checked,compared_symbols=len(checked),
        compared_bytes=sum(row['bytes'] for row in checked.values()),
        proof='Every bounded reachable symbol: exact linked bytes except equal target-relative relocations; recursive code, constants and initial data',
        limitation='Reuses decoder instruction-count evidence only; does not establish device FPS, OS storage latency or arbitrary external callback behavior')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('baseline',type=Path);parser.add_argument('candidate',type=Path)
    parser.add_argument('--output',type=Path)
    args=parser.parse_args();result=compare(args.baseline,args.candidate)
    encoded=json.dumps(result,indent=2)+'\n'
    if args.output:args.output.write_text(encoded)
    print(encoded)


if __name__ == '__main__':main()
