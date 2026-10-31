#!/usr/bin/env python3
"""Build a private one-file converter with embedded Python and native helpers.

Run this on the target OS with requirements.txt and PyInstaller installed.
The input helper directory comes from build_converter_helpers.py. Original
game files, extracted fonts and converted packs are never included. The Spine
runtime's redistribution terms must permit the intended private distribution.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]


def build(args):
    if importlib.util.find_spec('PyInstaller') is None:
        raise ValueError('Install PyInstaller in the build interpreter first')
    from converter import Recipe
    from converter_finalize import add_final_stages
    helpers=args.helpers.resolve(strict=True)
    output=args.output.resolve();work=args.work.resolve()
    for path in (output,work):
        if path==ROOT or ROOT in path.parents:
            raise ValueError('Bundle outputs must remain outside the public source repository')
        path.mkdir(parents=True,exist_ok=True)
    adapter='SpineSampleAdapter.exe' if os.name=='nt' else 'SpineSampleAdapter'
    noise='nspire_noise'+('.dll' if os.name=='nt' else '.dylib' if sys.platform=='darwin' else '.so')
    required=(adapter,noise,'OurNotes.Spine.Runtime.dll','FastNoiseLite.h','helper-build.json','Spine-Runtimes-LICENSE')
    if not all((helpers/name).is_file() for name in required):
        raise ValueError('Helper directory is incomplete or targets a different OS')
    recipe=Recipe('/game','/work','/runtime','dotnet','/noise','-')
    editions=recipe.source_stages();add_final_stages(recipe,editions,Path('/output'),['eng','zhs'],True)
    modules=sorted({stage.module for stage in recipe.stages if not stage.module.startswith('@')})
    datas=[];binaries=[]
    # Source recipes read public config/header files and a few older main-only
    # tools via runpy. Bundle these files, never private conversion outputs.
    for folder in ('tools','config','include','src'):
        for path in sorted((ROOT/folder).rglob('*')):
            if path.is_file() and '__pycache__' not in path.parts:
                datas.append((str(path),str(path.parent.relative_to(ROOT))))
    for name in ('VERSION','CMakeLists.txt','requirements.txt','requirements-release.txt'):
        if (ROOT/name).is_file():
            datas.append((str(ROOT/name),'.'))
    for name in required:
        (binaries if name in (adapter,noise) else datas).append((str(helpers/name),'converter_helpers'))
    # Native .NET binaries are already stripped and signed as appropriate for
    # their build; UPX must not rewrite their embedded bundles.
    # Analyze all stage imports to collect third-party dependencies, then keep
    # our tool modules exclusively as source files. Their __file__ must remain
    # <bundle>/tools/name.py because legacy recipes derive ROOT from it.
    spec=work/'nspire2-convert.spec'
    spec.write_text('from pathlib import Path\n'+
        f'a=Analysis([{str(ROOT/"convert.py")!r}], pathex=[{str(ROOT/"tools")!r}], '
        f'binaries={binaries!r}, datas={datas!r}, hiddenimports={modules!r}, '
        'hookspath=[], hooksconfig={}, runtime_hooks=[], excludes=[], noarchive=False)\n'+
        f'tools=Path({str(ROOT/"tools")!r}).resolve()\n'+
        'a.pure=[row for row in a.pure if not (Path(row[1]).resolve()==tools or tools in Path(row[1]).resolve().parents)]\n'+
        'pyz=PYZ(a.pure)\n'+
        'exe=EXE(pyz,a.scripts,a.binaries,a.datas,[],name="nspire2-convert",'
        'debug=False,bootloader_ignore_signals=False,strip=False,upx=False,console=True)\n')
    command=[sys.executable,'-m','PyInstaller','--noconfirm','--clean',
             '--distpath',str(output),'--workpath',str(work/'build'),str(spec)]
    subprocess.run(command,check=True,cwd=ROOT)
    executable=output/('nspire2-convert.exe' if os.name=='nt' else 'nspire2-convert')
    subprocess.run([str(executable),'--help'],check=True)
    # Keep OS facilities (.NET extraction, Windows system DLL resolution), but
    # remove the developer's Python, dotnet and compiler paths for this smoke.
    environment=os.environ.copy()
    environment['PATH']=str(Path(os.environ.get('SystemRoot','C:/Windows'))/'System32') if os.name=='nt' else '/usr/bin:/bin'
    for key in ('PYTHONPATH','PYTHONHOME','NSPIRE_SPINE_ADAPTER','NSPIRE_FASTNOISE_LIBRARY',
                'NSPIRE_SPINE_RUNTIME','NSPIRE_FASTNOISE_HEADER','NSPIRE_DOTNET'):
        environment.pop(key,None)
    smoke=subprocess.run([str(executable),'--self-test'],check=True,text=True,capture_output=True,env=environment)
    helper_test=json.loads(smoke.stdout)
    receipt=dict(schema=1,platform=sys.platform,embedded_python=True,native_cpp_converter=False,
        separately_installed_python_required=False,dotnet_sdk_required=False,cxx_compiler_required=False,
        original_assets_included=False,bytes=executable.stat().st_size,
        sha256=hashlib.sha256(executable.read_bytes()).hexdigest(),
        smoke_test='--help and embedded dependency/native helper self-test passed',helper_self_test=helper_test,
        original_game_conversion_tested=False)
    (output/'converter-bundle.json').write_text(json.dumps(receipt,indent=2)+'\n')
    (output/'Spine-Runtimes-LICENSE').write_bytes((helpers/'Spine-Runtimes-LICENSE').read_bytes())
    (output/'FastNoiseLite-LICENSE.txt').write_text((helpers/'FastNoiseLite.h').read_text().split('#ifndef',1)[0])
    return receipt


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('helpers','output','work'):
        parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args()
    try:
        print(json.dumps(build(args)))
    except (ValueError,OSError,subprocess.CalledProcessError) as exc:
        parser.exit(1,'Converter bundle build failed: '+str(exc)+'\n')


if __name__=='__main__':main()
