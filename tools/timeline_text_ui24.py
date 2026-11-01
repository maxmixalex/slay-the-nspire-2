"""Append original Timeline text and only its missing original-font glyphs.

This is a locale-tail operation after artwork grafting. Existing image streams,
glyphs, strings, IDs and clocks are preserved; all editions keep the exact same
common artwork. Original assets are read from the owner's pinned installation.
"""
from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
import struct
import unicodedata

from PIL import Image, ImageDraw
import regex

import content_tiles as t
from coalesce_tile_resources import view
from build_locale_editions import face_set
import group_strings
from replace_original_glyphs import styled_alphabet, text_format_control
import split_content as split

ROOT = Path(__file__).resolve().parents[1]
BASE_STRINGS = 8704
ADDED_STRINGS = 257
CLUSTER_BASE, CLUSTER_END = 0x10e000, 0x10f000
SUBSTITUTED = frozenset(('zhs', 'zht', 'jpn', 'kor', 'pol', 'rus', 'tha'))
PORT_LABELS = {
    'eng': ('Timeline', 'Legacy access: content open; old obtain dates unknown.',
            'Install the matching Timeline resources.', 'Select an Epoch; Enter to inspect.',
            'Obtained:', 'Revealed', 'Not obtained', 'Locked',
            'Reveal the matching Epoch in the Timeline.'),
    'zhs': ('时间线', '旧版兼容开放，历史获得时间未知。', '请安装配套的时间线资源。',
            '选择条目，按 Enter 查看。', '获得时间：', '已揭示', '尚未获得', '未解锁',
            '请在时间线揭示对应条目。'),
}


def contract():
    path = ROOT/'config/timeline-text-ui24.json'
    result = json.loads(path.read_text(encoding='utf-8'))
    t.require(result['string_base'] == BASE_STRINGS and
              len(result['strings']) == ADDED_STRINGS and
              len(result['epoch_order']) == 57 and
              result['epoch_fields'] == ['title', 'description', 'unlockInfo', 'unlockText'],
              'Timeline string identity contract changed')
    return result, t.sha256(path.read_bytes())


def original_texts(pack, language, rows):
    """Use the original locale table, with the existing English fallback rule."""
    names = sorted({row['table'] for row in rows})
    english = {name: json.loads(pack.read('localization/eng/'+name+'.json')) for name in names}
    selected = (english if language == 'eng' else
                {name: json.loads(pack.read('localization/'+language+'/'+name+'.json')) for name in names})
    values, fallback, empty = [], [], []
    for index, row in enumerate(rows):
        table, key = row['table'], row['key']
        value = selected[table].get(key)
        if value is None and key in english[table]:
            value = english[table][key]
            fallback.append(BASE_STRINGS+index)
        if value is None and row.get('optional', False):
            value = ''; empty.append(BASE_STRINGS+index)
        t.require(isinstance(value, str) and '\0' not in value and
                  len(value.encode('utf-8')) < t.STRING_LIMIT,
                  'Missing or invalid original Timeline text: '+language+'/'+table+'.'+key)
        values.append(value)
    return values, dict(original_locale=language, english_fallback_slots=fallback,
                       optional_absent_slots=empty)


def shape_texts(texts, existing_codepoints):
    """Use disjoint locale-local keys for complete combining graphemes.

    Preserve the original sequence, including normalization. Expanding every
    display key must recover the exact original UTF-8, not a rewritten phrase.
    """
    clusters, inverse = {}, {}
    def replace(match):
        value = match[0]
        if len(value) == 1 or not any(unicodedata.category(c).startswith('M') or c == '\u0e33' for c in value):
            return value
        if value not in clusters:
            cp = CLUSTER_BASE+len(clusters)
            t.require(cp < CLUSTER_END and cp not in existing_codepoints,
                      'Timeline grapheme key collides with an existing glyph')
            clusters[value] = cp; inverse[cp] = value
        return chr(clusters[value])
    shaped = [regex.sub(r'\X', replace, text) for text in texts]
    t.require(all(''.join(inverse.get(ord(c), c) for c in after) == before
                  for before, after in zip(texts, shaped)),
              'Timeline shaping changed original text')
    return shaped, inverse


def glyph_requirements(texts, language):
    # Actual Timeline reader: 10px regular list/body/story; source label 12px
    # heading. Rich spans retain their explicit bold/italic roles.
    labels = PORT_LABELS['zhs' if language == 'zhs' else 'eng']
    body = styled_alphabet([*texts, *labels])
    required = {(cp, 10, 0) for cp in set().union(*body)}
    for face in range(1, 4):
        required.update((cp, 10, face) for cp in body[face])
    title = set().union(*styled_alphabet([*texts[0:57*4:4], labels[0]]))
    required.update((cp, 12, 0 if language in SUBSTITUTED else 5) for cp in title)
    # The port menu heading explicitly selects the bold scene face; the
    # original Epoch heading uses the source locale's substitution rule.
    required.update((ord(c), 12, 5) for c in labels[0])
    required.update((ord(c), 10, 5) for c in labels[0])
    return {(cp, px, face) for cp, px, face in required if not text_format_control(cp)}


