#!/usr/bin/env python3
"""Bind the standalone converter to its real linked ARM decoder oracle.

This is a desktop conversion dependency. It neither changes the calculator's
memory limits nor replaces ARM execution with a cached/synthetic measurement.
"""
from __future__ import annotations
import hashlib
import json
import lzma
from pathlib import Path
import struct

ROOT=Path(__file__).resolve().parents[1]
HELPERS=('arm_division_regression.py','arm_sparse_tile_regression.py')


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def source_inventory(root=ROOT):
    root=Path(root)
    files=[path for folder in ('src','include','platform','third_party/lzma2')
           for path in (root/folder).rglob('*') if path.is_file() and '__pycache__' not in path.parts]
    files += [root/'tools/build_ndless.py',root/'tools/zehn.py',
              *(root/'tests'/name for name in HELPERS)]
    return {str(path.relative_to(root)).replace('\\','/'):sha256(path.read_bytes())
            for path in sorted(set(files))}


def verify(folder=None, *, root=ROOT):
    folder=Path(folder or Path(root)/'converter_helpers')
    elf=folder/'nSpire2.elf';manifest=folder/'arm-validation.json'
    if not elf.is_file() or not manifest.is_file():
        raise ValueError('Missing mandatory linked ARM validation helper; use the complete converter bundle')
    record=json.loads(manifest.read_text(encoding='utf-8'))
    if record.get('schema')!=1 or record.get('unicorn_version')!='2.1.4' or record.get('source_files')!=source_inventory(root):
        raise ValueError('ARM validation helper does not match this frozen converter runtime source')
    if record.get('elf_sha256')!=sha256(elf.read_bytes()):
        raise ValueError('ARM validation ELF hash differs from its build receipt')
    import unicorn
    if unicorn.__version__!='2.1.4':
        raise ValueError('ARM validation requires the bundled Unicorn2.1.4')
    return elf,record


def self_test(folder=None, *, root=ROOT):
    """Execute real linked LZMA2+index inverse before/after a legal removal."""
    elf,record=verify(folder,root=root)
    from prune_orphan_index_records_ui22 import ArmIndices,compact_records
    from tile_prediction import index_transform
    import content_tiles as t
    geometry=struct.pack('<HHHB',1,0,16,1)*16
    records=[t.FRAME.pack(16*(2+5+16*3),4,len(geometry))+struct.pack('<I',i)+geometry
             for i in (2,7,11)]
    before=b''.join(records);live={0:(len(records[0]),16,16),len(records[0])+len(records[1]):(len(records[2]),16,16)}
    after,mapping,removed=compact_records(before,3,16,live)
    if removed!=1 or after!=records[0]+records[2] or mapping!={0:0,256:128}:
        raise ValueError('ARM self-test did not construct the expected real index removal')
    arm=ArmIndices(elf);rows=[]
    for predicted in (False,True):
        pair=[]
        for raw,count in ((before,3),(after,2)):
            transformed=index_transform(raw,count,3,tile_count=16) if predicted else raw
            payload=lzma.compress(transformed,format=lzma.FORMAT_RAW,filters=[dict(
                id=lzma.FILTER_LZMA2,dict_size=512*1024,preset=9,lc=3,lp=0,pb=0)])
            pair.append(arm.index(payload,raw,count,predicted,512*1024,16))
        if pair[1]['total_instructions']>pair[0]['total_instructions']:
            raise ValueError('Actual linked ARM self-test cleanup regressed complete decoder work')
        rows.append(dict(predicted=predicted,before=pair[0],after=pair[1]))
    return dict(complete=True,elf_sha256=record['elf_sha256'],compiler=record['compiler'],
        source_inventory_sha256=sha256(json.dumps(record['source_files'],sort_keys=True).encode()),
        unicorn_version='2.1.4',real_linked_decoder_executed=True,orphan_records_removed=1,
        original_game_assets_used=False,cached_measurement_used=False,streams=rows)
