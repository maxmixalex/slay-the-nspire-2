#!/usr/bin/env python3
"""Append original selector art, prompts and exact-size font glyphs privately.

The highlight data image contains two little-endian words per logical pixel:
UNORM16 original SDF alpha followed by original RGB565. It is a shader input,
not an ordinary color image. The original half-float texture is sampled at
projected texel centers; existing artwork and glyphs are never resampled.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import struct

import numpy as np
from PIL import Image

import content_tiles as tiles
from append_ui9_tooltip_glyphs import verify_append
from lz4_block import encode as lz4_encode
from patch_feedback_assets import SOURCE_PCK_SHA256, write_image
from patch_frontend_assets import patch_pack
from patch_ui7_assets import append_glyphs
from pck import Pack
from replace_original_glyphs import Face, text_format_control, visible_alphabet
from world_scene_assets import load_scene_texture

BASE_AUX, AUXILIARY, VERSION, CLIPS, STRINGS = 2058, 2061, 33, 1765, 8704
HAND_CARDS = ('PURITY', 'THINKING_AHEAD', 'GLIMMER', 'PHOTON_CUT', 'HAND_TRICK',
              'NIGHTMARE', 'GUARDS', 'DECISIONS_DECISIONS', 'HEIRLOOM_HAMMER',
              'SCULPTING_STRIKE', 'SNAP', 'TRANSFIGURE', 'DUAL_WIELD')
PILE_CARDS = ('HEADBUTT', 'SECRET_TECHNIQUE', 'SECRET_WEAPON', 'SEEKER_STRIKE',
              'HOLOGRAM', 'COSMIC_INDIFFERENCE', 'DREDGE', 'GRAVEBLAST',
              'NEOWS_FURY', 'WISH')
HAND_KEYS = tuple('cards.'+key+'.selectionScreenPrompt' for key in HAND_CARDS) + (
    'relics.GAMBLING_CHIP.selectionScreenPrompt',
    'potions.GAMBLERS_BREW.selectionScreenPrompt',
    'potions.ASHWATER.selectionScreenPrompt',
    'potions.TOUCH_OF_INSANITY.selectionScreenPrompt')
PILE_KEYS = tuple('cards.'+key+'.selectionScreenPrompt' for key in PILE_CARDS) + (
    'powers.STRATAGEM_POWER.selectionScreenPrompt',
    'powers.FOREGONE_CONCLUSION_POWER.selectionScreenPrompt',
    'potions.DROPLET_OF_PRECOGNITION.selectionScreenPrompt',
    'potions.LIQUID_MEMORIES.selectionScreenPrompt',
    'relics.CHOICES_PARADOX.selectionScreenPrompt')
WORLD_KEYS = ('card_selection.TO_ENCHANT', 'relics.DOLLYS_MIRROR.selectionScreenPrompt',
              'relics.CLAWS.selectionScreenPrompt', 'relics.SEA_GLASS.selectionScreenPrompt',
              'events.BRAIN_LEECH.pages.SHARE_KNOWLEDGE.selectionScreenPrompt',
              'events.ROOM_FULL_OF_CHEESE.pages.GORGE.selectionScreenPrompt',
              'gameplay_ui.COMBAT_REWARD_CARD_REMOVAL.selectionScreenPrompt',
              'modifiers.SEALED_DECK.selectionPrompt')
TEXT_KEYS = tuple(sorted(HAND_KEYS+PILE_KEYS))
# These are the remaining empty slots, not a new larger string directory.
TEXT_SLOTS = tuple(range(8378, 8400))+tuple(range(8689, 8699))
TEXTS = tuple(zip(TEXT_SLOTS, TEXT_KEYS))
assert len(TEXTS) == len(TEXT_KEYS) == 32
SDF_SOURCE = 'images/packed/card_template/card_frame_sdf.exr'
SDF_IMPORTED = '.godot/imported/card_frame_sdf.exr-16e8461a702928a31749c8fd6baeda12.ctex'
SDF_SIZE = (145, 181)


def half_texture(data):
    """Read the pinned RGBAH GST2 texture, rejecting another codec/mip layout."""
    tiles.require(len(data) == 52+512*512*8 and data[:4] == b'GST2',
                  'Unexpected original SDF texture bounds')
    tiles.require(struct.unpack_from('<I', data, 4)[0] == 1 and
                  struct.unpack_from('<II', data, 8) == (512, 512) and
                  struct.unpack_from('<HHII', data, 40) == (512, 512, 0, 15),
                  'Original SDF must be the pinned single-level RGBAH texture')
    rgba = np.frombuffer(data, dtype='<f2', offset=52).reshape(512, 512, 4).astype(np.float64)
    tiles.require(np.isfinite(rgba).all() and rgba.min() >= 0 and rgba.max() <= 1,
                  'Original SDF contains unsupported nonfinite/HDR values')
    return rgba


def project_shader_input(rgba, size=SDF_SIZE):
    """Bilinear texture sampling with edge clamp, before output quantization."""
    tiles.require(rgba.ndim == 3 and rgba.shape[2] == 4 and
                  np.isfinite(rgba).all() and rgba.min() >= 0 and rgba.max() <= 1,
                  'Invalid shader input')
    width, height = size
    tiles.require(0 < width <= 160 and 0 < height <= 240, 'Projected SDF exceeds the image bounds')
    source_h, source_w, _ = rgba.shape
    xs = np.maximum(0, np.minimum(source_w-1, (np.arange(width)+.5)*source_w/width-.5))
    ys = np.maximum(0, np.minimum(source_h-1, (np.arange(height)+.5)*source_h/height-.5))
    x0, y0 = np.floor(xs).astype(int), np.floor(ys).astype(int)
    x1, y1 = np.minimum(x0+1, source_w-1), np.minimum(y0+1, source_h-1)
    fx, fy = (xs-x0)[None, :, None], (ys-y0)[:, None, None]
    top = rgba[y0[:, None], x0[None, :]]*(1-fx)+rgba[y0[:, None], x1[None, :]]*fx
    bottom = rgba[y1[:, None], x0[None, :]]*(1-fx)+rgba[y1[:, None], x1[None, :]]*fx
    pixels = top*(1-fy)+bottom*fy
    words = np.empty((height, width, 2), dtype='<u2')
    words[:, :, 0] = np.rint(pixels[:, :, 3]*65535).astype(np.uint16)
    rgb = np.rint(pixels[:, :, :3]*np.array((31, 63, 31))).astype(np.uint16)
    words[:, :, 1] = (rgb[:, :, 0] << 11) | (rgb[:, :, 1] << 5) | rgb[:, :, 2]
    return words.tobytes()


def write_shader_input(cache, raw):
    width, height = SDF_SIZE
    tiles.require(len(raw) == width*height*4, 'Wrong projected SDF byte count')
    compressed = struct.pack('<BI', 0, len(raw))+lz4_encode(raw)
    payload, fmt = (compressed, 5) if len(compressed) < len(raw) else (raw, 0)
    row = dict(width=width*2, height=height, format=fmt, decoded_format=0,
               logical_size=[width, height], raw_sha256=tiles.sha256(raw),
               file='selection-highlight-data.bin', sha256=tiles.sha256(payload), bytes=len(payload),
               source=SDF_SOURCE, source_size=[512, 512], auxiliary=BASE_AUX+1,
               word_layout=['alpha_unorm16', 'rgb565'],
               sampling='bilinear at projected texel centers, clamp-to-edge')
    tiles.atomic_bytes(cache/row['file'], payload)
    return row


def build_art(pck_path, cache):
    cache.mkdir(parents=True, exist_ok=True)
    pack = Pack(pck_path)
    try:
        scene = pack.read('scenes/cards/upgrade_preview.tscn').decode()
        tiles.require(all(value in scene for value in ('upgrade_arrow.png',
                      'offset_left = -101.5', 'offset_right = 101.5',
                      'offset_top = -32.5', 'offset_bottom = 32.5')),
                      'Pinned upgrade arrow layout changed')
        source = 'images/ui/cards/upgrade_preview/upgrade_arrow.png'
        original = load_scene_texture(pack, source).convert('RGBA')
        # The HBox's three65px children do not have Expand. Its spare8px
        # remain at the trailing edge, rather than widening each arrow.
        arrow = original.resize((12, 12), Image.Resampling.LANCZOS)
        row = write_image(cache, 'selection-upgrade-arrow', arrow)
        row.update(source=source, source_size=list(original.size), size=[12, 12], auxiliary=BASE_AUX)
        sdf = half_texture(pack.read(SDF_IMPORTED))
        rows = [row, write_shader_input(cache, project_shader_input(sdf))]
        frame_source = 'images/packed/inspect_relic_screen/relic_inspect_frame.png'
        original_frame = load_scene_texture(pack, frame_source).convert('RGBA')
        tiles.require(original_frame.size == (304, 304), 'Original relic frame geometry changed')
        frame = write_image(cache, 'relic-inspect-frame', original_frame.resize((58, 58), Image.Resampling.LANCZOS))
        frame.update(source=frame_source, source_size=[304, 304], size=[58, 58], auxiliary=BASE_AUX+2)
        rows.append(frame)
        localized = {}
        for language in ('eng', 'zhs'):
            tables = {name:json.loads(pack.read('localization/'+language+'/'+name+'.json'))
                      for name in ('cards', 'potions', 'powers', 'relics')}
            localized[language] = {}
            for _, qualified in TEXTS:
                table, key = qualified.split('.', 1)
                value = tables[table].get(key)
                tiles.require(isinstance(value, str) and value and '\0' not in value,
                              'Missing original selection prompt: '+language+' / '+qualified)
                localized[language][qualified] = value
    finally:
        pack.close()
    manifest = dict(schema=1, reference='v0.111.0 / 41cef1ea',
                    reference_sha256=SOURCE_PCK_SHA256, output_version=VERSION,
                    images=rows, clips=[], localized_text=localized,
                    runtime_added_pixel_buffers=0)
    tiles.atomic_bytes(cache/'selection-assets.json', json.dumps(manifest, indent=2).encode()+b'\n')
    return manifest


def append(data, manifest, cache):
    source = tiles.PackView(data, BASE_AUX, versions=(VERSION,),
                           profile=(558, STRINGS, CLIPS, BASE_AUX))
    language = source.string_payload(6110)[:-1].decode('ascii')
    for slot, _ in TEXTS:
        tiles.require(source.string_payload(slot) == b'\0', 'Selector slot is already occupied')
    result, receipt = patch_pack(data, manifest, cache, base_aux=BASE_AUX,
        auxiliary=AUXILIARY, input_version=VERSION, version=VERSION,
        texts=TEXTS, base_clips=CLIPS, base_strings=STRINGS)
    target = tiles.PackView(result, AUXILIARY, versions=(VERSION,),
                           profile=(558, STRINGS, CLIPS, AUXILIARY))
    changed = set(TEXT_SLOTS)
    for i in range(STRINGS):
        if i not in changed:
            tiles.require(source.string_payload(i) == target.string_payload(i),
                          'A preexisting selector-independent string changed')
    for slot, key in TEXTS:
        tiles.require(target.string_payload(slot) ==
            manifest['localized_text'][language][key].encode()+b'\0', 'Selector prompt readback differs')
    receipt.update(original_text_keys=len(TEXTS), old_payloads_and_indices_exact=True,
                   resident_cache_growth_bytes=0)
    return result, receipt


def source_text_index():
    root = Path(__file__).resolve().parents[1]/'src'
    result = {}
    for name in ('world_text_index.inc', 'locale_text_index.inc', 'native_ui_text_index.inc',
                 'run_modes_text_index.inc'):
        result.update((key, int(slot)) for key, slot in
                      re.findall(r'\{"([^"]+)",\s*(\d+)\}', (root/name).read_text()))
    result.update((key, slot) for slot, key in TEXTS)
    return result


def glyph_requirements(source):
    index, required = source_text_index(), set()
    def add(keys, sizes, faces):
        for key in keys:
            tiles.require(key in index, 'Unbound original selector key: '+key)
            text = source.string_payload(index[key])[:-1].decode('utf-8')
            tiles.require(bool(text), 'Empty original selector key: '+key)
            alphabet = visible_alphabet(text) | set(map(ord, '0123456789'))
            required.update((cp, size, face) for cp in alphabet for size in sizes for face in faces)
    add(HAND_KEYS, (6,), (5,))
    add(PILE_KEYS, (5,), (0, 5))
    add(WORLD_KEYS, (5,), (0, 5))
    add(('gameplay_ui.CHOOSE_A_PACK',), (6,), (5,))
    # The source enchant selector shows the currently applied enchantment
    # beside its comparison. Include every retained original title/body,
    # not just the particular ancient encountered by an acceptance seed.
    add(tuple(key for key in index if key.startswith('enchantments.') and key.endswith('.title')), (5,), (5,))
    add(tuple(key for key in index if key.startswith('enchantments.') and key.endswith('.description')), (5,), (0, 5))
    add(('card_selection.TO_DISCARD', 'card_selection.TO_EXHAUST', 'card_selection.TO_TRANSFORM'), (6,), (5,))
    add(('card_selection.TO_EXHAUST', 'card_selection.TO_UPGRADE', 'card_selection.TO_REMOVE',
         'card_selection.TO_TRANSFORM'), (5,), (0, 5))
    add(('card_selection.VIEW_UPGRADES',), (4,), (5,))
    add(('gameplay_ui.CHOOSE_CARD_UPGRADE_HEADER', 'gameplay_ui.CHOOSE_CARD_HEADER'), (6,), (5,))
    add(('gameplay_ui.CHOOSE_CARD_SKIP_BUTTON', 'gameplay_ui.TO_INSPECT_PROMPT'), (5, 6), (5,))
    add(tuple('gameplay_ui.PAUSE_MENU.'+key for key in
              ('RESUME', 'SETTINGS', 'COMPENDIUM', 'GIVE_UP', 'SAVE_AND_QUIT')), (5, 6), (5,))
    add(('gameplay_ui.PAUSE_MENU.PAUSED',), (12,), (4,))
    add(tuple('gameplay_ui.POTION_POPUP.'+key for key in ('discard', 'drink', 'throw')), (5, 6), (5,))
    add(tuple(key for key in index if key.startswith('relics.') and key.endswith('.title'))+
        ('inspect_relic_screen.LOCKED_TITLE', 'inspect_relic_screen.UNDISCOVERED_TITLE'), (7,), (5,))
    add(tuple(key for key in index if key.startswith('relics.') and key.endswith('.description'))+
        ('inspect_relic_screen.LOCKED_DESCRIPTION', 'inspect_relic_screen.UNDISCOVERED_DESCRIPTION'), (5,), (0, 5))
    add(tuple(key for key in index if key.startswith('relics.') and key.endswith('.flavor')), (4,), (6,))
    return {(cp, px, face) for cp, px, face in required if cp >= 32 and not text_format_control(cp)}


def append_fonts(data, regular, bold, display, italic):
    source = tiles.PackView(data, AUXILIARY, versions=(VERSION,))
    known = {(row[0], row[2]) for row in source.glyphs}
    rows = []
    for cp, px, style in sorted(glyph_requirements(source)):
        packed = px | ((style & 3) << 6) | ((style & 4) << 3)
        if (cp, packed) in known:
            continue
        face = italic if style == 6 else display if style == 4 else bold if style == 5 else regular
        tiles.require(cp in face.coverage, 'Original selector font lacks codepoint '+str(cp))
        rows.append(face.glyph(cp, px, style))
    out, count = append_glyphs(data, AUXILIARY, CLIPS, rows, version=VERSION)
    target = tiles.PackView(out, AUXILIARY, versions=(VERSION,))
    tiles.require(len(target.glyphs) <= 32768, 'Selector glyph directory exceeds its existing bound')
    proof = verify_append(data, out, AUXILIARY, VERSION)
    actual = {(row[0], row[2]) for row in target.glyphs}
    for cp, px, style in glyph_requirements(target):
        tiles.require((cp, px | ((style & 3) << 6) | ((style & 4) << 3)) in actual,
                      'Original selector glyph coverage incomplete')
    return bytes(out), dict(added_glyphs=count, glyphs=len(target.glyphs),
        regular_font=regular.metadata(), bold_font=bold.metadata(), display_font=display.metadata(), italic_font=italic.metadata(),
        original_glyphs_exact=proof, missing_required_glyphs=0)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('input-eng', 'input-zhs', 'output-dir', 'font-dir'):
        parser.add_argument('--'+name, required=True, type=Path)
    parser.add_argument('--pck', type=Path)
    parser.add_argument('--cache-dir', type=Path)
    parser.add_argument('--fonts-only', action='store_true',
                        help='append missing glyphs to an existing v33/2061 pair without rebuilding art')
    args = parser.parse_args()
    output = tiles.private_path(args.output_dir, 'Private selector output')
    fonts = tiles.private_path(args.font_dir, 'Original font directory')
    if not args.fonts_only:
        if not args.pck or not args.cache_dir:
            parser.error('--pck and --cache-dir are required for the original artwork append')
        cache = tiles.private_path(args.cache_dir, 'Private selector cache')
        manifest = build_art(tiles.private_path(args.pck, 'Pinned PCK'), cache)
    reports = {}
    for language, names in (('eng', ('kreon_regular.ttf', 'kreon_bold.ttf', 'supplement/spectral_bold.ttf', 'supplement/bitter_medium_italic.ttf')),
                            ('zhs', ('NotoSansMonoCJKsc-Regular.otf', 'SourceHanSerifSC-Bold.otf', 'SourceHanSerifSC-Bold.otf', 'SourceHanSerifSC-Medium.otf'))):
        path = tiles.private_path(getattr(args, 'input_'+language), 'Previous private content')
        destination = output/('ui9-'+language+'.nsp')
        tiles.require(path.resolve() != destination.resolve(), 'Preserve the previous pack')
        if args.fonts_only:
            data = path.read_bytes()
            receipt = dict(font_patch_only=True, input_bytes=len(data), input_sha256=tiles.sha256(data))
        else:
            data, receipt = append(path.read_bytes(), manifest, cache)
        out, font_receipt = append_fonts(data, *(Face(fonts/name) for name in names))
        receipt.update(fonts=font_receipt, final_bytes=len(out), final_sha256=tiles.sha256(out))
        tiles.atomic_bytes(destination, out)
        reports[language] = receipt
    tiles.atomic_bytes(output/'selection-conversion.json', json.dumps(reports, indent=2).encode()+b'\n')
    print(json.dumps(reports), flush=True)


if __name__ == '__main__':
    main()
