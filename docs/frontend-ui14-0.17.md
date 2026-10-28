# UI14 front-end corrections

Reference: the owner's pinned **v0.111.0 / 41cef1ea** assembly and PCK.
This report describes the front-end work, separate from combat, rooms, and
the new installation layout. Original source data and captured images remain
private. Pixel-oracle counts are assertions, not feature-coverage percentages.

The first complete front-end actual-resource gates close at
`0418cf3e63dabb456e4a9aaa49062d32f7ac747c` with the final 5,275-entry artwork
catalog. Earlier failed builds and oracles below are retained as intermediate
results, not current blockers. The later ordinary-OS device measurements below
found remaining performance work despite these native correctness passes.

## Source behavior and changes

### Credits

`NCreditsScreen._Process` continuously moves its target upward at the authored
speed and eases the content toward that target. It has no end-of-content close
condition. The original waits for a click or Back after its initial close lock,
then fades out. The port keeps that behavior rather than returning to Settings
automatically after the final image.

The Nspire port credit now uses the same green major-section style as the
source localization/trailer headings, with the bold section face. The author
line remains a separate cream-colored line at the Trailer → FMOD boundary.

### Bestiary and scrolling controls

The right catalog measures each localized label and reserves a compact 14px
one-line row or the required two-line height. Region headings have their own
18px band. Row offsets remain in the shared source coordinate system, and the
selection arrow targets the measured row center. The action list on the left
also measures its labels instead of separating every entry by the previous
fixed 26px pitch. Every source move remains reachable through the same paging
and keyboard actions.

`NScrollbarTrain.OnMouseEntered` immediately applies 1.15× scale; its
`OnMouseExited` uses a 500ms Expo-Out tween back to one. The shared
`native_scrollbar_grabber` supplies that centered hover/press scale
and recovery to deck/collection grids, bestiary, progress,
settings, run setup, room text/rewards, character descriptions, and the run-end
summary. The settings language thumb keeps its own original small train
texture and uses the same motion. Its track, drag range and hit targets retain
their existing meaning. The separate FTUE reader follows this motion in the
room/text follow-up.

### Deck controls and run setup

The deck's upgrade control occupies a reserved row above the footer, to the
right of the visible Back tab. Its card grid ends above that row. The sorter
labels use the source bold face, gold ink and separate dark shadow; their full
localized strings use the shared smooth overflow painter.

Custom-run rich text now clips long ascension descriptions to the actual
description rectangle and scrolls by pixels. Short text retains its original
vertical centering. The fallback confirmation paragraph uses the same complete
bounded-text path instead of showing just its first three lines.

### Random selection and departures

The random portrait uses its source 132:196 aspect ratio. The random background
retains the complete water material, its full gradient/noise textures, the
authored light, and all 40 decorative particles. Its renderer now shares the
five noise rows' horizontal interpolation across the 240 display rows and
precomputes their 256 possible displacement values per frame. RGB565 bilinear
interpolation uses separated packed channels with the same rounding.

The original light is opaque and the original dot is RGB white. Dedicated
color-only and alpha-only sampling avoids fetching/reconstructing unused
channels. On the ordinary opaque display, the light's tint and additive alpha
are small precomputed channel maps, and the dot's tint is constant. Parent
modulation retains the general painter path. The converter rejects changes to
either source texture contract. No emitter, sprite, source texel, or animation
clock was removed to make the background faster.

`NMainMenu.OnContinueButtonPressedAsync` uses the saved character's transition
material and an 800ms FadeOut before loading the saved room, then FadeIn. The
port's Continue action now shares the departure presentation used by Embark.
The resumed snapshot never passes through the new-run initializer. Inputs are
locked throughout the transition; the saved game and RNG remain unchanged.
The source Regent and Defect materials both reference Ironclad's transition
image, so three source masks correctly cover all five characters.

The transition also restores the parallel quadratic black fade during the
texture wipe and the cubic fade-in. Indexed masks use a small threshold table;
the ordinary frame is shaded directly in RGB565. The mask is prepared before
the timed wipe. When a continued combat needs pose preparation, integration
holds the fade-in clock until that preparation finishes.

### Defeat and summary

