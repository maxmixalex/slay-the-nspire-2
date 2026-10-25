# Work state — 2026-10-25

## UI12 playable follow-up — acceptance completed

Delivered runtime: **`9398633d8fd715c008560cc038f61d8f85c98466`**. Validation source:
**`1848338c68d4f10e9fa46d414729087fbd0b1224`**; the only difference is the
independent Architect regression source. All 504 production `src`/`include`
files match. Verified source is synchronized to `main`; no original assets,
fonts, ROM/NAND, private assemblies or generated game packs enter GitHub.

The prior recovered UI12 baseline below is retained with its actual receipts.
This follow-up additionally stops invisible glyph-span raster work on empty
merchant preparation canvases and restores the original Architect/player actor
layer. Its VisualOnly scene uses existing idle/attack/hurt clips, source first
line waits, 400ms speech reveal, authored dialogue/next labels and the original
left-side death animation at the terminal handoff. It does not add Osty from
old combat state. Long English/Chinese dialogue uses the existing reader.

Actual original-resource validation found two real omissions: the dialogue was
queried from `events` instead of `ancients`, and a partial-history fallback
covered the actors before the terminal quote stage. Both are fixed; the original
pixel and nonempty-text assertions now pass. Test fixture corrections preserve
completed-run exclusion from ordinary Continue and the independent Silent
actor/UI clocks. These test corrections are not misreported as game features.

### Final acceptance and deliverables

- **135/135** registered native tests pass, **0 failures**, 64.05s; **310 Python
  tests** and tool compilation pass. Final actual-resource checks pass for UI12
  combat feedback (12,594), merchant preparation (223), and the original
  English/Japanese camera/sleep/lifecycle gate (2,122,238).
- The independent actor gate passes all **20** five-character × Normal/Fast ×
  English/Chinese combinations, 7,279 assertions per language. It checks original
  sprite ink/anchors, repeated hits, actual text and scrolling, the two-actor
  terminal composition, no residual Osty, death and canonical-state invariance.
- Final ARM is **1,600,738 bytes**, SHA-256
  `5b62c0797bb10d49e9d7d36bc478dff808dc1cdf83cc55633433d2bc3d6a7275`.
  It requests **20,363,472 bytes**, below the unchanged 24 MiB ceiling. Common,
  English, Chinese and launcher install at **69,514,501 bytes**. No new resource
  payload, App/State field or persistent cache was added by the actor follow-up.
- Final Firebird journal **`ui12-architect-device-final-55becdaab6a4`** passes
  with unchanged source. Ordinary revision69 Continue/SaveQuit/normal OS exit
  preserves the canonical run. A separate native-generated original-entry
  Ironclad fixture shows dialogue, Proceed, actor actions, original-position
  death, summary, Main Menu and normal OS exit. It is not a natural three-act
  playthrough or a new performance benchmark.
- The ordinary save and progress were then restored through USB and read back
  **byte-for-byte equal** to the original 7,708-byte/1,191-byte files. Production
  load/validate/roundtrip passes, resource failures are zero, and the controller
  has stopped normally.
- The paired calculator ZIP and separate fourteen-language archive are verified.
  Evidence contains **82 files / 65 lossless 320×240 PNGs / 10 source groups**;
  earlier screenshots remain labelled with their actual runtime.

Exact closed journals, file sizes, SHA-256 identities and test scopes are in
`validation-ui12-0.17.json`; the user-feedback matrix is
`ui12-lcd-followup-0.17.md`. The first complete native attempt could not launch
two unchanged host ELF files missing execute bits. Its failure remains recorded;
a complete same-session permission-corrected rerun passed all 135. Intermediate
actor failures and superseded candidate builds remain separate from final passes.

The existing matched Shop result still belongs to `1826969`: mean room interval
improved 23.23%, while the largest cold interval rose to 2703.094 ms. The new empty
clip fast path has correctness evidence, not a newly measured speed percentage.
Camera/noise/LCD mappings and the specific remaining Doom/Architect visual
families are explicitly separated in `screen-effects-ui12-0.17.md`; no blanket
GPU equality or 95% score is claimed. The user's full Ancient animation storage
exception and audio/multiplayer/Timeline exclusions remain.

This acceptance cycle has no open build, packaging, source-persistence or
emulator task. Install the four matched default files; keep ordinary saves,
progress and settings. Any later implementation work starts from this verified
runtime and preserves its paired UI40 resources.


## UI12 recovered 169 baseline — historical completed acceptance

Runtime checkpoint: **`169cea998135a47e5f8580b15df798244e0e7e6a`**. The final documentation/source
checkpoint is synchronized from `work/0.17-world-parity` to **`main`** after
verification. Every coherent source stage was pushed and independently fetched
before its long validation; public source excludes private game data, fonts,
calculator images and generated packs. The earlier approval block described in
historical notes below was resolved by the user's explicit resumed instruction.

The interrupted source at `af95e293` was recovered intact, preserved remotely at
`fe11b6d756012b889eb48d5911df408caa126da1`, and advanced through the integrated
feedback/console/input/renderer fixes. `ui12-lcd-followup-0.17.md` is the full
feedback matrix; `validation-ui12-0.17.json` binds the deliverable, executed gates,
ordinary flows, measured limits and excluded attempts to exact checkpoints.

The calculator deliverable has a **1,595,942-byte launcher** and requests
**20,356,556 bytes** under the 24 MiB ceiling. Common + ENG + ZHS +
launcher install at **69,509,705 bytes**. All sixteen language resources
were independently verified; only ENG/ZHS are in the default installation.
The repaired Necrobinder adds 98,098 shared bytes, the sampled studio-logo
sequence 165,511, and the swarm/sleep sprites 2,284. The accepted UI11 animation
precision remains; this stage does not remove animation frames to fit storage.

### Verified behavior and complete suites

- The complete registered native suite passes **134/134**
  against the frozen release runtime. Python discovery plus the six additional
  converter/packaging modules passes **310 tests**. Compilation of tools passes.
- Actual final-resource gates cover all sixteen locale descriptions/fonts,
  startup/SkipIntroLogo, merchant preparation and occlusion, English/Chinese
  combat, and the dedicated UI12 card/banner/cache/early-Loot contracts.
- Real source-driven screen tests cover rumble/trauma/punch/drunk schedules,
  normal Smith, sleep/wake tails and sprite bounds, Doom's actual execution
  boundaries, and Architect Normal/Fast/Off/Continue/terminal lifecycles. These
  are presentation contracts and do not consume canonical RNG or alter saves.
- Ordinary Firebird `f4a7cd59` completed an Ancient-to-battle/reward/map/save/
  Continue flow. `44ebf0e` naturally continued to floor 6, confirmed a single
  Battle Start then Player Turn, and drew Rage normally. Its selected card
  revealed the complete `gain 3 Block.` tail. The ordinary saved state was
  floor 6, turn 5, HP 59/85, gold 280, deck 12, revision 68; it validated and
  roundtripped exactly through the production decoder.
- Final `169cea99` ordinary startup/Continue, actual digit-1 / Enter Rage play,
  SaveQuit and return to the OS pass in `78793058034c`; USB production readback
  validates and roundtrips revision 69 exactly. Separate Flag-console original
  `fight`/Tab fixtures verify moving phobia swarm and sleeping Z, then restore
  the ordinary save/progress with identical USB readback (`ea63bb4cdfce`).
  Earlier captures are never relabelled as the final binary.

### Performance and real limits

The closed matched Shop fixture compared UI11 N with `1826969c` under the same
canonical input and protocol. Mean room interval fell 106.880 → 82.053 ms
(**-23.23%**), while the maximum increased 2,387.054 → 2,703.094 ms
(**+13.24%**). Canonical readback stayed identical and resource failures were
zero. Inventory preparation provides visible feedback and bounded image/text
steps; three separate observed cold 180 ms Esc taps cancelled. Cold I/O latency
has not been eliminated. These are Firebird SP804 timings, not physical-device
FPS. `44ebf0e`'s ordinary three-enemy combat sample averaged 38.522 ms over
1,438 frames, with a 487.305 ms maximum; it is not a matched speed comparison.

Failed/unfinished earlier attempts remain excluded: the old short-tap merchant
smoke, the ambiguous black-screen/blocked-USB exit attempt, superseded native
builds, and an optional exhaustive memory/range resource traversal. The latter
thrashed the shared tiled cache and was interrupted; its passing components have
separate exit receipts. A new Doom gate exposed a real death-screen target reset;
the release preserves the original combat scene for the source 300 ms punch and
verifies visible frames before the real terminal screen. No diagnostic-only
probe is counted as completion of a feature.

Audio, multiplayer and Timeline retain the agreed exclusions. Full Ancient
actors retain the user-authorized storage exception. Dense GPU effects and
exact desktop rasterization remain CPU/LCD mappings. Architect camera timing is
implemented; this does not claim every authored player-attack/FireBurst particle.
No manual WASM/browser validation or physical-device speed claim is made.

## UI12 active feedback correction — historical pre-recovery notes

The user explicitly authorized pushing UI11, then requested the next complete
calculator usability pass. Remote main/work were read at UI10 `9dc0c112`.
The atomic fast-forward of UI11 `40def2b0` was rejected by automatic approval
review despite that authorization: it requested the exact source payload and
destination be bound in user-authored text. No alternate transport was used.
All UI12 checkpoints remain local until that block is resolved.

Three isolated worktrees implement frontend/credits/collections, combat/cards/
rewards, and rooms/map/character selection. Root owns shared fractional glyph
layout, private resource integration, calculator physical inputs, the top bar
and final Firebird acceptance. The console is now assigned to the frontend
agent after its coherent UI stage. This section records work in progress;
no UI12 emulator or completion claim is made yet.

The root integration is now frozen at J `1074b720cf281629b52003794739e5c71d1daa27`.
Its ARM launcher builds successfully at 1,570,350 bytes and requests 20,126,948
bytes under the 24 MiB build ceiling. H's unchanged final UI40 resources plus J
install at 69,318,101 bytes with English and Simplified Chinese. All sixteen
locale conversions completed; 15 H locale loading/formatting routes passed,
then the Turkish route exposed an invisible U+00AD mistaken for a visible glyph.
J handles that format control and fixes two final cross-family kerning cases;
matching native gates are running. Full NSL2 English and Chinese reconstruction
already passes with I's test-only include fix and unchanged H loader library.

Three independent ordinary Firebird sessions have executed H: typed Flag-key
console, original settings controls, ordinary new-run/character transition,
Ancient choices, save/title/language change/continue, and a normal first battle.
Their acceptance remains scoped to their actual checkpoints. Real interaction
found an aimed-card clipping defect and an Ancient first-click focus defect;
J implements both. A further ordinary OfferReward alignment omission was found
in the combat session and is being fixed before final delivery. Native tests
are correctness evidence, never calculator speed measurements.

Root implements fractional advances, true source pair kerning/negative bearings,
official TI key glyphs, real pad-edge navigation, 1–0 hand shortcuts, exact-old-
default-only migration, expanded potion pages and top-bar tooltip/arrival anchors.
The frontend implements original material buttons/tabs/scrollbars, credits and
collections, and the actual typed command console. World implements source random
selection/transitions, reconstructed Necrobinder head/fire, in-place reading and
map/room layout. Combat implements target-only values, actor/HUD/card layout,
thought bubbles, early loot, actual Smith animation and merchant card caching.

