# UI15 feedback implementation and acceptance map

Reference: the owner's pinned v0.111.0 / `41cef1ea`. The complete private DLL,
PCK and the original scenes were recovered and checked. This map reconciles
the latest feedback with UI14; it supersedes the earlier zero-ascension hover
instruction. Implementation, actual-resource checks and ordinary-OS timing
retain separate meanings. No test count is a percentage of original fidelity.

## Latest feedback

| Feedback | Implemented behavior and evidence |
| --- | --- |
| Default language | Saved language selection takes precedence; otherwise system language, then installed English, then the first installed language. Shared parser/selector/SDK adapter passes906 checks. An ordinary CX OS3.1 first launch selects Chinese from its actual `zh_CN` settings without a saved override. |
| Space accelerates credits | Physical held Space advances the original scrolling clock at5×. Release, physical input cancellation and closing reset it. Actual ENG/ZHS credits checks include both rates and source close timing. The original does not automatically dismiss its finished credits. |
| Bestiary right-side vertical drift | Fixed14px monster rows and18px section rows. A long single-line name moves horizontally within its own viewport; its row never moves vertically. Actual English/Chinese layout and overflow cases pass. |
| Continue information too large | Shared120px panel with66px/79px height, preserving source date, act/floor, ascension and health/gold hierarchy. Actual English/Chinese long-date comparisons pass. |
| Confirm versus Back size | Shared confirm and Back use40×24 hit targets and47×32 source bodies, including the character selector. Paired layouts use the same control projection. |
| Ancient Continue still reserves an empty description | The separate Ancient renderer measures label-only options at24px, descriptions at35px, and anchors the complete measured stack at its source bottom. Its14px label viewport prevents a false one-pixel overflow; settled actual-resource hover frames remain stationary. |
| Uneven room/floor/boss/time icons | The four icons use explicit independent spacing and hit regions. Elapsed time appears only in the clock tooltip; room/floor descriptions contain no added time. Map/deck/settings keep their established positions. |
| Short-overflow scrollbar artwork stretches | Visual grabber size is independent of the logical proportional thumb and bounded to its source square. Existing focus/return scaling remains. Character/defeat handles keep their fixed projected sizes. |
| Large Capsule relic flashes | Each actual acquisition/activation receives its own flash. The original grants two relics sequentially; Capsule and those newly obtained relics flash. It does not flash unrelated inventory. The production callback path and source boundary cases cover this behavior. |
| HUD tooltip width | The original `NHoverTipSet` itself uses a fixed360px column and measured height. The LCD uses its readable104px projection and measured content height; a short title is not used to invent a different tooltip style. |
| Ancient entry heal | Real positive entry healing schedules the original fullscreen effect after200ms. At full HP the actual gain is zero, so the Ancient layout shows no invented healing. Four original textures and actual entry callbacks are integrated. |
| Zero ascension | No character hover at ascension zero, applying the owner's correction. The flame and positive ascension number share their source anchor. |
| Center deck upgrades | The entire checkbox/label group is centered in its reserved206–221px footer row, clear of Back and the card grid. Actual ENG/ZHS footer cases pass. |
| Deck sort text and dark bar | Original left alignment, Kreon Bold/substituted Regular, readable6px glyphs,250×42 source button proportions,50px source gaps and separate60px outer strip. Tint affects the image material without tinting the labels. All16 packs include the required small glyphs. |
| Card type text too large | Attack/Skill/Power/Status labels use the reduced source projection and actual card-face measurements. Actual English/Chinese card/HUD gates pass. |
| Selection eye overlaps cards | Drawing and hit target move left together to x5. The selection-preview gates cover both primary languages. |
| Generated cards appear abruptly | The original250ms growth/arrival sequence remains. At most one already-committed upcoming face is prepared per preceding frame in the existing hand cache, so its first visible frame need not decode a completely cold face. Enemy null-creator semantics and original per-batch waits are restored. Device action-window performance is reported separately. |
| Intent sprite/number relationship | Original sprite/number anchors, raised group placement and explicit source number offsets replace the old shared lower anchor. Health and creature formation positions remain independent. |
| Smooth scrolling plus duplicate tooltip | A full card description stays in its own smooth clipped viewport. Redundant description-only tips are removed; mechanic, power, relic and card-preview tips retain their distinct purpose. Single-line overflow is horizontal, paragraphs move in pixels, with end holds. |
| Tablet of Truth upgrade animation | Permanent upgrade notifications originate from the real core commit, including single, random and whole-deck upgrades. The presentation retains each card's own before/after facts. |
| Loot title too low | The title uses the corrected ribbon anchor between the prior over-high and over-low placements, covered by actual room rendering. |
| History room hover trigger | Only eligible traveled non-current nodes with a real history record trigger it. Leaving/changing/closing clears old hit ownership; entering the visible reader retains that floor for scrolling. Its174px panel no longer covers most of the screen. |
| Chest glows without hover | When a pointer is visible, only its actual chest rectangle can activate the outline; stale keyboard selection and covered modals cannot light it. |
| White mismatch on elite/rest map hover | Base and outline images retain the same original128×128 atlas canvas including margins. Their silhouettes now align under source hover scaling. Traveled nodes retain the map-paper outline color. |
| Merchant rug opening and colorless prices | Source two-row placement, child-relative prices and focus draw order are retained. Lower colorless centers are58/119 at y168. Both settled card sizes use existing face-cache tiles. Hidden price glyphs do not load offscreen font groups. Independent card streams and fair background preparation reduce unrelated cold reads. Final first-open peaks are256.012ms from saved Shop and549.438ms after map travel; the remaining cold stall is explicit. No extra loading artwork or input lock is added. |
| Rest button border proportions | Complete original icon/outline fitted extents and transparent padding are retained in Q8 until final projection. At full focus the icon is49×35 and outline52×37, with the same authored pivot. |
| Rest Smith sequence | Nine original impact frames, the original hammer and spark sequence drive actual smithing; upgrade commit and temporary reveal follow the real operation. The old static/incorrect impact is replaced. |

