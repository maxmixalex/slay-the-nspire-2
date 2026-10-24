#!/usr/bin/env python3
"""Separate private NSP2/17..21 artwork from locale metadata, glyphs and text.

The virtual concatenation prefix + artwork + suffix is byte-identical to each
input. No image/animation is decoded, recompressed, resampled or omitted. This
is a storage/installation change; the runtime reads and decodes requested payloads on demand.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from tempfile import TemporaryDirectory

ROOT = Path(__file__).resolve().parents[1]
REFERENCE = 0x41CEF1EA
FILE_LIMIT = 256 * 1024 * 1024
DIRECTORY_LIMIT = 2 * 1024 * 1024
MARKER = 6110
COMMON_HEADER = 64
LANGUAGE_HEADER = 96
LANGUAGES = {"eng": 1, "zhs": 2}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def fnv(data, seed=2166136261):
    for byte in data:
        seed = ((seed ^ byte) * 16777619) & 0xFFFFFFFF
    return seed


def sha(data):
    return hashlib.sha256(data).hexdigest()


def u32(data, offset):
    require(0 <= offset <= len(data) - 4, "Truncated integer")
    return struct.unpack_from("<I", data, offset)[0]


def layout(data, language):
    """Check semantic boundaries before assigning any byte to the common file."""
    require(language in LANGUAGES, "Unsupported language")
    require(40 <= len(data) <= FILE_LIMIT, "Invalid input size")
    magic, version, reference, length, ni, ng, ns, nc, nk, checksum = struct.unpack_from("<4s9I", data)
    require(magic == b"NSP2" and version in (17, 18, 19, 20, 21, 22) and reference == REFERENCE,
            "Expected pinned NSP2/17 through NSP2/22")
    require(length == len(data), "Input length differs")
    require(0 < ni <= 65535 and 0 < ng <= 32768 and MARKER < ns <= 16384 and
            0 < nc <= 65535 and 0 < nk <= 65535, "Invalid directory counts")
    glyph_at = 40 + ni * 16
    string_at = glyph_at + ng * 16
    aux_at = string_at + ns * 8 + nc * 16 + nk * 16
    require(aux_at < min(length, DIRECTORY_LIMIT), "Directory outside bound")
    # v18 appends auxiliary ids without changing the NSP2 header. A valid id
    # cannot equal the TIL1 magic, so the bounded scan is unambiguous.
    base = aux_at
    while data[base:base + 4] != b"TIL1":
        require(base + 4 <= min(length, DIRECTORY_LIMIT) and
                (base - aux_at) // 4 < 65535 and u32(data, base) < ni,
                "Invalid auxiliary catalog or missing TIL1")
        base += 4
    require(base > aux_at and base + 64 <= min(length, DIRECTORY_LIMIT),
            "Invalid tile header position")
    tile = struct.unpack_from("<4s15I", data, base)
    require(tile[1] == 1 and tile[2] == 16 and tile[3] == 14 and
            tile[10] == 770,
            "Unexpected TIL1 profile")
    groups = tile[5] + tile[6]
    descriptors, table, frames = tile[8], tile[9], tile[7]
    costs = (ni + 1) & ~1 if version >= 20 else 0
    if version >= 20:
        require(tile[14] == table + groups * 8 and tile[15] == ni, "Invalid decoded-cost table")
    require(0 < groups <= 131070 and 0 < frames <= 65535 and
            table == base + 64 and descriptors == table + groups * 8 + costs and
            descriptors <= DIRECTORY_LIMIT, "Tile directory outside bound")
    descriptor_end = descriptors + frames * 16
    require(descriptor_end <= length, "Truncated frame descriptors")
    artwork_ranges = []

    def graphic(offset, count):
        require(offset % 2 == 0 and count > 0 and descriptor_end <= offset <= length - count,
                "Artwork outside the payload section")
        artwork_ranges.append((offset, offset + count))

    for i in range(groups):
        off, count = struct.unpack_from("<II", data, table + i * 8)
        graphic(off, count)
        require(count >= 32, "Truncated compressed animation group")
        group = struct.unpack_from("<4s7I", data, off)
        require(group[0] == b"TGR1" and group[2] == i and group[4] == count - 32 and
                group[7] == 0, "Invalid compressed animation group header")
    for i in range(ni):
        off, width, height, fmt, count = struct.unpack_from("<IHHII", data, 40 + i * 16)
        require(0 < width <= 320 and 0 < height <= 240 and fmt <= (7 if version >= 20 else 6),
                "Invalid image geometry or encoding")
        if fmt == 6:
            require(count == 16 and descriptors <= off <= descriptor_end - 16 and
                    (off - descriptors) % 16 == 0 and u32(data, off) == base,
                    "Invalid frame descriptor")
        else:
            graphic(off, count)
            if fmt == 7:
                require(count == 16, "Invalid retained descriptor length")
                relative, group_size, raw_at, shape = struct.unpack_from("<iIII", data, off)
                group_at = off + relative
                graphic(group_at, group_size)
                require(group_size > 32, "Truncated retained group")
                group = struct.unpack_from("<4s7I", data, group_at)
                require(group[0] == b"RGR1" and 0 < group[1] <= 512 * 1024 and
                        group[2] == group_size - 32 and group[3] == 14 and shape >> 24 <= 4 and
                        0 < (shape & 0xffffff) <= group[1] and
                        raw_at <= group[1] - (shape & 0xffffff), "Invalid retained group/slice")
    require(artwork_ranges, "No artwork to share")
    begin = min(r[0] for r in artwork_ranges)
    end = max(r[1] for r in artwork_ranges)
    # Only alignment padding may occupy the spaces between actual art records.
    # In particular, never accidentally move text or font bytes into common.
    cursor = begin
    for first, last in sorted(set(artwork_ranges)):
        if first > cursor:
            require(not any(data[cursor:first]), "Non-artwork bytes inside common region")
        cursor = max(cursor, last)
    for i in range(ng):
        cp, off, px, w, h, advance, encoded = struct.unpack_from("<IIHBBHH", data, glyph_at + i * 16)
        count = encoded or w * h
        require(cp <= 0x10FFFF and 0 < w <= 40 and 0 < h <= 40 and
                0 < px <= 32 and advance <= 40 and end <= off <= length - count,
                "Glyphs must be after the artwork region")
    marker = None
    for i in range(ns):
        off, count = struct.unpack_from("<II", data, string_at + i * 8)
        require(count < 16384 and end <= off < length and count < length - off and
                data[off + count] == 0, "Text must be after artwork and terminated")
        if i == MARKER:
            marker = bytes(data[off:off + count])
    require(marker == language.encode("ascii"), "Language label does not match the NSP2 marker")
    return dict(version=version, prefix=begin, artwork=end - begin, suffix=length - end,
                original_bytes=length, original_fnv=checksum, original_sha256=sha(data),
                auxiliary=(base - aux_at) // 4, images=ni, glyphs=ng, strings=ns, clips=nc)


def common_file(artwork):
    digest = bytes(32)
    header = bytearray(struct.pack("<4s7I32s", b"NSC1", 1, REFERENCE, COMMON_HEADER,
                                  COMMON_HEADER + len(artwork), len(artwork), 0, 0, digest))
    return bytes(header) + bytes(artwork)


def language_file(original, row, language, common_digest):
    prefix, art, suffix = row["prefix"], row["artwork"], row["suffix"]
    header = bytearray(struct.pack("<4s11I32s16s", b"NSL1", 1, REFERENCE, LANGUAGE_HEADER,
        LANGUAGE_HEADER + prefix + suffix, len(original), prefix, art, suffix,
        LANGUAGES[language], row["original_fnv"], 0, common_digest, bytes(16)))
    return bytes(header) + bytes(original[:prefix]) + bytes(original[prefix + art:])


def reconstruct(common, localized, language):
    """Independent container readback used by packaging and conversion tests."""
    require(language in LANGUAGES and len(common) >= COMMON_HEADER and
            len(localized) >= LANGUAGE_HEADER, "Invalid split container size/language")
    c = struct.unpack_from("<4s7I32s", common)
    l = struct.unpack_from("<4s11I32s16s", localized)
    require(c[:4] == (b"NSC1", 1, REFERENCE, COMMON_HEADER) and c[4] == len(common) and
            c[5] == len(common) - COMMON_HEADER, "Invalid common header")
    require(l[:4] == (b"NSL1", 1, REFERENCE, LANGUAGE_HEADER) and l[4] == len(localized) and
            l[9] == LANGUAGES[language], "Invalid language header")
    total, prefix, art, suffix = l[5:9]
    require(total <= FILE_LIMIT and prefix >= 40 and art > 0 and suffix > 0 and
            prefix + art + suffix == total and art == c[5] and
            LANGUAGE_HEADER + prefix + suffix == len(localized),
            "Mismatched split bounds")
    result = localized[LANGUAGE_HEADER:LANGUAGE_HEADER + prefix] + common[COMMON_HEADER:] + localized[LANGUAGE_HEADER + prefix:]
    require(result[:4] == b"NSP2", "Reconstructed NSP2 tag differs")
    ni, ng = u32(result, 16), u32(result, 20)
    at = 40 + ni * 16 + ng * 16 + MARKER * 8
    off, count = struct.unpack_from("<II", result, at)
    require(count == 3 and result[off:off + 4] == language.encode() + b"\0", "Reconstructed locale differs")
    return result


def split(packs):
    require(set(packs) == set(LANGUAGES), "Both eng and zhs inputs are required")
    rows = {language: layout(data, language) for language, data in packs.items()}
    reference = rows["eng"]
    artwork = memoryview(packs["eng"])[reference["prefix"]:reference["prefix"] + reference["artwork"]]
    for language, data in packs.items():
        row = rows[language]
        require(row["version"] == reference["version"] and row["artwork"] == len(artwork) and
                memoryview(data)[row["prefix"]:row["prefix"] + row["artwork"]] == artwork,
                "Language inputs do not have identical artwork; rebuild them together")
    common = common_file(artwork)
    languages = {language: language_file(data, rows[language], language, common[32:64])
                 for language, data in packs.items()}
    for language, localized in languages.items():
        require(reconstruct(common, localized, language) == packs[language],
                "Split storage changed the original NSP2 bytes")
    original = sum(len(p) for p in packs.values())
    stored = len(common) + sum(len(p) for p in languages.values())
    receipt = dict(schema=1, reference="v0.111.0 / 41cef1ea", format="NSC1/NSL1",
        complete=True, original_bytes=original, stored_bytes=stored, saved_bytes=original - stored,
        byte_identical_roundtrip=True, runtime_integrity_checks=False,
        common=dict(file="nSpire2.common.tns", bytes=len(common), sha256=sha(common),
                    artwork_bytes=len(artwork), artwork_sha256=sha(artwork)),
        languages={language: dict(row, file=f"nSpire2.{language}.tns", bytes=len(languages[language]),
                                  sha256=sha(languages[language])) for language, row in rows.items()})
    return common, languages, receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--eng", type=Path, required=True)
    parser.add_argument("--zhs", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    destination = args.output.resolve()
    require(destination != ROOT and ROOT not in destination.parents,
            "Private derived resources must stay outside the source repository")
    input_paths = {args.eng.resolve(), args.zhs.resolve()}
    require(len(input_paths) == 2, "Languages must have separate source files")
    names = ["nSpire2.common.tns", "nSpire2.eng.tns", "nSpire2.zhs.tns", "split-receipt.json"]
    require(not any((destination / name).resolve() in input_paths for name in names),
            "Outputs must not replace the input packs")
    common, languages, receipt = split({"eng": args.eng.read_bytes(), "zhs": args.zhs.read_bytes()})
    destination.mkdir(parents=True, exist_ok=True)
    with TemporaryDirectory(prefix=".split-", dir=destination) as temporary:
        stage = Path(temporary)
        (stage / names[0]).write_bytes(common)
        for language, data in languages.items():
            (stage / f"nSpire2.{language}.tns").write_bytes(data)
        (stage / names[3]).write_text(json.dumps(receipt, ensure_ascii=False, indent=2) + "\n")
        for name in names:
            os.replace(stage / name, destination / name)
    print(json.dumps(receipt, sort_keys=True))


if __name__ == "__main__":
    main()
