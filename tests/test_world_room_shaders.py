"""Asset-free checks of room shader math, resource decoding and geometry."""
from pathlib import Path
import sys
import unittest
from unittest.mock import patch
import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from world_scene_assets import Node, Property, SceneLoader, SceneRenderError, _Renderer
from world_room_shaders import (RoomRenderer, SHADERS, fire_fragment, generated_texture,
                                light_fragment, quad_uv, sample_texture)


class Pack:
    def __init__(self, documents):
        self.documents = documents
        self.entries = dict.fromkeys(documents)

    def read(self, path):
        return self.documents[path.removeprefix('res://')].encode()


def solid(*values):
    return np.array([[values]], dtype=np.float32)


def fire_inputs():
    return dict(Noise1Texture=solid(0, 0, .5, 1), Noise2Texture=solid(0, 0, .5, 1),
                NoiseMask=solid(0, 0, .25, 1), Distortion1Texture=solid(0, 0, 0, 1),
                Distortion2Texture=solid(0, 0, 0, 1), DistortionMask=solid(1, 1, 1, 1))


def curve_resource(*, tangent=0., resolution=100, mode=0):
    return f'''[gd_resource type="CurveXYZTexture" format=3]
[sub_resource type="Curve" id="X"]
_data = [Vector2(0, 1), {tangent}, {tangent}, {mode}, 0, Vector2(1, 2), {tangent}, {tangent}, 0, 0]
point_count = 2
bake_resolution = {resolution}
[resource]
width = 4
curve_x = SubResource("X")
'''


