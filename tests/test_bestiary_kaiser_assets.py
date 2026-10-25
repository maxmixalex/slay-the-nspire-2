"""Source-clock supplements preserve world pixels and every prior catalogue."""
from pathlib import Path
import math
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

import numpy as np
from PIL import Image

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import bestiary_kaiser_assets as bestiary
import compact_tile_resources as compact
import content_tiles as tiles
from patch_feedback_assets import write_image
from patch_frontend_assets import patch_pack
import split_content as split
from test_compact_tile_resources import grouped_text
from test_compact_ui6_merchant import source_pack


class BestiaryKaiserAssetsTest(unittest.TestCase):
    def test_only_reachable_actions_and_original_sampling_clock(self):
        jobs=bestiary.jobs()
        self.assertEqual(len(jobs),18);self.assertEqual(sum(j['frames'] for j in jobs),344)
        self.assertEqual({j['move'] for j in jobs if j['model']=='Crusher'},{-1,0,1,2,3,4})
        self.assertEqual({j['move'] for j in jobs if j['model']=='Rocket'},{-1,0,1,2,3,4})
        for job in jobs:
            values=bestiary.frame_times(job,job['frames'])
            self.assertEqual(len(values),job['frames']);self.assertEqual(values[0],0)
            self.assertEqual(values[-1],(job['frames']-1)*job['duration']/job['frames'])
            self.assertTrue(all(a<b for a,b in zip(values,values[1:])))
        with self.assertRaises(ValueError):bestiary.frame_times(jobs[0],jobs[0]['frames']-1)
        header=(Path(__file__).resolve().parents[1]/'include/bestiary_kaiser_assets.hpp').read_text()
        for source in bestiary.SOURCES:self.assertIn(str(source),header)

    def test_missing_pixels_partition_original_nearest_sampling_without_a_seam(self):
        def raster(_draw,_scale,origin,shape):
            # A synthetic unbounded original skeleton raster makes all four
            # crop boundaries observable, including negative coordinates.
            x0,y0=np.rint(bestiary.WORLD_ORIGIN-origin).astype(int)
            x,y=np.meshgrid(np.arange(shape[0])+x0,np.arange(shape[1])+y0)
            return Image.fromarray(np.stack((x%256,y%256,(x+y)%256,np.full_like(x,255)),axis=2).astype('u1'))
        with patch.object(bestiary,'raster',raster):
            for side,left in (('Crusher',173),('Rocket',-114)):
                image=np.asarray(bestiary.missing_image([],side));expected=np.zeros((240,320,4),dtype='u1')
                for y in range(240):
                    wy=math.floor((y-23)*218/206)
                    for x in range(320):
                        wx=math.floor((x-left)*320/302)
                        if not(0<=wx<320 and 0<=wy<218):expected[y,x]=[wx%256,wy%256,(wx+wy)%256,255]
                self.assertTrue(np.array_equal(image,expected));self.assertGreater(np.count_nonzero(image[:,:,3]),0)
                _,_,owned=bestiary.coordinates(side);self.assertFalse(np.any(image[owned]))

    def test_append_keeps_tgr2_grouped_fonts_and_strings_exact(self):
        original=grouped_text(source_pack(forward=True));before,_=compact.repack(original,workers=1)
        source=compact.view(before);self.assertTrue(any(tiles.TileReader(source).filters))
        with tempfile.TemporaryDirectory() as directory:
            cache=Path(directory);image=Image.new('RGBA',(28,31),(70,160,231,0))
            for y in range(2,29):
                for x in range(3,25):image.putpixel((x,y),(70,160,231,(x*23+y*31)%256))
            rows=[write_image(cache,'synthetic-0',image,sparse=True),write_image(cache,'synthetic-1',image,sparse=True)]
            art=dict(images=rows,clips=[dict(first=0,count=2,frame_ms=83,pivot=[-57,-40],sprite_units=311)],
                     auxiliary_rows=[],localized_text={'eng':{}})
            grouped=bestiary.group_art(rows,cache)
            self.assertEqual(grouped['report']['exact_frame_aliases'],1)
            ni,ng,ns,nc,nk,na=source.counts
            after,report=patch_pack(before,art,cache,base_aux=na,auxiliary=na,input_version=33,version=34,
                                   texts=(),base_clips=nc,base_strings=ns,appended_art=grouped)
            target=tiles.PackView(after,na,versions=(34,));old,new=tiles.TileReader(source),tiles.TileReader(target)
            self.assertEqual(target.counts,(ni+2,ng,ns,nc+1,nk,na));self.assertEqual(source.clips,target.clips[:nc])
            self.assertEqual(target.clips[-1],(ni,2,83,-57,-40,311))
            self.assertEqual(source.cards,target.cards);self.assertEqual(source.aux,target.aux)
            for i,entry in enumerate(source.images):
                self.assertEqual(old.image_raw(i) if entry[3]==6 else source.image_raw(i),
                                 new.image_raw(i) if entry[3]==6 else target.image_raw(i))
            for i in range(ng):
                self.assertEqual(source.glyph_payload(i),target.glyph_payload(i))
                self.assertEqual(source.glyph_alpha(i),target.glyph_alpha(i))
            for i in range(ns):self.assertEqual(source.string_payload(i),target.string_payload(i))
            self.assertEqual(target.image_raw(ni),target.image_raw(ni+1))
            self.assertTrue(report['unchanged_old_payload']);self.assertTrue(report['unchanged_old_directory_indices'])
            self.assertEqual(split.layout(after,'eng')['version'],34)
            corrupted=bytearray(after);struct.pack_into('<I',corrupted,4,35)
            with self.assertRaises(ValueError):split.layout(corrupted,'eng')


if __name__=='__main__':unittest.main()
