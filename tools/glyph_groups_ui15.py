#!/usr/bin/env python3
"""Regroup complete current locale glyphs by their demanded face and size.

The resource keeps every native glyph's exact raw/RLE pixels, dimensions,
bearing, advance and pair-kerning record. Only private compressed block order
changes. Artwork and animation streams remain byte-identical, and no runtime
cache, font engine, preload, data version or language fallback is added.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import struct
import time

import content_tiles as tiles
import group_glyphs
import group_strings
import split_content as split
from ui12_resources import view


def repack(data, *, order='band-frequency', group_bytes=group_glyphs.DEFAULT_GROUP_BYTES):
    source = view(data)
    language = bytes(source.string_payload(split.MARKER)[:-1]).decode('ascii')
    bounds = split.layout(data, language)
    batches = group_glyphs.plan(source, group_bytes, order=order)
    raw, statistics = group_glyphs.write(source, batches, bounds)
    raw = bytearray(raw)
    struct.pack_into('<I', raw, 4, source.version)
    glyphs = view(raw)
    # The old writer expands text while rebuilding the live glyph tail.
    # Reuse the existing exact text codec, including its literal boot marker.
    packed, text_statistics = group_strings.write(glyphs, group_strings.plan(glyphs),
        group_strings.nonstring_end(glyphs, split.layout(raw, language)))
    packed = bytearray(packed)
    struct.pack_into('<I', packed, 4, source.version)
    result = bytes(packed)
    target = view(result)
    after = split.layout(result, language)
    tiles.require(target.counts == source.counts and target.version == source.version,
                  'Regrouping changed resource version or registration counts')
    tiles.require(target.images == source.images and target.clips == source.clips and
                  target.cards == source.cards and target.aux == source.aux,
                  'Regrouping changed artwork, animation or model registration')
    first, end = bounds['prefix'], bounds['prefix']+bounds['artwork']
    tiles.require((first, bounds['artwork']) == (after['prefix'], after['artwork']) and
                  data[first:end] == result[first:end], 'Regrouping changed shared artwork bytes')
    alpha_bytes = 0
    for index, (before, current) in enumerate(zip(source.glyphs, target.glyphs)):
        tiles.require(before[:1]+before[2:6] == current[:1]+current[2:6] and
                      before[6] & ~tiles.GLYPH_GROUP_FLAG == current[6] & ~tiles.GLYPH_GROUP_FLAG,
                      'Regrouping changed a glyph key, geometry, bearing, advance or native encoding')
        tiles.require(source.glyph_payload(index) == target.glyph_payload(index),
                      'Regrouping changed native glyph alpha bytes')
        alpha_bytes += before[3]*before[4]
    for index in range(len(source.strings)):
        tiles.require(source.string_payload(index) == target.string_payload(index),
                      'Regrouping changed a localized string')
    tiles.require(not target.strings[split.MARKER][1] & tiles.STRING_GROUP_FLAG,
                  'Regrouping compressed the literal boot locale marker')
    return result, dict(schema=1, reference='v0.111.0 / 41cef1ea', language=language,
        input_version=source.version, output_version=target.version,
        input_bytes=len(data), output_bytes=len(result), saved_bytes=len(data)-len(result),
        input_sha256=tiles.sha256(data), output_sha256=tiles.sha256(result), order=order,
        group_target_bytes=group_bytes, runtime_memory_added=0, runtime_preload_bytes=0,
        glyphs=statistics, strings=text_statistics,
        verification=dict(glyphs_verified=len(source.glyphs), alpha_bytes_equivalent=alpha_bytes,
            strings_verified=len(source.strings), exact_native_glyph_payloads_and_metrics=True,
            exact_utf8_and_string_indices=True, exact_artwork_and_animation_streams=True,
            artwork_sha256=tiles.sha256(data[first:end]), literal_boot_marker=True))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ('input', 'output', 'receipt'):
        parser.add_argument('--'+option, type=Path, required=True)
    parser.add_argument('--order', choices=('physical', 'band-frequency'), default='band-frequency')
    parser.add_argument('--group-bytes', type=int, choices=group_glyphs.GROUP_CHOICES,
                        default=group_glyphs.DEFAULT_GROUP_BYTES)
    args = parser.parse_args()
    source = tiles.private_path(args.input, 'Private source locale')
    target = tiles.private_path(args.output, 'Private regrouped locale')
    receipt_path = tiles.private_path(args.receipt, 'Private regrouping receipt')
    tiles.require(len({source, target, receipt_path}) == 3, 'Input, output and receipt must differ')
    began = time.monotonic()
    result, receipt = repack(source.read_bytes(), order=args.order, group_bytes=args.group_bytes)
    receipt['elapsed_seconds'] = round(time.monotonic()-began, 3)
    tiles.atomic_bytes(target, result)
    tiles.atomic_bytes(receipt_path, json.dumps(receipt, ensure_ascii=False, indent=2).encode()+b'\n')
    print(json.dumps(receipt, ensure_ascii=False))


if __name__ == '__main__':
    main()
