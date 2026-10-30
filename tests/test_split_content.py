"""Public synthetic split-pack conversion and corruption regressions."""
import importlib.util
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
spec = importlib.util.spec_from_file_location("split_content", ROOT / "tools/split_content.py")
split = importlib.util.module_from_spec(spec)
spec.loader.exec_module(split)


def fixture(language, version=17, images=2):
    ni, ng, ns, nc, nk, aux = images, 1, 6111, 1, 1, 1
    base = 40 + ni*16 + ng*16 + ns*8 + nc*16 + nk*16 + aux*4
    table, descriptors = base + 64, base + 72 + (((ni + 1) & ~1) if version >= 20 else 0)
    prefix = descriptors + 16
    image = b"\x00\x00\x01\x00"
    group = struct.pack("<4s7I", b"TGR1", 0, 0, 4, 4, 0, 1, 0) + b"tile"
    art = group + image
    glyph = b"\xff"
    text = language.encode() + b"\0"
    out = bytearray(prefix) + art + glyph + text
    struct.pack_into("<4s9I",out,0,b"NSP2",version,split.REFERENCE,len(out),ni,ng,ns,nc,nk,0)
    for i in range(ni - 1):struct.pack_into("<IHHII",out,40+i*16,prefix+len(group),2,1,0,len(image))
    struct.pack_into("<IHHII",out,40+(ni-1)*16,descriptors,2,1,6,16)
    struct.pack_into("<IIHBBHH",out,40+ni*16,65,prefix+len(art),8,1,1,1,0)
    for i in range(ns):struct.pack_into("<II",out,40+ni*16+16+i*8,prefix+len(art)+len(glyph),3)
    struct.pack_into("<4s15I",out,base,b"TIL1",1,16,14,1,1,0,1,descriptors,table,770,1,512*1024,620000,table+8 if version >= 20 else 0,ni if version >= 20 else 0)
    struct.pack_into("<II",out,table,prefix,len(group))
    struct.pack_into("<4I",out,descriptors,base,0,0,4)
    struct.pack_into("<I",out,36,split.fnv(memoryview(out)[40:]))
    return bytes(out)


