"""Bestiary's explicit scene faces must not expand every display-size band."""

import re
import unittest

from nspire_converter import replace_original_glyphs as fonts
from nspire_converter.paths import ROOT


class BestiaryFontBands(unittest.TestCase):
    def source(self):
        root = ROOT / "cpp/src"
        indices = (root / "detail/data/world_text_index.hpp").read_text() + (
            root / "detail/data/text_index.hpp"
        ).read_text()
        slots = {
            key: int(slot) for key, slot in re.findall(r'\{\s*"([^"]+)"\s*,\s*(\d+)\s*\}', indices)
        }

        class Source:
            version = 28
            strings = [(0, 0)] * 8704
            blocks = {}

            def block(self, offset, length):
                return self.blocks[offset][:length]

        source = Source()
        values = {
            "monsters.NIBBIT.name": "\u4e00",
            "encounters.DECIMILLIPEDE_ELITE.title": "\u4e01",
            "bestiary.LOCKED.monsterTitle": "\u4e02",
            "bestiary.ACTIONS.header": "\u4e03",
            "bestiary.MODE.viewActions": "\u4e04",
            "bestiary.MODE.viewStats": "\u4e05",
            "acts.OVERGROWTH.title": "\u4e06",
            "bestiary.EVENTS.title": "\u4e07",
            "bestiary.QUOTE_PLACEHOLDER": "[i]\u4e08[/i]",
            # Ordinary prose stays in the existing full regular alphabet.
            "bestiary.STATS.layout": "\u4e09",
            "monsters.NIBBIT.moves.BUTT.title": "\u4e0a",
        }
        for offset, (key, value) in enumerate(values.items(), 1):
            raw = value.encode("utf-8")
            source.blocks[offset] = raw
            source.strings[slots[key]] = (offset, len(raw))
        return source

    def test_only_authored_faces_and_fixed_sizes(self):
        source = self.source()
        bands = fonts.bestiary_scene_alphabets(source)
        self.assertEqual({cp for cp in bands[4, 10] if cp >= 0x4E00}, {0x4E00, 0x4E01, 0x4E02})
        self.assertEqual({cp for cp in bands[5, 5] if cp >= 0x4E00}, {0x4E03, 0x4E04, 0x4E05})
        self.assertEqual({cp for cp in bands[5, 6] if cp >= 0x4E00}, {0x4E06, 0x4E07})
        self.assertEqual({cp for cp in bands[6, 5] if cp >= 0x4E00}, {0x4E08})
        self.assertNotIn(0x4E09, set().union(*bands.values()))
        self.assertNotIn(0x4E0A, set().union(*bands.values()))
        source.version = 27
        self.assertTrue(all(not value for value in fonts.bestiary_scene_alphabets(source).values()))


if __name__ == "__main__":
    unittest.main()
