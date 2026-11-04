"""Source extraction for seven continuously evaluated card Afflictions.

Original textures stay private. Source shader fields retain their source
dimensions and bytes; static decoration is projected once to the largest LCD
card presentation. Particle children are explicit commands, never discarded by
the older static scene renderer. Infection and Ancient-card fire are unrelated
complete authored flipbooks and are not modified here.
"""

from __future__ import annotations

import hashlib
import json
import math
import struct
from pathlib import Path

import numpy as np
from PIL import Image, ImageChops

from nspire_converter import content_tiles as t
from nspire_converter.card_affliction_fields import encode, support
from nspire_converter.card_overlay_assets import AFFLICTIONS, CardRenderer
from nspire_converter.world_scene_assets import (
    _node_matrix,
    _property,
    _translate,
    _vector,
    load_scene_texture,
)

BASE_AUX, BASE_IMAGES = 7229, 60294
FIRST_AUX, SLOTS = 5202, 56
NATIVE = (156, 218)
VIEWPORT = (324, 454)
PAGE_BYTES = 131072
MATERIALS = {
    "bound_border_shader.gdshader": 1,
    "bound_main_shader.gdshader": 2,
    "entangled_main_shader.gdshader": 3,
    "galvanized_main_shader.gdshader": 4,
    "hexed_main_shader.gdshader": 5,
    "ringing_main_shader.gdshader": 6,
    "smog_main_shader.gdshader": 7,
    "entangled_leaf_shader.gdshader": 8,
}


def q20(value):
    a = np.asarray(value, dtype="f8") * 1048576
    t.require(
        np.all(np.isfinite(a)) and np.max(np.abs(a), initial=0) < 2147483647,
        "Card material fixed-point bound",
    )
    return np.where(a >= 0, np.floor(a + 0.5), np.ceil(a - 0.5)).astype("<i4")


class Bank:
    def __init__(self):
        self.fields = []
        self.lookup = {}
        self.curves = []
        self.curve_lookup = {}
        self.pictures = []
        self.picture_lookup = {}
        self.approximations = []

    def field(self, array, used=None):
        raw, proof = encode(np.ascontiguousarray(array, dtype="u1"), used)
        key = hashlib.sha256(raw).hexdigest()
        if key not in self.lookup:
            self.lookup[key] = len(self.fields)
            self.fields.append((raw, dict(proof, sha256=key)))
        return self.lookup[key]

    def curve(self, array):
        a = q20(array)
        t.require(
            a.ndim == 3 and a.shape[0] == 1 and 2 <= a.shape[1] <= 1024 and 1 <= a.shape[2] <= 4,
            "Card curve shape",
        )
        raw = a.tobytes()
        key = hashlib.sha256(raw).hexdigest()
        if key not in self.curve_lookup:
            self.curve_lookup[key] = len(self.curves)
            self.curves.append((raw, dict(width=a.shape[1], channels=a.shape[2], sha256=key)))
        return self.curve_lookup[key]

    def picture(self, image):
        from nspire_converter.sparse_image import encode as sparse_encode

        raw = sparse_encode(image)
        key = hashlib.sha256(struct.pack("<2H", *image.size) + raw).hexdigest()
        if key not in self.picture_lookup:
            self.picture_lookup[key] = len(self.pictures)
            self.pictures.append((image.width, image.height, 4, raw))
        return self.picture_lookup[key]

    def noise_proxy(self, array):
        from nspire_converter.world_room_shaders import sample_texture

        reduced = np.asarray(
            Image.fromarray(array).resize((256, 256), Image.Resampling.BILINEAR), "u1"
        )
        # A difference of bilinear surfaces attains its extrema at the union of
        # their cell corners. Check that full grid, including the repeat seam.
        positions = np.unique(
            np.r_[0.0, (np.arange(1024) + 0.5) / 1024, (np.arange(256) + 0.5) / 256, 1.0]
        ).astype("f4")
        a = array.astype("f4")[:, :, None] / 255
        b = reduced.astype("f4")[:, :, None] / 255
        maximum = 0.0
        for y in positions:
            uv = np.stack((positions, np.full_like(positions, y)), axis=-1)
            maximum = max(
                maximum,
                float(
                    np.max(
                        np.abs(
                            sample_texture(a, uv, repeat=True) - sample_texture(b, uv, repeat=True)
                        )
                    )
                ),
            )
        # The authored quad projects243.82223 pixels per unit u at the largest
        # 156x218 card overlay. Green modulation is<=1, so this also bounds motion.
        pixels = maximum * 0.018 * (422 * 1.2 * 156 / 324)
        t.require(pixels < 0.05, "Card driver reduction would move a visible edge")
        self.approximations.append(
            dict(
                field="entangled-blue-distortion",
                source=[1024, 1024],
                stored=[256, 256],
                tested_bilinear_cell_corners=len(positions) ** 2,
                maximum_normalized_error=maximum,
                largest_card_displacement_bound_pixels=pixels,
                visible_color_alpha_textures_unchanged=True,
            )
        )
        return self.field(reduced)


