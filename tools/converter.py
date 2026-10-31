#!/usr/bin/env python3
"""Build calculator resources directly from the owner's original installation.

The same entry point runs from source or inside a Python executable bundle. A
bundle embeds Python; it is not a native C++ implementation. Each recipe stage
runs in a fresh process so the original texture atlases do not accumulate in
memory. Only successfully completed, unchanged stage outputs are resumed.
"""
from __future__ import annotations

import argparse
from dataclasses import dataclass
import hashlib
import importlib
import importlib.util
import json
import os
from pathlib import Path
import runpy
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
LANGUAGES = ('eng', 'zhs', 'zht', 'deu', 'esp', 'fra', 'ind', 'ita', 'jpn', 'kor',
             'pol', 'ptb', 'rus', 'spa', 'tha', 'tur')


@dataclass(frozen=True)
class Stage:
    name: str
    module: str
    arguments: tuple[str, ...]
    outputs: tuple[Path, ...]


def digest(path):
    value = hashlib.sha256()
    with Path(path).open('rb') as stream:
        while block := stream.read(1024*1024):
            value.update(block)
    return value.hexdigest()


def atomic_json(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix+'.tmp')
    temporary.write_text(json.dumps(value, indent=2, ensure_ascii=False)+'\n', encoding='utf-8')
    os.replace(temporary, path)


def locate_game(requested=None):
    candidates = [Path(requested)] if requested else [Path.cwd(), Path(sys.executable).parent
                  if getattr(sys, 'frozen', False) else ROOT]
    for candidate in candidates:
        candidate = candidate.resolve()
        if candidate.is_file() and candidate.name == 'SlayTheSpire2.pck':
            candidate = candidate.parent
        if (candidate/'SlayTheSpire2.pck').is_file() and (candidate/'release_info.json').is_file():
            return candidate
    raise ValueError('Place the converter in the original game folder, run it there, or use --game PATH. '
                     'SlayTheSpire2.pck and release_info.json are required.')


def file_option(explicit, environment, bundled, label):
    path = Path(explicit or os.environ.get(environment) or ROOT/'converter_helpers'/bundled).resolve()
    if not path.is_file():
        raise ValueError(f'{label} is missing: {path}. Use a complete converter bundle or set {environment}.')
    return path


def launch_prefix():
    return [sys.executable] if getattr(sys, 'frozen', False) else [sys.executable, str(ROOT/'convert.py')]


def configure_helpers():
    """Prefer bundled native helpers; source developers may use their SDK."""
    folder=ROOT/'converter_helpers'
    adapter=folder/('SpineSampleAdapter.exe' if os.name=='nt' else 'SpineSampleAdapter')
    suffix='.dll' if os.name=='nt' else '.dylib' if sys.platform=='darwin' else '.so'
    noise=folder/('nspire_noise'+suffix)
    for key,path in (('NSPIRE_SPINE_ADAPTER',adapter),('NSPIRE_FASTNOISE_LIBRARY',noise)):
        if not os.environ.get(key) and path.is_file():
            os.environ[key]=str(path)
        if getattr(sys,'frozen',False) and not Path(os.environ.get(key,'')).is_file():
            raise ValueError('Incomplete converter bundle: '+path.name+' is missing')


