"""Reclaim tile bytes made unreachable by exact artwork replacements.

Image IDs and all live index records remain stable. Only tiles with no reference
from any image descriptor become empty records. Keeping their numeric positions
avoids rebuilding unrelated animation streams or adding a new runtime codec.
"""
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import lzma
from pathlib import Path
import struct
import numpy as np

import content_tiles as tiles
from prune_tile_directory_ui13 import live_directory, prune, verify
from tile_filter import filter_tiles
from ui12_resources import view


def reachable_tiles(source):
    reader, descriptors, _ = live_directory(source)
    live = np.zeros(reader.tile_count, dtype=np.bool_)
    by_group = {}
    for _global, group, at, size in descriptors.values():by_group.setdefault(group, []).append((at, size))
    for group, records in by_group.items():
        raw, starts = reader.group(reader.tile_groups+group)
        for at, size in records:
            tiles.require(starts.get(at) == size, 'Live image does not name a complete index record')
            _decoded, index_bytes, _geometry = tiles.FRAME.unpack_from(raw, at)
            ids = np.frombuffer(raw, dtype='<u4', count=index_bytes//4, offset=at+tiles.FRAME.size)
            tiles.require(index_bytes%4 == 0 and np.all(ids < reader.tile_count), 'Invalid live tile ID')
            live[ids] = True
    return live


def reclaim(data, cache, workers=4):
    source = view(data);reader = tiles.TileReader(source);live = reachable_tiles(source)
    cache = Path(cache);cache.mkdir(parents=True, exist_ok=True)
    candidates = []
    for group, (at, size, raw_size, _crc, kind, items) in enumerate(reader.groups[:reader.tile_groups]):
        selected = live[group*reader.per_group:group*reader.per_group+items]
        if np.all(selected):continue
        candidates.append((group, selected.copy()))

    def convert(item):
        group, selected = item;at, size, raw_size, _crc, _kind, items = reader.groups[group]
        old = bytes(source.block(at-32, size+32));h = list(tiles.GROUP.unpack_from(old))
        encoded_raw = tiles.decode_lzma2(old[32:], raw_size)
        if reader.filters[group]:
            from tile_filter import restore_tiles
            raw = restore_tiles(encoded_raw, reader.filters[group])
        else:raw = encoded_raw
        records = np.frombuffer(raw, dtype='u1').reshape(items, tiles.TILE_BYTES).copy()
        records[~selected] = 0;records[~selected, :2] = 1
        updated = records.tobytes()
        if updated == raw:return group, None, 0
        for index in np.flatnonzero(selected):
            begin=int(index)*tiles.TILE_BYTES
            tiles.require(raw[begin:begin+tiles.TILE_BYTES] == updated[begin:begin+tiles.TILE_BYTES],
                          'Reachability reclamation changed a live tile')
        key = hashlib.sha256(b'UI14-unreachable-tiles-v1'+old+selected.tobytes()).hexdigest();path = cache/(key+'.bin')
        best = None
        if path.is_file():
            candidate = path.read_bytes();ch = tiles.GROUP.unpack_from(candidate)
            decoded = tiles.decode_lzma2(candidate[32:], raw_size)
            if ch[0] == b'TGR2':
                from tile_filter import restore_tiles
                decoded = restore_tiles(decoded, ch[7])
            tiles.require(decoded == updated and ch[1:4] == tuple(h[1:4]) and ch[6] == h[6],
                          'Cached reclaimed tiles differ')
            best = candidate
        else:
            modes = (0,) if not np.any(selected) else tuple(dict.fromkeys((reader.filters[group], 0)))
            for mode in modes:
                filtered = filter_tiles(updated, mode) if mode else updated
                for lc, lp, pb in ((4, 0, 1), (3, 1, 1)):
                    encoded = lzma.compress(filtered, format=lzma.FORMAT_RAW, filters=[dict(
                        id=lzma.FILTER_LZMA2, preset=9, dict_size=tiles.GROUP_LIMIT,
                        nice_len=273, depth=512, lc=lc, lp=lp, pb=pb)])
                    tiles.require(tiles.decode_lzma2(encoded, raw_size) == filtered, 'Reclaimed stream roundtrip differs')
                    header = h.copy();header[0] = b'TGR2' if mode else b'TGR1'
                    header[4] = len(encoded);header[5] = 0;header[7] = mode
                    candidate = tiles.GROUP.pack(*header)+encoded
                    if best is None or len(candidate) < len(best):best = candidate
            tiles.atomic_bytes(path, best)
        if ((len(best)+1)&~1) >= ((len(old)+1)&~1):return group, None, 0
        return group, best, len(old)-len(best)

    payloads = {};saved = 0
    with ThreadPoolExecutor(max_workers=workers) as pool:
        for index, (group, payload, reduction) in enumerate(pool.map(convert, candidates)):
            if payload is not None:payloads[group] = payload;saved += reduction
            if (index+1)%32 == 0:
                print(json.dumps(dict(stage='reclaim_tiles', completed=index+1,
                    total=len(candidates), saved_payload_bytes=saved)), flush=True)
    output, report = prune(data, tile_payloads=payloads)
    proof = verify(source, output, live_tiles=live)
    report.update(unreferenced_tiles=int(np.sum(~live)), reclaimed_groups=len(payloads),
                  all_live_tile_pixels_unchanged=True, **proof)
    return output, report