class SplitContentTest(unittest.TestCase):
    def test_outer_pages_align_suffix_without_changing_logical_resources(self):
        import storage_parts as storage
        for version, images in ((17, 2), (38, 4000), (40, 2)):
            original = {lang: fixture(lang, version, images) for lang in ('eng', 'zhs')}
            common, locales, _ = split.split(original, compress_directory=True)
            for lang, localized in locales.items():
                with self.subTest(version=version, language=lang):
                    aligned = split.align_language_suffix(localized)
                    header, info = split._language_header(aligned)
                    self.assertLess(info['alignment_padding'], 4096)
                    self.assertEqual(aligned[info['directory_end']:info['tail']],
                                     bytes(info['alignment_padding']))
                    # The virtual/physical phase is constant across the whole
                    # raw suffix, including requests crossing its boundaries.
                    self.assertEqual((info['tail'] - info['raw'] - header[7]) % 4096, 0)
                    self.assertEqual(split.align_language_suffix(aligned), aligned)
                    self.assertEqual(split.reconstruct(common, aligned, lang), original[lang])
                    packed, receipt = storage.compress_blocks(localized)
                    self.assertEqual(storage.expand_blocks(packed), aligned)
                    self.assertEqual(split.reconstruct(common, packed, lang), original[lang])
                    self.assertEqual(receipt['input_bytes'], len(localized))
                    self.assertEqual(receipt['saved_bytes'], len(localized)-len(packed))
                    self.assertEqual(receipt['aligned_input_bytes'], len(aligned))
                    self.assertTrue(receipt['suffix_page_aligned'])
                    self.assertEqual(storage.compress_blocks(aligned)[0], packed)

    def test_outer_pages_preserve_uncompressed_legacy_container(self):
        import storage_parts as storage
        original = {lang: fixture(lang) for lang in ('eng', 'zhs')}
        common, locales, _ = split.split(original)
        for lang, localized in locales.items():
            self.assertEqual(split.align_language_suffix(localized), localized)
            packed, _ = storage.compress_blocks(localized)
            self.assertEqual(storage.expand_blocks(packed), localized)
            self.assertEqual(split.reconstruct(common, packed, lang), original[lang])

    def test_alignment_padding_rejects_nonzero_oversized_and_wrong_phase(self):
        original = {lang: fixture(lang, 38, images=4000) for lang in ('eng', 'zhs')}
        common, locales, _ = split.split(original, compress_directory=True)
        localized = locales['eng']
        # Unpadded editions are still accepted, independently of their phase.
        self.assertEqual(split.reconstruct(common, localized, 'eng'), original['eng'])
        aligned = split.align_language_suffix(localized)
        _, info = split._language_header(aligned)
        self.assertGreater(info['alignment_padding'], 0)
        damaged = bytearray(aligned);damaged[info['directory_end']] = 1
        with self.assertRaises(ValueError):split.reconstruct(common, damaged, 'eng')
        for extra in (1, 4096):
            damaged = bytearray(aligned[:info['tail']]) + bytes(extra) + aligned[info['tail']:]
            struct.pack_into('<I', damaged, 16, len(damaged))
            struct.pack_into('<I', damaged, 116, info['tail'] + extra)
            with self.assertRaises(ValueError):split.reconstruct(common, damaged, 'eng')
        damaged = bytearray(aligned[:info['tail']-1]) + aligned[info['tail']:]
        struct.pack_into('<I', damaged, 16, len(damaged))
        struct.pack_into('<I', damaged, 116, info['tail'] - 1)
        with self.assertRaises(ValueError):split.reconstruct(common, damaged, 'eng')

    def test_compressed_directory_exact_versions_and_unchanged_common(self):
        for version in (17, 32, 36, 38, 39):
            original = {lang: fixture(lang, version) for lang in ('eng', 'zhs')}
            common, legacy, _ = split.split(original)
            compressed_common, locales, receipt = split.split(original, compress_directory=True)
            self.assertEqual(common, compressed_common)
            self.assertEqual(receipt['format'], 'NSC1/NSL2')
            self.assertGreater(receipt['directory_saved_bytes'], 0)
            for lang, localized in locales.items():
                self.assertEqual(localized[:4], b'NSL2')
                self.assertEqual(split.language_resource_version(localized), version)
                self.assertEqual(split.reconstruct(common, localized, lang), original[lang])
                self.assertEqual(split.compress_language_directory(localized), localized)
                raw, block, count, table, data_at, tail, prop, reserved = struct.unpack_from('<8I', localized, 96)
                self.assertEqual(raw % 4096, 0)
                self.assertEqual((block, table, data_at, prop, reserved), (65536, 128, 128 + count * 8, 14, 0))
                self.assertEqual(localized[tail:], legacy[lang][96 + raw:])
                self.assertEqual(receipt['directory_compression'][lang]['saved_bytes'], len(legacy[lang]) - len(localized))

    def test_multiple_blocks_and_version_probe_only_decodes_first(self):
        original = {lang: fixture(lang, 38, images=4000) for lang in ('eng', 'zhs')}
        common, locales, _ = split.split(original, compress_directory=True)
        for lang, localized in locales.items():
            self.assertEqual(split.u32(localized, 104), 2)
            self.assertEqual(split.reconstruct(common, localized, lang), original[lang])
            off, size = struct.unpack_from('<II', localized, 136)
            self.assertFalse(size & split.DIRECTORY_LITERAL)
            damaged = bytearray(localized)
            damaged[off + size - 1] ^= 0xff
            self.assertEqual(split.language_resource_version(damaged), 38)
            with self.assertRaises(ValueError):split.reconstruct(common, damaged, lang)

    def test_literal_blocks_do_not_need_an_encoded_stream(self):
        original = {lang: fixture(lang, 38, images=4000) for lang in ('eng', 'zhs')}
        common, legacy, _ = split.split(original)
        # Valid raw LZMA2 packets cost more than the original block. The writer
        # must use a literal block and the reader must copy those exact bytes.
        def stored(raw, **_kwargs):
            return b'\x01' + struct.pack('>H', len(raw) - 1) + raw + b'\0'
        with mock.patch.object(split.lzma, 'compress', side_effect=stored):
            compressed = {lang: split.compress_language_directory(data) for lang, data in legacy.items()}
        for lang, localized in compressed.items():
            for index in range(split.u32(localized, 104)):
                self.assertTrue(split.u32(localized, 128 + index * 8 + 4) & split.DIRECTORY_LITERAL)
            self.assertEqual(split.reconstruct(common, localized, lang), original[lang])

    def test_compressed_bounds_reject_aliases_gaps_bad_streams_and_unaligned_tail(self):
        packs = {lang: fixture(lang, 38, images=4000) for lang in ('eng', 'zhs')}
        common, locales, _ = split.split(packs, compress_directory=True)
        localized = locales['eng']
        for offset, value in ((4, 1), (12, 96), (16, len(localized) + 1),
                              (96, split.u32(localized, 96) - 1), (100, 32768),
                              (104, 33), (108, 96), (112, 128), (116, len(localized) + 1),
                              (120, 15), (124, 1), (128, split.u32(localized, 128) + 1),
                              (132, 0), (136, split.u32(localized, 128))):
            with self.subTest(offset=offset):
                damaged = bytearray(localized);struct.pack_into('<I', damaged, offset, value)
                with self.assertRaises(ValueError):split.reconstruct(common, damaged, 'eng')
        damaged = bytearray(localized);damaged[split.u32(localized, 128)] ^= 0xff
        with self.assertRaises(ValueError):split.reconstruct(common, damaged, 'eng')
        with self.assertRaises(ValueError):split.reconstruct(common, localized[:-1], 'eng')
        # The legacy metadata area keeps its earlier non-authoritative fields.
        damaged = bytearray(localized);damaged[80] ^= 1
        self.assertEqual(split.reconstruct(common, damaged, 'eng'), packs['eng'])

    def test_packaging_accepts_both_containers_and_keeps_compressed_bytes(self):
        from build_web import resource_package
        from package_release import resource_version
        packs = {lang: fixture(lang, 38) for lang in ('eng', 'zhs')}
        for compressed in (False, True):
            common, locales, _ = split.split(packs, compress_directory=compressed)
            with tempfile.TemporaryDirectory() as directory:
                root = Path(directory);common_path = root/'common.tns';common_path.write_bytes(common)
                paths = {}
                for lang, localized in locales.items():
                    paths[lang] = root/(lang+'.tns');paths[lang].write_bytes(localized)
                    self.assertEqual(resource_version(paths[lang]), 38)
                result = resource_package(common_path=common_path, eng_path=paths['eng'], zhs_path=paths['zhs'])
                self.assertEqual(result['mode'], 'split')
                import base64
                self.assertEqual(base64.b64decode(result['common']), common)
                self.assertEqual({lang: base64.b64decode(data) for lang, data in result['languages'].items()}, locales)

    def test_release_version_probe_accepts_42_in_all_containers_and_rejects_43(self):
        from package_release import resource_version

        def outer_literal(inner):
            # Independent legal NSL3 literal pages allow a future logical
            # header to reach the reader without asking the current writer
            # to accept/rewrite an unsupported NSP version.
            block = 4096
            count = (len(inner) + block - 1) // block
            begin = 32 + 4 * (count + 1)
            offsets = [begin + i*block | 0x80000000 for i in range(count)]
            offsets.append(begin + len(inner))
            return (struct.pack('<4s7I', b'NSL3', 3, split.REFERENCE,
                                begin + len(inner), len(inner), block, count, begin) +
                    struct.pack('<' + 'I' * len(offsets), *offsets) + inner)

        original = {'eng': fixture('eng', 42), 'zhs': fixture('zhs', 42)}
        common, raw, _ = split.split(original)
        # Stored NSL2 directory blocks let the test change only the logical
        # version word; every container length and offset remains valid.
        with mock.patch.object(split.lzma, 'compress', side_effect=lambda data, **_kwargs: bytes(len(data) + 1)):
            compressed = split.compress_language_directory(raw['eng'])
        self.assertTrue(split.u32(compressed, 132) & split.DIRECTORY_LITERAL)
        current = {'nsl1': raw['eng'], 'nsl2': compressed,
                   'nsl3-nsl1': outer_literal(raw['eng']), 'nsl3-nsl2': outer_literal(compressed)}
        future_raw = bytearray(raw['eng'])
        struct.pack_into('<I', future_raw, split.LANGUAGE_HEADER + 4, 43)
        future_compressed = bytearray(compressed)
        struct.pack_into('<I', future_compressed, split.u32(compressed, 128) + 4, 43)
        future = {'nsl1': bytes(future_raw), 'nsl2': bytes(future_compressed),
                  'nsl3-nsl1': outer_literal(future_raw), 'nsl3-nsl2': outer_literal(future_compressed)}
        with tempfile.TemporaryDirectory() as directory:
            for name, data in current.items():
                with self.subTest(container=name, version=42):
                    self.assertEqual(split.language_resource_version(data), 42)
                    self.assertEqual(split.reconstruct(common, data, 'eng'), original['eng'])
                    path = Path(directory)/(name + '.tns'); path.write_bytes(data)
                    self.assertEqual(resource_version(path), 42)
            for name, data in future.items():
                with self.subTest(container=name, version=43):
                    with self.assertRaisesRegex(ValueError, 'Invalid logical resource version'):
                        split.language_resource_version(data)
                    path = Path(directory)/(name + '-future.tns'); path.write_bytes(data)
                    with self.assertRaisesRegex(ValueError, 'Expected a valid NSL1/NSL2/NSL3 language resource'):
                        resource_version(path)

    def test_calculator_archive_includes_unicode_notice_with_distinct_name(self):
        import json
        import zipfile
        original = {lang: fixture(lang, 41) for lang in ('eng', 'zhs')}
        common, locales, _ = split.split(original)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root/'launcher.tns').write_bytes(b'public synthetic launcher')
            (root/'common.tns').write_bytes(common)
            for lang, data in locales.items():
                (root/(lang+'.tns')).write_bytes(data)
            result = subprocess.run([sys.executable, str(ROOT/'tools/package_release.py'),
                '--launcher', str(root/'launcher.tns'), '--common', str(root/'common.tns'),
                '--eng', str(root/'eng.tns'), '--zhs', str(root/'zhs.tns'),
                '--output', str(root/'output'), '--runtime-checkpoint', '1'*40,
                '--resource-checkpoint', '2'*40, '--stage', 'public-notice-test',
                '--calculator-only', '--local-checkpoints'], cwd=ROOT,
                capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            receipt = json.loads(result.stdout)
            with zipfile.ZipFile(receipt['calculator']) as archive:
                names = archive.namelist()
                self.assertEqual(len(names), len(set(names)))
                self.assertEqual(archive.read('data/licenses/Unicode-License.txt'),
                                 (ROOT/'third_party/unicode/LICENSE').read_bytes())
                self.assertNotIn('data/licenses/LICENSE', names)
                self.assertEqual([name for name in names if name.startswith('state/')], ['state/'])
                self.assertEqual(archive.read('state/'), b'')
                self.assertEqual(json.loads(archive.read('data/BUILD-INFO.json'))['resource_format'], 41)

    def test_lossless_known_versions(self):
        for version in (17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39):
            original={lang:fixture(lang,version) for lang in ("eng","zhs")}
            common,locales,receipt=split.split(original)
            self.assertEqual(receipt["common"]["artwork_bytes"],40)
            self.assertEqual(receipt["stored_bytes"],sum(map(len,original.values()))-40+256)
            for language,local in locales.items():
                self.assertEqual(split.reconstruct(common,local,language),original[language])
                with self.assertRaises(ValueError):split.reconstruct(common,local,"eng" if language=="zhs" else "zhs")

    def test_corrupt_and_mismatched(self):
        packs={lang:fixture(lang) for lang in ("eng","zhs")}
        common,locales,_=split.split(packs)
        for at in (0,4,16,20):
            damaged=bytearray(common);damaged[at]^=1
            with self.assertRaises(ValueError):split.reconstruct(damaged,locales["eng"],"eng")
        for at in (0,4,16,24,28,36,len(locales["eng"])-1):
            damaged=bytearray(locales["eng"]);damaged[at]^=1
            with self.assertRaises(ValueError):split.reconstruct(common,damaged,"eng")
        for at in (24,28,32,63):
            damaged=bytearray(common);damaged[at]^=1
            self.assertEqual(split.reconstruct(damaged,locales["eng"],"eng"),packs["eng"])
        for at in (40,44,48,80,95):
            damaged=bytearray(locales["eng"]);damaged[at]^=1
            self.assertEqual(split.reconstruct(common,damaged,"eng"),packs["eng"])
        damaged=bytearray(common);damaged[64]^=1
        self.assertNotEqual(split.reconstruct(damaged,locales["eng"],"eng"),packs["eng"])
        damaged=bytearray(packs["zhs"]);row=split.layout(damaged,"zhs");damaged[row["prefix"]+39]^=1
        struct.pack_into("<I",damaged,36,split.fnv(memoryview(damaged)[40:]))
        with self.assertRaises(ValueError):split.split(dict(packs,zhs=damaged))

    def test_locale_and_ranges(self):
        data=fixture("eng")
        with self.assertRaises(ValueError):split.layout(data,"zhs")
        with self.assertRaises(ValueError):split.layout(fixture("eng",0xffffffff),"eng")
        for at,value in ((12,len(data)+1),(16,65536),(20,32769),(24,6110)):
            bad=bytearray(data);struct.pack_into("<I",bad,at,value)
            with self.assertRaises(ValueError):split.layout(bad,"eng")
        # A glyph inside the shared art would violate the architecture even
        # when the old pack checksum is intentionally made self-consistent.
        bad=bytearray(data);row=split.layout(data,"eng");struct.pack_into("<I",bad,76,row["prefix"])
        struct.pack_into("<I",bad,36,split.fnv(memoryview(bad)[40:]))
        with self.assertRaises(ValueError):split.layout(bad,"eng")


if __name__=="__main__":unittest.main()