class RoomShaderMath(unittest.TestCase):
    def test_sampler_uses_half_texel_wrap_and_clamp_not_pixel_endpoints(self):
        texture = np.array([[[0.], [1.]]], np.float32)
        uv = np.array([[0, .5], [1, .5], [.25, .5], [.75, .5], [-.25, .5]], np.float32)
        np.testing.assert_allclose(sample_texture(texture, uv).ravel(), [0, 1, 0, 1, 0])
        np.testing.assert_allclose(sample_texture(texture, uv, repeat=True).ravel(), [.5, .5, 0, 1, 1])
        with self.assertRaisesRegex(SceneRenderError, 'Nonfinite'):
            sample_texture(texture, np.array([np.inf, 0]))

    def test_curve_xyz_retains_values_above_one_and_samples_i_over_width(self):
        loader = SceneLoader(Pack({'curve.tres': curve_resource(tangent=1.)}))
        pixels = generated_texture(loader, 'curve.tres')
        self.assertEqual(pixels.dtype, np.float32)
        np.testing.assert_allclose(pixels[0, :, 0], [1., 1.25, 1.5, 1.75], atol=2e-7)
        np.testing.assert_array_equal(pixels[0, :, 1:], np.zeros((4, 2)))
        self.assertEqual(float(sample_texture(pixels, np.array([.5, 0.]))[0]), 1.375)

    def test_curve_uses_bake_cache_before_texture_sampling(self):
        # A zero-tangent cubic sampled into a three-entry cache becomes linear
        # between endpoints and its midpoint. Direct cubic sampling gives 1.15625.
        loader = SceneLoader(Pack({'curve.tres': curve_resource(resolution=3)}))
        pixels = generated_texture(loader, 'curve.tres')
        self.assertAlmostEqual(float(pixels[0, 1, 0]), 1.25)
        loader = SceneLoader(Pack({'curve.tres': curve_resource(mode=1)}))
        with self.assertRaisesRegex(SceneRenderError, 'tangent mode'):
            generated_texture(loader, 'curve.tres')

    def test_gradient_includes_endpoint_and_truncates_ldr_bytes(self):
        source = '''[gd_resource type="GradientTexture1D" format=3]
[sub_resource type="Gradient" id="Ramp"]
offsets = PackedFloat32Array(0, 1)
colors = PackedColorArray(0, 0, 0, 0, 1, 1, 1, 1)
[resource]
width = 3
gradient = SubResource("Ramp")
'''
        pixels = generated_texture(SceneLoader(Pack({'ramp.tres': source})), 'ramp.tres')
        np.testing.assert_allclose(pixels[0, :, 0], [0, 127/255, 1])
        unsupported = source.replace('offsets = ', 'interpolation_color_space = 1\noffsets = ')
        with self.assertRaisesRegex(SceneRenderError, 'linear sRGB'):
            generated_texture(SceneLoader(Pack({'ramp.tres': unsupported})), 'ramp.tres')

    def test_fire_reads_blue_and_overwrites_alpha_with_its_own_coverage(self):
        uv = np.array([[[.5, .5]]], np.float32)
        uniforms = dict(InvertNoiseMask=False, InnerColorStep=(0, .5), OuterStep=(0, .5),
                        OuterColor=(.2, .1, .3, 0), InnerColor=(.4, .6, .2, 0))
        # (noise .5*.5 + mask .25) * blue .5 = .25, smoothstep = .5.
        pixels = fire_fragment(solid(1, 0, .5, 0), uv, 0, uniforms, fire_inputs())
        np.testing.assert_allclose(pixels, [[[(.2 + .2), (.1 + .3), (.3 + .1), .5]]], atol=1e-7)
        inverted = fire_fragment(solid(1, 0, .5, 0), uv, 0, dict(uniforms, InvertNoiseMask=True), fire_inputs())
        self.assertEqual(float(inverted[0, 0, 3]), 1.)

    def test_dark_fire_reverses_inner_threshold_and_multiplies_outer_color(self):
        pixels = fire_fragment(solid(0, 0, .5, 1), np.array([[[.5, .5]]], np.float32), 0,
            dict(InvertNoiseMask=False, InnerColorStep=(.5, 1), OuterStep=(0, .5),
                 OuterColor=(.2, .4, .6, 1), InnerColor=(.1, .2, .3, 1)), fire_inputs(), dark=True)
        np.testing.assert_allclose(pixels, [[[.12, .28, .48, .5]]], atol=1e-7)

    def test_fire_time_and_global_offset_pan_original_repeating_noise(self):
        textures = fire_inputs()
        textures['Noise1Texture'] = np.array([[[0, 0, 0, 1], [0, 0, 1, 1]]], np.float32)
        textures['Noise2Texture'] = solid(0, 0, 1, 1)
        textures['NoiseMask'] = solid(0, 0, 0, 1)
        uniforms = dict(InvertNoiseMask=False, OuterStep=(0, 1), Noise1Panning=(.5, 0))
        uv = np.array([[[.25, .5]]], np.float32)
        first = fire_fragment(solid(0, 0, 1, 1), uv, 0, uniforms, textures)
        later = fire_fragment(solid(0, 0, 1, 1), uv, 1, uniforms, textures)
        shifted = fire_fragment(solid(0, 0, 1, 1), uv, 0,
            dict(uniforms, GlobalOffset=(.5, 0)), textures, dark=True)
        self.assertEqual(float(first[0, 0, 3]), 0.)
        self.assertEqual(float(later[0, 0, 3]), 1.)
        self.assertEqual(float(shifted[0, 0, 3]), 1.)

    def test_light_uses_model_scale_and_each_alpha_once_ignoring_ramp_alpha(self):
        texture = np.array([[[.2, .4, .6, 1], [.8, .6, .4, .5]]], np.float32)
        uv = np.array([[[.75, .5]]], np.float32)
        matrix = np.array([[0., -3., 123.], [-2., 0., -99.], [0., 0., 1.]])
        # Model x scale=2; one pixel offset becomes .25 UV, so distorted
        # texture sampling is exactly between texels. Camera scale is absent.
        pixels = light_fragment(texture, uv, 0, solid(1, 1, 0), solid(.5, 1, .25, 0),
            [1, 0], matrix, [.5, 1, .5, .4])
        np.testing.assert_allclose(pixels, [[[.125, .5, .0625, .15]]], atol=1e-7)

    def test_quad_uv_keeps_camera_and_negative_model_geometry(self):
        screen = np.array([[2., 0, 1], [0, 2., 0], [0, 0, 1.]])
        matrix = np.array([[-1., 0, 3], [0, 1., 1], [0, 0, 1.]])
        uv, mask = quad_uv((8, 6), screen, matrix, (0, 0, 2, 1))
        self.assertEqual(int(np.count_nonzero(mask)), 8)
        np.testing.assert_allclose(uv[2, 3], [.875, .25])
        np.testing.assert_allclose(uv[3, 6], [.125, .75])


