#!/usr/bin/env python3
"""Offline composition of the owner's Godot world scenes.

Only resource names, transforms and our renderer are shipped.  The PCK, textures
and optional Spine runtime remain private conversion inputs.  A scene is not an
image: combat backgrounds acquire one child scene per numbered layer at runtime.
This module performs that instancing before applying the authored CanvasItem
transforms.  Its diagnostics distinguish a static snapshot from particle/shader
simulation; unsupported screen effects are never rendered as their mask image.

The default camera is the original 1920 x 1080 composition, uniformly covered
onto the requested calculator area.  ``viewport`` and ``camera`` are explicit
overrides for callers exporting a particular encounter/window configuration.
"""
from __future__ import annotations

import ast
import hashlib
import math
import re
from collections import OrderedDict
from dataclasses import dataclass, field
from pathlib import PurePosixPath

import numpy as np
from PIL import Image, ImageChops


class SceneRenderError(ValueError):
    pass


# Audited v0.111.0 inline shader fingerprints; no original shader source is
# distributed. Fake Merchant's masked noise UV wobble changes a small authored
# image, not a screen-texture mask. A static scene deliberately keeps that image
# undistorted and records the unsupported motion and its original parameters.
_STATIC_INLINE_SHADERS = {
    'cfd5ba28dcfd923c2bf72f11ad941475b737802f1dd74e6efd2861075d79c4ff':
        'masked_noise_uv_distortion_frozen_to_authored_texture',
}


@dataclass(frozen=True)
class Ref:
    scope: str
    key: str
    external: bool


@dataclass(frozen=True)
class Call:
    name: str
    args: tuple


@dataclass(frozen=True)
class Property:
    text: str
    scope: str


_TOKEN = re.compile(r'\s*(?:([&^]?"(?:\\.|[^"\\])*")|'
                    r'([-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?)|'
                    r'([A-Za-z_][A-Za-z_0-9./]*)|(.))', re.S)


class _Reader:
    """Small non-executing parser for Godot resource values, not GDScript."""
    def __init__(self, text, scope):
        self.scope = scope
        self.tokens = [next(x for x in t if x != '') for t in _TOKEN.findall(text)]
        self.at = 0

    def pop(self, expected=None):
        if self.at >= len(self.tokens):
            raise SceneRenderError('Truncated Godot resource value')
        token = self.tokens[self.at]
        self.at += 1
        if expected is not None and token != expected:
            raise SceneRenderError(f'Expected {expected!r}, found {token!r}')
        return token

    def peek(self):
        return self.tokens[self.at] if self.at < len(self.tokens) else None

    def sequence(self, end):
        out = []
        while self.peek() != end:
            out.append(self.value())
            if self.peek() != end:
                self.pop(',')
        self.pop(end)
        return tuple(out)

    def value(self):
        token = self.pop()
        if token.startswith(('"', '&"', '^"')):
            # Godot permits physical newlines inside a quoted resource string.
            # Escape only those physical control characters before the safe
            # literal decoder; the shader remains data and is never executed.
            quoted = token[token.index('"'):].replace('\r', '\\r').replace('\n', '\\n')
            try:
                return ast.literal_eval(quoted)
            except (ValueError, SyntaxError) as exc:
                raise SceneRenderError('Invalid quoted Godot resource value') from exc
        if token in ('true', 'false', 'null'):
            return {'true': True, 'false': False, 'null': None}[token]
        if token[0] in '+-.0123456789':
            try:
                return float(token) if any(x in token for x in '.eE') else int(token)
            except ValueError as exc:
                raise SceneRenderError('Invalid numeric resource value') from exc
        if token == '[':
            return self.sequence(']')
        if token == '{':
            out = {}
            while self.peek() != '}':
                key = self.value()
                self.pop(':')
                out[key] = self.value()
                if self.peek() != '}':
                    self.pop(',')
            self.pop('}')
            return out
        if self.peek() == '(':
            self.pop('(')
            args = self.sequence(')')
            if token in ('ExtResource', 'SubResource'):
                return Ref(self.scope, str(args[0]), token == 'ExtResource')
            return Call(token, args)
        raise SceneRenderError(f'Unsupported resource expression {token!r}')


def parse_value(text, scope=''):
    reader = _Reader(text, scope)
    out = reader.value()
    if reader.peek() is not None:
        raise SceneRenderError('Trailing resource expression')
    return out


def _property(props, key, default=None):
    value = props.get(key)
    return default if value is None else parse_value(value.text, value.scope)


def _vector(value, length=2):
    if isinstance(value, Call):
        value = value.args
    if value is None:
        value = (0,) * length
    if len(value) != length:
        raise SceneRenderError(f'Expected {length} vector components')
    return np.asarray(value, dtype=float)


def _translate(x, y):
    return np.array([[1., 0., x], [0., 1., y], [0., 0., 1.]])


def _scale(x, y=None):
    return np.diag([x, x if y is None else y, 1.])


def _node_matrix(props, origin=(0., 0.)):
    direct = _property(props, 'transform')
    if direct is not None:
        a, b, c, d, tx, ty = _vector(direct, 6)
        return np.array([[a, c, tx], [b, d, ty], [0., 0., 1.]])
    pos = np.asarray(origin) + _vector(_property(props, 'position'))
    sx, sy = _vector(_property(props, 'scale', (1, 1)))
    angle = float(_property(props, 'rotation', 0))
    skew = float(_property(props, 'skew', 0))
    matrix = np.array([[math.cos(angle) * sx, -math.sin(angle + skew) * sy, 0.],
                       [math.sin(angle) * sx, math.cos(angle + skew) * sy, 0.],
                       [0., 0., 1.]])
    pivot = _vector(_property(props, 'pivot_offset'))
    return _translate(*(pos + pivot)) @ matrix @ _translate(*-pivot)