## Earlier feedback carried into this build

| Area | Retained implementation |
| --- | --- |
| Console and installation | Debug console is always compiled. Only `nSpire2.tns` is at the installation root; shared/locale resources and supporting documents are in `data/`, every writable file in `state/`. No old filename or obsolete-save migration layer is introduced. |
| Credits, Bestiary and defeat | Nspire port credit is a green major heading. Compact Bestiary rows and centered selection arrow remain. Defeat uses the narrower score/button layout, rebuilt original badges and ribbon-centered death text. |
| Random selector | Original icon proportions, complete water/light layers and all40 particles remain. Exact interpolation and tile-span optimizations preserve native output rather than dropping particles or animation frames. Matching device windows identify their actual speed. |
| Departures and Continue | Continue and Embark share the saved/selected character's original departure lifecycle. The natural run visibly replays its Ironclad silhouette wipe and room fade on Continue without changing its save/RNG. |
| Card piles and special borders | Draw/Discard/Exhaust retain source focus/press/return motion. Ancient frame/banner/fire and Infection/Bound/Entangled/Galvanized/Hexed/Tainted/Ringing/Smog materials retain their own art and precedence. |
| Creature scale | Shrink, Beetle Juice, Big Mushroom, Gardener, Test Subject, Osty, Sewer Clam and orb-manager transforms remain connected to real facts. Decimillipede's source rejection is preserved. Vantom's previously missing continuous bone motion is now implemented below. |
| Rewards and selections | Empty terminal rewards fade and wait for Proceed; they do not automatically open the map. Wood Carvings/Torus previews sit lower. Fixed-count transform selection omits unrelated Confirm. Enchantment and Sovereign Blade forging retain their actual successful-commit animations. |
| Earlier retained differences | Complete solo-reachable monster command awaits, nested hook timing, conditional summons/dialogue and sequential death callbacks are now connected. Vantom retains the750ms independent root-bone transition, original tail space, compound clips, shader masks and all four source emitters. See the reconciliation ledger. |

## Input completion added during ordinary-OS acceptance

The original cold-render input omission is now repaired for both physical
keys and complete touch gestures. The shared32-entry FIFO preserves contact
origin, motion, press/release and their input timestamps until the normal
foreground dispatcher. Held-key repeat remains once per frame even with touch
jitter. The actual original merchant touch sequence now opens the rug; real
50ms selection/target/confirmation plays one card and preserves its selection
through the side-key release. Normal OS exit, a fresh Continue and a second
SaveQuit preserve the same7430-byte run exactly at `bd158d205d877f767f59fe2a1eb33874cb925899`.

A final source audit finds no remaining generic ellipsis replacement in these
bounded text painters. Literal punctuation, the eye fallback symbol and the
original ten-name discovery summary remain authored content. The pinned
`NRunSummary.GetDiscoveryBodyText` itself appends the four dots after its first
ten names; that summary is distinct from a label discarding its unread tail.

## Executed gates and measurement boundaries

Integration9, local commit `a04451c07e1d63fa50cf95466381fce2a0d96ff8`, passes
140/140 native suites,354 Python checks and the portable compile-only target.
All16 complete packs pass real loading and53,072 descriptions with zero
unexpanded templates. Actual English/Chinese frontend, event, HUD, deck and
Bestiary gates pass. Original screen effects and the complete Vantom frame
traversal pass. Later runtime/resource increments have their own identities
and must not be relabeled as that full baseline sweep. The original full
sanitizer run is136/140; v14 closes its four shared enemy-pointer failures.
Integration16 then passes20/20 selected native and sanitizer suites with nine
actual-resource paths each, and bd158 passes6/6 changed-path suites with six
actual-resource paths each. Exact identities and the retained failures are
recorded in [validation-ui15-0.17.json](validation-ui15-0.17.json).

At integration9 (`a044`), the ordinary CX OS natural run starts legally,
finishes Neow's real choices,
wins the first three-Slime fight, receives gold/card rewards and waits for
Proceed. SaveQuit, Continue and a second SaveQuit preserve the same7590-byte
save exactly. Continue visibly retains the original transition and final exit
returns to the OS. The final bd158/strings15 program and resources have their own matching
installed identities, changed-path gates, real merchant opening paths and
normal OS exits. The complete player ZIP is read back and independently
compared against those exact inputs, as recorded in WORK_STATE. Remaining
performance limits are reported rather than described as eliminated.

See [frontend-ui15-0.17.md](frontend-ui15-0.17.md),
[combat-ui15-0.17.md](combat-ui15-0.17.md),
[rooms-ui15-0.17.md](rooms-ui15-0.17.md) and
[ui15-retained-differences-0.17.md](ui15-retained-differences-0.17.md).
The accepted full-Ancient-actor animation storage exception and excluded
audio, multiplayer and Timeline remain the agreed scope. A natural first
victory is not an unexecuted natural three-act victory or a physical-device
FPS claim.
