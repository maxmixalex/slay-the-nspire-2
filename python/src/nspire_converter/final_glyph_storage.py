"""Store exact native glyph coverage in bounded, locality-ordered RGR1 groups.

RLE is a glyph representation, not a quality setting. Expanding it before LZMA2
lets neighboring alpha planes share matches and removes per-glyph RLE expansion.
Every coverage byte, metric, key, text, image and animation remains exact. Small
labels retain UI15's 4 KiB cold-read bound; larger bands use at most 32 KiB. This
uses existing raw glyphs, RGR1 and caches, with no runtime format or allocation.
"""

from __future__ import annotations

import hashlib
import json
import lzma
import struct
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from nspire_converter import compact_ui_fonts as writer
from nspire_converter import content_tiles as t
from nspire_converter import split_content as split
from nspire_converter.coalesce_tile_resources import source_lzma_profile, view
from nspire_converter.group_glyphs import Batch, Record

SMALL_BYTES = 4096
LARGE_BYTES = 32768
PROFILES = ((0, 1, 0), (0, 1, 1), (2, 1, 0), (3, 0, 2))
ENCODER = b"UI23-exact-native-alpha-small4-large32-extreme-v1"


class CoverageView:
    def __init__(self, original):
        self.original = original
        self.glyphs = [(*row[:6], 0) for row in original.glyphs]

    def __getattr__(self, name):
        return getattr(self.original, name)


def plan(source, cache, workers):
    records = []
    identical = {}
    frequency = Counter()
    old_profiles = set()
    for i in range(len(source.strings)):
        frequency.update(source.string_payload(i)[:-1].decode("utf-8"))
    # Read existing cohorts in physical order, then use the established
    # band/frequency order. Equal coverage may alias even when metrics differ.
    for i in sorted(range(len(source.glyphs)), key=lambda j: (source.glyphs[j][1], j)):
        row = source.glyphs[i]
        payload = source.glyph_alpha(i)
        t.require(len(payload) == row[3] * row[4], "Glyph coverage has a different size")
        if payload not in identical:
            identical[payload] = Record(payload)
            records.append(identical[payload])
        identical[payload].ids.append(i)
        if row[6] & t.GLYPH_GROUP_FLAG:
            relative, size, _at, _length = struct.unpack_from("<iIII", source.data, row[1])
            start = row[1] + relative
            old_profiles.add(source_lzma_profile(source.block(start + 32, size - 32)))
    context_limit = max((lc + lp for lc, lp, _pb in old_profiles), default=3)
    position_limit = max((pb for _lc, _lp, pb in old_profiles), default=2)
    profiles = tuple(p for p in PROFILES if sum(p[:2]) <= context_limit and p[2] <= position_limit)
    profiles = profiles or ((0, 0, 0),)
    records.sort(
        key=lambda record: min(
            (source.glyphs[i][2], -frequency[chr(source.glyphs[i][0])], source.glyphs[i][0], i)
            for i in record.ids
        )
    )
    batches = []
    pending = []
    size = 0
    limit = 0
    for record in records:
        target = (
            SMALL_BYTES
            if any((source.glyphs[i][2] & 31) <= 10 for i in record.ids)
            else LARGE_BYTES
        )
        if pending and (target != limit or size + len(record.payload) > target):
            batches.append((pending, size, limit))
            pending = []
            size = 0
        pending.append(record)
        size += len(record.payload)
        limit = target
    if pending:
        batches.append((pending, size, limit))
    cache = Path(cache)
    cache.mkdir(parents=True, exist_ok=True)

    def encode(item):
        records, size, limit = item
        raw = b"".join(record.payload for record in records)
        t.require(0 < size == len(raw) <= limit, "Glyph cohort exceeds its cold-read bound")
        identity = ENCODER + bytes(x for p in profiles for x in p) + raw
        path = cache / (hashlib.sha256(identity).hexdigest() + ".lzma2")
        if path.is_file():
            encoded = path.read_bytes()
        else:
            candidates = [
                lzma.compress(
                    raw,
                    format=lzma.FORMAT_RAW,
                    filters=[
                        dict(
                            id=lzma.FILTER_LZMA2,
                            preset=9 | lzma.PRESET_EXTREME,
                            dict_size=t.GROUP_LIMIT,
                            nice_len=273,
                            depth=1024,
                            lc=lc,
                            lp=lp,
                            pb=pb,
                        )
                    ],
                )
                for lc, lp, pb in profiles
            ]
            encoded = min(candidates, key=len)
            t.atomic_bytes(path, encoded)
        t.require(t.decode_lzma2(encoded, size) == raw, "Glyph coverage cache/codec differs")
        profile = source_lzma_profile(encoded)
        t.require(
            sum(profile[:2]) <= context_limit and profile[2] <= position_limit,
            "Glyph decoder probability workspace increased",
        )
        previous = sum((len(record.payload) + 1) & ~1 for record in records)
        cost = ((len(encoded) + 33) & ~1) + 16 * len(records)
        return Batch(records, encoded if cost < previous else None, size, previous), limit

    with ThreadPoolExecutor(max_workers=workers) as pool:
        encoded = list(pool.map(encode, batches))
    return encoded, dict(
        previous_max_lc_plus_lp=context_limit,
        previous_max_pb=position_limit,
        profiles=profiles,
        small_label_group_limit=SMALL_BYTES,
        large_label_group_limit=LARGE_BYTES,
        dictionary_bytes=t.GROUP_LIMIT,
        glyph_rle_expansions_removed=sum(
            bool(row[6] & ~t.GLYPH_GROUP_FLAG) for row in source.glyphs
        ),
    )


