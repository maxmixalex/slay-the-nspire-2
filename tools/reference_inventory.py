#!/usr/bin/env python3
"""Read the reference installation without running it or copying game assets.

The CLI metadata reader only decodes PE sections, stream directories and the
TypeDef table. It is not a decompiler. TypeDef counts include deprecated/token
models and must not be advertised as counts of normally obtainable content.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
CATEGORIES = ('Cards', 'Relics', 'Potions', 'Characters', 'Monsters', 'Encounters', 'Events')


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open('rb') as source:
        while block := source.read(1024 * 1024):
            digest.update(block)
    return digest.hexdigest()


def model_types(path: Path) -> dict[str, list[dict[str, object]]]:
    data = path.read_bytes()

    def take(offset: int, size: int) -> bytes:
        if offset < 0 or size < 0 or offset > len(data) - size:
            raise ValueError('PE metadata range outside the file')
        return data[offset:offset + size]

    def uint(offset: int, size: int = 4) -> int:
        return int.from_bytes(take(offset, size), 'little')

    if take(0, 2) != b'MZ':
        raise ValueError('Expected a PE assembly')
    pe = uint(0x3C)
    if take(pe, 4) != b'PE\0\0':
        raise ValueError('Invalid PE header')
    optional = pe + 24
    magic = uint(optional, 2)
    if magic not in (0x10B, 0x20B):
        raise ValueError('Unknown PE optional header')
    sections = []
    section_count = uint(pe + 6, 2)
    if section_count > 96:
        raise ValueError('Too many PE sections')
    section_table = optional + uint(pe + 20, 2)
    for i in range(section_count):
        vs, address, raw_size, raw = struct.unpack('<4I', take(section_table + i * 40 + 8, 16))
        sections.append((address, max(vs, raw_size), raw))

    def rva(address: int) -> int:
        for start, size, raw in sections:
            if start <= address < start + size:
                return raw + address - start
        raise ValueError('Metadata RVA does not map to a PE section')

    directory = optional + (112 if magic == 0x20B else 96)
    cli = rva(uint(directory + 14 * 8))
    metadata = rva(uint(cli + 8))
    if take(metadata, 4) != b'BSJB':
        raise ValueError('Expected CLI metadata')
    pos = (metadata + 16 + uint(metadata + 12) + 3) & ~3
    count = uint(pos + 2, 2)
    if count > 32:
        raise ValueError('Invalid metadata stream count')
    pos += 4
    streams = {}
    for _ in range(count):
        offset, size = struct.unpack('<2I', take(pos, 8))
        pos += 8
        end = data.find(b'\0', pos, pos + 32)
        if end < 0:
            raise ValueError('Unterminated metadata stream name')
        name = data[pos:end].decode('ascii')
        pos = (end + 4) & ~3
        streams[name] = take(metadata + offset, size)
    table = streams.get('#~', streams.get('#-'))
    if table is None or '#Strings' not in streams or len(table) < 24:
        raise ValueError('Missing metadata tables or string heap')
    valid = int.from_bytes(table[8:16], 'little')
    rows = [0] * 64
    pos = 24
    for i in range(64):
        if valid >> i & 1:
            if pos + 4 > len(table):
                raise ValueError('Truncated row directory')
            rows[i] = int.from_bytes(table[pos:pos + 4], 'little')
            if rows[i] > 1000000:
                raise ValueError('Excessive metadata table size')
            pos += 4
    heap = table[6]
    string_size = 4 if heap & 1 else 2
    guid_size = 4 if heap & 2 else 2
    scope_size = 2 if max(rows[i] for i in (0, 26, 35, 1)) < 16384 else 4
    extends_size = 2 if max(rows[i] for i in (2, 1, 27)) < 16384 else 4
    # Skip Module and TypeRef to reach TypeDef. Index widths depend on row counts.
    pos += rows[0] * (2 + string_size + 3 * guid_size)
    pos += rows[1] * (scope_size + 2 * string_size)
    field_size = 2 if rows[4] < 65536 else 4
    method_size = 2 if rows[6] < 65536 else 4
    row_size = 4 + 2 * string_size + extends_size + field_size + method_size
    if pos + rows[2] * row_size > len(table):
        raise ValueError('Truncated TypeDef table')
    strings = streams['#Strings']

    def text(index: int) -> str:
        end = strings.find(b'\0', index)
        if end < 0:
            raise ValueError('Unterminated metadata string')
        return strings[index:end].decode('utf-8')

    result: dict[str, list[dict[str, object]]] = {k: [] for k in CATEGORIES}
    for i in range(rows[2]):
        start = pos + row_size * i
        flags = int.from_bytes(table[start:start + 4], 'little')
        name = text(int.from_bytes(table[start + 4:start + 4 + string_size], 'little'))
        namespace = text(int.from_bytes(table[start + 4 + string_size:start + 4 + 2 * string_size], 'little'))
        for category in CATEGORIES:
            if namespace == 'MegaCrit.Sts2.Core.Models.' + category:
                result[category].append({'name': name, 'abstract': bool(flags & 0x80),
                    'availability': 'requires-pool-and-unlock-verification'})
    return result


def verify_release(game: Path, *, cache_identity: bool = False) -> tuple[dict, Path, dict]:
    """Select the supported release from its metadata, without scanning assets.

    Full conversion caches can request content identities to distinguish their
    input files. Those hashes are cache keys, never an integrity acceptance gate.
    Ordinary incremental converters do not need to read the whole PCK first.
    """
    lock = json.loads((ROOT / 'config' / 'reference-release.json').read_text())
    info = json.loads((game / 'release_info.json').read_text(encoding='utf-8-sig'))
    dll = game / 'sts2.dll'
    if not dll.is_file():
        dll = game / 'data_sts2_windows_x86_64' / 'sts2.dll'
    if info.get('commit') != lock['commit'] or info.get('version') != lock['version']:
        raise ValueError('The installation does not match the pinned reference release.')
    identity = {'version': info['version'], 'commit': info['commit']}
    if cache_identity:
        identity.update(assembly=sha256(dll), pck=sha256(game / 'SlayTheSpire2.pck'))
    return lock, dll, identity


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game', type=Path)
    parser.add_argument('--output', type=Path, default=ROOT / 'dist' / 'reference-inventory.json')
    args = parser.parse_args()
    lock, dll, identity = verify_release(args.game)
    models = model_types(dll)
    counts = {k: len(v) for k, v in models.items()}
    if counts != lock['metadata_type_counts']:
        raise ValueError('Reference metadata counts differ from the audited baseline')
    output = {'version': lock['version'], 'commit': lock['commit'], 'reference': identity,
              'metadata_type_counts': counts, 'models': models,
              'count_semantics': 'Metadata definitions, not verified obtainable content or port coverage.'}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(output, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(counts, sort_keys=True))
    print(f'Wrote {args.output}; these counts are NOT an implementation-completeness claim.')


if __name__ == '__main__':
    main()
