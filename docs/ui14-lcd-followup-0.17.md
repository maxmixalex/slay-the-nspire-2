# UI14 feedback map

Reference: the owner's v0.111.0 / `41cef1ea`. This map connects the current
feedback to implemented code and the detailed validation ledgers. Named
performance windows, original-resource checks, and a natural run retain
their separate meanings; assertion counts are not fidelity percentages.
This is a coverage map, not a blanket performance-closure claim. Random,
departure and generated-card frame rates must cite their matching final
Firebird windows in the detailed ledgers. Correct pixels and implemented
optimizations alone do not resolve a reported frame-rate problem.

| Feedback | Implementation and evidence owner |
| --- | --- |
| Console build switch | The console is part of every native, Ndless and portable build. The previous compile switch and separate delivery variant are removed. `debug_ui.inc`, build scripts and CI use the same runtime. |
| Installation layout | Only `nSpire2.tns` stays at the installation root. Shared and locale resources are under `data/`; run, progress, preferences, language, profile selection, temporary/backup and opt-in diagnostic files are under `state/`. The launcher creates `state/`. No old-filename fallback is introduced. |
| Credits ending and port heading | The original does not auto-close at the end. It accepts dismissal after its initial lock and fades out. The port credit is a green major section, with a separate author line. See the frontend ledger. |
| Bestiary spacing and arrow | Both columns use measured compact rows, and the selection arrow follows the selected row's center. Actual English and Chinese captures are checked. |
| Random selector | The icon keeps 132:196 proportions. Water, the original light and all 40 particles remain; complete frame pixels still match the general sampler. Final stage16 ordinary-OS Firebird measures19.0350FPS in the cold10s window and20.3914FPS in the following matched30s, versus the earlier9.8567FPS warm result. The final warm window has zero reads/failures and no frames above100ms; it is not30FPS. The frontend ledger records exact source and window boundaries. |
| Event options without secondary text | Label-only options use the label's required height; their hit regions and the surrounding stack follow that height. |
| HUD clock and room/floor/boss spacing | The clock is an icon; elapsed time belongs only to its tooltip. Room and floor text contain no extra timer. The three location indicators have explicit gaps, including crowded potion inventories. |
| Ascension zero and fire alignment | Character hover remains active at zero ascension. The number and flame share the corrected anchor. |
| Defeat width, title and icons | The score column is 216px and the main button 112px. The death title follows the ribbon center. Thirty-one score/badge records use rebuilt original artwork at their intended LCD sizes. |
| Embark transition | Original masks, channel lookup tables and first-present timing retain the visible tween while diagnostics include cold setup. Final stage16 captures show ordinary Ironclad's first changed image at1.840s and full black at2.640s; Random resolves to Silent at1.160s and reaches black at2.920s, retaining the1000ms reveal plus800ms departure within40ms sampling. Both reach Neow, save and exit normally. Their13/38 character-frame mixed windows include setup and are not isolated animation-FPS measurements; see the frontend ledger. |
| Deck upgrade control | The grid ends above a reserved footer row. The upgrade checkbox and its label remain clear of Back. |
| Deck sort labels | Original bold face, gold ink, separate shadow and complete clipped overflow text are used; both primary languages have actual-resource captures. |
| Scrollbar hover | Shared scrollbar trains implement the source 1.15 focus scale and 500ms recovery. The settings language thumb and FTUE reader use the same behavior through their respective source skin. |
| Ancient card borders | Nine Ancient portraits, the frame, text background, banner and ten flame frames replace the ordinary colorless treatment. |
| Pile hover motion | Draw, Discard and Exhaust icons share the original focus/press/return motion; their count labels remain separate. |
| Combat performance | Stable card rasters and per-instance sample keys avoid repeated formatting. The currently displayed enemy intent may be prepared within the existing bounded room preparation. Performance uses the named matching device windows for real enemy turns and generated cards. Cache/preparation correctness alone does not establish FPS, and an empty combat screenshot does not replace those windows. |
| Shrink and other body-scale effects | ShrinkPower, Beetle Juice, Big Mushroom, Gardener growth, Test Subject respawn growth and Defect orb-manager scaling are connected to their actual facts. Existing Osty and Sewer Clam growth remain. Vantom has correct full/half body endpoints but still lacks the source 750ms bone transition; see the retained-differences ledger. |
| Hand description scrolling | Long hand descriptions move in LCD pixels, with end holds. Only their text viewport is restored and repainted. |
| Other text overflow | Shared fitted labels, room text, source hover text, selection and FTUE readers retain full text and move within their clip. The original run-summary first-ten-items marker is content-defined, not renderer ellipsizing. |
| Rewards opening the map | Empty terminal rewards fade for 250ms and wait for Proceed; an empty nonterminal overlay returns to its parent. The original terminal screen does not automatically open the map. |
| Loot title height | The title moves four pixels lower on its ribbon. |
| Sovereign Blade forging | Actual Forge commands drive the original hilt variants, orange glow, three smith hits, sparks and blade orbit. Existing exhausted blades also receive the correct strength update. |
| Wood Carvings / Torus preview | The preview follows its option lower on the screen; related keywords can move above it to keep the entire preview readable. |
| Single-card transformation confirmation | Fixed-count selection grids omit the unrelated disabled Confirm control. The original transformation preview and its separate confirmation remain. |
| Enchantment | A successful permanent commit emits the original reveal, tab/icon, sparkle, hold and deck flight. Canceled or failed selections emit none. A restored combat choice prepares its room before the choice is displayed, preserving the first second of the effect. |
| Merchant room, rug and bubble | Exact invariant/moving frame factoring preserves all 1,817 source frames. There is no invented pre-rug loading animation. Cards and prices have wider separation, and the bubble's text uses its source body center. Stage12 measures30.0073FPS in the matched warm closed-room window, but the first-opening maximum render is still3.7716s. These conditions are separate; the warm result does not close the cold spike. |
| Rest outlines | The packed icon and separate outline use the source aspect projections and independent focus tween. |
| History room tooltip | The panel is 174px instead of 272px, with its existing full-entry reader. |
| Continue animation | Continue uses the saved character's same departure lifecycle as Embark. It does not create another run or reset the restored State/RNG. |
| Enemy-generated card animation | A real Haunted Ship turn creates five Dazed cards. Shared-raster sample churn is fixed and the registered action's preparation is checked separately. The named final device generated-card action window determines performance acceptance; a longer settled-idle window cannot replace it. |
| Special status and affliction frames | Infection retains 30 source frames. Bound, Entangled, Galvanized, Hexed, Tainted, Ringing and Smog have their own materials and correct precedence. Their bounded CPU material sampling is disclosed. |

## Detailed evidence

- [Frontend, credits, deck, defeat and departures](frontend-ui14-0.17.md)
- [Combat, HUD, card materials, scaling, forging and enchantment](combat-ui14-0.17.md)
- [Events, rewards, selections, rest and merchant](rooms-ui14-0.17.md)
- [Earlier retained differences and exact remaining source requirements](ui14-retained-differences-0.17.md)
- [Current integration and acceptance state](WORK_STATE.md)

The earlier death-to-reward shortcut is also replaced with the source's four
normal/fast and ordinary/boss timing branches. The already implemented UI11–13
visual families are reconciled in the retained-differences ledger rather
than counted again as new UI14 work.

The accepted full-Ancient-actor-animation storage exception remains. Audio,
multiplayer and Timeline remain outside the agreed offline single-player
scope. Complete per-monster surrounding command waits and Vantom's missing
bone interpolation require the pinned original input that was removed by
the environment cleanup. No guessed wait table or whole-sprite scale is
substituted for that missing evidence.