class Recipe:
    def __init__(self, game, work, runtime, dotnet, noise, key_font, workers=2):
        self.game, self.work = Path(game), Path(work)
        self.pck, self.fonts = self.game/'SlayTheSpire2.pck', self.work/'fonts'
        self.runtime, self.dotnet, self.noise, self.key_font = runtime, dotnet, noise, key_font
        self.workers, self.stages = workers, []

    def directory(self, name):
        return self.work/name

    def add(self, name, module, arguments, outputs):
        if any(stage.name == name for stage in self.stages):
            raise ValueError('Duplicate converter stage: '+name)
        stage = Stage(name, module, tuple(map(str, arguments)), tuple(map(Path, outputs)))
        self.stages.append(stage)
        return stage.outputs

    def pair(self, name, module, previous, prefix, extra=()):
        output = self.directory(name)
        paths = tuple(output/(prefix+'-'+code+'.nsp') for code in ('eng', 'zhs'))
        self.add(name, module, ['--pck', self.pck, '--input-eng', previous[0], '--input-zhs', previous[1],
            '--cache-dir', self.directory('cache')/name, '--output-dir', output, *extra], paths)
        return paths

    def source_stages(self):
        """All catalog appends start with extracted PCK data, never a prior pack."""
        font = self.fonts/'NotoSansMonoCJKsc-Regular.otf'
        native = ['--runtime', self.runtime, '--dotnet', self.dotnet]
        self.add('fonts', '@fonts', [self.pck, self.fonts, self.key_font], [self.fonts/'font-extraction.json', font])
        base = self.directory('base')/'nSpire2.nsp.tns'
        self.add('base', 'build_content', [self.game, '--output', base.parent, '--font', font], [base])
        world = self.directory('world')/'world.nsp'
        self.add('world', 'build_world', [self.game, base, '--output', world, '--font', font, *native,
            '--fastnoise-header', self.noise, '--cache', self.directory('cache/world')], [world])
        tiled = self.directory('tiles')/'tiles.nsp'
        self.add('tiles', 'content_tiles', ['--input', world, '--output', tiled, '--work', self.directory('cache/tiles'),
            '--receipt', tiled.with_suffix('.json'), '--workers', self.workers,
            '--max-output-bytes', 256*1024*1024], [tiled])
        ui = self.directory('native-ui')/'native-ui.nsp'
        self.add('native-ui', 'append_native_ui', ['--input', tiled, '--pck', self.pck, '--output', ui,
            '--receipt', ui.with_suffix('.json')], [ui])
        local = self.directory('bilingual')
        pair = tuple(local/f'nSpire2-0.17-{code}.nsp.tns' for code in ('eng', 'zhs'))
        self.add('bilingual', 'localize_content', ['--input', ui, '--pck', self.pck,
            '--output-dir', local, '--font', font, '--language', 'both'], pair)
        pair = self.pair('feedback', 'patch_feedback_assets', pair, 'feedback', ['--jobs', self.workers])
        pair = self.pair('merchant', 'patch_merchant_effects', pair, 'merchant')
        title = self.directory('cache/title')
        self.add('title', 'title_assets', ['--pck', self.pck, '--output', title, '--background'],
                 [title/'title-logo.json', title/'title-background.json'])
        retained = self.directory('retained')
        self.add('retained', 'repack_retained', ['--eng', pair[0], '--zhs', pair[1], '--output', retained,
            '--logo-atlas', title/'title-logo.json', '--title-background', title/'title-background.json'],
            [retained/'retained-eng.nsp', retained/'retained-zhs.nsp'])
        pair = tuple(retained/f'retained-{code}.nsp' for code in ('eng','zhs'))
        for name, module, prefix, extra in (
            ('frontend', 'patch_frontend_assets', 'frontend', ()),
            ('ui6', 'patch_ui6_assets', 'ui6', ()),
            ('selection', 'patch_ui7_assets', 'ui7', [*native, '--font', font]),
            ('phobia', 'patch_phobia_assets', 'phobia', native),
            ('profile', 'patch_profile_assets', 'ui7', ()),
            ('hud', 'patch_hud_ui7_assets', 'ui7', ()),
            ('history', 'patch_history_ui8_assets', 'ui8', ()),
            ('death', 'character_death_assets', 'ui9', native),
            ('journal', 'patch_history_journal_ui8_assets', 'ui9', ()),
            ('game-over', 'patch_game_over_transition_assets', 'ui9', ()),
        ):
            pair = self.pair(name, module, pair, prefix, extra)
        for name, module in (('glyph-groups','group_glyphs'), ('string-groups','group_strings')):
            output = self.directory(name)
            next_pair = tuple(output/f'{code}.nsp' for code in ('eng','zhs'))
            for code, source, target in zip(('eng','zhs'), pair, next_pair):
                self.add(name+'-'+code, module, ['--input', source, '--output', target,
                    '--receipt', target.with_suffix('.json')], [target])
            pair = next_pair
        for name, module, extra in (
            ('card-resources','patch_card_resource_assets',()),
            ('ftue','ftue_ui9_assets',['--font-dir', self.fonts]),
            ('selector-fonts','selection_ui9_assets',['--font-dir', self.fonts]),
            ('kaiser','bestiary_kaiser_assets',native),
        ):
            pair = self.pair(name,module,pair,'ui9',extra)
        scene = self.directory('scene-fonts')
        paths = tuple(scene/f'ui9-{code}.nsp' for code in ('eng','zhs'))
        self.add('scene-fonts','scene_ui9_fonts',['--input-eng',pair[0],'--input-zhs',pair[1],
            '--font-dir',self.fonts,'--output-dir',scene],paths)
        color = self.directory('retained-filter')
        pair = tuple(color/f'ui9-{code}.nsp' for code in ('eng','zhs'))
        self.add('retained-filter','compact_retained_colors',['--eng',paths[0],'--zhs',paths[1],
            '--output',color,'--cache',self.directory('cache/retained-filter'),'--jobs',self.workers],pair)
        for name,module in (('fake-merchant','fake_merchant_ui10_assets'),
                            ('history-header','history_header_ui10_assets'),
                            ('ui10','append_ui10_final_assets')):
            pair=self.pair(name,module,pair,'ui10')
        editions=self.directory('editions')
        self.add('editions','build_locale_editions',['--input',pair[0],'--pck',self.pck,
            '--fonts',self.fonts,'--output',editions,'--languages','all','--keep-logical'],
            self.edition_files(editions)+[editions/'locales-receipt.json'])
        ui12=self.directory('ui12')
        self.add('ui12','ui12_resources',['--editions',editions,'--pck',self.pck,'--fonts',self.fonts,
            '--output',ui12,'--cache',self.directory('cache/ui12'),'--languages','all'],self.edition_files(ui12))
        legacy=self.directory('startup-input')
        self.add('startup-input','@rename-editions',[ui12,legacy,'old'],self.edition_files(legacy,old=True))
        startup=self.directory('startup')
        self.add('startup','startup_logo_resources',['--input-dir',legacy,'--output-dir',startup,
            '--cache-dir',self.directory('cache/startup'),'--pck',self.pck,'--runtime',self.runtime,
            '--fonts',self.fonts,'--languages','all'],self.edition_files(startup,old=True))
        swarm=self.directory('swarm')
        self.add('swarm','phobia_swarm_resources',['--pck',self.pck,'--input-dir',startup,
            '--output-dir',swarm,'--cache-dir',self.directory('cache/swarm')],self.edition_files(swarm,old=True))
        ready=self.directory('ui13-input')
        self.add('ui13-input','@rename-editions',[swarm,ready,'new'],
            self.edition_files(ready)+[ready/'eng.nsp'])
        artcache=self.directory('cache/ui13')
        for name,command,extra in (('ui13-assets','assets',()),
            ('ui13-retained','extend-retained',['--fastnoise-header',self.noise]),
            ('ui13-fake-rug','extend-fake-rug',())):
            # Each command extends the same verified source manifest. Use a
            # separate completion snapshot, since later stages update it.
            self.add(name,'@ui13-assets',[command,'--pck',self.pck,'--cache',artcache,*native,*extra,
                '--snapshot',artcache/(name+'.json')],[artcache/(name+'.json')])
        ui13=self.directory('ui13')
        self.add('ui13','ui13_resources',['pack','--artwork',ready/'eng.nsp',
            '--base-common',ready/'nSpire2.dat.tns','--editions',ready,'--fonts',self.fonts,
            '--output',ui13,'--pck',self.pck,'--cache',artcache],
            self.edition_files(ui13)+[ui13/'ui13-eng.nsp'])
        ui14=self.directory('ui14')
        self.add('ui14','ui14_resources',['--artwork',ui13/'ui13-eng.nsp',
            '--source-common',ui13/'nSpire2.dat.tns','--source-editions',ui13,'--pck',self.pck,
            '--cache',self.directory('cache/ui14'),'--output',ui14,'--workers',self.workers],
            self.edition_files(ui14)+[ui14/'ui14-eng.nsp'])
        vantom=self.directory('cache/vantom')
        self.add('vantom','vantom_ui15_assets',['--pck',self.pck,*native,'--input',ui14/'ui14-eng.nsp',
            '--cache',vantom,'--sample-work',self.directory('cache/vantom-samples'),
            '--fastnoise-header',self.noise],[vantom/'vantom-assets.json'])
        effects=self.directory('cache/vantom-effects')/'catalog.json'
        self.add('vantom-effects','@vantom-effects',[self.pck,effects],[effects])
        ui15=self.directory('ui15')
        self.add('ui15','ui15_resources',['--source-common',ui14/'nSpire2.dat.tns',
            '--source-editions',ui14,'--pck',self.pck,'--fonts',self.fonts,
            '--cache',self.directory('cache/ui15'),'--output',ui15,
            '--vantom-catalog',vantom/'vantom-assets.json','--extra-catalog',effects,'--languages','all'],
            self.edition_files(ui15)+[ui15/'ui15-eng.nsp'])
        return ui15

    @staticmethod
    def edition_files(directory, old=False):
        return [directory/('nSpire2.common.tns' if old else 'nSpire2.dat.tns'),
                *(directory/(f'nSpire2.{code}.tns' if old else f'nSpire2.{code}.dat.tns') for code in LANGUAGES)]


