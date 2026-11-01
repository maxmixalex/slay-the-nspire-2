# Nonupeipe source fog candidate

This isolated extension implements the six authored `fog3`–`fog8` layers before
Nonupeipe's five stationary light layers. The sole opaque TextureRect precedes
all these nodes in the pinned original scene. It adds no whole-room raster,
particle pool, persistent buffer, image scaling, or animation-frame sampling.

The source has 44 particles across six emitters (5, 5, 7, 7, 10, 10), with 15 or
30-second lifetimes and 100 seconds of preprocessing. The four 200×200 atlas
frames retain every original alpha byte. Their source RGB is exactly white,
including transparent texels; the runtime applies the original color/ramp and
node alpha in additive source order. The signed parent scales are applied
after particle rotation, including the source horizontal reflections.

The extension consumes auxiliary 7222 and images 59712–59716, following the
Tez cinder endpoint 7222/59712. Its endpoint is 7223/59717. Four alpha images
use existing format0 byte planes; a 2,048-byte typed parameter image holds
source-derived 256-sample angle, scale, and grayscale lookup grids. The
retained codec and existing picture cache require no new format or capacity.

The runtime uses a deterministic presentation seed and the closed-form sum
of semi-implicit 60 Hz constant gravity. The original nodes use `fixed_fps=0`,
so desktop integration follows its actual frame deltas. This implementation
does **not** claim the original GPU random history or identical variable-step
trajectories. Curves, lifetime, preprocessing, atlas selection, node geometry,
and draw order remain authored inputs. Reduced mode uses clock zero and the
existing completed-scene cache; it does not run or replay hidden particle state.

Focused source checks cover original scene/decoded texture identities,
parameter bounds, deterministic wide-clock samples, an independent floating
gravity sum, and clamped source bilinear alpha. The initial numeric fixture
passed 306,300 checks with maximum measured alpha sampling error
0.682538293/255. The two-group exact asset payload is 44,006 bytes before
catalog/directory overhead, with maximum decoded group 120,000 bytes.

**Acceptance is pending:** full production Canvas comparison against a source
reference, full/reduced actual scene behavior, clipped blend work and guest
cost, final ARM allocation, combined physical installation budget, and strict
catalog/liveness/reducer integration. This source checkpoint is not a claim
that all Nonupeipe/Vakuu particle families or the final release are complete.
