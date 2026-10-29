"""Synthetic locality checks for complete artwork concern spans."""
from pathlib import Path
import random
import sys
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import storage_parts as storage


class ConcernLocalityTest(unittest.TestCase):
    def test_page_sized_islands_remove_only_existing_boundaries(self):
        for size in (1,16,1714,2900,storage.BLOCK):
            with self.subTest(size=size):
                spans=[(64,10000,0),(10000,10000+size,5),
                       (10000+size,30000,0)]
                self.assertEqual(storage.fold_small_concern_spans(spans),[(64,30000,0)])

    def test_large_island_keeps_its_whole_decoder_group(self):
        spans=[(64,10000,0),(10000,10000+storage.BLOCK+1,5),
               (10000+storage.BLOCK+1,30000,0)]
        self.assertEqual(storage.fold_small_concern_spans(spans),spans)

    def test_edges_and_different_neighbors_keep_their_concern(self):
        for spans in ([(64,80,5)],[(64,80,5),(80,10000,0)],
                      [(64,10000,0),(10000,10016,5)],
                      [(64,10000,0),(10000,10016,5),(10016,20000,1)]):
            self.assertEqual(storage.fold_small_concern_spans(spans),spans)

    def test_interleaved_islands_use_no_scene_or_language_ids(self):
        for owner in range(len(storage.NAMES)):
            other=(owner+1)%len(storage.NAMES)
            spans=[(64,10000,owner),(10000,11000,other),(11000,16000,owner),
                   (16000,17000,other),(17000,30000,owner)]
            self.assertEqual(storage.fold_small_concern_spans(spans),[(64,30000,owner)])

    def test_adjacent_equal_spans_merge_before_checking_island_size(self):
        spans=[(64,10000,0),(10000,12050,5),(12050,14100,5),(14100,30000,0)]
        self.assertEqual(storage.fold_small_concern_spans(spans),
                         [(64,10000,0),(10000,14100,5),(14100,30000,0)])

    def test_every_request_has_no_more_file_fragments(self):
        rng=random.Random(41)
        for _ in range(100):
            source=[];at=64
            for _ in range(rng.randrange(1,60)):
                end=at+rng.randrange(1,20000);source.append((at,end,rng.randrange(8)));at=end
            folded=storage.fold_small_concern_spans(source)
            before={first for first,_,_ in source[1:]}
            after={first for first,_,_ in folded[1:]}
            self.assertTrue(after<=before)
            self.assertEqual((folded[0][0],folded[-1][1]),(64,at))
            self.assertEqual(sum(b-a for a,b,_ in folded),at-64)
            for first,last,_ in source:
                self.assertTrue(any(a<=first and last<=b for a,b,_ in folded))
            self.assertEqual(storage.fold_small_concern_spans(folded),folded)
            for _ in range(10):
                first=rng.randrange(64,at);last=rng.randrange(first+1,at+1)
                self.assertLessEqual(sum(first<x<last for x in after),
                                     sum(first<x<last for x in before))

    def test_rejects_invalid_or_discontinuous_spans(self):
        for spans in ([(64,64,0)],[(64,65,8)],[(64,65,-1)],
                      [(64,100,0),(101,200,5)],[(64,100,0),(99,200,5)]):
            with self.assertRaises(ValueError):storage.fold_small_concern_spans(spans)


if __name__=='__main__':unittest.main()
