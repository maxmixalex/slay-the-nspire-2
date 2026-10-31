#!/usr/bin/env python3
"""Build the reduced-pose UI23 variant from complete private UI22 artwork.

Looping actors and scenery retain one exact source pose. Nonlooping actions
retain their first, middle and terminal poses (at most three). Every original
clip ID, sample count, frame clock, pivot and gameplay event remains unchanged:
discarded samples become descriptor aliases, not a shortened animation.
Registered actor/Vantom tables select the corresponding whole source pose.
Only unreachable image/index/tile/retained payloads are reclaimed.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import struct
import time

MAGIC = 0x3146414E  # NAF1, NSP2/46's retired checksum field at byte 36.
MINIMUM_VERSION = 46
VANTOM_FIRST, VANTOM_END = 1383, 1615
VANTOM_AUX, VANTOM_COUNT = 5288, 116
FRAMED_AUX, FRAMED_COUNT = 5422, 1784
OLD_FRAMED_CLIPS = (105, 123, 124, 125, 548, 706, 707, 1263, 1626, -2)
SELECTION_FIRST, SELECTION_END = 1705, 1710
ANCIENT_AUX, ANCIENT_COUNT = 7212, 2
ANCIENT_DURATIONS = (18000,16667)


def require(condition, message):
    if not condition:
        raise ValueError(message)


def reduced(data):
    return len(data) >= 40 and data[:4] == b'NSP2' and \
        struct.unpack_from('<I', data, 4)[0] >= MINIMUM_VERSION and \
        struct.unpack_from('<I', data, 36)[0] == MAGIC


def mark(data):
    require(len(data) >= 40 and data[:4] == b'NSP2' and
            struct.unpack_from('<I', data, 4)[0] >= MINIMUM_VERSION,
            'Reduced poses require NSP2/46 or newer')
    out = bytearray(data)
    struct.pack_into('<I', out, 36, MAGIC)
    return bytes(out)


def sample_map(count, loop):
    """Hold the latest retained pose; never display a death terminal early."""
    require(0 < count <= 256, 'Animation sample count is outside the runtime bound')
    samples = [0] if loop else sorted({0, count // 2, count - 1})
    return [max(sample for sample in samples if sample <= frame) for frame in range(count)]


def ancient_rows(source, image):
    """Validate the optional typed Ancient loop extension, not legacy clips."""
    if len(source.aux)==ANCIENT_AUX:
        require(len(source.images)==58613,'Unexpected images after the complete UI22 catalog')
        return []
    require(len(source.aux)==ANCIENT_AUX+ANCIENT_COUNT and len(source.images)>58613,
            'Unknown reduced-pose auxiliary extension')
    result=[];seen=set()
    for row,duration in enumerate(ANCIENT_DURATIONS):
        index=source.aux[ANCIENT_AUX+row]
        width,height,fmt,raw=image(index)
        require(fmt==0 and width*height==8 and len(raw)==16,'Ancient loop table must contain eight exact words')
        words=struct.unpack('<8H',raw)
        first,count,ms=words[2:5]
        require(words[:2]==(0x414e,1) and words[7]==0 and ms==33 and
                (words[5]|words[6]<<16)==duration and count==(duration+ms-1)//ms and
                first>=58613 and first+count<=len(source.images),'Invalid Ancient loop range or source clock')
        frames=set(range(first,first+count))
        require(not seen.intersection(frames) and index not in frames,'Ancient loop directories overlap')
        seen.update(frames)
        result.append(dict(table=index,first=first,frames=count,frame_ms=ms,duration_ms=duration))
    return result


def clip_policy():
    """Read semantic loop flags from the pinned public animator recipe."""
    from world_ui_profile import CLIP_ROWS
    root = Path(__file__).resolve().parents[1]
    jobs = {job['id']: job for job in CLIP_ROWS}
    loops = {clip: bool(row['loop']) for clip, row in jobs.items()}
    # Existing player, legacy Nibbit and camp directory identities.
    looping = {0, 4, 8, 9, 10, 11, 16, 17, 21, 22, 26, 27, 31, 32, 37}
    loops.update({clip: clip in looping for clip in range(38)})
    # Merchant/selection/rest/environment clips never affect action timing.
    for clip in range(1695, 1710):
        loops[clip] = True
    phobia = json.loads((root / 'config/phobia-ui7-0.17.json').read_text())
    for clip, row in enumerate(phobia['clips'], 1710):
        loops[clip] = loops[row['source_directory']]
    for clip in range(1742, 1753):
        loops[clip] = True
    for clip in range(1753, 1765):
        loops[clip] = False
    # Bestiary supplements must use exactly the same source pose as the body.
    sources = (179,203,227,228,263,286,298,310,842,866,890,913,926,949,973,1009,1021,1034)
    for clip, original in enumerate(sources, 1765):
        loops[clip] = loops[original]
    require(set(loops) == set(range(1783)), 'Reduced pose policy omits a clip')
    return loops


def framed_rows(raw):
    require(len(raw) >= 20 and len(raw) % 2 == 0, 'Truncated registered actor table')
    words = list(struct.unpack('<' + 'H' * (len(raw) // 2), raw))
    count, used = words[2], words[8]
    require(words[:2] == [0x4652, 1] and words[9] == 3 and 0 < count <= 256 and
            10 + count * 2 <= used <= len(words), 'Invalid registered actor table')
    rows = []
    for frame in range(count):
        at, layers = words[10 + frame * 2:12 + frame * 2]
        require(0 < layers <= 16 and at <= used and 3 * layers <= used - at,
                'Registered actor layer list escapes its table')
        rows.append([tuple(words[at + i * 3:at + (i + 1) * 3]) for i in range(layers)])
    return words, rows


def reduce_framed(raw, loop, first=None):
    words, rows = framed_rows(raw)
    mapping = sample_map(len(rows), loop)
    out = words[:10] + [0] * (2 * len(rows))
    before = {piece[0] for row in rows for piece in row}
    kept = set()
    for frame, selected in enumerate(mapping):
        pieces = list(rows[selected])
        if first is not None:
            require(rows[frame][0][0] == first + frame,
                    'Registered actor primary frame identity changed')
            # Keep stable primary IDs for existing release guards. Their
            # descriptors alias the selected primary; position comes from it.
            pieces[0] = (first + frame, *pieces[0][1:])
        out[10 + frame * 2:12 + frame * 2] = [len(out), len(pieces)]
        for piece in pieces:
            out.extend(piece)
        kept.update(piece[0] for piece in rows[selected])
    require(len(out) <= 65535, 'Registered actor table exceeds word offsets')
    out[8] = len(out)
    width = min(320, len(out)); height = (len(out) + width - 1) // width
    require(height <= 240, 'Registered actor table exceeds image bounds')
    out.extend([0] * (width * height - len(out)))
    return (width, height, 0, struct.pack('<' + 'H' * len(out), *out)), before, kept


def vantom_references(raw):
    require(len(raw) >= 24 and len(raw) % 2 == 0, 'Truncated Vantom table')
    words = list(struct.unpack('<' + 'H' * (len(raw) // 2), raw))
    frames, used = words[2], words[10]
    require(words[:2] == [0x5654, 3] and words[8:10] == [7, 12] and
            0 < frames <= 256 and 12 + 5 * frames <= used <= len(words), 'Invalid Vantom table')
    refs = set()
    for frame in range(frames):
        at, count = words[12 + frame * 5:14 + frame * 5]
        require(count <= 64 and at <= used and count * 7 <= used - at,
                'Vantom layers escape their parameter table')
        for part in range(count):
            row = words[at + part * 7:at + (part + 1) * 7]
            require(row[3] <= 7, 'Invalid Vantom layer mode')
            refs.add(row[0])
            if row[3] & 3 == 3:
                refs.add(row[1])
            refs.update(value for value in row[5:7] if value != 65535)
    return words, refs


def reduce_vantom(raw, loop):
    words, before = vantom_references(raw)
    original = words[12:12 + words[2] * 5]
    for frame, selected in enumerate(sample_map(words[2], loop)):
        words[12 + frame * 5:17 + frame * 5] = original[selected * 5:selected * 5 + 5]
    # Preserve emitter grids, layer lists and all pointers verbatim. Only the
    # frame table chooses a retained source pose, including matching emitters.
    output = struct.pack('<' + 'H' * len(words), *words)
    return output, before, vantom_references(output)[1]


def apply_aliases(data, aliases):
    """Alias complete descriptors and decoded costs, reading only originals."""
    import coalesce_tile_resources as coalesce
    import content_tiles as t
    source = coalesce.view(data)
    costs = t.GLOBAL.unpack_from(data, source.base_end)[14]
    out = bytearray(data)
    for image, target in aliases.items():
        require(0 <= image < len(source.images) and 0 <= target < len(source.images),
                'Reduced pose alias escapes the image catalog')
        out[40 + image * 16:56 + image * 16] = data[40 + target * 16:56 + target * 16]
        out[costs + image] = data[costs + target]
    return bytes(out)


def validate_profile(source, image):
    """Prove pose bounds/typed consistency while retaining UI22's full guard."""
    from retired_assets_ui22 import PLAN
    require(reduced(source.data), 'Reduced-pose header marker is missing')
    policy = clip_policy()
    protected = set(range(66)) | set(source.aux) | {row[0] for row in source.cards}
    retired = {row['id'] for row in PLAN['retired_clips']}
    decoded = set(); checked_clips = 0; framed_count = 0; vantom_count = 0
    def decode(index):
        require(0 <= index < len(source.images), 'Reduced pose references an absent image')
        if index not in decoded:
            image(index); decoded.add(index)
    # Typed roots have priority over the old whole-frame slots Vantom reuses.
    for row in range(VANTOM_COUNT):
        index = source.aux[VANTOM_AUX + row]
        _w, _h, fmt, raw = image(index)
        require(fmt == 0, 'Reduced Vantom parameters are not exact words')
        words, refs = vantom_references(raw)
        protected.update(refs)
        # Each of the116 rows belongs to at least one full-size/shrunken clip.
        import re
        header = (Path(__file__).resolve().parents[1] / 'include/vantom_ui15_resources.hpp').read_text()
        values = re.search(r'VantomUi15ClipRows\[\]\s*=\s*\{([^}]+)\}', header)
        require(values is not None, 'Missing Vantom mapping')
        mapping = [int(value) & 127 for value in re.findall(r'\d+', values[1])]
        clip = VANTOM_FIRST + mapping.index(row)
        expected = sample_map(words[2], policy[clip])
        frames = [tuple(words[12+frame*5:17+frame*5]) for frame in range(words[2])]
        require(all(frame == frames[selected] for frame, selected in zip(frames, expected)),
                'Reduced Vantom table exceeds its retained pose schedule')
        for value in refs:
            decode(value)
        vantom_count += 1
    for clip, (first, count, _ms, *_registration) in enumerate(source.clips):
        if clip in retired or VANTOM_FIRST <= clip < VANTOM_END:
            continue
        mapping = sample_map(count, policy[clip])
        for frame, selected in enumerate(mapping):
            index, target = first + frame, first + selected
            if index not in protected:
                require(source.images[index] == source.images[target],
                        f'Reduced clip {clip} frame {frame} is not its selected pose')
            if frame == selected:
                decode(target)
        checked_clips += 1
    framed = [(source.aux[5409+row], clip) for row, clip in enumerate(OLD_FRAMED_CLIPS)]
    framed.extend((source.aux[FRAMED_AUX+row], row if row < 1783 else -2)
                  for row in range(FRAMED_COUNT) if source.aux[FRAMED_AUX+row])
    for index, clip in framed:
        _w, _h, fmt, raw = image(index)
        require(fmt == 0, 'Reduced actor parameters are not exact words')
        _words, rows = framed_rows(raw)
        mapping = sample_map(len(rows), clip == -2 or policy[clip])
        def pose(row):
            return tuple((source.images[picture], x, y) for picture, x, y in row)
        for frame, selected in enumerate(mapping):
            require(pose(rows[frame]) == pose(rows[selected]),
                    f'Reduced actor {clip} has inconsistent registration/pose')
            if frame == selected:
                for picture, _x, _y in rows[frame]:
                    decode(picture)
        framed_count += 1
    ancients=ancient_rows(source,image)
    for row in ancients:
        first=row['first']
        require(all(source.images[index]==source.images[first]
                    for index in range(first,first+row['frames'])),
                'Reduced Ancient loop must alias its opaque first source pose')
        decode(first)
    return dict(complete=True, profile='reduced-poses', checked_clips=checked_clips,
        registered_tables=framed_count, vantom_tables=vantom_count,
        ancient_loops=len(ancients),
        decoded_representative_images=len(decoded), at_most_three_action_poses=True,
        original_counts_clocks_and_terminal_schedule=True)