The main action is 112px wide, and score rows use a centered 216px column with
22px row bodies and 24px pitch. The death title is centered on the ribbon's
actual middle rather than its lower edge. Score icons are rebuilt at 16×18,
and badges/discovery icons at 24×24, from their original source images; no
rendered icon needs to upscale the previous 8–13px payload. The same existing
31 auxiliary IDs are replaced, preserving no obsolete resource variant.

The discovery tooltip's first-ten-items rule and literal `....` are original
`NRunSummary` content: its method takes ten titles and appends that marker when
the category has more. This is separate from the port's former label clipping.

## Executed validation

The second source checkpoint is
`8c5593f273d5bf8778d00fe329b4e09d4f7b7246`. Its frozen source was built using
Clang 18 and Ninja. The focused synthetic/native gates passed for departure,
random shader/particles, bestiary, frontend save lifecycle, game over, input,
button motion, and credits. The first CTest launch reported one input-test
executable without its execute bit; restoring the locally generated ELF's
mode and rerunning that test passed without a source change.

Owner-resource gates on that checkpoint passed:

| Gate | Result |
| --- | --- |
| ENG and ZHS bestiary, real glyphs and supplemental actors | Passed; actual captures inspected for catalog/action spacing and arrow placement |
| Random material and all emitters | Passed independent floating-point shader samples at four full-frame clocks, repeated-frame checks and unchanged canonical save |
| ENG frontend | Passed actual title/settings/character paths and guarded Continue/save lifecycle |
| ZHS credits | Passed actual text, close lock, fade timing and return-to-settings flow |

The initial owner-resource game-over oracle excluded the old title rectangle
beginning at y+9. It correctly rejected pixels from the newly centered title,
whose label now starts at y+5. Its exclusion rectangle was updated to the new
label plus its one-pixel outline while retaining source checks over the top
and both ribbon wings. That corrected gate and the new higher-resolution icon
pack still need execution after their next source/resource checkpoint.

The summary focus test also contained a pre-existing fixture identity error:
it changed only the history record's ascension, so the renderer selected the
shorter archived run. The corrected fixture preserves the actual run identity,
constructs a genuinely long score/badge display, and asserts which record the
renderer uses before testing scroll/reveal behavior.

The third full build at
`80bf6090116b7380825fab54e52ba72b99404746` stopped on the combat module's
`sizeof(CombatCardMotion)` bound (7888 versus 6144 bytes). No third-stage
runtime pass is claimed here.

The fourth checkpoint,
`97a63d78979daba7d12fa8475b305e836fc9503c`, built all 316 Ninja steps with
Clang 18. Its actual-resource gates used the new 5,275-entry auxiliary
catalog, including the 31 rebuilt score/badge/discovery images. All 31 image
directory sizes match their intended 16×18 or 24×24 dimensions.

| Actual-resource gate | Fourth-stage result |
| --- | --- |
| Random shader, particle fast paths and canonical save | Passed, 317,957 assertions |
| Random frame comparison against the previous general particle painter | Four full-frame PPMs at 0, 1,250, 7,500 and 16,000ms are byte-identical; 307,200 pixels compared |
| ENG / ZHS frontend and guarded Continue/save lifecycle | Passed, 69,878 / 69,905 assertions |
| ENG / ZHS credits, including scrolling beyond the end | Passed, 230,436 assertions per locale |
| ENG / ZHS bestiary | Passed with final-pack captures |

The settled Random selector and both localized port-credit components were
visually inspected. The random portrait retains its narrow source aspect;
the port heading is the green bold section style above the separate author
line. Final-pack bestiary captures retain compact action/catalog rows and an
arrow centered on the selected text.

Two actual-resource gates exposed remaining validation work. The game-over
pixel oracle still excluded only the first eleven pixels of the Continue
button's text, although its centered label occupies a 22px-high rectangle.
Its exclusion now follows that exact rectangle plus the one-pixel outline;
source-art margins remain checked. Captures now precede the oracle so any
future failure retains the actual settled screen and reports its first
different pixel. That corrected gate had not yet run at this stage. The
card/HUD actual gate found unequal direct and cached card pixels in ENG and
ZHS before reaching the deck captures; the shared card/cache owner is
correcting that drawing-order issue.

