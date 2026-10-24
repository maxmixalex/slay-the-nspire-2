# UI8 game over: observed score, badges, discoveries, and retained powers

Reference: owner-supplied v0.111.0 / 41cef1ea C# and PCK. This records implemented behavior and its boundaries; it is not a whole-game fidelity percentage.

## Original source contracts

- `Nodes.Screens.GameOverScreen/NGameOverScreen.cs`: initial death/victory screen, Continue → summary, score lines, badge container, discoveries, and Main Menu. Continue/Main Menu use Select on release. The pinned source creates a View Run child-screen handler but `_Ready()` disables its button and the scene hides it; no later enable call exists. The port therefore hides and disables this entry, including its Confirm hotkey. Normal Compendium → Run History remains available.
- `Nodes.Screens.GameOverScreen/{NScoreLine,NBadge,NDiscoveredItem,NRunSummary}.cs` and `scenes/screens/game_over_screen/{game_over_screen,score_line,badge,discovered_item,game_over_button}.tscn`: uniformly project the 1680 × 1260 viewport. The banner is 125 × 31; score rows 100 × 9 with a 7-pixel step; badge cells 13 × 13; bottom buttons 50 × 12. Actual source textures replace the former oversized Proceed and unrelated merchant image.
- `Runs/ScoreUtility.cs`: weighted per-act floor points, cumulative gold gained / 100 for one player, 50 points per elite, 100 per boss, then the original binary64 Ascension multiplier and truncation. The source unconditionally excludes a final Elite room; a final Boss is excluded only on a loss. This pinned behavior is retained.
- `Models.Badges/BadgePool.cs` and the nineteen single-player badge models: original order, final deck/relic/HP/gold conditions, actual 20-card turn, actual shop spending, rest choices, unknown map points, and perfect bosses. The four multiplayer-only badges are outside the single-player target.
- `Nodes.Screens.RunHistoryScreen/NRunHistory.cs`: encounter/event loss localization, character/grammar substitution, and seed-local deterministic false-victory/abandon quote selection. Presentation never advances simulation RNG.
- `Localization.Formatters/LocaleNumberFormatter.cs`: `n()` uses N0 numeric grouping and midpoint-to-even rounding for the shipped English/Chinese locales.
- `Nodes.Combat/{NPowerContainer,NPower}.cs`: power nodes retained after `IsInProgress` ends. Before the controller's canonical victory clear, the live UI retains shown/observed power entries, values, source facts, and acquisition order.

## Actual observations and persistence

`UiCoreObserver` observes completed core commands. Gold gained remains separate from final balance. Merchant spending is recorded only when the price is actually paid, including a confirmed removal. Cancelled Smith/Cook/removal selection does not invent spending or rest completion. Unblocked player HP loss updates the current map point. A later hit at the same point also invalidates prior Perfect credit there, matching the A10 second-boss behavior. Twenty-card turns are captured from the shared finished-card counter before its turn reset.

The current run stores discovery counts and only the first ten original model IDs per category, in observation order, because that is what `NRunSummary` displays. Existing global discovery bits prevent duplicates. The UI shows the original count/body/title plus the first ten names and the original ellipsis when needed. It does not copy a large collection bitmap into each historical record.

NSH6 appends the new fields after the unchanged NSH1–5 prefix:

1. A versioned 15-byte serialized facts record for each populated run, then three bytes per observed floor (map type, rest-choice mask, observed/damage/completed-boss bits).
2. Four current-run discovery counts and up to ten IDs per category.
3. The local cumulative Architect damage total.

Old files decode with the new fields empty. Resuming an old record begins forward observation with `complete = false`; missing past gold, fights, turns, or damage are never backfilled. Score-dependent badges/score are withheld for incomplete histories. Positive observed conditions and actual final-deck conditions can still be shown. No remote leaderboard or global Architect total is fabricated. The daily score helper uses the original local encoded formula only when observations are complete.

Native measured sizes: `State = 200368` (unchanged), `UiFloorHistory = 32` (unchanged; former padding used), `UiRunFacts = 16`, `UiRunDiscoveries = 88`, `UiRunHistory = 7560`, `UiProgress = 274600` (+616 bytes from stage5), `CombatStatus = 51356` (no additional arrays). The existing host codec path and bounds apply to all three hosts; the final ARM gate still measures every static instance.

## UI lifecycle

Terminal presentation is bound to the actual run/character/floor/terminal-phase identity and resets on load or a new run. It does not finish the run twice or delete an unrelated on-disk save. The internal history lookup still uses the actual current record, but the original hidden View Run entry cannot activate through pointer, semantic UI action, or physical Confirm.