def internal(module, arguments):
    """Execute source stages in either an ordinary or an embedded interpreter."""
    if module == '@fonts':
        from extract_original_fonts import extract
        from pck import Pack
        pck, output, keyfont = map(Path, arguments)
        pack = Pack(pck)
        try:
            fonts = sorted(name[:-7] for name in pack.entries if name.startswith('fonts/') and
                           name.lower().endswith(('.ttf.import','.otf.import')))
        finally:
            pack.close()
        if not fonts:
            raise ValueError('Original PCK contains no supported imported font sources')
        extract(pck,output,fonts)
        (output/'supplement').mkdir(exist_ok=True)
        for name in ('spectral_bold.ttf','bitter_medium_italic.ttf'):
            shutil.copy2(output/name,output/'supplement'/name)
        if str(keyfont)!='-':
            shutil.copy2(keyfont,output/'TINspireKeysCX.ttf')
    elif module == '@rename-editions':
        import split_content as split
        source, output, mode = Path(arguments[0]),Path(arguments[1]),arguments[2]
        old = mode == 'old'
        output.mkdir(parents=True,exist_ok=True)
        inputs = Recipe.edition_files(source,old=not old)
        outputs = Recipe.edition_files(output,old=old)
        for before,after in zip(inputs,outputs):
            shutil.copy2(before,after)
        if not old:
            common=(output/'nSpire2.dat.tns').read_bytes()
            (output/'eng.nsp').write_bytes(split.reconstruct(common,(output/'nSpire2.eng.dat.tns').read_bytes(),'eng'))
    elif module == '@ui13-assets':
        args=list(arguments);position=args.index('--snapshot');snapshot=Path(args[position+1]);del args[position:position+2]
        internal('ui13_resources',args)
        cache=Path(args[args.index('--cache')+1])
        shutil.copy2(cache/'ui13-assets.json',snapshot)
    elif module == '@vantom-effects':
        from vantom_vfx_ui15_assets import source_assets,BASE_AUXILIARY,AUXILIARY_COUNT
        pck,output=map(Path,arguments)
        rows=source_assets(pck,output.parent)
        atomic_json(output,dict(base_auxiliary=BASE_AUXILIARY,auxiliary_count=AUXILIARY_COUNT,images=rows))
    elif module.startswith('@'):
        from converter_finalize import execute_builtin
        if not execute_builtin(module, arguments):
            raise ValueError('Unknown converter action: '+module)
    else:
        if not module.replace('_','').isalnum() or not (ROOT/'tools'/(module+'.py')).is_file():
            raise ValueError('Unknown converter stage: '+module)
        previous=sys.argv
        try:
            sys.argv=[module,*map(str,arguments)]
            loaded=importlib.import_module(module)
            if callable(getattr(loaded,'main',None)):
                # Importing functions under their real module names keeps
                # ProcessPoolExecutor workers pickleable on Windows too.
                loaded.main()
            else:
                runpy.run_path(str(ROOT/'tools'/(module+'.py')),run_name='__main__')
        finally:
            sys.argv=previous


