#!/usr/bin/env python3
"""Replace private UI7 font payloads without re-encoding any artwork.

Font files are caller-supplied private inputs. Each optional face is an actual
font, never an offset duplicate of the regular glyph. A missing face or an
uncovered codepoint falls back to the existing regular-face contract. This
tool does not infer that one English scene's italic face applies to all UI.
"""
from __future__ import annotations

import argparse
from collections import Counter
import json
import math
from pathlib import Path
import re
import struct

from PIL import Image, ImageDraw, ImageFont
from fontTools.ttLib import TTFont

import content_tiles as tiles
from split_content import layout

SIZES = (8, 10, 12, 16, 20)
SCENE_SIZES = (6, 8, 10, 12, 14, 16, 20)
FACE_NAMES = ('regular', 'bold', 'italic', 'bold_italic')
KNOWN_TAGS = frozenset(('gold', 'red', 'blue', 'green', 'purple', 'orange',
    'aqua', 'white', 'gray', 'b', 'i', 'sine', 'jitter', 'rainbow', 'thinky_dots'))
TAG = re.compile(r'\[(/?)([a-z_]+)(?:\s+[^\]]*)?\]')


def text_format_control(cp):
    """Match unicode_text.hpp without changing any stored localization byte."""
    return (0x200b <= cp <= 0x200f or 0x202a <= cp <= 0x202e or
            0x2060 <= cp <= 0x206f or cp == 0xfeff or
            0xfe00 <= cp <= 0xfe0f or 0xe0100 <= cp <= 0xe01ef)


def visible_alphabet(text):
    return {ord(ch) for ch in text if ord(ch) >= 32 and not text_format_control(ord(ch))}


def auxiliary_body_slots(source):
    """Full notes/credits are rendered only at 8/10px by their own reader.

    Keep every codepoint and rich-text face, without baking unused large and
    six-pixel copies of an alphabet introduced solely by historical articles.
    """
    if getattr(source, 'version', 0) < 28: return set()
    from auxiliary_ui8_assets import TEXTS
    return {slot for slot, key in TEXTS
            if key == 'auxiliary.CREDITS' or key.startswith('patch_notes.')}


def styled_alphabet(texts):
    """Collect actual rich-text characters, including nested b/i regions.

    Dynamic numeric/name substitutions also need ASCII. Non-ASCII replacement
    strings not present in the collected spans retain the regular fallback;
    the converter never silently fills a missing face with tofu glyphs.
    """
    result = [set() for _ in FACE_NAMES]
    for text in texts:
        stack, face, at = [], 0, 0
        for match in TAG.finditer(text):
            if match[2] not in KNOWN_TAGS:
                continue
            result[face].update(visible_alphabet(text[at:match.start()]))
            closing, tag = match[1], match[2]
            if closing:
                for i in range(len(stack)-1, -1, -1):
                    if stack[i][0] == tag:
                        face = stack[i][1]
                        del stack[i:]
                        break
            elif len(stack) < 16:
                stack.append((tag, face))
                if tag == 'b': face |= 1
                if tag == 'i': face |= 2
            at = match.end()
        result[face].update(visible_alphabet(text[at:]))
    for face in range(1, 4):
        if result[face]: result[face].update(range(32, 127))
    return result


