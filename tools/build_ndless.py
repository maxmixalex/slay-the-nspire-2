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
    parser.add_argument('--trace-io',action='store_true',help='Developer-only bounded OS I/O trace; not a release/timing acceptance build')
    parser.add_argument('--compression',choices=('zlib','zopfli'),default='zlib',help='Use zopfli for compact release packaging; requires requirements-release.txt')
    args=parser.parse_args()
    clang=shutil.which('clang++') or shutil.which('clang++-18');link=shutil.which('ld.lld') or shutil.which('ld.lld-18')
    if not clang or not link:raise SystemExit('Install LLVM with clang++ and ld.lld, and put both on PATH.')
    directory=ROOT/'build'/'ndless';directory.mkdir(parents=True,exist_ok=True);(ROOT/'dist').mkdir(exist_ok=True)
    sources=['src/characters.cpp','src/core.cpp','src/potions_extra.cpp','src/potions.cpp','src/ui_input.cpp','src/relics.cpp','src/run.cpp','src/resource_source.cpp','src/storage_source.cpp','src/ui_progress_codec.cpp','src/content.cpp','src/app.cpp','src/pointer.cpp','src/world_card_factory.cpp', 'src/original_events.cpp', 'src/world_relics.cpp', 'src/world_rng.cpp','src/reference_rng.cpp','src/reference_map.cpp','src/map_preview.cpp','src/reference_math.cpp','src/reference_act.cpp','src/map_preview_input.cpp','src/freestanding.cpp','platform/ndless/main.cpp','platform/ndless/entry.S','src/lzma2_block.cpp','third_party/lzma2/LzmaDec.c','third_party/lzma2/Lzma2Dec.c']
    flags=['--target=arm-none-eabi','-mcpu=arm926ej-s','-marm','-mfloat-abi=soft','-mno-unaligned-access','-O2','-ffreestanding','-fno-builtin','-fno-exceptions','-fno-rtti','-fno-threadsafe-statics','-fno-stack-protector','-fno-unwind-tables','-fno-asynchronous-unwind-tables','-ffunction-sections','-fdata-sections','-fstack-usage','-Wframe-larger-than=8192','-I'+str(ROOT/'include')]
    if args.trace_io:flags.append('-DSPIRE_DEVICE_IO_TRACE=1')
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
    # Clang emits address-significance metadata. Safe ICF can share identical
    # unobserved function bodies while preserving distinct significant
    # addresses; the unrestricted "all" mode is deliberately not enabled.
    subprocess.run([link,'-T',str(ROOT/'platform/ndless/link.ld'),'--gc-sections','--icf=safe','--emit-relocs','-Map='+str(directory/'nSpire2.map'),*objects,'-o',str(elf)],check=True)
    output=args.output or ROOT/'dist'/'nSpire2.tns'
    output.parent.mkdir(parents=True,exist_ok=True);build(elf,output,args.compression)

if __name__=='__main__':main()