The private character repair adds only 98,098 bytes by replacing its old tile
references and reclaiming dead tiles, instead of appending 8.42 MB of sparse
frames. Full images/clip clocks remain preserved at the accepted UI11 precision.
The final source-only report and package are still being assembled. No WASM or
browser validation is run. Runtime source stages have local archives/journals;
none is described as pushed while automatic approval blocks the remote update.

Host ENOSPC interrupted two native journal closures and truncated a frontend
emulator snapshot. The failed/unfinished receipts are retained, not counted as
passes. Removing only old reproducible UI9/UI10 CMake objects/libraries/executables
restored space; original inputs, source, reports and emulator evidence were kept.
The frontend restarted from a clean independent NAND and again installed Ndless
normally before H. The earlier truncated snapshot/installer errors did not execute
H and are not attributed to the game.

### UI12 world final acceptance and complete registered suites

World production stages are merged through the L source-order correction.
Ordinary Firebird H exercised Random reveal/original-mask departure, Neow,
compact map legend/drawing tips, UI language change plus saved-run Continue,
Chinese inline event keyboard/thumb scrolling and the rest room. J executed
the actual Smith selection/confirmation, full enlargement/flash/deck flight,
and the repaired first pointer Ancient choice after reading focus. L verified
the source portrait order and a real mouse click on Random. Its session
`ui12-world-firebird-l-preview-95adc07760f1` passed, returned perf through USB,
flushed its independent NAND and quit normally; `source_changed=false`.

The M-matched native core library now passes all eight complete world UI /
lifecycle suites plus the registered focused reader gate (403,476 checks).
The final event, room, map, act-intro counts are 5,161 / 8,809 / 98,037 /
47,282; no complete suite is replaced by a reader-only invocation. Test-only
checkpoints `c083b79`, `fdab3db`, `e16933b`, `5981dd3` update changed LCD
geometry, explicit legend controls, the actual 1,600ms departure prelude,
settled post-heal actor samples and deliberate pointer hover ownership.
Canonical saves, RNG assertions, four exact Defect composite colors and
uint32 map-clock wrap remain checked. All final journals passed with unchanged
source fingerprints; the earlier failures remain separately recorded.

`docs/world-ui12-0.17.md` records checkpoints, complete suite receipts, ordinary
emulator observations, measured limits and the optional unadded Ancient
animation. No WASM/browser test or physical-device performance claim is made.
The remote approval rejection remains unchanged; these are local persisted
checkpoints, not pushed source.

## UI11 final N — readable LCD, storage and ordinary ARM execution

UI11 continues from accepted UI10 `9dc0c11291b828e4e17a347e7ef7a27c7b371e60`.
The user-authorized 320 × 240 adaptation uses roughly 1.5× main text/control
sizes, reflow, scrollable descriptions, thinner outlines and corrected Latin
spacing. Exact UI10 screenshot equality is no longer the acceptance criterion.
The runtime removes patch notes and feedback, preserves original credits and
appends the zyf722/Codex Nspire port credit. Focused cards, inspection, events,
Ancients, shops, collections and settings now use the readable LCD layouts.
The terminal summary also has a readable scrolling article and keyboard focus
reveal. Ordinary K/L gameplay and final N title/terminal execution now pass in
Firebird; their distinct scopes are recorded below and in
`ui11-lcd-adaptation-0.17.md`.

### Implemented retained differences

- Combat now has actual hand gold/red/cyan highlight conditions, selection
  flashes, exhaust dissolve/embers, seven-node power-card travel, recorded
  trails, aggregate shuffle and generated-card entry effects. All five orb
  kinds report real passive/evoke events, including repeated evokes and extra
  Gold-Plated Cables activations, without resampling targets or changing RNG.
- Heal smoke/desaturation, rest embers, merchant candles/light nodes, chest
  coins/rarity glows/empty smoke, card acquisition trails and separate potion/
  relic arrival flashes are linked to real actions; restored rooms do not
  replay completed effects.
- Bestiary previews retain detached phase and liveness facts for the tested
  regeneration, charge, burrow, stun, hatch, growth, summon and multi-actor
  down/revive contexts. Character-select particle contexts and duplicate-actor
  cosmetic scale/hue are implemented with bounded independent display state.

`combat-secondary-ui11-0.17.md`, `room-vfx-ui11-0.17.md` and the current
contract rows in `ui8-source-ui-audit-0.17.json` identify the implementation and
limits. Dense GPU turbulence, continuous particle paths, HDR bloom, smoke/noise
materials and exact desktop glyph rasterization remain explicit CPU/LCD
approximations. Closing their real triggers does not establish pixel identity
or a measured 95% restoration score.

### Resource, language and memory results

The final private common art file is **61,186,168 bytes**. English is
**1,049,523 bytes** and Simplified Chinese **5,034,485 bytes**: the default
resource files total **67,270,176 bytes** before the launcher. Final N's
**1,506,285-byte** launcher gives a four-file installation of **68,776,461
bytes** (68.78 MB decimal), **24.9206% smaller** than UI10's 91,604,965 bytes.
This is storage reduction, not a speedup percentage. The common file alone
is **23,648,624 bytes** smaller than UI10. Lossless retiling is followed by authorized bounded mixed
RGB555/454/444 and alpha6 animation precision; all frames, timings, dimensions
and geometry remain intact. Static UI/cards/icons/text remain exact, and the
five character-select static planes use the same map as their animated layers
to avoid seams. Color precision is explicitly lossy, not a lossless-compression
claim. See `storage-memory-ui11-0.17.md`.

All **16 original locale editions** have actual source text, original font
chains and verified glyph coverage. Each optional edition shares the same
common file; installing all editions is not the default 65–70 MB target.
Original missing translations use English fallback (Thai: 1,966 bound keys;
other non-English editions: 91–129); calculator-only instructions outside ZHS
and early boot text retain the documented fallback. See
`localization-ui11-0.17.md` for the installation table and formatter limits.

The build allocation ceiling is **24 MiB**. The D candidate's **23,874,780-byte**
contiguous Zehn request failed in ordinary Ndless with `Memory allocation
failed`; being under that ceiling was not sufficient. F uses a real **5 MiB
image cache / 480 keys** and **6 MiB tile-group cache / 48 pages**, producing a
**19,755,048-byte** linked allocation that starts successfully in ordinary
Firebird. Subsequent K/L ordinary gameplay and N's final launch/terminal/exit
also pass. Final N's measured linked allocation is **19,806,488 bytes**; the
5 MiB image and 6 MiB tile caches are real runtime buffers, while 24 MiB is
the build ceiling rather than a claim that the OS accepted a 24 MiB block.

### Executed focused correctness

- I (`4e4ce0692a01132de613c01e3cb6a8a56f0d001c`): all 16 final locale editions
  pass **38,644 checks / 53,072 model-event descriptions**, with zero failures
  or unexpanded templates. Actual room effects pass **105 checks**. Independent
  card-highlight scalar/clipping checks pass **12,592,419 comparisons**.
- J (`f16573e1b9c6da390cd9944bd447acb4dc0482d3`): actual final common/English
  combat secondary checks pass **325,107**. J's test and renderer were compiled
  against the unchanged I core library; this is explicit incremental evidence.
- Bestiary phase fixture checkpoint
  `49340f020bfbe96c49cc6de71aa122e1556c269c` passes **139 checks** with unchanged
  H core and canonical-save preservation. F tile/cache checks pass
  **16,396,600** and **5,966** respectively. Exact receipts are in the module
  reports and source audit; these are correctness, not speed measurements.
- Final Game Over fixture checkpoint
  `02f0f4b266dc1ac9578a43e02521a07d95d647de` passes **176 checks**, compiling
  the actual L UI against unchanged I core. Original score/badge/history and
  release timing, long-summary scroll bounds, focus wrap/visible auto-reveal
  and canonical SaveData/RNG preservation pass. Build `e4bc30c76261` and run
  `d246a073ed6e` both record `source_changed=false`. This asset-independent
  gate is not a font screenshot or calculator speed measurement; the two
  earlier fixture-only failures remain explained in the source audit.

### Ordinary ARM/Firebird acceptance and measured limits

- **K ordinary continuation** (`90d981c4e42e5f67f7a72b52f37fa1ac45410a71`):
  continued the F floor-4 combat, inspected/played cards, completed four turns,
  won, claimed gold and Setup Strike, opened the retained-room map, saved and
  exited normally. Actual NSZ1 readback passes production decoding, L native
  load/validate and exact canonical resave. HP 73→64, gold 109→125, deck 11→12
  and revision 30→48; Setup Strike is the only added card. The NSH9 readback
  matches floor 4 and four battle turns. Final canonical phase is Reward; the
  open map does not claim travel to another floor. Journal `8a5389fd74a2` and
  readback `dfa5befd59c0` pass with `source_changed=false`.
- **L ordinary continuation** (`7722cad8adb6626081b0100e5017d3de095904c5`):
  traveled from that saved run to the floor-5 Tablet of Truth, used the full
  reader and healed 64→80, then entered floor-6 three-slime combat. Actual
  middle-enemy targeting changed its HP 27→9; ending the turn left player HP 73.
  Voluntary Give Up opened the readable summary, scrolling, Main Menu and
  normal Quit. Journal `93bd401af87c` passes with `source_changed=false`.
- **L Chinese room/Bestiary**: ordinary cold OS/Ndless launch used constructed
  canonical Neow and compendium-progress fixtures. Actual option selection
  changed HP/max HP 80→91, finished the event and advanced revision 4→6; native
  readback confirms it. Five-action paging exposes and plays the sixth Death
  action. Journals `d6f0eebb90d7` and `7012def96add` pass. These fixtures verify
  Chinese interaction and presentation, not ordinary-playthrough reachability.
- **Final N** (`e95893c9effaa9b5f7fd09158cca546d7b65ce3b`) changes only two
  initial-defeat-quote rendering lines from L: natural paragraph height and
  source `quote_y + 8` placement. Ordinary boot, corrected quote, summary,
  Main Menu, normal Quit and exported performance pass journal `383291056740`
  with `source_changed=false`. The release owner directly reviewed the quote
  below the banner without corpse overlap. N's 206 title / 338 room / 67
  overlay samples are a focused terminal smoke, **not an N combat benchmark**.

Measured mean frame intervals and cold outliers remain visible:

| Actual session / scene | Frames | Mean interval ms | Mean render ms | Maximum interval ms | Over 100 ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| K ordinary combat | 1,324 | 35.800 | 27.880 | 382.874 | 14 |
| L ordinary three-enemy combat | 430 | 41.751 | 34.526 | 752.655 | 16 |
| K map | 46 | 80.050 | 79.165 | 452.087 | 3 |
| L map | 81 | 72.307 | 71.338 | 393.829 | 2 |
| L Chinese collection fixture | 784 | 42.713 | 20.161 | 1,807.068 | 18 |
| L Chinese room fixture | 102 | 54.170 | 46.605 | 1,393.280 | 3 |

