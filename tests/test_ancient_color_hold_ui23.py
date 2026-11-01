import sys
import unittest
from pathlib import Path
from unittest.mock import patch
import numpy as np
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
from ancient_color_hold_ui23 import held_colors


def frames(greens):
    return [np.array([[(20 << 11) | (g << 5) | 10]], dtype='<u2') for g in greens]


class TemporalHold(unittest.TestCase):
    def test_static_ink_is_exact_and_inputs_unchanged(self):
        frame = np.arange(768, dtype='<u2').reshape(24,32)
        inputs = [frame.copy() for _ in range(9)]
        outputs, proof = held_colors(inputs)
        self.assertTrue(all(np.array_equal(value,frame) for value in inputs+outputs))
        self.assertEqual(proof['source_static_pixels_exact'],768)
        self.assertEqual(proof['candidate_pixel_updates'],0)

    def test_exact_wrap_endpoints_and_longest_middle_segment(self):
        source = frames(range(10,21));output,proof = held_colors(source)
        self.assertEqual([int((x[0,0] >> 5) & 63) for x in output], [10,10,10,15,15,15,15,15,20,20,20])
        self.assertEqual(proof['source_exact_wrap_endpoint_pixels'],1)
        self.assertTrue(np.array_equal(output[0],source[0]) and np.array_equal(output[-1],source[-1]))

    def test_shared_cyclic_seam_and_exact_cadence(self):
        source = frames([10,11,12,13,14,13,12,11,10]);output,proof = held_colors(source)
        self.assertEqual(len(output),len(source))
        self.assertEqual([int((x[0,0] >> 5) & 63) for x in output], [10,10,10,13,13,13,10,10,10])
        self.assertTrue(proof['cyclic_seam_verified'])
        self.assertEqual(proof['source_exact_wrap_endpoint_pixels'],0)

    def test_adversarial_channels_keep_source_bounds(self):
        rng=np.random.default_rng(0x41cef1ea)
        source=[rng.integers(0,65536,size=(7,11),dtype=np.uint16) for _ in range(33)]
        output,proof=held_colors(source)
        for before,after in zip(source,output):
            for shift,mask,bound in ((11,31,1),(5,63,2),(0,31,1)):
                self.assertLessEqual(int(np.abs(((before>>shift)&mask).astype(int)-((after>>shift)&mask)).max()),bound)
        self.assertEqual(proof['frames'],33)

    def test_invalid_input_rejected(self):
        for source in ([],[np.zeros((1,1),dtype='u1')],[np.zeros((241,1),dtype='<u2')],
                       [np.zeros((1,1),dtype='<u2'),np.zeros((1,2),dtype='<u2')]):
            with self.assertRaises(ValueError):held_colors(source)

    def test_consumer_wrapper_restores_policy_even_on_failure(self):
        import ancient_scene_compact_ui23 as consumer
        before=consumer.scenes.held_colors
        def fail():
            self.assertIs(consumer.scenes.held_colors,held_colors)
            raise RuntimeError('test failure')
        with patch.object(consumer.scenes,'main',fail):
            with self.assertRaises(RuntimeError):consumer.main()
        self.assertIs(consumer.scenes.held_colors,before)

    def test_light_verifier_keeps_strict_delegate_and_labels_policy(self):
        import ancient_lights_compact_ui23 as consumer
        import ancient_scene_compact_ui23 as scenes
        original_hold=scenes.scenes.held_colors
        for policy in ('exact','cyclic-hold-r1-g2-b1-exact-fallback'):
            with patch.object(consumer.lights,'verify_scene_family',return_value={'matched_policy':policy,'frames':546}) as strict:
                def check():
                    self.assertIs(scenes.scenes.held_colors,held_colors)
                    return consumer.lights.verify_scene_family('actual-images','actual-timeline','actual-source','actual-hashes')
                with patch.object(consumer.lights,'main',check):proof=consumer.main()
                strict.assert_called_once_with('actual-images','actual-timeline','actual-source','actual-hashes')
                self.assertEqual(proof['matched_policy'],'exact' if policy=='exact' else consumer.POLICY)
                self.assertIs(consumer.lights.verify_scene_family,strict)
            self.assertIs(scenes.scenes.held_colors,original_hold)


if __name__=='__main__':unittest.main()
