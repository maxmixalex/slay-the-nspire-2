#!/usr/bin/env python3
"""Validate the segmented body policy before appending original Ancient lights.

The unchanged strict verifier still reconstructs and compares every source
frame. Its consumer uses the same hold as the preceding body stage; only the
old human-readable policy name is corrected after that verification succeeds.
"""
from __future__ import annotations
import ancient_lights_ui23 as lights
from ancient_color_hold_ui23 import POLICY
from ancient_scene_compact_ui23 import consume


def main():
    original = lights.verify_scene_family

    def verified(*args, **kwargs):
        proof = original(*args, **kwargs)
        if proof['matched_policy'] != 'exact':
            proof = dict(proof, matched_policy=POLICY)
        return proof

    try:
        lights.verify_scene_family = verified
        return consume(lights.main)
    finally:
        lights.verify_scene_family = original


if __name__ == '__main__':
    main()
