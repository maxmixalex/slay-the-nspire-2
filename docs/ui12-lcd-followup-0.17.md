# UI12 calculator usability follow-up

Reference: the owner's v0.111.0 / 41cef1ea. UI12 continues the readable UI11
adaptation; it does not revert to the earlier small desktop projection. Original
art, model rules and source interactions are retained, with deliberate reflow,
clipping, scrolling and physical-key changes for 320 × 240.

This is the release integration report. The interrupted workspace was recovered
and its source preserved on GitHub before validation. The delivered runtime is
`9398633d8fd715c008560cc038f61d8f85c98466`; verified source is synchronized to `main`. Exact
launcher/resource hashes, sizes and closed validation scopes are recorded in
`validation-ui12-0.17.json`. This release uses ordinary Ndless/Firebird acceptance
and contains the calculator build; no manual WASM/browser validation ran.

## Implemented feedback

| Area | Implemented behavior |
| --- | --- |
| English spacing | UI40 stores natural 1/64-pixel advances, negative bearings and source pair kerning. Fractional positions survive measuring, wrapping and painting; rounding occurs at the LCD pixel boundary. Kerning follows the actual fallback font and does not borrow absent pairs from another family. |
| Settings | Controls occupy the left pane; tooltips have a separate right lane. The dropdown arrow is centered. Original tabs, scrollbar track/ends/thumb and pointer click/drag are used. |
| Disappearing buttons | The original HSV material path now tiles oversized controls within its existing 64 × 48 workspace. Credits, Settings tabs, card filters and Proceed no longer vanish or become a solid highlighted face. |
| Credits | Role labels align toward the center gutter. Complete 12px heading glyphs replace sparse heading subsets. The Nspire/zyf722/Codex credit follows Trailer and precedes FMOD. |
| Title | Corrected atlas rotation restores the upright spire. Logo layers move up 12px and menu rows 24px; stacked fog opacity is reduced. |
| Shared Back button | Original artwork is reduced to 47 × 32, with a 40 × 24 hit area. Compendium buttons, statistics and history clear its left-hand area. |
| Compendium | Artwork and text have interior margins; descriptions use readable text inside their borders. Focused long names/paragraphs scroll within their own clipped area. |
| Statistics/history | Containers begin at x=52. Unsupported achievements/Timeline and unknown dash/N/A values are omitted. Actual potion discovery remains. History reflows metadata, card cells and icon columns. Statistics tips reserve a right lane for the focused value. |
| Bestiary | Background dimming and original region/rarity colors are restored. The mode button is 118 × 22 with 12px text. |
| Card details | Cost and Stars have thin four-sided outlines, names sit higher in the banner, and type labels use the correct locale/font role. Filters retain their original material textures. |
| Character selection | Sixth Random choice, original visible character order, source RNG selection, reveal and departure masks. Canonical character IDs remain unchanged. Description padding, clipped scrolling and centered outlined Ascension text are corrected. |
| Necrobinder | The head and both purple-flame contexts are reconstructed from the original scene/Spine attachment coordinates; all 96 affected frames retain their original timing. |
| Ancients | Background seam closed, floating entrance title raised, settled room name/epithet at the upper right. Full options and the existing dialogue area scroll in place. A pointer choice takes keyboard-reading focus on its first click. |
| Top bar | Room/floor no longer share space with the timer; time is in the floor tooltip. Portrait hover includes character/Ascension information. Five visible potion positions and paging cover the engine's sixteen slots. |
| Acquisition flashes | Potion and relic effects use the same current page/grid anchors as the actual destination icons. |
| Map | A compact bottom-right Legend button opens the correctly spaced original-symbol list. Drawing tips clear the tool buttons and display the current physical binding. Close availability follows the source floor condition. |
| Calculator keys | Pad-edge directions, center selection, Tab/Doc/Home/Del/Var/Menu/Shift/Ctrl bindings and direct 1–0 hand shortcuts. Labels use TI's official NspireCX key glyphs. Exact untouched old defaults migrate; customized banks are preserved. |
| Battle opening | Battle Start precedes Player Turn. Whole banners are raised with the combat scene so an early aimed card stays readable. Original clocks and restored-run suppression remain. |
| Combat layout | Creatures, bars and hitboxes move up 14px together. Energy/Stars, pile icons/red counts, hand counts, exhaust and End Turn have separate readable positions. |
| Hand behavior | Ten direct slot shortcuts, thinner blue highlights, no empty-hand placeholder, bounded fan/focus positions. A selected/aimed card and its interpolated path remain wholly inside the LCD. |
| Failed plays | Real failed card attempts show the localized thought bubble with source smoke artwork. |
| Target previews | Enemy-specific damage appears only on the card actually aimed at that enemy; an unselected card/inspector cannot inherit a remembered Vulnerable target. |
| Loot | Reward opening is independent of the corpse-animation tail after the real effect endpoint. Both ordinary and world-choice reward paths use the same inset panel, left-aligned rows and focused long-label scrolling. |
| Proceed | Original face material and separate highlighted outline are preserved; pointer hover and click remain active. |
| Merchant | Normal 65 × 91 stock cards use the existing exact card-face cache, including the rightmost stock position whose capture window is now kept within the LCD. Opaque stock coverage skips only fully hidden rug/background work. Rug/stock/hand positions and card-removal hover are corrected. |
| Events | Body text uses an in-place original scrollbar, keyboard reading and pointer dragging. There is no redundant Details/Return-to-event reader. Option labels stay clipped to their original button texture. Finished Fake Merchant prose re-enters that same reader. |
| Rest/Smith | Rest outlines fit the enlarged action images. A real committed upgrade drives card growth, hold, golden flash/sparks and flight to the deck, using the existing face cache. |
| Console | The normal calculator build includes the typed original command console, toggled by Flag. Editing, history, Tab completion and actual model/state commands replace the old preset menu. |
| Architect ending | Existing original player and Architect clips now form the VisualOnly room and remain at their authored positions through the ending. Source first-line waits, 400ms speech reveal and independent option clocks are restored. Actual English/Chinese dialogue and per-line labels use the original ancients table; long prose scrolls within the lower bubble. The original-position player death replaces the earlier centered terminal stand-in. Compatibility text waits for its proper quote stage. |
| Hidden preparation work | Empty-clip merchant preparation still resolves/caches its glyphs, then skips their invisible raster spans. Existing exact pixel checks and actual seven-slot preparation pass; no additional heap, cache or asset payload is introduced. |

