# UI23 native Ancient materials

This stage adds the four visible Pael light plates, the two visible Orobas light
plates, and the visible Orobas and Tanx water materials from the pinned original
game. It does not activate explicitly hidden source nodes. The additional GPU
emitters and Neow screen-reflection post are separate stages with their own
ordering and acceptance requirements.

## Source and catalog

The producer verifies the original scenes, shaders, material, decoded texture
dimensions and decoded pixel hashes. All six light plates use the source
additive blend. Their native LCD projection, tint, source alpha, ten-second
lifetime, two-second preprocessing and complete 256-sample alpha ramps remain
explicit. The producer does not replace their colored source textures with
white alpha masks.

The input endpoint is exactly 7,217 auxiliary entries and 59,677 images. Three
typed roots and 23 image leaves produce endpoint 7,220 / 59,700. Every leaf is
reachable through those roots; schema checks retain fixed dimensions, formats,
source-base references, noise hashes, ramp bytes and zero padding. Final release
validation also binds the material profile to the NAF1 header marker.

## Full animation representation

Orobas uses a 960×720 sampler split into nine 320×240 leaves, with the complete
original 50×50 R8 procedural-noise texture. Tanx uses five 320×240 shader response
images and the reviewed 250×250, four-bit noise representation. This preserves
the native output dimensions and material motion; it is a bounded source
sampling policy, not a claim that every original shader sample is identical.

Sampler colors differ by at most one red/blue and two green RGB565 levels from
their sampled source colors. The measured Tanx projected shader comparison,
which also includes five-response interpolation and the noise representation,
reached channel differences [3, 4, 2] over the reviewed twelve clock positions.
Its native 320×240 comparison was reviewed separately. The original texture has
no mip levels; an unprovided source mip chain is not used to justify downscaling.

The retained image format stores modular RGB565 residuals. On a cold request,
`Content::material_image` reconstructs the image in its existing cache pages.
Orobas predicts from the original native room image. Tanx reconstructs endpoints
from that room image, then the middle and quarter responses from explicit
parents. A bounded dependency tree has no cycles. Subsequent image requests
return the reconstructed image without repeating the residual arithmetic.

The reconstructed marker uses one previously unused bit in the existing cache
entry. These fourteen full-profile leaves use image identity rather than payload
alias identity: equal residual bytes can predict different pixels. Ordinary
images retain normal alias sharing. Cache discard, publication, owner changes
and content reopen clear the marker through the existing cache lifetime rules.
No new framebuffer, image arena, particle pool or cache capacity is introduced.

The shader clock is the existing continuous visual clock, with the source
3,600-second TIME wrap. Particle opacity uses room elapsed time plus source
preprocessing. The original engine's historical global epoch is unknown; this
implementation does not claim to recover it.

## Reduced animation representation

The original-folder converter separately samples both water shaders at explicit
source clock zero using their original texture and noise pixels, with no full
profile color/noise approximation. All nine Orobas leaves alias that room's
exact native pose; all five Tanx leaves alias its pose. The catalog and roots
remain fixed, while the cache shares each room's one payload.

Static mode is recorded in the typed parameters and must match NAF1 at final
publication. The general pose reducer rejects animated material samplers: an
approximate sampler cannot be relabeled as an exact source pose. The converter
therefore runs the source material stage with `--no-animations` before the
normal final pose reduction and profile marking.

## Verification and remaining release gates

Focused tests exercise source parameter contracts, profile mismatch rejection,
bounded residual colors, exact reduced source sampling, cache alias separation,
all cache chain links, discard/republication and independent scalar pixel
reconstruction. Actual original-source candidates have also been reconstructed
by native C++ and compared by complete image hashes.

The measured full candidate's retained groups occupy 119,948 compressed bytes;
the exact static candidate occupies 124,306 bytes before final resource
retuning. These are candidate group totals, not the final installed increment.
Maximum decoded groups are 258,048 and 261,650 bytes respectively, within the
existing 256 KiB group bound. The static payloads deduplicate to one pose per
water room despite the unchanged fourteen leaf descriptors.

Production ARM allocation, complete App cold/warm costs, guest performance and
the final extracted installation size remain mandatory release gates. Isolated
host timing and unchanged capacity alone do not establish those results.