class Source(CardRenderer):
    def __init__(self, pack, bank):
        super().__init__(pack, *NATIVE, VIEWPORT, load_scene_texture, None, 0, 1)
        self.bank = bank
        self.markers = {}
        self.particles = []
        self.inputs = {}

    def mark(self, row):
        image = Image.new("RGBA", self.canvas)
        self.markers[id(image)] = row
        return image

    def remember(self, path):
        path = path.removeprefix("res://")
        self.inputs[path] = hashlib.sha256(self.pack.read(path)).hexdigest()

    def shader_layer(self, node, path, image, rect, matrix, tint, shader, material, blend, info):
        if not shader or "/afflictions/" not in shader:
            return super().shader_layer(
                node, path, image, rect, matrix, tint, shader, material, blend, info
            )
        matches = [value for key, value in MATERIALS.items() if shader.endswith(key)]
        t.require(len(matches) == 1, "Unreviewed continuous card shader: " + shader)
        kind = matches[0]
        self.remember(shader)
        original = np.asarray(image, "u1")
        repeat = _property(node.props, "texture_repeat", 0) == 2
        row = dict(
            type="material",
            kind=kind,
            node=path,
            shader=shader,
            repeat=repeat,
            rect=list(rect),
            matrix=matrix[:2].ravel().tolist(),
            screen=self.screen[:2].ravel().tolist(),
            tint=list(tint),
            fields=[-1] * 4,
            curves=[-1] * 2,
            parameters=[],
        )

        def value(name, default=None):
            return _property(material, "shader_parameter/" + name, default)

        def vector(name, size=2):
            return _vector(value(name), size)

        def texture(name):
            a = self._shader_texture(value(name))
            scaled = np.rint(a * 255)
            t.require(
                np.max(np.abs(a - scaled / 255)) < 1e-6,
                "Source byte texture unexpectedly has non-byte values",
            )
            return np.uint8(np.clip(scaled, 0, 255))

        def curve(name):
            return self.bank.curve(self._shader_texture(value(name)))

        if kind == 1:
            t.require(
                np.array_equal(original[:, :, 0], original[:, :, 1])
                and np.array_equal(original[:, :, 0], original[:, :, 2]),
                "Bound border no longer grayscale",
            )
            t.require(
                np.allclose(vector("main_st", 4), (1, 3, 0.25, 0), rtol=0, atol=1e-7)
                and np.allclose(vector("mask_erosion"), (0.4, 0.6), rtol=0, atol=1e-7),
                "Bound source parameters changed",
            )
            row["fields"][0] = self.bank.field(original[:, :, 0])
            row["fields"][2] = self.bank.field(texture("mask")[:, :, 3])
        elif kind == 2:
            t.require(
                np.allclose(vector("bright_st", 4), (0, 0, 0.5, 0), rtol=0, atol=1e-7)
                and value("bright_energy") == 1.5,
                "Bound brightness source changed",
            )
            row["fields"][0] = self.bank.field(original[:, :, (0, 3)], support(original[:, :, 3]))
            # v=0 uses first/last rows under repeat, or only the first under clamp.
            strip = original[[0, -1] if repeat else [0], :, 1]
            row["fields"][3] = self.bank.field(strip)
        elif kind == 3:
            t.require(
                np.allclose(vector("distortion_st", 4), (1, 1, 0, 0.175), rtol=0, atol=1e-7)
                and abs(float(value("distortion_intensity")) - 0.018) < 1e-7,
                "Vine source parameters changed",
            )
            reach = math.ceil(0.018 * original.shape[1]) + 2
            used = support(original[:, :, 3], reach, 1)
            if repeat:
                # Wrap support is included explicitly, even if the present source edge is
                # transparent. The packed field may therefore be sampled on either side.
                for shift in (-original.shape[1], original.shape[1]):
                    for y, x in np.argwhere(original[:, :, 3]):
                        lo = max(0, x + shift - reach)
                        hi = min(original.shape[1], x + shift + reach + 1)
                        if lo < hi:
                            used[max(0, y - 1) : y + 2, lo:hi] = True
            row["fields"][0] = self.bank.field(original[:, :, (0, 1, 3)], used)
            row["fields"][1] = self.bank.noise_proxy(original[:, :, 2])
            row["curves"][0] = curve("vine_lut")
        elif kind == 4:
            expected = {
                "blink_st": (0, 1, 0.67, 0),
                "blink_smoothstep": (0.2, 0.6),
                "distortion_st": (1, 1, -0.3, 0),
                "smoothstep_factors": (0.7, 0.3),
            }
            t.require(
                all(
                    np.allclose(vector(k, len(v)), v, rtol=0, atol=1e-7)
                    for k, v in expected.items()
                )
                and abs(value("distortion_offset") + 0.2) < 1e-7
                and abs(value("distortion_intensity") - 0.15) < 1e-7,
                "Galvanized source parameters changed",
            )
            row["fields"][0] = self.bank.field(original)
            row["curves"][0] = curve("lut")
        elif kind == 5:
            t.require(
                np.allclose(vector("noise_st", 4), (1, 1, -0.25, 0.1), rtol=0, atol=1e-7)
                and np.allclose(vector("noise_smoothstep"), (0.5, 0.2), rtol=0, atol=1e-7),
                "Hexed source parameters changed",
            )
            row["fields"][0] = self.bank.field(original[:, :, (0, 3)], support(original[:, :, 3]))
            row["fields"][1] = self.bank.field(original[:, :, 2])
            row["parameters"] = q20(
                np.concatenate(
                    [vector(k, 4) for k in ("main_color", "bright_color", "border_color")]
                )
            ).tolist()
        elif kind == 6:
            t.require(abs(value("animation_speed") - 0.4) < 1e-7, "Ringing source rate changed")
            row["fields"][0] = self.bank.field(original)
            row["curves"] = [curve("thin_ring_animation"), curve("ring_base_alpha")]
        elif kind == 7:
            t.require(
                np.array_equal(original[:, :, 0], original[:, :, 1])
                and np.array_equal(original[:, :, 0], original[:, :, 2]),
                "Smog no longer grayscale",
            )
            first, second = vector("main_st", 4), vector("secondary_st", 4)
            t.require(
                all(
                    abs(float(v) * 100 - round(float(v) * 100)) < 1e-5
                    for v in (first[2], second[2])
                ),
                "Smog rate outside exact rational grammar",
            )
            row["fields"][0] = self.bank.field(original[:, :, 0])
            row["fields"][2] = self.bank.field(texture("mask")[:, :, (0, 1)])
            row["curves"][0] = curve("lut")
            row["parameters"] = q20(
                [*first, *second, value("secondary_smoke_influence"), *vector("erosion_factors")]
            ).tolist() + [round(first[2] * 100), round(second[2] * 100)]
        else:
            raise ValueError("Leaf is a particle material, not a static canvas node")
        return self.mark(row), blend

    def visit(
        self, node, parent_matrix, parent_size, parent_tint, parent_z, path, clips=(), bones=None
    ):
        if node.kind != "GPUParticles2D":
            return super().visit(
                node, parent_matrix, parent_size, parent_tint, parent_z, path, clips, bones
            )
        if not _property(node.props, "visible", True):
            self.report("omitted", node, path, reason="authored_hidden")
            return
        props = node.props
        matrix = parent_matrix @ _node_matrix(props)
        tint = (
            parent_tint
            * _vector(_property(props, "modulate", (1, 1, 1, 1)), 4)
            * _vector(_property(props, "self_modulate", (1, 1, 1, 1)), 4)
        )
        z = int(_property(props, "z_index", 0)) + (
            parent_z if _property(props, "z_as_relative", True) else 0
        )
        row = dict(
            type="particles",
            node=path,
            matrix=matrix[:2].ravel().tolist(),
            tint=tint.tolist(),
            amount=int(_property(props, "amount", 8)),
            lifetime=float(_property(props, "lifetime", 1)),
            preprocess=float(_property(props, "preprocess", 0)),
            fixed_fps=int(_property(props, "fixed_fps", 30)),
        )
        self.particles.append((node, row))
        self.commands.append((z, self.mark(row), 0, clips))