The fifth checkpoint,
`023ae87b911093568eae2986b524b54523aa3312`, stopped during core compilation
because two newly introduced combat-feedback calls referenced `mx` and `mn`
before those helpers were declared. Its failed journal is retained; no
fifth-stage runtime pass is claimed.

## Seventh-stage actual-resource closure

The sixth checkpoint built the corrected production core and the focused
pixel targets. The complete game-over gates passed in both languages. The
card/HUD gate passed the formerly failing direct-versus-cache comparisons,
then exposed two obsolete test expectations: a row-based hand-reader clock,
and a keyboard focus chain that stopped before the restored Timer icon. The
new test checks one-pixel progression, both end holds and a complete return
cycle, plus Portrait → Timer → boundary → Portrait keyboard navigation.
The sixth full build also found a missing standard `<cstring>` include in
the new canvas fixture; that compilation failure was retained and corrected.

The seventh checkpoint includes those fixes and the product's default-visible
Timer icon. Its complete Clang 18 build passes all 316 Ninja steps, with the
source fingerprint unchanged during compilation. Its final actual-resource
gates pass:

| Gate | ENG assertions | ZHS assertions |
| --- | ---: | ---: |
| Complete card/HUD, exact caches, smooth hand-reader cycle, keyboard navigation, deck and original tooltip owners | 713,528 | 791,154 |
| Five complete defeat actors, source transition/summary timings, release gates and unchanged canonical saves | 85,811 | 86,796 |

The final deck sorters and upgrade views were inspected in both languages.
The bold gold sorter labels sit above the card grid; the upgrade checkbox and
label occupy a separate footer row with clear space from the Back tab. The
grid clips above that control row. Defeat and summary captures show the
centered ribbon title, narrower main button and score column, and the rebuilt
score/discovery art at its intended resolution.

The pixel-cache fix retains the source drawing order instead of replacing the
strict image comparison with a tolerance. None of these gates changes a
canonical run while exercising presentation. Native correctness and host
timing do not establish Firebird or physical-calculator frame rates; those
measurements remain separate integration evidence.

## Device performance follow-up

The Random measurements use independent ordinary-OS Firebird flash copies.
Only the snapshot header's flash-path field is retargeted; every other raw
snapshot byte is compared, and the emulator's actual started-flash path is
checked. The runtime, common data and language data are installed normally.
Application exit writes the foreground SP804 report at32,768 Hz. Matching
scene-counter differences exclude the same Ironclad setup and exit sequence.
They are actual emulated ARM timings, not native host frame rates.

| Checkpoint | Measured Random window | Frames / hardware seconds | FPS | Mean render | Resource reads in this window |
| --- | --- | ---: | ---: | ---: | ---: |
| `0418cf3e` | First30s, including Random's initial load | 293 /30.3011 | 9.6696 |102.961ms |22 |
| `0418cf3e` | Following60s | 591 /59.9590 |9.8567 |101.118ms |0 |
| `6355aef2` |10→40s matched window |316 /30.0496 |10.5159 |94.744ms |0 |
| `a2b9ff73` |10→40s matched window |327 /30.0456 |10.8835 |91.529ms |0 |
| `d8510f20` | First 10s, including Random's initial load | 122 / 10.2726 | 11.8763 | 83.613ms | 22 |
| `d8510f20` | 10→40s matched window | 387 / 29.9874 | 12.9054 | 77.152ms | 0 |
| `f1ad95b3` | First 10s, including Random's initial load | 134 / 10.2934 | 13.0181 | 76.253ms | 22 |
| `f1ad95b3` | 10→40s matched window | 417 / 29.9666 | 13.9155 | 71.527ms | 0 |
| `b5c127d8` | First 10s, including Random's initial load | 155 / 10.2951 | 15.0557 | 65.980ms | 22 |
| `b5c127d8` | 10→40s matched window | 484 / 29.9661 | 16.1516 | 61.578ms | 0 |
| `41839649` | First 10s, including Random's initial load | 179 / 10.2885 | 17.3980 | 57.052ms | 22 |
| `41839649` | 10→40s matched window | 551 / 29.9718 | 18.3840 | 54.060ms | 0 |
| `af154470` | First 10s, including Random's initial load | 196 / 10.2968 | 19.0350 | 52.117ms | 22 |
| `af154470` | 10→40s matched window | 611 / 29.9637 | 20.3914 | 48.705ms | 0 |