class Face:
    def __init__(self, path):
        self.path = Path(path)
        with TTFont(self.path, lazy=True) as font:
            self.coverage = set(font.getBestCmap() or ())
            self.family = font['name'].getDebugName(1)
            self.subfamily = font['name'].getDebugName(2)
        self.fonts = {px: ImageFont.truetype(str(self.path), px) for px in SCENE_SIZES}

    def glyph(self, cp, px, style=0):
        tiles.require(cp in self.coverage, 'Font does not cover requested glyph')
        font, char = self.fonts[px], chr(cp)
        advance = max(1, math.ceil(font.getlength(char)))
        width, height = max(advance, font.getbbox(char)[2]), px + 5
        tiles.require(0 < width <= 40 and advance <= 40, 'Glyph outside runtime bound')
        image = Image.new('L', (width, height))
        ImageDraw.Draw(image).text((0, px+1), char, font=font, fill=255, anchor='ls')
        raw, encoded = image.tobytes(), bytearray()
        at = 0
        while at < len(raw):
            end = at+1
            while end < len(raw) and raw[end] == raw[at] and end-at < 255: end += 1
            encoded.extend((end-at, raw[at])); at = end
        payload = bytes(encoded) if len(encoded) < len(raw) else raw
        return (cp, px | ((style & 3) << 6) | ((style & 4) << 3), width, height, advance,
                len(payload) if payload is not raw else 0, payload)

    def metadata(self):
        return dict(file=self.path.name, family=self.family, style=self.subfamily,
                    sha256=tiles.sha256(self.path.read_bytes()), cmap_count=len(self.coverage))


def generate(source, faces, fallback=None):
    texts = [bytes(source.block(off, length)).decode('utf-8') for off, length in source.strings]
    requested = styled_alphabet(texts)
    auxiliary = auxiliary_body_slots(source)
    core_requested = styled_alphabet([value for slot, value in enumerate(texts) if slot not in auxiliary])
    core_required = set().union(*core_requested)
    rows, retained, substituted = [], [], []
    primary = faces[0]
    tiles.require(primary is not None, 'A regular font is required')
    fallbacks = list(fallback) if isinstance(fallback, (tuple, list)) else [fallback] if fallback else []
    def regular_face(cp):
        return next((font for font in [primary, *fallbacks] if cp in font.coverage), None)
    fallback_sources = {font.path.name if hasattr(font, 'path') else str(i): set() for i, font in enumerate(fallbacks)}
    def record_fallback(cp, font):
        if font is not primary:
            substituted.append(cp)
            index = fallbacks.index(font)
            fallback_sources[font.path.name if hasattr(font, 'path') else str(index)].add(cp)
    originals = {(row[0], row[2]): row for row in source.glyphs if row[2] < 32}
    previous_codepoints = {cp for cp, _px in originals}
    required = set().union(*requested)
    new_codepoints = required - {cp for cp, _px in originals}
    missing_regular = []
    # Every old regular codepoint/size remains addressable, including runtime
    # literals absent from localization and Unicode input hints.
    for (cp, px), row in originals.items():
        if text_format_control(cp):
            rows.append((cp, px, 1, 1, 0, 0, b'\0'))
            continue
        face = regular_face(cp)
        if face is not None:
            rows.append(face.glyph(cp, px))
            record_fallback(cp, face)
        else:
            _cp, off, _px, w, h, advance, encoded = row
            rows.append((cp, px, w, h, advance, encoded,
                         bytes(source.block(off, encoded or w*h))))
            retained.append(cp)
    # UI append packs deliberately retain their old glyph payload. Complete
    # the alphabet from every localized span before choosing styled variants;
    # otherwise a new modifier/tooltip may silently render '?' in Chinese.
    for cp in sorted(new_codepoints):
        face = regular_face(cp)
        if face is None:
            missing_regular.append(cp)
            continue
        for px in SIZES if cp in core_required else (8, 10):
            row = face.glyph(cp, px)
            rows.append(row)
            originals[(cp, px)] = None
        record_fallback(cp, face)
    if getattr(source, 'version', 0) >= 28:
        for cp in sorted({cp for cp, _px in originals}):
            if (cp, 6) in originals: continue
            if cp not in previous_codepoints and cp not in core_required: continue
            if text_format_control(cp): continue
            face = regular_face(cp)
            if face is not None:
                rows.append(face.glyph(cp, 6)); originals[(cp, 6)] = None
    styled_counts, unavailable = {}, {}
    for style in range(1, 4):
        font = faces[style]
        eligible = requested[style] & {cp for cp, _px in originals}
        covered = eligible & font.coverage if font is not None else set()
        for cp, px in originals:
            if cp in covered and (cp in core_requested[style] or px in (8, 10)):
                rows.append(font.glyph(cp, px, style))
        styled_counts[FACE_NAMES[style]] = len(covered)
        unavailable[FACE_NAMES[style]] = sorted(eligible-covered)
    rows.sort(key=lambda row: (row[1], row[0]))
    return rows, dict(fonts={FACE_NAMES[i]: face.metadata() for i, face in enumerate(faces) if face},
        fallback_font=fallbacks[0].metadata() if fallbacks else None,
        fallback_fonts=[font.metadata() for font in fallbacks],
        fallback_codepoints_by_font={key: sorted(value) for key, value in fallback_sources.items()},
        zero_width_format_controls=sorted({ord(ch) for text in texts for ch in text if text_format_control(ord(ch))}),
        regular_codepoints=len({cp for cp, _ in originals}),
        appended_regular_codepoints=sorted(new_codepoints-set(missing_regular)),
        missing_regular_codepoints=missing_regular,
        styled_codepoints=styled_counts, missing_style_codepoints=unavailable,
        retained_old_regular_codepoints=sorted(set(retained)),
        fallback_regular_codepoints=sorted(set(substituted)),
        auxiliary_only_codepoints=sorted(new_codepoints-core_required-set(missing_regular)),
        auxiliary_body_sizes=[8, 10],
        rasterizer='Pillow/FreeType at native 6/8/10/12/16/20 pixels; not Godot glyph-pixel equivalence')


