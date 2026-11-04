"""Retirement requires the fixed plan, all live roots, and complete new art."""

import copy
import lzma
import struct
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

from nspire_converter import content_tiles as t
from nspire_converter import release_content_guard as guard
from nspire_converter import retired_assets as retired
from nspire_converter.retire_legacy_resources import encode_same
from nspire_converter.sparse_tile_filter import restore as sparse_restore
from nspire_converter.sparse_tile_filter import transform as sparse_transform
from nspire_converter.tile_filter import filter_tiles, restore_tiles
from nspire_converter.tile_prediction import index_transform
from test_release_content_guard import fixture


class RetirementGuardTest(unittest.TestCase):
    def test_runtime_rejects_a_new_consumer_of_retired_animation(self):
        retired.verify_runtime_routes()
        path = retired.ROOT / "cpp/src/content.cpp"
        read = Path.read_text

        def changed(source, *args, **kwargs):
            text = read(source, *args, **kwargs)
            return text + '\nlookup("reactions/attack_left");\n' if source == path else text

        with patch.object(Path, "read_text", changed):
            with self.assertRaisesRegex(ValueError, "retired animation gained a literal caller"):
                retired.verify_runtime_routes()

    def test_ancient_extension_keeps_retirement_marker_and_disjoint_live_roots(self):
        data, _ = fixture(7212)
        source = t.PackView(data, 7212, versions=(45, 46))
        source = copy.copy(source)
        source.images = list(source.images)
        source.aux = list(source.aux)
        tables = {}
        original_image = source.image_raw
        first = 58613
        for duration in (18000, 16667):
            count = (duration + 32) // 33
            index = first + count
            source.images.extend([(0, 320, 240, 0, 320 * 240 * 2)] * count)
            source.images.append((0, 8, 1, 0, 16))
            source.aux.append(index)
            tables[index] = (
                8,
                1,
                0,
                struct.pack("<8H", 0x414E, 1, first, count, 33, duration, 0, 0),
            )
            first = index + 1

        def image(index):
            return tables[index] if index in tables else original_image(index)

        self.assertTrue(retired.validate_retired_profile(source, image)["complete"])
        saved = tables[source.aux[-1]]
        tables[source.aux[-1]] = (8, 1, 0, struct.pack("<8H", 0x414E, 1, 321, 506, 33, 16667, 0, 0))
        with self.assertRaisesRegex(ValueError, "Ancient loop range"):
            retired.validate_retired_profile(source, image)
        tables[source.aux[-1]] = saved
        source.aux[7211] = 58611
        with self.assertRaisesRegex(ValueError, "marker/catalog"):
            retired.validate_retired_profile(source, image)

    def test_marker_clip_tuple_and_empty_image_fail_closed(self):
        original, at = fixture(7212)
        cases = [
            ("marker", at["marker"] + 8, retired.PLAN["plan_tag"] ^ 1, "plan marker"),
            ("marker identity", at["aux"] + 7211 * 4, 58611, "marker/catalog"),
            ("clip", at["clips"] + 8 * 16, 322, "registration"),
            ("opaque old frame", 40 + 321 * 16 + 8, 0, "empty sparse"),
        ]
        for label, offset, value, error in cases:
            data = bytearray(original)
            struct.pack_into("<I", data, offset, value)
            with self.subTest(label=label), self.assertRaisesRegex(ValueError, error):
                guard.validate_release_content(data, "ui22")

    def test_direct_card_aux_and_clip_roots_are_never_retired(self):
        original, at = fixture(7212)
        for label, offset in (
            ("card", at["clips"] + 1783 * 16),
            ("aux", at["aux"] + 123 * 4),
            ("active clip", at["clips"] + 4 * 16),
        ):
            data = bytearray(original)
            struct.pack_into("<I", data, offset, 321)
            with (
                self.subTest(label=label),
                self.assertRaisesRegex(ValueError, "direct clip/card/auxiliary root"),
            ):
                guard.validate_release_content(data, "ui22")

    def test_typed_framed_and_vantom_roots_are_never_retired(self):
        original, at = fixture(7212)
        old_osty = at["tables"][9] + 16 * 2
        vantom = struct.unpack_from("<I", original, 40 + 50000 * 16)[0] + 17 * 2
        for label, offset in (("old framed table", old_osty), ("Vantom mesh/texture", vantom)):
            data = bytearray(original)
            struct.pack_into("<H", data, offset, 321)
            with (
                self.subTest(label=label),
                self.assertRaisesRegex(ValueError, "typed actor/mesh/mask root"),
            ):
                guard.validate_release_content(data, "ui22")

    def test_complete_merchant_and_old_unknown_resources_are_required(self):
        original, at = fixture(7212)
        data = bytearray(original)
        struct.pack_into("<H", data, 40 + 45956 * 16 + 4, 56)
        with self.assertRaisesRegex(ValueError, "merchant moving-layer dimensions"):
            guard.validate_release_content(data, "ui22")
        source = t.PackView(original, 7212, versions=(45, 46))
        ids = retired.retired_images(source)
        self.assertEqual(len(ids), 1598)
        self.assertFalse(ids & set(range(840, 999)))
        self.assertFalse(ids & set(retired.PLAN["preserved_ui20_leaves"]))
        self.assertFalse(
            {row["id"] for row in retired.PLAN["retired_clips"]} & set(range(1765, 1783))
        )

    def test_existing_plain_predicted_and_color_codecs_stay_exact(self):
        dictionary = 1024 * 1024
        reader = SimpleNamespace(dictionary=dictionary, tile_count=1360, tile_groups=1)
        one = t.FRAME.pack(2, 4, 2) + struct.pack("<I", 0) + b"\0\0"
        two = t.FRAME.pack(2, 4, 2) + struct.pack("<I", 17) + b"\0\0"
        cases = []
        for predicted in (False, True):
            old = index_transform(one, 1, 3, tile_count=1360) if predicted else one
            cases.append((b"TGI2" if predicted else b"TGR1", 2, 1, old, two, int(predicted)))
        color = bytearray(770)
        color[:2] = b"\x01\x01"
        color[2:4] = b"\x34\x12"
        next_color = bytearray(color)
        next_color[2:4] = b"\xcd\xab"
        cases.append((b"TGR2", 1, 0, filter_tiles(bytes(color), 1), bytes(next_color), 1))
        color[514] = 128
        next_color[514] = 128
        cases.append((b"TGR3", 1, 0, sparse_transform(bytes(color), 1), bytes(next_color), 1))
        with tempfile.TemporaryDirectory() as folder:
            for tag, kind, group, old, raw, mode in cases:
                encoded = lzma.compress(
                    old,
                    format=lzma.FORMAT_RAW,
                    filters=[dict(id=lzma.FILTER_LZMA2, dict_size=dictionary, lc=4, lp=0, pb=1)],
                )
                original = (
                    t.GROUP.pack(
                        tag,
                        kind,
                        group,
                        len(raw),
                        len(encoded),
                        len(old) if tag == b"TGR3" else 0,
                        1,
                        mode,
                    )
                    + encoded
                )
                payload, _ = encode_same(raw, original, reader, group, Path(folder))
                self.assertEqual(payload[:4], tag)
                transformed_size = t.GROUP.unpack_from(payload)[5] if tag == b"TGR3" else len(raw)
                decoded = t.decode_lzma2(payload[32:], transformed_size, dictionary)
                decoded = (
                    sparse_restore(decoded, 1, mode)
                    if tag == b"TGR3"
                    else restore_tiles(decoded, mode)
                    if kind == 1
                    else index_transform(decoded, 1, 3, True, 1360)
                    if mode
                    else decoded
                )
                self.assertEqual(decoded, raw)


if __name__ == "__main__":
    unittest.main()