All four reports have **zero resource-read failures**. Preparation is separate:
K's 17 preparation frames average 139.442 ms, maximum 1,205.963 ms; L's 45 average
91.601 ms, maximum 987.152 ms. L also records a 1,214.386 ms mixed-room interval.
Map/overlay work and cold-entry stalls remain measurable limits; these values
are not removed from the report or described as uniformly smooth performance.

`performance-ui11-0.17.json` binds each result to its source checkpoint,
closed journal, raw report hash and exact scope. It records SP804 emulated
calculator time, not host wall time or a physical-device measurement. K/L
use different encounters and cache histories; there is no matched UI10/F
baseline, speedup percentage, median or percentile claim. Startup counts only
selected resource processing/App initialization, with first render separate.
No UI11 WASM/browser build or test was run. Final private package/readback and
delivery receipts are maintained by the release owner.

All UI11 checkpoints cited above are local frozen stages with logged results.
Automatic approval review rejected remote push; this work state does **not**
claim UI11 was synchronized remotely. Private game assets stay outside public
Git. The UI10 K delivery and browser/performance receipts below are historical
and must not be reused as acceptance for changed UI11 code.

## Final UI10 K runtime and delivery acceptance

The delivered runtime and regenerated resources are frozen at remotely verified
`61c5a93acbd9730ab345a4add8293bda744820f0`, tree
`3e155a32748e3917ba8f369de4da7cd8678389fe`. The final closeout changes only
documentation. Main-menu/Continue/profile/collection/statistics/history and
General/Input/run-setup work from the earlier UI10 stages remains implemented;
the final production change skips inactive long-press work while preserving
input ownership, residual decay, the source threshold and the source bar.

Matching K acceptance is complete:

- Native `c6dc1b85d710`: **121/121 pass**, 104.98 seconds of CTest execution.
- Python `2c681130c3ef`: **252 + 54 pass**; compileall also passes.
- ARM `4413aa9add78`: launcher and linked division/LZMA2/tile/RGR instruction
  and ABI suites pass. WASM `cf00a6665385` also passes.
- Actual resources: **96/96 bilingual targets pass** (root 17, front 15 and
  combat 16 per language). Both complete split streams pass separately.
- Complete image comparison covers **1,228 PPM captures**: root 536, front
  214 and combat 478. Root uses I except the 38 corrected J FTUE captures;
  front is exactly I; combat keeps only J's three corrected Settings hover
  captures and one added Long Press capture per language. All 116 card/HUD
  captures match J, and both hold-bar states per language match I. Two combat
  captures per locale are emitted beside their target directory and are
  included in this total, although the generic per-target receipt lists omit
  those four files. No unexplained runtime image difference remains.
- Ordinary offline browser `e70b5849ae91`: **21 flows / 47 captures**, no
  page errors or external requests. The final HTML SHA matches the closed
  artifact receipt both before and after execution.

Every completed K source-guarded journal reports `source_changed=false`.
These tests cover port behavior and source-derived contracts; neither their
count nor equality between port versions establishes a 95% original-game
restoration score. See `ui10-front-source-review-0.17.md`,
`settings-run-setup-ui10-0.17.md`, and `ui8-source-ui-audit-0.17.json`.

Resource conversion `64c1f36fbb05` reproduces both logical NSP2/39 packs and
all three physical files exactly as H and I. The installed resources total
**90,114,227 bytes**. K's launcher is **1,490,738 bytes** and the complete
four-file installation is **91,604,965 bytes**. Actual prefix/image/BSS
allocation is **16,256,400 bytes**, leaving **520,816 bytes** below the
unchanged 16 MiB ceiling. Relative to I, the launcher is 102 bytes smaller
while allocation is 88 bytes larger; these are separate measurements.

K ordinary installation `1f6bbbeca0eb` fully reads back the new launcher and
associates the identical resources with H's complete USB readbacks. Cold
execution `9808e0b2a6b0` opens the persisted Long Press control, switches it
off and on, continues the actual floor-2 reward room, saves, and exits to the
OS normally. All seven LCD captures are reviewed and all five profile files
are read back. Matched K validation `a2b145331ee7` confirms the original
230,540-byte canonical run and backup remain exact, NSH9 records the actual
save/revision 24, and the 108-byte NSPF5 retains flags 644 and both key banks.
The original NAND is unchanged; the emulator is stopped. H's four capacity
cases and I's 9.8-second physical hold remain labeled with their actual
execution checkpoints rather than being counted again as K tests.

Final performance association `9a986cce7aaf` compares all 25 official core
objects, their archive order, and the complete 7,756,600-byte benchmark ELF
exactly with the measured private candidate. The four balanced same-battle
samples therefore apply to the final native implementation: **4.22% lower
median CPU time**, with all 14,400 paired frames and canonical bytes equal
and no warm I/O. The 9.53% mean is influenced by one high I sample and is
not the preferred general description. Earlier F/I regressions, cold-start
costs and all raw samples remain in `performance-ui10-0.17.md`. No physical
calculator FPS or universal speedup is claimed.

Packaging `b42a15d72cae` and independent closed-artifact readback
`868e0847e8d0` pass all 13 ZIP members. The final private deliverables are:

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `Slay-the-Nspire-2-0.17-ui10-bilingual-calculator.zip` | 90,276,254 | `6ffc75158b29df152502e9b85cac4973f88b9349f70d24d9ea6ecbc3d090c460` |
| `Slay-the-Nspire-2-0.17-ui10-bilingual-offline.html` | 129,282,581 | `d03be509015e5bb8ad515fd0933ca07236804e57a61e0a265be85bbdf2a576a1` |

Both manifests identify K for runtime and resources; formats remain resource
39, canonical save 17, history 9, and settings 5. Original assets and embedded
deliverables remain outside public Git. The K GitHub Actions run
`40178587658` was still queued at the pre-closeout readback; the I and J runs
were cancelled and H `40174150511` passed. Local matching acceptance above
must not be presented as a completed remote K Actions run.

### Historical UI10 remaining scope, superseded by UI11 above

This records the limits at UI10 delivery. UI11's current implementation,
authorized LCD/color adaptations and actual remaining gates are recorded at
the top of this file and in the current source-audit contracts.

The source inventory covers 3,538 C# paths, 714 nonempty node-namespace files
(plus two excluded empty files), 1,022 scenes and 72 flow contracts. Inventory
and targeted body review do not mean every original method has been audited
or reproduced. The 95% goal remains unproven. Prioritize the recorded
phase-specific/multi-creature Bestiary and secondary VFX contexts, original
glyph ink/kerning and the measured Continue-date advance difference, then
the remaining original startup/settings platform differences. Keep the
explicit LCD, bounded-history, UTC and offline-service adaptations visible.
Audio, multiplayer and Timeline remain excluded by the established scope.

The 30–40 MB storage goal is also unmet. Preserve exact pixels and animation
data while investigating further bounded compression; do not trade original
UI fidelity for an unmeasured size target. The measured bitmap/cache/table
and foreground-workspace improvements are retained, with counterexamples.

## UI10k adopts the measured idle-off optimization

The final bounded experiment changes only two production branches, keeping
I's App fields, layout, global address/size and render address/size unchanged.
The long-press timer advances when the preference is enabled or an owner,
residual timer or unfinished fade still needs service. The progress-bar
visual is computed only when that setting is enabled. The elapsed-run clock,
500ms threshold, cancellation, source colors,250ms fade, canonical formats,
resources and resident pools remain unchanged.

Four balanced I/off-guard pairs each render3,600 identical frames; all28,800
frames, canonical bytes and zero warm I/O checks pass. Every pair improves,
with CPU median1446.0925->1385.028ms (-4.22%). Mean1529.02075->1383.31975ms
(-9.53%) includes a visibly high I sample, which remains in the receipt.
This is a measured improvement in one native battle workload, not an ARM
FPS or universal speed claim. See `performance-ui10-0.17.md` for every sample
and the earlier contradictory/negative windows.

The two production files are byte-identical to the measured private variant.
The existing hold regression adds App-level disabled-but-residual owner,
timer and fade assertions after its original captures. Its short1731 checks
pass with the unchanged I ABI. K requires its own verified remote checkpoint,
matching full native/ARM/WASM/Python and actual-resource/browser gates, final
closed artifacts and ordinary-OS launcher/cold-persistence check. Source
association with the measured binary must be verified before describing K's
performance as accepted. J's fixture corrections and all earlier evidence
remain preserved; source/test counts still do not establish95% fidelity.

## UI10j verification follow-up after the accepted I runtime

I runtime/resource source is remotely verified at
`76c022a1e2db17c49bf3ab8edb335dbb98618582`, tree
`546128015f0087d2327b28f3514019ccdc53d44d`. Matching native121/121 passes in
101.74 seconds, Python252+54 passes, ARM linked division/LZMA/tile/RGR and
WASM pass, and all96 bilingual actual-resource targets plus both complete
NSL2 stream comparisons pass. The ordinary offline browser passes21 flows
and47 captures without errors or external requests. Its real long-press
setting, reload, short taps, pointer cancellation, physical E repeat and HTML
button lifetimes produce canonical turns1,2,3 while preserving both other
profile saves. These layers remain distinct from an original-fidelity score.

The launcher is1,490,840 bytes; actual Zehn-prefix+image+BSS allocation is
16,256,312 bytes, leaving520,904 bytes below the unchanged16MiB ceiling.
All I-required glyphs already exist in H, so all three final resource files
are byte-identical to H; their total remains90,114,227 bytes. The full four-
file installation is91,605,067 bytes. Actual USB readback verifies the I
launcher and reuses the proven-identical H resource readbacks. The largest
specified clean history-write case again writes and reads all16 files,
2,701,448 bytes, successfully with I's actual108-byte NSPF5 record.

The ordinary cloned OS enables Long Press through the real General row and
persists bit7 with the other settings and both key tables unchanged. From the
preserved floor2 reward room it enters the next real battle. A250ms short
release and an Esc-cancelled hold leave the230,556-byte battle save exact;
holding physical E for9,800ms across enemy action ends exactly one turn.
The resulting canonical bytes equal one matching-core end_turn operation.
The original floor2 primary/backup, NSH9 metadata and language are restored;
an independent cold reopen retains the new preference and the original
230,540-byte canonical run. H's four capacity cases and two-language cold
proofs remain separate historical evidence rather than being renamed I.

I's independent closed-artifact readback passes all13 ZIP members. The
calculator ZIP is90,275,858 bytes (SHA256
`cb95c73324efb7cc1bc98e87b78400eb72fec87275818bab5e7b71b92a70c976`);
the offline HTML is129,282,437 bytes (SHA256
`8ecb49ca4108410fbd634e250a40328ee25ef4886d44cf7ff10090bebda18c06`).
The browser verifies that exact HTML before and after execution.

Screenshot review found a verification-only gap: three old Settings hover
captures used obsolete positions. One showed Phobia rather than Screenshake;
Text Effects and Keyboard-Only did not show the named tooltip. J corrects
their fixed source-coordinate points, adds Long Press hover, and compares
the real Settings result with the specified source title/body and anchor.
It also moves the new long-press FTUE assertions after all older captures, so
their added elapsed time does not change the old animated-background samples.
All new assertions remain. J is remotely verified at
`8a2ebda2cfab09d1ce90925599255cd7e11cf14e`, tree
`12dacbcf996f2cfd8024347508e1ee39e3542ea4`. Its focused gate
`ui10j-fixture-acceptance-2a725e41f14d` passes:683 production inputs compare
exactly to I, the accepted I core remains unchanged, both synthetic targets
and all four bilingual actual runs pass. Each locale's19 original FTUE
captures are again byte-identical to H. Each locale's54 other card/HUD
captures remain exact; the three corrected hover captures and the new Long
Press capture pass their full-screen binding/position oracle with unchanged
canonical bytes. No I launcher, WASM, resource or deliverable changes.

