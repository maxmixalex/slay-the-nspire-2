# Stationary Ancient GPU lights and Vakuu sparkles

This extension implements Nonupeipe's five stationary GPU light overlays and
Vakuu's three stationary GPU overlays plus five later sparkle emitters. It
preserves their original scene-child boundaries: Vakuu's two earlier overlays,
then its later third overlay, CPU lights, and finally the five sparkle emitters.
The intervening turbulent speck families and earlier cinder family remain
separate, unfinished work. Nonupeipe's six fog emitters also remain unfinished.

The original false-queen-light texture is1251×1820 with constant white RGB.
Its eight node transforms and particle scale are constant; only their common
10-second alpha envelope changes. The producer reads the original full-size
alpha and bilinearly samples it at each native320×240 pixel center under the
authored transforms. It stores the cropped alpha masks, not another room image.
Rounding to alpha8 has maximum error below0.5/255 at these samples. The original
source scene and decoded texture hashes are mandatory inputs.

The two original GPU gradient textures are kept as256-sample byte tables and
sampled with clamped bilinear texture coordinates at runtime. Light preprocessing
is2seconds; sparkle preprocessing is5seconds, lifetime5seconds, with original
20/40/20/20/20 counts, sphere-volume emission geometry, transforms, scale range,
color and normal blend. The source engine's random history is unavailable;
a deterministic cosmetic seed supplies reproducible samples and never advances
the saved gameplay RNG. Godot4.5.1's sphere-volume formula is the numerical
reference. This is not a claim of identical original GPU random histories.

All ten views occupy192,978 decoded bytes /52existing4KiB image-cache pages.
The current existing-format grouping has two groups, largest130,780decodedbytes,
and25,692storedbytes before directory/auxiliary overhead. These are measured
isolated data results, not final installed-package accounting. No persistent
particle array, framebuffer, image-cache capacity or heap allocation is added.

The reduced profile fixes both elapsed clocks at zero and uses the existing
complete Ancient still-scene cache. Its background and envelopes stay fixed
while dialogue and HUD continue normally. Full mode uses room-relative elapsed
time. The reserved typed endpoint is7221auxiliaries/59714images after the
preceding7220/59704 material block; its explicit leaf references must be included
in the final combined liveness/reducer/content guards before publication.

The isolated numeric and source-bound producer tests are complete. Integrated
Content/reducer guards, actual App pixels, complete final installed bytes,
production allocation and guest cost remain acceptance gates. This document
does not close the remaining fog, turbulent-speck or cinder families, and does
not claim physical-calculator testing.
