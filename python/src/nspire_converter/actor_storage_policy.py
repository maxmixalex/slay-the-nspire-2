"""Storage boundaries for actor materials with separately accepted precision.

The Necrobinder attack includes the authored translucent slash. Its sampled
RGB565/alpha8 output and small retained decode group are part of that material's
accepted representation, rather than the ordinary animated-actor color policy.
"""

from nspire_converter import content_tiles as t

ACTOR_AUXILIARY_BASE = 5422
ACTOR_AUXILIARY_END = 7206
EXACT_MATERIAL_CLIPS = (28,)  # Necrobinder attack, including its slash shader.


def exact_material_images(source):
    """Include ordinary samples and every registered overflow piece.

    Earlier resource versions have no native actor tables and retain their existing
    policy. Once that catalog is present, a missing or malformed material table is
    an error instead of silently losing the precision/storage exception.
    """
    if len(source.aux) < ACTOR_AUXILIARY_END:
        return set()
    from nspire_converter.retired_assets import framed_references

    images = set()
    for clip in EXACT_MATERIAL_CLIPS:
        t.require(clip < len(source.clips), "Exact actor material clip is absent")
        first, count, *_ = source.clips[clip]
        t.require(
            count > 0 and 0 <= first < first + count <= len(source.images),
            "Exact actor material sample range is invalid",
        )
        images.update(range(first, first + count))
        table = source.aux[ACTOR_AUXILIARY_BASE + clip]
        t.require(0 < table < len(source.images), "Exact actor material table is absent")
        _width, _height, fmt, raw = source.image_raw(table)
        t.require(fmt == 0, "Exact actor material table must contain opaque words")
        images.update(framed_references(raw))
    t.require(
        all(0 <= image < len(source.images) for image in images),
        "Exact actor material piece is outside the image catalog",
    )
    return images


def verify_exact_materials(source, target, images):
    """Check the actual decoded samples and retained-versus-tiled cache path."""
    for image in sorted(images):
        t.require(
            source.images[image][3] == target.images[image][3]
            and source.image_raw(image) == target.image_raw(image),
            "Exact actor material pixels or storage representation changed",
        )
    return dict(images=len(images), decoded_pixels_exact=True, storage_formats_exact=True)
