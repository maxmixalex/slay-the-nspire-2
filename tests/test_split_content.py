"""Public synthetic split-pack conversion and corruption regressions."""
import importlib.util
from pathlib import Path
import struct
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