Quiet F/I tests retain the same physical F input. Requested-row batching
reduces callbacks46->31/62->39 and seeks43->12/59->12 for English/Chinese,
while total CPU means11.2674->11.32905ms and19.9334->19.6404ms do not show a
universal CPU gain. One identical actual battle exposes a concrete regression:
180-frame CPU mean74.73375->85.2925ms (+14.13%), median74.736->82.8135ms
(+10.81%), despite all720 paired frame fingerprints/canonical bytes matching
and zero warm I/O. The full raw samples remain. A bounded H/I comparison is
now separating pre-hold changes from the new input feature before any proposed
production optimization. Host CPU is not physical-calculator FPS.

The H/I isolation window does not reproduce the first difference: H's180-
frame CPU mean/median is77.8655/77.141ms and the same I binary gives
76.41175/76.3015ms. Idle hold adds only fixed branches; no extra pixel loop or
font traversal was found. Source inspection alone does not attribute timing
to the changed transform-function or App layout. A final balanced F/H/I
window lengthens this same battle sample to distinguish cross-window noise;
it introduces no new scene. That balanced F/H/I run passes43,200 rendered
frames,14,400 fingerprints per version, canonical equality and zero warm I/O.
Its3,600-frame CPU means are1445.43225/1467.781/1616.211ms, so I is11.82%
above F and10.11% above H in this same-window test. A private default-off
branch experiment is therefore isolating code-path cost while retaining I's
App layout. No production optimization is accepted from speculation or from
the earlier short-window result.

Final performance interpretation, documentation, main ref synchronization and
private delivery remain to be completed. No95% restoration claim is inferred
from source inventories or passing tests.

## UI10i closes two further source-review findings

H is remotely preserved at `0eb82c19b8794f5235e6c5283d339f0c93f3a21f`,
tree `77e404db6c133adacb7c02bbd7c275dc319c4ef0`. Its matching native120/120,
Python251+54, ARM linked ABI/codec, WASM,94 bilingual actual-resource targets
and both complete split-stream comparisons pass. The ordinary browser
passes18 flows/41 captures without page errors or external requests. These
are execution layers, not an original-game fidelity percentage.
H GitHub Actions40174150511 also passes; G Actions40172979898 retains its
failed result alongside the already documented G local findings.

H's ARM allocation is16,251,288 bytes,525,928 below the unchanged16MiB ceiling.
The final physical resources total90,114,227 bytes; the installed four files
total91,602,752. All four ordinary cloned-OS USB capacity cases pass with
exact readbacks:1,027,498;1,421,773;2,085,823;2,701,448 bytes. This last case
models the specified history-write peak, not every combination of stale
failure residues. Normal English/Chinese Save and Quit preserves the actual
230,540-byte canonical run while migrating its17,731-byte NSH8 metadata to
18,267-byte NSH9 (469 bytes in the device container). Its observed saved
revision matches canonical revision24; unknown old build IDs remain unknown.
The independent Chinese and English cold reopens also pass, with both primary
and backup canonical bytes unchanged and the final English preference exact.

Continued source review found an in-scope functional omission: the original
optional Long Press Confirmations setting. I implements its default-off,
per-profile End Turn gate, strictly greater than500ms, release/unfocus decay,
one action per owned press and the source additive204x6 progress bar. Mouse,
mapped physical keys and HTML toolbar controls share the native timer. The
toolbar now forwards real down/up/cancel lifetimes, preserving ordinary
non-combat confirmations. NSPF5 uses reserved bit7 in the unchanged108-byte
preference record; older records retain their original validation and masks.
Canonical NSV17 and NSH9 are unchanged. Short JS/module/transaction checks
pass; matching native, ARM, actual UI, ordinary browser and OS checks remain
required. See `settings-run-setup-ui10-0.17.md`.

The known-date Continue capture also exposed an incorrect H oracle premise:
the original DateLabel has automatic wrapping disabled. Direct pinned Godot
measurement of its400x90 rectangle and font-size estimator retains24px for
English/Chinese unknown and representative longest date strings. I keeps the
corresponding5px glyphs, renders only explicit newlines and uses the measured
English6px/Chinese7px line spacing. It does not invent a smaller original font
to hide the port's integer glyph-advance differences. The previous H pass and
this subsequent visual discovery are both retained. All I production work
must start after the combined source has been pushed and read back.

## UI10h closes the G acceptance findings

G is remotely preserved at `7d159cb4534c7923c10f1e5570a1befea1c9d687`,
tree `20460e9f30fc5cc256fa7f3d12653e0e2329deda`; main remains the accepted F
baseline. The matching G ARM and WASM gates pass. The launcher is1,485,615
bytes and its actual Zehn-prefix+image+BSS allocation is16,247,324 bytes,
leaving529,892 bytes below the unchanged16MiB ceiling. Linked ARM division,
LZMA2, tile and retained-filter regressions pass; WASM is6,761,143 bytes.

G native passes118/120 suites in84.92 seconds. Instrumented production hits
confirm that the two failed UI tests request the former SettingsBack action
after Settings adopted the original Back control. H clicks the actual Back
and adds the strict clipped(0,173,30,21) bounds; both short corrected tests
pass. The original missing-hit failures remain in their receipts. G Python runs250
tests with one error: thirteen new Chinese Settings/date fallback literals
need reviewed inline locale bindings. The subsequent54-test gate was not
executed. The missing bindings are now reviewed without moving old pack
string indexes.

The first G resource conversion fails on the new9px source heading band,
which the offline glyph geometry grammar had not admitted. H keeps old
formats strict: append the86 original images to create NSP2/39 first, then
append the exact fonts and compact only their new payload. This retains the
same old artwork, catalog identities and bounded runtime directory. The
failed G conversion produced no accepted resource set.

H also closes the explicitly identified History room-icon tilt/hover
rotation and Input-settings focus-release motion where supported by the
pinned source. All H code, resources, actual bilingual UI, ordinary browser,
updated NSH9 capacity and normal cloned-OS flows remain pending until a
matching frozen H checkpoint is built and executed. The completed F proofs
are not substituted for these new gates.

The web storage regression retains the maximum295,349-byte NSH8 case and
adds the295,885-byte NSH9 case, including denied writes, interrupted profile
deletion and recovery in another host instance. Its opaque-byte short check
passes; the frozen H execution must also use both valid C++ exported records.

## UI10g follows the fully accepted F baseline

F is now verified on both `main` and `work/0.17-world-parity` at
`361c36c27f06c4905e1899b8bdc2ab281ba9b9cc`, exact tree
`a031c1bc29513b80fbf3fd629bb07ef20fd53453`. Its GitHub Actions run40170450855
passes; the preceding E run40168822819 also completed successfully. F's
matching local native119 suites, Python249+54, linked ARM
ABI/LZMA/tile/RGR, WASM,94 bilingual actual UI targets and both complete
NSL2 virtual-stream comparisons pass. The normal offline browser passes18
flows/41 captures with no errors or external requests. F/E compare identical
468 combat and20 key front captures. These counts are distinct execution
layers and are never added into a source-fidelity percentage.

The ordinary cloned OS passes all four actual USB capacity cases:
1,025,362;1,419,637;2,083,681;2,699,304 bytes. Thus both E failures are closed
by actual writes and readbacks. Original files were restored, Chinese and
English Continue/Save and Quit and two cold reopens pass, and the canonical
73/80HP,109gold,floor2,11-card run remains byte-identical. The original NAND
guard is unchanged. F installs91,480,069 bytes; its physical resources total
90,014,771 bytes,2,394,329 fewer than E, with unchanged logical NSP2/38 data.
Its loaded ARM allocation is16,213,224 bytes,563,992 below the unchanged
16MiB limit. The completed private F ZIP is90,183,913 bytes; HTML129,077,681.
An initial long write into the shared workspace lost the ZIP central
directory after the packaging process's own successful readback. A separate
closed `/tmp` package passed independent full archive reads. Its HTML is
byte-identical to the successfully tested browser artifact. The damaged
workspace copy was discarded; its failed independent readback is retained.

F quiet measurements now cover glyph metadata caching, RGR2 and NSL2 startup
with complete raw samples and retained cold outliers. Warm frame CPU improves
in the aggregate, while NSL2 trades extra decode CPU for much less startup
I/O. `performance-ui10-0.17.md` records both sides. G batches only the requested
NSL2 table rows into256 stack bytes to remove redundant callbacks/seeks;
same-input F/G timing is still required. There is no new resident pool or
integrity-only startup scan.

The next source batch restores whole-control Compendium press modulation,
History body fonts/rows/death quotes/badges and64 original room icons,
Settings and Standard/Custom/Daily geometry/fonts/input and22 source-sized
controls. It adds a small exact font suffix and one combined append-only
NSP2/39 artwork catalog. Sparse9px headings are opt-in so ordinary card/text
fitting cannot select their incomplete alphabet. The full old payload remains
an immutable input; no earlier artwork is recompressed.

Optional NSH9 UI metadata records real successful run-save timestamps and new
runs' observed port build identifiers, retaining unknown old values. Canonical
NSV1/version17 remains unchanged. The fixed536-byte tail grows the maximal
history fixture from295,349 to295,885 bytes. The same-revision optional-write
failure limit is documented in `run-metadata-ui10-0.17.md` rather than hidden
behind extra invalidation writes. Short source/codec/font tests and mixed
diagnostics do not replace matching G native/Python/ARM/WASM, font/artwork
conversion, actual bilingual UI/browser and updated ordinary-OS acceptance.
Old E38 packs correctly fail the new History oracle on missing4px ellipsis
and Chinese quote glyphs; the exact added glyph requirements retain those
failures until the matching G resources are executed.

## UI10f directory storage closes the remaining capacity failure

UI10e source is verified at `04038b2cda0240bfa2be1af79bf62401ec26e7a9`, tree
`0013609a7d365c985f62f8c2798a3af790c12589`. Its native119/119, Python244+54,
ARM linked ABI/codec, WASM and all94 bilingual actual-resource gates pass.
The ordinary offline browser passes18 flows with41 captures, no page errors
and no external requests. Its first Chromium launch was blocked by sandbox
socket permissions before executing the page; the permitted retry is a
separate successful receipt. E runtime allocation is16,211,240 bytes, leaving
565,976 bytes below16MiB. E actual scene/font resources preserve old payloads.

Ordinary USB installation and complete readback of all four E files pass.
Packed capacity1,025,362 and raw-backup migration1,419,637 bytes pass; the
2,083,681-byte raw-history and2,699,304-byte all-raw cases fail actual writes.
The four original run/backup/history/language files were fully restored and
independently decoded, with no fixture files left behind and original NAND
unchanged. The failed E capacity gates are not accepted as completed storage.

