#!/usr/bin/env python3
"""Build the freestanding ARM926 Ndless launcher using LLVM (no network)."""
from pathlib import Path
import shutil
import subprocess
from zehn import build
ROOT=Path(__file__).resolve().parents[1]

def main()->None:
    clang=shutil.which('clang++');link=shutil.which('ld.lld')
    if not clang or not link:raise SystemExit('Install LLVM with clang++ and ld.lld, and put both on PATH.')
    directory=ROOT/'build'/'ndless';directory.mkdir(parents=True,exist_ok=True);(ROOT/'dist').mkdir(exist_ok=True)
    sources=['src/core.cpp','src/relics.cpp','src/run.cpp','src/content.cpp','src/app.cpp','src/pointer.cpp','src/freestanding.cpp','platform/ndless/main.cpp','platform/ndless/entry.S']
    flags=['--target=arm-none-eabi','-mcpu=arm926ej-s','-marm','-mfloat-abi=soft','-mno-unaligned-access','-O2','-ffreestanding','-fno-builtin','-fno-exceptions','-fno-rtti','-fno-threadsafe-statics','-fno-stack-protector','-fno-unwind-tables','-fno-asynchronous-unwind-tables','-ffunction-sections','-fdata-sections','-I'+str(ROOT/'include')]
    objects=[]
    for i,source in enumerate(sources):
        out=directory/f'{i}.o';objects.append(str(out))
        subprocess.run([clang,*flags,*(['-std=c++17'] if source.endswith('.cpp') else []),'-c',str(ROOT/source),'-o',str(out)],check=True)
    elf=directory/'nSpire2.elf'
    subprocess.run([link,'-T',str(ROOT/'platform/ndless/link.ld'),'--gc-sections','--emit-relocs','-Map='+str(directory/'nSpire2.map'),*objects,'-o',str(elf)],check=True)
    build(elf,ROOT/'dist'/'nSpire2.tns')

if __name__=='__main__':main()