@dataclass
class Node:
    name: str
    kind: str
    scope: str
    props: dict = field(default_factory=dict)
    children: list = field(default_factory=list)
    instance: str | None = None
    # PackedScene applies properties before adding each child. Spine's resource
    # setter has already appended its internal meshes at that point. Keep these
    # operations so an explicit child index can be replayed against that list.
    child_ops: list = field(default_factory=list)


@dataclass
class Document:
    path: str
    sections: list
    external: dict
    internal: dict
    resource: tuple | None = None


def _quoted_after(text, quoted=False):
    escaped = False
    for character in text:
        if escaped:
            escaped = False
        elif character == '\\' and quoted:
            escaped = True
        elif character == '"':
            quoted = not quoted
        elif character == ';' and not quoted:
            break
    return quoted


def _sections(text, path):
    # A shader's quoted source can itself contain assignments and bracketed
    # lines. Only recognize headers and property keys outside that string.
    blocks = []
    quoted = False
    for line in text.splitlines():
        match = None if quoted else re.match(r'^\[([a-z_]+)(.*?)\]\s*$', line)
        if match:
            blocks.append((match[1], match[2], []))
        elif blocks:
            blocks[-1][2].append(line)
        quoted = _quoted_after(line, quoted)
    if quoted:
        raise SceneRenderError('Unterminated resource string: ' + path)
    result = []
    for kind, attributes, lines in blocks:
        reader = _Reader(attributes, path)
        attrs = {}
        while reader.peek() is not None:
            key = reader.pop()
            reader.pop('=')
            attrs[key] = reader.value()
        props = {}
        key, quoted = None, False
        for line in lines:
            found = None if quoted else re.match(r'^([^\s=][^=]*?)\s*=\s*(.*)$', line)
            if found:
                key = found[1].strip()
                props[key] = Property(found[2], path)
            elif key and (quoted or line.strip() and not line.lstrip().startswith(';')):
                prev = props[key]
                props[key] = Property(prev.text + '\n' + line, path)
            quoted = _quoted_after(line, quoted)
        result.append((kind, attrs, props))
    return result


class SceneLoader:
    def __init__(self, pack):
        self.pack = pack
        self.documents = {}

    def document(self, path):
        path = path.removeprefix('res://')
        if path not in self.documents:
            data = self.pack.read(path)
            if len(data) > 16 * 1024 * 1024:
                raise SceneRenderError('Scene resource exceeds conversion bound')
            sections = _sections(data.decode('utf-8'), path)
            external = {a['id']: (a, p) for k, a, p in sections if k == 'ext_resource'}
            internal = {a['id']: (a, p) for k, a, p in sections if k == 'sub_resource'}
            resource = next(((a, p) for k, a, p in sections if k == 'resource'), None)
            self.documents[path] = Document(path, sections, external, internal, resource)
        return self.documents[path]

    def resource(self, ref):
        if isinstance(ref, str):
            path = ref.removeprefix('res://')
            if not path.endswith('.tres'):
                return {'type': 'Texture2D', 'path': path}, {}
            doc = self.document(path)
            header = next((a for k, a, p in doc.sections if k == 'gd_resource'), {})
            if doc.resource is None:
                raise SceneRenderError('Resource file has no root resource: ' + path)
            return {**header, 'path': path}, doc.resource[1]
        if not isinstance(ref, Ref):
            raise SceneRenderError('Expected a scoped resource reference')
        doc = self.document(ref.scope)
        try:
            attrs, props = (doc.external if ref.external else doc.internal)[ref.key]
        except KeyError as exc:
            raise SceneRenderError(f'Missing resource {ref.scope}:{ref.key}') from exc
        if ref.external and str(attrs.get('path', '')).endswith('.tres'):
            child = self.document(attrs['path'])
            header = next((a for k, a, p in child.sections if k == 'gd_resource'), {})
            if child.resource:
                return {**attrs, **header}, child.resource[1]
        return attrs, props

    def path(self, ref):
        attrs, _ = self.resource(ref)
        path = attrs.get('path')
        if not path:
            raise SceneRenderError('Expected an external resource path')
        return str(path).removeprefix('res://')

    def instantiate(self, path, stack=()):
        path = path.removeprefix('res://')
        if path in stack or len(stack) >= 32:
            raise SceneRenderError('Recursive or excessively nested scene instance')
        doc = self.document(path)
        root = None
        nodes = {}
        for kind, attrs, props in doc.sections:
            if kind != 'node':
                continue
            name = attrs['name']
            parent_path = attrs.get('parent')
            key = '.' if parent_path is None else name if parent_path == '.' else parent_path + '/' + name
            if 'instance' in attrs:
                child_path = self.path(attrs['instance'])
                node = self.instantiate(child_path, stack + (path,))
                node.name = name
                node.instance = child_path
                node.props.update(props)
            elif key in nodes:
                # Editable inherited children override their original properties.
                node = nodes[key]
                node.props.update(props)
                if 'index' in attrs and parent_path is not None:
                    parent = nodes[parent_path]
                    parent.child_ops.append((node, int(attrs['index'])))
                    parent.children.remove(node)
                    parent.children.insert(int(attrs['index']), node)
                continue
            else:
                if 'type' not in attrs:
                    raise SceneRenderError(f'Unknown inherited node {path}:{key}')
                node = Node(name, attrs['type'], path, dict(props))
            if root is None:
                root = node
            else:
                try:
                    parent = nodes[parent_path]
                    parent.children.append(node)
                    at = int(attrs.get('index', -1))
                    parent.child_ops.append((node, at))
                    if at >= 0:
                        parent.children.remove(node)
                        parent.children.insert(at, node)
                except KeyError as exc:
                    raise SceneRenderError(f'Missing scene parent {path}:{parent_path}') from exc
            def index(current, at):
                nodes[at] = current
                for child in current.children:
                    index(child, child.name if at == '.' else at + '/' + child.name)
            index(node, key)
            if len(nodes) > 16384:
                raise SceneRenderError('Scene node count exceeds conversion bound')
        if root is None:
            raise SceneRenderError(f'No root node in {path}')
        return root


