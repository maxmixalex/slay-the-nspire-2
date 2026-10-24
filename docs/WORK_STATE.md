# Work state — 2026-10-24

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

## Current verified runtime and resources

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
