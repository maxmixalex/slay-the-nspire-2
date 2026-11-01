#!/usr/bin/env python3
"""Consume verified source poses with the bounded temporal-segment color hold.

The sampler and its seven dependency files remain unchanged. Only an explicit
--bounded-color-hold request selects this post-sampling consumer policy. The
exact/reduced path and all original sampling/cache validation remain intact.
"""
from __future__ import annotations
import ancient_scene_ui23 as scenes
from ancient_color_hold_ui23 import held_colors


def consume(main):
    original = scenes.held_colors
    try:
        scenes.held_colors = held_colors
        return main()
    finally:
        scenes.held_colors = original


def main():
    return consume(scenes.main)


if __name__ == '__main__':
    main()
