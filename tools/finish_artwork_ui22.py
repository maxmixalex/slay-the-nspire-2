#!/usr/bin/env python3
"""Finish a full-precision UI22 artwork pack without changing its pixels.

Reclaim unreachable tile slots, preserve a compact same-quality control, add
bounded selection patches, and remove newly unreachable tile/retained bytes.
All phases retain the existing codec and the original font/text streams. The
result is ready for refresh_artwork_resources.py and its sixteen locale binds.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import time

import character_select_delta_ui22 as selection
import coalesce_tile_resources as coalesce
import compact_retained_slices as retained
import compact_tile_slots as slots
import content_tiles as t
import retile_appended_resources as retile
import retile_character_ui12 as dice


def compact_tiles(data, work, workers):
    source = coalesce.view(data)
    plan, mapping, keep, compacted, report = slots.make_plan(source, work, workers=workers)
    output, _ = coalesce.rewrite(data, plan, version=source.version)
    proof = coalesce.verify(data, output, plan, mapping, keep, None,
                            pixel_groups=True, compact_slots=compacted)
    # An extra index mapping can occasionally cost more than the removed
    # slots. Keep the complete original layout in that case.
    accepted = len(output) <= len(data)
    report.update(candidate_selected=accepted, verification=proof)
    return (output if accepted else data), report


def save(path, data, report):
    report.update(output_bytes=len(data), output_sha256=t.sha256(data))
    t.atomic_bytes(path, data)
    t.atomic_bytes(path.with_suffix('.json'), json.dumps(report, indent=2).encode()+b'\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('input', 'output', 'work'):
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--workers', type=int, choices=(1,2), default=2)
    args = parser.parse_args()
    for path in (args.output, args.work):
        t.private_path(path, 'Private finished artwork path')
    t.require(args.input.resolve() != args.output.resolve(), 'Never overwrite source artwork')
    args.work.mkdir(parents=True, exist_ok=True); started = time.monotonic()
    original = args.input.read_bytes(); source = coalesce.view(original)
    t.require(source.version == 45 and len(source.aux) == selection.BASE_AUXILIARY,
              'Finish the complete full-precision UI22 source catalog')
    control, control_report = compact_tiles(original, args.work/'control-slots', args.workers)
    save(args.work/'eng-compact-control7206.nsp', control, control_report)
    print(json.dumps(dict(stage='same_quality_control_complete', bytes=len(control),
                          sha256=t.sha256(control))), flush=True)
    geometry, geometry_report = selection.convert(control, args.work)
    save(args.work/'eng-selection7211.nsp', geometry, geometry_report)
    # The new spans no longer read many pixels from the old whole-frame
    # tiles. Dice their exact sparse output once more so those unreachable
    # colors can disappear, while every image_raw byte remains identical.
    patch_source = coalesce.view(geometry)
    selected = {image for first, count, *_ in patch_source.clips[
                    selection.FIRST_CLIP:selection.FIRST_CLIP+selection.CHARACTERS]
                for image in range(first, first+count)}
    patch_work = args.work/'patch-tiles'; patch_work.mkdir(exist_ok=True)
    patch_plan = dice.make_plan(patch_source, patch_work, selected, precision=8)
    patch_plan['compact_replaced_directory'] = True
    encoded = retile.compress_plan(patch_plan, patch_work, patch_work/'encoded', args.workers)
    patched, patch_report = retile.rewrite(patch_source, patch_plan, encoded)
    patch_report.update(verification=dice.verify(patch_source, patched, patch_plan, patch_work),
        exact_sparse_images=len(selected), reused_tile_references=patch_plan['reused_tile_references'],
        recycled_tile_slots=patch_plan['recycled_slots'],
        removed_only_pixels_outside_all_live_spans=True)
    save(args.work/'eng-selection-retiled7211.nsp', patched, patch_report)
    output, tile_report = compact_tiles(patched, args.work/'selection-slots', args.workers)
    old = coalesce.view(output)
    choices = retained.make_plan(old, args.work/'retained', workers=args.workers, preserve_profile=True)
    compacted = retained.rewrite(old, choices)
    retained_proof = retained.verify(old, compacted, choices)
    t.require(len(compacted) <= len(output), 'Retained reclamation increased disk bytes')
    retained_report = dict(groups=len(choices), changed_groups=sum(c.payload is not None for c in choices),
        saved_bytes=len(output)-len(compacted), raw_bytes_removed=sum(c.receipt['removed_raw_bytes'] for c in choices),
        existing_literal_profiles_preserved=True, verification=retained_proof,
        results=[c.receipt for c in choices])
    report = dict(complete=True, input_bytes=len(original), input_sha256=t.sha256(original),
        same_quality_control=control_report, selection=geometry_report, sparse_patch_retile=patch_report,
        tile_reclamation=tile_report,
        retained_reclamation=retained_report, runtime_arena_growth=0,
        no_frame_clock_color_alpha_or_locale_quantization=True,
        seconds=round(time.monotonic()-started, 3))
    save(args.output, compacted, report)
    print(json.dumps(dict(complete=True, input_bytes=len(original), output_bytes=len(compacted),
                          output_sha256=t.sha256(compacted), seconds=report['seconds'])), flush=True)


if __name__ == '__main__':
    main()
