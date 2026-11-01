# Continuous card Afflictions

The ACU1 producer replaces the existing 56 auxiliary pictures at 5202–5257.
Image IDs, the 60,294-image catalog,7,229 auxiliaries,1,783 gameplay clips and
every clip clock remain unchanged. Older packs still use their eight-phase
compatibility path. Production `card_face_animated_layers` and the rotated
`hand_card_animated_layers` both select the new renderer for ACU1 data.

Nine source materials implement Bound border/main, Entangled, Galvanized,
Hexed, Tainted, Ringing and the two Smog layers. They evaluate the presentation
clock directly with fixed-point texture sampling, curve tables and bounded
integer polar coordinates. They have no sampled phase selector. The source
Infection 30-frame and Ancient-card 10-frame flipbooks are unrelated and remain
unchanged.

The source scenes contain 55 visible particles:40 specks,11 leaves and 4
lightning particles. The desktop converter evaluates the pinned Godot 4.5.1
process equations, emission shapes, lifetime/scale/color curves, friction and
curl turbulence. The calculator samples the original 30/60 Hz process tracks;
leaf shader motion continues between the stationary particle poses. This is a
deterministic seed bank. The original GPU's unrecorded random history and
arbitrary world-space emission history are not recovered.

## Precision and bounds

Visible material color/alpha texels retain their source dimensions and bytes.
The one reduced field is the 1024×1024 blue channel that drives the vine's
horizontal displacement. It becomes 256×256. The full union of original and
replacement bilinear cell corners contains 1,643,524 points; its measured
maximum normalized difference is 0.0112132431. With the source .018 displacement,
the largest 156×218 overlay and the authored 422 px/1.2 scale quad, the resulting
edge displacement is bounded by 0.04922 LCD pixels. This reduces decoded fields
by 984,960 bytes. It does not blur card art or creature outlines.

Particle inverse transforms keep Q12 coefficients in the existing Q20 wire
representation. The converter bounds every frame through its forward quad;
the largest resulting LCD displacement is 0.02526 pixels. Color/alpha tracks
remain byte samples. The runtime uses four additional fractional bits while
stepping rotated quads and performs no floating-point work in the raster loop.

The private source comparison checks nine materials at five non-phase-aligned
times, and the three particle fragments at four time/instance offsets. Across
these 57 cases the largest mean premultiplied/alpha difference from the source
equations is 0.1254 of an 8-bit code; the largest 99.9 th-percentile difference is
2.155 codes. Rare vine edge pixels can differ more, consistent with the stated
subpixel displacement bound. This is bounded LCD approximation, not a claim of
bit-identical full-resolution desktop shaders.

## Storage and memory

The table below records the historical private candidate. Its 47,872-byte
logical increase is not the final producer's result. The official bad0168
producer's `card-afflictions46-eng` stage changes the logical pack from
102,799,238 to 102,852,818 bytes: **+53,580 bytes**. After all later storage and
publication stages, the 11 required English/Chinese data files total
**68,155,584 bytes**, versus the preceding completed UI23 data's 68,100,890:
**+54,694 bytes**. Those figures have different transformation boundaries and
must not be conflated or described as an overall shrink. Final edition totals
also include the matching launcher, instructions and licenses.

The official `storage-attribution.json` records these boundaries. Its later
bounded actor color/alpha step is lossy; exact tile coalescing, slot compaction,
sparse transparent-domain encoding and literal-context retuning are separately
lossless relative to their inputs. The Entangled blue-displacement field below
is the same source texture reused by Entangled and Tainted, not two distinct
resized textures.

| Item | Bytes |
| --- | ---: |
| Old 28 groups touched by the 56 phase pictures | 504,695 |
| Of those, two groups also used by retained adjacent art | 56,497 |
| Old groups wholly reclaimed | 448,198 |
| New sparse source fields, decoded | 3,178,568 |
| New curves, decoded | 30,720 |
| New particle tracks, decoded | 194,522 |
| New static decorations, sparse decoded | 713,203 |
| New typed table including padding | 7,168 |
| New complete packed replacement, including 29 groups/descriptors | 511,254 |
| Historical private candidate: logical increase after retained-block rewrite | 47,872 |

The bank occupies 26 pages, each at most 131,200 decoded bytes. The largest
group is 256,264 decoded bytes, below the existing 256 KiB group target and the
reader's existing limit. There is no new persistent frame or particle pool.

A combat hand containing three or more distinct Affliction families borrows
the existing 8.5 MiB image /4.25 MiB tile configuration. The total picture arena
stays 12.75 MiB. Once selected, this profile remains active until leaving combat,
so drawing or playing cards across the three-family threshold does not repeatedly
clear caches. Other combats retain the ordinary 6.75 MiB image /6 MiB tile split.
Every family requests only its required field/curve/track pages and static
decorations. All requests finish before the renderer borrows cache pointers;
it then checks residency and makes no Content request inside the pixel loop.

The real App range-reader probe exercises all seven families in one hand.
After 12 seconds covering the idle loops,120 successive frames show 120 changes in the hand region as well as in the full
framebuffer with zero source calls and zero source bytes, using 1,917 of 2,176
image pages and 278 entries. A host run measured approximately 5–6.3 ms per frame;
that is unpaced host CPU timing, not calculator FPS.

The accepted f760 combat renderer, unchanged by the final d935 Ancient-option
optimization, has actual ordinary 64 MiB Firebird paired measurements with the
explicit small3 TEST ONLY pack: three families produce 18.3430 FPS / 54.0973 ms
render over 551 frames; seven produce 9.43565 FPS / 105.6172 ms over 283 frames.
Both have zero warm source calls and bytes. Exact shared field lookup, opaque
card-face occlusion and reciprocal smoothstep division improve the earlier
same-fixture 10.806 / 4.988 FPS results while preserving whole-frame pixels.
These are emulator stress results, not physical-device performance or a final
full-package guest claim. Current full/reduced package acceptance belongs to
the delivery report; see [runtime evidence](ui23-runtime-performance.md).

## Reproducible entered-game fixture

Open the always-present developer console with Flag. Enter these commands in
order, then close the console with Flag. The commands edit a temporary run and
do not autosave it.

```text
new Ironclad 73
fight BOWLBUGS_WEAK
card BASH Hand
card BASH Hand
afflict BOUND 1 0
afflict ENTANGLED 1 1
afflict GALVANIZED 1 2
afflict HEXED 1 3
afflict TAINTED 1 4
afflict RINGING 1 5
afflict SMOG 1 6
```

The optional native acceptance executable is `card_affliction_render_probe`.
It takes a private logical NSP path and an optional screenshot directory,
creates this fixture through the actual console command handler and reports
warm source I/O, cache use and host frame time. The normal renderer is used.

`--no-animations` retains this card/UI presentation clock. It only reduces the
declared actor/scenery loops and keeps necessary action poses at their original
times. Continuous card materials are also required by the converter's final
installation validation, so a missing card stage cannot silently publish an
older eight-phase pack.