def verify(original, output):
    source, target = view(original), view(output)
    language = source.string_payload(split.MARKER)[:-1].decode("ascii")
    before, after = split.layout(original, language), split.layout(output, language)
    t.require(
        source.version == target.version
        and source.counts == target.counts
        and source.images == target.images
        and source.clips == target.clips
        and source.cards == target.cards
        and source.aux == target.aux,
        "Glyph storage changed artwork, identities or clocks",
    )
    begin, end = before["prefix"], before["prefix"] + before["artwork"]
    t.require(
        (begin, before["artwork"]) == (after["prefix"], after["artwork"])
        and original[begin:end] == output[begin:end]
        and original[36:40] == output[36:40],
        "Glyph storage changed artwork bytes or animation profile",
    )
    coverage = hashlib.sha256()
    alpha_bytes = 0
    for i, (a, b) in enumerate(zip(source.glyphs, target.glyphs)):
        t.require(a[:1] + a[2:6] == b[:1] + b[2:6], "Glyph key/size/bearing/advance changed")
        raw = source.glyph_alpha(i)
        t.require(raw == target.glyph_alpha(i), "Glyph alpha coverage changed")
        coverage.update(raw)
        alpha_bytes += len(raw)
    for i in range(len(source.strings)):
        t.require(source.string_payload(i) == target.string_payload(i), "Localized text changed")
    return dict(
        glyphs=len(source.glyphs),
        glyph_alpha_bytes_exact=alpha_bytes,
        coverage_sha256=coverage.hexdigest(),
        glyph_keys_metrics_coverage_exact=True,
        text_exact=len(source.strings),
        artwork_clocks_profile_exact=True,
    )


def convert(data, cache, workers=2):
    t.require(workers in (1, 2), "Use one or two bounded glyph workers")
    source = view(data)
    language = source.string_payload(split.MARKER)[:-1].decode("ascii")
    batches, profile = plan(source, cache, workers)
    output, statistics = writer.write(CoverageView(source), batches, split.layout(data, language))
    output = bytearray(output)
    output[36:40] = data[36:40]
    output = bytes(output)
    proof = verify(data, output)
    # The format choice must pay for its complete metadata and descriptors.
    selected = len(output) < len(data)
    result = output if selected else data
    return result, dict(
        schema=1,
        complete=True,
        language=language,
        input_bytes=len(data),
        candidate_bytes=len(output),
        output_bytes=len(result),
        saved_bytes=len(data) - len(result),
        selected=selected,
        input_sha256=t.sha256(data),
        output_sha256=t.sha256(result),
        resident_bytes_added=0,
        decoder_probability_workspace_not_increased=True,
        glyph_rle_representation_only=True,
        profile=profile,
        candidate_statistics=statistics,
        verification=proof,
        limitation="Exact storage proof; production cold-render acceptance is a separate release gate.",
    )


def main():
    import argparse

    parser = argparse.ArgumentParser(description=__doc__)
    for option in ("input", "output", "work"):
        parser.add_argument("--" + option, type=Path, required=True)
    parser.add_argument("--workers", type=int, choices=(1, 2), default=2)
    args = parser.parse_args()
    t.require(args.input.resolve() != args.output.resolve(), "Preserve original locale")
    result, proof = convert(args.input.read_bytes(), args.work, args.workers)
    t.atomic_bytes(args.output, result)
    t.atomic_bytes(args.output.with_suffix(".json"), (json.dumps(proof, indent=2) + "\n").encode())
    print(json.dumps(proof), flush=True)


if __name__ == "__main__":
    main()
