#!/usr/bin/env python3
"""Compile the same C++ game to a self-contained offline browser host."""
from pathlib import Path
import subprocess,shutil,base64,argparse,json,re
ROOT=Path(__file__).resolve().parents[1]
def compile_runtime(debug_commands=False):
    clang=shutil.which('clang++') or shutil.which('clang++-18');link=shutil.which('wasm-ld') or shutil.which('wasm-ld-18')
    if not clang or not link:raise SystemExit('LLVM clang++ and wasm-ld are required.')
    directory=ROOT/'build'/('web-debug' if debug_commands else 'web');directory.mkdir(parents=True,exist_ok=True)
    flags=['--target=wasm32','-std=c++17','-O2','-ffreestanding','-fno-builtin','-fno-exceptions','-fno-rtti','-fno-threadsafe-statics','-fno-stack-protector','-I'+str(ROOT/'include')]
    flags += ['-DSPIRE_DEBUG_COMMANDS='+str(int(debug_commands))]
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
    debug_exports=['debug_buffer','debug_capacity','debug_execute','debug_result','debug_catalog','debug_catalog_count'] if debug_commands else []
    subprocess.run([link,'--no-entry','--export-memory','--initial-memory=16777216','--max-memory=16777216','-z','stack-size=262144',*['--export='+e for e in ['start','start_split','resource_language','framebuffer','input','frame','closed','advance','draw_frame','pointer_move','pointer_button','pointer_cancel','pointer_scroll','text_entry_active','text_input','performance_scene','set_seed_entropy','set_preferences','preferences_flags','preferences_pending','preferences_ack']+debug_exports],*objects,'-o',str(output)],check=True)
    print(f'Built WebAssembly runtime; {output.stat().st_size:,} bytes')
    return output

def player_template(language='zhs'):
    text=(ROOT/'platform/web/player.html').read_text(encoding='utf-8')
    if language not in ('zhs','eng'):raise ValueError(f'Unsupported browser language: {language}')
    bindings={locale:json.loads((ROOT/f'config/web-locale-{locale}.json').read_text(encoding='utf-8')) for locale in ('zhs','eng')}
    if set(bindings['zhs'])!=set(bindings['eng']):raise ValueError('Browser locale keys differ')
    used=set(re.findall(r'data-i18n(?:-aria)?="([a-z_]+)"',text))|set(re.findall(r"\bt\('([a-z_]+)'\)",text))
    if used!=set(bindings['zhs']):raise ValueError(f'Browser locale coverage differs: {used ^ set(bindings["zhs"])}')
    if any(not value for table in bindings.values() for value in table.values()):raise ValueError('Empty browser translation')
    return text.replace('__LOCALES__',json.dumps(bindings,ensure_ascii=False,separators=(',',':')).replace('</','<\\/'))

def resource_package(content_path=None,language='zhs',common_path=None,eng_path=None,zhs_path=None):
    encode=lambda data:base64.b64encode(data).decode('ascii')
    if common_path is None:
        if eng_path or zhs_path:raise ValueError('Locale files require --common')
        content=(content_path or ROOT/'dist'/'nSpire2.nsp.tns').read_bytes()
        return {'mode':'legacy','language':language,'content':encode(content)}
    if content_path:raise ValueError('Choose --content or --common with locale files')
    from split_content import reconstruct
    common=common_path.read_bytes();languages={}
    for locale,path in (('eng',eng_path),('zhs',zhs_path)):
        if path:
            data=path.read_bytes()
            reconstruct(common,data,locale) # Resolve segments and the requested language mapping.
            languages[locale]=encode(data)
    if not languages:raise ValueError('At least one of --eng and --zhs is required')
    return {'mode':'split','common':encode(common),'languages':languages}

def package_runtime(content_path=None,destination=None,language='zhs',common_path=None,eng_path=None,zhs_path=None,wasm_path=None,source_checkpoint=None):
    output=wasm_path or ROOT/'build'/'web'/'nSpire2.wasm'
    if not output.is_file():raise SystemExit('Compile the matching WebAssembly runtime before packaging.')
    resources=resource_package(content_path,language,common_path,eng_path,zhs_path)
    text=player_template(language).replace('__WASM__',base64.b64encode(output.read_bytes()).decode()).replace('__RESOURCE_PACKAGE__',json.dumps(resources,separators=(',',':')))
    notices='\n\n'.join((ROOT/name).read_text(encoding='utf-8') for name in
        ('NOTICE.md','COPYING','licenses/Go-BSD-3-Clause.txt','licenses/OkHSV-Godot-MIT.txt','licenses/Noto-Boot-OFL.txt'))
    if source_checkpoint:
        if not re.fullmatch('[0-9a-f]{40}',source_checkpoint):raise ValueError('Source checkpoint must be a full Git commit ID')
        notices+='\n\nCorresponding runtime source:\nhttps://github.com/maxmixalex/slay-the-nspire-2/tree/'+source_checkpoint+'\n'
    # A non-executable raw-text element preserves the complete license text,
    # including double hyphens that are invalid inside an HTML comment.
    text+='\n<script type="text/plain" id="distribution-notices">'+notices.replace('</script','<\\/script')+'</script>\n'
    destination=destination or ROOT/'dist'/'nSpire2-desktop.html'
    destination.parent.mkdir(parents=True,exist_ok=True)
    destination.write_text(text,encoding='utf-8');print(f'Built {resources["mode"]} offline desktop host; WebAssembly {output.stat().st_size:,} bytes')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    mode=parser.add_mutually_exclusive_group()
    mode.add_argument('--compile-only',action='store_true',help='Build the runtime while private content conversion runs separately')
    mode.add_argument('--package-only',action='store_true',help='Package the already built runtime and the matching private pack')
    parser.add_argument('--debug-commands',action='store_true',help='Compile developer commands; off in normal builds')
    parser.add_argument('--language',choices=('zhs','eng'),default='zhs',help='Language of a legacy --content pack; split installations select either language in the browser')
    parser.add_argument('--content',type=Path,help='Private content pack to embed')
    parser.add_argument('--common',type=Path,help='Common artwork NSC1 file; encoded only once in the HTML')
    parser.add_argument('--eng',type=Path,help='English NSL1 language file')
    parser.add_argument('--zhs',type=Path,help='Chinese NSL1 language file')
    parser.add_argument('--output',type=Path,help='Generated offline HTML destination')
    args=parser.parse_args()
    runtime=ROOT/'build'/('web-debug' if args.debug_commands else 'web')/'nSpire2.wasm'
    if not args.package_only:runtime=compile_runtime(args.debug_commands)
    if not args.compile_only:package_runtime(args.content,args.output,args.language,args.common,args.eng,args.zhs,wasm_path=runtime)
if __name__=='__main__':main()