F therefore adds NSL2 compression of only the existing cached metadata prefix,
keeping all frame descriptors, common artwork and logical NSP2/38 bytes exact.
The short actual candidate saves2,394,329 bytes without a new resident pool.
Its4KiB-aligned boundary and cached-prefix constraint prevent directory decode
reentry during image reads. See `split-directory-ui10-0.17.md` for format,
ownership and negative cases. The matching frozen F source, physical-container
readback, ARM/WASM/browser, real capacity retry, quiet performance comparison,
private delivery and verified main synchronization remain required.

## UI10e source follow-up and final matched acceptance

UI10d is remotely preserved at `78ac86e35651d3832e1b9638001aa0aa6be3f0a0`,
tree `9a271d5013c8397932bd4c6414aba56a1281ebd9`. Its full Python gate passes
240+54 tests and compileall; ARM and WASM builds pass. The linked ARM image
uses16,196,964 bytes including the Zehn prefix and BSS, leaving580,252 bytes
under the unchanged16MiB allocation limit. ARM division/LZMA/tile/RGR2
regressions pass, including malformed retained-color groups.

The d native gate passes117/118 targets. Its single failure is the frontend
test incorrectly applying new title-menu bounds to the separately laid-out
Pause menu; independent bounds for both menus are restored. The complete d
actual-resource gate passes88/94 targets: the same frontend premise fails
both locales, the Profiles oracle composited reticles before rather than
after text in both locales, Chinese collection headings reveal three real
missing5px bold glyphs, and English world_ui has a separately investigated
fixture failure. All failures remain in their original receipts.

The e implementation restores the source joined-title/description Card
Library search and exact query whitespace handling; Statistics composes its
scrolling subtree once through the source alpha mask; History restores the
source header fonts, positions, information grouping and reminder icon.
Ordinary and Fake Merchant inventory navigation now skips sold entries;
price/icon geometry follows original Godot font/container measurements.
Turn banners project the source24px/16px outline widths through their current
zoom, using one bounded scanline to combine half-alpha coverage before drawing.
The full e runtime and actual resources still require their own acceptance.

The completed d resource split is92,408,754 bytes. Its exact RGR2 transform
saves949,484 bytes and new-glyph compaction saves21,498 bytes against their
matching inputs. The first attempted reused compression receipt was stopped
after exposing an offline reuse bug; it is not counted as a pass. The later
conversion with the legacy receipt passes. The e converter recognizes both
receipt forms, validates cached decoded content, and recompresses missing
cache entries rather than silently skipping profitable groups.

Font-only iteration now preserves versions36–38 and the complete existing
payload. The next resource build appends only missing e glyphs to d's37,
compacts only that new literal tail, and appends the original6×6 History lock
as NSP2/38. No old image recompression, new runtime pool or full startup
validation pass is introduced. The final common/locale set, ordinary browser
and cloned-OS USB capacity/Continue results, quiet benchmark, private delivery
and verified main synchronization remain required. Source inventory and test
counts do not establish a95% original-game restoration measurement.

## UI10d title, confirmations and source speech

The verified UI10c checkpoint is `3d6d79907bb76269474acd8bd5c27baf575837f2`.
All118 native CTest suites pass. Its ARM build and linked ABI/LZMA/tile
regressions pass; the actual Zehn-prefix+loaded-image+BSS allocation is
16,180,804 bytes, leaving596,412 bytes under the unchanged16MiB ceiling.
WASM compiles. Both complete scene-font appends preserve their input payloads:
3534 English and36123 Chinese glyphs, with zero missing required glyphs.
The formal Python run failed on missing private lz4 imports, an obsolete
unknown-version fixture and new Chinese calendar bindings. The environment
and both source issues are corrected in the next checkpoint; a full rerun
remains required rather than relabeling that failure.

The next coherent batch restores source main-menu typography/spacing and its
label/reticle motion, the Continue information panel, and generic popup
geometry/material/backdrop timing. Source SaveTime is unavailable in the
canonical run state, so Continue displays an explicit unknown timestamp.
An independent actual-resource oracle exposed blank Card Library sorting
captions caused by looking in the wrong string index; the renderer now uses
the world index. Locale calendar bindings and an extreme statistics clipping
premise are corrected without weakening their ordinary label assertions.

NSP2/37 appends six source-derived images: the independent blue Fake Merchant
welcome bubble and five precisely sized popup skins. The welcome has its own
delay/reveal/scale/rotation/fade state and does not restart when inventory is
toggled or purchased. Existing images, strings, glyphs and clip clocks are
preserved. An offline appended-glyph compactor reuses existing RGR1 groups
without adding a reader or runtime pool; its c-input proof saves21,517 bytes,
including all descriptor/header/alignment costs. Final d savings must be
measured with final fonts.

Source equations, short functional tests and targeted c bilingual captures
are recorded in the module reviews. The d batch still requires matched
native/Python/ARM/WASM builds, all bilingual actual-resource gates, ordinary
browser and cloned-OS capacity/Continue checks, final private deliverables
and verified main synchronization. The source review continues to identify
real remaining differences; no95% claim is inferred from coverage counts.

## UI10 source-sized scenes, locale fonts and shared workspace

The next coherent integration includes the source Statistics layout and real
row focus/hover tips, Profiles/Compendium independent pixel oracles, localized
profile/history dates, and normal-font locale substitution across frontend,
rooms, reward/tutorial, Ancient and run-end labels. Ordinary events now use
the original text column and option height with dynamic full-page prose;
rest uses the full UI projection; merchant text and turn banners use their
source bands and timing. The rest tutorial shares the ordinary option-picture
renderer. Chomper/Byrdonis multi-hit moves now enqueue each original Attack
animation; OnlyPlayAnimOnce remains a single cue. Short7655 world checks pass;
the full matching runtime gate is still required.

The foreground map and resource-transfer workspaces now share mutually
exclusive storage without reducing their capacities or image caches. The
static object drops from1,412,116 to794,792 bytes (617,324 saved), with explicit
Map-vs-Save/Codec/Bytes leases. Dirty workspace/map/save/LZMA cycling passes
788,050 short checks, and retained-resource callback/reentrancy checks pass
3,403,377. Actual linked ARM allocation remains a separate gate.

The prior cb52b94 RGR2 conversion has completed:689 of840 retained groups are
smaller, all51,123 images per locale decode exactly, and glyphs,8704 strings,
1783 clip clocks and1184 tile/index streams are preserved. That intermediate
split totals92,183,452 bytes,949,484 fewer than its matching input. The final
font append changes the locale payload sizes and must be measured separately;
new output, complete acceptance and ordinary USB migration capacity are pending.

Five stage-A native gates passed (content, content_tiles, split_resource,
boot_locale and library). Its world_ui/world_text failures were traced to two
test-fixture premises: missing original hover artwork and Artifact absorbing
the setup Weak. Both tests retain explicit fallback/source-art distinctions
and now apply the intended canonical power setup. Short reruns pass; the
failed formal receipt remains evidence and is not relabeled a pass.

All3538 pinned C# paths are available for review except five generated or
excluded-platform/multiplayer empty files. Nine in-scope empty bodies were
recovered. Source availability, a directory scan and test counts are not a
95% original-game restoration measurement. Matched native/Python/ARM/WASM,
bilingual rendered scenes, ordinary browser and cloned-OS acceptance, final
private packages and main synchronization remain the delivery sequence.

## UI10 integration resumed from the preserved UI9 source — 2026-10-25

The interrupted Card Library stage is now preserved remotely at
`7332b2f23d0ac41b342326df4eba0e8e6cda13c7`, exact tree
`f3293d412be66221fbd341c98d440de94bb8fb0a`. It is byte-identical to the
previous local `f2e4e83` tree. The new frozen conversion in this turn passes:
English3292 and Chinese32764 glyphs, no missing library labels, unchanged
old image/animation/text payloads. This does not complete the new runtime gate.

The next coherent source stage restores source-sized Profiles labels, original
panel/text transforms, delete-label timing and navigation past empty profiles;
centers the Card Library count; and replaces the legacy combined enemy-intent
panel with the original independent intent/creature hover chain. Intent motion,
number fonts and the localized multiplication sign follow the pinned source.
A bounded glyph-descriptor cache removes repeated binary searches during card
layout and outlined text. Its final performance and ARM budget are still pending.

NSP2/35 appends exact profile and intent fonts without discarding old glyphs.
Its versioned glyph count bound grows to65535, while the resident directory
stays at2MiB and every range/geometry check remains enforced. Old versions
retain their old limits. The required extra entries are only25 English and28
Chinese at this source stage; no font bitmap is silently scaled or substituted.
The final paired resources and exact scene oracles remain required.

Recovered ordinary UI9 save receipts prove the73HP/109gold/floor2 run and
full canonical run/history decoding; the dense three-profile capacity case
also passed. The two raw-migration capacity cases failed actual USB writes,
so neither is counted as a pass. File restoration passed; its last Continue
journal was left running/unknown. A complete live-payload audit found only
about1.4KiB of alignment gaps. A new exact retained-color format is being
implemented separately to address the real migration headroom problem.
It is not included or accepted by this font/UI source stage.

This turn recovered all pinned C# source again for direct review. Source
availability and native/backend agreement are not a numerical original-game
fidelity measurement. Complete source-stage testing, rendered scene review,
normal browser and cloned-OS acceptance, final packages and verified main
synchronization remain the delivery gates.

## Complete matched UI gates and Card Library source follow-up — 2026-10-25

The frozen `bbb47e23e0939bbcf2534ad2ba7d4221e6adce5f` runtime passes all116
native CTest suites. All45 actual private-resource UI targets pass in both
languages:17 world/root,13 front and15 combat,90 results in total. There are
1178 resulting captures, with module-specific composed reviews. The ordinary
browser run now passes18 flows with41 captures, zero page errors and zero
external requests. Its corrected test source is
`66b433cb574f3f0348b34bbde41e7ee4ef7ba51d`; all486 runtime/build files are
byte-identical to the accepted8l runtime. The first browser cursor failure and
native wrapper's wrong-build-directory failure are retained separately from
the successful runs. GitHub's8l run was cancelled by the newer8m checkpoint,
not reported as a CI pass.

Composed browser review exposed another actual implementation gap: Card
Library still used8px labels in its narrow original sidebar. The next source
stage restores original4px bold sorting, rarity and checkbox labels,5px search
and cost text,4px card counts and6px empty results, together with the source
control positions, label-sized sort arrows and selection colors. Card stats
use their original5/6px glyphs at the actual card scales and clip only at the
original label boundary. The old8px temporary bitmap/rescaling path is
removed, including its 5808-byte stack image/alpha buffers. Matching
new-font composed and runtime acceptance is required after this checkpoint.

The required glyph budget was proved against both actual v34 packs. The
selector generator incorrectly counted the literal characters of[red]/[gold]
as4px italic flavor text. All306 Chinese relic flavors were independently
fed through the real C++ RichReader: nine keys `/ [ ] d e g l o r` never appear
as visible output. They were added only by that selector stage, and no other
runtime screen uses the same face/size. The new converter retires only these
nine directory keys, retaining even their old payload bytes. It adds all
required Card Library characters: English3282+10=3292 and Chinese32749−9+24
=32764, within the unchanged32768 limit. The converter verifies all other
old glyph keys/metrics/alpha, every string, image, clip clock and the complete
old payload. The public correction changes the collector, not original text.
The immutable21/22 inputs are preserved; new outputs will be23/24 after the
new source is remotely verified.

