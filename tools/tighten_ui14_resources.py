"""Tighten the current bounded streams without changing pixels or grouping.

The stronger encoder is an offline operation. The existing decoder, dictionary,
image coordinates, frame durations and random-access groups are unchanged.
Every chosen stream is independently decoded before it enters the output.
"""
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import lzma
from pathlib import Path
import struct

import content_tiles as tiles
from compact_tile_resources import grouped_slices
from recompress_resources import Relocation, choose_group
from retained_filter import group_layout
from ui12_resources import view

ENCODER = b'UI14-exact-LZMA2-extreme-depth1024-v1'


def describe(payload):
    h = list(tiles.GROUP.unpack_from(payload))
    if h[0] in (b'TGR1', b'TGR2'):
        tiles.require(h[4] == len(payload)-32, 'Invalid tile stream')
        return h, 32, h[3], 4
    raw, start, _mode, _slices = group_layout(payload, 40)
    return h, start, raw, 2


def entries(source):
    result = {}
    header = tiles.GLOBAL.unpack_from(source.data, source.base_end)
    for i in range(header[5]+header[6]):
        at, size = struct.unpack_from('<II', source.data, header[9]+8*i)
        result[at] = size
    for at, (relative, size, _raw_at, _shape) in grouped_slices(source).items():
        target = at+relative
        tiles.require(target not in result or result[target] == size, 'Shared stream extent differs')
        result[target] = size
    previous = 0
    for at, size in sorted(result.items()):
        tiles.require(at >= previous and at % 2 == 0 and size > 32 and at+size <= len(source.data),
                      'Overlapping or invalid stream')
        previous = at+size
    return sorted(result.items())