`GameOverPresentation` implements the source timeline without changing the simulation or persistence schema:

| Presentation | Pinned source sequence |
| --- | --- |
| Initial entry | Event-room full blackout and creature fade: 200 ms; backdrop transition: 1,500 ms; UI fade: 250 ms. Continue enables at 1,950 ms in event rooms and 1,750 ms elsewhere. |
| Continue | 500 ms Quart-out entry from +190 source pixels; opacity 0 → 1. Input enables when entry starts, as in the source. |
| Summary | Dark red summary backstop fades for 500 ms; banner moves up 32 source pixels using 500 ms Cubic-out. |
| Score lines | Each line enters from −50 source pixels using 300 ms Spring-out, then waits 100 ms before the next. All lines are followed by a 500 ms pause. |
| Badges | Wait 250 ms; each badge enters from +40 source pixels using 250 ms Spring-out; finish with a 500 ms pause. |
| Discoveries | Header fades for 250 ms; the first item starts after 100 ms, then visible categories enter sequentially from +100 source pixels using 300 ms Back-out. |
| Main Menu | Enables after the discovery sequence and fades over 500 ms. `NReturnToMainMenuButton._Ready()` applies its additional −140 source-pixel offset; the sole visible menu button is centered. |
| Loss quote | Fade in over 1,500 ms with a 2,000 ms Expo-out position change. Summary replaces an already visible opening quote only after its 250 ms fade-out, waits 1,000 ms, then fades in the encounter quote. Pressing Continue at exactly zero quote opacity retains the source's no-replacement branch. |

Timeline unlock/score-bar presentation is excluded rather than filled with invented unlock progress. The offline Daily path also does not simulate a network leaderboard.

Input captures its terminal-run context at key-down and commits on release. A press captured while disabled remains consumed even if the button becomes ready before release. A modal or screen change between press and release also consumes the old input. Pointer hit bounds follow the moving controls. Badge/discovery hover tips use their actual visible positions; directional keyboard focus follows the source's completed-summary gate.

The animation state occupies 40 bytes. It caches nineteen immutable badge tiers in 38 bits after summary begins, avoiding repeated deck/floor scans during every render and hover. Completeness checks no longer calculate the software-binary64 score merely to inspect a flag; the actual score retains its original arithmetic.

The character module supplies the five original non-looping Die clips in resource version 29, with exact terminal frames. `game_over_actor` is layered above the entry backstop and below the summary backstop; event creatures share the 200 ms entry fade. Older resource versions remain readable and do not substitute an unrelated portrait or Hurt/Camp pose.

Retained victory powers exist only in live presentation. Loading an already-cleared reward save does not reconstruct unavailable powers. Resetting or entering a new battle clears the retained view. The root reward-room underlay draws the retained player strip only while the identity-bound freeze is valid.

## Verification and remaining differences

Stage6 native acceptance passed the new observed-facts, game-over UI, power-order, input, and auxiliary-screen suites. Its unrelated full-suite result was 93/97; the run-modes suite's obsolete NSH3 expectation has since been corrected to NSH6 with an explicit old-prefix fixture. Stage7 short syntax checks pass for the changed runtime units and game-over tests; a standalone presentation check passes the original timing boundaries and confirms the 40-byte size. Full Stage7 native/ARM/WASM acceptance runs from the parent's next immutable checkpoint and must be reported separately.

`game_over_facts_ui8_regression` checks score arithmetic and final-room semantics, nineteen-badge representative boundaries, positive facts versus incomplete prefixes, actual shop/removal and rest cancellation, actual generated-and-played twenty-card turn, blocked/unblocked damage, same-point Perfect invalidation, discovery deduplication, NSH6 round trips and old NSH5 decoding, source timing boundaries, release input contexts, the original hidden View Run gate, and unchanged simulation save bytes. Existing game-over flow tests wait for the actual intro/summary timeline, check absent early hits, and verify observed gold rather than the removed balance row. NSH1 and NSH5 migration tests construct genuine historical prefixes. Power regression covers live victory with/without feedback, persistence/reset, and semantic save equivalence.

Remaining material visual/flow differences are explicit: the entry backdrop uses a bounded raster fade in place of the original threshold shader; the original per-floor detailed acquired/removed/upgraded/bought/reward-card-choice journal is not yet retained. First-time tutorial overlays are also still absent. Die resource/particle acceptance is reported by the character module's Stage7 validation, separately from these UI timing checks. Timeline unlock presentation, audio, and multiplayer are outside this single-player port's scope; original disabled View Run and Achievements controls are not counted as missing behavior.
