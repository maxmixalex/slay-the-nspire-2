# UI12 calculator usability follow-up

Reference: the owner's v0.111.0 / 41cef1ea. UI12 continues the readable UI11
adaptation; it does not revert to the earlier small desktop projection. Original
art, model rules and source interactions are retained, with deliberate reflow,
clipping, scrolling and physical-key changes for 320 × 240.

This is the integration report. The final acceptance/performance section is
completed after the remaining ordinary Firebird sessions close. The recovered source tree is now remotely preserved on
`work/0.17-world-parity` at `fe11b6d`; the interrupted validation and final
package are being completed in the resumed session. No WASM/browser validation
is used.

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

Detailed command semantics and offline/platform limits are in
`developer-console-ui12-0.17.md`. Combat clocks, observer invariance and exact
card-cache checks are in `combat-ui12-0.17.md`; frontend material/layout
contracts are in `ui12-front-feedback-0.17.md`.

## Storage and allocation

The final shared UI40 resource is 61,501,372 bytes. English is 1,133,049 bytes
and Simplified Chinese is 5,113,330 bytes. The launcher is about 1.57 MB, keeping
the default installation at about 69.32 MB decimal. The final exact launcher and
installation counts are bound to the release receipt, not estimated from ZIP
size.

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

The allocation ceiling stays 24 MiB. The integrated launchers request about
20.13 MB / 19.19 MiB, including real 5 MiB image and 6 MiB tile-group caches.
Ordinary Ndless execution, rather than the ceiling alone, is the acceptance
gate. The historical UI11 23.87 MB allocation failure remains relevant; this
release does not claim that Ndless accepted a contiguous 24 MiB block.

## Executed native contracts

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

## Ordinary Firebird evidence

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

## Measured limits

H ordinary combat recorded 1,157 frames: mean interval 36.535 ms, mean render
30.344 ms, maximum interval 241.272 ms and sixteen intervals over 100 ms. H map
in that same run averaged 71.406 ms; the independent mixed world session's map
averaged 66.801 ms. Cold room/preparation and language changes can still exceed
one second. Those samples use the calculator's emulated SP804 timer; they are
not host wall-clock or physical-device FPS claims, and different encounters
are not used to invent a speedup percentage.

The matched merchant comparison uses the identical canonical Shop save and
matching stock/hover actions on UI11 N and the final UI12 merchant implementation.
Its closed-session result is required before reporting a speedup.

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
