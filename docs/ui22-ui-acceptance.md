# UI22 menu, reward, lifecycle and final resource acceptance

Reference: owner-supplied Slay the Spire 2 v0.111.0 / `41cef1ea`.
Private original assets, decompiled reference, locale packs and captures stay
outside the repository. These are executed native production-renderer checks;
they do not establish Firebird execution or physical-calculator frame rate.

## Latest feedback and the delivered behavior

| Feedback | Implementation in this iteration | Acceptance on the final resources |
| --- | --- | --- |
| Continue metadata covers Abandon Run | Measure the complete localized menu, focused-label padding, logo and profile bounds before placing the popup in the available gutter. | Final English and Chinese mouse/keyboard routes pass the pixel-exclusion and onscreen checks. Both final captures were inspected; the earlier German, French and Russian checks also cover long-label fallback. |
| Loot potion and relic tooltips disappeared | Route all seven reward families to their actual item tips, preserve card previews, and provide a modal, paginated details reader for long descriptions. | Both final catalogs retain the complete text of all 361 potion/relic models. Mouse, keyboard, claimed-index gaps, modal input, return focus and Load reset pass. Final hover captures remain readable. |
| Character selection drops frames | Reuse exact selected-character frame changes and fix shared-scene cache lifetime when an App is reconstructed at the same address. | Final five-character/Random transitions and cross-locale cache reopening pass. Actual timing, cold reads and the accepted performance boundary are recorded in [the selection report](ui22-selection.md). The rejected Random experiments are not included. |
| Character, summon and death animations lose detail or color | Restore original RGB565/alpha8 source values, register source-density actor frames for the actual scale bounds, and use a shared sampling grid across split pieces. | The independent original-source image and actor-Canvas gates cover the restored data. Final English/Chinese death routes and the actual Osty growth, death, dead-loop and revival sequence pass; see [source precision](ui22-source-precision.md). |

The new reward reader, reused-address cache reset, source-density registration
and exact affliction phase clock close specific interaction, lifecycle,
representation and timing gaps. Osty's death/dead-loop/revival timing already
existed before UI22; this iteration supplies corrected source artwork and
validates that existing sequence. It is not reported as a new timing feature.

## Continue metadata placement

The previous panel followed only the Continue label. That could intersect the
larger Abandon Run label and its hit region. The shared title layout now measures
all localized menu labels, including the focused scale and reticle padding, and
uses the resulting bounds for both hit testing and popup placement. A compact
right gutter is preferred. A long-menu fallback occupies the clear upper-right
area. Both placements account for the complete logo and profile-control bounds,
not just text baselines. The original rise/fade behavior is retained.

`world_frontend_ui_regression` checks 20 combinations per actual locale: five
menu focuses, ascension zero/three, and mouse/keyboard. At the same clock, showing
the metadata must leave every menu-hit pixel, complete logo pixel and profile
control pixel unchanged. The panel must remain onscreen and paint visible pixels.
The independent text-placement check in `profiles_ui7_regression` uses the same
measured placement; its font raster reference remains independent.

English, Simplified Chinese, German, French and Russian all passed. The French
long-label fallback was also inspected in the actual 320x240 capture. No menu
item or profile control is covered, and the popup remains close to the menu.

## Loot potion and relic hover tips

The root cause was an absent central hover route: reward rows use `UiAction` or
`WorldChoice`, so the existing shop/inventory routes never selected them. The
new route identifies the actual visible reward and supports mouse hover and
keyboard focus for ordinary, world and event rewards, including claimed-index
gaps and standalone potion offers. Gold and ordinary card rows retain their
existing behavior. Detached relic previews do not borrow an owned-inventory
index.

The placement follows the source `NRewardButton` left-side hover behavior. At
the normal reward row's x=87 position, a readable 80-pixel panel uses the left
gutter with 10-pixel text. Derived cards retain their separate right-side space.
Long headers use the existing paused horizontal scroll; the reward row also
retains the full item label. Body text is paginated without discarding words.

The **V** details command opens the reward reader; potion Details can enter it
as well. Up/Down or the footer arrows change pages. Back/Details returns to the
same reward focus. A modal input layer prevents an overlay click, Confirm,
EndTurn or navigation shortcuts from claiming/skipping a covered reward. A click
outside the reader exits only. Normal card Details and the top-bar potion
controls retain their original routes. Save/Load/Quit remain global commands;
an actual Load check restored a different run, cleared reader state and restored
normal scene input.

### Executed actual-language coverage

