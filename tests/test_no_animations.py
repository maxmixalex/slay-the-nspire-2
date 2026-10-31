import struct
import sys
import unittest
from pathlib import Path
from types import SimpleNamespace

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import no_animations as reduced


class ReducedPoseTests(unittest.TestCase):
    def test_ancient_typed_loops_keep_original_period_and_disjoint_ranges(self):
        first=58615;tables={};aux=[0]*7214
        for row,duration in enumerate(reduced.ANCIENT_DURATIONS):
            count=(duration+32)//33;index=58613+row;aux[7212+row]=index
            tables[index]=struct.pack('<8H',0x414e,1,first,count,33,duration,0,0)
            first+=count
        source=SimpleNamespace(aux=aux,images=[None]*first)
        image=lambda index:(8,1,0,tables[index])
        rows=reduced.ancient_rows(source,image)
        self.assertEqual([row['frames'] for row in rows],[546,506])
        self.assertEqual([row['duration_ms'] for row in rows],[18000,16667])
        tables[58614]=struct.pack('<8H',0x414e,1,58615,506,33,16667,0,0)
        with self.assertRaisesRegex(ValueError,'overlap'):
            reduced.ancient_rows(source,image)
        source.aux.append(0)
        with self.assertRaisesRegex(ValueError,'Unknown'):
            reduced.ancient_rows(source,image)

    def test_action_clocks_and_terminal_not_early(self):
        for count in range(1, 257):
            mapped = reduced.sample_map(count, False)
            self.assertEqual(len(mapped), count)
            self.assertLessEqual(len(set(mapped)), 3)
            self.assertEqual(mapped[0], 0)
            self.assertEqual(mapped[-1], count - 1)
            self.assertTrue(all(sample <= frame for frame, sample in enumerate(mapped)))
            if count > 1:
                self.assertNotIn(count - 1, mapped[:-1])
            self.assertEqual(reduced.sample_map(count, True), [0] * count)

    def test_reject_bad_counts(self):
        for count in (0, -1, 257):
            with self.assertRaises(ValueError):
                reduced.sample_map(count, False)

    def test_complete_semantic_policy(self):
        policy = reduced.clip_policy()
        self.assertEqual(set(policy), set(range(1783)))
        for clip in (0, 11, 17, 22, 27, 32, 1705, 1709):
            self.assertTrue(policy[clip])
        for clip in (1, 2, 3, 33, 34, 35, 36, 1753, 1764):
            self.assertFalse(policy[clip])

    def test_profile_is_bounded_and_old_checksums_are_not_modes(self):
        header = bytearray(40)
        header[:4] = b'NSP2'
        struct.pack_into('<I', header, 4, 46)
        self.assertFalse(reduced.reduced(header))
        marked = reduced.mark(header)
        self.assertTrue(reduced.reduced(marked))
        self.assertEqual(marked[:36], header[:36])
        old = bytearray(marked); struct.pack_into('<I', old, 4, 45)
        self.assertFalse(reduced.reduced(old))
        with self.assertRaises(ValueError):
            reduced.mark(old)
        self.assertFalse(reduced.reduced(marked[:39]))

    @staticmethod
    def framed_fixture():
        # Variable piece counts/registration, including an empty terminal
        # picture ID. The converter must choose whole poses, not mix layers.
        frames = [[(100+i, 7+i, 65530+i), *([(200+i, 65535, 3)] if i == 2 else [])]
                  for i in range(5)]
        words = [0x4652, 1, 5, 83, 5376, 0, 9500, 0, 0, 3] + [0] * 10
        for i, pieces in enumerate(frames):
            words[10+i*2:12+i*2] = [len(words), len(pieces)]
            for row in pieces:
                words.extend(row)
        words[8] = len(words)
        return struct.pack('<'+'H'*len(words), *words), frames

    def test_registered_whole_pose_and_identity(self):
        raw, original = self.framed_fixture()
        (width, height, fmt, output), before, kept = reduced.reduce_framed(raw, False, 100)
        words, frames = reduced.framed_rows(output)
        self.assertEqual(words[:8], list(struct.unpack_from('<8H', raw)))
        self.assertEqual(len(output), width*height*2)
        self.assertEqual(fmt, 0)
        for frame, selected in enumerate([0, 0, 2, 2, 4]):
            self.assertEqual(frames[frame][0], (100+frame, *original[selected][0][1:]))
            self.assertEqual(frames[frame][1:], original[selected][1:])
        self.assertEqual(kept, {100,102,104,202})
        self.assertEqual(before, {100,101,102,103,104,202})

    def test_dead_loop_retains_exact_source_pose(self):
        raw, frames = self.framed_fixture()
        (_, _, _, output), _, kept = reduced.reduce_framed(raw, True)
        self.assertEqual(reduced.framed_rows(output)[1], [frames[0]]*5)
        self.assertEqual(kept, {100})

    def test_vantom_preserves_mesh_mask_and_emitter_relationships(self):
        words = [0x5654,3,5,81,4254,0,1,2,7,12,0,0] + [0]*25
        for frame in range(5):
            words[12+frame*5:17+frame*5] = [len(words),1,frame,frame+1,123+frame]
            # Texture, geometry ID, offset, mesh mode, depth, mask, continuation.
            words.extend([100+frame,200+frame,0,3,2,300+frame,400+frame])
        words[10] = len(words)
        raw = struct.pack('<'+'H'*len(words), *words)
        output, before, after = reduced.reduce_vantom(raw, False)
        result = list(struct.unpack('<'+'H'*len(words), output))
        self.assertEqual(result[:12], words[:12])
        self.assertEqual(result[37:], words[37:])
        for frame, selected in enumerate([0,0,2,2,4]):
            self.assertEqual(result[12+frame*5:17+frame*5], words[12+selected*5:17+selected*5])
        self.assertEqual(len(before), 20)
        self.assertEqual(after, {base+i for base in (100,200,300,400) for i in (0,2,4)})


if __name__ == '__main__':
    unittest.main()
