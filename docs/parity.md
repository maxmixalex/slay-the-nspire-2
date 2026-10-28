# Original-game fidelity contract

Reference remains the owner's pinned v0.111.0 / 41cef1ea. Do not silently mix other releases or Slay the Spire 1 rules into this build. Audio, multiplayer and Timeline progression are excluded, not deferred; animation remains in scope.

Reflow, larger targets, scrolling, detail overlays and keyboard/touchpad adaptation are allowed for readable use at 320x240. Temporal visual sampling has a separate performance/fidelity boundary below; it is not a readability adjustment. Card effects, pools, enemy behavior and random probabilities must not be changed merely to make a demo work. Unsupported content and substitutions remain explicit gaps. Portraits and extracted text alone do not constitute supported gameplay.

## Current coverage and evidence

Verified progress is synchronized from `work/0.17-world-parity` to `main`.
Earlier versions of this file described the 0.12 two-character subset; that is
historical. The complete **0.16** card/potion foundation is retained, and the
original world drives the playable three-act route. The UI3 runtime at
`ead747dcdffded08d641445a811af48a4fafd1de` passed all **73 native Release suites**;
`d6b3d31b9f00ecd144ab1b4a1748fa1c2e731c5a` then passed all **73 ASan/UBSan suites**
with the same runtime and corrected test workspace allocation. See
[runtime acceptance](validation-ui3-runtime-0.17.json) and [WORK_STATE.md](WORK_STATE.md).
Each report retains its executed source and resource identity; separate partial
runs are not combined into a claimed complete pass.

| Area | Implemented integration | Evidence and limits |
| --- | --- | --- |
| Five characters, colorless and potions | Five single-player pools, their generated/status cards, 53 colorless models and 64 potions; event/curse additions bring the card registry to 558 | The source17 full suite retains character, colorless, potion, migration and UI tests; 860 character/Giant Rock variants execute acquisitions. Registry counts are not an original-reference interaction proof. |
| Original world | Seven-column maps, four regional templates across three act positions, original room queues, unknown-room odds, ancients, events and the final Architect continuation | World/map/lifecycle regressions exercise real routing and canonical save continuation. The old adapted expedition remains a separate compatibility path. |
| Enemies | Bounded eight-slot runtime with moves, intents, powers, summons, identities, revivals and exit/reward continuations | 109 IDs include compatibility/display models. The test driver executes all 87 non-display encounter definitions at A0 and A9, producing 174 variants, plus targeted boundary scenarios. Further audits can still identify rule bugs. |
| Original resources | Current NSP2/20 resources separate one shared artwork/animation file from English and Chinese text/glyph directories; retained image compression preserves pixels and timing | Final bilingual resources total 81,100,703 bytes, or 82,084,385 bytes with the launcher before saves. Both complete packs reconstruct exactly. Image/glyph/string decoding is on demand; runtime integrity hashes and eager payload scans are removed. Earlier NSP2/15 sizes and complete catalog proofs remain historical. |
| UI and presentation | Original assets and scene relationships for title/run setup, combat, events/ancients, regional rooms, rewards, map windows and travel | Actual resource renders and cross-backend tests validate the port. They do not substitute for original-application captures and visual acceptance. |

## Adaptations versus remaining fidelity work

The calculator needs readable font sizes, wrapping/pagination, usable hit regions,
scrolling, a constrained viewport and keyboard/touchpad controls. The combat hand
fan, focus scale and inspection overlays document such choices. Each adaptation
still needs visual review; a small screen does not justify changing gameplay.

Separate implementation differences remain outside that allowance:

- Original scene transitions and effects must be compared across their full
  lifecycle. Source19's act-entry camera/banner passed its own frozen tests
  and actual-resource checks; it does not establish full original-PC parity.
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
Source19 separately passed 31 affected suites and 480 exact frames / 30 saves
across 23 scenarios; see [validation-source19-0.17.json](validation-source19-0.17.json).
Source20 separately passed ten affected sanitizer suites, all three builds,
linked ARM decoder/ABI checks and the complete lossless resource conversion;
see [validation-source20-0.17.json](validation-source20-0.17.json).
These establish regression and backend agreement within the tested cases, not
complete original-game parity. The older fifteen-target results in
`combat-ui-0.12.md` remain historical. UI3 separately passed actual bilingual
native/WASM comparisons (23 scenarios, 649 frames and 30 saves per language),
ordinary Firebird battle/save/exit flows and real offline-browser DOM and local
save continuation. See [Firebird](validation-ui3-firebird-0.17.json) and
[browser](validation-ui3-browser-0.17.json) reports for their exact scope and
remaining cold-page delay. These do not certify physical-handheld frame rate,
the complete original game's visual fidelity or measured peak RAM.

Hover, paging, details and visual clocks must not consume gameplay RNG or mutate gameplay state. Choices retain actual instance IDs. A changed revision, new modal or scroll cancels captured pointer actions. The owner authorized GitHub Actions in `maxmixalex/slay-the-nspire-2`; native sanitizer, conversion and portable-build jobs are enabled and passed for UI3.