The catalog check visits every formatted tip's actual rich-text glyph layout,
collects the non-whitespace Unicode sequence across all pages, and compares it
to the complete formatted description. It also verifies contiguous page spans,
onscreen geometry, one copy of each derived card, and no overlap with reward
rows or sibling tips. Character counts below include both passive and full
reader layouts; they are verification work, not a count of new features.

| Locale | Potion/relic models | Characters retained across all pages | Passive entries requiring multiple pages |
| --- | ---: | ---: | ---: |
| English | 361 | 48,276 | 24 |
| Simplified Chinese | 361 | 21,610 | 15 |
| German | 361 | 57,784 | 37 |
| French | 361 | 64,052 | 38 |
| Russian | 361 | 48,544 | 28 |

All seven reward-route scenarios passed mouse and keyboard ownership checks,
actual painted-pixel checks, modal controls and unchanged canonical-save checks.
The five actual-language full frontend and reward runs passed, plus both English
and Chinese profile runs: twelve successful target invocations. The five-locale
reader contact sheet and passive potion/relic captures were visually inspected.

## Exact affliction phase clock

The original Entangled material stores `distortion_st=(1,1,0,0.175)` in
`materials/vfx/ui/card/afflictions/entangled/vfx_ui_card_affliction_entangled_main.tres`.
Its shader offsets the texture by `TIME * distortion_st.w`; the original
Tainted scene references that same material. A repeated texture therefore has
an authored period of exactly 40/7 seconds. Rounding it to 5714 milliseconds
made each port cycle end 2/7 milliseconds early and accumulated phase drift.

`card_affliction_frame` now selects the existing eight samples using the exact
rational phase `((now % 40000) * 7 / 5000) % 8`. Other affliction clocks use
bounded 32-bit constant arithmetic as well. There are no additional resource
images, tables or allocations. The original complete ten-frame Ancient fire
sequence and thirty-frame Infection sequence remain unchanged.

`combat_ui14_regression` compares all seven clocks to an independent 64-bit
rational reference at transitions, long elapsed times and the uint32 boundary.
For Entangled and Tainted it also compares the actual rendered card overlay to
the source image selected by that reference. Eight sampled affliction phases
remain a finite representation; this change resolves the accumulated clock
error without claiming continuous shader rendering.

The actual complete-color control passed 77,968 checks with the corrected
capture helper from `83ebb477cb495c4dc986dcb8d68fd6b2e72fff6e`. A fixture hand
change can restart normal combat preparation; the helper now drains that work
at the requested visual clock and rejects a wholly black capture. Both full
card scenes at 39,999 ms were inspected after this correction. The earlier
black transition captures are not accepted card-image evidence.

## Reused App address and shared scene memory

An independent review of the selection raster cache found that a newly
constructed App at a previously used address can reopen Content at generation
one. Address plus generation alone could then match a stale scene raster.
Both App initialization paths now invalidate the shared scene owner before
opening data. The large arrays are not cleared or duplicated.

`scene_raster_cache_regression` constructs two successive App objects at exactly
the same address using both memory and ranged input. Each starts at Content
generation one, invalidates the owner and reconstructs all 76,800 pixels from
its new source. Existing Random/merchant ownership checks remain in place.

## Random performance decision

Two exact white-dot sampling candidates were evaluated with complete rendered
frames, cold entry and rapid character/Random switches. Their frame pixels
matched, but the measured warm complete-frame and white-dot CPU costs increased.
Both candidates were removed from the production runtime. Random retains its
previous rendering path, with only a zero-alpha early return and its boundary
check retained. No Random speedup or storage reduction is claimed. The five
character selection optimization is a separate result.

## Guarded retirement of unreachable legacy artwork

The complete UI22 profile can now reclaim 1,598 old images belonging to the two
superseded merchant fallback clips and 72 standalone Kaiser states that have no
entry in the current combat or Bestiary routing. The fixed public retirement
plan pins the reviewed source routes and exact clip registrations. Conversion
also checks the actual pack's direct roots and typed actor/mesh/mask references;
any overlap stops conversion. The live merchant's ten moving layers and eleven
fixed/background planes remain complete. The 18 Bestiary Kaiser supplements,
26 earlier registered-frame leaves and 159 unclassified static images are
retained.

Retired image IDs keep their original dimensions and become empty sparse
records. All 1,783 clip tuples, pivots, frame counts, sample clocks, card entries
and prior auxiliary IDs stay unchanged. The 57,014 nonretired images retain
their exact pixels, alpha and geometry, as do every glyph and string. Physical
compaction removes only slots with no remaining image descriptor, using the
same codec, transform and LZMA profile as each affected group. No runtime pixel
buffer or animation dependency chain is added.

