"""Replace the retired Timeline text tail with three original character labels.

Read every label from the selected original locale. Keep all 8704 preceding
strings and existing glyph pixels/metrics, copy compressed artwork unchanged,
and rasterize only absent glyphs from the original font family.
"""
from __future__ import annotations

import json
from pathlib import Path
import struct
import unicodedata

import regex
import content_tiles as t
from build_locale_editions import face_set
from coalesce_tile_resources import view
import group_strings
from replace_original_glyphs import styled_alphabet, text_format_control
import split_content as split
from timeline_text_ui24 import original_glyph

BASE_STRINGS, STRINGS, LEGACY_STRINGS = 8704, 8707, 8961
KEYS = ('CHARACTER_SELECT.locked.title', 'CHARACTER_SELECT.lockedRelic.title',
        'CHARACTER_SELECT.lockedRelic.description')
SUBSTITUTED = frozenset(('zhs', 'zht', 'jpn', 'kor', 'pol', 'rus', 'tha'))


def original_texts(pack, code):
    path = f'localization/{code}/main_menu_ui.json'
    table = json.loads(pack.read(path)); result = [table.get(key) for key in KEYS]
    t.require(all(isinstance(value, str) and value and '\0' not in value and
                  len(value.encode('utf-8')) < t.STRING_LIMIT for value in result),
              'Original character lock text is missing: '+code)
    return result, dict(original_locale=code, source_table=path, keys=KEYS, english_fallback_slots=[])


def shape_texts(texts, existing):
    # UI24's retained glyphs may already occupy 10E000. Give new combining
    # graphemes separate, unused keys without touching any preceding text.
    clusters, inverse = {}, {}; candidate = 0x10d000
    def replace(match):
        nonlocal candidate
        value = match[0]
        if len(value) == 1 or not any(unicodedata.category(c).startswith('M') or c == '\u0e33' for c in value):
            return value
        if value not in clusters:
            while candidate in existing: candidate += 1
            t.require(candidate < 0x10e000, 'Character label grapheme keys exhausted')
            clusters[value] = candidate; inverse[candidate] = value; candidate += 1
        return chr(clusters[value])
    shaped = [regex.sub(r'\X', replace, text) for text in texts]
    t.require(all(''.join(inverse.get(ord(c), c) for c in after) == before
                  for before, after in zip(texts, shaped)), 'Character label shaping changed source UTF-8')
    return shaped, inverse


def glyph_requirements(texts, code):
    body = styled_alphabet(texts); regular = set().union(*body)
    wanted = {(cp, 10, 0) for cp in regular}
    for face in range(1, 4): wanted.update((cp, 10, face) for cp in body[face])
    heading_face, bold_face = (0, 0) if code in SUBSTITUTED else (4, 5)
    wanted.update((ord(c), px, heading_face) for c in texts[0] for px in (12, 14, 16, 18))
    wanted.update((ord(c), 10, bold_face) for c in texts[1])
    return {(cp, px, face) for cp, px, face in wanted if not text_format_control(cp)}


def missing_glyphs(source, texts, clusters, code, fonts):
    known = {(row[0], row[2]) for row in source.glyphs}; wanted = glyph_requirements(texts, code)
    missing = sorted((cp, px, face) for cp, px, face in wanted
                     if (cp, px | ((face & 3) << 6) | ((face & 4) << 3)) not in known)
    if not missing: return [], wanted, []
    faces, fallback = face_set(Path(fonts), code); additions, used = [], {}
    for cp, px, face in missing:
        word = clusters.get(cp, chr(cp)); alphabet = set(map(ord, word))
        chosen = next((font for font in (faces[face], *fallback) if alphabet <= font.coverage), None)
        if chosen is None and len(word) == 1 and unicodedata.category(word) == 'Zs': chosen = faces[face]
        t.require(chosen is not None, 'Original fonts lack character label grapheme '+repr(word))
        additions.append(original_glyph(chosen, cp, word, px, face)); used[chosen.path.name] = chosen
    return additions, wanted, [font.metadata() for _, font in sorted(used.items())]