def original_glyph(font, cp, text, px, face):
    """Use the existing v40 fractional advance/bearing encoding at native size."""
    measured = font.fonts[px]
    missing_space = len(text) == 1 and unicodedata.category(text) == 'Zs' and cp not in font.coverage
    value = ' ' if missing_space else text
    q = round(measured.getlength(value)*64*(.5 if missing_space and cp == 0x202f else 1))
    left = 0 if missing_space else min(0, measured.getbbox(value)[0])
    width = max(1, math.ceil(q/64), measured.getbbox(value)[2])-left
    height = px+5
    t.require(-7 <= left <= 0 and 0 <= q <= 2560 and width <= 40 and height <= 40,
              'Original Timeline glyph exceeds existing font bounds')
    image = Image.new('L', (width, height))
    if not missing_space:
        ImageDraw.Draw(image).text((-left, px+1), value, font=measured, fill=255, anchor='ls')
    raw = image.tobytes()
    from ui12_resources import rle
    payload, encoded = rle(raw)
    band = px | ((face & 3) << 6) | ((face & 4) << 3)
    return cp, band, width, height, 0x8000 | ((-left) << 12) | q, encoded, payload


def missing_glyphs(source, texts, clusters, language, fonts):
    known = {(row[0], row[2]) for row in source.glyphs}
    wanted = glyph_requirements(texts, language)
    missing = sorted((cp, px, face) for cp, px, face in wanted
                     if (cp, px | ((face & 3) << 6) | ((face & 4) << 3)) not in known)
    if not missing:
        return [], wanted, []
    faces, fallback = face_set(Path(fonts), language)
    additions, used = [], {}
    for cp, px, face in missing:
        word = clusters.get(cp, chr(cp)); alphabet = set(map(ord, word))
        chosen = next((font for font in (faces[face], *fallback) if alphabet <= font.coverage), None)
        if chosen is None and len(word) == 1 and unicodedata.category(word) == 'Zs':
            chosen = faces[face]
        t.require(chosen is not None, 'Original fonts lack Timeline grapheme '+repr(word))
        additions.append(original_glyph(chosen, cp, word, px, face))
        used[chosen.path.name] = chosen
    return additions, wanted, [font.metadata() for _, font in sorted(used.items())]


def append_records(data, texts, additions):
    """Relocate directories once, copying every old compressed payload byte."""
    source = view(data); tiled = t.TileReader(source)
    ni, ng, ns, nc, nk, _ = source.counts
    t.require(ns == BASE_STRINGS and len(texts) == ADDED_STRINGS,
              'Timeline append requires the unchanged 8704-string source')
    old_keys = {(row[0], row[2]) for row in source.glyphs}
    keys = {(row[0], row[1]) for row in additions}
    t.require(len(keys) == len(additions) and not keys & old_keys,
              'Timeline glyph additions must be new and unique')
    delta = len(additions)*16+len(texts)*8
    glyph_delta = len(additions)*16
    out = bytearray(data[:source.glyph_at]); out.extend(bytes((ng+len(additions))*16))
    out.extend(data[source.string_at:source.clip_at]); out.extend(bytes(len(texts)*8))
    out.extend(data[source.clip_at:])
    for i, (off, w, h, fmt, length) in enumerate(source.images):
        t.IMAGE.pack_into(out, 40+i*16, off+delta, w, h, fmt, length)
    rows = [(cp, off+delta, band, w, h, advance, encoded)
            for cp, off, band, w, h, advance, encoded in source.glyphs]
    for cp, band, w, h, advance, encoded, raw in additions:
        t.require(0 < w <= 40 and 0 < h <= 40 and 0 <= encoded < t.GLYPH_GROUP_FLAG and
                  len(raw) == (encoded or w*h), 'Invalid Timeline glyph payload')
        out.extend(bytes(len(out) & 1)); at = len(out); out.extend(raw)
        rows.append((cp, at, band, w, h, advance, encoded))
    rows.sort(key=lambda row: (row[2], row[0]))
    for i, row in enumerate(rows):
        t.GLYPH.pack_into(out, source.glyph_at+i*16, *row)
    for i, (off, length) in enumerate(source.strings):
        struct.pack_into('<II', out, source.string_at+glyph_delta+i*8, off+delta, length)
    header = list(t.GLOBAL.unpack_from(data, source.base_end)); table = header[9]
    for field in (8, 9, 14):
        header[field] += delta
    t.GLOBAL.pack_into(out, source.base_end+delta, *header)
    t.require(header[8] <= t.DIRECTORY_LIMIT, 'Timeline exceeds the unchanged directory pool')
    for i in range(tiled.tile_groups+tiled.index_groups):
        off, length = struct.unpack_from('<II', data, table+i*8)
        struct.pack_into('<II', out, table+delta+i*8, off+delta, length)
    for i in range(tiled.frame_count):
        at = tiled.descriptors+i*16
        global_at, group, raw_at, length = t.DESCRIPTOR.unpack_from(data, at)
        t.DESCRIPTOR.pack_into(out, at+delta, global_at+delta, group, raw_at, length)
    t.HEADER.pack_into(out, 0, b'NSP2', source.version, t.REFERENCE, len(out),
                      ni, len(rows), ns+len(texts), nc, nk, struct.unpack_from('<I', data, 36)[0])
    class NewStrings:
        def __init__(self):
            self.data = out
            self.string_at = source.string_at+glyph_delta+ns*8
            self.strings = [(i, len(text.encode('utf-8'))) for i, text in enumerate(texts)]
        def string_payload(self, index):
            raw = texts[index].encode('utf-8')+b'\0'
            t.require(b'\0' not in raw[:-1] and len(raw) <= t.STRING_LIMIT,
                      'Invalid Timeline text payload')
            return raw
    new = NewStrings()
    output, strings = group_strings.write(new, group_strings.plan(new), len(out))
    output = bytearray(output)
    output[4:8] = data[4:8]; output[36:40] = data[36:40]
    return bytes(output), strings