def extract(pack):
    bank = Bank()
    families = []
    sources = {}
    particle_sources = []
    for kind in AFFLICTIONS:
        r = Source(pack, bank)
        path = "scenes/cards/overlays/afflictions/" + kind + ".tscn"
        r.remember(path)
        node = r.loader.instantiate(path)
        r.visit(node, _translate(162, 227), np.zeros(2), np.ones(4), 0, node.name)
        commands = []
        for _, image, blend, clips in sorted(r.commands, key=lambda x: x[0]):
            t.require(image is not None, "Unexpected shared scene in card overlay")
            row = r.markers.get(id(image))
            mask = None
            for clip in clips:
                mask = clip.copy() if mask is None else ImageChops.multiply(mask, clip)
            if row is not None:
                row = dict(
                    row,
                    blend=blend,
                    clip=-1 if mask is None else bank.field(np.asarray(mask, "u1")),
                )
            else:
                image = image.copy()
                if mask is not None:
                    image.putalpha(ImageChops.multiply(image.getchannel("A"), mask))
                array = np.asarray(image, "u1")
                if not array[:, :, 3].any():
                    continue
                row = dict(
                    type="static",
                    picture=bank.picture(image),
                    blend=blend,
                    bbox=list(image.getchannel("A").getbbox()),
                )
            commands.append(row)
        families.append(dict(kind=kind, commands=commands, hidden=r.diagnostics["omitted"]))
        sources.update(r.inputs)
        particle_sources.extend((kind, r, node, row) for node, row in r.particles)
    t.require(
        sum(r["type"] == "material" for f in families for r in f["commands"]) == 9,
        "Seven Affliction shader registry changed",
    )
    t.require(
        [sum(r["amount"] for r in f["commands"] if r["type"] == "particles") for f in families]
        == [10, 11, 8, 8, 0, 18, 0],
        "Visible Affliction particle registry changed",
    )
    return bank, families, sources, particle_sources


