import contextlib
from pathlib import Path
import re
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import ancient_extensions_ui23 as e


class TypedCatalogIntegration(unittest.TestCase):
    def test_only_complete_endpoints_and_runtime_catalog_agree(self):
        source=Path(__file__).resolve().parents[1]/'src/content.cpp'
        text=source.read_text().split('ancient_catalog[][2]={',1)[1].split('};',1)[0]
        self.assertEqual(dict((int(a),int(b)) for a,b in re.findall(r'\{(\d+),(\d+)\}',text)),e.ENDPOINTS)
        for aux,images in e.ENDPOINTS.items():
            self.assertEqual(e.catalog(SimpleNamespace(aux=[0]*aux,images=[0]*images)),aux)
            for da,di in ((0,-1),(0,1),(-1,0),(1,0)):
                with self.subTest(aux=aux,da=da,di=di),self.assertRaises(ValueError):
                    e.catalog(SimpleNamespace(aux=[0]*(aux+da),images=[0]*(images+di)))

    def test_each_completed_prefix_validates_actual_final_payload_and_profile(self):
        modules=[('tanx_scene_ui23',7215),('ancient_lights_ui23',7217),
                 ('ancient_materials_ui23',7220),('ancient_gpu_masks_ui23',7221),
                 ('tez_cinders_ui23',7222),('nonu_fog_ui23',7223),('ancient_turbulence_ui23',7224),
                 ('neow_post_assets_ui23',7225),('vakuu_cinders_ui23',7226),('neow_particles_ui23',7227)]
        marker=object();image=lambda i:('actual-final-file',i)
        for end in e.ENDPOINTS:
            source=SimpleNamespace(aux=list(range(end)),images=list(range(e.ENDPOINTS[end])),data=marker,version=46)
            calls=[]
            def validator(expected):
                def check(prefix,callback,**kwargs):
                    self.assertEqual((len(prefix.aux),len(prefix.images)),(expected,e.ENDPOINTS[expected]))
                    self.assertIs(prefix.data,marker);self.assertIs(callback,image)
                    self.assertEqual(callback(len(prefix.images)-1),('actual-final-file',len(prefix.images)-1))
                    self.assertEqual(kwargs,{'final':True} if expected in (7220,7225) else {})
                    calls.append(expected)
                return check
            with contextlib.ExitStack() as stack:
                for module,count in modules:stack.enter_context(patch(module+'.validate',validator(count)))
                roots=e.validate(source,image,final=True)
            self.assertEqual(calls,[count for _,count in modules if count<=end])
            self.assertEqual(roots,set(range(59667,e.ENDPOINTS[end])) if end>=7215 else set())

    def test_failure_in_an_earlier_prefix_is_not_hidden_by_new_extension(self):
        source=SimpleNamespace(aux=[0]*7227,images=[0]*59732)
        with patch('tanx_scene_ui23.validate',side_effect=ValueError('corrupt original typed root')):
            with self.assertRaisesRegex(ValueError,'corrupt original'):e.validate(source,lambda i:None)

    def test_source_recipe_chains_every_real_producer_before_quality_and_reduction(self):
        from converter import Recipe
        from converter_finalize import add_final_stages
        names=['ancient-materials46','ancient-gpu-masks46','tez-cinders46','nonu-fog46','ancient-trajectories46',
               'neow-post46','vakuu-cinders46','neow-particles46']
        for reduced in (False,True):
            recipe=Recipe('/game','/work','/private/runtime','dotnet','/noise','-')
            add_final_stages(recipe,recipe.source_stages(),Path('/output'),['eng','zhs'],reduced)
            stages={stage.name:stage for stage in recipe.stages}
            for before,after in zip(names,names[1:]):
                stage=stages[after]
                self.assertEqual(stage.arguments[stage.arguments.index('--input')+1],str(stages[before].outputs[0]))
                self.assertEqual(stage.outputs[1],stage.outputs[0].with_suffix('.json'))
            self.assertEqual(stages['quality46'].arguments[0],str(stages[names[-1]].outputs[0]))
            cinder=stages['tez-cinders46'].arguments
            self.assertEqual(cinder[cinder.index('--runtime')+1],'/private/runtime')
            self.assertEqual(cinder[cinder.index('--light-work')+1],'/work/cache/ancient-scenes45')
            self.assertNotIn('--sampled',stages['ancient-trajectories46'].arguments)
            self.assertEqual('--no-animations' in stages[names[0]].arguments,reduced)
            self.assertEqual('--no-animations' in stages['neow-post46'].arguments,reduced)
            post=stages['neow-post46'].arguments
            self.assertEqual(post[post.index('--noise-header')+1],'/noise')
        recipe=Recipe('/game','/work','/private/runtime','dotnet','/noise','-')
        recipe.trajectory_samples='/verified/source-samples'
        add_final_stages(recipe,recipe.source_stages(),Path('/output'),['eng'],False)
        args=next(s for s in recipe.stages if s.name=='ancient-trajectories46').arguments
        self.assertEqual(args[-2:],('--sampled','/verified/source-samples'))


if __name__=='__main__':unittest.main()
