"""The parallel cache must be readable by the unchanged serial converter."""

import unittest
from concurrent.futures import ThreadPoolExecutor
from copy import deepcopy
from pathlib import Path
from tempfile import TemporaryDirectory
from unittest.mock import patch

from nspire_converter import prefetch_world_clips as prefetch
from nspire_converter.build_world import KAISER_LAYOUT, ConversionCache, job_state
from nspire_converter.prefetch_world_clips import PrefetchCache, unique_atomic_bytes
from nspire_converter.world_enemy_assets import SpineSampler, manifest
from nspire_converter.world_ui_profile import ENEMY_CLIP_ROWS


class PrefetchProtocol(unittest.TestCase):
    def test_shared_track_groups_preserve_other_arm_and_sampling_inputs(self):
        model = next(model for model in manifest()["models"] if model["model"] == "Rocket")
        jobs = [job for job in ENEMY_CLIP_ROWS if job["model"] == "Rocket"]
        first = next(job for job in jobs if job["clip"] == "right/attack")
        same = [
            job for job in jobs if job["clip"] == first["clip"] and job["move"] == first["move"]
        ]
        key = prefetch.clip_group_key(first, model, KAISER_LAYOUT)
        peers = [job for job in same if prefetch.clip_group_key(job, model, KAISER_LAYOUT) == key]
        self.assertEqual(len(peers), 6)
        self.assertEqual({job["visual_state"]["facts"]["kaiser_left_dead"] for job in peers}, {0})
        for field, value in (("frames", first["frames"] + 1), ("duration", first["duration"] + 1)):
            self.assertNotEqual(
                key, prefetch.clip_group_key(dict(first, **{field: value}), model, KAISER_LAYOUT)
            )
        self.assertNotEqual(
            key, prefetch.clip_group_key(first, model, dict(KAISER_LAYOUT, scale=1))
        )
        changed_model = dict(model, scene_controls=[{"node": "new fact-dependent driver"}])
        self.assertNotEqual(key, prefetch.clip_group_key(first, changed_model, KAISER_LAYOUT))
        changed = deepcopy(first)
        changed["visual_state"].pop("track")
        self.assertEqual(changed["key"], prefetch.clip_group_key(changed, model, KAISER_LAYOUT))

    def test_group_reuses_cached_peer_with_each_original_job_identity(self):
        model = next(model for model in manifest()["models"] if model["model"] == "Rocket")
        jobs = [
            (number, job, KAISER_LAYOUT)
            for number, job in enumerate(ENEMY_CLIP_ROWS)
            if job["model"] == "Rocket"
        ]
        first = next(item for item in jobs if item[1]["clip"] == "right/attack")
        key = prefetch.clip_group_key(first[1], model, KAISER_LAYOUT)
        peers = [item for item in jobs if prefetch.clip_group_key(item[1], model, item[2]) == key]
        with TemporaryDirectory() as root:
            cache = PrefetchCache(Path(root), {"schema": 2, "input": "fixed"})
            scope = {"model": "pinned", "pipeline": "enemy-batched-rgba-1"}
            source = peers[0]
            settings = dict(job=source[1], visual_state=job_state(source[1], source[2]))
            metadata = dict(frames=source[1]["frames"], visual_state=settings["visual_state"])
            payload = b"exact encoded original frames"
            cache.put("clip", settings, metadata, payload, scope=scope)
            worker = cache, None, None, {"Rocket": model}, {"Rocket": scope}, {}
            with (
                patch.object(prefetch, "_worker", worker),
                patch.object(
                    prefetch,
                    "prefetch_job",
                    return_value=(source[0], source[1]["key"], "reused", len(payload)),
                ) as bake,
            ):
                results = prefetch.prefetch_group((source, peers[1:]))
                bake.assert_called_once_with(source)
            self.assertEqual(len(results), 5)
            for _, job, layout in peers:
                own = dict(job=job, visual_state=job_state(job, layout))
                self.assertEqual(
                    cache.get("clip", own, scope=scope),
                    (dict(metadata, visual_state=own["visual_state"]), payload),
                )
            changed = deepcopy(metadata)
            changed["frames"] += 1
            own = dict(job=peers[1][1], visual_state=job_state(peers[1][1], KAISER_LAYOUT))
            cache.put("clip", own, changed, payload, scope=scope)
            with (
                patch.object(prefetch, "_worker", worker),
                patch.object(prefetch, "prefetch_job", return_value=results[0]),
            ):
                with self.assertRaisesRegex(ValueError, "producers disagree"):
                    prefetch.prefetch_group((source, peers[1:]))

    def test_supplied_runtime_selects_its_own_adapter_and_pixel_cache(self):
        with TemporaryDirectory() as root, patch.object(SpineSampler, "_prepare"):
            runtime = Path(root) / "sampler.dll"
            identities = []
            for binary in (b"synthetic runtime A", b"synthetic runtime B"):
                runtime.write_bytes(binary)
                sampler = SpineSampler(object(), runtime=runtime, work_dir=Path(root) / "adapter")
                identities.append(sampler.runtime_identity)
                sampler.close()
            self.assertNotEqual(*identities)

    def test_serial_and_prefetch_protocols_match(self):
        with TemporaryDirectory() as root:
            cache = PrefetchCache(Path(root), {"schema": 2, "input": "fixed"})
            serial = ConversionCache(Path(root), {"schema": 2, "input": "fixed"})
            metadata, payload = {"frames": 3, "size": [4, 5]}, b"original frame data"
            scope, settings = {"model": "pinned", "pipeline": "enemy-rgba-1"}, {"clip": 7}
            with ThreadPoolExecutor(max_workers=4) as pool:
                list(
                    pool.map(
                        lambda index: cache.put(
                            "clip",
                            settings,
                            metadata,
                            payload,
                            scope=scope,
                            reused_from=dict(
                                pipeline="equivalent-kaiser-tracks", key=f"{index:064x}"
                            ),
                        ),
                        range(20),
                    )
                )
            self.assertEqual(serial.get("clip", settings, scope=scope), (metadata, payload))
            self.assertFalse(list(Path(root).rglob("*.tmp")))
            (cache.directory / "clip" / (cache.key("clip", settings, scope) + ".bin")).write_bytes(
                b"broken"
            )
            with self.assertRaisesRegex(ValueError, "incomplete or changed"):
                serial.get("clip", settings, scope=scope)

    def test_atomic_file_is_one_complete_producer(self):
        with TemporaryDirectory() as root:
            target = Path(root) / "result.bin"
            values = [bytes([i]) * (10000 + i) for i in range(20)]
            with ThreadPoolExecutor(max_workers=4) as pool:
                list(pool.map(lambda value: unique_atomic_bytes(target, value), values))
            self.assertIn(target.read_bytes(), values)
            self.assertEqual(list(Path(root).iterdir()), [target])


if __name__ == "__main__":
    unittest.main()
