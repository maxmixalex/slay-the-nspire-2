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


def build(output,clang,linker):
    output=Path(output).resolve();output.mkdir(parents=True,exist_ok=True)
    clang=Path(clang).resolve(strict=True);linker=Path(linker).resolve(strict=True)
    before=source_inventory();compiler=subprocess.check_output([str(clang),'--version'],text=True).strip()
    if 'clang version 18.' not in compiler and 'clang version 18.' not in compiler.lower():
        raise ValueError('The production ARM validation helper requires LLVM18')
    with tempfile.TemporaryDirectory(prefix='nspire2-arm-toolchain-') as folder:
        tools=Path(folder)
        os.symlink(clang,tools/'clang++');os.symlink(linker,tools/'ld.lld')
        env=os.environ.copy();env['PATH']=str(tools)+os.pathsep+env.get('PATH','')
        subprocess.run([sys.executable,str(ROOT/'tools/build_ndless.py'),'--output',str(tools/'unused-launcher.tns')],
                       cwd=ROOT,env=env,check=True)
    if source_inventory()!=before:raise ValueError('ARM source changed while building the validation helper')
    elf=(ROOT/'build/ndless/nSpire2.elf').read_bytes();(output/'nSpire2.elf').write_bytes(elf)
    record=dict(schema=1,elf_sha256=sha256(elf),elf_bytes=len(elf),compiler=compiler,
        compiler_sha256=sha256(clang.read_bytes()),linker_sha256=sha256(linker.read_bytes()),
        linker=subprocess.check_output([str(linker),'--version'],text=True).strip(),
        source_files=before,unicorn_version='2.1.4',calculator_memory_limits_changed=False,
        command='tools/build_ndless.py; unchanged production ARM926 flags; LLVM18')
    (output/'arm-validation.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8')
    proof=self_test(output);record['linked_self_test']=proof
    (output/'arm-validation.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(complete=True,elf_sha256=record['elf_sha256'],elf_bytes=len(elf),streams=len(proof['streams']))),flush=True)


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--clang',default=shutil.which('clang++-18'));p.add_argument('--linker',default=shutil.which('ld.lld-18'))
    a=p.parse_args()
    if not a.clang or not a.linker:p.error('Install LLVM18 for the build; end users need no compiler')
    build(a.output,a.clang,a.linker)

if __name__=='__main__':main()