def background_layers(pack, path, variants=None):
    """Resolve the original _bg_ groups; choose a recorded variant per group.

    Defaults deliberately use the first sorted resource, not editor VisibleLayer
    hints (NBgLayerDebug only executes in the editor).  A caller can supply a
    group -> index/path map, or one nonnegative index for all groups.
    """
    directory = path.removeprefix('res://').rsplit('/', 1)[0] + '/layers/'
    groups = {}
    for key in sorted(pack.entries):
        if not key.startswith(directory) or not key.endswith('.tscn'):
            continue
        name = key[len(directory):]
        if '/' in name:
            raise SceneRenderError('Combat layer directory unexpectedly contains a subdirectory')
        if '_bg_' in name:
            group = name.split('_bg_', 1)[1].split('_', 1)[0]
        elif '_fg_' in name:
            group = 'fg'
        else:
            raise SceneRenderError('Unclassified combat background layer ' + key)
        groups.setdefault(group, []).append(key)
    if not groups:
        raise SceneRenderError('No original combat layers under ' + directory)
    result = []
    for number, group in enumerate(sorted(k for k in groups if k != 'fg')):
        result.append((f'Layer_{number:02d}', group, groups[group]))
    if 'fg' in groups:
        result.append(('Foreground', 'fg', groups['fg']))
    selected = []
    for node, group, choices in result:
        choice = variants.get(group, 0) if isinstance(variants, dict) else variants or 0
        if isinstance(choice, str):
            chosen = choice.removeprefix('res://')
            if chosen not in choices:
                raise SceneRenderError('Requested layer is not in the original variant group')
        else:
            if not isinstance(choice, int) or choice < 0 or choice >= len(choices):
                raise SceneRenderError(f'Invalid variant index {choice} for {group}')
            chosen = choices[choice]
        selected.append({'node': node, 'group': group, 'path': chosen, 'choices': choices})
    return selected


# Numeric camera facts from the pinned encounter models.  Ordinary act scenery
# can be used by several encounters; an encounter-specific camera is optional.
_CAMERAS = {
    'ceremonial_beast_boss': (.9, 0, 50), 'kaiser_crab_boss': (.75, 0, 35),
    'queen_boss': (.9, 0, 60), 'the_insatiable_boss': (.9, 0, 0),
    'waterfall_giant_boss': (.9, 0, 0), 'knowledge_demon_boss': (.85, 0, 70),
}
_CONTROLS = {'Control', 'TextureRect', 'ColorRect', 'Panel', 'Container'}
_PARTICLES = {'CPUParticles2D', 'GPUParticles2D'}
_PLAIN = {'Node', 'Node2D', 'Marker2D', 'SpineBoneNode', 'SpineSlotNode', 'SpineMesh2D'}


