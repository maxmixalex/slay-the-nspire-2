#!/usr/bin/env python3
"""Import proven original-color frames and source rebakes in one exact retile.

Input manifests and raw spools are private source artifacts. Explicit source
rebakes take priority over historical color restoration, retaining later layout
and sampling corrections. The complete output is the same-quality tiled control
before selection geometry, unreachable-slot compaction and storage partitioning.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import time

import content_tiles as t
from group_ui7_hands import RowImages
from retile_appended_resources import view, compress_plan, rewrite
from retile_character_ui12 import make_plan, verify
from prune_tile_directory_ui13 import prune


class RawInputs:
    def __init__(self):
        self.rows, self.handles = {}, {}

    def add(self, row, root, provenance):
        image = int(row['image'])
        copied = dict(row, provenance=provenance)
        if 'file' in copied:
            path = Path(copied['file']); copied['file'] = str(path if path.is_absolute() else root/path)
        if 'spool' in copied:
            path = Path(copied['spool']); copied['spool'] = str(path if path.is_absolute() else root/path)
        self.rows[image] = copied

    def decode(self, image, original):
        row = self.rows.get(image)
        if row is None:
            return original
        if 'current_sha256' in row:
            t.require(t.sha256(original[3]) == row['current_sha256'],
                      'A historical restoration would overwrite newer source pixels')
        if 'spool' in row:
            path = row['spool']
            if path not in self.handles:
                self.handles[path] = Path(path).open('rb')
            handle = self.handles[path]; handle.seek(row['offset']); raw = handle.read(row['bytes'])
            width, height, fmt = row['width'], row['height'], row['format']
        else:
            payload = Path(row['file']).read_bytes()
            t.require(len(payload) == row['bytes'] and t.sha256(payload) == row['sha256'],
                      'A source replacement payload changed')
            width, height, fmt, raw = RowImages([row], [payload]).image_raw(0)
        t.require(len(raw) == row.get('raw_bytes', len(raw)) and t.sha256(raw) == row['raw_sha256'],
                  'Source replacement raw bytes changed')
        t.require(fmt == 4, 'Animation restoration requires original sparse pixel geometry')
        return width, height, fmt, raw

    def close(self):
        for handle in self.handles.values():
            handle.close()


def source_rows(args, source):
    inputs = RawInputs(); selected = set(); actor_images = set()
    if args.actor_receipt:
        actor = json.loads(args.actor_receipt.read_text())
        t.require(actor.get('complete') and actor['output_sha256'] == t.sha256(source.data),
                  'Actor receipt does not bind this source pack')
        actor_images.update(actor['replaced_images'])
        for clip in actor['clips']:
            actor_images.update(image for frame in clip['registered_frames'] for image, _x, _y in frame)
        selected.update(actor_images)
    restoration = json.loads(args.restoration.read_text())
    spool = restoration['original_spool']['file']
    restored = 0
    for row in restoration['restored_images']:
        if row['image'] in actor_images:
            continue
        inputs.add(dict(row, spool=spool), args.restoration.parent, 'verified-original-color')
        selected.add(row['image']); restored += 1
    for path in args.replacements:
        manifest = json.loads(path.read_text())
        rows = manifest if isinstance(manifest, list) else manifest.get('rows', manifest.get('images'))
        t.require(isinstance(rows, list), 'Source replacement manifest has no image rows')
        for row in rows:
            if isinstance(manifest, dict) and 'rows' in manifest:
                row = dict(row, spool=manifest['file'])
            inputs.add(row, path.parent, 'source-replacement:'+path.name); selected.add(row['image'])
    if args.necrobinder_selection:
        path = args.necrobinder_selection; manifest = json.loads(path.read_text())
        t.require(manifest['characters'] == ['necrobinder'] and len(manifest['images']) == 97 and
                  manifest['clips'][0]['count'] == source.clips[1709][1] == 96,
                  'Necrobinder original selection sample catalog changed')
        first = source.clips[1709][0]
        ids = [source.aux[1504], *range(first, first+96)]
        for image, row in zip(ids, manifest['images']):
            inputs.add(dict(row, image=image), path.parent, 'UI12-corrected-source-selection')
            selected.add(image)
    # Registered native-density source is newer than every historical and
    # derived restoration. Duplicate IDs must keep its corrected placement.
    for image in actor_images:
        inputs.rows.pop(image, None)
    t.require(selected and max(selected) < len(source.images) and min(selected) >= 0,
              'Exact replacement names an absent image')
    return inputs, selected, dict(restored_original_images=restored, rebaked_actor_images=len(actor_images),
                                  explicit_raw_replacements=len(inputs.rows))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--restoration', type=Path, required=True)
    parser.add_argument('--actor-receipt', type=Path)
    parser.add_argument('--replacements', type=Path, action='append', default=[])
    parser.add_argument('--necrobinder-selection', type=Path)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--workers', type=int, choices=(1,2), default=2)
    args = parser.parse_args()
    for path in (args.work, args.output):
        t.private_path(path, 'Private exact artwork path')
    args.work.mkdir(parents=True, exist_ok=True); started = time.monotonic()
    source = view(args.input.read_bytes())
    t.require(source.version == 45, 'Exact UI22 import requires the established NSP2/45 codec')
    inputs, selected, report = source_rows(args, source)
    try:
        plan = make_plan(source, args.work, selected, precision=8, raw_transform=inputs.decode)
    finally:
        inputs.close()
    t.atomic_bytes(args.work/'plan.json', json.dumps(plan, indent=2).encode()+b'\n')
    t.atomic_bytes(args.work/'provenance.json', json.dumps(inputs.rows, indent=2).encode()+b'\n')
    encoded = compress_plan(plan, args.work, args.work/'encoded', args.workers)
    output, sizes = rewrite(source, plan, encoded)
    proof = verify(source, output, plan, args.work)
    output, pruning = prune(output)
    result = dict(complete=True, precision='RGB565/alpha8 exact', **report, **sizes,
        verification=proof, prune=pruning, input_sha256=t.sha256(source.data), output_sha256=t.sha256(output),
        output_bytes=len(output), exact_sparse_bytes=plan['original_sparse_bytes'],
        selected_images=len(selected), recycled_tile_slots=plan['recycled_slots'],
        new_logical_tile_slots=plan['tiles']-plan['old_tiles'],
        seconds=round(time.monotonic()-started, 3))
    t.atomic_bytes(args.output, output)
    t.atomic_bytes(args.output.with_suffix('.json'), json.dumps(result, indent=2).encode()+b'\n')
    print(json.dumps(result), flush=True)


if __name__ == '__main__':
    main()