def scene_glyphs(source, faces):
    """Pack actual explicit faces only for the screens that select them."""
    if source.version < 28 or not any(faces): return [], {}
    texts = [bytes(source.block(off, length)).decode('utf-8') for off, length in source.strings]
    index = Path(__file__).resolve().parents[1]/'src/world_text_index.inc'
    entries = re.findall(r'\{"([^\"]+)",(\d+)\}', index.read_text())
    alphabets = [set(range(32,127)),set(range(32,127)),set(range(32,127))]
    for key, ordinal in entries:
        ordinal=int(ordinal)
        if ordinal >= len(texts): continue
        if re.fullmatch(r'ancients\.[^.]+\.title',key):
            alphabets[0].update(map(ord,texts[ordinal].upper()))
        if re.fullmatch(r'ancients\.[^.]+\.epithet',key) or re.fullmatch(r'monsters\.[^.]+\.name',key):
            alphabets[2].update(map(ord,texts[ordinal]))
        if key.startswith('events.CRYSTAL_SPHERE.'):
            alphabets[1].update(set().union(*styled_alphabet([texts[ordinal]])))
    auxiliary = auxiliary_body_slots(source)
    alphabets[1].update(styled_alphabet([value for slot, value in enumerate(texts) if slot not in auxiliary])[1])
    auxiliary_bold = styled_alphabet([value for slot, value in enumerate(texts) if slot in auxiliary and slot != 8507])[1]
    auxiliary_headings = set(map(ord, texts[8507])) if len(texts)>8507 and 8507 in auxiliary else set()
    for key, ordinal in entries:
        if key.startswith('card_reward_ui.') or key in ('gameplay_ui.CHOOSE_CARD_HEADER', 'gameplay_ui.TO_INSPECT_PROMPT', 'gameplay_ui.COMBAT_REWARD_HEADER_LOOT'):
            ordinal=int(ordinal)
            if ordinal<len(texts): alphabets[1].update(map(ord,texts[ordinal]))
    rows=[];report={}
    for i,face in enumerate(faces):
        if face is None: continue
        covered={cp for cp in alphabets[i] if cp>=32 and not text_format_control(cp) and cp in face.coverage}
        present=set()
        for cp in sorted(covered):
            for px in SCENE_SIZES:
                rows.append(face.glyph(cp,px,4+i));present.add((cp,px))
        if i==1:
            # PatchText explicitly selects KreonBold at its 8px body size;
            # the credits reader uses the same face for 10px section labels.
            for alphabet,px in ((auxiliary_bold,8),(auxiliary_headings,10)):
                for cp in sorted(alphabet):
                    if cp>=32 and not text_format_control(cp) and cp in face.coverage and (cp,px) not in present:
                        rows.append(face.glyph(cp,px,5));present.add((cp,px));covered.add(cp)
        report[('display','explicit_bold','scene_italic')[i]]=dict(**face.metadata(),codepoints=len(covered),glyphs=len(present))
    return rows,report


