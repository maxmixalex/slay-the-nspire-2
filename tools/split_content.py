#!/usr/bin/env python3
"""Separate private NSP2/17..39 artwork from locale metadata, glyphs and text.

The virtual concatenation prefix + artwork + suffix is byte-identical to each
input. No image/animation is decoded, recompressed, resampled or omitted. This
is a storage/installation change; the runtime reads and decodes requested payloads on demand.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import lzma
import os
from pathlib import Path
import struct
from tempfile import TemporaryDirectory
from retained_filter import group_layout as retained_group_layout

ROOT = Path(__file__).resolve().parents[1]
REFERENCE = 0x41CEF1EA
FILE_LIMIT = 256 * 1024 * 1024
DIRECTORY_LIMIT = 2 * 1024 * 1024
MARKER = 6110
COMMON_HEADER = 64
LANGUAGE_HEADER = 96
COMPRESSED_LANGUAGE_HEADER = 128
DIRECTORY_BLOCK_BYTES = 65536
DIRECTORY_ALIGNMENT = 4096
DIRECTORY_LITERAL = 0x80000000
LANGUAGES = {code: index for index, code in enumerate((
    "eng", "zhs", "zht", "deu", "esp", "fra", "ind", "ita",
    "jpn", "kor", "pol", "ptb", "rus", "spa", "tha", "tur"), 1)}


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
    require(magic == b"NSP2" and version in (17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43) and reference == REFERENCE,
            "Expected pinned NSP2/17 through NSP2/40")
    require(length == len(data), "Input length differs")
    require(0 < ni <= 65535 and 0 < ng <= (65535 if version >= 35 else 32768) and MARKER < ns <= 16384 and
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
        transformed = group[0] == b"TGR2" and version >= 33 and i < tile[5] and group[1] == 1 and group[7] in (1, 2)
        predicted = group[0] == b"TGI2" and version >= 41 and i >= tile[5] and group[1] == 2 and group[7] == 1
        require((transformed or predicted or (group[0] == b"TGR1" and group[7] == 0)) and
                group[2] == i and group[4] == count - 32, "Invalid compressed animation group header")
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
                raw_size, _start, _mode, _slices = retained_group_layout(data[group_at:group_at + group_size], version)
                require(shape >> 24 <= 4 and 0 < (shape & 0xffffff) <= raw_size and
                        raw_at <= raw_size - (shape & 0xffffff), "Invalid retained group/slice")
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
        grouped = encoded & 0x8000
        require(not grouped or version >= 32, "Grouped glyph requires NSP2/32")
        original_encoded = encoded & 0x7fff
        count = original_encoded or w * h
        require(not original_encoded or original_encoded % 2 == 0 and original_encoded <= 2 * w * h,
                "Invalid glyph run length")
        require(cp <= 0x10FFFF and 0 < w <= 40 and 0 < h <= 40 and
                0 < px <= (255 if version >= 24 else 32) and
                0 < (px & (31 if version >= 28 else 63)) <= 32 and
                (advance <= 40 or (version >= 40 and
                 ((advance & 0x8000 and (advance & 4095) <= 2560) or
                  (0x100000 <= cp < 0x100000+95*95 and (advance & 0xf000) == 0x4000)))) and
                end <= off <= length - (16 if grouped else count),
                "Glyphs must be after the artwork region")
        if grouped:
            relative, group_size, raw_at, raw_bytes = struct.unpack_from('<iIII', data, off)
            group_at = off + relative
            require(32 < group_size <= 617323 and end <= group_at <= length - group_size and
                    raw_bytes == count, "Glyph group must be after artwork with an exact slice length")
            group = struct.unpack_from('<4s7I', data, group_at)
            require(group[0] == b'RGR1' and 0 < group[1] <= 65536 and
                    group[2] == group_size - 32 and group[3] == 14 and
                    0 < group[4] <= 32768 and group[5:] == (0, 0, 0) and
                    raw_at <= group[1] and raw_bytes <= group[1] - raw_at,
                    "Invalid glyph group header/slice")
    marker = None
    for i in range(ns):
        off, stored = struct.unpack_from("<II", data, string_at + i * 8)
        grouped, count = stored & 0x80000000, stored & 0x7fffffff
        require(not grouped or version >= 33, "Grouped string requires NSP2/33")
        require(count < 16384 and end <= off <= length - (16 if grouped else count + 1),
                "Text must be after artwork within its byte bound")
        if grouped:
            require(i != MARKER, "Locale marker must remain ungrouped")
            relative, group_size, raw_at, raw_bytes = struct.unpack_from('<iIII', data, off)
            group_at = off + relative
            require(32 < group_size <= 617323 and end <= group_at <= length - group_size and
                    raw_bytes == count + 1 and (group_at + group_size <= off or off + 16 <= group_at),
                    "Invalid grouped string descriptor")
            group = struct.unpack_from('<4s7I', data, group_at)
            require(group[0] == b'RGR1' and 0 < group[1] <= 32768 and
                    group[2] == group_size - 32 and group[3] == 14 and
                    0 < group[4] <= 16384 and group[5:] == (0, 0, 0) and
                    raw_at <= group[1] and raw_bytes <= group[1] - raw_at,
                    "Invalid string group header/slice")
        else:
            require(data[off + count] == 0, "Text must be terminated")
        if i == MARKER:
            marker = bytes(data[off:off + count])
    require(marker == language.encode("ascii"), "Language label does not match the NSP2 marker")
    return dict(version=version, prefix=begin, artwork=end - begin, suffix=length - end,
                directory_raw_bytes=descriptors & ~(DIRECTORY_ALIGNMENT - 1),
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


def _language_header(localized):
    require(len(localized) >= LANGUAGE_HEADER, "Truncated language container")
    header = struct.unpack_from("<4s11I32s16s", localized)
    compressed = header[0] == b"NSL2"
    expected = (b"NSL2", 2, REFERENCE, COMPRESSED_LANGUAGE_HEADER) if compressed else \
               (b"NSL1", 1, REFERENCE, LANGUAGE_HEADER)
    require(header[:4] == expected and header[4] == len(localized) and
            header[9] in LANGUAGES.values(), "Invalid language header")
    total, prefix, art, suffix = header[5:9]
    require(total <= FILE_LIMIT and prefix >= 40 and art > 0 and suffix > 0 and
            prefix + art + suffix == total, "Invalid virtual language bounds")
    if not compressed:
        require(LANGUAGE_HEADER + prefix + suffix == len(localized), "Invalid raw locale length")
        return header, None
    require(len(localized) >= COMPRESSED_LANGUAGE_HEADER, "Truncated NSL2 extension")
    raw, block, count, table, data_at, tail, prop, reserved = struct.unpack_from("<8I", localized, 96)
    require(DIRECTORY_ALIGNMENT <= raw <= min(prefix, DIRECTORY_LIMIT) and raw % DIRECTORY_ALIGNMENT == 0 and
            block == DIRECTORY_BLOCK_BYTES and
            count == (raw + block - 1) // block and 1 <= count <= 32 and
            table == COMPRESSED_LANGUAGE_HEADER and data_at == table + count * 8 and
            data_at <= tail <= len(localized) and prop == 14 and reserved == 0 and
            tail + prefix - raw + suffix == len(localized), "Invalid compressed directory bounds")
    cursor, entries = data_at, []
    for index in range(count):
        off, stored = struct.unpack_from("<II", localized, table + index * 8)
        literal, size = bool(stored & DIRECTORY_LITERAL), stored & ~DIRECTORY_LITERAL
        raw_size = min(block, raw - index * block)
        require(off == cursor and 0 < size <= tail - off and
                (size == raw_size if literal else size < raw_size),
                "Invalid compressed directory block")
        entries.append((off, size, raw_size, literal))
        cursor += size
    # NSL3 stores independent 4 KiB pages around this container. A bounded
    # zero gap may align the raw glyph/string suffix to its virtual NSP2
    # page boundary, so a normal cache miss still decodes just one page.
    # Older NSL2 files have no gap and keep their original addressing.
    padding = tail - cursor
    require(0 <= padding < DIRECTORY_ALIGNMENT and
            (not padding or (tail - art) % DIRECTORY_ALIGNMENT == 0) and
            not any(localized[cursor:tail]), "Invalid compressed directory alignment padding")
    return header, dict(raw=raw, tail=tail, entries=entries,
                        directory_end=cursor, alignment_padding=padding)


def _decode_directory_block(localized, entry):
    off, size, raw_size, literal = entry
    encoded = localized[off:off + size]
    if literal:
        return bytes(encoded)
    try:
        decoder = lzma.LZMADecompressor(format=lzma.FORMAT_RAW,
            filters=[dict(id=lzma.FILTER_LZMA2, dict_size=512 * 1024)])
        raw = decoder.decompress(encoded, max_length=raw_size + 1)
        require(len(raw) == raw_size and decoder.eof and not decoder.unused_data,
                "Compressed directory block has wrong output or trailing bytes")
        return raw
    except lzma.LZMAError as error:
        raise ValueError("Invalid compressed directory stream") from error


def _directory_boundary(prefix, total):
    require(len(prefix) >= 40 and prefix[:4] == b"NSP2" and u32(prefix, 8) == REFERENCE and
            u32(prefix, 12) == total, "Invalid logical NSP2 header")
    version = u32(prefix, 4)
    require(17 <= version <= 43, "Unsupported logical NSP2 version")
    ni, ng, ns, nc, nk = struct.unpack_from("<5I", prefix, 16)
    require(0 < ni <= 65535 and 0 < ng <= (65535 if version >= 35 else 32768) and
            MARKER < ns <= 16384 and 0 < nc <= 65535 and 0 < nk <= 65535,
            "Invalid logical NSP2 counts")
    base = 40 + (ni + ng + nc + nk) * 16 + ns * 8
    bound = min(len(prefix), DIRECTORY_LIMIT)
    start = base
    while prefix[base:base + 4] != b"TIL1":
        require(base + 4 <= bound and (base - start) // 4 < 65535 and u32(prefix, base) < ni,
                "Invalid directory auxiliary table")
        base += 4
    require(base > start and base + 64 <= bound, "Truncated logical tile header")
    tile = struct.unpack_from("<4s15I", prefix, base)
    groups, frames, descriptors, table = tile[5] + tile[6], tile[7], tile[8], tile[9]
    costs = (ni + 1) & ~1 if version >= 20 else 0
    require(tile[1:4] == (1, 16, 14) and tile[10] == 770 and
            0 < groups <= 131070 and 0 < frames <= 65535 and table == base + 64 and
            descriptors == table + groups * 8 + costs and descriptors <= DIRECTORY_LIMIT and
            descriptors + frames * 16 <= len(prefix), "Invalid logical directory boundary")
    if version >= 20:
        require(tile[14] == table + groups * 8 and tile[15] == ni, "Invalid logical cost table")
    # Cold image decoding already owns the transfer Bytes lease. Its 4 KiB
    # descriptor-page reads must never overlap a compressed directory block.
    return descriptors & ~(DIRECTORY_ALIGNMENT - 1)


def _language_payload(localized, header, compressed):
    if compressed is None:
        return bytes(localized[LANGUAGE_HEADER:])
    directory = b"".join(_decode_directory_block(localized, entry) for entry in compressed["entries"])
    payload = directory + bytes(localized[compressed["tail"]:])
    require(_directory_boundary(payload[:header[6]], header[5]) == compressed["raw"],
            "Compressed bytes extend beyond the resident directory")
    return payload


def language_resource_version(localized):
    """Read the logical NSP2 header, decoding only the first NSL2 block."""
    if localized[:4] == b'NSL3':
        from storage_parts import expand_blocks
        localized = expand_blocks(localized)
    header, compressed = _language_header(localized)
    prefix = _decode_directory_block(localized, compressed["entries"][0]) if compressed else \
             localized[LANGUAGE_HEADER:LANGUAGE_HEADER + 40]
    require(len(prefix) >= 40 and prefix[:4] == b"NSP2" and u32(prefix, 8) == REFERENCE and
            u32(prefix, 12) == header[5] and 17 <= u32(prefix, 4) <= 43,
            "Invalid logical resource version")
    return u32(prefix, 4)


def compress_language_directory(localized, *, details=None):
    """Replace only the resident directory with bounded independent NSL2 blocks."""
    header, existing = _language_header(localized)
    if existing is not None:
        _language_payload(localized, header, existing)
        if details is not None:
            details.update(existing_nsl2=True, saved_bytes=0)
        return bytes(localized)
    prefix = localized[LANGUAGE_HEADER:LANGUAGE_HEADER + header[6]]
    directory_bytes = _directory_boundary(prefix, header[5])
    count = (directory_bytes + DIRECTORY_BLOCK_BYTES - 1) // DIRECTORY_BLOCK_BYTES
    data_at = COMPRESSED_LANGUAGE_HEADER + count * 8
    output = bytearray(data_at)
    output[:LANGUAGE_HEADER] = localized[:LANGUAGE_HEADER]
    rows = []
    for index in range(count):
        raw = bytes(prefix[index * DIRECTORY_BLOCK_BYTES:min((index + 1) * DIRECTORY_BLOCK_BYTES, directory_bytes)])
        best, selected = raw, "literal"
        for name, options in (("preset6", dict(preset=6)),
                              ("preset9-nice273-depth512", dict(preset=9, nice_len=273, depth=512))):
            encoded = lzma.compress(raw, format=lzma.FORMAT_RAW,
                filters=[dict(id=lzma.FILTER_LZMA2, dict_size=512 * 1024, lc=3, lp=0, pb=2, **options)])
            if len(encoded) < len(best):
                require(_decode_directory_block(encoded, (0, len(encoded), len(raw), False)) == raw,
                        "Directory compression changed bytes")
                best, selected = encoded, name
        literal = selected == "literal"
        struct.pack_into("<II", output, COMPRESSED_LANGUAGE_HEADER + index * 8,
                         len(output), len(best) | (DIRECTORY_LITERAL if literal else 0))
        output.extend(best)
        rows.append(dict(raw_bytes=len(raw), stored_bytes=len(best), encoder=selected,
                         raw_sha256=sha(raw), stored_sha256=sha(best)))
    tail_at = len(output)
    output.extend(localized[LANGUAGE_HEADER + directory_bytes:])
    struct.pack_into("<4sI", output, 0, b"NSL2", 2)
    struct.pack_into("<II", output, 12, COMPRESSED_LANGUAGE_HEADER, len(output))
    struct.pack_into("<8I", output, 96, directory_bytes, DIRECTORY_BLOCK_BYTES, count,
                     COMPRESSED_LANGUAGE_HEADER, data_at, tail_at, 14, 0)
    result = bytes(output)
    checked, compressed = _language_header(result)
    require(_language_payload(result, checked, compressed) == localized[LANGUAGE_HEADER:],
            "NSL2 changed the complete logical locale payload")
    if details is not None:
        details.update(directory_raw_bytes=directory_bytes, block_bytes=DIRECTORY_BLOCK_BYTES,
                       directory_alignment=DIRECTORY_ALIGNMENT,
                       blocks=rows, raw_frame_and_suffix_bytes=len(output) - tail_at,
                       input_bytes=len(localized), output_bytes=len(output),
                       saved_bytes=len(localized) - len(output), dictionary_bytes=512 * 1024,
                       lc=3, lp=0, pb=2, byte_identical_directory=True,
                       frame_descriptors_stay_raw=True, runtime_pixel_pools_added=0)
    return result


def align_language_suffix(localized, *, details=None):
    """Align NSL2's raw suffix for an outer independent-page container.

    The NSP2 suffix maps to physical ``tail + virtual - directory - artwork``.
    The directory is already page aligned, so matching tail and artwork
    phases avoids fetching/decompressing two pages for one ordinary cache
    miss. Only zero padding between directory streams and raw tail changes;
    the complete logical locale payload remains byte identical.
    """
    header, compressed = _language_header(localized)
    if compressed is None:
        if details is not None:
            details.update(suffix_page_aligned=False, alignment_padding_bytes=0)
        return bytes(localized)
    end = compressed["directory_end"]
    padding = (header[7] - end) % DIRECTORY_ALIGNMENT
    tail = end + padding
    output = bytearray(localized[:end])
    output.extend(bytes(padding))
    output.extend(localized[compressed["tail"]:])
    struct.pack_into("<I", output, 16, len(output))
    struct.pack_into("<I", output, 116, tail)
    result = bytes(output)
    checked, aligned = _language_header(result)
    require(_language_payload(result, checked, aligned) ==
            _language_payload(localized, header, compressed),
            "Suffix alignment changed the logical locale payload")
    if details is not None:
        details.update(suffix_page_aligned=True, suffix_page_bytes=DIRECTORY_ALIGNMENT,
                       alignment_padding_bytes=padding,
                       alignment_added_bytes=len(result)-len(localized))
    return result


def reconstruct(common, localized, language):
    """Independent container readback used by packaging and conversion tests."""
    if localized[:4] == b'NSL3':
        from storage_parts import expand_blocks
        localized = expand_blocks(localized)
    require(language in LANGUAGES and len(common) >= COMMON_HEADER and
            len(localized) >= LANGUAGE_HEADER, "Invalid split container size/language")
    c = struct.unpack_from("<4s7I32s", common)
    l, compressed = _language_header(localized)
    require(c[:4] == (b"NSC1", 1, REFERENCE, COMMON_HEADER) and c[4] == len(common) and
            c[5] == len(common) - COMMON_HEADER, "Invalid common header")
    require(l[9] == LANGUAGES[language], "Invalid language header")
    total, prefix, art, suffix = l[5:9]
    require(total <= FILE_LIMIT and prefix >= 40 and art > 0 and suffix > 0 and
            prefix + art + suffix == total and art == c[5],
            "Mismatched split bounds")
    payload = _language_payload(localized, l, compressed)
    result = payload[:prefix] + common[COMMON_HEADER:] + payload[prefix:]
    require(result[:4] == b"NSP2", "Reconstructed NSP2 tag differs")
    ni, ng = u32(result, 16), u32(result, 20)
    at = 40 + ni * 16 + ng * 16 + MARKER * 8
    off, count = struct.unpack_from("<II", result, at)
    require(count == 3 and result[off:off + 4] == language.encode() + b"\0", "Reconstructed locale differs")
    return result


def split(packs, *, compress_directory=False):
    require(bool(packs) and set(packs) <= set(LANGUAGES), "Expected supported original language inputs")
    rows = {language: layout(data, language) for language, data in packs.items()}
    first = next(iter(packs))
    reference = rows[first]
    artwork = memoryview(packs[first])[reference["prefix"]:reference["prefix"] + reference["artwork"]]
    for language, data in packs.items():
        row = rows[language]
        require(row["version"] == reference["version"] and row["artwork"] == len(artwork) and
                memoryview(data)[row["prefix"]:row["prefix"] + row["artwork"]] == artwork,
                "Language inputs do not have identical artwork; rebuild them together")
    common = common_file(artwork)
    languages = {language: language_file(data, rows[language], language, common[32:64])
                 for language, data in packs.items()}
    directory_receipts = {}
    if compress_directory:
        for language in languages:
            directory_receipts[language] = {}
            languages[language] = compress_language_directory(languages[language], details=directory_receipts[language])
    for language, localized in languages.items():
        require(reconstruct(common, localized, language) == packs[language],
                "Split storage changed the original NSP2 bytes")
    original = sum(len(p) for p in packs.values())
    stored = len(common) + sum(len(p) for p in languages.values())
    receipt = dict(schema=1, reference="v0.111.0 / 41cef1ea", format="NSC1/NSL2" if compress_directory else "NSC1/NSL1",
        complete=True, original_bytes=original, stored_bytes=stored, saved_bytes=original - stored,
        byte_identical_roundtrip=True, runtime_integrity_checks=False,
        common=dict(file="nSpire2.dat.tns", bytes=len(common), sha256=sha(common),
                    artwork_bytes=len(artwork), artwork_sha256=sha(artwork)),
        languages={language: dict(row, file=f"nSpire2.{language}.dat.tns", bytes=len(languages[language]),
                                  sha256=sha(languages[language])) for language, row in rows.items()})
    if compress_directory:
        receipt.update(directory_compression=directory_receipts,
                       directory_saved_bytes=sum(row["saved_bytes"] for row in directory_receipts.values()))
    return common, languages, receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--eng", type=Path, required=True)
    parser.add_argument("--zhs", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--compress-directory", action="store_true", help="Use bounded NSL2 directory blocks; keep all logical NSP2 and common bytes unchanged")
    args = parser.parse_args()
    destination = args.output.resolve()
    require(destination != ROOT and ROOT not in destination.parents,
            "Private derived resources must stay outside the source repository")
    input_paths = {args.eng.resolve(), args.zhs.resolve()}
    require(len(input_paths) == 2, "Languages must have separate source files")
    names = ["nSpire2.dat.tns", "nSpire2.eng.dat.tns", "nSpire2.zhs.dat.tns", "split-receipt.json"]
    require(not any((destination / name).resolve() in input_paths for name in names),
            "Outputs must not replace the input packs")
    common, languages, receipt = split({"eng": args.eng.read_bytes(), "zhs": args.zhs.read_bytes()},
                                       compress_directory=args.compress_directory)
    destination.mkdir(parents=True, exist_ok=True)
    with TemporaryDirectory(prefix=".split-", dir=destination) as temporary:
        stage = Path(temporary)
        (stage / names[0]).write_bytes(common)
        for language, data in languages.items():
            (stage / f"nSpire2.{language}.dat.tns").write_bytes(data)
        (stage / names[3]).write_text(json.dumps(receipt, ensure_ascii=False, indent=2) + "\n")
        for name in names:
            os.replace(stage / name, destination / name)
    print(json.dumps(receipt, sort_keys=True))


if __name__ == "__main__":
    main()
