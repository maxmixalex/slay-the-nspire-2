"""Compiled recipes use bundled inventories rather than absent Python sources."""

import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

from nspire_converter import (
    actor_animation,
    ancient_scene,
    assemble_scene_resources,
    bestiary_kaiser_assets,
    character_assets,
    character_death_assets,
    layered_animation_assets,
    patch_phobia_assets,
    rebake_enemy,
    vantom_assets,
)
from nspire_converter.inventory import inventory


class CompiledRecipeInputs(unittest.TestCase):
    def test_cache_keys_remain_available_without_source_files(self):
        inventory()
        read = Path.read_bytes

        def read_data(path):
            if path.suffix in (".py", ".cs"):
                raise AssertionError(f"Recipe attempted to read compiled-away source {path}")
            return read(path)

        with tempfile.TemporaryDirectory() as directory:
            runtime = Path(directory) / "runtime.identity"
            runtime.write_bytes(b"sampler identity fixture")
            pack = SimpleNamespace(entries={})
            with patch.object(Path, "read_bytes", read_data):
                results = [
                    actor_animation.input_identity(pack, runtime),
                    ancient_scene.signature(pack, runtime, {}),
                    assemble_scene_resources.identity(),
                    bestiary_kaiser_assets.identity(runtime),
                    character_assets.converter_identity(),
                    character_death_assets.identity(runtime),
                    layered_animation_assets.signature(pack, runtime, {}),
                    patch_phobia_assets.identity(runtime),
                    rebake_enemy.cache_identity(pack, runtime),
                    vantom_assets.source_identity(pack, runtime),
                ]
            self.assertTrue(all(len(bytes.fromhex(value)) == 32 for value in results))


if __name__ == "__main__":
    unittest.main()