def tighten(data, cache, workers=4, artwork_only=False):
    source = view(data);cache = Path(cache);cache.mkdir(parents=True, exist_ok=True)
    old_entries = entries(source)
    selected_entries = old_entries
    if artwork_only:
        import split_content
        language = source.string_payload(split_content.MARKER)[:-1].decode('ascii')
        bounds = split_content.layout(data, language);end = bounds['prefix']+bounds['artwork']
        selected_entries = [(at, size) for at, size in old_entries if at+size <= end]

    def convert(item):
        at, size = item
        old = bytes(data[at:at+size]);h, start, raw_size, encoded_field = describe(old)
        raw = tiles.decode_lzma2(old[start:], raw_size)
        key = hashlib.sha256(ENCODER+old).hexdigest();path = cache/(key+'.bin')
        best = old
        if path.is_file():
            candidate = path.read_bytes();ch, cs, cr, cf = describe(candidate)
            original = h.copy();original[encoded_field] = ch[cf]
            tiles.require(original == ch and start == cs and raw_size == cr and
                          old[32:start] == candidate[32:cs] and
                          tiles.decode_lzma2(candidate[cs:], cr) == raw,
                          'Cached stream differs')
            if len(candidate) < len(best):best = candidate
        else:
            for lc, lp, pb in ((4, 0, 0), (4, 0, 1), (3, 1, 1)):
                encoded = lzma.compress(raw, format=lzma.FORMAT_RAW, filters=[dict(
                    id=lzma.FILTER_LZMA2, preset=9 | lzma.PRESET_EXTREME,
                    dict_size=tiles.GROUP_LIMIT, nice_len=273, depth=1024,
                    lc=lc, lp=lp, pb=pb)])
                if start+len(encoded) >= len(best):continue
                tiles.require(tiles.decode_lzma2(encoded, raw_size) == raw,
                              'Stronger encoding changed decoder bytes')
                ch = h.copy();ch[encoded_field] = len(encoded)
                best = tiles.GROUP.pack(*ch)+old[32:start]+encoded
            tiles.atomic_bytes(path, best)
        payload, physical, _ = choose_group(old, best, raw_size)
        return at, size, payload, physical, raw_size

    plan = [];saved = 0
    with ThreadPoolExecutor(max_workers=workers) as pool:
        for row in pool.map(convert, selected_entries):
            plan.append(row);saved += row[1]-len(row[3])
            if len(plan) % 64 == 0:
                print(json.dumps(dict(stage='tighten', completed=len(plan), total=len(selected_entries),
                                      saved_bytes=saved)), flush=True)
    replacements = [row[:4] for row in plan if len(row[3]) < row[1]]
    relocate = Relocation(replacements);sizes = {};out = bytearray();previous = 0
    for at, size, payload, physical in replacements:
        out.extend(data[previous:at]);out.extend(physical);previous = at+size;sizes[at] = len(payload)
    out.extend(data[previous:])
    for i, (at, w, h, fmt, size) in enumerate(source.images):
        tiles.IMAGE.pack_into(out, 40+i*16, relocate(at), w, h, fmt, size)
    for i, (cp, at, band, w, h, advance, encoded) in enumerate(source.glyphs):
        tiles.GLYPH.pack_into(out, source.glyph_at+i*16, cp, relocate(at), band, w, h, advance, encoded)
    for i, (at, size) in enumerate(source.strings):
        struct.pack_into('<II', out, source.string_at+i*8, relocate(at), size)
    header = list(tiles.GLOBAL.unpack_from(data, source.base_end))
    descriptors, table = header[8], header[9]
    for field in (8, 9, 14):header[field] = relocate(header[field])
    tiles.GLOBAL.pack_into(out, source.base_end, *header)
    for i in range(header[5]+header[6]):
        at, size = struct.unpack_from('<II', data, table+i*8)
        struct.pack_into('<II', out, relocate(table+i*8), relocate(at), sizes.get(at, size))
    for i in range(header[7]):
        at = descriptors+i*16;global_at, group, raw_at, size = tiles.DESCRIPTOR.unpack_from(data, at)
        tiles.DESCRIPTOR.pack_into(out, relocate(at), relocate(global_at), group, raw_at, size)
    for at, (relative, size, raw_at, shape) in grouped_slices(source).items():
        target = at+relative
        struct.pack_into('<iIII', out, relocate(at), relocate(target)-relocate(at),
                         sizes.get(target, size), raw_at, shape)
    struct.pack_into('<I', out, 12, len(out));struct.pack_into('<I', out, 36, 0)
    final = view(out)
    tiles.require(source.counts == final.counts and source.cards == final.cards and
                  source.aux == final.aux and source.clips == final.clips, 'Stable model/clock metadata changed')
    final_entries = entries(final)
    tiles.require(len(old_entries) == len(final_entries), 'Stream count changed')
    for (old_at, old_size), (new_at, new_size) in zip(old_entries, final_entries):
        tiles.require(new_at == relocate(old_at), 'Stream relocation differs')
        old = bytes(data[old_at:old_at+old_size]);new = bytes(out[new_at:new_at+new_size])
        a, ai, ar, af = describe(old);b, bi, br, bf = describe(new)
        a[af] = b[bf] = 0
        tiles.require(a == b and ai == bi and ar == br and old[32:ai] == new[32:bi] and
                      tiles.decode_lzma2(old[ai:], ar) == tiles.decode_lzma2(new[bi:], br),
                      'Final decoder input differs')
    for i in range(len(source.strings)):
        tiles.require(source.string_payload(i) == final.string_payload(i), 'Localized text changed')
    for i in range(len(source.glyphs)):
        tiles.require(source.glyphs[i][2:] == final.glyphs[i][2:] and
                      source.glyph_payload(i) == final.glyph_payload(i), 'Font pixels or metrics changed')
    return bytes(out), dict(input_bytes=len(data), output_bytes=len(out), saved_bytes=len(data)-len(out),
        groups=len(plan), improved_groups=len(replacements), unchanged_decoder_bytes=True,
        unchanged_pixels_geometry_clocks=True, runtime_memory_added=0)