On the complete original-color English input, this step reduces the logical
pack from **103,797,230 to 102,342,354 bytes**, a net saving of **1,454,876 bytes**
including the new format marker. It removes 1,094 unused descriptors, nine
unused index groups and 20,472 tile records. Only 22 pixel streams change, and
21 groups require fewer decoded pages; this is not a device frame-rate claim.
The resulting pack has auxiliary endpoint 7212 and one additional 16-byte
metadata image. Older launchers reject that endpoint. The matching launcher
checks the marker and the layered merchant's necessary resident directory
identities, without a resource-wide integrity scan or pixel preloading.

Published checkpoint `0b5e969dff43cee998ab081dfb7af889eaf2bf18` passed the actual
memory/ranged-reader regression: valid new and old profiles, damaged marker and
merchant metadata, failed marker reads, and a failed merchant pixel read that
cannot select the retired fallback. The prior runtime independently rejects
7212 through both readers. A further **723 complete production frames** match
the unretired pack byte for byte, including all hit regions and unchanged saves
while rendering: all five merchant characters through focus/open/close input,
every Crusher/Rocket Bestiary action, eight actual Kaiser combat turns and an
observed arm death. Twelve full-scene captures were inspected. This retirement
does not remove a currently reachable animation frame or reduce color fidelity.
The final resource bindings below include this retirement and the later
lossless storage finishing steps.

## Final native and actual-resource acceptance

The production runtime is checkpoint
`b9425530b24b0d851b57892872fc07865859cea1`. Its complete clang 18 Release build
passed **162 of 162 CTest regressions**, zero failures, in **65.17 seconds**.
The private build and CTest journals record an unchanged source tree. This
supersedes the earlier 160-test gate at `d940fec9` without discarding that
intermediate evidence.

NSP2 version 46 adds exact regeneration of eligible ordinary image geometry.
It preserves decoded pixels, alpha, sparse output bytes, image IDs, page costs
and the source pixel-group read order. The dedicated 230,400-byte temporary
buffer does not reduce the existing image, tile or selected-hand caches.
Interleaved synchronous reads, paused preloads, cancellation, failed reads,
reopening and reused Content addresses pass the owner-lifetime regressions.
An older runtime independently rejects the real version-46 pack at open in
both memory and ranged modes. Final release validation also rejects the
experimental selection bitmap form (`FFFC`); the final packs use ordinary
implicit geometry (`FFFD`) and explicit selection geometry. The rejected
163,840-byte selection bitmap saving is not part of the delivered result.

The final actual-resource gate is bound to these immutable logical files:

| Locale | Bytes | SHA-256 |
| --- | ---: | --- |
| English | 94,924,498 | `e2fc2beb4fe1e4d9ff5e54188f0c134bc9da2ee680ef2acc06ad5f7cd6784147` |
| Simplified Chinese | 99,349,212 | `e1add5f33fbbf8f2093fcf3676200518a0550808ad575bf6e9b0071b3ce7b3d6` |

Journal `ui22-final-bilingual-actual-ui-097962ba87cc` and its
`final-ui46/receipt.json` record **18 successful actual-resource target runs**:
frontend, profiles, rewards, keyword tips, world UI, five-character UI, death
routing and affliction cards in each language; the cross-locale scene-cache
check; and ordinary-command Osty gameplay. Both resource hashes are unchanged
after execution. The final reward checks retain 48,276 English and 21,610
Chinese non-whitespace rich-text characters across all passive and reader
pages, including the same 24 and 15 passive multipage entries reported above.

The final English source-pixel oracle also passes all **58,613 images through
each of the memory, ranged and incremental-preload readers**. Each reader
reproduces the independent old-format reference's **1,065,442,263 raw bytes**;
the reference body was exported before the new geometry decoder existed and
is unchanged. Journal
`ui22-final46-complete-original-source-proof-f849e4193964` records 200,421,907
checks, 28,655,100 preload steps, at most 24,172 steps for one image, and no
source reads after Ready. The 33 existing all-opaque format normalizations per
reader are accepted only after every color byte and implicit alpha value 255
matches; no non-opaque difference is allowed. The earlier source-Canvas gate
covers 427 actions, 11,051 frames and 61,816 projected views, including
noninteger scales and gray rendering: 5,106,124,800 LCD values match the fresh
original-source reference. Its active-pixel identity is preserved by the
final resource transformation and the final raw oracle above.