Detailed command semantics and offline/platform limits are in
`developer-console-ui12-0.17.md`. Combat clocks, observer invariance and exact
card-cache checks are in `combat-ui12-0.17.md`; frontend material/layout
contracts are in `ui12-front-feedback-0.17.md`.

## Storage and allocation

The final shared UI40 resource is **61,669,167 bytes**. English is
**1,130,274 bytes** and Simplified Chinese **5,114,322 bytes**. The delivered
launcher is **1,600,738 bytes**, so the four-file default installation
is **69,514,501 bytes** before saves. ZIP size is not the installation
size. All sixteen language resources together with the common file occupy
91,975,031 bytes; the default installation includes only English and Chinese.

The original 37-frame, 12 Hz sampled studio-logo sequence adds 165,511 shared
bytes. The three original swarm/sleep sprites add only 2,284 shared bytes.
The existing 1,783 clip clocks remain intact. Optional auxiliary endpoints
2161 / 2198 / 2199 / 2201 retain exact format gates; the partial 2200 extension
is rejected rather than being treated as a complete feature.

UI10 installed 91,604,965 bytes; accepted UI11 N installed 68,776,461 bytes.
UI12 is slightly larger than UI11 because it restores missing artwork and adds
correct fonts/keys/console behavior. It remains within the user's 65–70 MB
default target. This is not a claim of further net shrinkage from UI11.