The intermediate results drove the successive optimizations below. The final
sixteenth window is 20.3914 FPS, about 2.07 times the earlier measured warm
9.8567 FPS; it does not establish 30 FPS. CPU rendering cost and cold loading
remain separate. Every particle, original texture and source animation clock
is retained, and host-native speed is not used as calculator FPS evidence.

The ninth checkpoint added a225 KiB exact water memo keyed by the complete
immutable gradient/storage identity, owner generation, dimensions and shader
strength. Each display pixel reuses its last RGB565 result only when its newly
calculated noise value is identical. All40 particles and the independent light
still update and render. Texture interpolation also preserves the exact two
rounding stages while removing redundant multiplies. The ordinary OS rejected
this build with `[Zehn] Memory allocation failed!`, despite its22,983,964-byte
allocation passing the static24 MiB ceiling. USB readback matched the runtime;
this was a real contiguous-heap limit, not a corrupt transfer.

The tenth checkpoint pays for that memo by reducing the existing picture pool
by256 KiB. Ordinary scenes retain6.75 MiB of decoded images and6 MiB of tile
groups; merchant scenes retain11.75 MiB and1 MiB respectively. Its22,722,076-byte
allocation is below the previously bootable seventh build. Fresh ordinary-OS
launch and USB readback both pass. The pool-boundary regression writes the
last image byte while independently fragmenting and filling the tile banks.

The eleventh checkpoint replaces the rotated particles' two dynamic64-bit
divisions per row with quotient/remainder recurrence. It preserves C++
division toward zero, including negative UV coordinates and zero crossings.
The independent direct-division oracle checks72,300 cases. Synthetic tests
pass112,513 assertions, and actual ENG/ZHS each pass425,309. Four complete
Random backgrounds remain byte-identical to the earlier general sampler,
comparing307,200 pixels. Its ordinary-OS launch passes at22,722,636 bytes.
The measured improvement is modest; the following work targets the light's
per-pixel color sampling and zero contributions.

The measured tenth-build ordinary Ironclad departure produced only four
character-scene frames over 1.276 hardware seconds, including click/setup and
the first mask read. The twelfth checkpoint prepares the selected/Continue
mask while its action is visible, uses exact per-channel RGB565 fade tables,
and separates first-frame preparation from the 800ms visual transition clock.
Diagnostic intervals still include all of that preparation work.

Fresh ordinary-OS captures at 40ms intervals show the twelfth build's first
Ironclad departure image about 1.52s after input and a fully black image at
2.32s: approximately 800ms of visible departure. The room becomes visibly
lit near 5.00s and fully visible near 5.40s after its cold preparation. The
character-scene difference contains 11 frames, including about 1.44s spent
in input/new-run setup; its 4.74 FPS mixed average is not the animation's
isolated frame rate. Mean rendering in that mixed window is 73.72ms. The
run reaches Neow, saves and returns through the title to the ordinary OS.

The twelfth-build Random departure resolves to Silent in the measured run.
Its first selected-character image appears at 2.32s after input and reaches
full black at 4.08s, consistent within the 40ms capture spacing with the
original 1000ms reveal plus 800ms departure. The cold creation and selected
character's asset work are retained in the reported 4.155s mixed window.
Room entry, natural Save and Quit, and the 7,126-byte save readback pass with
zero resource failures. These measurements establish the actual transitions;
they do not claim 30 FPS during every departure frame.

The twelfth checkpoint's light optimization retains both rounded bilinear
stages while computing only the source's nonzero red/green tint channels.
Monotone tint bounds skip provably zero contributions. Its synthetic gate
passes 138,487 assertions; actual ENG/ZHS each pass 451,283. The departure
gate passes 1,256,703 synthetic assertions and 1,256,761 per actual locale,
including all 42 complete source-mask/phase/time combinations. Card/HUD
actual gates again pass 713,528 ENG and 791,154 ZHS assertions. Four Random
backgrounds per language remain byte-identical to the general sampler's
307,200-pixel reference. Its 22,726,956-byte allocation launches normally.