The additional retained-color codec is deliberately not installed. A short
native comparison using the actual LZMA2 decoder and once-per-group inverse
preserves all output bytes but slows the measured192 interleaved cold pairs
from3875.739 to4073.144µs on average (+5.093%). This is not an ARM timing claim;
there is no demonstrated performance benefit to justify that storage tradeoff.
Adding the necessary per-group slice table would reduce its full candidate
saving to870,076 bytes. A separate byte-delta sample improves only one of16
groups, by14,891 bytes total, and is not promoted to a new resource format.
The current reader, decoded pools and image payloads remain unchanged. The
offline oracle is retained as future engineering evidence, not a shipped
storage feature.

Ordinary cloned-OS acceptance has cold-booted the real8l launcher, switched
language in Settings, started Ironclad, accepted tutorials with physical5,
walked Neow and the map, completed the three-page combat tutorial, won the
first battle through real inputs, received rewards and completed Save/Quit
and Continue. The continued state shows73/80HP,109gold and Floor2; the reward
scene has no residual hand, energy, piles or End Turn control. Game exit and
OS flush/quit complete normally. Final save-file decoding, capacity cases,
new-library launcher acceptance and private delivery remain separate gates.
Long-lived emulator outputs now reside in `/tmp` until closed: earlier
scratch output paths retained truncated prefixes despite success logs, and
their independent cold-boot reads are retained as distinct evidence.

## Ordinary browser cursor isolation and storage candidate — 2026-10-25

Runtime `bbb47e23e0939bbcf2534ad2ba7d4221e6adce5f` passes the matching ARM
build and12,880 /42 /14 linked checks. The launcher is1,421,585 bytes;
actual allocation is16,755,332 bytes, leaving21,884 under16MiB. The real
history callback passes9469 checks and exports74,001 bytes that independently
decode to the exact295,349-byte NSH8 fixture. Python discovery217 plus the
additional54 tests pass. Matching native and actual-scene groups are running.

The ordinary v34 browser run passes the first15 complete flows, including
Sealed's real four-column selection. Its next assertion incorrectly counted
the visible source mouse cursor inside the relic-icon comparison rectangle.
A normal DOM replay proves that moving the cursor within the still-open paper
restores the rectangle exactly, without any other input. The corrected test
keeps the real icon click, moves the cursor out of that rectangle, and now
requires exact equality. Separate normal-DOM diagnostics also pass right/left
relic navigation, backstop/Escape, Potion focus purity and the visible card
library timer. These are diagnostics; the full formal browser rerun remains
required. This test-only change does not require a different runtime binary.

A private full retained-color experiment preserves every source byte while
reducing candidate LZMA2 payloads by913,893 bytes across474 of840 groups.
The pure offline transform and exhaustive scalar-oracle tests are preserved
here. There is not yet a new resource reader, delivered compressed pack,
device storage saving or performance result for this experiment. A field-plane
shuffle alternative was rejected after its sample saved nothing. Adoption
requires the actual bounded reader, full reversible pack proof and a cold-load
comparison; the current v34 three-file resources remain unchanged.

## Matched v34 gates and final visible repairs — 2026-10-25

The remotely verified source `c3a44adfce84ef6355c89b0038210f58ab63afa2`
(tree `8b48c0cdd67c11476c5ae7754ed4d0f048826442`) passes all115 native
CTest suites and GitHub Actions run40155811647. Python discovery passes217
tests. The first combined local journal then failed because the wrapper named
three nonexistent modules; that failed receipt is retained. A separate correct
invocation passes all54 additional animation, packing and logging tests plus
compileall. These are build/converter results, not an original-game fidelity
percentage.

The complete v34 resources have51,123 images,1783 clips,558 card records,
2061 auxiliary records and8704 strings. English has3282 glyphs and Chinese
32749, within the unchanged32768-entry limit. The bilingual three-file split
totals93,128,740 bytes. The18 Kaiser supplements contain344 original samples;
old image payloads, clip clocks, glyphs and strings are unchanged. Both locale
full packs and their split reconstruction compare exactly. Both complete
Kaiser poses were inspected with the missing source pixels now restored.

Matching ARM is1,419,971 bytes, with actual Zehn prefix plus loaded memory/BSS
of16,753,056 bytes:24,160 bytes below the unchanged16MiB limit. Linked ARM
division/ABI, LZMA2 and TGR checks pass12,880 /42 /14 cases. Matching WASM is
6,632,992 bytes. This is not physical-device performance evidence.

Against the final v34 packs, all17 root scene targets pass in each language,
producing265 captures per locale. Composed review covered direct title,
character selection, the centered map/legend, events, rest, merchant and
owned hovers, treasure, rewards, pause, potions, tutorials and run end. The
first front group passes11/13 per locale; the first combat group passes10/15.
The failures are retained. They expose old test premises: rendering before
bounded combat preparation, advancing through an inspector before its source
exit animation, and comparing event entrance animation as though only rainbow
text could change. The repaired short diagnostics preserve canonical-save
bytes, animation clocks, input barriers and actual text bounds. They require
the next frozen full rerun before being counted as accepted.

Actual visual review also found two implementation defects. Purity's prompt
left `{Cards:diff()}` unresolved; its canonical3/5 source variable is now bound,
with all32 selection prompts checked at both upgrade states. Collection
category headings used8px fitted text instead of fixed source24/inline28
projected to5px. Their complete regular/bold text, source colors and shadow
are restored without truncating the two longest English headings. All20
actual heading variants in each language use existing exact glyph keys; no
resource or glyph-capacity increase is needed. The original fixed4px View
Upgrades labels in deck/world selectors also bypass the general card-text
fitter's5px minimum. Their full labels retain the original control state and
hit regions. Matching full/browser acceptance of these changes is pending.
The calculator's default physical5 Confirm alias is also accepted by the
blocking tutorial dispatcher, consistently with ordinary controls; the
release transition retains tutorial input ownership and save purity.

The first ordinary browser run passed14 entry, settings, profile, tutorial,
save/continue and language-persistence checks before its Sealed navigation
fixture repeatedly visited four cards: Right wraps within the source's
four-column row. The corrected fixture uses actual Right/Down navigation,
checks that nine cards cannot confirm and that ten commit only on release.
This is a test repair, not an alternate release navigation behavior. A new
ordinary-browser run must use the next matching WASM and final v34 resources.

The exact highlight optimization has now passed748 old/new whole-frame pairs
(57,446,400 RGB565 pixels), with identical canonical State, hits and presentation.
In an isolated native warm-render measurement,1920 interleaved old/new pairs
per locale show mean1477.545→1446.472µs English and1468.639→1445.731µs Chinese
(2.103% and1.560% reductions). Warm resource read calls and bytes are zero.
This is the measured scene on the native memory provider, not ARM FPS.
A16-of732 tile-group context sample saved only592 of936,431 compressed bytes;
it is not a full-pack saving and does not justify another resource rewrite.

Calculator history now shares the existing512-byte streaming NSZ1 envelope
with runs, preserving canonical NSH1–8 and raw fallback. Its valid full NSH8
fixture compresses295,349→74,001 bytes and decodes exactly. One process byte
tracks existing full-load recovery facts; a known malformed primary cannot
overwrite the valid recovery backup. Fault injection covers normal, unknown
I/O, invalid-primary, missing-primary, rollback and retry paths. The short
actual-callback test passes9463 cases; matching native/ARM and ordinary device
execution remain required. No resident buffer or startup integrity scan is
added. See `run-storage-ui9-0.17.md` for exact limits.

Capacity cases must remain distinct: with these measured packed histories and
dense runs, a three-profile history-update peak is1,025,362 bytes; retaining
three conservative old raw run backups raises it to1,419,637. The first update
with three old raw full histories instead peaks at2,083,681, and the all-raw
conservative bound remains2,699,304. Filesystem costs are additional. Rounded
OS free-space labels are not exact capacity evidence. The ordinary cloned OS
is cold-booted and its game directory is empty; final four-file installation,
actual play/save/continue, capacity write/readback and restoration are still
open. Final calculator ZIP/offline HTML delivery and verified main sync follow
those gates. Original assets and captures remain private.

## Kaiser crop completion and bounded selector fonts — 2026-10-25

The matching `f3585dc5f8be8c12ca76187d130b4e556fd9b946` native build now
passes115/115 CTest suites. Its ARM launcher is1,419,446 bytes; the actual
serialized Zehn prefix plus loaded memory/BSS is16,752,616 bytes, leaving
24,600 bytes under the unchanged16MiB limit. Linked ARM division/ABI,
LZMA2 and TGR checks pass12,880 /42 /14 cases. The matching WASM is6,632,532
bytes. These are build/runtime gates, not original-game visual parity or
physical-device timing evidence. The Python suite passed211/212 before
rejecting two unregistered Bestiary fallback labels; their locale bindings
are repaired here, with the focused six localization tests passing.

Source-based composed inspection exposed a real Kaiser Bestiary crop: the
shared world animation ends at a320×218 rectangle, while the original
Bestiary translation shows pixels outside it. The next resource profile is
NSP2/34 with18 supplemental clips /344 frames, bringing the exact animation
directory to1783 entries. Only visible original pixels outside the retained
world crop are appended at native sampling coordinates. Original world
frames, frame clocks and earlier directory IDs remain unchanged. The runtime
draws sparse supplements using the existing image cache and no new pixel
buffer. Both full/split readers and the boot language hint support the new
profile; old v33/1765 profiles remain accepted. Focused format checks pass,
including the unchanged32768 glyph limit. Full generation, composed capture,
new ARM allocation and ordinary-host gates are still required.

The private recovery chain passed stages6–18, including lossless retained,
glyph, string and tile compression. Stage19 correctly stopped when selector
font insertion would produce32777 Chinese glyphs, nine over the existing
bound. The cause was adding0–9 to every static label/title/flavor even when
none occurs in that text. The converter now adds dynamic numeric characters
only for template-bearing text and preserves every literal character and
every old glyph. Exact budgets are English3189+93=3282 and Chinese32021+728
=32749, leaving19 entries. Seven focused converter tests pass. The full
selector/split rerun remains pending this checkpoint.

The ordinary merchant/room regression passed14,606 checks in each language
against the recovered stage18 resources, and the actual stock relic/potion
and owned-relic hover captures were inspected. This is an early room-specific
check; final selection resources and the complete bilingual run still need
their matching acceptance. Ordinary calculator installation, maximum-save
capacity, whole-frame highlight timing, offline browser flows and final
private delivery remain open. No restoration percentage is inferred from
these counts.

## Integrated follow-up candidate — 2026-10-25

The next runtime stage corrects additional visible differences found while
reviewing the original scenes and methods. Bestiary now follows the original
list/header geometry, fixed font bands, controls, per-move animation preview,
and physical key release semantics. Mode preference is persisted on submenu
close. Its presentation structure increases392 bytes; canonical State and
save formats do not change. The shared Kaiser frame registration still needs
actual composed inspection after its corrected0.75 source transform.

