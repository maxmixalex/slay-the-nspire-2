#!/usr/bin/env python3
"""Re-sample the original Necrobinder loop with corrected slot-child anchors.

Private inputs and all generated pixels stay in the caller's private directory.
The replacement manifest preserves the existing 96 frames, 8-second clock and
disjoint static/dynamic planes; no extra runtime buffers or animations are added.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

from character_static_layers import decoded, pixels_image
from character_ui7_assets import build_art
from pck import Pack


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pck', type=Path, required=True)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--cache', type=Path, required=True)
    parser.add_argument('--dotnet', default='dotnet')
    args = parser.parse_args()
    result = build_art(Pack(args.pck), args.cache, runtime=args.runtime,
                       dotnet=args.dotnet, characters=('necrobinder',))
    replacements = []
    for index, row in enumerate(result['images']):
        colors, alpha = decoded(row, args.cache)
        path = args.cache/('necrobinder-static.png' if index == 0 else f'necrobinder-delta-{index-1:03d}.png')
        pixels_image(colors, alpha).save(path)
        entry = {'aux': 1504} if index == 0 else {'clip': 1709, 'frame': index-1}
        replacements.append(dict(entry, png=str(path.resolve())))
    manifest = args.cache/'ui12-necrobinder-replacements.json'
    manifest.write_text(json.dumps(replacements, indent=2)+'\n')
    print(json.dumps({'manifest': str(manifest.resolve()), 'images': len(replacements),
                      'frames': 96, 'duration_ms': 8000,
                      'static_layer_report': result['static_layer_reports'][0]}), flush=True)


if __name__ == '__main__':
    main()
