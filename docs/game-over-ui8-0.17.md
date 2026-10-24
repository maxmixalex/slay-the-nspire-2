# UI8 game over: observed score, badges, discoveries, and retained powers

Reference: owner-supplied v0.111.0 / 41cef1ea C# and PCK. This records implemented behavior and its boundaries; it is not a whole-game fidelity percentage.

## Original source contracts

- `Nodes.Screens.GameOverScreen/NGameOverScreen.cs`: initial death/victory screen, Continue → summary, score lines, badge container, discoveries, View Run child screen, and Main Menu. Continue/Main Menu use Select on release; View Run uses Confirm on release.
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

View Run selects the actual current run in the retained ring. It opens the existing full history map/relic/tiny-deck inspection screen and Back returns directly to the current summary. This child route does not expose the compendium root. It neither finishes the run twice nor deletes an unrelated on-disk save.

Input captures its terminal-run context at key-down and commits on release. A modal or screen change between press and release consumes the old input. Pointer hit bounds follow the original projected controls. Badge/discovery hover tips also work with directional keyboard focus.

Retained victory powers exist only in live presentation. Loading an already-cleared reward save does not reconstruct unavailable powers. Resetting or entering a new battle clears the retained view. The root reward-room underlay draws the retained player strip only while the identity-bound freeze is valid.

## Verification and remaining differences

Short syntax checks pass for all changed runtime translation units and the new/updated tests. `git diff --check` passes. The 31 added PCK image paths and 35 bilingual text entries were read and validated without conversion; resource conversion is performed from the parent's separate immutable asset checkpoint. Full native/ARM/WASM acceptance runs from the parent's runtime checkpoint, and results must be reported separately.

`game_over_facts_ui8_regression` checks score arithmetic and final-room semantics, nineteen-badge representative boundaries, positive facts versus incomplete prefixes, actual shop/removal and rest cancellation, actual generated-and-played twenty-card turn, blocked/unblocked damage, same-point Perfect invalidation, discovery deduplication, NSH6 round trips and old NSH5 decoding, release input contexts, View Run return, and unchanged simulation save bytes. Existing game-over flow tests now verify observed gold rather than the removed balance row; NSH1 and NSH5 migration tests construct genuine historical prefixes. Power regression covers live victory with/without feedback, persistence/reset, and semantic save equivalence.

Remaining material visual/flow differences are explicit: the five hero Die clips are absent from the old character resource directory (Idle/Attack/Hurt/Cast/Camp); the source score/badge/discovery cascade and transition shader are not fully animated here; the original per-floor detailed acquired/removed/upgraded/bought/reward-card-choice journal is not yet retained. First-time tutorial overlays are also still absent. Timeline unlock presentation, audio, and multiplayer are outside this single-player port's scope; original disabled Achievements controls are not counted as missing behavior.
