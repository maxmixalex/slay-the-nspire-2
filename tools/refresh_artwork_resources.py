#!/usr/bin/env python3
"""Publish one complete private artwork update with unchanged locale payloads.

Replacement/appended art comes from an explicit logical English pack. Existing
cards, animation clips, strings and the auxiliary prefix stay stable. All
requested original fonts and text are copied without regeneration; the new
artwork is partitioned once and every locale must reconstruct byte exactly.
Subsequent storage passes can preserve this matched concern layout.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import time

import coalesce_tile_resources as coalesce
import content_tiles as t
import split_content as split
import storage_parts as storage
from ui14_resources import graft_locale


def convert(artwork_path, source, target, work, *, locales=None):
    artwork_path=t.private_path(artwork_path,'Private updated artwork')
    source=t.private_path(source,'Private original resources')
    target=t.private_path(target,'Private updated resources')
    work=t.private_path(work,'Private logical verification packs')
    t.require(source!=target and source not in target.parents and target not in source.parents,
              'Never overwrite the original resource directory')
    codes=list(locales or split.LANGUAGES)
    t.require('eng' in codes and len(codes)==len(set(codes)) and set(codes)<=set(split.LANGUAGES),
              'English is required and requested locales must be unique')
    t.require(all((source/f'nSpire2.{code}.dat.tns').is_file() for code in codes),
              'A requested original locale is missing')
    target.mkdir(parents=True,exist_ok=True);work.mkdir(parents=True,exist_ok=True)
    artwork=artwork_path.read_bytes();view=coalesce.view(artwork)
    old_common=storage.join_parts((source/'nSpire2.dat.tns').read_bytes(),
        {name:(source/name).read_bytes() for name in storage.PART_FILES})
    bounds=split.layout(artwork,'eng')
    common=split.common_file(artwork[bounds['prefix']:bounds['prefix']+bounds['artwork']])
    index,parts,partition=storage.partition(common,artwork)
    t.atomic_bytes(target/'nSpire2.dat.tns',index)
    for name,payload in parts.items():t.atomic_bytes(target/name,payload)
    report=dict(schema=1,version=view.version,artwork_sha256=t.sha256(artwork),
        images=len(view.images),clips=len(view.clips),auxiliaries=len(view.aux),
        partition=partition,locales={})
    started=time.monotonic()
    for code in ('eng',*(code for code in codes if code!='eng')):
        previous=split.reconstruct(old_common,(source/f'nSpire2.{code}.dat.tns').read_bytes(),code)
        logical,proof=graft_locale(artwork,previous,code)
        bounds=split.layout(logical,code)
        t.require(common[64:]==logical[bounds['prefix']:bounds['prefix']+bounds['artwork']],
                  'Updated locales no longer share identical artwork')
        language=split.compress_language_directory(split.language_file(logical,bounds,code,bytes(32)))
        language,stored=storage.compress_blocks(language)
        t.require(split.reconstruct(common,language,code)==logical,'Updated locale reconstruction differs')
        t.atomic_bytes(target/f'nSpire2.{code}.dat.tns',language)
        if code in ('eng','zhs'):t.atomic_bytes(work/f'{code}-complete{view.version}.nsp',logical)
        report['locales'][code]=dict(stored_bytes=len(language),logical_sha256=t.sha256(logical),
                                    proof=proof,storage=stored)
        print(json.dumps(dict(stage='refreshed_artwork',locale=code,stored_bytes=len(language),**proof)),flush=True)
    names=('nSpire2.dat.tns',*storage.PART_FILES,*(f'nSpire2.{code}.dat.tns' for code in codes))
    report.update(complete=True,elapsed_seconds=time.monotonic()-started,
                  installed_bytes=sum((target/name).stat().st_size for name in names))
    t.atomic_bytes(target/'artwork-refresh-receipt.json',json.dumps(report,indent=2).encode()+b'\n')
    return report


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('artwork','input','output','work'):parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--locales',nargs='+',choices=split.LANGUAGES)
    args=parser.parse_args()
    report=convert(args.artwork,args.input,args.output,args.work,locales=args.locales)
    print(json.dumps({key:value for key,value in report.items() if key not in ('partition','locales')}),flush=True)


if __name__=='__main__':main()
