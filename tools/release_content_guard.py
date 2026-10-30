"""Validate content required by a named release before writing player artifacts.

A format version describes a decoder, not which optional UI extensions exist.
UI20 requires actual checked framed tables and reward images; a legacy catalog
converted to the same wire format must not be published under that release name.
"""
from __future__ import annotations

import re
import struct

import content_tiles as tiles
from repack_retained import auxiliary_count

BASE_AUX = 5409
REQUIRED_AUX = 5422
CLIPS = (105, 123, 124, 125, 548, 706, 707, 1263, 1626)
CLOCKS = ((17, 78, 10177), (35, 83, 16149), (17, 82, 6647),
          (90, 83, 6144), (45, 83, 13987), (38, 81, 18428),
          (40, 82, 5678), (12, 83, 15124), (31, 81, 18264))
MAGIC = 0x4652


def validate_release_content(data, stage):
    match = re.match(r'ui([0-9]+)(?:$|[-_])', stage, re.IGNORECASE)
    if not match or int(match[1]) < 20:
        return None
    count = auxiliary_count(data)
    tiles.require(count >= REQUIRED_AUX,
                  'UI20 content is missing: expected framed actions, Osty dead_loop and reward icons (5422 auxiliaries)')
    source = tiles.PackView(data, count, versions=tuple(range(44, tiles.MAXIMUM_CONTENT_VERSION + 1)))
    tiles.require(len(source.clips) >= 1783 and len(source.images) >= 58099,
                  'UI20 complete image/clip catalog is missing')
    table_ids = source.aux[BASE_AUX:BASE_AUX + 10]
    reward_ids = source.aux[BASE_AUX + 10:REQUIRED_AUX]
    tiles.require(len(set(table_ids + reward_ids)) == 13, 'UI20 auxiliary entries must be distinct')
    forbidden = set(table_ids + reward_ids)
    reader = None
    checked_pixels = set()

    def image(image_id):
        nonlocal reader
        tiles.require(0 <= image_id < len(source.images), 'UI20 layer image is outside catalog')
        if source.images[image_id][3] == 6:
            if reader is None:
                reader = tiles.TileReader(source)
            return reader.image_raw(image_id)
        return source.image_raw(image_id)

    tables = []
    for row, image_id in enumerate(table_ids):
        width, height, fmt, raw = image(image_id)
        tiles.require(fmt == 0 and len(raw) == width * height * 2 and len(raw) >= 20,
                      'UI20 framed registry must be an exact word image')
        words = struct.unpack('<' + 'H' * (len(raw) // 2), raw)
        magic, version, frames, frame_ms = words[:4]
        units, reference = words[4] | words[5] << 16, words[6] | words[7] << 16
        used, stride = words[8:10]
        tiles.require(magic == MAGIC and version == 1 and stride == 3 and
                      0 < frames <= 256 and 0 < frame_ms <= 1000 and
                      0 < units < 1000000 and 0 < reference < 1000000 and
                      10 + 2 * frames <= used <= len(words), 'Invalid UI20 framed registry header')
        if row < len(CLIPS):
            clip = CLIPS[row]
            first, expected_frames, expected_ms, _x, _y, expected_reference = source.clips[clip]
            tiles.require((expected_frames, expected_ms, expected_reference) == CLOCKS[row],
                          'UI20 pinned enemy clip metadata differs')
            tiles.require((frames, frame_ms, units, reference) ==
                          (expected_frames, expected_ms, 5376, expected_reference),
                          'UI20 enemy frame clock or native registration differs')
        else:
            clip = -2
            tiles.require((frames, frame_ms, units, reference) == (3, 56, 4254, 4254),
                          'UI20 Osty dead_loop is missing or has the wrong clock')
        next_word = 10 + 2 * frames
        parts = 0
        for frame in range(frames):
            start, layers = words[10 + 2 * frame:12 + 2 * frame]
            tiles.require(start == next_word and 0 < layers <= 16 and start + 3 * layers <= used,
                          'UI20 framed layer range is invalid')
            if row < len(CLIPS):
                tiles.require(words[start] == first + frame,
                              'UI20 enemy frame does not retain its original image identity')
            for layer in range(layers):
                part = words[start + 3 * layer]
                tiles.require(part not in forbidden, 'UI20 frame refers to a registry or reward icon')
                if part not in checked_pixels:
                    image(part)  # Exercise the actual retained/tiled decoder, not just dimensions.
                    checked_pixels.add(part)
            next_word += 3 * layers
            parts += layers
        tiles.require(next_word == used and not any(words[used:]),
                      'UI20 framed registry has unclaimed or nonzero padding words')
        tables.append(dict(clip=clip, auxiliary=BASE_AUX + row, frames=frames,
                           frame_ms=frame_ms, parts=parts))
    rewards = []
    for name, image_id in zip(('rare', 'uncommon', 'special'), reward_ids):
        width, height, fmt, raw = image(image_id)
        tiles.require((width, height) == (23, 23), 'UI20 reward icon must use its native 23px size')
        rewards.append(dict(name=name, image=image_id, decoded_sha256=tiles.sha256(raw)))
    tiles.require(len({row['decoded_sha256'] for row in rewards}) == 3,
                  'UI20 reward icons must contain distinct artwork')
    return dict(complete=True, auxiliary_count=count, framed_tables=tables,
                checked_frame_images=len(checked_pixels), reward_icons=rewards)
