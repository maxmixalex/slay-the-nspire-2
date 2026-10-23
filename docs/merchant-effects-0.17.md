# Merchant focus and candle extension

## Source and implementation

This source stage targets the existing pinned **v0.111.0 / 41cef1ea** owner
reference. It appends a merchant focus skin and the visible flat candle flames
to private NSP2/18 resources. It does not replace the previously executed v18
delivery or its validation records. Conversion and native execution results
are recorded below; target execution has a separate receipt. A syntax check
is not any of those execution gates.

The reference assembly SHA-256 is
`0861bfa1df347538d932f22d580e75420f08082792eb914e53b4882764acdbe9`.
The owner PCK SHA-256 is
`c60f672ee7804e6aefa1e19a582fa1c80b126a7b0eef4d084d3abf110df2eab7`.
The converter verifies the latter before reading source assets. Original
assembly, scene dumps, textures, samples and generated packs stay private.

The assembly's `NMerchantButton.RefreshFocus` (`0x06007d3d`) selects the
`outline` skin for ordinary focus and resets slots to their setup attachments.
`OnUnfocus` (`0x06007d3a`) restores `default`. The ready callback
(`0x0600c31d`) starts `idle_loop`; focus changes do not restart its animation.
The corner reticle belongs to the separate targeting-selection branch, so v19
ordinary focus draws the original outline skin instead. The actual merchant
body remains the only shop-opening hit target. The Proceed artwork is retained.

The original top skeleton contains fourteen default attachments and eleven
outline attachments. A selected skin resolves each attachment from itself,
then falls back to the default skin. Its deform timeline must come from that
same owning skin. `animation.geometry(..., skin=...)` implements those two
rules; merging attachment dictionaries while keeping skin-zero deforms would
produce the wrong animated mesh. The default converter path remains skin zero.
The synthetic converter regression exercises both an overridden attachment and
a default fallback with deliberately conflicting deform tracks.

The focused top uses the same scene transform, camera and 145 sample times as
the existing v18 top. Both skins retain their 41 ms directory entry, but runtime
selects a sample against the authored 6,000 ms idle duration. This avoids the
5,945 ms loop caused by multiplying the rounded entry by 145. The bottom idle
shares that duration. All use the existing real presentation clock, so changing
focus or enabling Fast does not restart or double the animation.

## Candle material and layers

The reference is `scenes/rooms/merchant_room.tscn`, its visible
`SteppedFireMix` nodes and `shaders/vfx/vfx_stepped_shader_fire_flat.tres`.
`merchant_effects.py` reads the authored transforms, texture references and
material uniforms. It evaluates the shader's two panning noise samples,
inverted noise mask, two distortion samples, distortion mask, base density,
inner color step and outer alpha step. Sampling uses linear texel centers and
the material's repeat-enabled noise / repeat-disabled mask modes. Conversion
uses four samples per output axis and premultiplied source-over downsampling.
Texture/material equations come from the pinned input; no descriptive image
generation or hand-painted substitute is used.

The left candle follows the animated `left_candle_flame` bone. Its
`SpineBoneNode.show_behind_parent` is true, while the other three visible
flames follow the bottom skeleton in scene order. The runtime order is:

1. Registered room background.
2. Left bone-attached flame.
3. Bottom merchant skeleton.
4. Three foreground flames.
5. Existing class shop actor and ordinary/focused top merchant skeleton.

The four additive fire sprites are explicitly hidden in the original scene
and remain hidden. GPU cinder particles, additive light particles and the
Necrobinder's separate flame shaders are still outside this slice.

The two flame clips sample 150 frames at 40 ms over `[0, 6)` seconds and repeat.
The bone pose is six-second periodic; shader noise driven by unrestricted
`TIME` is not inherently six-second periodic. This is a finite offline loop,
not a claim to reproduce indefinitely advancing GPU noise or all original
particles. Its loop boundary and material/source hashes are recorded in the
private conversion manifest. Actual output requires visual review at 320×218,
including the transition from the last sample to the first.

## Catalog, storage and memory boundaries

| Profile | Auxiliary entries | Animation clips | New clip indices |
| --- | ---: | ---: | --- |
| Preserved NSP2/18 | 1,262 | 1,702 | Existing indices unchanged |
| NSP2/19 | 1,262 | 1,705 | 1702 focus; 1703 behind flame; 1704 front flames |

The private patcher accepts only the exact known v18 input profile and appends
445 images and three clips. It preserves all old directory indices, animation
entries and encoded pixels. The new 7,168-byte directory growth relocates
offsets without rebuilding historical art. New art is inserted immediately
before the glyph/text suffix in the same order for both languages; the common
resource therefore contains it once.