def replace(data, language, glyphs):
    """Rebuild only the glyph/text tail and relocate the bounded directory."""
    before = layout(data, language)
    tiles.require(before['version'] >= 24, 'Styled glyphs require the UI7 resource contract')
    source = tiles.PackView(data, before['auxiliary'], versions=(24, 25, 26, 27, 28))
    tiled = tiles.TileReader(source)
    ni, old_ng, ns, nc, nk, auxiliary = source.counts
    glyphs = sorted(glyphs, key=lambda row: (row[1], row[0]))
    tiles.require(0 < len(glyphs) <= 32768, 'Styled glyph directory exceeds runtime capacity')
    keys = [(row[0], row[1]) for row in glyphs]
    tiles.require(len(set(keys)) == len(keys), 'Duplicate styled glyph key')
    tiles.require({(r[0], r[2]) for r in source.glyphs if r[2] < 32} <= set(keys),
                  'Replacement dropped an existing regular glyph')
    art_end = before['prefix'] + before['artwork']
    delta = (len(glyphs)-old_ng)*tiles.GLYPH.size
    out = bytearray(data[:source.glyph_at])
    out.extend(bytes(len(glyphs)*tiles.GLYPH.size))
    out.extend(data[source.string_at:art_end])
    for i, (off, w, h, fmt, length) in enumerate(source.images):
        tiles.IMAGE.pack_into(out, 40+i*16, off+delta, w, h, fmt, length)
    header = list(tiles.GLOBAL.unpack_from(data, source.base_end))
    old_table = header[9]
    for i in (8, 9, 14): header[i] += delta
    tiles.GLOBAL.pack_into(out, source.base_end+delta, *header)
    for i in range(tiled.tile_groups+tiled.index_groups):
        off, length = struct.unpack_from('<II', data, old_table+i*8)
        struct.pack_into('<II', out, old_table+delta+i*8, off+delta, length)
    for i in range(tiled.frame_count):
        at = tiled.descriptors+i*16
        global_at, group, offset, length = tiles.DESCRIPTOR.unpack_from(data, at)
        tiles.DESCRIPTOR.pack_into(out, at+delta, global_at+delta, group, offset, length)
    # Retain UI6's frequent-codepoint locality across sizes and faces. Pixel
    # aliases occupy one payload, while each face keeps its own advance/shape.
    frequency = Counter()
    for off, length in source.strings: frequency.update(bytes(source.block(off, length)).decode('utf-8'))
    glyph_offsets, shared = {}, {}
    for index in sorted(range(len(glyphs)), key=lambda i: (-frequency[chr(glyphs[i][0])], glyphs[i][0], glyphs[i][1])):
        cp, px, w, h, advance, encoded, raw = glyphs[index]
        tiles.require(cp <= 0x10ffff and px <= 255 and (px & (31 if source.version >= 28 else 63)) in (SCENE_SIZES if source.version >= 28 else SIZES) and
            0 < w <= 40 and 0 < h <= 40 and advance <= 40 and
            len(raw) == (encoded or w*h), 'Invalid replacement glyph')
        if raw not in shared:
            out.extend(bytes(len(out) & 1)); shared[raw] = len(out); out.extend(raw)
        glyph_offsets[index] = shared[raw]
    for i, (cp, px, w, h, advance, encoded, _raw) in enumerate(glyphs):
        tiles.GLYPH.pack_into(out, source.glyph_at+i*16, cp, glyph_offsets[i], px, w, h, advance, encoded)
    strings = {}
    for i, (off, length) in enumerate(source.strings):
        raw = bytes(source.block(off, length+1))
        if raw not in strings:
            out.extend(bytes(len(out) & 1)); strings[raw] = len(out); out.extend(raw)
        struct.pack_into('<II', out, source.string_at+delta+i*8, strings[raw], length)
    tiles.HEADER.pack_into(out, 0, b'NSP2', source.version, tiles.REFERENCE, len(out), ni,
                          len(glyphs), ns, nc, nk, 0)
    target = tiles.PackView(out, auxiliary, versions=(24, 25, 26, 27, 28))
    tiles.TileReader(target)
    after = layout(out, language)
    tiles.require(after['prefix'] == before['prefix']+delta and after['artwork'] == before['artwork'] and
        out[after['prefix']:after['prefix']+after['artwork']] == data[before['prefix']:art_end],
        'Font replacement changed shared artwork bytes')
    tiles.require(target.clips == source.clips and target.cards == source.cards and target.aux == source.aux,
                  'Font replacement changed model indices')
    tiles.require(all(bytes(target.block(n, ln+1)) == bytes(source.block(o, lo+1))
        for (o, lo), (n, ln) in zip(source.strings, target.strings)), 'Font replacement changed locale text')
    return bytes(out), dict(input_bytes=len(data), output_bytes=len(out),
        input_sha256=tiles.sha256(data), output_sha256=tiles.sha256(out),
        previous_glyphs=old_ng, glyphs=len(glyphs), directory_growth_bytes=delta,
        resident_directory_bytes=target.base_end+64+(tiled.tile_groups+tiled.index_groups)*8+((ni+1)&~1),
        directory_capacity_bytes=tiles.DIRECTORY_LIMIT, artwork_bytes=before['artwork'],
        artwork_bytes_exact=True, strings_exact=True, model_indices_exact=True,
        discarded_obsolete_glyph_payload=True, glyph_locality='character-frequency then face/size')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--language', choices=('eng', 'zhs'), required=True)
    parser.add_argument('--font', type=Path, required=True)
    parser.add_argument('--bold-font', type=Path)
    parser.add_argument('--italic-font', type=Path)
    parser.add_argument('--bold-italic-font', type=Path)
    parser.add_argument('--fallback-font', type=Path, action='append', default=[],
        help='Repeat in priority order; each missing character uses the first real covering font')
    parser.add_argument('--display-font', type=Path)
    parser.add_argument('--scene-bold-font', type=Path)
    parser.add_argument('--scene-italic-font', type=Path)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    input_path, output_path, receipt_path = [tiles.private_path(path, label) for path, label in
        ((args.input, 'Input pack'), (args.output, 'Output pack'), (args.receipt, 'Receipt'))]
    tiles.require(len({input_path, output_path, receipt_path}) == 3, 'Input/output/receipt must differ')
    paths = (args.font, args.bold_font, args.italic_font, args.bold_italic_font)
    faces = [Face(tiles.private_path(path, 'Original font')) if path else None for path in paths]
    fallback = [Face(tiles.private_path(path, 'Fallback font')) for path in args.fallback_font]
    data = input_path.read_bytes(); info = layout(data, args.language)
    source = tiles.PackView(data, info['auxiliary'], versions=(24, 25, 26, 27, 28))
    glyphs, font_report = generate(source, faces, fallback)
    tiles.require(not font_report['missing_regular_codepoints'],
                  'Fallback chain does not cover localized characters: '+repr(font_report['missing_regular_codepoints']))
    scene_faces=[Face(tiles.private_path(path, 'Original scene font')) if path else None for path in
                 (args.display_font,args.scene_bold_font,args.scene_italic_font)]
    extra,scene_report=scene_glyphs(source,scene_faces);glyphs.extend(extra)
    font_report['scene_fonts']=scene_report
    output, report = replace(data, args.language, glyphs)
    report.update(schema=1, reference='v0.111.0 / 41cef1ea', language=args.language, **font_report)
    tiles.atomic_bytes(output_path, output)
    tiles.atomic_bytes(receipt_path, json.dumps(report, indent=2).encode()+b'\n')
    print(json.dumps({key: report[key] for key in ('language', 'glyphs', 'output_bytes',
          'directory_growth_bytes', 'resident_directory_bytes', 'styled_codepoints')}))


if __name__ == '__main__': main()
