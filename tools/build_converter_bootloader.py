#!/usr/bin/env python3
"""Build a private pinned PyInstaller6.16 bootloader compatible with Unicorn.

Unicorn2.1.4 setjmp/longjmp is incompatible with CFG-enabled host executables
(upstream unicorn#2281, PyInstaller#9352/#9390). Our actual Windows source-mode
oracle passed with CFG0 while the stock frozen bootloader failed with CFG1.
This build changes only the two bootloader CFG compiler/linker flags. It does
not alter OS policy, the Unicorn library, or any mandatory linked ARM checks.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tarfile
import urllib.request

VERSION='6.16.0'
URL='https://files.pythonhosted.org/packages/94/94/1f62e95e4a28b64cfbb5b922ef3046f968b47170d37a1e1a029f56ac9cb4/pyinstaller-6.16.0.tar.gz'
ARCHIVE_SHA='53559fe1e041a234f2b4dcc3288ea8bdd57f7cad8a6644e422c27bb407f3edef'
WSCRIPT_SHA='db57aff976c609146fef15451587cc333895eb620c531f0a77013e287de51b4f'
REMOVED=("            ctx.env.append_value('CFLAGS', '/guard:cf')\n",
         "            ctx.env.append_value('LINKFLAGS', '/guard:cf')\n")


def sha(data):return hashlib.sha256(data).hexdigest()


def compatible_wscript(raw):
    if sha(raw)!=WSCRIPT_SHA:raise ValueError('PyInstaller bootloader source differs from pinned6.16')
    text=raw.decode('utf-8')
    for line in REMOVED:
        if text.count(line)!=1:raise ValueError('Unexpected number of bootloader CFG build flags')
        text=text.replace(line,'            # Unicorn2.1.4: documented non-CFG bootloader compatibility build.\n',1)
    return text.encode('utf-8')


def pe_properties(data):
    if data[:2]!=b'MZ' or len(data)<64:raise ValueError('Not a Windows bootloader PE')
    at=struct.unpack_from('<I',data,60)[0]
    if data[at:at+4]!=b'PE\x00\x00' or len(data)<at+24+72:raise ValueError('Invalid Windows bootloader PE header')
    machine,=struct.unpack_from('<H',data,at+4);optional=at+24
    magic,=struct.unpack_from('<H',data,optional)
    if machine!=0x8664 or magic!=0x20b:raise ValueError('Bootloader must remain WindowsAMD64 PE32+')
    subsystem,flags=struct.unpack_from('<HH',data,optional+68)
    return dict(machine='AMD64',subsystem=subsystem,dll_characteristics=flags,
                control_flow_guard=bool(flags&0x4000),dynamic_base=bool(flags&0x40),
                nx_compatible=bool(flags&0x100),high_entropy_va=bool(flags&0x20))


def require_compatible_pe(properties):
    if (properties['control_flow_guard'] or not properties['dynamic_base'] or
            not properties['nx_compatible'] or not properties['high_entropy_va'] or properties['subsystem']!=3):
        raise ValueError('Compatible bootloader must retain ASLR, high-entropy VA, NX and console subsystem; only CFG is disabled')


def build(work):
    if sys.platform!='win32':raise ValueError('Build this target with the Windows MSVC toolchain')
    import PyInstaller
    if PyInstaller.__version__!=VERSION:raise ValueError('Pinned PyInstaller6.16.0 is required')
    work=Path(work).resolve();work.mkdir(parents=True,exist_ok=True)
    archive=work/'pyinstaller-6.16.0.tar.gz'
    if not archive.exists():
        with urllib.request.urlopen(URL,timeout=60) as response:raw=response.read(8*1024*1024)
        if sha(raw)!=ARCHIVE_SHA:raise ValueError('Pinned PyInstaller source download hash differs')
        archive.write_bytes(raw)
    if sha(archive.read_bytes())!=ARCHIVE_SHA:raise ValueError('Cached PyInstaller source hash differs')
    source=work/'source'
    if source.exists():raise ValueError('Use a clean private bootloader build directory')
    source.mkdir()
    with tarfile.open(archive,'r:gz') as stream:stream.extractall(source,filter='data')
    tree=source/'pyinstaller-6.16.0';script=tree/'bootloader/wscript'
    original=script.read_bytes();patched=compatible_wscript(original);script.write_bytes(patched)
    command=[sys.executable,'waf','all','--target-arch=64bit']
    log=work/'bootloader-build.log'
    with log.open('w',encoding='utf-8') as output:
        result=subprocess.run(command,cwd=tree/'bootloader',stdout=output,stderr=subprocess.STDOUT)
    if result.returncode:
        print(log.read_text(encoding='utf-8'),file=sys.stderr,flush=True)
        raise subprocess.CalledProcessError(result.returncode,command)
    relative=Path('bootloader/Windows-64bit-intel/run.exe')
    original_package=Path(PyInstaller.__file__).resolve().parent
    old=(original_package/relative).read_bytes();new=(tree/'PyInstaller'/relative).read_bytes()
    before,after=pe_properties(old),pe_properties(new)
    require_compatible_pe(after)
    # Leave the installed package intact. Only the next explicit build command
    # uses this private package tree through its scoped PYTHONPATH.
    vendor=work/'vendor/PyInstaller'
    shutil.copytree(original_package,vendor,ignore=shutil.ignore_patterns('__pycache__'))
    (vendor/relative).write_bytes(new)
    licenses={str(path.relative_to(tree)):sha(path.read_bytes()) for path in tree.glob('COPYING*') if path.is_file()}
    record=dict(schema=1,pyinstaller_version=VERSION,source_url=URL,source_sha256=ARCHIVE_SHA,
        original_wscript_sha256=sha(original),patched_wscript_sha256=sha(patched),
        removed_build_lines=REMOVED,command=command,build_log_sha256=sha(log.read_bytes()),
        stock_bootloader_sha256=sha(old),compatible_bootloader_sha256=sha(new),
        stock_pe=before,compatible_pe=after,license_files=licenses,
        windows_system_policy_changed=False,unicorn_changed=False,mandatory_arm_gate_unchanged=True,
        upstream=['https://github.com/unicorn-engine/unicorn/issues/2281',
                  'https://github.com/pyinstaller/pyinstaller/issues/9352',
                  'https://github.com/pyinstaller/pyinstaller/pull/9390'])
    (work/'bootloader-compatibility.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(record),flush=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--work',type=Path,required=True)
    build(parser.parse_args().work)

if __name__=='__main__':main()
