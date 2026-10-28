#!/usr/bin/env python3
"""Compile a separately obtained official Spine 4.2 C# runtime privately.

This repository contains neither Esoteric Software runtime code nor its binary.
The source directory must retain the upstream LICENSE and a source-manifest.json
recording its fixed revision and file identities. The output is a local asset
conversion dependency; it is never linked into a released game executable.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
UPSTREAM = 'EsotericSoftware/spine-runtimes'
COMMIT = 'e7dc1435fa4a0083ab431f1b28e083c14a1f5c68'


def private_path(path):
    path = Path(path).resolve()
    if path == ROOT or ROOT in path.parents:
        raise ValueError('Private Spine inputs/output must stay outside the public repository')
    return path


def build(source, output, dotnet):
    source, output = private_path(source), private_path(output)
    manifest = json.loads((source / 'source-manifest.json').read_text())
    if manifest['upstream'] != UPSTREAM or manifest['commit'] != COMMIT:
        raise ValueError('Supply the reviewed fixed official Spine 4.2 source revision')
    inputs = []
    for row in manifest['files']:
        path = source / row['path']
        raw = path.read_bytes()
        if len(raw) != row['bytes'] or hashlib.sha256(raw).hexdigest() != row['sha256']:
            raise ValueError('Changed official source file: ' + row['path'])
        if path.suffix == '.cs':
            inputs.append(path)
    if len(inputs) != 47 or not (source / 'LICENSE').is_file():
        raise ValueError('Incomplete official runtime source/license')
    listing = subprocess.check_output([str(dotnet), '--list-sdks'], text=True)
    sdks = re.findall(r'^(\S+) \[([^\]]+)\]$', listing, re.M)
    version_key = lambda value: tuple(int(x) for x in re.findall(r'\d+', value))
    version, directory = max(sdks, key=lambda row: version_key(row[0]))
    directory = Path(directory)
    refs_root = directory.parent / 'packs/Microsoft.NETCore.App.Ref'
    refs_version = max((p for p in refs_root.iterdir() if p.is_dir()), key=lambda p: version_key(p.name))
    refs = sorted((refs_version / 'ref' / ('net' + str(version_key(refs_version.name)[0]) + '.0')).glob('*.dll'))
    if not refs:
        raise ValueError('Missing local .NET reference assemblies')
    output.mkdir(parents=True, exist_ok=True)
    assembly = output / 'OurNotes.Spine.Runtime.dll'
    subprocess.run([str(dotnet), str(directory / version / 'Roslyn/bincore/csc.dll'),
                    '-nologo', '-target:library', '-optimize+', '-deterministic+',
                    '-out:' + str(assembly), *('-r:' + str(p) for p in refs),
                    *(str(p) for p in inputs)], check=True)
    shutil.copyfile(source / 'LICENSE', output / 'Spine-Runtimes-LICENSE')
    receipt = dict(upstream=UPSTREAM, commit=COMMIT, source_files=len(inputs),
                   sdk=version, assembly=str(assembly), bytes=assembly.stat().st_size,
                   sha256=hashlib.sha256(assembly.read_bytes()).hexdigest(),
                   distribution='private offline conversion dependency only')
    (output / 'runtime-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--dotnet', type=Path, required=True)
    args = parser.parse_args()
    build(args.source, args.output, args.dotnet)