def source_inventory(pack, output):
    """Small source-only staging; no pack mutation or animation phase baking."""
    bank, families, sources, particles = extract(pack)
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    rows = []
    for i, (raw, proof) in enumerate(bank.fields):
        name = f"field-{i:03}.bin"
        t.atomic_bytes(output / name, raw)
        rows.append(dict(proof, file=name))
    curves = []
    for i, (raw, proof) in enumerate(bank.curves):
        name = f"curve-{i:03}.bin"
        t.atomic_bytes(output / name, raw)
        curves.append(dict(proof, file=name))
    pictures = []
    for i, (w, h, fmt, raw) in enumerate(bank.pictures):
        name = f"picture-{i:03}.bin"
        t.atomic_bytes(output / name, raw)
        pictures.append(
            dict(width=w, height=h, format=fmt, bytes=len(raw), sha256=t.sha256(raw), file=name)
        )
    receipt = dict(
        complete=False,
        source_fields_complete=True,
        particle_tracks_complete=False,
        implementation="continuous-source-fields",
        families=families,
        fields=rows,
        curves=curves,
        pictures=pictures,
        source_text_sha256=sources,
        total_field_bytes=sum(len(r) for r, _ in bank.fields),
        total_picture_bytes=sum(len(r[3]) for r in bank.pictures),
        max_particle_capacity=55,
        texture_resizing=False,
        new_persistent_bytes=0,
    )
    t.atomic_bytes(output / "source-fields.json", (json.dumps(receipt, indent=2) + "\n").encode())
    return receipt


COMMAND = struct.Struct("<4B2h4h2h13i4i6i4hIHH")
TRACK = struct.Struct("<IIHHI")
FRAME_BYTES = 38