Each language's death target passes 4,175,314 total assertions, including all
five players in combat, event, rest and merchant room states. It supplies a
dead-player fixture to the production `game_over_actor` route and checks the
actual body clips, source room placement, distinct first/middle frames,
terminal holds, particle decay and original timer. The supplied Necrobinder
review PNG is the merchant-room terminal route. Separate existing captures
cover Necrobinder's combat terminal and event first/middle/terminal poses.
These tests intentionally isolate the actor layer on black; they do not claim
to show a complete combat screen or simulate the lethal input itself. The
ordinary-command Osty route renders complete gameplay scenes and passes 1,989
checks: Bodyguard growth, Bone Shards death, the full 17-frame death action,
dead-loop, and the 12-frame revival through terminal idle. Seven scene captures
are retained. The final dead-loop and completed-revival scenes were visually
inspected. Rendering and presentation-clock advances leave canonical saves
unchanged.

The final English/Chinese Continue captures and English potion hover were
visually inspected at the native 320x240 resolution. The panel does not cover
Continue, Abandon Run, the logo or profile controls. The hover description and
its sibling keyword explanation are readable; long titles retain their
existing scroll and complete reward-row label. Four review PNGs are lossless
format conversions of the existing PPM captures, with identical RGB bytes;
they are not rescaled or reconstructed images.

## Final installation archives

An independent boundary audit read both completed ZIPs, checked every CRC and
path, and compared each of the 25 data files plus the launcher byte for byte
with the frozen release inputs. It also matched their hashes, sizes and member
lists to the delivery manifest. The bilingual archive has only the launcher,
`data/` contents and an empty `state/` at its outer level. There are no packaged
user saves, temporary saves, backups or test markers. The optional archive
contains exactly the other fourteen language files and their instructions
under `data/`.

| Archive | ZIP bytes | All extracted bytes | Runtime `.tns` bytes |
| --- | ---: | ---: | ---: |
| Bilingual full installation | 98,728,539 | 99,650,627 | 99,526,983 |
| Optional fourteen languages | 17,321,125 | 18,855,677 | 18,854,842 |

The bilingual ZIP SHA-256 is
`e75fa7e3c8e4bec3e4bc2f4381b4fb760aead6ab55b3236c5c24daa5196f8e9f`;
the optional ZIP SHA-256 is
`bda9f42a2dcf62f0afa9799ef2b0626d204a11a5c49b3da07facb709bf2defc6`.
The independent receipt is `delivery/independent-audit.json` in the private
release evidence. The packages themselves were not changed by this audit.

The actual packaged README directs an upgrade to exit first, preserve the
entire existing `state/`, replace `nSpire2.tns` and the complete old `data/`
folder, and install only matching optional languages into `data/`. It warns
against replacing an existing state folder with the archive's empty one.
There is no required removal of a particular user-state file. The public
index and every concern file remain required; optional languages can be omitted.
First-launch language fallback and the included developer console are described.
Version 0.1.0-ui22, NSP2/46, save format 17, the pinned original reference and
the runtime/resource checkpoints match the packaged BUILD-INFO. Licenses and
notices are present. README and BUILD-INFO make no claim of UI22 Firebird
execution; NOTICE preserves the separately qualified historical evidence.

## Earlier source and actual-language evidence

The UI/lifecycle runtime and public tests above were executed from published
checkpoint `701c85a1d270bed2beca20a5481388d22ca79a9d`. The actual-language inputs
were the complete UI21 resource controls, whose text and UI catalogs are retained
by UI22. Earlier failed journals record the obsolete two-language test fixture,
a small synthetic fixture's missing hover text, and the overlay Back shortcut
that previously entered Pause; the corrected checkpoint passes all five scoped
synthetic UI/lifecycle targets and all actual-language runs above.

The first complete 427-action / 11,051-frame actor resource input also passed
`character_death_ui9_regression`, `world_ui_regression` and
`five_character_ui_regression` through the production renderer. These checks
cover all five characters' combat/event/rest/shop death routing, terminal holds,
particle decay, and actual Osty growth, kill, dead-loop and revival gameplay.
On the earlier complete-color control, an additional ordinary-command
Osty flow passed 1,989 checks: Bodyguard growth, Bone Shards death, all 17 death
frames elapsed, the actual dead-loop route, and the 12-frame revival through
terminal idle. Seven full-scene captures were inspected, including the corpse
and the middle/end of revival; canonical saves remained unchanged while the
presentation clock advanced.
The native source-pixel oracle and final-resource receipts are separate release
gates. Actual resource acceptance and original-source pixel equality do not
establish device execution or physical-calculator frame rate.

The accepted audio/multiplayer/Timeline exclusions and static Ancient-scene
exception remain explicit in the product scope and retained-differences ledger.
Finite sampled affliction shaders remain a distinct material representation;
this UI acceptance does not claim complete Godot GPU equivalence.
