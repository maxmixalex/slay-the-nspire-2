#!/usr/bin/env python3
"""Build the public ARM launcher and package its decoder validation helper."""
from __future__ import annotations
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from converter_arm_validation import ROOT,sha256,source_inventory,self_test
from build_ndless import SIZE_OPTIMIZED_SOURCES,SIZE_OPTIMIZATION


def build(output,clang,linker):
    output=Path(output).resolve();output.mkdir(parents=True,exist_ok=True)
    # LLVM's multicall linker selects its flavor from argv[0]. Preserve
    # ld.lld's invocation name while resolving only to verify the target.
    clang=Path(clang).absolute();linker=Path(linker).absolute()
    clang.resolve(strict=True);linker.resolve(strict=True)
    before=source_inventory();compiler=subprocess.check_output([str(clang),'--version'],text=True).strip()
    if 'clang version 18.' not in compiler and 'clang version 18.' not in compiler.lower():
        raise ValueError('The production ARM validation helper requires LLVM18')
    with tempfile.TemporaryDirectory(prefix='nspire2-arm-toolchain-') as folder:
        tools=Path(folder)
        os.symlink(clang,tools/'clang++');os.symlink(linker,tools/'ld.lld')
        env=os.environ.copy();env['PATH']=str(tools)+os.pathsep+env.get('PATH','')
        subprocess.run([sys.executable,str(ROOT/'tools/build_ndless.py'),'--output',str(tools/'unused-launcher.tns'),
                        '--size-event-rules'],
                       cwd=ROOT,env=env,check=True)
    if source_inventory()!=before:raise ValueError('ARM source changed while building the validation helper')
    elf=(ROOT/'build/ndless/nSpire2.elf').read_bytes();(output/'nSpire2.elf').write_bytes(elf)
    record=dict(schema=1,elf_sha256=sha256(elf),elf_bytes=len(elf),compiler=compiler,
        compiler_sha256=sha256(clang.read_bytes()),linker_sha256=sha256(linker.read_bytes()),
        linker=subprocess.check_output([str(linker),'--version'],text=True).strip(),
        source_files=before,unicorn_version='2.1.4',calculator_memory_limits_changed=False,
        original_events_optimization=SIZE_OPTIMIZATION['src/original_events.cpp'].lstrip('-'),
        size_optimized_sources=list(SIZE_OPTIMIZED_SOURCES),source_optimizations=dict(SIZE_OPTIMIZATION),
        command='tools/build_ndless.py; production ARM926 -Oz for event rules/input, -Os for progress codec, -O2 elsewhere, no LTO; LLVM18')
    (output/'arm-validation.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8')
    proof=self_test(output);record['linked_self_test']=proof
    (output/'arm-validation.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(complete=True,elf_sha256=record['elf_sha256'],elf_bytes=len(elf),streams=len(proof['streams']))),flush=True)


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--size-event-rules',action='store_true',default=True,help='Compatibility flag; the release ARM helper uses the same -Oz/-Os control/codec policy as the launcher')
    p.add_argument('--clang',default=shutil.which('clang++-18'));p.add_argument('--linker',default=shutil.which('ld.lld-18'))
    a=p.parse_args()
    if not a.clang or not a.linker:p.error('Install LLVM18 for the build; end users need no compiler')
    build(a.output,a.clang,a.linker)

if __name__=='__main__':main()
