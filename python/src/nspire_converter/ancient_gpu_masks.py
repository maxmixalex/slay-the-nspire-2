#!/usr/bin/env python3
"""Source-bound native masks for stationary Ancient GPU light layers.

Only the constant texture/transform part is projected. Lifetime, preprocessing,
source color/alpha envelopes, node order and the separate particle families
remain runtime behavior. No whole room or animation frame is rasterized here.
"""

from __future__ import annotations

import json
import struct
from pathlib import Path

import numpy as np

from nspire_converter import content_tiles as t
from nspire_converter.world_scene_assets import SceneLoader, _node_matrix, _property, _sections

SCENES = {
    "nonupeipe": "29f03f3684d0c441a2d29e3078f21715519414d24cea77d9fffddd47a56ecc3e",
    "vakuu": "5aa121b200b8a94b432f3254307c74b5208eeb2eaefacefa7b59d764432cb146",
}
LIGHT = "images/vfx/monsters/queen/false_queen_light.png"
DOT = "images/vfx/tiny_glow_dot.png"
ORDER = (
    ("nonupeipe", "light_overlay7"),
    ("nonupeipe", "light_overlay8"),
    ("nonupeipe", "light_overlay9"),
    ("nonupeipe", "light_overlay10"),
    ("nonupeipe", "light_overlay11"),
    ("vakuu", "light_overlay3"),
    ("vakuu", "light_overlay4"),
    ("vakuu", "light_overlay5"),
)
MAGIC = b"AGM1"
VERSION = 1
RAMP_SHA256 = "9f7ae2544b2332dffc049875b2147089aead01c91c0e6f7ceb0c9cba72323b48"
# Parameter image:16-byte header, eight12-byte native mask rows, dot image ID,
# then source-owned bounded padding. Every mask row has its explicit leaf ID.
PARAMETER_BYTES = 1536


def source_documents(pack):
    result = {}
    for scene, digest in SCENES.items():
        name = f"scenes/events/background_scenes/{scene}.tscn"
        data = pack.read(name)
        t.require(t.sha256(data) == digest, "Ancient GPU scene source differs: " + scene)
        result[scene] = {
            a["name"]: p for k, a, p in _sections(data.decode("utf8"), name) if k == "node"
        }
    return result


def bilinear_alpha(alpha, u, v):
    h, w = alpha.shape
    x = u * w - 0.5
    y = v * h - 0.5
    ix = np.floor(x).astype(np.int32)
    iy = np.floor(y).astype(np.int32)
    fx = x - ix
    fy = y - iy
    l = np.clip(ix, 0, w - 1)
    r = np.clip(ix + 1, 0, w - 1)
    top = np.clip(iy, 0, h - 1)
    bottom = np.clip(iy + 1, 0, h - 1)
    return (alpha[top, l] * (1 - fx) + alpha[top, r] * fx) * (1 - fy) + (
        alpha[bottom, l] * (1 - fx) + alpha[bottom, r] * fx
    ) * fy


def projected_alpha(alpha, props):
    # Exact accepted source AncientContainer projection at the320x240LCD.
    s = 3195634 / 16777216
    camera = np.array(
        [[s, 0, -114518200509 / 16777216 / 256], [0, s, 89989748010 / 16777216 / 256], [0, 0, 1]],
        dtype=float,
    )
    transform = camera @ _node_matrix(props) @ np.diag([0.5, 0.5, 1.0])
    y, x = np.mgrid[:240, :320]
    screen = np.stack((x.ravel() + 0.5, y.ravel() + 0.5, np.ones(320 * 240)))
    local = np.linalg.inv(transform) @ screen
    h, w = alpha.shape
    u = local[0] / w + 0.5
    v = local[1] / h + 0.5
    inside = (u >= 0) & (u < 1) & (v >= 0) & (v < 1)
    values = bilinear_alpha(alpha, u[inside], v[inside])
    output = np.zeros((240, 320), dtype="u1")
    output.ravel()[inside] = np.floor(values + 0.5).clip(0, 255).astype("u1")
    ys, xs = np.where(output)
    t.require(len(xs) > 0, "Authored light projected to an empty mask")
    left, top, right, bottom = map(int, (xs.min(), ys.min(), xs.max() + 1, ys.max() + 1))
    crop = output[top:bottom, left:right]
    error = float(np.max(np.abs(output.ravel()[inside].astype(float) - values), initial=0))
    t.require(error <= 0.5000001, "Native alpha rounding exceeds half a source alpha unit")
    return (left, top, crop), error


