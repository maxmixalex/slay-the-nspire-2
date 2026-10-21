# Original-game fidelity contract

Reference remains the owner's hash-pinned v0.111.0 / 41cef1ea. Do not silently mix other releases or Slay the Spire 1 rules into this build. Audio, multiplayer and Timeline progression are excluded, not deferred; animation remains in scope.

Reflow, larger targets, scrolling, detail overlays, offline visual sampling and keyboard/touchpad adaptation are allowed for 320x240. Card effects, pools, enemy behavior and random probabilities must not be changed merely to make a demo work. Unsupported content and substitutions remain explicit gaps. Portraits and extracted text alone do not constitute supported gameplay.

## Current coverage and evidence

The active branch is `work/0.17-world-parity`. Earlier versions of this file
described the 0.12 two-character subset; that is historical and no longer the
current implementation. The complete **0.16** card/potion foundation is retained,
and the original world now drives the playable three-act route. Source17
`26a9097521a971b69a4b58494985c6062a24017e` is the latest complete 52-suite baseline
recorded in [validation-source17-0.17.json](validation-source17-0.17.json).
Follow [WORK_STATE.md](WORK_STATE.md) for later changes and their own validation;
results from different source revisions must not be combined into a full pass.

| Area | Implemented integration | Evidence and limits |
| --- | --- | --- |
| Five characters, colorless and potions | Five single-player pools, their generated/status cards, 53 colorless models and 64 potions; event/curse additions bring the card registry to 558 | The source17 full suite retains character, colorless, potion, migration and UI tests; 860 character/Giant Rock variants execute acquisitions. Registry counts are not an original-reference interaction proof. |
| Original world | Seven-column maps, four regional templates across three act positions, original room queues, unknown-room odds, ancients, events and the final Architect continuation | World/map/lifecycle regressions exercise real routing and canonical save continuation. The old adapted expedition remains a separate compatibility path. |
| Enemies | Bounded eight-slot runtime with moves, intents, powers, summons, identities, revivals and exit/reward continuations | 109 IDs include compatibility/display models. The test driver executes all 87 non-display encounter definitions at A0 and A9, producing 174 variants, plus targeted boundary scenarios. Further audits can still identify rule bugs. |
| Original resources | Complete private NSP2/14 directory with 45,724 images, 1,695 clips, 11,915 glyph records and 6,100 strings | Canonical decoding, preserved historical payloads, resource bounds and actual UI loading passed. The 231,312,495-byte pack is too large for the target's documented storage; compression is an unresolved delivery gate. |
| UI and presentation | Original assets and scene relationships for title/run setup, combat, events/ancients, regional rooms, rewards, map windows and travel | Actual resource renders and cross-backend tests validate the port. They do not substitute for original-application captures and visual acceptance. |

## Adaptations versus remaining fidelity work

The calculator needs readable font sizes, wrapping/pagination, usable hit regions,
scrolling, a constrained viewport and keyboard/touchpad controls. The combat hand
fan, focus scale and inspection overlays document such choices. Each adaptation
still needs visual review; a small screen does not justify changing gameplay.

Separate implementation differences remain outside that allowance:

- Original scene transitions and effects must be compared across their full
  lifecycle. The source19 act-entry camera/banner work has its own new tests
  and checkpoint; it cannot inherit source17's completed results.
- Parameterized rainbow text now follows the pinned engine's OkHSV function,
  but its shared presentation-clock origin does not recreate each original
  label's elapsed-time origin. See [rich-rainbow-0.17.md](rich-rainbow-0.17.md).
- Map rotation/dot noise substitutes stable local cosmetic noise for the
  original process-global chaotic stream. That is an explicit cosmetic gap,
  independent of the saved map RNG and the readable viewport.
- Original dynamic shader/particle layers, exact animation phase and the
  complete screen-by-screen comparison still require acceptance. Offline
  sampling, original assets and metadata counts alone do not establish that.
  The current enemy bake samples loops at 8 fps and actions at 12 fps while
  preserving their full durations. That temporal sampling is a performance
  adaptation requiring separate review, not a font/readability exception.

The **95%+** goal is therefore still an acceptance target, not a reported result.
Do not relabel unsupported effects, phase differences or resource omissions as
the permitted 5% readability adjustment. No percentage is inferred from passing
test counts or a weighted checklist.

## Evidence boundary

Distinguish discovery, inspected semantics, implementation, original-reference
trace agreement and target testing. Source17's 52 native suites and 357 exact
native/WASM frames / 25 saves are recorded at their immutable source above.
They establish regression and backend agreement within the tested cases, not
complete original-game parity. The older fifteen-target results in
`combat-ui-0.12.md` remain historical. No new Firebird, physical-handheld,
browser DOM/persistent-storage, sustained frame-rate or peak-RAM certification
is claimed for the current world stage.

Hover, paging, details and visual clocks must not consume gameplay RNG or mutate gameplay state. Choices retain actual instance IDs. A changed revision, new modal or scroll cancels captured pointer actions. Remote CI stays disabled until explicitly re-enabled by the owner.
