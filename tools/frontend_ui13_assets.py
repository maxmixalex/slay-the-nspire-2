"""Original dropdown scrollbar and localized private port-credit payloads.

The pinned dropdown background/highlight are authored ColorRects, not bitmaps.
Only their three small scrollbar textures need an image-directory append. The
credit translations are port-owned; their source-font raster stays private.
"""
from __future__ import annotations

import json
from pathlib import Path
import unicodedata

from PIL import Image

import content_tiles as tiles
from patch_feedback_assets import write_image
from pck import Pack
from world_scene_assets import load_scene_texture

ROOT = Path(__file__).resolve().parents[1]
BASE_AUX, AUXILIARY, CREDIT_SLOT = 2201, 2204, 8703
LANGUAGES = ('eng', 'zhs', 'zht', 'deu', 'esp', 'fra', 'ind', 'ita',
             'jpn', 'kor', 'pol', 'ptb', 'rus', 'spa', 'tha', 'tur')


def source_assets(pck, cache):
    """Return three append rows; the caller owns the common-pack rewrite."""
    cache = Path(cache)
    cache.mkdir(parents=True, exist_ok=True)
    owned = not hasattr(pck, 'read')
    pack = Pack(pck) if owned else pck
    specs = (
        ('dropdown-track-edge', 'small_scrollbar_track_edge.png', (8, 8)),
        ('dropdown-track-center', 'small_scrollbar_track_center.png', (8, 16)),
        ('dropdown-train', 'small_scrollbar_train.png', (8, 25)),
    )
    try:
        scene = pack.read('scenes/screens/settings_dropdown.tscn').decode()
        tiles.require('patch_margin_top = 20' in scene and 'patch_margin_bottom = 20' in scene,
                      'Pinned dropdown train nine-patch changed')
        rows = []
        for index, (key, name, size) in enumerate(specs):
            path = 'images/packed/common_ui/' + name
            tiles.require(path in scene, 'Pinned dropdown texture changed: ' + path)
            original = load_scene_texture(pack, path).convert('RGBA')
            image = original.resize(size, Image.Resampling.LANCZOS)
            row = write_image(cache, key, image)
            row.update(source=path, auxiliary=BASE_AUX+index,
                       source_size=list(original.size), size=list(size),
                       modulation='runtime source black/half alpha track; gold or quarter alpha train')
            image.save(cache/(key+'.png'))
            rows.append(row)
        return rows
    finally:
        if owned: pack.close()


def credit_text(language):
    table = json.loads((ROOT/'config/port-credits.json').read_text())
    tiles.require(set(table) == set(LANGUAGES), 'Port credits must cover every supported language')
    tiles.require(language in table, 'Unknown credit language')
    rows = table[language]
    tiles.require(len(rows) == 2 and all(isinstance(row, str) and row and '\n' not in row for row in rows),
                  'Port credits require a title and one author line')
    return '\n'.join(rows)


def localize_credits(source, language, font_dir):
    """Return texts/glyphs/report, preserving every unrelated existing payload.

    The optional languages already contain source-shaped graphemes. New credit
    clusters therefore allocate after the last occupied private-use codepoint;
    neither the existing strings nor their glyph identities are rebased.
    """
    import regex
    from build_locale_editions import face_set, glyph_for

    texts = [source.string_payload(i)[:-1].decode('utf-8') for i in range(len(source.strings))]
    tiles.require(len(texts) > CREDIT_SLOT, 'Missing reserved port-credit string slot')
    glyphs = [(cp, band, w, h, advance, encoded & ~tiles.GLYPH_GROUP_FLAG,
               bytes(source.glyph_payload(index)))
              for index, (cp, _offset, band, w, h, advance, encoded) in enumerate(source.glyphs)]
    source_text = credit_text(language)
    current = texts[CREDIT_SLOT]
    occupied = {row[0] for row in glyphs}
    next_private = max((cp for cp in occupied if 0xf0000 <= cp < 0xffffd), default=0xeffff)+1
    shaped = {}
    def cluster(match):
        nonlocal next_private
        value = unicodedata.normalize('NFC', match[0])
        if len(value) == 1 or not any(unicodedata.category(ch).startswith('M') or ch == '\u0e33' for ch in value):
            return value
        if value not in shaped:
            tiles.require(next_private < 0xffffd, 'No private grapheme slot remains')
            shaped[value] = next_private
            next_private += 1
        return chr(shaped[value])
    rendered = regex.sub(r'\X', cluster, source_text)
    # An already localized non-clustered edition is byte-stable on rerun.
    # Callers use the preserved UI12 input for Thai, avoiding orphan clusters.
    texts[CREDIT_SLOT] = rendered
    fonts, fallback = face_set(Path(font_dir), language)
    clusters = {cp: value for value, cp in shaped.items()}
    existing = {(row[0], row[1]) for row in glyphs}
    added = []
    for cp in sorted(set(map(ord, rendered)) - {ord('\n')}):
        text = clusters.get(cp, chr(cp))
        codepoints = set(map(ord, text))
        face = next((candidate for candidate in [fonts[0], *fallback]
                     if codepoints <= candidate.coverage), None)
        tiles.require(face is not None, 'Source font chain lacks port-credit glyph: '+repr(text))
        for size in (8, 10, 12):
            if (cp, size) in existing: continue
            glyphs.append(glyph_for(face, cp, text, size, 0))
            added.append([cp, size])
    glyphs.sort(key=lambda row: (row[1], row[0]))
    tiles.require(len(glyphs) <= 65535, 'Port credits exceed the bounded glyph directory')
    return texts, glyphs, dict(language=language, slot=CREDIT_SLOT,
        source_text=source_text, added_glyphs=added, shaped_graphemes=shaped,
        preserved_existing_glyphs=len(source.glyphs),
        unchanged_previous_text=current == rendered)
