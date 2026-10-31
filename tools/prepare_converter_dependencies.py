#!/usr/bin/env python3
"""Fetch pinned public runtime sources and build converter-only dependencies.

No original game file is downloaded or read. Sources, licenses and build output
remain outside the public checkout. Download hashes identify the reviewed
dependency versions; they do not add calculator startup integrity checks.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import subprocess
import urllib.request

ROOT=Path(__file__).resolve().parents[1]


def fetch(url,path,expected):
    if path.is_file() and hashlib.sha256(path.read_bytes()).hexdigest()==expected:
        return
    request=urllib.request.Request(url,headers={'User-Agent':'nspire2-converter-dependency-builder'})
    with urllib.request.urlopen(request,timeout=60) as response:
        data=response.read(4*1024*1024+1)
    if len(data)>4*1024*1024 or hashlib.sha256(data).hexdigest()!=expected:
        raise ValueError('Pinned dependency source differs: '+url)
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_bytes(data)


def prepare(output,dotnet):
    output=Path(output).resolve()
    if output==ROOT or ROOT in output.parents:
        raise ValueError('Dependency sources and binaries must remain outside the public checkout')
    lock=json.loads((ROOT/'config/converter-dependencies.json').read_text())
    source=output/'spine-source';runtime=output/'spine-runtime'
    source.mkdir(parents=True,exist_ok=True);runtime.mkdir(parents=True,exist_ok=True)
    base=f'https://raw.githubusercontent.com/{lock["upstream"]}/{lock["commit"]}/'
    jobs=[(base+'spine-csharp/src/'+row['path'],source/row['path'],row['sha256']) for row in lock['files']]
    jobs.append((base+'LICENSE',runtime/'Spine-Runtimes-LICENSE',lock['license_sha256']))
    noise=lock['fastnoise']
    jobs.append((f'https://raw.githubusercontent.com/{noise["upstream"]}/{noise["ref"]}/{noise["path"]}',
                 output/'FastNoiseLite.h',noise['sha256']))
    with ThreadPoolExecutor(max_workers=4) as pool:
        list(pool.map(lambda row:fetch(*row),jobs))
    project=source/'OurNotes.Spine.Runtime.csproj'
    project.write_text('<Project Sdk="Microsoft.NET.Sdk"><PropertyGroup>'
        '<TargetFramework>net8.0</TargetFramework><AssemblyName>OurNotes.Spine.Runtime</AssemblyName>'
        '<ImplicitUsings>disable</ImplicitUsings><Nullable>disable</Nullable><DebugType>none</DebugType>'
        '<EnableDefaultCompileItems>false</EnableDefaultCompileItems></PropertyGroup><ItemGroup>'
        '<Compile Include="**/*.cs" Exclude="obj/**;bin/**" /></ItemGroup></Project>\n')
    subprocess.run([str(dotnet),'build',str(project),'-c','Release','-o',str(runtime)],check=True)
    assembly=runtime/'OurNotes.Spine.Runtime.dll'
    receipt=dict(upstream=lock['upstream'],commit=lock['commit'],source_files=len(lock['files']),
        assembly=str(assembly),bytes=assembly.stat().st_size,
        sha256=hashlib.sha256(assembly.read_bytes()).hexdigest(),original_game_assets=False,
        license='Spine-Runtimes-LICENSE')
    (runtime/'runtime-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    return receipt


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--dotnet',default='dotnet')
    args=parser.parse_args()
    print(json.dumps(prepare(args.output,args.dotnet)))


if __name__=='__main__':main()
