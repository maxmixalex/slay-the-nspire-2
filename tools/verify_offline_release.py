#!/usr/bin/env python3
"""Read an actual offline HTML and compare its embedded runtime and resources."""
from __future__ import annotations
import argparse
import base64
import hashlib
import json
from pathlib import Path
import re
import struct

from split_content import language_resource_version, reconstruct
from storage_parts import PART_FILES, join_parts


def require(value, message):
    if not value:raise ValueError(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def verify(html_path, wasm_path, common_path, locales, runtime_checkpoint,
           *, local_checkpoints=False, english_logical=None):
    require(re.fullmatch('[0-9a-f]{40}',runtime_checkpoint),'Expected full runtime checkpoint')
    html=html_path.read_bytes()
    match=re.search(rb'\bconst packages=(\{[^\r\n]+\});',html)
    require(match is not None,'Offline resource package is missing')
    package=json.loads(match.group(1))
    require(package.get('mode')=='split' and set(package.get('languages',{}))==set(locales),
            'Offline languages differ from the supplied installation')
    match=re.search(rb"WebAssembly\.instantiate\(decode\('([^']+)'\)",html)
    require(match is not None,'Embedded WebAssembly is missing')
    wasm=base64.b64decode(match.group(1),validate=True)
    require(wasm==wasm_path.read_bytes(),'Embedded WebAssembly differs from matching build')
    require(wasm[:8]==b'\0asm\1\0\0\0','Invalid WebAssembly header')
    source=common_path.read_bytes()
    multipart=source[:4]==b'NSM1'
    if multipart:
        source=join_parts(source,{name:(common_path.parent/name).read_bytes() for name in PART_FILES})
    common=base64.b64decode(package['common'],validate=True)
    require(common==source,'Embedded common bytes differ from the supplied artwork')
    receipt=dict(schema=1,complete=False,html_bytes=len(html),html_sha256=digest(html),
        wasm_bytes=len(wasm),wasm_sha256=digest(wasm),common_bytes=len(common),
        common_sha256=digest(common),multipart_input=multipart,locales={},
        runtime_checkpoint=runtime_checkpoint,local_checkpoints=local_checkpoints,
        driver_sha256=digest(Path(__file__).read_bytes()))
    for code,path in locales.items():
        localized=base64.b64decode(package['languages'][code],validate=True)
        original=path.read_bytes()
        require(localized==original,'Embedded locale differs: '+code)
        logical=reconstruct(common,localized,code)
        require(logical==reconstruct(source,original,code),'Reconstructed locale differs: '+code)
        if code=='eng' and english_logical:
            require(logical==english_logical.read_bytes(),'English differs from verified logical pack')
        header=struct.unpack_from('<4s9I',logical)
        receipt['locales'][code]=dict(stored_bytes=len(localized),stored_sha256=digest(localized),
            logical_bytes=len(logical),logical_sha256=digest(logical),
            version=language_resource_version(localized),images=header[4],glyphs=header[5],
            strings=header[6],clips=header[7],byte_equal=True,
            verified_logical_equal=bool(code=='eng' and english_logical))
    match=re.search(rb'<script type="text/plain" id="distribution-notices">(.*?)</script>',html,re.S)
    require(match is not None,'Offline distribution notices are missing')
    notices=match.group(1); checkpoint=runtime_checkpoint.encode()
    published=b'https://github.com/maxmixalex/slay-the-nspire-2/tree/'+checkpoint
    if local_checkpoints:
        require(b'Corresponding runtime source checkpoint (local):\n'+checkpoint in notices and
                b'has not been published to GitHub' in notices and published not in html,
                'Offline HTML misrepresents the local source checkpoint')
    else:require(published in notices,'Published source receipt is missing')
    receipt.update(complete=True,runtime_byte_equal=True,common_byte_equal=True,
                   source_receipt_verified=True)
    return receipt


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('html','wasm','common','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    for code in ('eng','zhs'):parser.add_argument('--'+code,type=Path)
    parser.add_argument('--runtime-checkpoint',required=True)
    parser.add_argument('--local-checkpoints',action='store_true')
    parser.add_argument('--english-logical',type=Path,help='Optional already verified complete logical pack')
    args=parser.parse_args()
    locales={code:getattr(args,code) for code in ('eng','zhs') if getattr(args,code)}
    require(bool(locales),'Supply the matching installed locale files')
    result=verify(args.html,args.wasm,args.common,locales,args.runtime_checkpoint,
                  local_checkpoints=args.local_checkpoints,english_logical=args.english_logical)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result))


if __name__=='__main__':main()