Card descriptions now assemble the original keyword prefix/suffix order,
including the missing Purity / Purity+ / Scrawl+ Retain paragraphs, mutable
Retain/Sly/Ethereal and Hexed, detached enchantment facts and localized Replay.
Only complete keyword paragraphs are replaced; prose mentioning the same
words remains. Cookie relic icons now distinguish canonical/no-owner process
variation from actual player/history ownership; collection outlines use that
same process variant. This adds one byte of process UI state, no gameplay RNG
draw or save field.

Ordinary merchant relic/potion stock now uses the complete original model
hover set and source-sized columns, replacing the remaining174px legacy box
and five-line cutoff. Associated cards and keywords, detached stock values,
modal/input suppression and save purity have focused regression coverage.

The highlight renderer skips source samples proven transparent by an exact
32-bit threshold. The old scalar shader remains unchanged;12,592,419 oracle
checks pass. An isolated actual-SDF kernel improved23.41% /34.25% in two
scenarios; whole-frame and ARM timing remain unmeasured. Zehn's allocation
gate now includes its serialized prefix as well as loaded memory/BSS and
rejects overflow before replacing an existing output. The unchanged16MiB
boundary passes two synthetic ELF cases. Source syntax and pure checks pass;
all changed runtime code still requires the next matching full/native,
actual bilingual, browser and ordinary calculator gates.

The exact-font source is already preserved at
`4e8a6e5377eed0b869de24a829555a5259377e7e`, tree
`e5ef51a959f85475e3807579fa2504ff133e34a9`, with remote ref readback. The private
conversion chain has passed fonts, death append, history/journal/transition
and tooltip glyph stages. The final compression/selection/split chain remains
running, and is not yet a final resource acceptance result.

## Recovered full gates and Bestiary font stage — 2026-10-25

The matched `3e61c041ffdc02e82ea8f8e14dc27601e2751aac` native build passed
113/113 CTest suites. The wrapper's redirected console log is incomplete;
the independent complete CTest LastTest.log contains all113 passes and no
failed test. Python passed209 tests plus54 animation/packing/logging checks.
These are native/converter gates, not original-game visual parity evidence.

The original input chain has been reconstructed through v28 history art,
and all1006 character-death frames /12 clips have been regenerated. A private
recovery archive preserves the split stage5 pack, death cache and exact font
and converter inputs. It is a development checkpoint, not the final release.

Bestiary source review found the MonsterName uses Spectral Bold48 (native10),
Actions/Mode uses bold28 (native5), act/event dividers use bold32 (native6),
and dialogue uses italic28 (native5). The font converter now adds only the
actual reachable titles and labels at those exact face/size bands. A focused
font-band regression passes. The full private font build must establish the
final glyph count against the unchanged32768-entry limit. Bestiary geometry
and input, Retain description, Cookie ownership, highlight and allocation
repairs are still being integrated separately from this font-only stage.

## Environment recovery and texture-bound repair — 2026-10-25

The complete selector/inspection source is preserved at
`1e50404901ab398574fcf8fa44d863e8961248ef`. The transient executor was recycled
while its native, ARM, WASM and resource jobs were running. Their missing
completion records are **unknown**, not passes. The repository was restored
at that exact commit. The original owner PCK/DLL and previously delivered UI6
split resources were recovered from the saved original archives; their source
identities were verified. Newer private packs and conversion caches must be
rebuilt, then persisted privately before further long acceptance work.

The relic resource append failure is now reproduced against the exact original
PCK: the source frame texture is408×408, whereas the scene's containing Control
is304×304. The converter confused those bounds. The repair accepts the actual
408px texture, records both bounds and retains the58×58 projected frame.
Runtime inspector geometry remains based on the304px scene Control.
The corrected conversion and final composed acceptance remain required.