The Necrobinder repair initially required about 8.42 MB when appended as sparse
frames. Retiling its replacements, reclaiming 18,908 dead tiles and appending
only 595 new tiles reduced that repair to 98,098 bytes. 699,290 protected tiles,
544 unchanged index groups and all unaffected text/glyph payloads/clip clocks
were checked. The accepted UI11 animation precision is retained; this does not
introduce another blanket color reduction or remove frames.

All sixteen original locale editions are available independently. Installing
all of them at once exceeds the default calculator storage target; install only
the desired languages with the one shared artwork file. Original missing
translations and calculator-only text retain the already documented fallback
in `localization-ui11-0.17.md`.

The allocation ceiling stays **24 MiB**. The delivered launcher requests
**20,363,472 bytes**, including the existing bounded 5 MiB image and
6 MiB tile-group caches. Actual ordinary Ndless startup and saved-run Continue
are required acceptance evidence; the ceiling is not itself proof of execution.
The historical UI11 23.87 MB allocation failure remains recorded. This release
does not claim that Ndless accepted an arbitrary contiguous 24 MiB allocation.

## Recovered integration acceptance

The complete final registered native suite and Python conversion/packaging
results are recorded against the final frozen source in
`validation-ui12-0.17.json`. Native checks establish functional, pixel-cache,
format and canonical-state contracts; they do not establish calculator speed.

The final actor follow-up uses runtime `9398633` and test checkpoint `1848338`;
their only tree difference is the independent Architect regression source.
All **135 registered native tests** pass, as do **310 Python tests** and tool
compilation. Current actual-resource gates pass **12,594** combat UI checks,
**223** merchant-preparation checks and **2,122,238** English/Japanese
camera/sleep/lifecycle checks. The separate Architect gate passes in English
and Chinese across five characters and Normal/Fast mode: **20 combinations**,
7,279 assertions in each language, including canonical-state comparisons and
pixel checks. These counts are not counts of independent gameplay features.

Actual actor validation exposed an empty dialogue lookup and premature
partial-history text. Both were fixed in production; the original pixel and
text assertions remain. Test-only corrections preserve the rule that completed
runs are excluded from ordinary Continue and that the Silent's 1,867ms death
clip is shorter than the terminal UI's independent 1,950ms gate. One complete
native attempt could not start two host executables because their execution
bits were missing. Its failure remains recorded; after repairing file modes,
the complete 135-test rerun passed. No unstarted test is counted as a pass.

The actual final-resource work at `44ebf0e` separately passed all sixteen locale
editions (53,072 model/event descriptions, no missing visible codepoints or
unexpanded templates), startup/SkipIntroLogo, incremental merchant preparation,
English and Chinese combat, exact merchant occlusion, and the dedicated UI12
battle/card gate. The latter exercises one Battle Start banner, the selected
Rage reader reaching its final value, target-only damage, the real winning-card
reward boundary, all seven merchant cache positions and label placement.

An extra exhaustive memory/range resource traversal was interrupted because
alternating two tiled owners deliberately thrashed the shared group pool. Its
batch is **not** reported as passed. Each already completed component has its
own exit result; the dedicated UI12 gate has a separately closed passing
journal. The independent resource inventory still verifies all seventeen stored
files and sixteen reconstructed logical hashes against conversion receipts.

The source-driven sleep/swarm/camera gates cover real creature identities,
wake transitions, canonical-state invariance and original sprite bounds.
Doom and Architect lifecycle acceptance is recorded with its exact final
runtime; their new observer notices do not change gameplay state, RNG or the
save layout. The Architect work is its camera timing and terminal-scene hold,
not a claim to reproduce every authored character-attack or FireBurst particle.

## Historical UI12 pre-recovery native contracts

These establish correctness, not calculator speed or whole-game pixel identity.

