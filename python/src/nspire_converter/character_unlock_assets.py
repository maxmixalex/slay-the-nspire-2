"""Append the original character lock textures without rewriting existing art.

Only these eight LCD samples are encoded. Existing animation, normal maps,
tile groups, image IDs, clip clocks and locale payloads are copied byte for byte.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from nspire_converter import content_tiles as t
from nspire_converter.build_content import prepared
from nspire_converter.coalesce_tile_resources import view
from nspire_converter.patch_feedback_assets import write_image
from nspire_converter.patch_frontend_assets import patch_pack
from nspire_converter.world_scene_assets import load_scene_texture

BASE_AUX, AUXILIARY = 7229, 7237
BASE_IMAGES, IMAGES = 60294, 60302
CHARACTERS = ("ironclad", "silent", "defect", "regent", "necrobinder", "random")
SCENE = "scenes/screens/char_select/char_select_button.tscn"
SPECS = tuple(
    (
        f"locked-{name}",
        f"images/packed/character_select/char_select_{name}_locked.png",
        (36, 40),
        (132, 196 if name == "random" else 195),
    )
    for name in CHARACTERS
) + (
    ("lock", "images/packed/character_select/char_select_lock3.png", (30, 40), (128, 128)),
    (
        "unlocked",
        "images/packed/character_select/char_select_lock3_unlocked.png",
        (30, 40),
        (128, 128),
    ),
)


def source_assets(pack, work):
    work = Path(work)
    work.mkdir(parents=True, exist_ok=True)
    scene = pack.read(SCENE).decode("utf-8")
    for value in (
        "custom_minimum_size = Vector2(100, 148)",
        "modulate = Color(1, 1, 1, 0.4)",
        "scale = Vector2(0.6, 0.6)",
        "offset_left = -20.0",
        "offset_top = -30.0",
        "offset_right = 50.0",
        "offset_bottom = 74.0",
    ):
        t.require(value in scene, "Original character lock scene changed: " + value)
    rows = []
    for key, path, size, source_size in SPECS:
        original = load_scene_texture(pack, path).convert("RGBA")
        t.require(
            original.size == source_size, "Original character lock texture size changed: " + path
        )
        # Match the existing 36x40 normal portraits, including their margins.
        # Keep source alpha here; the scene's 0.4 lock opacity is applied once
        # by the runtime. Both square locks remain square on a 30x40 canvas.
        image = prepared(original, size, "contain")
        row = write_image(work, key, image)
        row.update(
            key=key,
            source=path,
            source_size=list(original.size),
            canvas=list(size),
            fit="contain",
            auxiliary=BASE_AUX + len(rows),
            source_rgba_sha256=t.sha256(original.tobytes()),
            sampled_rgba_sha256=t.sha256(image.tobytes()),
        )
        image.save(work / (key + ".png"))
        rows.append(row)
    return rows


def validate(source, image):
    t.require(
        (len(source.aux), len(source.images)) == (AUXILIARY, IMAGES),
        "Character lock catalog is incomplete",
    )
    t.require(
        source.aux[BASE_AUX:] == list(range(BASE_IMAGES, IMAGES)),
        "Character lock image IDs changed",
    )
    for index, spec in enumerate(SPECS, BASE_IMAGES):
        width, height, fmt, raw = image(index)
        t.require(
            (width, height) == spec[2] and fmt == 1 and len(raw) == width * height * 3,
            "Character lock LCD image must preserve RGB565 and alpha8",
        )
    return set(range(BASE_IMAGES, IMAGES))


def append(data, pack, work):
    source = view(data)
    t.require(
        source.version == 46
        and (len(source.aux), len(source.images), len(source.clips))
        == (BASE_AUX, BASE_IMAGES, 1783),
        "Character locks require the complete preceding artwork catalog",
    )
    rows = source_assets(pack, work)
    from nspire_converter.split_content import LANGUAGES

    manifest = dict(images=rows, clips=[], localized_text={code: {} for code in LANGUAGES})
    output, proof = patch_pack(
        data,
        manifest,
        Path(work),
        base_aux=BASE_AUX,
        auxiliary=AUXILIARY,
        input_version=46,
        version=46,
        texts=(),
        base_clips=len(source.clips),
        base_strings=len(source.strings),
    )
    # The historical append writer clears the optional presentation word.
    output = bytearray(output)
    output[36:40] = data[36:40]
    output = bytes(output)
    target = view(output)
    validate(target, target.image_raw)
    proof.update(
        complete=True,
        output_sha256=t.sha256(output),
        source_scene=SCENE,
        original_source_images=rows,
        animation_profile_preserved=True,
        previous_artwork_reencoded=False,
        new_pixel_canvas="RGB565/alpha8",
    )
    return output, proof


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("input", "output", "pck", "work"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    from nspire_converter.pck import Pack

    source = t.private_path(args.input, "Private existing artwork")
    target = t.private_path(args.output, "Private character lock artwork")
    work = t.private_path(args.work, "Private source texture samples")
    t.require(source != target, "Preserve the input artwork")
    pack = Pack(t.private_path(args.pck, "Owner original PCK"))
    try:
        output, proof = append(source.read_bytes(), pack, work)
    finally:
        pack.close()
    t.atomic_bytes(target, output)
    t.atomic_bytes(target.with_suffix(".json"), (json.dumps(proof, indent=2) + "\n").encode())
    print(
        json.dumps(
            dict(complete=True, output_bytes=len(output), auxiliary=AUXILIARY, images=IMAGES)
        ),
        flush=True,
    )


if __name__ == "__main__":
    main()