The thirteenth checkpoint replaces the 225 KiB water memo with a bounded
horizontal-interpolation table of at most 288 source color pairs and a 64²
index. The table retains every first-stage fraction and rounding result;
the ordinary vertical stage completes the exact filter. Actual pair counts
are validated before publishing the table, and unsupported dimensions,
formats or excess pairs use the complete general sampler. Owner, resource
generation and all Picture storage fields form its input identity. Synthetic
tests pass 169,071 assertions; actual ENG/ZHS each pass 481,923, including
capacity fallback, generation changes and exact whole-frame comparisons.
The runtime's allocation is 22,655,568 bytes, 71,388 below the twelfth build,
and ordinary-OS launch plus exact USB readback pass.

The existing optional foreground report now separates actual water, light
and dot work without adding a game UI or changing their clocks. In the
thirteenth build's matched warm 30s window, the measured means are:

| Render work | Mean per frame |
| --- | ---: |
| Water | 16.906ms |
| Light | 25.185ms |
| All 40 dots and their simulation | 7.313ms |
| Remaining UI and texture access | 22.123ms |

That window has zero resource reads and failures; 416 of its 417 frames
exceed 50ms. The fourteenth checkpoint validates a bounded 8 KiB light-quad
bitset and a 512-byte vertical water bitset. Channel differences of at most
one permit an exact endpoint selection away from the half fraction; unsafe
quads and half fractions retain the general bilinear sampler. The character
UI also avoids transparent title-outline padding, repeated portrait-outline
lookup, and repeated black-tint image work. All source particles and clocks
remain unchanged.

The fourteenth synthetic gate passes 199,138 assertions; actual ENG/ZHS each
pass 511,990. All four full backgrounds in both languages match the earlier
general sampler byte for byte, and both complete Random-page captures in
each language match the thirteenth build. Ordinary-OS launch and USB readback
pass with 22,668,012 bytes allocated. Its matched warm window has no resource
reads or failures and no frames above 100ms, but 483 of 484 exceed 50ms.
The seven measured render scopes now identify the remaining work:

| Render work | Fourteenth build, mean per frame |
| --- | ---: |
| Water | 15.785ms |
| Light | 24.158ms |
| All 40 dots and their simulation | 8.016ms |
| Character information | 4.667ms |
| Portraits | 4.987ms |
| Controls, both drawing passes combined | 1.312ms |
| Ascension controls | 1.312ms |
| Other rendering and texture access | 1.341ms |

The controls scope is called twice per frame; the table divides its combined
ticks by the number of character frames, not by its call count. The light
bitset alone saves only about 1ms, so 16.15 FPS is still unfinished Random
performance. Actual ARM disassembly shows that the optimized-build light
sampler and dot sampler still save and restore nine registers around every
covered pixel. The fifteenth checkpoint separates the light/dot loops,
proves in-bounds source spans once per row, and inlines their small exact hot
paths and water lookup. Complete border, unsupported-format, unsafe-quad and
parent modulation paths remain available. Synthetic tests pass 255,950
assertions; actual ENG/ZHS each pass 568,802. The eight background captures
still match the independent general painter and the four complete page
captures match the fourteenth build. Ordinary-OS startup and exact USB
readback pass with 22,671,436 bytes allocated.

The fifteenth matched warm window measures water at 15.208ms, light at
17.469ms and all dots/simulation at 7.760ms per frame. The light saves 6.689ms
against the fourteenth window. The remaining UI costs are unchanged; resource
reads and failures are zero, with 528 of 551 frames above 50ms and none above
100ms. The 18.38 FPS result still requires improvement. An independent review
identified why the dot loop's gain is limited: a rotated particle's bounding
box often has out-of-bounds endpoints even when its middle is inside.