class _Renderer:
    def __init__(self, pack, width, height, viewport, texture_loader, sampler, time, supersample):
        self.pack = pack
        self.loader = SceneLoader(pack)
        self.width, self.height = width, height
        self.canvas = (width * supersample, height * supersample)
        self.viewport = np.array(viewport, dtype=float)
        scale = max(self.canvas[0] / viewport[0], self.canvas[1] / viewport[1])
        self.screen = _translate((self.canvas[0] - viewport[0] * scale) / 2,
                                 (self.canvas[1] - viewport[1] * scale) / 2) @ _scale(scale)
        self.texture_loader = texture_loader
        self.sampler = sampler
        self.owns_sampler = False
        self.time = time
        self.phobia_mode = False
        self.textures = OrderedDict()
        self.texture_bytes = 0
        self.commands = []
        self.diagnostics = {'viewport': list(viewport), 'size': [width, height],
                            'sample_time': time, 'fit': 'uniform_cover', 'layers': [],
                            'drawn': [], 'omitted': [], 'approximations': [],
                            'spines': [], 'shared_background': [], 'conditions': []}

    def report(self, category, node, path, **details):
        item = {'node': path, 'scene': node.scope, **details}
        self.diagnostics[category].append(item)

    def texture(self, ref):
        attrs, props = self.loader.resource(ref)
        if attrs.get('type') == 'AtlasTexture':
            source = self.texture(_property(props, 'atlas'))
            x, y, w, h = _vector(_property(props, 'region'), 4)
            result = source.crop((round(x), round(y), round(x + w), round(y + h)))
            margin = _vector(_property(props, 'margin', (0, 0, 0, 0)), 4)
            if any(margin):
                mx, my, mw, mh = margin
                full = Image.new('RGBA', (round(w + mw), round(h + mh)))
                full.alpha_composite(result, (round(mx), round(my)))
                result = full
            return result
        path = str(attrs.get('path', '')).removeprefix('res://')
        if not path:
            raise SceneRenderError('Unsupported generated texture ' + str(attrs.get('type')))
        if path in self.textures:
            self.textures.move_to_end(path)
            return self.textures[path]
        image = self.texture_loader(self.pack, path).convert('RGBA')
        needed = image.width * image.height * 4
        while self.textures and self.texture_bytes + needed > 96 * 1024 * 1024:
            _, old = self.textures.popitem(last=False)
            self.texture_bytes -= old.width * old.height * 4
        self.textures[path] = image
        self.texture_bytes += needed
        return image

    def material(self, node):
        ref = _property(node.props, 'material')
        if ref is None:
            return None, {}, 0, {}
        attrs, props = self.loader.resource(ref)
        blend = int(_property(props, 'blend_mode', 0))
        # Godot CanvasItemMaterial: mix/add/sub/mul/premul; Spine uses a different enum.
        blend = {0: 0, 1: 1, 2: 4, 3: 2, 4: 5}.get(blend, 0)
        shader = _property(props, 'shader')
        if shader is None:
            return None, props, blend, {}
        shader_attrs, shader_props = self.loader.resource(shader)
        source = shader_attrs.get('path')
        if source:
            return str(source).removeprefix('res://'), props, blend, {}
        # VisualShader is a Shader subclass. Godot serializes both its graph
        # and the generated code string in an inline resource; the same strict
        # hash/diagnostic path applies without treating its mask as plain art.
        if shader_attrs.get('type') not in ('Shader', 'VisualShader'):
            raise SceneRenderError('Material shader is not a Shader resource: ' + node.scope)
        code = _property(shader_props, 'code')
        if not isinstance(code, str):
            raise SceneRenderError('Inline Shader has no source string: ' + node.scope)
        digest = hashlib.sha256(code.encode('utf-8')).hexdigest()
        identity = shader.scope + '::' + shader.key
        return identity, props, blend, {'inline_sha256': digest,
            'static_reason': _STATIC_INLINE_SHADERS.get(digest)}

    def control_size(self, node, parent_size, image=None):
        props = node.props
        left = float(_property(props, 'anchor_left', 0)) * parent_size[0] + float(_property(props, 'offset_left', 0))
        top = float(_property(props, 'anchor_top', 0)) * parent_size[1] + float(_property(props, 'offset_top', 0))
        right = float(_property(props, 'anchor_right', 0)) * parent_size[0] + float(_property(props, 'offset_right', 0))
        bottom = float(_property(props, 'anchor_bottom', 0)) * parent_size[1] + float(_property(props, 'offset_bottom', 0))
        size = np.maximum(0, [right - left, bottom - top])
        minimum = _vector(_property(props, 'custom_minimum_size'))
        if image is not None:
            # Godot TextureRect ExpandMode (4.x class reference): proportional
            # fitting changes minimum size, independently of stretch_mode.
            mode = int(_property(props, 'expand_mode', 0))
            if mode == 0:
                minimum = np.maximum(minimum, image.size)
            elif mode in (2, 3):
                minimum[0] = max(minimum[0], size[1] * (image.width / image.height if mode == 3 else 1))
            elif mode in (4, 5):
                minimum[1] = max(minimum[1], size[0] * (image.height / image.width if mode == 5 else 1))
            elif mode != 1:
                raise SceneRenderError('Unknown TextureRect expand mode')
        for axis in (0, 1):
            if size[axis] < minimum[axis]:
                grow = int(_property(props, 'grow_horizontal' if axis == 0 else 'grow_vertical', 1))
                delta = minimum[axis] - size[axis]
                if grow == 0:
                    if axis == 0: left -= delta
                    else: top -= delta
                elif grow == 2:
                    if axis == 0: left -= delta / 2
                    else: top -= delta / 2
                size[axis] = minimum[axis]
        return (left, top), size

    def image_geometry(self, node, image, size):
        props = node.props
        if node.kind in ('TextureRect', 'ColorRect'):
            width, height = size
            x = y = 0.
            mode = int(_property(props, 'stretch_mode', 0))
            if mode in (2, 3, 4, 5, 6):
                iw, ih = image.size
                if mode in (2, 3):
                    width, height = iw, ih
                else:
                    factor = (max if mode == 6 else min)(size[0] / iw, size[1] / ih)
                    width, height = iw * factor, ih * factor
                if mode in (3, 5, 6):
                    x, y = (size - [width, height]) / 2
            elif mode == 1:
                if width <= 0 or height <= 0:
                    return image, (0, 0, 0, 0)
                tiled = Image.new('RGBA', (math.ceil(width), math.ceil(height)))
                for oy in range(0, tiled.height, image.height):
                    for ox in range(0, tiled.width, image.width):
                        tiled.paste(image, (ox, oy))
                image = tiled
            elif mode != 0:
                raise SceneRenderError('Unknown TextureRect stretch mode')
        else:
            if _property(props, 'region_enabled', False):
                x0, y0, width, height = _vector(_property(props, 'region_rect'), 4)
                image = image.crop((round(x0), round(y0), round(x0 + width), round(y0 + height)))
            hframes, vframes = int(_property(props, 'hframes', 1)), int(_property(props, 'vframes', 1))
            if hframes < 1 or vframes < 1:
                raise SceneRenderError('Invalid Sprite2D frame grid')
            if hframes != 1 or vframes != 1:
                frame = int(_property(props, 'frame', 0))
                coords = _property(props, 'frame_coords')
                fx, fy = _vector(coords) if coords else (frame % hframes, frame // hframes)
                fw, fh = image.width / hframes, image.height / vframes
                image = image.crop(tuple(round(x) for x in (fx * fw, fy * fh, (fx + 1) * fw, (fy + 1) * fh)))
            width, height = image.size
            x, y = _vector(_property(props, 'offset'))
            if _property(props, 'centered', True):
                x -= width / 2
                y -= height / 2
        if _property(props, 'flip_h', False):
            image = image.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
        if _property(props, 'flip_v', False):
            image = image.transpose(Image.Transpose.FLIP_TOP_BOTTOM)
        return image, (x, y, width, height)

    def image_layer(self, image, rect, matrix, tint, wind=None):
        x, y, width, height = rect
        if width <= 0 or height <= 0:
            return Image.new('RGBA', self.canvas)
        local = _translate(x, y) @ _scale(width / image.width, height / image.height)
        if wind is not None:
            # The pinned wind shader displaces x linearly with UV.y.  At its four
            # quad vertices this is exactly an affine shear, including heightOffset.
            top, bottom = wind
            local = _translate(x + top, y) @ np.array([
                [width / image.width, (bottom - top) / image.height, 0.],
                [0., height / image.height, 0.], [0., 0., 1.]])
        mapping = self.screen @ matrix @ local
        if abs(np.linalg.det(mapping)) < 1e-12:
            return Image.new('RGBA', self.canvas)
        inverse = np.linalg.inv(mapping)
        result = image.transform(self.canvas, Image.Transform.AFFINE,
                                 tuple(inverse[:2].ravel()), Image.Resampling.BICUBIC)
        if np.any(np.asarray(tint) != 1):
            data = np.asarray(result, dtype=float) * np.asarray(tint)
            result = Image.fromarray(np.uint8(np.clip(np.rint(data), 0, 255)))
        return result

    def shader_layer(self, node, path, image, rect, matrix, tint, shader,
                     material, blend, shader_info):
        """Optional reviewed shader evaluator; ordinary scene output is unchanged.

        A subclass returns a full-canvas RGBA image and its blend mode, or None
        to retain the existing shader handling and explicit omission report.
        """
        return None

    def spine(self, node, path, matrix, tint, clips, z):
        resource = self.loader.path(_property(node.props, 'skeleton_data_res'))
        if self.sampler is None:
            from world_enemy_assets import SpineSampler
            self.sampler = SpineSampler(self.pack)
            self.owns_sampler = True
        metadata = self.sampler.metadata(resource)
        animations = metadata['clips']
        auto = any(str(self.script(c)).endswith('/NSpineAutoPlayer.cs') for c in node.children)
        if auto and len(animations) != 1:
            raise SceneRenderError('NSpineAutoPlayer requires exactly one animation: ' + resource)
        clip = animations[0]['name'] if auto else None
        frame = self.sampler.sample(resource, clip, [self.time])[0]
        flip = _scale(1, -1)
        # animation.raster expects y-up; express the output-canvas points in that
        # convention so its final y flip yields the authored Godot screen points.
        screen_matrix = flip @ self.screen @ matrix @ flip
        from animation import raster
        commands = {}
        geometries = self.sampler.draw(resource, frame)
        if len(geometries) != len(frame['draw']):
            raise SceneRenderError('Spine geometry lost its slot association: ' + resource)
        for draw, geometry in zip(frame['draw'], geometries):
            vertices, uvs, triangles, texture, color, blend = geometry
            v = np.column_stack((vertices, np.ones(len(vertices))))
            points = (screen_matrix @ v.T).T[:, :2]
            layer = raster([(points, uvs, triangles, texture, np.asarray(color) * tint, 0)],
                           1., np.array([0., 0.]), self.canvas)
            commands.setdefault(draw['slot'], []).append((z, layer, blend, clips))
        self.report('spines', node, path, resource=resource, clip=clip,
                    draw_slots=len(frame['draw']), source_version=metadata.get('source_version'))
        bones = {('__spine__', 'matrix'): matrix, ('__spine__', 'path'): path}
        for bone in frame.get('bones', []):
            if 'node_matrix' in bone:
                a, b, tx, c, d, ty = bone['node_matrix']
                bones[bone['name']] = np.array([[a, b, tx], [c, d, ty], [0, 0, 1.]])
            else:
                # Compatibility for simple fixtures and older adapter output.
                # Actual Godot Follow nodes use rotation/scale, dropping shear.
                a, b, tx, c, d, ty = bone['matrix']
                bones[bone['name']] = flip @ np.array([[a, b, tx], [c, d, ty], [0, 0, 1.]]) @ flip
        for slot in frame.get('slots', []):
            a, b, tx, c, d, ty = slot['node_matrix'] if 'node_matrix' in slot else slot['matrix']
            local = np.array([[a, b, tx], [c, d, ty], [0, 0, 1.]])
            bones[('slot', slot['slot'])] = local if 'node_matrix' in slot else flip @ local @ flip
        # The adapter emits slots in Skeleton.DrawOrder, including slots with no
        # active attachment. Each still owns an internal SpineMesh2D placeholder.
        draw_order = frame.get('draw_order', [slot['slot'] for slot in frame.get('slots', [])])
        if not draw_order:
            draw_order = list(commands)
        if len(draw_order) != len(set(draw_order)) or any(slot not in draw_order for slot in commands):
            raise SceneRenderError('Invalid complete Spine draw order: ' + resource)
        return bones, commands, draw_order

    def spine_children(self, node, path, draw_order):
        """Replay Godot 4.5 PackedScene + Spine 4.2 sort_slot_nodes once."""
        meshes = [('mesh', i) for i in range(len(draw_order))]
        order = list(meshes)
        children = {id(child): child for child in node.children}
        operations = node.child_ops or [(child, -1) for child in node.children]
        for child, at in operations:
            token = ('node', id(child))
            if token not in order:
                order.append(token)
            if at >= 0:
                if at >= len(order):
                    raise SceneRenderError('Scene child index exceeds instantiated siblings: ' + path + '/' + child.name)
                order.remove(token)
                order.insert(at, token)
        by_slot = {}
        for kind, identity in order:
            if kind != 'node':
                continue
            child = children[identity]
            if child.kind == 'SpineSlotNode':
                slot = _property(child.props, 'slot_name')
                if slot not in draw_order:
                    raise SceneRenderError('Missing sampled Spine slot ' + str(slot) + ': ' + path + '/' + child.name)
                by_slot.setdefault(slot, []).append(('node', identity))
        for i, slot in enumerate(draw_order):
            # The original captures this index once per slot. Recomputing it
            # for every child gives different results for shared slot nodes.
            mesh_index = order.index(meshes[i])
            for token in by_slot.get(slot, []):
                order.remove(token)
                order.insert(mesh_index + 1, token)
        return [(kind, draw_order[identity] if kind == 'mesh' else children[identity])
                for kind, identity in order]

    def script(self, node):
        ref = _property(node.props, 'script')
        return self.loader.path(ref) if ref else ''

    def visit(self, node, parent_matrix, parent_size, parent_tint, parent_z, path, clips=(), bones=None):
        props = node.props
        if not _property(props, 'visible', True):
            self.report('omitted', node, path, reason='authored_hidden')
            return
        if node.name == 'PhobiaModeVisual' and not self.phobia_mode:
            self.report('conditions', node, path, condition='phobia_mode=false')
            return
        if node.instance and node.instance.endswith('/kaiser_crab_boss_setup.tscn'):
            matrix = parent_matrix @ _node_matrix(props)
            z = int(_property(props, 'z_index', 0)) + (parent_z if _property(props, 'z_as_relative', True) else 0)
            self.commands.append((z, None, 0, ()))
            self.report('shared_background', node, path, resource=node.instance,
                        transform=matrix[:2].ravel().tolist(),
                        screen_transform=(self.screen @ matrix)[:2].ravel().tolist(),
                        roles=['KaiserCrab', 'Crusher', 'Rocket'], z_index=z,
                        static_planes=['back', 'front'])
            return
        if any(self.script(c).endswith('/NCeremonialBeastBgVfx.cs') for c in node.children):
            self.report('conditions', node, path, condition='ceremonial_beast_first_phase_hidden')
            return
        if self.script(node).endswith('/NQueenRepyBgVfx.cs'):
            self.report('conditions', node, path, condition='freed_repy=false')
        if node.kind in _PARTICLES:
            self.report('omitted', node, path, reason='dynamic_particles', kind=node.kind)
            return
        if node.kind == 'SpineMesh2D':
            # The official 4.2 constructor starts renderer_object at null and
            # binds no serialized mesh geometry. generate_meshes_for_slots
            # creates distinct internal instances, already represented by
            # spine_commands. Additional scene-authored mesh nodes therefore
            # draw nothing; their transforms, ordering and children still apply.
            self.report('conditions', node, path, condition='serialized_spine_mesh_empty')
        image = None
        texture_ref = _property(props, 'texture')
        if node.kind in ('Sprite2D', 'TextureRect') and texture_ref:
            image = self.texture(texture_ref)
        if node.kind == 'ColorRect':
            image = Image.new('RGBA', (1, 1), tuple(round(v * 255) for v in _vector(_property(props, 'color', (1, 1, 1, 1)), 4)))
        size = parent_size
        origin = (0, 0)
        if node.kind in _CONTROLS:
            origin, size = self.control_size(node, parent_size, image)
        local = _node_matrix(props, origin)
        followed = None
        if node.kind == 'SpineBoneNode' and int(_property(props, 'bone_mode', 0)) == 1:
            raise SceneRenderError('Spine Drive node requires runtime driver state: ' + path)
        if node.kind == 'SpineBoneNode' and bones is not None:
            bone_name = _property(props, 'bone_name')
            if bone_name not in bones:
                raise SceneRenderError('Missing sampled Spine bone ' + str(bone_name))
            # BoneNode searches ancestors and assigns a GLOBAL transform, so
            # intervening scene wrappers must not transform the bone twice.
            followed = bones[('__spine__', 'matrix')] @ bones[bone_name]
        elif node.kind == 'SpineSlotNode' and bones is not None and bones.get(('__spine__', 'path')) == path.rsplit('/', 1)[0]:
            slot_name = _property(props, 'slot_name')
            if ('slot', slot_name) not in bones:
                raise SceneRenderError('Missing sampled Spine slot ' + str(slot_name) + ': ' + path)
            local = bones[('slot', slot_name)]
        # SlotNode requires a direct SpineSprite parent. A nested ordinary
        # SlotNode keeps its authored transform, matching the runtime warning.
        matrix = followed if followed is not None else parent_matrix @ local
        tint = parent_tint * _vector(_property(props, 'modulate', (1, 1, 1, 1)), 4)
        own_tint = tint * _vector(_property(props, 'self_modulate', (1, 1, 1, 1)), 4)
        z = int(_property(props, 'z_index', 0)) + (parent_z if _property(props, 'z_as_relative', True) else 0)
        layer = None
        shader, material, blend, shader_info = self.material(node)
        if image is not None:
            image, rect = self.image_geometry(node, image, size)
            wind = None
            handled = self.shader_layer(node, path, image, rect, matrix, own_tint,
                                        shader, material, blend, shader_info)
            if handled is not None:
                layer, blend = handled
            elif shader and shader.endswith('/wind_sway.gdshader'):
                def amount(key, default):
                    return float(_property(material, 'shader_parameter/' + key, default))
                time = self.time * amount('speed', 1) + amount('offset', 0)
                low, high = amount('minStrength', .05), amount('maxStrength', .01)
                diff = (high - low) ** 2
                strength = min(max(low + diff + math.sin(time / amount('interval', 3.5)) * diff, low), high)
                force = (math.sin(time) + math.cos(time * amount('detail', 1))) * strength * amount('strengthScale', 100)
                height = amount('heightOffset', 0)
                wind = (force * max(0, 1 - height), force * max(0, -height))
            elif shader and shader.endswith('/vfx_water_reflection.gdshader'):
                self.report('approximations', node, path, reason='water_distortion_frozen_to_authored_texture', shader=shader)
            elif shader_info.get('static_reason'):
                self.report('approximations', node, path, reason=shader_info['static_reason'],
                            shader=shader, shader_sha256=shader_info['inline_sha256'],
                            speed=float(_property(material, 'shader_parameter/speed', .1)),
                            strength=float(_property(material, 'shader_parameter/strength', .1)),
                            sample_time=self.time)
            elif shader:
                self.report('omitted', node, path, reason='dynamic_shader', shader=shader,
                            **({'shader_sha256': shader_info['inline_sha256']} if shader_info else {}))
                image = None
            if image is not None:
                if handled is None:
                    layer = self.image_layer(image, rect, matrix, own_tint, wind)
                if node.kind == 'TextureRect' and int(_property(props, 'stretch_mode', 0)) == 6:
                    bounds = self.image_layer(Image.new('RGBA', (1, 1), 'white'),
                                              (0, 0, *size), matrix, np.ones(4))
                    layer.putalpha(ImageChops.multiply(layer.getchannel('A'), bounds.getchannel('A')))
                self.report('drawn', node, path, kind=node.kind,
                            texture=self.loader.path(texture_ref) if texture_ref and self.loader.resource(texture_ref)[0].get('path') else None,
                            transform=matrix[:2].ravel().tolist(), rect=list(rect), blend=blend)
        child_clips = clips
        clipping = int(_property(props, 'clip_children', 0))
        if clipping:
            if layer is None:
                if shader:
                    self.report('omitted', node, path, reason='children_depend_on_dynamic_shader_mask')
                    return
                raise SceneRenderError('CanvasItem child mask could not be rendered: ' + path)
            child_clips = clips + (layer.getchannel('A'),)
        if _property(props, 'clip_contents', False):
            mask = self.image_layer(Image.new('RGBA', (1, 1), 'white'), (0, 0, *size), matrix, np.ones(4))
            child_clips = child_clips + (mask.getchannel('A'),)
        sampled_bones = bones
        if node.kind == 'SpineSprite':
            sampled_bones, spine_commands, draw_order = self.spine(node, path, matrix, own_tint, clips, z)
            ordered = self.spine_children(node, path, draw_order)
            # Spine forces every direct Node2D behind its parent. Mesh nodes
            # and slot followers are therefore ordered together, even when a
            # scene serialized a different show_behind_parent value.
            def behind(item):
                kind, child = item
                return kind == 'mesh' or child.kind not in _CONTROLS | {'Node'} or bool(_property(child.props, 'show_behind_parent', False))
            for kind, child in sorted(ordered, key=lambda item: not behind(item)):
                if kind == 'mesh':
                    self.commands.extend(spine_commands.get(child, []))
                else:
                    self.visit(child, matrix, size, tint, z, path + '/' + child.name, child_clips, sampled_bones)
            return
        elif node.kind not in _CONTROLS | _PLAIN | {'Sprite2D'}:
            raise SceneRenderError('Unsupported visible scene node ' + node.kind)
        ordered = sorted(node.children, key=lambda n: not bool(_property(n.props, 'show_behind_parent', False)))
        for child in ordered:
            if _property(child.props, 'show_behind_parent', False):
                self.visit(child, matrix, size, tint, z, path + '/' + child.name, child_clips, sampled_bones)
        if layer is not None and clipping != 1:
            self.commands.append((z, layer, blend, clips))
        for child in ordered:
            if not _property(child.props, 'show_behind_parent', False):
                self.visit(child, matrix, size, tint, z, path + '/' + child.name, child_clips, sampled_bones)

    def finish(self, plane='all'):
        out = np.zeros((self.canvas[1], self.canvas[0], 4), dtype=float)
        after_shared = False
        for _, image, blend, clips in sorted(self.commands, key=lambda item: item[0]):
            if image is None:
                after_shared = True
                continue
            if plane == 'back' and after_shared or plane == 'front' and not after_shared:
                continue
            if clips:
                image = image.copy()
                alpha = image.getchannel('A')
                for clip in clips:
                    alpha = ImageChops.multiply(alpha, clip)
                image.putalpha(alpha)
            src = np.asarray(image, dtype=float) / 255.
            sa, da = src[:, :, 3:4], out[:, :, 3:4]
            alpha = sa + da * (1 - sa)
            s, d = src[:, :, :3], out[:, :, :3]
            if blend == 1:
                color = np.minimum(1, d + s * sa)
            elif blend == 4:
                color = np.maximum(0, d - s * sa)
            elif blend == 5:
                color = np.divide(s + d * da * (1 - sa), alpha, out=np.zeros_like(s), where=alpha > 0)
            else:
                mix = s * d if blend == 2 else s + d - s * d if blend == 3 else s
                premul = s * sa * (1 - da) + d * da * (1 - sa) + mix * sa * da
                color = np.divide(premul, alpha, out=np.zeros_like(s), where=alpha > 0)
            out[:, :, :3], out[:, :, 3:4] = color, alpha
        image = Image.fromarray(np.uint8(np.clip(np.rint(out * 255), 0, 255)))
        if image.size != (self.width, self.height):
            image = image.resize((self.width, self.height), Image.Resampling.LANCZOS)
        if image.getchannel('A').getbbox() is None and plane == 'all':
            raise SceneRenderError('Scene has no rendered pixels; inspect its required dynamic nodes')
        self.diagnostics['opaque_fraction'] = float(np.mean(np.asarray(image)[:, :, 3] == 255))
        self.diagnostics['draw_commands'] = sum(command[1] is not None for command in self.commands)
        self.diagnostics['plane'] = plane
        return image, self.diagnostics


def load_scene_texture(pack, path, *, texture_loader=None, cache=None):
    """Read a Texture2D or AtlasTexture, including its untrimmed margin.

    ``path`` is the complete original resource path. ``cache`` may be a shared
    OrderedDict of decoded source textures; it remains bounded to 96 MiB.
    This also supports compressed.sprites resources outside ui_atlas.sprites.
    """
    if texture_loader is None:
        from build_content import texture as texture_loader
    renderer = _Renderer(pack, 1, 1, (1, 1), texture_loader, None, 0., 1)
    if cache is not None:
        if not isinstance(cache, OrderedDict):
            raise SceneRenderError('Texture cache must be an OrderedDict')
        renderer.textures = cache
        renderer.texture_bytes = sum(image.width * image.height * 4 for image in cache.values())
    return renderer.texture(path).copy()


def select_phobia_visuals(root):
    """Apply NCombatBackgroundLayer's two mutually exclusive child branches.

    A regular render never calls this routine. Only an explicit alternate
    render can override the authored visibility of the replacement branch.
    """
    changed = 0
    alternatives = [child for child in root.children if child.name == 'PhobiaModeVisual']
    if alternatives:
        regular = [child for child in root.children if child.name == 'Visual']
        if len(alternatives) != 1 or len(regular) != 1:
            raise SceneRenderError('Ambiguous original phobia background branches')
        regular[0].props['visible'] = Property('false', regular[0].scope)
        alternatives[0].props['visible'] = Property('true', alternatives[0].scope)
        changed += 1
    for child in root.children: changed += select_phobia_visuals(child)
    return changed


def render_scene(pack, path, width=320, height=218, mode='scene', *, variants=None,
                 viewport=(1920, 1080), camera=None, texture_loader=None,
                 sampler=None, sample_time=0., supersample=1, plane='all', phobia_mode=False):
    """Return ``(RGBA image, diagnostics)`` for an original static world scene.

    ``sampler`` implements world_enemy_assets.SpineSampler.  Passing an injected
    texture loader/sampler permits asset-free geometry regressions.  No fallback
    art is used: an unavailable visible Spine resource is an error.  The shared
    Kaiser actor is intentionally returned as placement metadata for runtime
    compositing of its body/left/right/reaction tracks.  ``plane='back'`` and
    ``plane='front'`` split static commands at that actor's authored draw order,
    so the runtime can place foreground foliage over the shared animation.
    """
    if mode not in ('scene', 'background'):
        raise SceneRenderError('Mode must be scene or background')
    if plane not in ('all', 'back', 'front'):
        raise SceneRenderError('Plane must be all, back or front')
    if not (1 <= width <= 4096 and 1 <= height <= 4096 and 1 <= supersample <= 4):
        raise SceneRenderError('Invalid output dimensions or supersampling')
    if min(viewport) <= 0:
        raise SceneRenderError('Invalid logical viewport')
    if texture_loader is None:
        from build_content import texture as texture_loader
    renderer = _Renderer(pack, width, height, viewport, texture_loader, sampler, sample_time, supersample)
    renderer.phobia_mode = bool(phobia_mode)
    path = path.removeprefix('res://')
    root = renderer.loader.instantiate(path)
    renderer.diagnostics.update(path=path, mode=mode)
    if mode == 'background':
        selected = background_layers(pack, path, variants)
        for choice in selected:
            parent = next((node for node in root.children if node.name == choice['node']), None)
            if parent is None:
                raise SceneRenderError('Original background lacks layer slot ' + choice['node'])
            child = renderer.loader.instantiate(choice['path'])
            # NCombatBackground.AddLayer explicitly makes the selected root visible.
            child.props['visible'] = Property('true', choice['path'])
            parent.children.append(child)
        renderer.diagnostics['layers'] = selected
        title = PurePosixPath(path).parent.name
        scale, dx, dy = camera if camera is not None else _CAMERAS.get(title, (1, 0, 0))
        cx, cy = np.asarray(viewport) / 2
        # NCombatRoom transforms SceneContainer about its center; BgContainer is
        # centered in that node with the authored extra x offset of 23.
        parent_matrix = _translate(cx + dx, cy + dy) @ _scale(scale) @ _translate(23, 0)
        parent_size = np.zeros(2)
        renderer.diagnostics['camera'] = [scale, dx, dy]
    else:
        parent_matrix = np.eye(3)
        parent_size = np.asarray(viewport, dtype=float)
        if path.startswith('scenes/events/background_scenes/'):
            ratio = min(2.3333, max(1.3333, viewport[0] / viewport[1]))
            if ratio < 1.7777:
                t = (ratio - 1.3333) / (1.7777 - 1.3333)
                position = np.array([-140., 110.]) * (1 - t) + np.array([0., 40.]) * t
                scale = 1 * (1 - t) + .89 * t
            else:
                t = (ratio - 1.7777) / (2.3333 - 1.7777)
                position = np.array([0., 40.]) * (1 - t) + np.array([330., 40.]) * t
                scale = .89 * (1 - t) + 1 * t
            pivot = parent_size / 2
            parent_matrix = _translate(*(position + pivot)) @ _scale(scale) @ _translate(*-pivot)
            renderer.diagnostics['ancient_container'] = {'position': position.tolist(), 'scale': scale}
    try:
        if phobia_mode:
            renderer.diagnostics['phobia_layers'] = select_phobia_visuals(root)
        renderer.visit(root, parent_matrix, parent_size, np.ones(4), 0, root.name)
        return renderer.finish(plane)
    finally:
        if renderer.owns_sampler:
            renderer.sampler.close()

