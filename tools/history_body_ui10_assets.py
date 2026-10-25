#!/usr/bin/env python3
"""Build append-only original History room icons and their authored outlines."""
from PIL import Image
from patch_feedback_assets import write_image
from world_scene_assets import load_scene_texture

VERSION, BASE_AUX, AUXILIARY = 39, 2068, 2132
ROOMS = (
    'aeonglass_boss', 'ancient', 'ceremonial_beast_boss', 'darv', 'doormaker_boss',
    'elite', 'event', 'kaiser_crab_boss', 'knowledge_demon_boss', 'lagavulin_matriarch_boss',
    'monster', 'neow', 'nonupeipe', 'orobas', 'pael', 'queen_boss', 'rest_site', 'shop',
    'soul_fysh_boss', 'tanx', 'test_subject_boss', 'tezcatara', 'the_insatiable_boss',
    'the_kin_boss', 'treasure', 'unknown_elite', 'unknown_monster', 'unknown_shop',
    'unknown_treasure', 'vakuu', 'vantom_boss', 'waterfall_giant_boss',
)


def build_images(pack, cache, base_aux=BASE_AUX):
    """Return64 rows; the caller combines these with other v39 appended art.

    NMapPointHistoryEntry's64px icon uses0.7 scale. Its90px outline also
    uses0.7 before inheriting its parent's0.7. Both reach13 LCD pixels under
    the1.5 hover. Bake that largest actual display size with alpha intact;
    runtime applies the same bounded9..13px scale without another image pool.
    """
    rows = []
    for room in ROOMS:
        for outline in (False, True):
            suffix = room + ('_outline' if outline else '')
            source = f'images/ui/run_history/{suffix}.png'
            original = load_scene_texture(pack, source).convert('RGBA')
            # TextureRect KEEP_ASPECT_CENTERED; preserve transparent margins.
            image = Image.new('RGBA', (13, 13))
            fitted = original.copy()
            fitted.thumbnail((13, 13), Image.Resampling.LANCZOS)
            image.alpha_composite(fitted, ((13-fitted.width)//2, (13-fitted.height)//2))
            key = 'history-source-' + suffix
            row = write_image(cache, key, image)
            row.update(key=key, auxiliary=base_aux+len(rows), source=source,
                       source_size=list(original.size), source_control_size=[90, 90] if outline else [64, 64],
                       source_scale=[0.7, 0.7] if outline else [0.7], source_hover_scale=1.5,
                       source_filter='linear', bake_filter='lanczos', viewport_scale=[4, 21],
                       size=[13, 13], outline=outline)
            rows.append(row)
    return rows
