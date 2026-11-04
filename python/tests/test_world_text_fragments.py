import unittest

from nspire_converter import build_world_text_fragments as fragments
from nspire_converter.source_tables import cpp_tokens


class FragmentTableTest(unittest.TestCase):
    def test_current_generated_table_matches_authoritative_keys(self):
        source = fragments.SOURCE.read_text()
        self.assertEqual(
            cpp_tokens(fragments.OUTPUT.read_text()), cpp_tokens(fragments.render(source))
        )
        rows = fragments.read_rows(source)
        table = fragments.pack(rows)
        self.assertEqual(len(rows), 4361)
        for i, (key, value) in enumerate(rows):
            prefix = next(
                s for s, o in table["prefixes"].offsets.items() if o == table["prefix_offsets"][i]
            )
            suffix = next(
                s for s, o in table["suffixes"].offsets.items() if o == table["suffix_offsets"][i]
            )
            self.assertEqual(prefix + suffix, key.split(".", 1)[1])
            self.assertEqual(table["values"][i], value)

    def test_empty_and_repeated_fragments_keep_their_exact_identity(self):
        rows = fragments.read_rows(
            '{".empty_group",65535},\n{"a.",0},\n{"a.same.part",1},\n{"b.same.part",2},\n{"b.same.tail",3},\n'
        )
        table = fragments.pack(rows)
        self.assertEqual(table["prefix_offsets"][2:], [table["prefix_offsets"][2]] * 3)
        self.assertEqual(table["suffix_offsets"][2], table["suffix_offsets"][3])
        self.assertNotEqual(table["suffix_offsets"][3], table["suffix_offsets"][4])
        self.assertEqual(table["first"], [0, 1, 3])
        self.assertEqual(table["end"], [1, 3, 5])

    def test_invalid_rows_and_offset_overflow_fail(self):
        for source in (
            "",
            '{"key",1},',
            '{"a.key",65536},',
            '{"b.key",1},\n{"a.key",0},',
            '{"a.key",1},\n{"a.key",2},',
            '{"a.key",some_macro},',
        ):
            with self.subTest(source=source), self.assertRaises(ValueError):
                fragments.read_rows(source)
        pool = fragments.Pool()
        self.assertEqual(pool.intern("x" * 65535), 0)
        with self.assertRaises(ValueError):
            pool.intern("y")


if __name__ == "__main__":
    unittest.main()