def prepare(data, work):
    """Create exact pose aliases and typed table replacements; do no compression."""
    import coalesce_tile_resources as coalesce
    import content_tiles as t
    from ui12_resources import replace_images
    from retired_assets_ui22 import PLAN
    source = coalesce.view(data)
    require(source.version >= MINIMUM_VERSION and len(source.clips) == 1783 and
            len(source.images) >= 58613 and len(source.aux) in (7212,7214),
            'Expected the complete matching UI22 resource catalog')
    require(not reduced(data), 'The input already uses the reduced-pose profile')
    work = Path(work); work.mkdir(parents=True, exist_ok=True)
    policy = clip_policy(); reader = t.TileReader(source)
    def image(index):
        return reader.image_raw(index) if source.images[index][3] == 6 else source.image_raw(index)
    ancients=ancient_rows(source,image)
    replacements = {}; typed_before = set(); typed_after = set(); protected_vantom = set(); tables = []
    def save(index, row):
        w, h, fmt, raw = row
        path = work / f'parameters-{index}.raw'; t.atomic_bytes(path, raw)
        replacements[index] = dict(file=str(path), width=w, height=h, format=fmt,
            bytes=len(raw), sha256=t.sha256(raw), raw_sha256=t.sha256(raw))
    # C++'s row map identifies shared full-size/shrunken Vantom tables.
    import re
    header = (Path(__file__).resolve().parents[1] / 'include/vantom_ui15_resources.hpp').read_text()
    match = re.search(r'VantomUi15ClipRows\[\]\s*=\s*\{([^}]+)\}', header)
    require(match is not None, 'Missing Vantom table mapping')
    rows = [int(value) for value in re.findall(r'\d+', match[1])]
    require(len(rows) == VANTOM_END - VANTOM_FIRST, 'Vantom clip mapping changed')
    table_policy = {}
    for clip, row in enumerate(rows, VANTOM_FIRST):
        key = row & 127
        require(key not in table_policy or table_policy[key] == policy[clip],
                'Shared Vantom table has conflicting loop semantics')
        table_policy[key] = policy[clip]
    for row in range(VANTOM_COUNT):
        index = source.aux[VANTOM_AUX + row]; w, h, fmt, raw = image(index)
        require(fmt == 0, 'Vantom parameters must remain exact words')
        new, before, after = reduce_vantom(raw, table_policy[row])
        save(index, (w, h, fmt, new)); protected_vantom.update(before)
        typed_before.update(before); typed_after.update(after)
        tables.append(dict(kind='vantom', image=index, poses=len(set(sample_map(struct.unpack_from('<H', raw, 4)[0], table_policy[row])))))
    framed = [(source.aux[5409 + row], clip) for row, clip in enumerate(OLD_FRAMED_CLIPS)]
    framed.extend((source.aux[FRAMED_AUX + row], row if row < 1783 else -2)
                  for row in range(FRAMED_COUNT) if source.aux[FRAMED_AUX + row])
    for index, clip in framed:
        _w, _h, fmt, raw = image(index)
        require(fmt == 0, 'Actor parameters must remain exact words')
        loop = clip == -2 or policy[clip]
        first = None if clip == -2 else source.clips[clip][0]
        new, before, after = reduce_framed(raw, loop, first)
        save(index, new); typed_before.update(before); typed_after.update(after)
        tables.append(dict(kind='framed', image=index, clip=clip, poses=len(set(sample_map(struct.unpack_from('<H', raw, 4)[0], loop)))))
    aliases = {}; clips = []; retired = {row['id'] for row in PLAN['retired_clips']}
    static = set(range(66)) | set(source.aux) | {row[0] for row in source.cards}
    direct = set(static)
    for clip, (first, count, ms, *_registration) in enumerate(source.clips):
        if VANTOM_FIRST <= clip < VANTOM_END:
            continue  # These legacy slots are now typed mesh/texture assets.
        direct.update(range(first, first + count))
        if clip in retired:
            continue
        mapping = sample_map(count, policy[clip])
        for frame, selected in enumerate(mapping):
            index, target = first + frame, first + selected
            if index in static or index in protected_vantom:
                continue
            require(index not in aliases or aliases[index] == target, 'Overlapping clip aliases conflict')
            aliases[index] = target
        clips.append(dict(clip=clip, loop=policy[clip], frames=count, frame_ms=ms,
                          kept_samples=sorted(set(mapping))))
    for row in ancients:
        for index in range(row['first'],row['first']+row['frames']):
            require(index not in aliases and index not in static,'Ancient pose overlaps an established resource')
            aliases[index]=row['first']
        tables.append(dict(kind='ancient',image=row['table'],poses=1,**row))
    # Only formerly typed resources proved unreachable by both old/new roots
    # are retired. Unknown images and every directly addressed asset stay.
    discarded = typed_before - typed_after - direct
    for index in sorted(discarded):
        require(index not in replacements, 'A live table cannot be retired')
        _at, w, h, _fmt, _size = source.images[index]
        save(index, (w, h, 4, bytes(2 * h)))
    # Preserve source parameter values before aliasing/replacing their images.
    aliased = apply_aliases(data, aliases)
    output, replacement = replace_images(aliased, {}, raw_replacements=replacements)
    target = coalesce.view(output)
    require(target.clips == source.clips and target.cards == source.cards and target.aux == source.aux,
            'Reduced poses changed gameplay clocks or stable catalog identities')
    report = dict(schema=1, profile='reduced-poses', header_marker='NAF1',
        source_sha256=t.sha256(data), aliases={str(k): v for k, v in aliases.items() if k != v},
        retired_typed_images=sorted(discarded), replacements=sorted(replacements),
        clips=clips, tables=tables, replacement=replacement,
        original_clip_counts_clocks_pivots_unchanged=True, exact_retained_source_pixels=True,
        loops_keep_one_pose=True, actions_keep_at_most_three_poses=True,
        terminal_pose_not_shown_before_original_terminal_sample=True)
    return output, report


