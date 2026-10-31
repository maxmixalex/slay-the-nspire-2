"""Restoration cannot overwrite a newer registered source frame."""
from pathlib import Path
from types import SimpleNamespace
import json
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import content_tiles as t
from exact_artwork_ui22 import RawInputs, source_rows
from retile_appended_resources import view
from test_retile_appended_resources import source


class ExactArtworkTest(unittest.TestCase):
    def test_truncated_spool_is_rejected_before_tile_expansion(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); (root/'source.raw').write_bytes(b'\x00\x00')
            inputs = RawInputs()
            inputs.add(dict(image=47003, spool='source.raw', offset=2, bytes=2), root, 'source')
            with self.assertRaisesRegex(ValueError, 'image 47003 exceeds its raw spool'):
                inputs.preflight()

    def test_raw_spool_is_bound_to_both_original_and_current_pixels(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); raw = b'\x00\x00'; (root/'source.raw').write_bytes(raw)
            inputs = RawInputs(); row = dict(image=7, width=1, height=1, format=4,
                spool='source.raw', offset=0, bytes=2, raw_sha256=t.sha256(raw), current_sha256=t.sha256(raw))
            inputs.add(row, root, 'original')
            try:
                self.assertEqual(inputs.decode(7, (1,1,4,raw)), (1,1,4,raw))
                with self.assertRaisesRegex(ValueError, 'newer source pixels'):
                    inputs.decode(7, (1,1,4,b'\x01\x00'))
            finally:
                inputs.close()

    def test_new_registered_source_wins_over_all_older_manifests(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); original = view(source())
            actor = dict(complete=True, output_sha256=t.sha256(original.data), replaced_images=[4],
                         clips=[dict(registered_frames=[[(4,0,0)]])])
            restoration = dict(original_spool=dict(file=str(root/'old.raw')),
                               restored_images=[dict(image=4, width=1, height=1, format=4,
                                   offset=0, bytes=2, raw_sha256=t.sha256(b'\0\0'))])
            derived = dict(file=str(root/'derived.raw'), rows=restoration['restored_images'])
            for name, data in (('actor',actor),('original',restoration),('derived',derived)):
                (root/(name+'.json')).write_text(json.dumps(data))
            args = SimpleNamespace(actor_receipt=root/'actor.json', restoration=root/'original.json',
                                   replacements=[root/'derived.json'], necrobinder_selection=None)
            inputs, selected, report = source_rows(args, original)
            self.assertEqual(selected, {4}); self.assertNotIn(4, inputs.rows)
            self.assertEqual(report['rebaked_actor_images'], 1)


if __name__ == '__main__':
    unittest.main()
