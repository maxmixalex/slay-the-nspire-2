#!/usr/bin/env python3
"""The visible Neow SCREEN_TEXTURE water post, after every source scene layer.

One bounded typed image carries the native source mask, the explicit procedural
noise representation, source projection and mip interpolation table. It never
contains a captured game screen or a replacement Neow animation family.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import struct
import subprocess
from pathlib import Path

import numpy as np

from nspire_converter import content_tiles as t
from nspire_converter.ancient_materials import bilinear
from nspire_converter.neow_water_post import _NEOW_NOISE_CALLER, nonnormalized_r8, source_values

BASE_AUX, AUXILIARY, BASE_IMAGES, IMAGES = 7224, 7225, 59729, 59730
SCENE = "scenes/events/background_scenes/neow.tscn"
SHADER = "shaders/vfx/vfx_water_reflection_post.gdshader"
SOURCES = {
    SCENE: "39274c6148a90e63d91322e741bc000b1e64e172442bcfa8fedc1a43cdb01389",
    SHADER: "e951383f6596625cc45ad5d353a4fad2f6a5c123b197f32062ec9d8e2f45ccb1",
}
MASK = "images/rooms/hive/the_insatiable/sand_pillar_base.png"
MASK_RGBA = "b61daf64d470303bdea9dcaa71bdb055bcd70f499e8825057889c30c5978370a"
GEOMETRY = "images/vfx/miasma_cloud.png"
GEOMETRY_RGBA = "2d56e2e4640003af6e07489b83232de91f1acda05248c6077a9c7a4d9dfb7d41"
NOISE_SHA = "dd9c57fee4af82a9344fc35fa4044734d7e534d9ca9cfc2219237738dd9b88ff"
RAW_SHA = (
    "83d65067db98ba4c2a5306dde2e3a2175f06f6f5e0862e025a9b8f52a8d795fb",
    "30109de6f718537c33ddffa251a38a55b9bf64fe8f18c08896aeea7efb9e1ddc",
)
# Exact renderer projection of the source Sprite2D, whose texture supplies only
# geometry. Its shader replaces COLOR and never samples the sprite's pixels.
MATRIX = (1.4714164309288071, 0.0, 172.9540542625691, 0.0, 0.995610832940437, 203.23658122955152)
W, H = 136, 116
UV0 = (0.5 + (0.5 - MATRIX[2]) / (MATRIX[0] * W), 0.5 + (0.5 - MATRIX[5]) / (MATRIX[4] * H))
STEP = (1 / (MATRIX[0] * W), 1 / (MATRIX[4] * H))
UV_WORDS = tuple(int(np.floor(v * 16777216 + 0.5)) for v in (*UV0, *STEP))
# Whole2x2 derivative quads. The drawable subregion is checked by source UV.
LEFT, TOP, RIGHT, BOTTOM = 72, 144, 274, 240
NOISE_AT, MASK_AT, LOD_AT = 128, 16512, 26512
STATIC_AT = NOISE_AT
FULL_HEIGHT, STATIC_HEIGHT = 56, 100
STATIC_COUNT = (RIGHT - LEFT) * (BOTTOM - TOP)


def seamless(library):
    """Original non-normalized OpenSimplex2S R8 + Godot seamless skirt."""
    width = height = 512
    sw = sh = int(512 * 0.293)
    hw = hh = 256
    source = nonnormalized_r8(source_values(library, width + sw, height + sh))

    def read(x, y, ax=False, ay=False):
        return int(
            source[
                (y + hh) % (height if ay else height + sh), (x + hw) % (width if ax else width + sw)
            ]
        )

    def alpha(i, size):
        v = np.clip(
            np.float32((np.float32(i) / np.float32(size) - np.float32(0.1)) / np.float32(0.8)), 0, 1
        )
        return int(np.float32(255) * (np.float32(1) - v * v * (np.float32(3) - np.float32(2) * v)))

    def blend(a, b, q):
        return ((q + 1) * b + (256 - q) * a) >> 8

    result = np.asarray(
        [[read(x, y, True, True) for x in range(width)] for y in range(height)], dtype="u1"
    )
    for x in range(hw, hw + sw):
        q = alpha(x - hw, sw)
        for y in range(height):
            if not hh <= y < hh + sh:
                result[y, x] = blend(int(result[y, x]), read(x, y, False, True), q)
    for y in range(hh, hh + sh):
        q = alpha(y - hh, sh)
        for x in range(width):
            if not hw <= x < hw + sw:
                result[y, x] = blend(int(result[y, x]), read(x, y, True, False), q)
    for y in range(hh, hh + sh):
        for x in range(hw, hw + sw):
            a, b = alpha(x - hw, sw), alpha(y - hh, sh)
            result[y, x] = blend(
                blend(read(x, y, True, True), read(x, y, False, True), a),
                blend(read(x, y, True, False), read(x, y), a),
                b,
            )
    t.require(t.sha256(result.tobytes()) == NOISE_SHA, "Original Neow procedural noise differs")
    return result


def helper(header, work):
    path = os.environ.get("NSPIRE_FASTNOISE_LIBRARY")
    if path:
        return Path(path)
    # Source-mode fallback; frozen distributions provide the genuine native
    # export and require no compiler on the user's machine.
    path = Path(work) / "neow-noise.so"
    cpp = Path(work) / "neow-noise.cpp"
    cpp.write_text(_NEOW_NOISE_CALLER)
    subprocess.run(
        [
            "clang++",
            "-O2",
            "-ffp-contract=off",
            "-fwrapv",
            "-shared",
            "-fPIC",
            "-I" + str(Path(header).parent),
            str(cpp),
            "-o",
            str(path),
        ],
        check=True,
    )
    return path


def lod_table():
    values = [
        0 if i <= 256 else min(256, int(np.floor(np.log2(np.sqrt(i / 256)) * 256 + 0.5)))
        for i in range(1025)
    ]
    return struct.pack("<1025H", *values)


def parameter_bytes(noise, mask, static_values=None):
    static = static_values is not None
    h = STATIC_HEIGHT if static else FULL_HEIGHT
    t.require(
        len(noise) == 128 * 128 and len(mask) == 100 * 100,
        "Neow post exact input dimensions differ",
    )
    mask_at = STATIC_AT + STATIC_COUNT * 2 if static else MASK_AT
    lod_at = mask_at + 10000 if static else LOD_AT
    raw = bytearray(256 * h * 2)
    struct.pack_into(
        "<16H",
        raw,
        0,
        0x4E57,
        1,
        BASE_IMAGES,
        BASE_AUX,
        256,
        h,
        128,
        100,
        NOISE_AT,
        mask_at,
        lod_at,
        1000,
        15,
        int(static),
        STATIC_AT,
        STATIC_COUNT,
    )
    struct.pack_into("<4i4H", raw, 32, *UV_WORDS, LEFT, TOP, RIGHT, BOTTOM)
    raw[64:96] = bytes.fromhex(NOISE_SHA)
    raw[96:128] = hashlib.sha256(static_values if static else noise).digest()
    if not static:
        raw[NOISE_AT : NOISE_AT + len(noise)] = noise
    raw[mask_at : mask_at + len(mask)] = mask
    raw[lod_at : lod_at + 2050] = lod_table()
    if static:
        t.require(
            len(static_values) == STATIC_COUNT * 2, "Neow exact clock0 field cardinality differs"
        )
        raw[STATIC_AT : STATIC_AT + len(static_values)] = static_values
    return bytes(raw)


def source_image(pack, header, work, no_animations=False):
    from nspire_converter.build_content import texture
    from nspire_converter.world_scene_assets import SceneLoader, _property, _sections, _vector

    for name, digest in SOURCES.items():
        t.require(t.sha256(pack.read(name)) == digest, "Neow post source changed: " + name)
    nodes = {
        a["name"]: p for k, a, p in _sections(pack.read(SCENE).decode("utf8"), SCENE) if k == "node"
    }
    t.require(
        all(
            _property(nodes[n], "visible", True) is False
            for n in ("water effect2", "water effect3")
        ),
        "Hidden water posts must stay hidden",
    )
    node = nodes["water effect"]
    loader = SceneLoader(pack)
    _, material = loader.resource(_property(node, "material"))
    t.require(
        tuple(_vector(_property(node, "position"))) == (1048, 957)
        and tuple(_vector(_property(node, "scale"))) == (7.725, 5.227),
        "Neow post source geometry changed",
    )
    t.require(
        _property(material, "shader_parameter/speed") == 1.0
        and _property(material, "shader_parameter/strength") == 0.015,
        "Neow post source clock/strength changed",
    )
    mask = np.asarray(texture(pack, MASK).convert("RGBA"), dtype="u1")
    geo = np.asarray(texture(pack, GEOMETRY).convert("RGBA"), dtype="u1")
    t.require(
        mask.shape == (100, 100, 4) and t.sha256(mask.tobytes()) == MASK_RGBA,
        "Neow post source mask changed",
    )
    t.require(
        geo.shape == (H, W, 4) and t.sha256(geo.tobytes()) == GEOMETRY_RGBA,
        "Neow post source Sprite2D geometry changed",
    )
    noise = seamless(helper(header, work))
    y, x = np.mgrid[:128, :128]
    small = np.floor(
        bilinear(noise.astype(float), (x + 0.5) / 128, (y + 0.5) / 128, True) + 0.5
    ).astype("u1")
    # Union of all bilinear breakpoints: the absolute difference of the two
    # bilinear fields takes its maximum at a corner of these rectangles.
    yy, xx = np.mgrid[:1024, :1024]
    u = xx / 1024
    v = yy / 1024
    error = np.abs(
        bilinear(noise.astype(float), u, v, True) - bilinear(small.astype(float), u, v, True)
    )
    maximum = float(error.max())
    t.require(maximum <= 2.0, "Neow projected noise representation exceeds its reviewed bound")
    static_values = None
    if no_animations:
        yy, xx = np.mgrid[TOP:BOTTOM, LEFT:RIGHT]
        values = bilinear(noise.astype(float), UV0[0] + xx * STEP[0], UV0[1] + yy * STEP[1], True)
        static_values = np.floor(values * 256 + 0.5).astype("<u2").tobytes()
    raw = parameter_bytes(small.tobytes(), mask[:, :, 3].tobytes(), static_values)
    t.require(
        t.sha256(raw) == RAW_SHA[int(no_animations)],
        "Neow source-bound post parameter pixels differ",
    )
    proof = dict(
        source_files=SOURCES,
        source_noise_sha256=NOISE_SHA,
        source_mask_rgba_sha256=MASK_RGBA,
        native_projection=MATRIX,
        source_draw_order="final visible command49 after body, CPU/GPU lights, fog, stars and vignette; before dialogue/HUD",
        source_mipmap_filter="linear_mipmap; complete screen snapshot plus mip1",
        noise_policy="128x128R8 source-filtered texture; exact original-noise clock0 field for reduced",
        maximum_continuous_noise_error=maximum,
        maximum_screen_shift_error_xy=[maximum * 0.015 * 320 / 255, maximum * 0.015 * 240 / 255],
        static=no_animations,
        original_global_epoch_known=False,
        raw_sha256=t.sha256(raw),
        raw_bytes=len(raw),
        cache_pages=(len(raw) + 4095) // 4096,
        transient_snapshot_bytes=192000,
        new_persistent_bytes=0,
        quality_cost_acceptance_pending=True,
    )
    return (256, STATIC_HEIGHT if no_animations else FULL_HEIGHT, 0, raw), proof


def validate(source, image, final=False):
    t.require(
        (len(source.aux), len(source.images)) == (AUXILIARY, IMAGES),
        "Neow post requires its complete exact catalog",
    )
    t.require(source.aux[BASE_AUX] == BASE_IMAGES, "Neow post typed root differs")
    w, h, fmt, raw = image(BASE_IMAGES)
    t.require(
        (w, fmt) == (256, 0) and h in (FULL_HEIGHT, STATIC_HEIGHT), "Neow post word view differs"
    )
    static = h == STATIC_HEIGHT
    t.require(
        t.sha256(raw) == RAW_SHA[int(static)],
        "Neow post original mask/noise/parameter bytes differ",
    )
    values = raw[STATIC_AT : STATIC_AT + STATIC_COUNT * 2] if static else None
    mask_at = STATIC_AT + STATIC_COUNT * 2 if static else MASK_AT
    t.require(
        raw
        == parameter_bytes(
            bytes(16384) if static else raw[NOISE_AT : NOISE_AT + 16384],
            raw[mask_at : mask_at + 10000],
            values,
        ),
        "Neow post header/geometry/clock/LOD/padding changed",
    )
    if static:
        t.require(
            max(struct.unpack("<" + str(STATIC_COUNT) + "H", values)) <= 255 * 256,
            "Neow static source field escaped normalized R8",
        )
    if final:
        t.require(
            static == (struct.unpack_from("<I", source.data, 36)[0] == 0x3146414E),
            "Neow static post must match final NAF1 profile",
        )
    return (BASE_IMAGES,)


def require_reduced_source(source, image):
    t.require(
        image(BASE_IMAGES)[1] == STATIC_HEIGHT,
        "Reduced Neow post requires original-noise clock0 source field",
    )


def append(data, pack, header, work, no_animations=False):
    from nspire_converter.coalesce_tile_resources import view
    from nspire_converter.patch_frontend_assets import patch_pack
    from nspire_converter.split_content import LANGUAGES
    from nspire_converter.tanx_scene import group_images

    source = view(data)
    t.require(
        source.version == 46
        and (len(source.aux), len(source.images), len(source.clips))
        == (BASE_AUX, BASE_IMAGES, 1783),
        "Neow post predecessor is incomplete",
    )
    work = Path(work)
    work.mkdir(parents=True, exist_ok=True)
    row, proof = source_image(pack, header, work, no_animations)
    w, h, fmt, raw = row
    t.atomic_bytes(work / "post.bin", raw)
    spec = dict(
        width=w,
        height=h,
        format=fmt,
        file="post.bin",
        bytes=len(raw),
        sha256=t.sha256(raw),
        raw_sha256=t.sha256(raw),
    )
    art = group_images([spec], work)
    manifest = dict(
        images=[spec], auxiliary_image_ids=[BASE_IMAGES], localized_text={c: {} for c in LANGUAGES}
    )
    output, details = patch_pack(
        data,
        manifest,
        work,
        base_aux=BASE_AUX,
        auxiliary=AUXILIARY,
        input_version=46,
        version=46,
        texts=(),
        base_clips=1783,
        base_strings=len(source.strings),
        appended_art=art,
    )
    from nspire_converter.no_animations import mark, reduced

    if reduced(data):
        output = mark(output)
    target = view(output)
    validate(target, target.image_raw)
    t.require(
        target.image_raw(BASE_IMAGES) == row
        and target.clips == source.clips
        and target.cards == source.cards
        and target.aux[:BASE_AUX] == source.aux
        and reduced(output) == reduced(data),
        "Neow post changed prior content or source-derived words",
    )
    return output, dict(
        complete=True,
        input_sha256=t.sha256(data),
        output_sha256=t.sha256(output),
        added_bytes=len(output) - len(data),
        source=proof,
        grouping=art["report"],
        append=details,
    )


def main():
    from nspire_converter.pck import Pack

    p = argparse.ArgumentParser(description=__doc__)
    for name in ("input", "output", "pck", "noise-header", "work"):
        p.add_argument("--" + name, type=Path, required=True)
    p.add_argument("--no-animations", action="store_true")
    a = p.parse_args()
    for path in (a.input, a.output, a.pck, a.work):
        t.private_path(path, "Private Neow post source/output")
    t.require(a.input.resolve() != a.output.resolve(), "Preserve original Neow post input")
    pack = Pack(a.pck)
    try:
        output, proof = append(a.input.read_bytes(), pack, a.noise_header, a.work, a.no_animations)
    finally:
        pack.close()
    t.atomic_bytes(a.output, output)
    t.atomic_bytes(a.output.with_suffix(".json"), (json.dumps(proof, indent=2) + "\n").encode())
    print(json.dumps(dict(complete=True, added_bytes=proof["added_bytes"])), flush=True)


if __name__ == "__main__":
    main()