def stage_key(stage, recipe_identity, previous):
    return hashlib.sha256(json.dumps(dict(recipe=recipe_identity, previous=previous,
        name=stage.name,module=stage.module,arguments=stage.arguments),sort_keys=True).encode()).hexdigest()


def run_stages(stages, work, identity, stop_after=None):
    receipts=Path(work)/'receipts';logs=Path(work)/'logs'
    receipts.mkdir(parents=True,exist_ok=True);logs.mkdir(parents=True,exist_ok=True)
    previous=identity
    for index,stage in enumerate(stages,1):
        key=stage_key(stage,identity,previous);receipt=receipts/(stage.name+'.json')
        saved=json.loads(receipt.read_text()) if receipt.is_file() else {}
        resumed=saved.get('status')=='complete' and saved.get('key')==key and all(
            path.is_file() and digest(path)==saved.get('outputs',{}).get(str(path)) for path in stage.outputs)
        if resumed:
            print(f'[{index}/{len(stages)}] {stage.name}: verified existing output',flush=True)
        else:
            print(f'[{index}/{len(stages)}] {stage.name}: converting (log: {logs/(stage.name+".log")})',flush=True)
            started=time.time();record=dict(stage=stage.name,key=key,status='running',started_at=started)
            atomic_json(receipt,record)
            try:
                with (logs/(stage.name+'.log')).open('w',encoding='utf-8') as output:
                    subprocess.run([*launch_prefix(),'--internal-stage',stage.module,*stage.arguments],
                                   stdout=output,stderr=subprocess.STDOUT,check=True)
                missing=[str(path) for path in stage.outputs if not path.is_file()]
                if missing:
                    raise ValueError('Stage did not produce declared outputs: '+', '.join(missing))
                record.update(status='complete',elapsed_seconds=time.time()-started,
                              outputs={str(path):digest(path) for path in stage.outputs})
                atomic_json(receipt,record)
            except BaseException as exc:
                record.update(status='failed',error=str(exc),elapsed_seconds=time.time()-started)
                atomic_json(receipt,record)
                raise
        previous=key
        if stop_after==stage.name:
            print('Stopped at the requested build stage; this is not an installable release.',flush=True)
            return False
    return True