The converter imposes a **2 MiB encoded-art ceiling**. The focus crop is at most
128×128; flame crops remain within the 320×218 scene. Every new decoded frame
must fit the existing 614,880-byte image limit. The converter reports actual
crop dimensions, encoded bytes and maximum decoded image bytes; they are not
assumed from these upper bounds. No resident image or directory capacity is
expanded: 748 image pages, 320 image entries and the 2 MiB directory remain.
ARM allocation must still pass the existing 16 MiB gate after integration.

`Content` validates the exact known 19 clip/auxiliary counts and rejects future
versions. The v18 catalog and visual fallback remain supported. Full integrity,
structure validation and bounded decode checks are unchanged. Split containers,
boot language probing and UI-template inspection accept 19; their unknown
version rejection is tested. `build_web.py` consumes the shared split reader.
The localization builder intentionally remains v16→v17, followed by the
append-only feedback v17→v18 and merchant v18→v19 stages.

## Required execution gates

`tests/test_merchant_conversion.py` provides small public skin/deform and
material sampling tests without original data. `merchant_visual_regression`
is a CMake target invoked with an actual v19 pack, its v18 predecessor and an
optional private screenshot directory. It checks the exact clip profile,
five-character room interactions, stable focus phase, pointer departure,
keyboard/pointer skin agreement, visible animation, six-second sample bounds,
unchanged canonical saves, bounded caches, malformed catalog rejection and
the older pack fallback. It is not registered as a resource-free CTest case.

Long conversion and execution must run from a pushed, read-back source
checkpoint using `tools/run_logged.py`. The conversion receipt must prove
historical payload preservation and new-image decoding; the paired split gate
must reconstruct both original full packs byte for byte. Native screenshots,
integrated Web and target ARM results must retain their own execution identity.
Existing v18 Firebird startup samples do not measure v19 resource loading.

## Executed conversion and native validation

Execution uses the pushed and read-back checkpoint
`06c7fd16e6d4d47dc25b628913ecdeef3a4fe76d`. The metadata-only companion
`validation-merchant-effects-0.17.json` records source identities, complete
conversion/split hashes and thirteen closed journals. Twelve passed; the
initial shader run failed before Chromium startup because local process IPC
was denied by the sandbox. A standard approved local retry ran the same
offline helper successfully. That failed attempt remains in the record.

Both language conversions passed all 445 new-image decode roundtrips and
historical payload, clip, card, auxiliary, glyph and text relocation checks.
The real native merchant suite passed **1,402 checks per language**, including
five characters, pointer and keyboard focus, phase continuity, unknown/bad
profiles, v18 fallback and canonical save preservation. The Python suites
passed three skin/material tests, five existing animation geometry tests,
twenty-four tile conversion tests and three split conversion/rejection tests.

| Measured new resource | Encoded bytes, with alignment | Crop |
| --- | ---: | --- |
| Focused merchant, 145 frames | 470,994 | 58×71 |
| Left flame, 150 frames | 9,048 | 11×9 |
| Front flames, 150 frames | 55,098 | 58×130 |
| Total new common art | **535,140** | — |

Maximum new decoded image size is **4,732 bytes**. Directory growth is exactly
7,168 bytes per language; neither resident directory nor image-cache capacity
changed. The v19 common file is 87,405,187 bytes, the English language file
1,801,249 and Chinese 4,576,927. All three total **93,783,363 bytes**, which is
549,476 bytes above the delivered v18 three-file set. The split converter and
the C++ streamed reader both reproduced each full input byte for byte.

The independent shader check compiled the pinned generated fragment graph as
WebGL2 under Chromium 151.0.7922.34 / ANGLE SwiftShader, with the authored
uniforms and textures. At 0, 1.24, 5.96 and 6 seconds, each 128×128 UV grid
agreed with the offline evaluator to a maximum **0.500093 of an 8-bit channel**;
mean error per channel was below 0.0234. This is consistent with conversion to
an 8-bit render target. It checks the material graph, not an original Godot
frame, color/HDR pipeline, particle simulation or complete scene transform.

Loop-boundary differences were measured after decoding the actual RGB565 and
alpha frames, using summed absolute premultiplied RGBA changes. The focus
last-to-first change is 0.563 times the largest ordinary adjacent-frame
change; the rear flame is 0.467 times and the front flames 0.998 times. The
front-flame seam is about 1.55 times its average adjacent-frame change. At
this resolution the boundary is within the observed normal step range; this
does not make the finite six-second noise loop equivalent to unrestricted
shader `TIME`. Native 320×240 focus/unfocus and candle screenshots were
visually inspected; private animation previews preserve the sampled cycle.

Integrated ARM/WASM allocation, browser behavior and Firebird startup,
Continue/save/exit tests belong to their platform records. In particular, the
earlier 61970600 v18 target measurements cannot be relabelled as v19 evidence.