The recovered remote CI run
[40118639939](https://github.com/maxmixalex/slay-the-nspire-2/actions/runs/40118639939)
built native, ARM and WASM. Native passed102/113 suites; converter tests
passed208/209 before stopping. The eleven native failures were traced to
test premises retained from the replaced selector/inspection UI: source
entrance/exit clocks, focus-driven hand layout, the original Back action,
filtered nonremovable cards, and source single-candidate shortcuts. The
repaired tests retain gameplay, invalid-input, save and pixel assertions.
Their individual Release reruns against the frozen1e504 core all passed;
the next full matching suite remains required. The Python failure required
the obsolete text-checkmark glyph although original checkbox images had
replaced that literal; the corrected symbol coverage check passed.

Recovery also rebuilt the exact1e504 WASM (6,612,714 bytes), the public
Firebird controller, original fonts and fixed official Spine dependency.
The original CXII NAND/boot identities were recovered; an isolated NAND
clone boots the ordinary OS6.4.0.74 and has installed Ndless. Original NAND
bytes remain unchanged. This is environment preparation, not final game
execution. Actual regenerated NSH8/dense-NSZ capacity fixtures match their
historical hashes and sizes:295,349 /121,503 bytes; final OS write/readback
and three-profile capacity acceptance remain required.

## Active objective and continuity

Continue 0.17 on `work/0.17-world-parity`: pinned original UI fidelity first,
calculator performance second, lossless storage third. Reference remains
**v0.111.0 / 41cef1ea**. Audio, multiplayer and Timeline are the only agreed
exclusions. The user's five reported defects were startup language selection,
legacy expedition settings, potion destination selection, a small merchant
inventory and a left-shifted map. Their runtime paths were corrected earlier
in this work; final composed acceptance is tracked below.

The user authorized source checkpoints, GitHub Actions and synchronizing
verified progress to `main`, plus delivery of a calculator ZIP and bilingual
self-contained offline HTML. Original game assets, PCK/DLL, fonts, emulator
images, generated packs, captures and embedded HTML stay private. Do not claim
an original restoration percentage from path inventories or passing tests.
Do not treat probes as implemented functionality. Preserve each coherent source
checkpoint remotely, read the ref back, freeze with `git archive`, then run
long jobs with `tools/run_logged.py`. Clang 18 is the verified native compiler;
a GCC 13 internal compiler error was recorded earlier.

The previous 300 KB chronological work log remains exactly available in Git:
[complete earlier stage record at d938a661](https://github.com/maxmixalex/slay-the-nspire-2/blob/d938a661c7a69e46b829a4a879ce6cd411be0f2b/docs/WORK_STATE.md).
This shorter current record preserves active decisions and verified boundaries;
it does not discard that history or replace module-specific evidence.

## Integration status before the complete selector/inspection checkpoint

The latest complete runtime/resource build is stage8e, `492915341106b129f730e46ed84d152637a24b7d`.
Its native/ARM/WASM and resource builds passed; the detailed acceptance below
retains the initial FTUE and map-window fixture failures and subsequent short
repairs. The earlier a0df measurements in this document are historical, not
the current source. No final artifact from the new selector/inspection work
has been delivered yet.

The coherent core checkpoints `1606be12622f1bffe8b557ed6ee01b8781b32429`
and `5afb7a9aa8b719b31dcd282c203f43b4341ce7c1` are preserved remotely with
exact tree/ref receipts. The complete integration adds original combat,
standard-deck and world selectors; automatic all-eligible source shortcuts;
pause and potion entrance/exit behavior; original relic inspection; and the
FTUE arrow orientation correction. Their independent short checks have
passed, but require the next matched full source/resource/ordinary-host gate.

The 5afb resource receipt is an intermediate v33/2060 result. The candidate
v33/2061 converter uses12×12 upgrade arrows, the source card-frame shader
input and58×58 relic frame,32 original prompts and append-only native glyphs.
Its five public synthetic data/format checks passed. The selected-card shader
optimization passed5,317,924 exact old/new scalar and coordinate comparisons;
ordinary ARM timing and final linked-code review remain separate evidence.
No renderer mutates canonical gameplay to satisfy these comparisons.

## Historical verified runtime and resources before stage8e

Runtime `a0df79d8347b76dc86804b0ab8d17f948c600204` passed **103/103 native**
suites and **201/201 Python** tests. Matching ARM and WASM built. Its launcher
is 1,350,761 bytes, SHA-256
`4e49fd48fad18fac98676fe0a0876e96a4d94d6e2f6b48f7994b7fbcea922ea3`;
allocation is 16,637,436 bytes under the unchanged 16MiB gate. Linked ARM checks
passed 12,880 division/ABI, 42 LZMA2 and 14 tile-filter cases. Linked checks are
not physical-device frame-rate evidence.

Actual final-v33 card/HUD runs passed 359,342 English / 383,522 Chinese checks.
Power/keyword/associated-card runs passed 635,832 per locale. The final root
scene pass executed nineteen targets per locale: eighteen passed; the only
failure was the map-window fixture opening combat input before explicit asset
preparation completed. Its source repair waits only for that bounded phase,
asserting unchanged canonical State and animation, and keeps all map/layout/
route/alpha checks. Formal matched re-execution remains required.

A subsequent multiple-image preview suggested missing GameOver banners and
buttons for some characters. Direct complete-region comparisons proved this
was a preview interpretation error: original PPM pixels match exactly across
Ironclad/Silent/Defect/Necrobinder in both locales. The source texture is present,
alpha is 255 and Content is healthy. Runtime drawing was correctly left alone.
An added opaque-source-pixel oracle passed short frozen-core checks (33,531
English / 32,814 Chinese); it needs the next formal matching suite run.

Resource conversion through `424c06c6d29f8230fcbac8969627fe20cea0a593` produced:

| Component | Bytes |
| --- | ---: |
| Common | 84,719,566 |
| English | 2,153,050 |
| Chinese | 5,149,970 |
| Three resource files | 92,022,586 |
| Including a0df launcher | 93,373,347 |

NSP2/33 has 2051 auxiliaries, 8704 strings and 1765 clips. Old original pixels,
alpha, frames, timing, glyphs and locale values remain exact. Grouped glyphs and
strings reuse existing cache/transfer memory. TGR2 keeps 728 smaller color
groups, preserves four non-saving TGR1 groups, and saves 3,733,886 common bytes;
443,044,043 decoded bytes per locale were compared. Twenty-four missing original
Chinese bold 5px title glyphs added 1,608 bytes. All 2,783 hover fields per
locale have zero missing required glyphs. An extra literal-context sample saved
only 632 bytes over 24 groups and was rejected; no further image degradation or
production context expansion was used.

Keep original v31, grouped-font v32 and final-v33 resources and exact receipts.
Superseded old packs/HTML and obsolete native builds were retired only with
file-hash manifests; captures and acceptance records remain. Main has not yet
been synchronized with this active branch.

## Ordinary calculator execution and save capacity

Final a0df files passed ordinary USB transfer and exact readback in a disposable
Firebird OS clone, preserving the original NAND. OS display: 92.3 MB total,
2.4 MB remaining after installation. The ordinary flow passed Chinese title →
in-game Settings/English → Standard/Ironclad → Neow Golden Pearl → centered
map → five-turn real combat victory → eleven gold and Twin Slam → Save & Quit
→ Continue with **69 HP, 260 gold, eleven cards**, then normal OS return.
Raw NS17 primary/backup were both 230,540 bytes; history 17,729 bytes. There
were zero reported resource failures. No developer commands or RAM injection
were used. Journal `ui9-firebird-final-flow-fdd5d6335ab8` is passed.

Installation alone did not guarantee three full profiles plus temporary writes.
A full NSH7 history is 295,347 bytes; a conservative canonical v17 run bound is
252,928. Existing ordinary backup semantics require a history-write peak of
`4H + 6R + 340 <= 2,699,296` bytes before filesystem allocation.

Checkpoint `d938a661c7a69e46b829a4a879ce6cd411be0f2b` adds the **NSZ1 host
storage envelope**. Canonical NSV1/v17, RNG, history and backup identities remain
unchanged. A two-pass zero/literal stream uses a 512-byte foreground stack
buffer. It writes compressed bytes only when strictly smaller; old raw saves
still load, and incompressible files never grow. No new pool or integrity-only
hash is introduced. Exact-host tests passed **1,410 checks**, covering old v3–13
fixtures/raw v17, canonical equality, short/truncated/oversized/trailing input,
close/write/rename failures, existing leases and primary-only loading policy.

A dense valid State with all live card/relic/reward/hook capacities and nonzero
metadata compresses **250,964 → 121,503 bytes**. Three such runs and full NSH7
histories peak at 1,910,746 bytes; retaining three conservatively bounded old raw
backups during migration peaks at 2,305,021 bytes. These are explicit fixture
bounds, not a claim that every arbitrary accepted raw payload compresses so
well. The unconditional raw fallback bound remains 2,699,296 bytes. See
`run-storage-ui9-0.17.md`. Ordinary8d readback now passed: the old230540-byte
raw run continued, and two explicit saves produced7456-byte NSZ1 primary and
backup. Both independently decode to the exact original canonical bytes. A
USB-session timeout remains recorded as failed; a restarted normal OS session
completed readback without changing any save bytes.

The d938 ARM build and all three linked gates pass. Launcher: 1,352,081 bytes,
SHA-256 `b7e453c9646b117dca2832872a019909b772a43a54b949614575d2008de7bb5b`;
allocation 16,639,308, remaining 137,908 bytes. Production ARM uses `-O2`.
Tile filter mode dispatch is now outside its pixel loops: the actual linked
function selects mode once per group. This is an executed code improvement;
static instruction differences are not physical-device timing measurements.

## Implemented UI and performance work

The source-led changes include full standard/custom/daily setup and twenty
modifiers; three profiles; original native fonts; original large merchant;
centered retained-room map, drawing and per-floor history; actual terminal
combat reward ownership; original card/relic/potion/keyword hover columns;
all five normal/merchant Die tracks and separate Defect/Regent particles;
source GameOver intro/summary/score/badges/discoveries; original reward/rest/
treasure/Ancient/Crystal flows; real preferences, timer, rebinding, feedback,
notes and credits. Details remain in the corresponding module audit documents.

Latest source corrections include the original card-rule resource cross,
preview-only changed upgrade-cost green/outline (permanent upgrades remain
ordinary), ninety-nine Power ExtraHoverTips with independent-instance values,
source Creature anchors/debuff skins, and adjacent associated-card placement.
Energy/stars/orbs/settings/map legend/drawing/GameOver tips use native 69px/5px
source projection. Proceed uses source YIQ HSV and independent hover/disable
motion with no per-button allocation increase.

Executed performance improvements include 200,368 bytes less State/codec BSS;
map workspace 1,114,120 → 794,788 bytes with 1024 exact graph/RNG comparisons;
merchant occlusion with 2376 exact full frames per locale and quiet native
steady-render reductions of 28.74% / 19.94%; and history glyph/string cache
indexing with 7200 fewer ARM bytes, 2160 exact frames per locale and zero warm
provider I/O (prior Chinese repeated-pass requests were 28.7 / 14.3 MB).

Same a0df runtime with v32 versus final-v33 packs produced **3246 exact frames
per locale**, unchanged State in six scenes, and zero reads in every final warm
window. Three quiet cold repeats show first/preparation increases of roughly
0.68–5.52 ms native (2.3–20.1%); whole-v33 grouping changes add 3–11 read requests
and 19.8–70.5 KB in those cold cases. Do not attribute this whole-pack result to
TGR alone. The subsequent actual ARM mode-loop improvement needs final timing.
The ordinary Firebird report's map mean includes cold/overlay frames; isolate
that cost before changing source geometry, frames or cache pools.

## Newly found required gap: singleplayer tutorials

The broader source audit found thirteen singleplayer FTUE screens previously
suppressed as already seen. They were not an agreed exclusion and are now being
implemented. The tutorials are accept/decline, three-page combat basics, map
selection, cannot-play, still-can-play, power card, shuffle, combat reward,
potion, relic, rest, merchant and singleplayer Ascension. Ascension uses
`SeenPopup` independently of the general tutorial enabled flag.

Resource tools append seven original images at auxiliaries 2051–2057 and thirty-
one original texts at reserved slots 8347–8377, preserving all old payloads and
keeping NSP2/33, 8704 strings and 1765 clips. Three focused converter tests pass;
the first production conversion passed with zero missing required glyphs.
Runtime layout review identified the source4px standard body requirement.
The small-font production attempt `ui8-ftue-smallfont-production-afb7039c49d3`
failed the older tool glyph-size whitelist before writing a final receipt.
The corrected v33-only validator and independent glyph-preservation test are
ready; rerun from the new frozen source before using the pack. See
`ftue-resources-ui9-0.17.md`.

The profile candidate adds only two serialized bytes in **NSH8**: thirteen seen
bits, enabled, initialized and a rejected reserved bit. Earlier NSH1–7 decode
to a compatibility state without fabricating seen history. Actual new empty
profiles enable tutorials after the existing save lookup; no new startup scan
of all profiles is added. No-persistence native fixtures keep legacy behavior.
Source release `NGame.IsReleaseGame()` is true; Reset Tutorials and its divider
are hidden in the pinned scene and remain hidden in the ordinary port.
The complete trigger/input/render candidate and its native/browser regression
sources are ready for the next combined freeze. The source4px production append
passed at f8701249 with zero missing glyphs: English3181 and Chinese31675 glyphs.
The final append also adds the original display face for the post-tutorial
Battle Start banner. No tutorial completion claim is made before integrated
execution.

Ordinary first-victory review found an additional concrete defect: the reward
list retained hand, energy and End Turn beneath it. `finish_combat_feedback`
mistook terminal WorldRelicChoice(parent Reward) for a suspended combat choice.
The candidate completes that trace, allowing the existing source-shaped
`combat_reward_room` to render. A new real select/aim/Strike regression against
the frozen a0df core reproduces the exact stuck-trace assertion after its full
canonical-save oracle has already passed. Matching execution of the repair is
required; the earlier all-scene declaration is not treated as proof of this
previously missing transition coverage.

## Remaining gates and boundaries

Finish concrete FTUE display/trigger/input integration and review every new
bilingual scene. Preserve source before production conversion/builds. Re-run
matching native/Python, final ARM/WASM, actual-resource map and GameOver oracles,
new-profile tutorials, save/Continue and the complete offline-browser flows.
Complete any measured map-render optimization without changing pixels/State.
Then build matching calculator ZIP/offline HTML, verify them, save the files for
the user, update evidence/audit statuses and fast-forward verified source to main.

The source inventory enumerates 3538 C# paths, 724 node paths and 1022 scene
paths; empty decompilation outputs have relevant recovered method sources.
Path enumeration is not review of every method. GPU noise/turbulence, some
secondary particles/trails/material variants, randomized Cookie/creature visual
variants and every monster-specific timing path still lack full differential
proof. Original-game paired captures and physical-hardware measurements are not
available. Do not convert adapted 320×240 rendering or emulator results into an
unsupported “95% original fidelity” assertion.

## Stage8e integrated acceptance — before fixture-only checkpoint

Runtime/resource checkpoint `492915341106b129f730e46ed84d152637a24b7d`
built native, ordinary ARM and ordinary WASM successfully. Python passed
204/204. Initial CTest passed105/106: only the new FTUE fixture failed.
Initial bilingual actual-resource acceptance passed20/21 targets per locale:
the remaining map-window failure was the last reset/wrap fixture starting
before actor preparation. All runtime sources remain unchanged by the fixture
repairs. Complete tests are retained, including canonical State and animation
purity during bounded preparation, source reward-card entrance timing, and
strict synthetic tile-directory bounds. Short repaired FTUE checks passed
59564 synthetic /5987 English /5284 Chinese; repaired map passed82678 per locale.
Formal execution must use the next remotely frozen test checkpoint.

Actual terminal select/aim/Strike now passes full canonical-save equivalence
and closes the combat display queue. Both actual locales pass4076 reward
checks, with inspected captures showing no hand, energy, piles or End Turn
beneath the terminal reward list or card choices. The GameOver source-pixel
oracle and all other actual scene targets pass.

Final FTUE resource conversion and byte-identical split roundtrip passed.
The seven original images,31 texts and native4px/18px font additions retain all
old payloads. Common84772852 + English2160488 + Chinese5184978 =92118318 bytes.
The ARM launcher is1370912 bytes (SHA256
`17634b518ef85b3a46ac4d1d42da7205e32e281ab3e71b2f44a518cd63968918`);
allocation16672244 leaves104972 bytes below16MiB. Linked12880 division/ABI,
42 LZMA2,14 tile-filter and9110 additional signed32-bit division/remainder
checks pass. These do not establish physical-device FPS.

Per-profile formal storage checks pass6413 core/export plus259 Ndless cases
and the actual exported NSH7/8 browser transaction tests. Full NSH8 is295349
bytes. The final ordinary offline HTML passed16 browser flows and27 captures,
including actual Yes/No, three tutorial pages, final-page marking, real Neow
and map travel, v17 first-turn save/reload/Continue without repeat prompts,
three-profile isolation and deletion, Settings language/fast/timer persistence.
No page errors or external requests occurred. The first Chrome launch was
blocked by sandbox setsockopt; its failed receipt remains distinct from the
subsequent approved local-only pass.

USB upgrade exposed a real temporary-storage constraint: replacing the old
Chinese sidecar while it still existed failed with-255. Removing only that old
resource and sending the new matching sidecar succeeded. The install session
remains recorded as failed; subsequent readback/startup uses a new gate. The
release instructions now explain removing all three old resource sidecars
before replacement while retaining every save/profile file. Final ordinary
map timing, new-profile first victory, three-profile capacity and final
source/main/artifact delivery are still in progress.

## Stage8f core/resource checkpoint — UI integration still in progress

Independent source review identified active legacy standard-deck, combat
selector, world-choice, pause and potion-popup presentation. These are now
being implemented in parallel, not deferred as a diagnostic-only result.
The core/resource checkpoint preserves the selector command shortcuts,
32 private original prompt bindings and a two-image append-only converter.
See `docs/selection-ui9-0.17.md` and the exact caller audit in
`docs/combat-selection-source-0.17.md`. The converter's four synthetic data
contracts and short Clang syntax checks pass; formal core execution and
private conversion start only after its branch ref is verified remotely.
The complete UI worktree remains uncommitted until those agents finish and
must receive its own remote checkpoint before the final full builds.

An additional actual FTUE pixel oracle found the relic arrow missing its
source vertical flip. The narrow correction changes870 pixels per locale;
the old parameter fails the independent orientation check and the corrected
private-overlay diagnostic passes6113 English/5410 Chinese checks. This is
a runtime correction, unlike the earlier FTUE/map fixture repairs. Its
formal final gate remains part of the complete UI checkpoint.
