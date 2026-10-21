"""Original static UI components, with their authored tint and nine-patch edges.

Only public resource parameters live here. The caller supplies a decoded image
from the owner's private PCK; no source pixels or localization are embedded.
"""
from __future__ import annotations

import math
import numpy as np
from PIL import Image
from build_content import prepared


def component(image: Image.Image, row: dict) -> Image.Image:
    size = tuple(row['size'])
    if len(size) != 2 or any(type(n) is not int or not 1 <= n <= limit for n, limit in zip(size, (320, 240))):
        raise ValueError('UI component exceeds the calculator image bounds')
    image = image.convert('RGBA')
    if list(image.size) != row['source_size']:
        raise ValueError('Original UI component dimensions changed: ' + row['key'])
    patch = row.get('nine_patch')
    if patch:
        margins = patch['margins']
        scale = patch['scale']
        scales = scale if isinstance(scale, (list, tuple)) else [scale, scale]
        if (len(margins) != 4 or any(type(n) is not int or n < 0 for n in margins) or
                len(scales) != 2 or any(not isinstance(v, (int, float)) or
                    not math.isfinite(v) or not 0 < v <= 1 for v in scales)):
            raise ValueError('Invalid original nine-patch margins or scale')
        left, top, right, bottom = margins
        if left + right >= image.width or top + bottom >= image.height:
            raise ValueError('Nine-patch has no original center region')
        dl, dt, dr, db = [round(n * scales[i % 2]) for i, n in enumerate(margins)]
        if dl + dr >= size[0] or dt + db >= size[1]:
            raise ValueError('Nine-patch cannot retain its borders at this size')
        sx, sy = [0, left, image.width-right, image.width], [0, top, image.height-bottom, image.height]
        dx, dy = [0, dl, size[0]-dr, size[0]], [0, dt, size[1]-db, size[1]]
        result = Image.new('RGBA', size)
        for y in range(3):
            for x in range(3):
                width, height = dx[x+1]-dx[x], dy[y+1]-dy[y]
                if not width or not height:
                    continue
                source = image.crop((sx[x], sy[y], sx[x+1], sy[y+1]))
                if not source.width or not source.height:
                    raise ValueError('Nine-patch cannot stretch an empty edge')
                result.paste(source.resize((width, height), Image.Resampling.LANCZOS), (dx[x], dy[y]))
    else:
        if row.get('fit') not in ('contain', 'cover', 'stretch', 'trim'):
            raise ValueError('Unknown UI component fitting mode')
        result = prepared(image, size, row['fit'])
    color = row.get('modulate', [1, 1, 1, 1])
    if len(color) != 4 or any(not math.isfinite(c) or not 0 <= c <= 1 for c in color):
        raise ValueError('Invalid original UI modulation')
    if color != [1, 1, 1, 1]:
        pixels = np.asarray(result, dtype=np.float64)
        result = Image.fromarray(np.rint(pixels * np.asarray(color)).astype(np.uint8), 'RGBA')
    return result
