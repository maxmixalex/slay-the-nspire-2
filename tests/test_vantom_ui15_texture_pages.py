"""Exact native texel and parameter preservation across a mesh page boundary."""
import array
import copy
from pathlib import Path
import sys
from tempfile import TemporaryDirectory
import unittest

import numpy as np
from PIL import Image

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from patch_feedback_assets import write_image
import vantom_ui15_texture_pages as p


class VantomTexturePageTests(unittest.TestCase):
    def fixture(self,folder):
        rows=[]
        for image,size in ((10,(352,274)),(11,(461,12)),(12,(461,12))):
            width,height=size;y,x=np.indices((height,width));rgba=np.empty((height,width,4),dtype=np.uint8)
            rgba[:,:,0]=x%256;rgba[:,:,1]=y%256;rgba[:,:,2]=(x+y)%256;rgba[:,:,3]=(x*3+y*7)%256
            picture=Image.fromarray(rgba);name=str(image);row=write_image(folder,name,picture)
            row.update(image=image,png=name+'.png',preserve_payload=True);picture.save(folder/row['png']);rows.append(row)
        geometry=p.words_image(folder,'geometry',[0x564d,1]+[0]*14);geometry.update(image=13,raw_words=True);rows.append(geometry)
        words=[0x5654,2,1,33,1024,0,0,0,6,12,0,0,17,2,0,0,29,
               10,13,0,3,2,65535,11,13,0,3,3,12,2,512,1024]+list(range(112))
        words[10]=len(words);parameter=p.words_image(folder,'parameters',words)
        return dict(parameter_version=2,first_parameter_image=1000,
            images=[copy.deepcopy(parameter) for _ in range(116)],replacement_images=rows,
            unique_images=4,appended_layer_images=0,directory_added_images=116),words

    def test_exact_rgb565_alpha8_and_curve_words_survive_native_pages(self):
        with TemporaryDirectory() as directory:
            folder=Path(directory);source,old=self.fixture(folder)
            original={r['image']:p.decoded(r,folder) for r in source['replacement_images']}
            result=p.bounded_catalog(source,folder)
            self.assertEqual(result['continuation_images'],1)
            self.assertEqual(result['texture_page_proof']['changed_textures'],3)
            rows={r['image']:r for r in result['replacement_images']}
            rows.update({1000+i:r for i,r in enumerate(result['images'])})
            for row in rows.values():self.assertLessEqual(row['width'],320);self.assertLessEqual(row['height'],240)
            words=array.array('H',p.decoded(result['images'][0],folder))
            self.assertEqual(words[:2].tolist(),[0x5654,3]);self.assertEqual(words[8],7)
            first=words[12];extra=words[first+6];self.assertEqual(extra,1116)
            self.assertEqual(words[first:first+6].tolist(),old[17:23])
            self.assertEqual(words[first+7:first+13].tolist(),old[23:29])
            self.assertEqual(words[words[16]:words[10]].tolist(),old[29:])
            for image,count in ((10,352*274),(11,461*12),(12,461*12)):
                parts=[rows[image]]+([rows[extra]] if image==10 else [])
                rgb=[];alpha=[];left=count
                for row in parts:
                    raw=p.decoded(row,folder);length=min(left,p.PAGE_PIXELS);at=row['width']*row['height']*2
                    rgb.append(raw[:length*2]);alpha.append(raw[at:at+length]);left-=length
                self.assertEqual(b''.join(rgb+alpha),original[image])
            self.assertEqual(p.decoded(rows[13],folder),original[13])

    def test_a_mask_cannot_silently_lose_its_continuation(self):
        with TemporaryDirectory() as directory:
            folder=Path(directory);source,words=self.fixture(folder)
            words[28]=10;parameter=p.words_image(folder,'masked-parameters',words)
            source['images']=[copy.deepcopy(parameter) for _ in range(116)]
            with self.assertRaisesRegex(ValueError,'dissolve mask'):
                p.bounded_catalog(source,folder)


if __name__=='__main__':unittest.main()