def convert(data, work, *, workers=2):
    """Reduce poses, reclaim unreachable bytes, and prove remaining content."""
    import coalesce_tile_resources as coalesce
    import compact_tile_slots as slots
    import compact_retained_slices as retained
    import content_tiles as t
    from prune_tile_directory_ui13 import prune, verify as verify_prune
    from retire_legacy_resources_ui22 import encode_same
    require(1 <= workers <= 2, 'Use one or two converter workers')
    t.HOST_CACHE_GROUPS = 16
    work = Path(work); started = time.monotonic()
    prepared, report = prepare(data, work / 'poses')
    t.atomic_bytes(work / 'reduced-poses-prepared.nsp', mark(prepared))
    print(json.dumps(dict(stage='reduced_poses_prepared', aliases=len(report['aliases']),
                          retired_typed=len(report['retired_typed_images']))), flush=True)
    pruned, pruning = prune(prepared)
    pruning['verification'] = verify_prune(coalesce.view(prepared), pruned)
    plan, mapping, keep, compacted, compaction = slots.make_plan(
        coalesce.view(pruned), work / 'tiles', workers=workers, encode_group=encode_same)
    compact, _ = coalesce.rewrite(pruned, plan, version=coalesce.view(pruned).version)
    compaction['verification'] = coalesce.verify(pruned, compact, plan, mapping, keep, None,
        pixel_groups=True, compact_slots=compacted)
    old = coalesce.view(compact)
    choices = retained.make_plan(old, work / 'retained', workers=workers, preserve_profile=True)
    result = retained.rewrite(old, choices)
    retained_proof = retained.verify(old, result, choices)
    result = mark(result)
    require(len(result) < len(data), 'Reduced-pose output did not reduce installed bytes')
    report.update(complete=True, input_bytes=len(data), output_bytes=len(result),
        saved_bytes=len(data)-len(result), output_sha256=t.sha256(result),
        pruning=pruning, compaction=compaction, retained_verification=retained_proof,
        elapsed_seconds=round(time.monotonic()-started, 3))
    return result, report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('input', 'output', 'work'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--workers', type=int, choices=(1, 2), default=2)
    args = parser.parse_args()
    import content_tiles as t
    source = t.private_path(args.input, 'Private source artwork')
    output = t.private_path(args.output, 'Private reduced artwork')
    work = t.private_path(args.work, 'Private conversion work')
    require(source != output, 'Never overwrite the original artwork')
    result, report = convert(source.read_bytes(), work, workers=args.workers)
    t.atomic_bytes(output, result)
    t.atomic_bytes(work / 'no-animations-receipt.json', json.dumps(report, indent=2).encode()+b'\n')
    print(json.dumps({key: report[key] for key in ('complete', 'input_bytes', 'output_bytes',
        'saved_bytes', 'output_sha256', 'elapsed_seconds')}), flush=True)


if __name__ == '__main__':
    main()