The sixteenth checkpoint trims each dot row to a contiguous interior span,
retaining the complete sampler for both margins. A 4 KiB light-tile maximum
table bounds all source quads and their right/bottom texel halo. Across 64
original light phases and 4,905,934 visible source samples, these 4x4 tiles
prove zero contribution for 43.816% of samples, versus 47.275% for a 64 KiB
per-quad maximum table. This is a source hit-rate comparison, not a device
speed claim. A row-local water cursor also reuses the selected horizontal
LUT pointer for repeated noise/column keys while applying each pixel's own
fraction; half-fraction and unsafe vertical cases preserve the full mix.
No cursor survives a row or changes a source clock.

## Final frontend checkpoint — 2026-10-27

The delivered runtime is `af154470daa96563f0695d043178285d26dcaee4`.
Its strict synthetic Random gate passes 297,749 assertions; actual English
and Chinese each pass 610,601. All four background clocks in both languages
remain byte-identical to the earlier general sampler, and both complete
Random-page captures per language remain byte-identical to the fifteenth
build. The new tests retain independent full RGBA rendering and check
33,792 row-cursor samples, the independently enumerated interior spans,
generation replacement and the tile-bound halo.

The final ARM allocation is 22,676,740 bytes, and ordinary-OS launch succeeds.
The runtime is 1,689,486 bytes with SHA256
`98d1408183e3bff8d99b6fd0cac7143ad7a25783074a3a2297cf69a04adcc375`.
The frontend installation's runtime readback is exact and its later selector
screenshots and snapshot were produced, but its event file ends after the
application-launch command. That partial trace is retained, not described as
a complete operation audit. An independent final installation has 104 events,
no operation errors, a recorded stop, four byte-exact USB readbacks and a
visually confirmed title. Its receipt is
`private-validation/device-stage16/installed-readback.json`. The frontend's
six measurement/departure/exit event streams separately close normally and
confirm their own independent flash paths.

The final cold Random window contains 196 frames in 10.296844 seconds:
19.034958 FPS with 52.117ms mean rendering. Its 22 reads request 274,989 bytes;
one frame exceeds 250ms, also accounting for its sole frame above 100ms.
In the following matched 29.963654 seconds, 611 frames give 20.391372 FPS
and 48.705ms mean rendering.
That warm window has zero reads and resource failures, 250 frames above
50ms, and none above 100ms or 250ms. Final per-frame render costs are:

| Work | Mean per frame |
| --- | ---: |
| Water | 13.288ms |
| Light | 16.532ms |
| All 40 dots and their simulation | 5.267ms |
| Character information | 4.670ms |
| Portraits | 4.987ms |
| Both controls passes combined | 1.311ms |
| Ascension controls | 1.311ms |
| Other rendering and texture access | 1.339ms |

Both ordinary and Random Embark are also exercised through the normal UI on
independent final-runtime clones. Forty-millisecond LCD captures retain the
input-to-first-paint preparation time:

| Path | First changed selector/reveal image | Full black | First visible Neow / fully lit | Character frames / mixed scene window |
| --- | ---: | ---: | ---: | ---: |
| Ordinary Ironclad | 1.840s | 2.640s | 5.400s / about 5.760s | 13 / 2.647308s |
| Random, resolved to Silent | 1.160s | 2.920s | 5.920s / about 6.320s | 38 / 3.012970s |

The ordinary captures retain approximately 800ms between the first departure
image and full black. The Random capture difference is 1,760ms, consistent
within the 40ms sampling with the original 1,000ms reveal plus 800ms
departure. The 13-frame and 38-frame windows include cold new-run, resource
and selected-character setup; dividing those counts by their mixed windows
does not measure the isolated animation frame rate. Ordinary input/new-run
update work totals about 1.795s; its first changed image is about 320ms later
than the earlier twelfth measurement, and this cost is not removed from the
report. Mean character rendering in the mixed windows is 58.188ms ordinary
and 62.754ms Random.

Both paths visibly reach Neow and complete natural Save and Quit, return to
the title and ordinary OS, and read back their saves: 7,025 bytes for Ironclad
and 7,094 bytes for Silent. The character windows request 302,111 bytes in
seven reads and 889,994 bytes in 58 reads respectively. Both full runs report
zero resource failures. The final records, original performance JSON and LCD
captures are kept under `private-validation/frontend-firebird-stage16`;
`stage16-summary.json` joins the exact source, ordinary-OS, pixel and device
evidence without converting these named scenarios into a global 30 FPS claim.
