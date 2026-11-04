"""Exact cumulative Ancient catalog endpoints and prefix validation.

Each authored module continues to validate its own complete prefix. A larger
catalog is admitted only by this explicit registry, never by a >= count test.
The image callback still decodes the actual final file; no payload is copied,
substituted, or hidden from a validator.
"""

from __future__ import annotations

ENDPOINTS = {
    7212: 58613,
    7214: 59667,
    7215: 59674,
    7217: 59677,
    7220: 59700,
    7221: 59710,
    7222: 59712,
    7223: 59717,
    7224: 59729,
    7225: 59730,
    7226: 59731,
    7227: 59732,
    7228: 59741,
    7229: 60294,
    7237: 60302,
}


def catalog(source):
    count = len(source.aux)
    if ENDPOINTS.get(count) != len(source.images):
        raise ValueError("Unknown or incomplete Ancient typed catalog endpoint")
    return count


class Prefix:
    def __init__(self, source, count):
        self._source = source
        self.aux = source.aux[:count]
        self.images = source.images[: ENDPOINTS[count]]

    def __getattr__(self, name):
        return getattr(self._source, name)


def validate(source, image, *, final=False):
    count = catalog(source)
    from nspire_converter.ancient_fire import validate as fire
    from nspire_converter.ancient_gpu_masks import validate as masks
    from nspire_converter.ancient_lights import validate as lights
    from nspire_converter.ancient_materials import validate as materials
    from nspire_converter.ancient_turbulence import validate as trajectories
    from nspire_converter.character_unlock_assets import validate as character_locks
    from nspire_converter.neow_fire import validate as neow_fire
    from nspire_converter.neow_particles import validate as neow_particles
    from nspire_converter.neow_post_assets import validate as post
    from nspire_converter.nonu_fog import validate as fog
    from nspire_converter.tanx_scene import validate as tanx
    from nspire_converter.tez_cinders import validate as cinders
    from nspire_converter.vakuu_cinders import validate as vakuu

    roots = set()
    for first, end, validator in (
        (59667, 7215, tanx),
        (59674, 7217, lights),
        (59677, 7220, materials),
        (59700, 7221, masks),
        (59710, 7222, cinders),
        (59712, 7223, fog),
        (59717, 7224, trajectories),
        (59729, 7225, post),
        (59730, 7226, vakuu),
        (59731, 7227, neow_particles),
        (59732, 7228, fire),
        (59741, 7229, neow_fire),
    ):
        if count < end:
            continue  # count is already one exact registered endpoint.
        prefix = Prefix(source, end)
        if end in (7220, 7225, 7229):
            validator(prefix, image, final=final)
        else:
            validator(prefix, image)
        roots.update(range(first, ENDPOINTS[end]))
    if count == 7237:
        roots.update(character_locks(source, image))
    return roots
