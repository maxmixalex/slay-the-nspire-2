#!/usr/bin/env python3
"""Compile the same C++ game to a self-contained offline browser host."""
from pathlib import Path
import subprocess,shutil,base64
ROOT=Path(__file__).resolve().parents[1]
def main():
    clang=shutil.which('clang++');link=shutil.which('wasm-ld')
    if not clang or not link:raise SystemExit('LLVM clang++ and wasm-ld are required.')
    directory=ROOT/'build'/'web';directory.mkdir(parents=True,exist_ok=True)
    flags=['--target=wasm32','-std=c++17','-O2','-ffreestanding','-fno-builtin','-fno-exceptions','-fno-rtti','-fno-threadsafe-statics','-fno-stack-protector','-I'+str(ROOT/'include')]
    objects=[]
    for i,source in enumerate(['src/characters.cpp','src/core.cpp','src/potions.cpp','src/ui_input.cpp','src/relics.cpp','src/run.cpp','src/content.cpp','src/app.cpp','src/pointer.cpp','src/reference_rng.cpp','src/reference_map.cpp','src/map_preview.cpp','src/reference_math.cpp','src/reference_act.cpp','src/map_preview_input.cpp','src/freestanding.cpp','platform/web/main.cpp']):
        out=directory/f'{i}.o';objects.append(str(out));subprocess.run([clang,*flags,'-c',str(ROOT/source),'-o',str(out)],check=True)
    output=directory/'nSpire2.wasm'
    # Fixed nine-MiB WASM memory covers the six-MiB pack, card caches and stack.
    # Ndless static allocation is bounded independently by the Zehn packer.
    subprocess.run([link,'--no-entry','--export-memory','--initial-memory=9437184','--max-memory=9437184','-z','stack-size=262144',*['--export='+e for e in ['content_buffer','start','framebuffer','input','frame','closed','advance','draw_frame','pointer_move','pointer_button','pointer_cancel','pointer_scroll','text_entry_active','text_input']],*objects,'-o',str(output)],check=True)
    content=(ROOT/'dist'/'nSpire2.nsp.tns').read_bytes()
    text=(ROOT/'platform/web/player.html').read_text(encoding='utf-8').replace('__WASM__',base64.b64encode(output.read_bytes()).decode()).replace('__CONTENT__',base64.b64encode(content).decode())
    notices=(ROOT/'NOTICE.md').read_text()+'\n'+(ROOT/'licenses/Go-BSD-3-Clause.txt').read_text()
    text+='\n<!-- '+notices.replace('--','—')+' -->\n'
    (ROOT/'dist'/'nSpire2-desktop.html').write_text(text,encoding='utf-8');print(f'Built offline desktop host; WebAssembly {output.stat().st_size:,} bytes')
if __name__=='__main__':main()
