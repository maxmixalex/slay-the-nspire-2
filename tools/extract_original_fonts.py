#!/usr/bin/env python3
"""Read FontFile.data from the owner's Godot imports, without running Godot.

Format sources (Godot4.5-stable, MIT):
  core/io/file_access_compressed.cpp (open_after_magic/get_buffer)
  core/io/resource_format_binary.cpp (open/load/parse_variant/_get_string)
  core/io/resource_format_binary.h (flags/reserved fields)
  core/io/compression.h (compression mode2 is Zstandard)
  scene/resources/font.cpp (stored PackedByteArray property `data`)
https://github.com/godotengine/godot/tree/4.5-stable

Only the requested embedded font source is emitted. Imported raster caches are
skipped by resource offsets and lengths, never mistaken for the font itself.
The PCK, extracted fonts and receipt must remain outside the public repository.
"""
from __future__ import annotations

import argparse
from collections import OrderedDict
import hashlib
import io
import json
from pathlib import Path
import re
import struct
import zlib


DEFAULT_FONTS = (
    'fonts/zhs/SourceHanSerifSC-Medium.otf',
    'fonts/kreon_regular.ttf',
    'fonts/kreon_bold.ttf',
)


class ResourceFormatError(ValueError):
    pass


class ResourceReader:
    """Bounded random access to ordinary RSRC or block-compressed RSCC data."""
    def __init__(self, source: bytes):
        self.source = memoryview(source)
        self.position = 0
        self.endian = '<'
        self.real_width = 4
        self.strings: list[str] = []
        self.cache: OrderedDict[int, bytes] = OrderedDict()
        if source[:4] == b'RSRC':
            self.compressed = False
            self.length = len(source)
            self.position = 4
            return
        if source[:4] != b'RSCC' or len(source) < 16:
            raise ResourceFormatError('Not a Godot binary resource')
        self.compressed = True
        self.mode, self.block_size, self.length = struct.unpack_from('<III', source, 4)
        if self.mode not in (1, 2, 3) or not self.block_size or self.length > 256*1024*1024:
            raise ResourceFormatError('Unsupported compression mode/extent')
        count = self.length // self.block_size + 1
        if 16 + count*4 > len(source):
            raise ResourceFormatError('Truncated compressed block table')
        sizes = struct.unpack_from('<'+str(count)+'I', source, 16)
        offset = 16 + count*4
        self.blocks = []
        for size in sizes:
            if offset+size > len(source):
                raise ResourceFormatError('Truncated compressed block')
            self.blocks.append((offset, size))
            offset += size
        if self.mode == 2:
            import zstandard
            self.decoder = zstandard.ZstdDecompressor()

    def seek(self, position: int):
        if not 0 <= position <= self.length:
            raise ResourceFormatError('Resource seek outside declared extent')
        self.position = position

    def skip(self, size: int):
        self.seek(self.position+size)

    def block(self, index: int) -> bytes:
        if index in self.cache:
            self.cache.move_to_end(index)
            return self.cache[index]
        offset, size = self.blocks[index]
        raw = self.source[offset:offset+size]
        expected = min(self.block_size, self.length-index*self.block_size)
        if self.mode == 2:
            decoded = self.decoder.decompress(raw, max_output_size=expected or 1)
        else:
            decoded = zlib.decompress(raw, -15 if self.mode == 1 else 31)
        if len(decoded) != expected:
            raise ResourceFormatError('Compressed block has an invalid decoded length')
        self.cache[index] = decoded
        while len(self.cache) > 4:
            self.cache.popitem(last=False)
        return decoded

    def read(self, size: int) -> bytes:
        if size < 0 or self.position+size > self.length:
            raise ResourceFormatError('Truncated resource field')
        if not self.compressed:
            result = bytes(self.source[self.position:self.position+size])
            self.position += size
            return result
        pieces = []
        while size:
            index, local = divmod(self.position, self.block_size)
            count = min(size, self.block_size-local)
            pieces.append(self.block(index)[local:local+count])
            self.position += count
            size -= count
        return b''.join(pieces)

    def integer(self, width=4):
        return int.from_bytes(self.read(width), 'little' if self.endian == '<' else 'big')

    def string(self):
        size = self.integer()
        return self.read(size).rstrip(b'\0').decode('utf-8')

    def property_name(self):
        index = self.integer()
        if index & 0x80000000:
            return self.read(index & 0x7fffffff).rstrip(b'\0').decode('utf-8')
        if index >= len(self.strings):
            raise ResourceFormatError('Invalid property string index')
        return self.strings[index]

    def skip_variant(self, depth=0):
        if depth > 32:
            raise ResourceFormatError('Excessive resource nesting')
        kind = self.integer()
        if kind in (1, 42, 43):
            return
        if kind in (2, 3, 23):
            self.skip(4)
        elif kind in (40, 41):
            self.skip(8)
        elif kind == 4:
            self.skip(self.real_width)
        elif kind in (5, 44):
            self.skip(self.integer())
        elif kind in (10, 11, 12, 13, 14, 15, 16, 17, 18, 50, 52):
            count = {10:2, 11:4, 12:3, 13:4, 14:4, 15:6, 16:9, 17:12, 18:6, 50:4, 52:16}[kind]
            self.skip(count*self.real_width)
        elif kind in (20, 45, 46, 47, 51):
            self.skip({20:16, 45:8, 46:16, 47:12, 51:16}[kind])
        elif kind == 22:
            names, subnames = self.integer(2), self.integer(2) & 0x7fff
            for _ in range(names+subnames):
                self.property_name()
        elif kind == 24:
            object_type = self.integer()
            if object_type in (2, 3):
                self.skip(4)
            elif object_type == 1:
                self.string(); self.string()
            elif object_type != 0:
                raise ResourceFormatError('Unknown resource reference')
        elif kind in (26, 30):
            count = (self.integer() & 0x7fffffff) * (2 if kind == 26 else 1)
            if count > self.length//4:
                raise ResourceFormatError('Invalid nested collection length')
            for _ in range(count):
                self.skip_variant(depth+1)
        elif kind == 31:
            count = self.integer()
            self.skip(count + (-count % 4))
        elif kind == 34:
            count = self.integer()
            if count > self.length//4:
                raise ResourceFormatError('Invalid packed string array')
            for _ in range(count):
                self.skip(self.integer())
        elif kind in (32, 33, 35, 36, 37, 48, 49, 53):
            count = self.integer()
            width = {32:4, 33:4, 35:3*self.real_width, 36:16, 37:2*self.real_width,
                     48:8, 49:8, 53:4*self.real_width}[kind]
            self.skip(count*width)
        else:
            raise ResourceFormatError(f'Unsupported resource variant {kind}')


