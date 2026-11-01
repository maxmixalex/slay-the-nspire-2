from pathlib import Path
import struct
import sys
import unittest
from types import SimpleNamespace
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import tanx_scene_ui23 as tanx


class TanxCatalog(unittest.TestCase):
    def test_source_append_precedes_quality_and_mandatory_final_prune_in_both_profiles(self):
        import converter
        from converter_finalize import add_final_stages
        for reduced in (False,True):
            recipe=converter.Recipe('/game','/work','/runtime','dotnet','/noise','-')
            editions=recipe.source_stages()
            add_final_stages(recipe,editions,Path('/output'),['eng','zhs'],reduced)
            stages={stage.name:stage for stage in recipe.stages};names=list(stages)
            stage=stages['tanx46'];args=stage.arguments
            self.assertEqual(stage.module,'tanx_scene_ui23')
            self.assertEqual(args[args.index('--input')+1],str(stages['pruned46'].outputs[0]))
            self.assertEqual(args[args.index('--pck')+1],str(recipe.pck))
            lights=stages['ancient-lights46'];light_args=lights.arguments
            self.assertEqual(light_args[light_args.index('--input')+1],str(stage.outputs[0]))
            self.assertEqual(light_args[light_args.index('--runtime')+1],str(recipe.runtime))
            # Later Ancient producers continue the same source pack. Follow
            # every link instead of assuming lights must remain the last one.
            by_output={str(s.outputs[0]):s for s in recipe.stages if s.outputs}
            upstream=stages['quality46'].arguments[0];visited=[]
            while upstream!=str(lights.outputs[0]):
                self.assertIn(upstream,by_output)
                current=by_output[upstream]
                self.assertNotIn(current.name,visited)
                visited.append(current.name)
                self.assertIn('--input',current.arguments)
                upstream=current.arguments[current.arguments.index('--input')+1]
            expected=names[names.index('ancient-lights46')+1:names.index('quality46')]
            self.assertEqual(visited,list(reversed(expected)))
            self.assertIn('--authored-lights',stages['ancient-scenes45'].arguments)
            self.assertLess(names.index('tanx46'),names.index('final-index46'))
            self.assertEqual(stages['final-index46'].module,'@mandatory-orphan-prune')

    def test_integer_lookup_matches_independent_rational_formula(self):
        raw=tanx.smooth_lut()
        for a in range(256):
            for b in range(256):
                x=a*b
                numerator=255*(3*x*x*65025-2*x*x*x)
                expected=(2*numerator+65025**3)//(2*65025**3)
                self.assertEqual(raw[a*256+b],expected)

    def test_exact_parameter_catalog_rejects_refs_geometry_lut_and_padding(self):
        raw=tanx.parameter_bytes(bytes(range(200)))
        row=tanx.parameters(raw,tanx.IMAGES)
        self.assertEqual(row['gradient'],bytes(range(200)))
        for at in (0,4,16,24,232+12345,len(raw)-1):
            changed=bytearray(raw);changed[at]^=1
            with self.subTest(at=at),self.assertRaises(ValueError):tanx.parameters(changed,tanx.IMAGES)
        with self.assertRaises(ValueError):tanx.parameters(raw,tanx.IMAGES+1)

    def test_six_source_masks_are_explicit_roots_with_exact_shapes_and_channels(self):
        shapes=[(32,32),(8,16),*((200,200),)*4]
        images={tanx.BASE_IMAGES+i:(w,h,1,b'\xff'*(w*h*2)+bytes(w*h)) for i,(w,h) in enumerate(shapes)}
        images[tanx.BASE_IMAGES+6]=(256,129,0,tanx.parameter_bytes(bytes(200)))
        source=SimpleNamespace(images=[None]*tanx.IMAGES,aux=[0]*tanx.BASE_AUX+[tanx.BASE_IMAGES+6])
        self.assertEqual(tanx.validate(source,images.__getitem__)['images'],tuple(range(tanx.BASE_IMAGES,tanx.BASE_IMAGES+6)))
        images[tanx.BASE_IMAGES]=(32,32,1,bytes(32*32*3))
        with self.assertRaisesRegex(ValueError,'white RGB565'):tanx.validate(source,images.__getitem__)

if __name__=='__main__':unittest.main()
