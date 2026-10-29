#!/usr/bin/env python3
"""Check exact native glyph bands for the source label roles corrected in UI16.

Missing glyphs are reported by codepoint, never
silently accepted through runtime '?' or neighbouring-size fallback. Original
strings, fonts and resource files are not copied into the public repository.
An explicit repair-output adds only absent source glyphs, preserving the whole
old payload suffix, font cohorts, artwork and strings without recompression.
"""
from __future__ import annotations

import argparse
import json
import math
from pathlib import Path

from PIL import Image, ImageDraw
import content_tiles as tiles
import split_content as split
from build_locale_editions import FONT_NAMES, glyph_for
from card_library_fonts import rewrite_directory, verify_preserved
from replace_original_glyphs import Face, visible_alphabet, text_format_control
from scene_ui9_fonts import text_index
from ui12_resources import view, rle

SUBSTITUTED = frozenset(('zhs', 'zht', 'jpn', 'kor', 'pol', 'rus', 'tha'))
LABELS = {
    'card_selection.VIEW_UPGRADES': (8,),
    'gameplay_ui.PROCEED_BUTTON': (5, 6, 8, 10),
    'gameplay_ui.END_TURN_BUTTON': (10,),
    'gameplay_ui.CHOOSE_A_PACK': (8, 10),
    'gameplay_ui.CHOOSE_CARD_HEADER': (8, 10),
    'gameplay_ui.CHOOSE_CARD_SKIP_BUTTON': (8,),
    'gameplay_ui.TO_INSPECT_PROMPT': (8,),
    'gameplay_ui.POTION_POPUP.throw': (8,),
    'gameplay_ui.POTION_POPUP.drink': (8,),
    'gameplay_ui.POTION_POPUP.discard': (8,),
    'gameplay_ui.SORT_OBTAINED': (8,),
    'gameplay_ui.SORT_TYPE': (8,),
    'gameplay_ui.SORT_COST': (8,),
    'gameplay_ui.SORT_ALPHABET': (8,),
    'run_history.MAP_POINT_HISTORY.header': (12,),
    'run_history.HISTORY_ENTRY.rewardsHeader': (10,),
    'run_history.HISTORY_ENTRY.skippedHeader': (10,),
}


def requirements(source):
    code = source.string_payload(split.MARKER)[:-1].decode('ascii')
    normal = 0 if code in SUBSTITUTED else 5
    index, result = text_index(), set()

    def add(key, sizes, face):
        tiles.require(key in index, 'Unbound UI16 font label: '+key)
        text = source.string_payload(index[key])[:-1].decode('utf-8')
        tiles.require(bool(text), 'Empty UI16 font label: '+key)
        result.update((cp, px, face) for cp in visible_alphabet(text) for px in sizes)

    for key, sizes in LABELS.items():
        add(key, sizes, normal)
    for key in index:
        if key.endswith('.selectionScreenPrompt'):
            add(key, (8,), normal)
        elif key.startswith('ancients.') and key.endswith('.next'):
            add(key, (10,), 0 if code in SUBSTITUTED else 6)
    result.update((ord(ch), px, normal) for ch in '0123456789?X+-' for px in (8, 10, 12))
    # Ordinary labels use Regular in substituted locales; [b]/[i] glyphs in
    # rich descriptions remain separate, as MegaRichTextLabel specifies.
    return {(cp, px, face) for cp, px, face in result
            if not text_format_control(cp) and cp not in (0xe000, 0xe001)}


def audit(data):
    source = view(data)
    code = source.string_payload(split.MARKER)[:-1].decode('ascii')
    required = requirements(source)
    known = {(row[0], row[2]) for row in source.glyphs}
    missing = sorted((cp, px, face) for cp, px, face in required
        if (cp, px | ((face & 3) << 6) | ((face & 4) << 3)) not in known)
    return dict(language=code, ordinary_face=0 if code in SUBSTITUTED else 5,
                required_glyphs=len(required), missing=missing, added_storage_bytes=0)


def repair(data, fonts):
    """Append exact missing rasters; no prior glyph is decoded/re-encoded."""
    source, before = view(data), audit(data)
    if not before['missing']:
        return data, before
    code = before['language'];names = FONT_NAMES.get(code, FONT_NAMES['latin'])
    loaded, additions = {}, []
    for cp, px, style in before['missing']:
        name = names[1 if style in (1, 3, 5) else 2 if style in (2, 6) else 3 if style == 4 else 0]
        if name not in loaded:
            loaded[name] = Face(fonts/name)
        font = loaded[name]
        tiles.require(cp in font.coverage, 'Missing exact source font for codepoint '+str(cp))
        row = glyph_for(font, cp, chr(cp), px, style)
        if 32 <= cp < 0x530:
            face = font.fonts[px];advance = round(face.getlength(chr(cp))*64)
            left = min(0, face.getbbox(chr(cp))[0])
            width = max(1, math.ceil(advance/64), face.getbbox(chr(cp))[2])-left
            tiles.require(-7 <= left <= 0 and advance <= 2560, 'Source font metrics exceed UI40')
            mask = Image.new('L', (width, px+5))
            ImageDraw.Draw(mask).text((-left, px+1), chr(cp), font=face, fill=255, anchor='ls')
            raw, encoded = rle(mask.tobytes())
            row = (cp, row[1], width, px+5, 0x8000|((-left)<<12)|advance, encoded, raw)
        additions.append(row)
    output = rewrite_directory(data, len(source.aux), set(), additions, output_version=source.version)
    proof = verify_preserved(data, output, len(source.aux), set())
    after = audit(output)
    tiles.require(not after['missing'], 'Repair did not add every exact glyph')
    return output, dict(after, added_glyphs=before['missing'],
        added_storage_bytes=len(output)-len(data), verification=proof)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--common', required=True, type=Path)
    parser.add_argument('--editions', type=Path, action='append', required=True)
    parser.add_argument('--languages', default='all')
    parser.add_argument('--report', type=Path)
    parser.add_argument('--fonts', type=Path)
    parser.add_argument('--repair-output', type=Path)
    args = parser.parse_args()
    common = args.common.read_bytes()
    codes = list(split.LANGUAGES) if args.languages == 'all' else args.languages.split(',')
    tiles.require(bool(codes) and set(codes) <= set(split.LANGUAGES), 'Invalid locale list')
    if args.repair_output:
        tiles.require(args.fonts is not None, 'Source fonts are required for repairs')
        tiles.private_path(args.repair_output, 'Private font repair output').mkdir(parents=True, exist_ok=True)
    report = {}
    for code in codes:
        matches = [root/f'nSpire2.{code}.dat.tns' for root in args.editions]
        path = next((path for path in matches if path.is_file()), None)
        tiles.require(path is not None, 'Missing private language '+code)
        logical = split.reconstruct(common, path.read_bytes(), code)
        report[code] = audit(logical)
        if args.repair_output and report[code]['missing']:
            logical, report[code] = repair(logical, args.fonts)
            tiles.atomic_bytes(args.repair_output/(code+'.nsp'), logical)
            bounds = split.layout(logical, code)
            localized = split.compress_language_directory(split.language_file(logical, bounds, code, common[32:64]))
            tiles.atomic_bytes(args.repair_output/path.name, localized)
        print(json.dumps(report[code]), flush=True)
    if args.report:
        path = tiles.private_path(args.report, 'Private font-audit report')
        tiles.atomic_bytes(path, json.dumps(report, indent=2).encode()+b'\n')
    tiles.require(not any(row['missing'] for row in report.values()), 'Missing exact UI16 label glyphs')


if __name__ == '__main__':
    main()