def font_payload(source: bytes) -> tuple[bytes, dict]:
    reader = ResourceReader(source)
    big_endian = reader.integer()
    reader.integer()  # Historical use_real64 field; the format flags own it now.
    if big_endian not in (0, 1):
        raise ResourceFormatError('Invalid endian flag')
    reader.endian = '>' if big_endian else '<'
    major, minor, version = reader.integer(), reader.integer(), reader.integer()
    if major != 4 or not 4 <= version <= 6:
        raise ResourceFormatError(f'Unsupported Godot binary resource {major}.{minor}/{version}')
    resource_type = reader.string()
    if resource_type != 'FontFile':
        raise ResourceFormatError(f'Expected FontFile, found {resource_type}')
    reader.skip(8)  # Import metadata offset.
    flags = reader.integer()
    reader.real_width = 8 if flags & 4 else 4
    reader.skip(8)  # UID field exists whether or not UIDs are enabled.
    if flags & 8:
        reader.string()
    reader.skip(11*4)
    strings = reader.integer()
    if strings > reader.length//4:
        raise ResourceFormatError('Invalid resource string table')
    reader.strings = [reader.string() for _ in range(strings)]
    external = reader.integer()
    for _ in range(external):
        reader.string(); reader.string()
        if flags & 2:
            reader.skip(8)
    internal = reader.integer()
    if not 0 < internal <= reader.length//12:
        raise ResourceFormatError('Missing or invalid internal resource directory')
    entries = [(reader.string(), reader.integer(8)) for _ in range(internal)]
    # ResourceLoaderBinary::load identifies the last internal entry as main.
    reader.seek(entries[-1][1])
    if reader.string() != 'FontFile':
        raise ResourceFormatError('Main resource is not FontFile')
    properties = reader.integer()
    for _ in range(properties):
        name = reader.property_name()
        if name != 'data':
            reader.skip_variant()
            continue
        if reader.integer() != 31:
            raise ResourceFormatError('FontFile.data is not PackedByteArray')
        size = reader.integer()
        payload = reader.read(size)
        if payload[:4] not in (b'OTTO', b'\0\1\0\0', b'ttcf'):
            raise ResourceFormatError('FontFile.data does not contain a supported SFNT font')
        return payload, dict(godot=[major, minor], binary_format=version,
                            imported_bytes=len(source), resource_bytes=reader.length,
                            string_count=strings, internal_resources=internal,
                            font_bytes=size, sha256=hashlib.sha256(payload).hexdigest())
    raise ResourceFormatError('FontFile has no stored data property')


def extract(game: Path, output: Path, fonts=DEFAULT_FONTS) -> dict:
    from pck import Pack
    from fontTools.ttLib import TTFont
    output.mkdir(parents=True, exist_ok=True)
    report = dict(schema=1, method='Godot4.5 binary FontFile.data property', fonts=[])
    pack = Pack(game)
    try:
        for font in fonts:
            imported = pack.read(font+'.import').decode('utf-8')
            match = re.search(r'^path="res://([^"]+\.fontdata)"$', imported, re.M)
            if not match:
                raise ResourceFormatError('No fontdata remap for '+font)
            path = match.group(1)
            payload, item = font_payload(pack.read(path))
            # A real font parser verifies the extracted directory and cmap;
            # no signature carving or heuristic byte-range search is used.
            with TTFont(io.BytesIO(payload), lazy=True) as parsed:
                item['family'] = parsed['name'].getDebugName(1)
                item['style'] = parsed['name'].getDebugName(2)
                item['cmap_count'] = len(parsed.getBestCmap())
            destination = output/Path(font).name
            destination.write_bytes(payload)
            item.update(source=font, imported=path, output=str(destination))
            report['fonts'].append(item)
    finally:
        pack.close()
    (output/'font-extraction.json').write_text(json.dumps(report, indent=2)+'\n')
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--font', action='append', help='PCK source font path; may be repeated')
    args = parser.parse_args()
    result = extract(args.game, args.output, args.font or DEFAULT_FONTS)
    print(json.dumps(result, indent=2))