| Gate | Executed result / scope |
| --- | --- |
| J, all final H locale files | 38,644 checks, 53,072 model/event descriptions, zero failures or unresolved templates; all sixteen languages load through the production split/Content routes. |
| I test with unchanged H loader | Complete English and Chinese virtual-stream reconstruction: 63,747,778 and 68,235,588 identical bytes; 46,197 and 46,404 split adapter checks. |
| J LCD/input/HUD with final English | 129 checks for physical defaults, old-default migration, pad ownership, fractional wrap, valid glyph records and sixteen-slot/pickup anchors. |
| Source-font cross-check | Twenty independent Pillow/FreeType measurements against actual native text widths; maximum difference 0.484375 LCD pixel after final rounding. |
| J Ancient/event reading with final Chinese | 33 checks; the first pointer choice produces the same canonical save as direct `event_choice`; completed Fake Merchant text uses the inline reader. |
| L character/order/departure | 241 checks including independent source-order expectation, six hit regions, pointer selection, both navigation directions, deterministic Random and source departure clocks. |
| L combat with final resources | 1,010 checks covering actual cards, target-only values, speech, committed Smith, card-cache identity, bounded aimed-card paths and a real winning-card reward before the corpse tail finishes. |

The incremental L native library uses the actual L App and input objects with
J's byte-identical remaining production objects. An earlier order test
accidentally linked J's App renderer; that failed run is excluded and its
matching-source rebuild is recorded separately. ARM builds compile the complete
frozen source rather than substituting native execution for the calculator.

## Historical H–K ordinary Firebird evidence

- H ordinary play, without console: new Ironclad run, Neow, map, a four-turn
  battle, actual numeric selection and target damage, failed-energy speech,
  victory, gold and Rage reward, Proceed, map, Save and Quit. Production readback
  is a valid exact-roundtrip canonical save: floor 2, turn 4, HP 76/80, gold 268,
  deck 11, revision 28; NSH9 progress matches. Its controller journal records
  one host ENOSPC screenshot failure, so the entire journal is not called a
  pass. Successful interactions and readback remain separately scoped evidence.
- H/J world: original Random reveal/transition, repaired animated Necrobinder,
  actual Ancient choice, ordinary save/title/language switch/continue, Chinese
  inline event scrollbar, real Rest/Smith and the two corrected Ancient label/
  first-click cases. Console-created event/room fixtures are explicitly
  distinguished from ordinary map reachability.
- H/J frontend: ordinary OS/Ndless startup, original Settings tabs and drag
  scrollbars, unobstructed right-side tooltips, Credits hover, corrected credits
  roles/glyphs/port position, compendium and real typed Flag console. Commands
  demonstrate original additive energy/gold behavior, Potion Belt and actual
  Fire Potion acquisition; these are console fixtures, not ordinary rewards.
- K continues H's actual save and verifies the complete aimed card and the
  actual world-choice Loot path. Its closed journal has zero controller errors.
  L/M acceptance and matched merchant results are appended after closure.

## Historical H measurements

H ordinary combat recorded 1,157 frames: mean interval 36.535 ms, mean render
30.344 ms, maximum interval 241.272 ms and sixteen intervals over 100 ms. H map
in that same run averaged 71.406 ms; the independent mixed world session's map
averaged 66.801 ms. Cold room/preparation and language changes can still exceed
one second. Those samples use the calculator's emulated SP804 timer; they are
not host wall-clock or physical-device FPS claims, and different encounters
are not used to invent a speedup percentage.

## Closed matched merchant comparison

The same canonical Shop fixture and the same open/idle/card-hover/removal-tip/
close protocol were exercised on UI11 N and UI12 `1826969c`. Both sessions
closed normally with unchanged canonical readback and zero resource failures.
This is an emulator Shop fixture, separate from ordinary map progression.

| Whole room protocol | UI11 N | UI12 | Change |
| --- | ---: | ---: | ---: |
| Frames | 521 | 674 | Different counts over the same protocol |
| Mean frame interval | 106.880 ms | 82.053 ms | -23.23% |
| Mean render | 105.916 ms | 79.793 ms | -24.66% |
| Maximum frame interval | 2,387.054 ms | 2,703.094 ms | +13.24% |

