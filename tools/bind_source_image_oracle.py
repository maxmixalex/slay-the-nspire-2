#!/usr/bin/env python3
"""Bind a completed old-format byte oracle to a new lossless target pack.

This does not decode either pack or regenerate any image. It verifies the
old reference pack and byte spool hashes, keeps every oracle row/pixel byte,
and changes only the 28-byte native-test header's target size/checksum. The
receipt binds both complete packs and both spools. This permits an old reader
which has never implemented a new codec to remain the independent reference.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from tempfile import NamedTemporaryFile

HEADER = struct.Struct('<8s5I')
ROW = struct.Struct('<5I')
IMAGE = struct.Struct('<IHHII')
MAX_IMAGE_BYTES = 320 * 240 * 8 + 2 * 240
ROOT = Path(__file__).resolve().parents[1]


def require(ok, message):
    if not ok:
        raise ValueError(message)


def pack_identity(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as handle:
        header = handle.read(40)
        require(len(header) == 40, 'Truncated reference/target pack header')
        magic, version, reference, size, images, *_rest, checksum = struct.unpack('<4s9I', header)
        require(magic == b'NSP2' and reference == 0x41cef1ea and 0 < images <= 65535 and
                40 <= size <= 256 * 1024 * 1024, 'Invalid reference/target pack header')
        directory = handle.read(images * IMAGE.size)
        require(len(directory) == images * IMAGE.size, 'Truncated reference/target image directory')
        digest.update(header); digest.update(directory); actual = len(header) + len(directory)
        for block in iter(lambda: handle.read(1024 * 1024), b''):
            digest.update(block); actual += len(block)
    require(actual == size, 'Reference/target pack size differs from its header')
    shapes = []
    for at in range(0, len(directory), IMAGE.size):
        _offset, width, height, fmt, _length = IMAGE.unpack_from(directory, at)
        require(0 < width <= 320 and 0 < height <= 240 and fmt <= 7, 'Invalid catalog image shape')
        shapes.append((width, height, fmt))
    return dict(version=version, bytes=size, images=images, checksum=checksum,
                sha256=digest.hexdigest(), shapes=shapes)


def bind(reference_pack, target_pack, oracle, output, report_path=None):
    reference_pack, target_pack, oracle, output = map(Path, (reference_pack, target_pack, oracle, output))
    report_path = Path(report_path) if report_path else oracle.with_suffix(oracle.suffix + '.json')
    require(output.resolve() not in {p.resolve() for p in (reference_pack, target_pack, oracle, report_path)},
            'The bound oracle must not overwrite an input')
    source, target = pack_identity(reference_pack), pack_identity(target_pack)
    require(source['images'] == target['images'] and source['shapes'] == target['shapes'],
            'Target image identities, formats or dimensions differ from the old reference')
    report = json.loads(report_path.read_text())
    require(report.get('complete') is True and report.get('first_image') == 0 and
            report.get('images') == report.get('catalog_images') == source['images'] and
            report.get('independent_python_decode') is True,
            'Only a complete independent source-image oracle can be rebound')
    require(report.get('input_sha256') == source['sha256'], 'Source oracle names a different reference pack')
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    source_digest, target_digest, body_digest = hashlib.sha256(), hashlib.sha256(), hashlib.sha256()
    decoded = written = 0
    try:
        with oracle.open('rb') as old, NamedTemporaryFile(dir=output.parent, prefix=output.name + '.', delete=False) as new:
            temporary = Path(new.name)
            header = old.read(HEADER.size)
            require(len(header) == HEADER.size, 'Truncated source oracle header')
            fields = HEADER.unpack(header)
            require(fields == (b'IMGR0022', source['images'], 0, source['images'], source['bytes'], source['checksum']),
                    'Source oracle header differs from its reference pack')
            source_digest.update(header)
            bound_header = HEADER.pack(b'IMGR0022', target['images'], 0, target['images'], target['bytes'], target['checksum'])
            require(new.write(bound_header) == len(bound_header), 'Bound oracle short header write')
            target_digest.update(bound_header); written += len(bound_header)
            for image in range(source['images']):
                row = old.read(ROW.size)
                require(len(row) == ROW.size, 'Truncated source oracle row')
                ident, width, height, fmt, size = ROW.unpack(row)
                require(ident == image and (width, height) == source['shapes'][image][:2] and
                        0 <= fmt <= 4 and 0 < size <= MAX_IMAGE_BYTES,
                        'Source oracle row identity, shape or length differs')
                raw = old.read(size)
                require(len(raw) == size, 'Truncated source oracle image bytes')
                for block in (row, raw):
                    source_digest.update(block); target_digest.update(block); body_digest.update(block)
                    require(new.write(block) == len(block), 'Bound oracle short body write')
                    written += len(block)
                decoded += size
            require(not old.read(1), 'Trailing source oracle bytes')
            require(source_digest.hexdigest() == report.get('oracle_sha256') and
                    written == report.get('oracle_bytes') and decoded == report.get('decoded_image_bytes'),
                    'Source oracle bytes/hash differ from the completed export receipt')
            new.flush(); os.fsync(new.fileno())
        verified, body_verified = hashlib.sha256(), hashlib.sha256()
        with temporary.open('rb') as handle:
            header = handle.read(HEADER.size); verified.update(header)
            require(header == bound_header, 'Bound oracle header changed during publication')
            for block in iter(lambda: handle.read(1024 * 1024), b''):
                verified.update(block); body_verified.update(block)
        require(verified.digest() == target_digest.digest() and body_verified.digest() == body_digest.digest(),
                'Bound oracle pixel body changed during publication')
        os.replace(temporary, output); temporary = None
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
    result = dict(schema=1, complete=True, images=source['images'], catalog_images=source['images'], first_image=0,
        reference_pack_sha256=source['sha256'], reference_pack_version=source['version'],
        target_pack_sha256=target['sha256'], target_pack_version=target['version'],
        source_oracle_sha256=source_digest.hexdigest(), oracle_sha256=target_digest.hexdigest(),
        oracle_bytes=written, decoded_image_bytes=decoded, image_body_sha256=body_digest.hexdigest(),
        independent_old_reference_preserved=True, image_body_bytes_identical=True,
        changed_bytes_region='Native oracle header only, bytes 0..27',
        reference=str(reference_pack), target=str(target_pack), source_oracle=str(oracle), output=str(output))
    receipt = output.with_suffix(output.suffix + '.json')
    with NamedTemporaryFile(dir=receipt.parent, prefix=receipt.name + '.', delete=False) as handle:
        temp = Path(handle.name)
        try:
            handle.write(json.dumps(result, indent=2).encode() + b'\n'); handle.flush(); os.fsync(handle.fileno())
        except BaseException:
            temp.unlink(missing_ok=True); raise
    try:
        os.replace(temp, receipt)
    finally:
        temp.unlink(missing_ok=True)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference-pack', type=Path, required=True)
    parser.add_argument('--target-pack', type=Path, required=True)
    parser.add_argument('--oracle', type=Path, required=True)
    parser.add_argument('--oracle-report', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    destination = args.output.expanduser().resolve()
    require(destination != ROOT and ROOT not in destination.parents, 'Private pixel oracle must stay outside the repository')
    print(json.dumps(bind(args.reference_pack, args.target_pack, args.oracle, destination, args.oracle_report)), flush=True)


if __name__ == '__main__':
    main()
