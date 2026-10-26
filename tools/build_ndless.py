#!/usr/bin/env python3
"""Build the freestanding ARM926 Ndless launcher using LLVM (no network)."""
from pathlib import Path
import shutil
import subprocess
import argparse
from zehn import build
ROOT=Path(__file__).resolve().parents[1]

def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,help='Launcher output path')
    args=parser.parse_args()
    clang=shutil.which('clang++');link=shutil.which('ld.lld')
    if not clang or not link:raise SystemExit('Install LLVM with clang++ and ld.lld, and put both on PATH.')
    directory=ROOT/'build'/'ndless';directory.mkdir(parents=True,exist_ok=True);(ROOT/'dist').mkdir(exist_ok=True)
    sources=['src/characters.cpp','src/core.cpp','src/potions_extra.cpp','src/potions.cpp','src/ui_input.cpp','src/relics.cpp','src/run.cpp','src/resource_source.cpp','src/ui_progress_codec.cpp','src/content.cpp','src/app.cpp','src/pointer.cpp','src/world_card_factory.cpp', 'src/original_events.cpp', 'src/world_relics.cpp', 'src/world_rng.cpp','src/reference_rng.cpp','src/reference_map.cpp','src/map_preview.cpp','src/reference_math.cpp','src/reference_act.cpp','src/map_preview_input.cpp','src/freestanding.cpp','platform/ndless/main.cpp','platform/ndless/entry.S','src/lzma2_block.cpp','third_party/lzma2/LzmaDec.c','third_party/lzma2/Lzma2Dec.c']
    flags=['--target=arm-none-eabi','-mcpu=arm926ej-s','-marm','-mfloat-abi=soft','-mno-unaligned-access','-O2','-ffreestanding','-fno-builtin','-fno-exceptions','-fno-rtti','-fno-threadsafe-statics','-fno-stack-protector','-fno-unwind-tables','-fno-asynchronous-unwind-tables','-ffunction-sections','-fdata-sections','-I'+str(ROOT/'include')]
    objects=[]
    for i,source in enumerate(sources):
        out=directory/f'{i}.o';objects.append(str(out))
        if source.endswith('.c'):
            # The unmodified SDK is C. Its tiny compatibility headers must not
            # shadow native/system headers or the game's C++ translation units.
            options=['-x','c','-std=c99','-I'+str(ROOT/'platform/decoder_headers')]
            compile_flags=[f for f in flags if f not in ('-fno-exceptions','-fno-rtti','-fno-threadsafe-statics')]
        else:
            options=['-std=c++17','-I'+str(ROOT/'third_party/lzma2')] if source.endswith('.cpp') else []
            if source=='src/lzma2_block.cpp':options+=['-I'+str(ROOT/'platform/decoder_headers')]
            compile_flags=flags
        subprocess.run([clang,*compile_flags,*options,'-c',str(ROOT/source),'-o',str(out)],check=True)
    elf=directory/'nSpire2.elf'
    subprocess.run([link,'-T',str(ROOT/'platform/ndless/link.ld'),'--gc-sections','--emit-relocs','-Map='+str(directory/'nSpire2.map'),*objects,'-o',str(elf)],check=True)
    output=args.output or ROOT/'dist'/'nSpire2.tns'
    output.parent.mkdir(parents=True,exist_ok=True);build(elf,output)

if __name__=='__main__':main()