def verify(data, output, texts, wanted=()):
    before, after = view(data), view(output)
    code = before.string_payload(split.MARKER)[:-1].decode('ascii')
    a, b = split.layout(data, code), split.layout(output, code)
    t.require(before.cards == after.cards and before.clips == after.clips and before.aux == after.aux and
              len(before.images) == len(after.images) and
              all(x[1:] == y[1:] for x, y in zip(before.images, after.images)) and
              data[36:40] == output[36:40], 'Timeline changed artwork identities, clocks or profile')
    t.require(a['artwork'] == b['artwork'] and
              data[a['prefix']:a['prefix']+a['artwork']] == output[b['prefix']:b['prefix']+b['artwork']],
              'Timeline changed shared compressed artwork')
    keys = {(row[0], row[2]): i for i, row in enumerate(after.glyphs)}
    for i, row in enumerate(before.glyphs):
        j = keys[(row[0], row[2])]
        t.require(row[2:] == after.glyphs[j][2:] and before.glyph_payload(i) == after.glyph_payload(j),
                  'Timeline changed an existing glyph')
    t.require(all((cp, px | ((face & 3) << 6) | ((face & 4) << 3)) in keys
                  for cp, px, face in wanted), 'Timeline lacks a required exact glyph')
    for i in range(len(before.strings)):
        t.require(before.string_payload(i) == after.string_payload(i), 'Timeline changed an existing string')
    for i, text in enumerate(texts, BASE_STRINGS):
        t.require(after.string_payload(i) == text.encode('utf-8')+b'\0', 'Timeline string roundtrip differs')
    return dict(existing_strings_exact=len(before.strings), existing_glyphs_exact=len(before.glyphs),
                appended_strings=len(texts), missing_required_glyphs=0,
                common_artwork_bytes_exact=a['artwork'], clip_clocks_and_catalogs_exact=True)


def patch(data, pack, fonts):
    source = view(data); code = source.string_payload(split.MARKER)[:-1].decode('ascii')
    spec, identity = contract()
    original, localization = original_texts(pack, code, spec['strings'])
    texts, clusters = shape_texts(original, {row[0] for row in source.glyphs})
    glyphs, wanted, font_proof = missing_glyphs(source, texts, clusters, code, fonts)
    output, compression = append_records(data, texts, glyphs)
    proof = verify(data, output, texts, wanted)
    return output, dict(schema=1, complete=True, language=code, contract_sha256=identity,
        input_sha256=t.sha256(data), output_sha256=t.sha256(output), input_bytes=len(data), output_bytes=len(output),
        original_utf8_sha256=t.sha256(b'\0'.join(text.encode('utf-8') for text in original)),
        original_utf8_bytes=sum(len(text.encode('utf-8')) for text in original),
        original_utf8_recovered_from_graphemes=True, shaped_graphemes=len(clusters),
        added_glyphs=len(glyphs), required_glyph_keys=len(wanted), original_fonts=font_proof,
        localization=localization, compression=compression, verification=proof)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ('input', 'output', 'pck', 'fonts'):
        parser.add_argument('--'+option, type=Path, required=True)
    args = parser.parse_args()
    t.require(args.input.resolve() != args.output.resolve(), 'Preserve the original locale input')
    from pck import Pack
    pack = Pack(args.pck)
    try:
        output, receipt = patch(args.input.read_bytes(), pack, args.fonts)
    finally:
        pack.close()
    t.atomic_bytes(args.output, output)
    t.atomic_bytes(args.output.with_suffix('.json'), (json.dumps(receipt, indent=2)+'\n').encode())
    print(json.dumps(receipt), flush=True)


if __name__ == '__main__':
    main()
