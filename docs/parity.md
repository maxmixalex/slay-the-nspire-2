# Original-game fidelity contract

Reference remains the owner's pinned v0.111.0 / 41cef1ea. Do not silently mix other releases or Slay the Spire 1 rules into this build. Audio, multiplayer and Timeline progression are excluded, not deferred; animation remains in scope.

Reflow, larger targets, scrolling, detail overlays and keyboard/touchpad adaptation are allowed for readable use at 320x240. Temporal visual sampling has a separate performance/fidelity boundary below; it is not a readability adjustment. Card effects, pools, enemy behavior and random probabilities must not be changed merely to make a demo work. Unsupported content and substitutions remain explicit gaps. Portraits and extracted text alone do not constitute supported gameplay.

## Current coverage and evidence

The selected local **UI18** runtime is
`44d7fb79f7d87d522038a074fa80d25e2bdeff31`, with NSP2/42 resource conversion
`3acb5b310d2d2cb42dbf98f21a79f0b45d8b42ed`. Use
[the UI18 delivery record](ui18-delivery-0.17.md), the current
[README](../README.md), and [WORK_STATE.md](WORK_STATE.md) for installation,
final acceptance, package identities and measured performance. The complete
UI18 suites contain **157 native tests and 345 Python tests**. Their preceding
integrated run at `299ee0085097cc281e1745a67dedcf36f395482c` passed 157/157
and 345/345; the delivery record identifies later matching-runtime results
separately rather than relabeling those earlier runs. The final source/document
tip is recorded separately from the tested program and resource converter.

UI18 closes the locale-specific history/daily calendar formatting gap; the
[calendar report](calendar-locales-ui18-0.17.md) distinguishes that implemented
behavior from the retained RTC/time-zone boundary. The
[feedback ledger](ui17-feedback-acceptance-0.17.md) and
[UI15 reconciliation](ui15-retained-differences-0.17.md) identify the already
implemented seed, Vantom, monster-command and reported UI work. Earlier pending
paragraphs do not supersede those implementations.

**Historical UI16 evidence:** runtime
`4f3c7abfb1c580d37f63a158385ff79a47e87874` and its 147/147 native result remain
in [the UI16 ledger](ui16-delivery-0.17.md), with the
[UI14 reconciliation](ui14-retained-differences-0.17.md) preserving its older
scope. These are not UI18 measurements. The latest source has not been
published to GitHub: automatic approval review rejected outbound source
publication, so the local source archive and all-ref Git bundle are the recovery
path. No new remote ref or CI success is claimed.

Earlier versions of this file described the 0.12 two-character subset; that
is historical. The complete **0.16** card/potion foundation is retained, and
the original world drives the playable three-act route. The UI3 runtime at
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
| Original resources | Current UI18 NSP2/42 resources use an NSM1 index, eight required artwork concerns, and independent NSL3 locale pages; exact group compaction preserves retained pixels and timing | The UI18 delivery record gives final installed totals and the separate 16-language calendar glyph addition. Historical UI16 resources totaled 79,262,097 bytes, or 81,022,714 bytes with its launcher; those are not current totals. Complete logical editions reconstruct exactly. Image/glyph/string decoding remains on demand without integrity-only runtime scans. |
| UI and presentation | Original assets and scene relationships for title/run setup, combat, events/ancients, regional rooms, rewards, map windows and travel | Actual resource renders and cross-backend tests validate the port. They do not substitute for original-application captures and visual acceptance. |

## Adaptations versus remaining fidelity work

The calculator needs readable font sizes, wrapping/pagination, usable hit regions,
scrolling, a constrained viewport and keyboard/touchpad controls. The combat hand
fan, focus scale and inspection overlays document such choices. Each adaptation
still needs visual review; a small screen does not justify changing gameplay.

The historical UI3 statements about missing rainbow origins, generic dynamic
effects, and command timing are superseded by the UI14/UI15 reconciliation
ledgers. In particular, separate rainbow/blur clocks, the named shader and
particle families, all solo-reachable monster command programs, and Vantom's
750 ms bone transition have production implementations. They are not newly
completed merely by being retested in UI16.

Separate representation and evidence limits remain outside the readability
allowance:

- Original scene transitions and effects must be compared across their full
  lifecycle. Source19's act-entry camera/banner passed its own frozen tests
  and actual-resource checks; it does not establish full original-PC parity.
- Map rotation/dot noise substitutes stable local cosmetic noise for the
  original process-global chaotic stream. That is an explicit cosmetic gap,
  independent of the saved map RNG and the readable viewport.
- RGB565/LDR output, integer rasterization, bounded CPU particles and sampled
  material phases do not establish continuous Godot GPU equivalence. Original
  source sampling and tested frame comparisons do not establish a complete
  screen-by-screen desktop acceptance. See the newer per-family ledgers for
  actual retained frame counts and timings; the old generic bake rate is not
  a description of every current actor and effect family.
- Complete Ancient actor animation remains the previously accepted storage
  exception. Its current event scene is static; gameplay, dialogue, options,
  healing, and Ancient card materials are implemented. Audio, multiplayer,
  and Timeline remain outside the agreed single-player scope.

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