def build_rows(pack, work):
    """Replace the old56 phase slots with fields, static art and a typed table."""
    from nspire_converter.card_affliction_particles import source_tracks, texture_fields

    bank, families, sources, particles = extract(pack)
    particle_tracks = {}
    for _family, renderer, node, row in particles:
        texture_fields(bank, renderer, node, row)
        particle_tracks[row["node"]] = source_tracks(
            bank, renderer, node, row, Path(work) / "helper"
        )
        for family in families:
            for command in family["commands"]:
                if command.get("node") == row["node"]:
                    command.update(row)
        sources.update(renderer.inputs)
    blob = bytearray()
    fields = []
    curves = []
    tracks = []

    def add(raw):
        blob.extend(bytes((-len(blob)) % 4))
        at = len(blob)
        blob.extend(raw)
        return at

    for raw, proof in bank.fields:
        fields.append((add(raw), len(raw)))
    for raw, proof in bank.curves:
        curves.append((add(raw), proof["width"], proof["channels"]))
    commands = []
    ranges = []
    for family in families:
        ranges.append((len(commands), len(family["commands"])))
        for row in family["commands"]:
            command = dict(row)
            if row["type"] == "particles":
                command["track_first"] = len(tracks)
                for track in particle_tracks[row["node"]]:
                    tracks.append(
                        (
                            track["lifetime_ms"],
                            track["delay_ms"],
                            track["fps"],
                            track["count"],
                            add(track["frames"]),
                        )
                    )
                command["track_count"] = len(tracks) - command["track_first"]
            commands.append(command)
    pages = (len(blob) + PAGE_BYTES - 1) // PAGE_BYTES
    t.require(
        1 + pages + len(bank.pictures) <= SLOTS, "Continuous cards exceed the reclaimed phase slots"
    )
    family_at = 64
    field_at = family_at + len(ranges) * 4
    curve_at = field_at + len(fields) * 8
    command_at = curve_at + len(curves) * 8
    track_at = command_at + len(commands) * COMMAND.size
    used = track_at + len(tracks) * TRACK.size
    header = struct.pack(
        "<4s14H8I",
        b"ACU1",
        1,
        SLOTS,
        pages,
        len(bank.pictures),
        len(fields),
        len(curves),
        len(commands),
        len(tracks),
        *NATIVE,
        COMMAND.size,
        FRAME_BYTES,
        0,
        0,
        len(blob),
        family_at,
        field_at,
        curve_at,
        command_at,
        track_at,
        used,
        0,
    )
    table = bytearray(header)
    table += b"".join(struct.pack("<2H", *r) for r in ranges)
    table += b"".join(struct.pack("<2I", *r) for r in fields)
    table += b"".join(struct.pack("<I2H", *r) for r in curves)
    for row in commands:
        kind = {"static": 0, "material": 1, "particles": 2}[row["type"]]
        inverse = [0] * 6
        box = row.get("bbox", [0, 0, *NATIVE])
        if kind == 1:
            matrix = np.vstack((np.asarray(row["matrix"]).reshape(2, 3), (0, 0, 1)))
            x, y, w, h = row["rect"]
            matrix = matrix @ np.array(((w, 0, x), (0, h, y), (0, 0, 1)))
            screen = np.vstack((np.asarray(row["screen"]).reshape(2, 3), (0, 0, 1)))
            projected = screen @ matrix
            inverse = q20(np.linalg.inv(projected) @ np.diag((*NATIVE, 1)))[:2].ravel().tolist()
            corners = projected @ np.array(((0, 1, 1, 0), (0, 0, 1, 1), (1, 1, 1, 1)))
            box = [
                *np.floor(corners[:2].min(axis=1)).astype(int),
                *np.ceil(corners[:2].max(axis=1)).astype(int),
            ]
        params = list(row.get("parameters", []))
        t.require(len(params) <= 13, "Card parameter extent")
        table += COMMAND.pack(
            kind,
            row.get("kind", 0) if kind else 0,
            row["blend"],
            bool(row.get("repeat", kind == 2)),
            row.get("clip", -1),
            row.get("picture", -1),
            *row.get("fields", [-1] * 4),
            *row.get("curves", [-1] * 2),
            *(params + [0] * (13 - len(params))),
            *q20(row.get("tint", [1] * 4)).tolist(),
            *inverse,
            *map(int, box),
            row.get("track_first", 0),
            row.get("track_count", 0),
            0,
        )
    table += b"".join(TRACK.pack(*r) for r in tracks)
    t.require(len(table) == used and COMMAND.size == 128, "Card table layout changed")
    table += bytes((-len(table)) % 512)
    rows = [(256, len(table) // 512, 0, bytes(table))]
    for at in range(0, len(blob), PAGE_BYTES):
        raw = bytes(blob[at : at + PAGE_BYTES])
        rows.append((320, 205, 0, raw + bytes(131200 - len(raw))))
    rows += bank.pictures
    rows += [(1, 1, 4, b"\0\0")] * (SLOTS - len(rows))
    t.require(
        all(w <= 320 and h <= 240 and len(raw) <= 256 * 1024 for w, h, fmt, raw in rows),
        "Card row decoder bound",
    )
    return rows, dict(
        source_files=sources,
        field_bytes=sum(len(raw) for raw, _ in bank.fields),
        curve_bytes=sum(len(raw) for raw, _ in bank.curves),
        particle_bytes=sum(
            len(track["frames"]) for group in particle_tracks.values() for track in group
        ),
        static_bytes=sum(len(row[3]) for row in bank.pictures),
        table_bytes=len(table),
        bank_bytes=len(blob),
        pages=pages,
        static_images=len(bank.pictures),
        fields=len(fields),
        curves=len(curves),
        commands=len(commands),
        particles=len(tracks),
        families=families,
        texture_resizing=bank.approximations,
        source_field_precision="8-bit; bounded driver-only reduction",
        continuous_materials=9,
        authored_particle_count=55,
        particle_frame_cadence="source30/60Hz",
        particle_affine_precision="Q12 carried inQ20",
        maximum_particle_affine_error_pixels=max(
            track["maximum_affine_error_pixels"]
            for group in particle_tracks.values()
            for track in group
        ),
        original_gpu_random_history=False,
        new_particle_pool_bytes=0,
    )


def validate(source, image):
    """Validate the bounded table before any optional runtime use."""
    w, h, fmt, raw = image(source.aux[FIRST_AUX])
    if fmt != 0 or raw[:4] != b"ACU1":
        return None
    t.require(
        w == 256 and len(raw) == w * h * 2 and len(raw) >= 64, "Continuous card table geometry"
    )
    head = struct.unpack_from("<4s14H8I", raw)
    (
        _,
        version,
        slots,
        pages,
        pictures,
        fields,
        curves,
        commands,
        tracks,
        nw,
        nh,
        command_bytes,
        frame_bytes,
        z0,
        z1,
        *offsets,
    ) = head
    length, family_at, field_at, curve_at, command_at, track_at, used, reserved = offsets
    t.require(
        (version, slots, nw, nh, command_bytes, frame_bytes, z0, z1, reserved)
        == (1, 56, 156, 218, 128, 38, 0, 0, 0)
        and pages > 0
        and 1 + pages + pictures <= 56
        and length <= pages * PAGE_BYTES
        and fields <= 24
        and curves <= 64
        and commands <= 64
        and tracks == 55
        and family_at == 64
        and field_at == 92
        and curve_at == field_at + fields * 8
        and command_at == curve_at + curves * 8
        and track_at == command_at + commands * 128
        and used == track_at + tracks * 16
        and used <= len(raw),
        "Continuous card header bounds",
    )
    bank = bytearray()
    for i in range(pages):
        pw, ph, pf, payload = image(source.aux[FIRST_AUX + 1 + i])
        t.require(
            (pw, ph, pf, len(payload)) == (320, 205, 0, 131200), "Continuous card page geometry"
        )
        bank += payload[:PAGE_BYTES]
    bank = bytes(bank[:length])
    from nspire_converter.card_affliction_fields import validate as field_validate

    for i in range(fields):
        at, size = struct.unpack_from("<2I", raw, field_at + i * 8)
        t.require(at <= length and size <= length - at, "Continuous card field bounds")
        field_validate(bank[at : at + size])
    for i in range(curves):
        at, width, channels = struct.unpack_from("<I2H", raw, curve_at + i * 8)
        t.require(
            2 <= width <= 1024
            and 1 <= channels <= 4
            and at <= length
            and width * channels * 4 <= length - at,
            "Continuous card curve bounds",
        )
    for i in range(commands):
        c = COMMAND.unpack_from(raw, command_at + i * 128)
        t.require(
            c[0] <= 2
            and c[1] <= 10
            and c[2] in (0, 1)
            and c[3] <= 1
            and -1 <= c[4] < fields
            and -1 <= c[5] < pictures
            and all(-1 <= x < fields for x in c[6:10])
            and all(-1 <= x < curves for x in c[10:12])
            and c[-3] <= tracks
            and c[-2] <= tracks - c[-3]
            and c[-1] == 0,
            "Continuous card command references",
        )
    for i in range(tracks):
        life, delay, fps, count, at = TRACK.unpack_from(raw, track_at + i * 16)
        t.require(
            0 < life <= 600000
            and delay < life
            and fps in (30, 60)
            and 0 < count <= 181
            and at <= length
            and count * 38 <= length - at,
            "Continuous card track bounds",
        )
    for i in range(7):
        first, count = struct.unpack_from("<2H", raw, family_at + i * 4)
        t.require(
            count and first <= commands and count <= commands - first,
            "Continuous card family bounds",
        )
    t.require(not any(raw[used:]), "Continuous card table padding")
    return dict(
        pages=pages,
        fields=fields,
        curves=curves,
        commands=commands,
        particles=tracks,
        bank_bytes=length,
        continuous_materials=9,
        slots_replaced=56,
        maximum_page_bytes=131200,
    )


def convert(data, pck, work):
    from nspire_converter.coalesce_tile_resources import view
    from nspire_converter.correct_source_resources import replace_images
    from nspire_converter.no_animations import mark, reduced
    from nspire_converter.pck import Pack
    from nspire_converter.tanx_scene import group_images

    source = view(data)
    t.require(
        len(source.aux) == BASE_AUX and len(source.images) == BASE_IMAGES,
        "Continuous cards require the complete source catalog",
    )
    t.require(
        validate(source, source.image_raw) is None, "Continuous card materials already installed"
    )
    work = Path(work)
    work.mkdir(parents=True, exist_ok=True)
    pack = Pack(pck)
    try:
        rows, proof = build_rows(pack, work)
    finally:
        pack.close()
    records = []
    ids = source.aux[FIRST_AUX : FIRST_AUX + SLOTS]
    replacements = {}
    t.require(
        len(set(ids)) == SLOTS and all(source.images[i][3] != 6 for i in ids),
        "Old Affliction slots are not independent retained images",
    )
    for i, (w, h, fmt, raw) in enumerate(rows):
        name = f"card-{i:02}.bin"
        t.atomic_bytes(work / name, raw)
        row = dict(
            file=name,
            width=w,
            height=h,
            format=fmt,
            bytes=len(raw),
            sha256=t.sha256(raw),
            raw_sha256=t.sha256(raw),
        )
        records.append(row)
        replacements[ids[i]] = dict(row, file=str(work / name))
    art = group_images(records, work, group_bytes=256 * 1024)
    art["image_ids"] = ids
    output, changed = replace_images(data, {}, raw_replacements=replacements, replacement_art=art)
    if reduced(data):
        output = mark(output)
    target = view(output)
    typed = validate(target, target.image_raw)
    for index, row in zip(ids, rows):
        t.require(target.image_raw(index) == row, "Continuous card decoded bytes changed")
    t.require(
        target.clips == source.clips and target.cards == source.cards and target.aux == source.aux,
        "Continuous cards changed gameplay or stable IDs",
    )
    proof.update(
        complete=True,
        input_bytes=len(data),
        output_bytes=len(output),
        added_bytes=len(output) - len(data),
        input_sha256=t.sha256(data),
        output_sha256=t.sha256(output),
        grouping=art["report"],
        replacement=changed,
        typed=typed,
        animation_profile_preserved=reduced(data) == reduced(output),
        no_animations_keeps_card_material_clock=True,
    )
    return output, proof


def main():
    import argparse

    p = argparse.ArgumentParser(description=__doc__)
    for name in ("input", "output", "pck", "work"):
        p.add_argument("--" + name, type=Path, required=True)
    a = p.parse_args()
    t.require(a.input.resolve() != a.output.resolve(), "Preserve continuous-card input")
    out, proof = convert(a.input.read_bytes(), a.pck, a.work)
    t.atomic_bytes(a.output, out)
    t.atomic_bytes(a.output.with_suffix(".json"), (json.dumps(proof, indent=2) + "\n").encode())
    print(
        json.dumps({key: proof[key] for key in ("complete", "added_bytes", "grouping", "typed")}),
        flush=True,
    )


if __name__ == "__main__":
    main()
