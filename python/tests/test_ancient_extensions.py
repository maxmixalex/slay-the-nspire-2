import contextlib
import re
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

from nspire_converter import ancient_extensions as e
from nspire_converter.paths import ROOT


class TypedCatalogIntegration(unittest.TestCase):
    def test_only_complete_endpoints_and_runtime_catalog_agree(self):
        source = ROOT / "cpp/src/content.cpp"
        text = (
            re.sub(r"\s+", "", source.read_text())
            .split("ancient_catalog[][2]={", 1)[1]
            .split("};", 1)[0]
        )
        self.assertIn(
            "{character_unlock_assets::AuxiliaryCount,character_unlock_assets::Images}", text
        )
        text = text.replace(
            "{character_unlock_assets::AuxiliaryCount,character_unlock_assets::Images}",
            "{7237,60302}",
        )
        self.assertEqual(
            dict((int(a), int(b)) for a, b in re.findall(r"\{(\d+),(\d+)\}", text)), e.ENDPOINTS
        )
        for aux, images in e.ENDPOINTS.items():
            self.assertEqual(e.catalog(SimpleNamespace(aux=[0] * aux, images=[0] * images)), aux)
            for da, di in ((0, -1), (0, 1), (-1, 0), (1, 0)):
                with self.subTest(aux=aux, da=da, di=di), self.assertRaises(ValueError):
                    e.catalog(SimpleNamespace(aux=[0] * (aux + da), images=[0] * (images + di)))

    def test_each_completed_prefix_validates_actual_final_payload_and_profile(self):
        modules = [
            ("tanx_scene", 7215),
            ("ancient_lights", 7217),
            ("ancient_materials", 7220),
            ("ancient_gpu_masks", 7221),
            ("tez_cinders", 7222),
            ("nonu_fog", 7223),
            ("ancient_turbulence", 7224),
            ("neow_post_assets", 7225),
            ("vakuu_cinders", 7226),
            ("neow_particles", 7227),
            ("ancient_fire", 7228),
            ("neow_fire", 7229),
            ("character_unlock_assets", 7237),
        ]
        marker = object()

        def image(i):
            return ("actual-final-file", i)

        for end in e.ENDPOINTS:
            source = SimpleNamespace(
                aux=list(range(end)), images=list(range(e.ENDPOINTS[end])), data=marker, version=46
            )
            calls = []

            def validator(expected):
                def check(prefix, callback, **kwargs):
                    self.assertEqual(
                        (len(prefix.aux), len(prefix.images)), (expected, e.ENDPOINTS[expected])
                    )
                    self.assertIs(prefix.data, marker)
                    self.assertIs(callback, image)
                    self.assertEqual(
                        callback(len(prefix.images) - 1),
                        ("actual-final-file", len(prefix.images) - 1),
                    )
                    self.assertEqual(
                        kwargs, {"final": True} if expected in (7220, 7225, 7229) else {}
                    )
                    calls.append(expected)
                    if expected == 7237:
                        return set(range(60294, 60302))

                return check

            with contextlib.ExitStack() as stack:
                for module, count in modules:
                    stack.enter_context(
                        patch("nspire_converter." + module + ".validate", validator(count))
                    )
                roots = e.validate(source, image, final=True)
            self.assertEqual(calls, [count for _, count in modules if count <= end])
            self.assertEqual(roots, set(range(59667, e.ENDPOINTS[end])) if end >= 7215 else set())

    def test_failure_in_an_earlier_prefix_is_not_hidden_by_new_extension(self):
        source = SimpleNamespace(aux=[0] * 7229, images=[0] * 60294)
        with patch(
            "nspire_converter.tanx_scene.validate",
            side_effect=ValueError("corrupt original typed root"),
        ):
            with self.assertRaisesRegex(ValueError, "corrupt original"):
                e.validate(source, lambda i: None)

    def test_neow_static_source_cannot_be_published_as_full_or_dynamic_as_reduced(self):
        from nspire_converter import neow_fire as n
        from nspire_converter import no_animations

        data = bytearray(40)
        data[:4] = b"NSP2"
        data[4] = 46
        for marked, static in ((False, True), (True, False)):
            source = SimpleNamespace(
                aux=[0] * 7229,
                images=[0] * 60294,
                data=no_animations.mark(data) if marked else bytes(data),
            )
            source.aux[7228] = 60293

            def image(index):
                return (256, 9, 0, b"parameter-parser-covered-by-owner-tests")

            with patch.object(
                n, "parameters", return_value={"source_static": static, "baked_light": False}
            ):
                with self.assertRaisesRegex(ValueError, "profile does not match final marker"):
                    n.validate(source, image, final=True)
        # A static intermediate is permitted before the reducer writes NAF1;
        # it still must continue into every page/mask payload check.
        source.data = bytes(data)

        def image(index):
            if index == 60293:
                return (256, 9, 0, b"parameter-parser-covered-by-owner-tests")
            raise RuntimeError("actual source page validation reached")

        for baked in (False, True):
            with patch.object(
                n, "parameters", return_value={"source_static": True, "baked_light": baked}
            ):
                with self.assertRaisesRegex(RuntimeError, "source page validation reached"):
                    n.validate(source, image, final=False)

    def test_source_recipe_chains_every_real_producer_before_quality_and_reduction(self):
        from nspire_converter.converter import Recipe
        from nspire_converter.converter_finalize import add_final_stages

        names = [
            "ancient-materials46",
            "ancient-gpu-masks46",
            "tez-cinders46",
            "nonu-fog46",
            "ancient-trajectories46",
            "neow-post46",
            "vakuu-cinders46",
            "neow-particles46",
            "ancient-fire46",
            "neow-fire46",
            "neow-static-field46",
            "card-afflictions46",
        ]
        for reduced in (False, True):
            recipe = Recipe("/game", "/work", "/private/runtime", "dotnet", "/noise", "-")
            add_final_stages(
                recipe, recipe.source_stages(), Path("/output"), ["eng", "zhs"], reduced
            )
            stages = {stage.name: stage for stage in recipe.stages}
            self.assertEqual(stages["ancient-scenes45"].module, "ancient_scene_compact")
            self.assertEqual(stages["ancient-lights46"].module, "ancient_lights_compact")
            self.assertEqual(
                "--bounded-color-hold" in stages["ancient-scenes45"].arguments, not reduced
            )
            for before, after in zip(names, names[1:]):
                stage = stages[after]
                self.assertEqual(
                    stage.arguments[stage.arguments.index("--input") + 1],
                    str(stages[before].outputs[0]),
                )
                self.assertEqual(stage.outputs[1], stage.outputs[0].with_suffix(".json"))
            self.assertEqual(stages["quality46"].arguments[0], str(stages[names[-1]].outputs[0]))
            cinder = stages["tez-cinders46"].arguments
            self.assertEqual(cinder[cinder.index("--runtime") + 1], "/private/runtime")
            self.assertEqual(
                cinder[cinder.index("--light-work") + 1], "/work/cache/ancient-scenes45"
            )
            fire = stages["ancient-fire46"].arguments
            self.assertEqual(fire[fire.index("--runtime") + 1], "/private/runtime")
            self.assertEqual(fire[fire.index("--light-work") + 1], "/work/cache/ancient-scenes45")
            neow = stages["neow-fire46"].arguments
            self.assertEqual(neow[neow.index("--runtime") + 1], "/private/runtime")
            self.assertEqual(neow[neow.index("--light-work") + 1], "/work/cache/ancient-scenes45")
            self.assertEqual("--no-animations" in neow, reduced)
            baked = stages["neow-static-field46"]
            self.assertEqual(baked.module, "neow_static_field")
            self.assertEqual(
                baked.arguments[baked.arguments.index("--work") + 1],
                "/work/cache/neow-static-field46",
            )
            self.assertEqual(stages["card-afflictions46"].module, "card_affliction")
            self.assertNotIn("--no-animations", stages["card-afflictions46"].arguments)
            self.assertNotIn("--sampled", stages["ancient-trajectories46"].arguments)
            self.assertEqual("--no-animations" in stages[names[0]].arguments, reduced)
            self.assertEqual("--no-animations" in stages["neow-post46"].arguments, reduced)
            post = stages["neow-post46"].arguments
            self.assertEqual(post[post.index("--noise-header") + 1], "/noise")
        recipe = Recipe("/game", "/work", "/private/runtime", "dotnet", "/noise", "-")
        recipe.trajectory_samples = "/verified/source-samples"
        add_final_stages(recipe, recipe.source_stages(), Path("/output"), ["eng"], False)
        args = next(s for s in recipe.stages if s.name == "ancient-trajectories46").arguments
        self.assertEqual(args[-2:], ("--sampled", "/verified/source-samples"))


if __name__ == "__main__":
    unittest.main()
