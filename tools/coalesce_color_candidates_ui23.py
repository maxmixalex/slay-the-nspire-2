#!/usr/bin/env python3
"""Exactly deduplicate modern candidate tiles without adding a frame group.

All post-color pixel bytes are retained. Reuse is limited to pixel groups an
image already uses, with the established codec, dictionary and literal context.
This offline experiment neither changes rendering nor adds runtime allocations.
"""
import argparse
import json
from pathlib import Path
import time

import coalesce_tile_resources as c
import compact_tile_slots as compact
import content_tiles as t
from animation_color_candidates_ui23 import encode_like
from tile_filter import filter_tiles
from tile_prediction import index_transform


def encode_same(raw, original, reader, group, _cache):
    header = list(t.GROUP.unpack_from(original))
    t.require(not reader.sparse[group], 'Sparse pixel groups need a separate bounded candidate')
    if group < reader.tile_groups:
        transformed = filter_tiles(raw, reader.filters[group]) if reader.filters[group] else raw
    else:
        transformed = (index_transform(raw, header[6], 3, tile_count=reader.tile_count)
                       if reader.predictors[group] else raw)
    encoded = encode_like(transformed, original[32:], reader.dictionary)
    header[4], header[5] = len(encoded), 0
    payload = t.GROUP.pack(*header) + encoded
    return payload, payload + bytes((len(original)-len(payload)) & 1)


def convert(data, work, workers=2):
    source = c.view(data)
    plan, mapping, keep, original, report = c.make_plan(source, work/'coalesce',
        workers=workers, allow_compacted=True, encode_group=encode_same)
    output, _ = c.rewrite(data, plan, version=source.version)
    proof = c.verify(data, output, plan, mapping, keep, original)
    del original
    # Remove the newly unused slots too; this reduces actual group decode bytes.
    plan2, mapping2, referenced, slots, compact_report = compact.make_plan(c.view(output),
        work/'compact', workers=workers, encode_group=encode_same)
    final, _ = c.rewrite(output, plan2, version=source.version)
    t.require(len(final) <= len(output), 'Slot compaction increases the complete candidate')
    compact_proof = c.verify(output, final, plan2, mapping2, referenced, None,
                             compact_slots=slots)
    return final, dict(coalescing=report, coalescing_proof=proof,
                       compaction=compact_report, compaction_proof=compact_proof)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--workers', type=int, choices=(1,2), default=2)
    args = parser.parse_args(argv)
    t.require(args.input.resolve() != args.output.resolve(), 'Never overwrite source artwork')
    for path in (args.input,args.output,args.work):t.private_path(path,'Private exact candidate')
    args.work.mkdir(parents=True,exist_ok=True);args.output.parent.mkdir(parents=True,exist_ok=True)
    started=time.monotonic();data=args.input.read_bytes()
    output, details=convert(data,args.work,args.workers)
    t.atomic_bytes(args.output,output)
    receipt=dict(schema=1,complete=True,not_release_approved=True,input_bytes=len(data),
        output_bytes=len(output),saved_bytes=len(data)-len(output),input_sha256=t.sha256(data),
        output_sha256=t.sha256(output),seconds=time.monotonic()-started,
        exact_pixels_geometry_clocks=True,no_new_frame_group=True,resident_bytes_added=0,
        complete_decoder_performance_required=True,**details)
    t.atomic_bytes(args.output.with_suffix('.json'),(json.dumps(receipt,indent=2)+'\n').encode())
    print(json.dumps({k:v for k,v in receipt.items() if k not in details}),flush=True)


if __name__=='__main__':main()
