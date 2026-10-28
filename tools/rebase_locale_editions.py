#!/usr/bin/env python3
"""Reuse verified locale glyphs/text over a new private, ID-compatible art pack.

No font is rerasterized and no translation is regenerated. Every original
glyph key, alpha payload, advance and text byte is compared after rewriting
the offsets. The shared art comes verbatim from the requested final pack.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

import content_tiles as tiles
import split_content as split
from build_locale_editions import rewrite, VERSIONS
from repack_retained import auxiliary_count


def view(data):
    return tiles.PackView(data, auxiliary_count(data), versions=VERSIONS)


def glyphs_from(source):
    return [(cp, band, width, height, advance, encoded & ~tiles.GLYPH_GROUP_FLAG,
             bytes(source.glyph_payload(index)))
            for index, (cp, _offset, band, width, height, advance, encoded)
            in enumerate(source.glyphs)]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--artwork',type=Path,required=True,help='Final logical NSP2/39 art input')
    parser.add_argument('--editions',type=Path,required=True,help='Previously verified optional locale directory')
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--keep-logical',action='store_true')
    args=parser.parse_args()
    editions=tiles.private_path(args.editions,'Private verified editions')
    output=tiles.private_path(args.output,'Private rebased locale output')
    tiles.require(output.resolve()!=editions.resolve(),'Rebase output must not replace its input editions')
    output.mkdir(parents=True,exist_ok=True)
    art_data=tiles.private_path(args.artwork,'Private final artwork').read_bytes()
    art_source=view(art_data)
    old_common=(editions/'nSpire2.common.tns').read_bytes()
    manifest=json.loads((editions/'locales-receipt.json').read_text())
    reports={};common=None
    for language,receipt in manifest['languages'].items():
        old_language=(editions/f'nSpire2.{language}.tns').read_bytes()
        tiles.require(tiles.sha256(old_language)==receipt['sha256'],'Verified locale input was changed: '+language)
        original=split.reconstruct(old_common,old_language,language);source=view(original)
        tiles.require(source.cards==art_source.cards and source.clips==art_source.clips and source.aux==art_source.aux,
                      'Final artwork changes model IDs or animation clocks')
        tiles.require(len(source.images)==len(art_source.images) and
                      all(before[1:3]==after[1:3] for before,after in zip(source.images,art_source.images)),
                      'Final artwork changes image IDs or dimensions')
        texts=[source.string_payload(i)[:-1].decode('utf-8') for i in range(len(source.strings))]
        glyphs=glyphs_from(source)
        localized,layout_report=rewrite(art_data,texts,glyphs,language)
        target=view(localized)
        tiles.require(glyphs_from(target)==glyphs,'Rebase changed a glyph key, advance or alpha payload')
        row=split.layout(localized,language)
        candidate=split.common_file(localized[row['prefix']:row['prefix']+row['artwork']])
        if common is None:
            common=candidate;tiles.atomic_bytes(output/'nSpire2.common.tns',common)
        tiles.require(candidate==common,'Rebased locales do not share exact art')
        language_file=split.compress_language_directory(split.language_file(localized,row,language,common[32:64]))
        tiles.require(split.reconstruct(common,language_file,language)==localized,'NSL2 virtual mapping differs after rebase')
        tiles.atomic_bytes(output/f'nSpire2.{language}.tns',language_file)
        if args.keep_logical:tiles.atomic_bytes(output/f'ui11-{language}.nsp',localized)
        reports[language]=dict(receipt,layout=layout_report,bytes=len(language_file),
            sha256=tiles.sha256(language_file),logical_bytes=len(localized),
            rebase=dict(previous_sha256=receipt['sha256'],glyph_payloads_unchanged=True,text_bytes_unchanged=True,
                        source_artwork_sha256=tiles.sha256(art_data)))
        tiles.atomic_bytes(output/f'{language}-receipt.json',json.dumps(reports[language],ensure_ascii=False,indent=2).encode()+b'\n')
        print(json.dumps(dict(language=language,bytes=len(language_file),glyphs=len(glyphs),
                             directory_bytes=layout_report['directory_bytes'])),flush=True)
    tiles.atomic_bytes(output/'locales-receipt.json',json.dumps(dict(schema=1,reference='v0.111.0 / 41cef1ea',
        common_bytes=len(common or b''),optional_language_installation=True,languages=reports),ensure_ascii=False,indent=2).encode()+b'\n')


if __name__=='__main__':main()