def replace_records(data, texts, additions=()):
    """Rebuild only the locale tail; retain original compressed glyph groups."""
    source = view(data); ni, ng, ns, nc, nk, _ = source.counts
    t.require(ns in (BASE_STRINGS, LEGACY_STRINGS) and len(texts) == 3,
              'Character text requires the unchanged prefix or legacy Timeline tail')
    keys = {(row[0], row[2]) for row in source.glyphs}; added = {(row[0], row[1]) for row in additions}
    t.require(len(added) == len(additions) and not keys & added, 'New character glyph keys must be unique')
    code = source.string_payload(split.MARKER)[:-1].decode('ascii'); bounds = split.layout(data, code)
    end = bounds['prefix']+bounds['artwork']; next_count = BASE_STRINGS+3
    delta = len(additions)*16+(next_count-ns)*8
    out = bytearray(data[:source.glyph_at]); out.extend(bytes((ng+len(additions))*16+next_count*8))
    out.extend(data[source.clip_at:end])
    for i, (off, w, h, fmt, length) in enumerate(source.images):
        t.IMAGE.pack_into(out, 40+i*16, off+delta, w, h, fmt, length)
    tiled = t.TileReader(source); header = list(t.GLOBAL.unpack_from(data, source.base_end)); table = header[9]
    for field in (8, 9, 14): header[field] += delta
    t.GLOBAL.pack_into(out, source.base_end+delta, *header)
    t.require(header[8] <= t.DIRECTORY_LIMIT, 'Character labels exceed the unchanged directory pool')
    for i in range(tiled.tile_groups+tiled.index_groups):
        off, length = struct.unpack_from('<II', data, table+i*8)
        struct.pack_into('<II', out, table+delta+i*8, off+delta, length)
    for i in range(tiled.frame_count):
        at = tiled.descriptors+i*16; global_at, group, raw_at, length = t.DESCRIPTOR.unpack_from(data, at)
        t.DESCRIPTOR.pack_into(out, at+delta, global_at+delta, group, raw_at, length)
    def append(raw):
        out.extend(bytes(len(out) & 1)); at = len(out); out.extend(raw); return at
    copied = {}; rows = []
    def copy(at, length):
        key = at, length
        if key not in copied: copied[key] = append(source.block(at, length))
        return copied[key]
    # Copy cohorts in their original physical order to retain cold locality.
    for cp, off, band, w, h, advance, encoded in sorted(source.glyphs, key=lambda row: row[1]):
        if encoded & t.GLYPH_GROUP_FLAG:
            relative, size, raw_at, length = struct.unpack_from('<iIII', data, off)
            group_at = copy(off+relative, size); at = (len(out)+1) & ~1
            at = append(struct.pack('<iIII', group_at-at, size, raw_at, length))
        else: at = copy(off, encoded or w*h)
        rows.append((cp, at, band, w, h, advance, encoded))
    for cp, band, w, h, advance, encoded, raw in additions:
        t.require(0 < w <= 40 and 0 < h <= 40 and 0 <= encoded < t.GLYPH_GROUP_FLAG and
                  len(raw) == (encoded or w*h), 'Invalid character glyph payload')
        rows.append((cp, append(raw), band, w, h, advance, encoded))
    rows.sort(key=lambda row: (row[2], row[0]))
    for i, row in enumerate(rows): t.GLYPH.pack_into(out, source.glyph_at+i*16, *row)
    strings = [source.string_payload(i) for i in range(BASE_STRINGS)] + [text.encode('utf-8')+b'\0' for text in texts]
    class NewStrings:
        def __init__(self):
            self.data = out; self.string_at = source.glyph_at+len(rows)*16
            self.strings = [(i, len(raw)-1) for i, raw in enumerate(strings)]
        def string_payload(self, index): return strings[index]
    t.HEADER.pack_into(out, 0, b'NSP2', source.version, t.REFERENCE, len(out), ni, len(rows), next_count, nc, nk, 0)
    new = NewStrings(); output, compression = group_strings.write(new, group_strings.plan(new), len(out))
    output = bytearray(output); output[4:8] = data[4:8]; output[36:40] = data[36:40]
    return bytes(output), compression


def verify(data, output, texts, wanted=()):
    before, after = view(data), view(output); code = before.string_payload(split.MARKER)[:-1].decode('ascii')
    a, b = split.layout(data, code), split.layout(output, code)
    t.require(before.cards == after.cards and before.clips == after.clips and before.aux == after.aux and
              len(before.images) == len(after.images) and all(x[1:] == y[1:] for x, y in zip(before.images, after.images)) and
              data[36:40] == output[36:40], 'Character text changed artwork identities or animation profile')
    t.require(data[a['prefix']:a['prefix']+a['artwork']] == output[b['prefix']:b['prefix']+b['artwork']],
              'Character text changed compressed artwork')
    keys = {(row[0], row[2]): i for i, row in enumerate(after.glyphs)}
    for i, row in enumerate(before.glyphs):
        j = keys[row[0], row[2]]
        t.require(row[2:] == after.glyphs[j][2:] and before.glyph_payload(i) == after.glyph_payload(j),
                  'Character text changed an existing glyph')
    t.require(all((cp, px | ((face & 3) << 6) | ((face & 4) << 3)) in keys for cp, px, face in wanted),
              'Character labels lack a required glyph')
    t.require(len(after.strings) == BASE_STRINGS+3, 'Character label string count differs')
    for i in range(BASE_STRINGS):
        t.require(before.string_payload(i) == after.string_payload(i), 'Character text changed a preceding string')
    for i, text in enumerate(texts, BASE_STRINGS):
        t.require(after.string_payload(i) == text.encode('utf-8')+b'\0', 'Character label roundtrip differs')
    return dict(existing_strings_exact=BASE_STRINGS, existing_glyphs_exact=len(before.glyphs),
                removed_timeline_strings=max(0, len(before.strings)-BASE_STRINGS), appended_strings=3,
                missing_required_glyphs=0, common_artwork_bytes_exact=a['artwork'], clip_clocks_and_catalogs_exact=True)


def patch(data, pack, fonts):
    source = view(data); code = source.string_payload(split.MARKER)[:-1].decode('ascii')
    original, localization = original_texts(pack, code)
    texts, clusters = shape_texts(original, {row[0] for row in source.glyphs})
    glyphs, wanted, font_proof = missing_glyphs(source, texts, clusters, code, fonts)
    output, compression = replace_records(data, texts, glyphs); proof = verify(data, output, texts, wanted)
    return output, dict(schema=1, complete=True, language=code, input_sha256=t.sha256(data),
        output_sha256=t.sha256(output), input_bytes=len(data), output_bytes=len(output),
        original_utf8_sha256=t.sha256(b'\0'.join(text.encode('utf-8') for text in original)),
        original_utf8_recovered_from_graphemes=True, shaped_graphemes=len(clusters),
        added_glyphs=len(glyphs), required_glyph_keys=len(wanted), original_fonts=font_proof,
        localization=localization, compression=compression, verification=proof)