def assets(pack):
    from nspire_converter.build_content import texture

    documents = source_documents(pack)
    light = np.asarray(texture(pack, LIGHT).convert("RGBA"), dtype="u1")
    t.require(
        light.shape == (1820, 1251, 4)
        and np.all(light[:, :, :3] == 255)
        and t.sha256(light.tobytes())
        == "bbc9313f804daed21af8cb414f5e161b38703e4823b6d12d3e5f14bab83dd732",
        "Source false-queen light mask changed",
    )
    dot = np.asarray(texture(pack, DOT).convert("RGBA"), dtype="u1")
    t.require(
        dot.shape == (24, 24, 4)
        and np.all(dot[:, :, :3] == 255)
        and t.sha256(dot.tobytes())
        == "ae5bc0e1f0c221416baa9f82aebff710cd8fed1d0b09ba9f4a845a8cd6b649b8",
        "Source tiny-glow mask changed",
    )
    # Both decoded source masks are pinned above, independently of filenames.
    rows = []
    bindings = []
    for scene, node in ORDER:
        props = documents[scene][node]
        t.require(
            _property(props, "amount") == 1
            and _property(props, "lifetime") == 10
            and _property(props, "preprocess") == 2
            and _property(props, "local_coords") is True,
            "Stationary source overlay timing differs",
        )
        (x, y, crop), error = projected_alpha(light[:, :, 3], props)
        h, w = crop.shape
        stride = (w + 1) & ~1
        raw = np.zeros((h, stride), dtype="u1")
        raw[:, :w] = crop
        rows.append((stride // 2, h, 0, raw.tobytes()))
        bindings.append(
            dict(
                scene=scene,
                node=node,
                x=x,
                y=y,
                w=int(w),
                h=int(h),
                stride=stride,
                maximum_source_alpha_rounding=error,
            )
        )
    rows.append((24, 24, 1, b"\xff" * (24 * 24 * 2) + dot[:, :, 3].tobytes()))
    from nspire_converter.world_room_shaders import generated_texture

    loader = SceneLoader(pack)
    material = _property(documents["vakuu"]["light_overlay3"], "process_material")
    _, props = loader.resource(material)
    light_ramp = np.rint(generated_texture(loader, _property(props, "color_ramp"))[0] * 255).astype(
        "u1"
    )
    material = _property(documents["vakuu"]["sparkles"], "process_material")
    _, props = loader.resource(material)
    sparkle_ramp = np.rint(
        generated_texture(loader, _property(props, "color_ramp"))[0] * 255
    ).astype("u1")
    t.require(
        light_ramp.shape == (256, 4)
        and np.all(light_ramp[:, :3] == 255)
        and sparkle_ramp.shape == (256, 4),
        "Source gradient format differs",
    )
    ramps = light_ramp[:, 3].tobytes() + sparkle_ramp.tobytes()
    return (
        rows,
        bindings,
        ramps,
        dict(
            scene_sha256=SCENES,
            light_rgba_sha256=t.sha256(light.tobytes()),
            dot_rgba_sha256=t.sha256(dot.tobytes()),
            no_dynamic_frame_rasterization=True,
            source_sampler="Full original alpha, bilinear at authored native LCD pixel centers",
            alpha_precision="Nearest alpha8; maximum half of one source alpha unit",
        ),
    )


def table(first, bindings, ramps):
    t.require(
        len(bindings) == 8 and 0 <= first <= 65526,
        "Ancient GPU mask image references exceed catalog",
    )
    raw = bytearray(struct.pack("<4s6H", MAGIC, VERSION, 8, first, first + 8, 320, 240))
    for index, row in enumerate(bindings):
        raw.extend(
            struct.pack("<6H", first + index, row["x"], row["y"], row["w"], row["h"], row["stride"])
        )
    t.require(len(ramps) == 1280, "Source ramp data size differs")
    raw.extend(bytes(128 - len(raw)))
    raw.extend(ramps)
    return bytes(raw) + bytes(PARAMETER_BYTES - len(raw))


def validate_parameter(raw, image_count):
    t.require(len(raw) == PARAMETER_BYTES, "Ancient GPU parameter length differs")
    magic, version, count, first, dot, w, h = struct.unpack_from("<4s6H", raw)
    t.require(
        (magic, version, count, w, h) == (MAGIC, VERSION, 8, 320, 240)
        and dot == first + 8
        and dot < image_count,
        "Ancient GPU parameter header or references differ",
    )
    rows = []
    for index in range(8):
        image, x, y, w, h, stride = struct.unpack_from("<6H", raw, 16 + index * 12)
        t.require(
            image == first + index
            and 0 < w <= 320
            and 0 < h <= 240
            and x + w <= 320
            and y + h <= 240
            and stride == ((w + 1) & ~1),
            "Ancient GPU native mask bounds differ",
        )
        rows.append(dict(image=image, x=x, y=y, w=w, h=h, stride=stride))
    t.require(
        not any(raw[112:128]) and not any(raw[1408:]), "Ancient GPU parameter padding differs"
    )
    t.require(t.sha256(raw[128:1408]) == RAMP_SHA256, "Ancient GPU source gradient bytes differ")
    return dict(images=tuple(range(first, first + 9)), dot=dot, rows=rows)


BASE_AUX, AUXILIARY, BASE_IMAGES, IMAGES = 7220, 7221, 59700, 59710


def validate(source, image):
    t.require(
        (len(source.aux), len(source.images)) == (AUXILIARY, IMAGES)
        and source.aux[BASE_AUX] == IMAGES - 1,
        "Ancient GPU masks require their exact typed catalog extension",
    )
    w, h, fmt, raw = image(IMAGES - 1)
    t.require((w, h, fmt) == (256, 3, 0), "Ancient GPU parameter view differs")
    result = validate_parameter(raw, len(source.images))
    t.require(
        result["images"] == tuple(range(BASE_IMAGES, BASE_IMAGES + 9)),
        "Ancient GPU leaves overlap earlier images",
    )
    for row in result["rows"]:
        w, h, fmt, raw = image(row["image"])
        t.require(
            (w, h, fmt) == (row["stride"] // 2, row["h"], 0) and len(raw) == row["stride"] * h,
            "Ancient GPU alpha mask dimensions differ",
        )
        if row["stride"] != row["w"]:
            t.require(
                not any(raw[row["w"] :: row["stride"]]), "Ancient GPU mask row padding differs"
            )
    w, h, fmt, raw = image(result["dot"])
    t.require(
        (w, h, fmt) == (24, 24, 1)
        and len(raw) == 24 * 24 * 3
        and raw[: 24 * 24 * 2] == b"\xff" * (24 * 24 * 2),
        "Ancient GPU tiny-glow source mask differs",
    )
    result["table"] = IMAGES - 1
    return result


def append(data, pck, work):
    from nspire_converter.coalesce_tile_resources import view
    from nspire_converter.no_animations import mark, reduced
    from nspire_converter.patch_frontend_assets import patch_pack
    from nspire_converter.pck import Pack
    from nspire_converter.split_content import LANGUAGES
    from nspire_converter.tanx_scene import group_images

    source = view(data)
    t.require(
        source.version == 46
        and len(source.aux) == BASE_AUX
        and len(source.images) == BASE_IMAGES
        and len(source.clips) == 1783,
        "Expected exact Neow/Tez and material extension before Ancient GPU masks",
    )
    work = Path(work)
    cache = work / "native-masks"
    cache.mkdir(parents=True, exist_ok=True)
    pack = Pack(pck)
    try:
        rows, bindings, ramps, proof = assets(pack)
    finally:
        pack.close()
    rows.append((256, 3, 0, table(BASE_IMAGES, bindings, ramps)))
    records = []
    for i, (w, h, fmt, raw) in enumerate(rows):
        name = f"{i:02}-{t.sha256(raw)}.bin"
        (cache / name).write_bytes(raw)
        records.append(
            dict(
                file=name,
                width=w,
                height=h,
                format=fmt,
                bytes=len(raw),
                sha256=t.sha256(raw),
                raw_sha256=t.sha256(raw),
            )
        )
    art = group_images(records, cache)
    manifest = dict(
        images=records,
        auxiliary_image_ids=[IMAGES - 1],
        localized_text={code: {} for code in LANGUAGES},
    )
    # Use the established append writer, preserving all old payload bytes and
    # image identities. It has no private original-pack substitution path.
    result, append_proof = patch_pack(
        data,
        manifest,
        cache,
        base_aux=BASE_AUX,
        auxiliary=AUXILIARY,
        input_version=46,
        version=46,
        texts=(),
        base_clips=len(source.clips),
        base_strings=len(source.strings),
        appended_art=art,
    )
    if reduced(data):
        result = mark(result)
    target = view(result)
    image = target.image_raw
    typed = validate(target, image)
    for i, row in enumerate(rows):
        t.require(image(BASE_IMAGES + i) == row, "Ancient GPU source mask changed in append")
    t.require(
        target.clips == source.clips
        and target.cards == source.cards
        and target.aux[:BASE_AUX] == source.aux,
        "Ancient GPU append changed prior metadata",
    )
    proof.update(
        complete=True,
        input_sha256=t.sha256(data),
        output_sha256=t.sha256(result),
        input_bytes=len(data),
        output_bytes=len(result),
        added_bytes=len(result) - len(data),
        source_bindings=bindings,
        grouping=art["report"],
        profile_preserved=reduced(result) == reduced(data),
        append=append_proof,
        typed=typed,
        decoded_mask_bytes=sum(len(x[3]) for x in rows),
        framebuffer_growth=0,
        particle_pool_growth=0,
        original_random_history_recovered=False,
        remaining=["turbulent speck families", "Vakuu cinder emitter", "Nonupeipe fog family"],
    )
    return result, proof


def main():
    import argparse

    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("input", "output", "pck", "work"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    data, proof = append(args.input.read_bytes(), args.pck, args.work)
    t.atomic_bytes(args.output, data)
    t.atomic_bytes(args.output.with_suffix(".json"), (json.dumps(proof, indent=2) + "\n").encode())
    print(
        json.dumps(
            {
                k: proof[k]
                for k in ("complete", "input_bytes", "output_bytes", "added_bytes", "output_sha256")
            }
        )
    )


if __name__ == "__main__":
    main()