class RoomShaderIntegration(unittest.TestCase):
    def renderer(self, renderer_type=RoomRenderer, pack=None):
        return renderer_type(pack or Pack({}), 4, 2, (4, 2),
            lambda pack, path: Image.new('RGBA', (2, 1), 'white'), None, 0., 1)

    def test_unhandled_shader_and_default_scene_renderer_behavior_is_unchanged(self):
        node = Node('Solid', 'ColorRect', 'fixture', {
            'offset_right': Property('4', 'fixture'), 'offset_bottom': Property('2', 'fixture'),
            'color': Property('Color(.2,.4,.8,.5)', 'fixture')})
        outputs = []
        for cls in (_Renderer, RoomRenderer):
            renderer = self.renderer(cls)
            renderer.visit(node, np.eye(3), np.array([4, 2]), np.ones(4), 0, 'Solid')
            outputs.append(renderer.finish())
        np.testing.assert_array_equal(np.asarray(outputs[0][0]), np.asarray(outputs[1][0]))
        self.assertEqual(outputs[0][1], outputs[1][1])
        self.assertIsNone(self.renderer().shader_layer(node, 'Solid', None, None, None,
            None, 'unsupported.gdshader', {}, 0, {}))

    def test_reviewed_shader_source_drift_fails_instead_of_drawing_mask(self):
        shader = 'scenes/rest_site/rest_site_light_shader.gdshader'
        renderer = self.renderer(pack=Pack({shader: 'different shader source'}))
        with self.assertRaisesRegex(SceneRenderError, 'fingerprint drift'):
            renderer._verify_shader(shader)

    def test_required_textures_and_curve_divisors_do_not_silently_fall_back(self):
        with self.assertRaisesRegex(SceneRenderError, 'texture is missing'):
            self.renderer()._shader_texture(None)
        with self.assertRaisesRegex(SceneRenderError, 'distortion divisor'):
            light_fragment(solid(1, 1, 1, 1), np.array([[[.5, .5]]]), 0,
                solid(0, 1, 0), solid(1, 1, 1, 1), [0, 0], np.eye(3), np.ones(4))

    def test_shader_hook_flips_uv_for_noise_and_does_not_apply_node_tint_again(self):
        shader = 'shaders/vfx/vfx_stepped_shader_fire_flat.tres'
        renderer = self.renderer()
        renderer.room_shader_time = 0.
        renderer.time = 5.
        node = Node('Fire', 'Sprite2D', 'fixture', {'flip_h': Property('true', 'fixture')})
        image = Image.fromarray(np.array([[[0, 0, 128, 255], [0, 0, 255, 255]]], np.uint8))
        flipped = image.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
        captured = {}
        def fragment(main, uv, time, uniforms, textures, **settings):
            captured.update(main=main, uv=uv, time=time)
            return np.broadcast_to([.2, .4, .6, .8], (*uv.shape[:-1], 4)).copy()
        with patch.object(renderer, '_verify_shader'), patch.object(renderer, '_shader_texture', return_value=solid(1, 1, 1, 1)), patch('world_room_shaders.fire_fragment', side_effect=fragment):
            result, blend = renderer.shader_layer(node, 'Fire', flipped, (0, 0, 2, 1),
                np.eye(3), np.zeros(4), shader, {}, 0, {})
        self.assertEqual(blend, 0)
        self.assertEqual(captured['time'], 0.)
        self.assertAlmostEqual(float(captured['uv'][0, 0, 0]), .75)
        np.testing.assert_array_equal(captured['main'], np.asarray(image, np.float32) / 255.)
        self.assertEqual(result.getpixel((0, 0)), (51, 102, 153, 204))
        self.assertEqual(result.getpixel((2, 0))[3], 0)
        self.assertEqual(renderer.diagnostics['room_shaders'][0]['shader_sha256'], SHADERS[shader][1])


if __name__ == '__main__':
    unittest.main()
