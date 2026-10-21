#!/usr/bin/env python3
"""Compile the same C++ game to a self-contained offline browser host."""
from pathlib import Path
import subprocess,shutil,base64,argparse
ROOT=Path(__file__).resolve().parents[1]
def compile_runtime():
    clang=shutil.which('clang++');link=shutil.which('wasm-ld')
    if not clang or not link:raise SystemExit('LLVM clang++ and wasm-ld are required.')
    directory=ROOT/'build'/'web';directory.mkdir(parents=True,exist_ok=True)
    flags=['--target=wasm32','-std=c++17','-O2','-ffreestanding','-fno-builtin','-fno-exceptions','-fno-rtti','-fno-threadsafe-statics','-fno-stack-protector','-I'+str(ROOT/'include')]
    objects=[]
    for i,source in enumerate(['src/characters.cpp','src/core.cpp','src/potions_extra.cpp','src/potions.cpp','src/ui_input.cpp','src/relics.cpp','src/run.cpp','src/resource_source.cpp','src/content.cpp','src/app.cpp','src/pointer.cpp','src/world_card_factory.cpp', 'src/original_events.cpp', 'src/world_relics.cpp', 'src/world_rng.cpp','src/reference_rng.cpp','src/reference_map.cpp','src/map_preview.cpp','src/reference_math.cpp','src/reference_act.cpp','src/map_preview_input.cpp','src/freestanding.cpp','platform/web/main.cpp','src/lzma2_block.cpp','third_party/lzma2/LzmaDec.c','third_party/lzma2/Lzma2Dec.c']):
        out=directory/f'{i}.o';objects.append(str(out))
        if source.endswith('.c'):
            compile_flags=[f for f in flags if f not in ('-std=c++17','-fno-exceptions','-fno-rtti','-fno-threadsafe-statics')]
            options=['-x','c','-std=c99','-I'+str(ROOT/'platform/decoder_headers')]
        else:
            compile_flags=flags;options=['-I'+str(ROOT/'third_party/lzma2')]
            if source=='src/lzma2_block.cpp':options+=['-I'+str(ROOT/'platform/decoder_headers')]
        subprocess.run([clang,*compile_flags,*options,'-c',str(ROOT/source),'-o',str(out)],check=True)
    output=directory/'nSpire2.wasm'
    # Fixed sixteen-MiB WASM memory contains bounded directories, caches and stack.
    # Pack payloads remain in the host ArrayBuffer and enter through range reads.
    subprocess.run([link,'--no-entry','--export-memory','--initial-memory=16777216','--max-memory=16777216','-z','stack-size=262144',*['--export='+e for e in ['start','framebuffer','input','frame','closed','advance','draw_frame','pointer_move','pointer_button','pointer_cancel','pointer_scroll','text_entry_active','text_input','performance_scene']],*objects,'-o',str(output)],check=True)
    print(f'Built WebAssembly runtime; {output.stat().st_size:,} bytes')
    return output

def package_runtime():
    output=ROOT/'build'/'web'/'nSpire2.wasm'
    if not output.is_file():raise SystemExit('Compile the matching WebAssembly runtime before packaging.')
    content=(ROOT/'dist'/'nSpire2.nsp.tns').read_bytes()
    text=(ROOT/'platform/web/player.html').read_text(encoding='utf-8').replace('__WASM__',base64.b64encode(output.read_bytes()).decode()).replace('__CONTENT__',base64.b64encode(content).decode())
    notices=(ROOT/'NOTICE.md').read_text()+'\n'+(ROOT/'licenses/Go-BSD-3-Clause.txt').read_text()+'\n'+(ROOT/'licenses/OkHSV-Godot-MIT.txt').read_text()
    text+='\n<!-- '+notices.replace('--','—')+' -->\n'
    (ROOT/'dist'/'nSpire2-desktop.html').write_text(text,encoding='utf-8');print(f'Built offline desktop host; WebAssembly {output.stat().st_size:,} bytes')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    mode=parser.add_mutually_exclusive_group()
    mode.add_argument('--compile-only',action='store_true',help='Build the runtime while private content conversion runs separately')
    mode.add_argument('--package-only',action='store_true',help='Package the already built runtime and the matching private pack')
    args=parser.parse_args()
    if not args.package_only:compile_runtime()
    if not args.compile_only:package_runtime()
if __name__=='__main__':main()