def main(argv=None):
    argv=list(sys.argv[1:] if argv is None else argv)
    if argv[:1]==['--internal-stage']:
        internal(argv[1],argv[2:]);return 0
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game',type=Path,help='Original game folder (default: current or executable folder)')
    parser.add_argument('--output',type=Path,help='Installed data output (default: game/nSpire2-converted)')
    parser.add_argument('--work',type=Path,help='Resumable intermediate directory (default: system temporary directory)')
    parser.add_argument('--no-animations',action='store_true',help='Retain only essential actor poses with original action timing')
    parser.add_argument('--languages',default='eng,zhs',help='Comma-separated original locale codes, or all')
    parser.add_argument('--workers',type=int,choices=(1,2),default=2)
    parser.add_argument('--runtime',type=Path,help=argparse.SUPPRESS)
    parser.add_argument('--dotnet',default=os.environ.get('NSPIRE_DOTNET','dotnet'),help=argparse.SUPPRESS)
    parser.add_argument('--fastnoise-header',type=Path,help=argparse.SUPPRESS)
    parser.add_argument('--key-font',type=Path,help='Optional owner-supplied TINspireKeysCX.ttf; default: public calculator keycaps')
    parser.add_argument('--plan',action='store_true',help='Show every source build stage without converting')
    parser.add_argument('--stop-after',help='Developer checkpoint; never produces a release receipt')
    args=parser.parse_args(argv)
    try:
        game=locate_game(args.game)
        from reference_inventory import verify_release
        _lock,assembly,reference=verify_release(game)
        if not assembly.is_file():
            raise ValueError('Original sts2.dll is missing from the game installation')
        output=(args.output or game/'nSpire2-converted').resolve()
        work=(args.work or Path(tempfile.gettempdir())/('nspire2-convert-'+hashlib.sha256(str(game).encode()).hexdigest()[:12])).resolve()
        if output==game or output==work or work in output.parents or output in work.parents:
            raise ValueError('Game, output and work directories must be separate')
        codes=list(LANGUAGES) if args.languages=='all' else args.languages.split(',')
        if not codes or len(set(codes))!=len(codes) or not set(codes)<=set(LANGUAGES) or 'eng' not in codes:
            raise ValueError('--languages must contain unique supported codes including eng')
        if args.plan:
            runtime=args.runtime or Path('<bundled-runtime>');noise=args.fastnoise_header or Path('<bundled-noise-header>');keyfont=args.key_font or Path('<bundled-key-font>')
        else:
            configure_helpers()
            for module in ('PIL','numpy','fontTools','zstandard','lz4'):
                if importlib.util.find_spec(module) is None:
                    raise ValueError(f'Missing converter dependency {module}; use the executable bundle or install requirements.txt')
            runtime=file_option(args.runtime,'NSPIRE_SPINE_RUNTIME','OurNotes.Spine.Runtime.dll','Spine runtime identity')
            noise=file_option(args.fastnoise_header,'NSPIRE_FASTNOISE_HEADER','FastNoiseLite.h','Pinned noise source')
            supplied=args.key_font or os.environ.get('NSPIRE_KEY_FONT')
            keyfont=Path(supplied).resolve() if supplied else Path('-')
            if supplied and not keyfont.is_file():
                raise ValueError('Optional calculator key font does not exist: '+str(keyfont))
        recipe=Recipe(game,work,runtime,args.dotnet,noise,keyfont,args.workers)
        source_editions=recipe.source_stages()
        # The post-catalog recipe is deliberately separate from source baking:
        # it performs only verified resource/font/storage transforms.
        if args.stop_after not in {stage.name for stage in recipe.stages}:
            from converter_finalize import add_final_stages
            add_final_stages(recipe,source_editions,output,codes,args.no_animations)
        if args.stop_after and args.stop_after not in {stage.name for stage in recipe.stages}:
            raise ValueError('Unknown --stop-after stage: '+args.stop_after)
        if args.plan:
            print(json.dumps(dict(reference=reference,game=str(game),output=str(output),work=str(work),
                no_animations=args.no_animations,languages=codes,stages=[dict(name=s.name,module=s.module,
                arguments=s.arguments,outputs=list(map(str,s.outputs))) for s in recipe.stages]),indent=2))
            return 0
        os.environ.update(NSPIRE_SPINE_RUNTIME=str(runtime),NSPIRE_FASTNOISE_HEADER=str(noise),NSPIRE_DOTNET=args.dotnet)
        work.mkdir(parents=True,exist_ok=True)
        # All public recipe/config files and source inputs identify this build.
        from run_logged import inventory
        identity=hashlib.sha256(json.dumps(dict(source=inventory(ROOT),reference=reference,
            pck=digest(game/'SlayTheSpire2.pck'),assembly=digest(assembly),runtime=digest(runtime),noise=digest(noise),
            helpers={key:digest(value) for key in ('NSPIRE_SPINE_ADAPTER','NSPIRE_FASTNOISE_LIBRARY')
                     if (value:=os.environ.get(key))},
            key_font=digest(keyfont) if str(keyfont)!='-' else 'public-keycaps'),sort_keys=True).encode()).hexdigest()
        complete=run_stages(recipe.stages,work,identity,args.stop_after)
        if complete:
            print(f'Conversion complete: {output}',flush=True)
        return 0
    except (ValueError,OSError,subprocess.CalledProcessError) as exc:
        print('Conversion failed: '+str(exc),file=sys.stderr)
        return 1


if __name__=='__main__':
    raise SystemExit(main())
