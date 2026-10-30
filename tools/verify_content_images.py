#!/usr/bin/env python3
"""Run the existing exact native image oracle in contiguous disk-bounded ranges."""
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import time

p = argparse.ArgumentParser()
p.add_argument('--binary', type=Path, required=True)
p.add_argument('--pack', type=Path, required=True)
p.add_argument('--reference', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--spool-mib', type=int, default=128)
p.add_argument('--workers', type=int, default=1)
a = p.parse_args()
if not 1 <= a.workers <= 4 or not 1 <= a.spool_mib <= 256:
    raise ValueError('Use one through four workers and 1..256 MiB per-image-range spool budgets')
a.output.mkdir(parents=True, exist_ok=True)
data = a.pack.read_bytes()
ni, ng, ns, nc, nk = struct.unpack_from('<5I', data, 16)
base = 40 + ni*16 + ng*16 + ns*8 + nc*16 + nk*16
while data[base:base+4] != b'TIL1':
    if base + 64 > min(len(data), 2*1024*1024):raise ValueError('Missing tile directory')
    base += 4
costs, images = struct.unpack_from('<II', data, base+56)
if images != ni or costs + ni > len(data):raise ValueError('Invalid decoded costs')
cap = a.spool_mib*1024*1024
ranges = []; first = size = 0
for image, pages in enumerate(data[costs:costs+ni]):
    # The existing pack table contains rounded decoded image byte lengths.
    # Reserve the absolute runtime image maximum for a missing/legacy entry.
    bound = (pages*4096 if pages else 320*240*8+2*240) + 16
    if size and size+bound > cap:
        ranges.append((first, image-first, size)); first = image; size = 0
    size += bound
if first < ni:ranges.append((first, ni-first, size))
receipt = dict(complete=False, images=ni, expected_spool_bound=cap, ranges=[],
    workers=a.workers, aggregate_spool_bound=cap*a.workers,
    pack=str(a.pack), pack_sha256=hashlib.sha256(data).hexdigest(),
    reference=str(a.reference), reference_sha256=hashlib.sha256(a.reference.read_bytes()).hexdigest(),
    binary_sha256=hashlib.sha256(a.binary.read_bytes()).hexdigest(),
    comparison='Every decoded byte via memory, ranged IO and incremental IO against original runtime spool')
del data
started = time.monotonic()


def run_range(interval):
    first, count, bound = interval
    log = a.output/f'images-{first:05}-{first+count-1:05}.log'
    command = ['/lib64/ld-linux-x86-64.so.2', str(a.binary), str(a.pack), str(a.reference),
               '--images', str(first), str(count), '--preload']
    print(json.dumps(dict(stage='native_exact_range_begin', first=first, count=count,
                          spool_bound=bound, log=str(log))), flush=True)
    before = time.monotonic()
    with log.open('w') as out:result = subprocess.run(command, stdout=out, stderr=subprocess.STDOUT)
    text = log.read_text()
    summary = re.search(r'(\d+) checks; (\d+) of (\d+) images \((\d+) decoded bytes\)', text)
    preload = re.search(r'incremental catalog: (\d+) images; (\d+) steps; maximum (\d+) steps', text)
    if result.returncode or not summary or not preload:
        raise ValueError(f'Incomplete native image range {first}/{count}: {result.returncode}; {log}')
    checks, measured, total, decoded = map(int, summary.groups())
    prepared, steps, maximum = map(int, preload.groups())
    if (measured, total, prepared) != (count, ni, count):raise ValueError('Native image range differs')
    if decoded+count*16 > bound:raise ValueError('Decoded byte cost exceeded its recorded bound')
    row = dict(first=first, count=count, decoded_bytes=decoded, checks=checks,
               preload_steps=steps, max_preload_steps=maximum, seconds=time.monotonic()-before,
               maximum_spool_bytes=decoded+count*16, log=str(log))
    return row


def write_receipt():
    temporary=a.output/'exact-image-receipt.tmp'
    temporary.write_text(json.dumps(receipt,indent=2)+'\n')
    temporary.replace(a.output/'exact-image-receipt.json')


# Each child constructs its own complete Content/cache and reads full groups.
# Prediction never crosses group boundaries; splitting the image catalog does
# not omit a predictor, cancellation phase or a decoder record from a child.
write_receipt()
with ThreadPoolExecutor(max_workers=a.workers) as pool:
    futures=[pool.submit(run_range,interval) for interval in ranges]
    for future in as_completed(futures):
        row=future.result();receipt['ranges'].append(row)
        receipt['ranges'].sort(key=lambda r:r['first'])
        write_receipt()
        print(json.dumps(dict(stage='native_exact_range_complete',**row)),flush=True)
covered=0
for row in receipt['ranges']:
    if row['first']!=covered or row['count']<=0:
        raise ValueError('Image catalog has a gap, overlap or empty interval')
    covered+=row['count']
if covered!=ni:raise ValueError('Incomplete image catalog')
receipt.update(complete=True, elapsed_seconds=time.monotonic()-started,
    decoded_bytes=sum(r['decoded_bytes'] for r in receipt['ranges']),
    checks=sum(r['checks'] for r in receipt['ranges']),
    preload_steps=sum(r['preload_steps'] for r in receipt['ranges']),
    max_preload_steps=max(r['max_preload_steps'] for r in receipt['ranges']),
    maximum_spool_bytes=max(r['maximum_spool_bytes'] for r in receipt['ranges']))
write_receipt()
print(json.dumps({k:v for k,v in receipt.items() if k!='ranges'}), flush=True)
