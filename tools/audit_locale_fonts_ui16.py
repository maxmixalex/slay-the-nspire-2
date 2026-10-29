#!/usr/bin/env python3
"""Check exact native glyph bands for the source label roles corrected in UI16.

Read-only private-asset gate. Missing glyphs are reported by codepoint, never
silently accepted through runtime '?' or neighbouring-size fallback. Original
strings, fonts and resource files are not copied into the public repository.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

import content_tiles as tiles
import split_content as split
from replace_original_glyphs import visible_alphabet, text_format_control
from scene_ui9_fonts import text_index
from ui12_resources import view

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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--common', required=True, type=Path)
    parser.add_argument('--editions', type=Path, action='append', required=True)
    parser.add_argument('--languages', default='all')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    common = args.common.read_bytes()
    codes = list(split.LANGUAGES) if args.languages == 'all' else args.languages.split(',')
    tiles.require(bool(codes) and set(codes) <= set(split.LANGUAGES), 'Invalid locale list')
    report = {}
    for code in codes:
        matches = [root/f'nSpire2.{code}.dat.tns' for root in args.editions]
        path = next((path for path in matches if path.is_file()), None)
        tiles.require(path is not None, 'Missing private language '+code)
        report[code] = audit(split.reconstruct(common, path.read_bytes(), code))
        print(json.dumps(report[code]), flush=True)
    if args.report:
        path = tiles.private_path(args.report, 'Private font-audit report')
        tiles.atomic_bytes(path, json.dumps(report, indent=2).encode()+b'\n')
    tiles.require(not any(row['missing'] for row in report.values()), 'Missing exact UI16 label glyphs')


if __name__ == '__main__':
    main()