These are SP804 emulator timer measurements, not host wall-clock or physical
calculator FPS. The complete protocol includes cold work and hovers; it is not
an isolated warm-idle benchmark. Progress dots appeared during preparation and
the first complete inventory sample was at six seconds. Cold stalls remain.
The 8 KiB I/O input latch preserves sampled press/release transitions for main-
loop dispatch. Three separately observed 180 ms cold-preparation Esc trials
cancelled successfully, including the `44ebf0e` repeat; this does not promise
zero input latency during every resource read.

## Ordinary recovered gameplay and persistence

The `f4a7cd59` ordinary run, without console commands, exercised Ancient choices,
an actual upgraded Strike, combat and denied-energy speech, a normal win,
Loot while a corpse remained visible, gold/card pickup, map, save/title and
Continue. Its production canonical readback was valid and exactly roundtripped
at floor 5, HP 79/85, gold 280, deck 12. Its 815 combat frames averaged
37.687 ms with a 300.262 ms maximum. This is encounter-specific evidence, not
a whole-game FPS claim.

The `44ebf0e` ordinary continuation took the only legal next map node to floor
6. Real captures show one Battle Start banner followed by Player Turn. By turn
5 it naturally drew Rage; selecting it then waiting revealed the complete
`this turn, gain 3 Block.` tail without moving the title, cost or neighbours.
The saved state for final-launcher Continue is HP 59/85, gold 280, deck 12.
The earlier `169cea99` baseline independently continued that real save, selected
and played Rage with digit 1 / Enter, showed Rage 3 and hand count 5 → 4,
saved revision 69 and returned normally to the calculator OS. USB readback
passed production decode, validation and exact canonical roundtrip, with zero
resource-read failures. The battle was not claimed to be another completed win.

A separate `169cea99` Flag-console fixture used actual typed `fight` commands and
Tab completion to exercise Entomancer's moving phobia swarm and the beetle's
sleeping Z. It returned to the OS and restored the ordinary revision 69 save
and progress, with identical USB readback. These deliberately constructed
contexts are not ordinary map progress or another performance benchmark.

The delivered `9398633` launcher independently passed ordinary revision69
Continue, SaveQuit, normal OS return and canonical-equal USB readback. A separate
native-generated original-entry Ironclad fixture then exercised actual Architect
dialogue, Proceed, visible actor actions, original-position death, ending summary,
Main Menu and normal OS exit. The earlier room outcomes in that fixture were
supplied by the native routing test; it is not a natural three-act victory.
Control-wait screenshot labels are not exact animation-clock measurements.
Afterward the original ordinary save/progress were restored and read back
byte-identically (7,708 / 1,191 bytes); resource failures were zero. The closed
journal is `ui12-architect-device-final-55becdaab6a4`, with `source_changed=false`.

Actual startup/SkipIntroLogo persistence was verified at `125adc82`: the
original sampled logo ran, NSPF6 stored the setting in 108 bytes, a real USB
readback confirmed its flag, and a fresh launch skipped to the title. Normal
Quit returned to the operating system. An earlier black-frame/blocked-USB
attempt remains unknown/excluded; it is not rewritten as a successful exit.


## Explicit platform adaptations

Ancient full actor animation remains omitted under the user's explicit
size-saving exception. The restored character select animations, actual
selection/arrival/death/reward/Smith triggers and source clocks are implemented.
Dense GPU turbulence, continuous particle fields, HDR bloom and exact desktop
glyph rasterization remain bounded CPU/LCD approximations. Random retains the
authored gradient and functional reveal; its GPU water distortion is not an
identical shader reproduction.

Audio, multiplayer and Timeline remain outside the agreed scope. Offline
services such as feedback and patch notes are removed. The console reports
unavailable desktop/network commands rather than pretending they succeeded.
No screenshot percentage or registry count is presented as a measured 95%
restoration score.
