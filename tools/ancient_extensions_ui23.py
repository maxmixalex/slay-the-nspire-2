"""Exact cumulative Ancient catalog endpoints and prefix validation.

Each authored module continues to validate its own complete prefix. A larger
catalog is admitted only by this explicit registry, never by a >= count test.
The image callback still decodes the actual final file; no payload is copied,
substituted, or hidden from a validator.
"""
from __future__ import annotations

ENDPOINTS={7212:58613,7214:59667,7215:59674,7217:59677,
           7220:59700,7221:59710,7222:59712,7223:59717,7224:59729,
           7225:59730,7226:59731,7227:59732}


def catalog(source):
    count=len(source.aux)
    if ENDPOINTS.get(count)!=len(source.images):
        raise ValueError('Unknown or incomplete Ancient typed catalog endpoint')
    return count


class Prefix:
    def __init__(self,source,count):
        self._source=source
        self.aux=source.aux[:count]
        self.images=source.images[:ENDPOINTS[count]]

    def __getattr__(self,name):return getattr(self._source,name)


def validate(source,image,*,final=False):
    count=catalog(source)
    from tanx_scene_ui23 import validate as tanx
    from ancient_lights_ui23 import validate as lights
    from ancient_materials_ui23 import validate as materials
    from ancient_gpu_masks_ui23 import validate as masks
    from tez_cinders_ui23 import validate as cinders
    from nonu_fog_ui23 import validate as fog
    from ancient_turbulence_ui23 import validate as trajectories
    from neow_post_assets_ui23 import validate as post
    from vakuu_cinders_ui23 import validate as vakuu
    from neow_particles_ui23 import validate as neow_particles
    roots=set()
    for first,end,validator in ((59667,7215,tanx),(59674,7217,lights),
            (59677,7220,materials),(59700,7221,masks),(59710,7222,cinders),
            (59712,7223,fog),(59717,7224,trajectories),(59729,7225,post),
            (59730,7226,vakuu),(59731,7227,neow_particles)):
        if count<end:continue  # count is already one exact registered endpoint.
        prefix=Prefix(source,end)
        if end in (7220,7225):validator(prefix,image,final=final)
        else:validator(prefix,image)
        roots.update(range(first,ENDPOINTS[end]))
    return roots
