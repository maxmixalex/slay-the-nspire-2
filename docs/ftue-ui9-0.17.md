# Original single-player tutorials

This stage restores the thirteen single-player FTUE identities in the pinned
original source. The initial question belongs to the release of **Embark** on
the character screen; it is not a startup screen or a language-selection flow.
Multiplayer tutorial and Timeline remain outside this port's agreed scope.

## Persistent identities

`include/ftue.hpp` fixes the following bit order in the per-profile metadata:

| Bit | Identity | Original key |
|---:|---|---|
| 0 | AcceptTutorials | `accept_tutorials_ftue` |
| 1 | CombatRules | `combat_rules_ftue` |
| 2 | MapSelect | `map_select_ftue` |
| 3 | CanPlayCards | `can_play_cards_ftue` |
| 4 | CannotPlayCard | `cannot_play_card_ftue` |
| 5 | PowerCard | `power_card_ftue` |
| 6 | Shuffle | `shuffle_ftue` |
| 7 | CombatReward | `combat_reward_ftue` |
| 8 | RelicReward | `obtain_relic_ftue` |
| 9 | ObtainPotion | `obtain_potion_ftue` |
| 10 | Merchant | `merchant_ftue` |
| 11 | RestSite | `rest_site_ftue` |
| 12 | AscensionSingleplayer | `ascension_singleplayer_ftue` |

Bits 13 and 14 represent Enabled and Initialized. Bit 15 is reserved and
rejected. Initialized distinguishes profiles created with this feature from old
profiles whose viewing history is unknown; it is not an invented completion.

The original `SerializableProgress` and `ProgressState` default EnableFtues to
true. `ProgressSaveManager.SeenFtue` returns true when ordinary tutorials are
disabled; `SeenPopup` checks actual completion independently of that preference.
The Ascension announcement uses **SeenPopup**. Accordingly, declining tutorials
records the initial question and disables ordinary FTUEs while leaving the
unseen Ascension announcement eligible. Both initial-question responses mark
the question before continuing the original Embark action.

The shared application APIs are `seen_ftue`, `seen_popup`, `mark_ftue`,
`set_ftues_enabled`, and `reset_ftues`. Mutations use the existing dirty/retry
progress-save path. They do not alter the simulation State, its RNG, or run-save
format. Duplicate completion marks do not write repeatedly.

## NSH8 compatibility and new-profile detection

NSH8 appends only one little-endian `uint16_t tutorial_flags` after the complete
NSH7 tail. Existing NSH7 field bytes retain their positions. Flags that contain a
preference or completion without Initialized are rejected. An all-zero record
remains the explicit legacy compatibility state.

NSH1–7 decoding leaves all tutorial bits zero. Queries suppress the new UI for
those existing profiles without claiming that any original tutorial was seen.
The internal reset API can opt the profile into the original fresh state:
Initialized plus Enabled, with no completion bits. Successful old metadata
decoding always wins over any new-profile heuristic. Automatic completion marks
cannot initialize that unknown legacy state: an ordinary late-floor reward or
merchant interaction must not unexpectedly make its Ascension popup eligible.
Explicit reset or a deliberate preference change can initialize it.

When a real profile host has no successfully loaded history, the application
waits for its existing current-run read. A valid old run still identifies an
existing profile even if no optional history file exists. A new empty profile,
or a deleted profile subsequently recreated, receives Initialized plus Enabled.
This avoids calling Web's asynchronously cached `profile_info` at startup and
does not scan all profile histories. Hosts without persistent profile callbacks
retain the old test/replay behavior; a tutorial test explicitly calls the reset
API to opt in.

The host callback currently returns only success/failure for history loading.
It cannot distinguish every corrupt-file or I/O-error case from absence. The
ordinary valid-run probe provides the conservative old-run boundary; an
unreadable history and an unreadable/absent run cannot establish old viewing
history. No data is fabricated to fill that gap.

The two-byte tail raises the full encoded profile bound from 295,347 bytes
(NSH7) to **295,349 bytes** (NSH8). C++ alignment raises `sizeof(UiProgress)` from
308,296 to **308,304 bytes**; there remains one live App progress object. With
the separately measured dense NSZ run file, three-profile history-update peaks
become 1,910,754 bytes after migration and 2,305,029 bytes with three older raw
backups. The unconditional raw-fallback peak is 2,699,304 bytes. These figures
exclude file-system allocation metadata and optional files.

## Original release gating

`NResetTutorialsButton._Ready` reveals its divider and row only when
`!NGame.IsReleaseGame()`. In the pinned `NGame.cs`, IsReleaseGame returns true;
both nodes also start with `visible = false` in `settings_screen.tscn`.
Consequently the ordinary release Settings UI does **not** gain a reset row.
The internal reset API supports semantic regression testing and any explicitly
enabled developer path without exposing a non-original release control.

## Trigger and continuation contract

The following conditions were independently checked against the pinned C#
callers, as well as the tutorial classes. A screen's presence in the resource
catalog does not establish that these interactive paths work.

| Tutorial | Actual source trigger | Completion and continuation |
|---|---|---|
| AcceptTutorials | Character selection's Embark action, before marking the player ready. Entering character selection alone does not ask. | Yes and No mark the question before invoking the same Embark continuation. No additionally disables ordinary tutorials. |
| CombatRules | `CombatManager.StartCombatInternal`, after combat setup and the initial `StartTurn`; the modal is installed before its contents start. | Three pages support previous/next. Pointer buttons activate on release; physical arrows use the source's nonrepeating `IsActionPressed`. Completion is marked only when Next is activated on page three; the normal combat-start banner then appears. |
| MapSelect | `NMapScreen.InitMapPrompt`, when unseen, after a 100 ms delay. | Marks when shown; confirmation restores the starting node's ordinary layer. Selecting the initial node must not bypass the active modal. |
| CanPlayCards | Releasing End Turn while the local player's `HasCardsToPlay()` is true. | Marks when shown. Confirmation returns to the same turn; it does not secretly end the turn. Three end-turn attempts with no playable cards automatically mark it without a popup. |
| CannotPlayCard | A manual play attempt fails `CanPlay` specifically with `EnergyCostTooHigh`. | Marks when shown. Stars, status restrictions, missing targets, and inherently unplayable cards are not this trigger. Eight successful manual plays automatically mark it without a popup. |
| PowerCard | The card-reward selection screen contains a Power card. | Highlights the first displayed Power, marks when shown, and returns to the same unclaimed selection. This is not triggered by every Power entering the hand or deck. |
| Shuffle | Draw is empty and Discard is nonempty in `CardPileCmd.ShuffleIfNecessary`. | Marks when shown; the source awaits confirmation **before** shuffling the discard pile into the draw pile. |
| CombatReward | A normal terminal reward screen's Proceed button currently says Skip and is released while the tutorial is unseen. | Marks when shown. Proceed is hidden during the tutorial, then restored; confirmation does not leave the room. Boss/victory terminal paths and nonterminal reward overlays use their separate original branches. Proceed after collecting rewards automatically marks it only after act floor four. |
| RelicReward | A displayed reward list contains a Relic reward, after two process frames; or the treasure relic selection appears, after its one-second wait. | Marks when shown and returns to the same unclaimed relic. A global `RelicObtained` event is not a substitute for this reward-display trigger. |
| ObtainPotion | `NPotionContainer.Add` accepts a newly added potion with `isInitialization == false`. | Marks when shown. Restoring an existing potion belt and rejected/full-belt additions do not retrigger it. |
| Merchant | The player tries to leave the merchant room before opening the inventory. | Marks when shown and keeps the room open. Merely opening the inventory marks it without a popup; simply arriving at the merchant does not show it. |
| RestSite | The rest-site choices appear, after a 500 ms wait. | Marks when shown. Choices and header are initially hidden and then fade in while the modal blocks gameplay input. |
| AscensionSingleplayer | Standard mode has positive maximum unlocked Ascension across any character, and `SeenPopup` is false. | Starts after a one-second delay with a 300 ms entrance. Marks only on confirmation; disabling ordinary tutorials does not suppress this popup. |

The two automatic-disabling counters have different source lifetimes:
`NCardPlay._totalCardsPlayedForFtue` is static for the process, whereas
`NEndTurnButton._endTurnWithNoPlayableCardsCount` belongs to a combat button
instance. Neither counter is a reason to invent a completed tutorial in an old
profile's migrated metadata.

`NModalContainer.Add` admits only one open modal, and `NFtue` registers a blocking
screen with `NHotkeyManager` until exit. Confirming a tutorial must not also play
the underlying card, select a reward, advance a map node, open Settings, or
perform a pending End Turn in the same input dispatch.

The existing synchronous simulation cannot suspend a `DrawOne` command inside
an observer without changing its callers and saved continuation semantics. The
Shuffle lesson therefore captures the displayed pre-shuffle pile counts and
stops the presentation queue before its shuffle cue. Confirmation releases that
queue; the canonical simulation and RNG have already resolved. This is an
explicit presentation adaptation, not a claim that the original asynchronous
core wait has been reproduced.

Contextual descriptions use the original 22-pixel source label at the port's
four-pixel font band, with genuine regular and bold glyphs. The combat lesson
and acceptance popup use the five-pixel description band; their headers and
contextual titles use the corresponding original five/six-pixel face. Final
resource verification must include both languages and these exact bands rather
than relying on an eight-pixel fallback to fit the small source panels.

The browser acceptance keeps the ordinary run/profile/language flows and
explicitly declines the new question for those fixtures. A separate fresh
profile accepts tutorials and must reach the real first combat, visit all three
pages, and persist completion only after the last confirmation. It uses visible
controls and persisted metadata for observation, without enabling an ordinary
build's debug API. Its source now uses the implemented pointer-hit geometry and
checks the stored v17 turn after the lesson to detect an End Turn slipping
through. The matching ordinary HTML passed this complete flow at checkpoint
`492915341106b129f730e46ed84d152637a24b7d`; see the executed browser receipt below.

## Source references

- `MegaCrit.Sts2.Core.Saves/SerializableProgress.cs`: EnableFtues and completed-key list.
- `MegaCrit.Sts2.Core.Saves/ProgressState.cs`: completion deduplication and ResetFtues.
- `MegaCrit.Sts2.Core.Saves.Managers/ProgressSaveManager.cs`: SeenFtue, SeenPopup, and immediate progress persistence.
- `MegaCrit.Sts2.Core.Nodes.Screens.CharacterSelect/NCharacterSelectScreen.cs`: OnEmbarkPressed asks before ready/start.
- `MegaCrit.Sts2.Core.Nodes.Ftue/NAcceptTutorialsFtue.cs`: Yes/No completion and continuation order.
- `MegaCrit.Sts2.Core.Multiplayer.Game.Lobby/StartRunLobby.cs`: single-player Ascension uses SeenPopup.
- `MegaCrit.Sts2.Core.Nodes.Screens.Settings/NResetTutorialsButton.cs`, `MegaCrit.Sts2.Core.Nodes/NGame.cs`, and `scenes/screens/settings_screen.tscn`: release visibility gate.
- `MegaCrit.Sts2.Core.Combat/CombatManager.cs` and `MegaCrit.Sts2.Core.Nodes.Ftue/NCombatRulesFtue.cs`: first-turn and three-page completion ordering.
- `MegaCrit.Sts2.Core.Nodes.Combat/NCardPlay.cs` and `NEndTurnButton.cs`: manual play, energy-only failure, and end-turn tutorial boundaries.
- `MegaCrit.Sts2.Core.Commands/CardPileCmd.cs`: pre-shuffle confirmation.
- `MegaCrit.Sts2.Core.Nodes.Screens/NRewardsScreen.cs` and `MegaCrit.Sts2.Core.Nodes.Screens.CardSelection/NCardRewardSelectionScreen.cs`: reward, relic, and Power display conditions.
- `MegaCrit.Sts2.Core.Nodes.Potions/NPotionContainer.cs`, `MegaCrit.Sts2.Core.Nodes.Rooms/NTreasureRoom.cs`, `NRestSiteRoom.cs`, `NMerchantRoom.cs`, and `MegaCrit.Sts2.Core.Nodes.Screens.Shops/NMerchantInventory.cs`: actual obtain/room interaction boundaries.
- `MegaCrit.Sts2.Core.Nodes.Screens.Map/NMapScreen.cs`, `MegaCrit.Sts2.Core.Nodes.CommonUi/NModalContainer.cs`, and `MegaCrit.Sts2.Core.Nodes.Ftue/NFtue.cs`: map timing, modal exclusivity, and global hotkey blocking.

## Verification status

`ftue_progress_regression` covers old NSH1–7 migration, exact NSH7-prefix
preservation, malformed/short NSH8 tails, all thirteen completion bits,
preference-versus-popup behavior, no startup profile scan, three independent
profiles, deleted-profile recreation, dirty-state retry, simulation byte
purity, and the fully populated 295,349-byte profile. At persistence source-ready
time, its source and the changed codec/application translation units passed
short syntax checks. The matching combined checkpoint
`492915341106b129f730e46ed84d152637a24b7d` subsequently passed **6,413 checks**
and exported the valid profiles. Receipt:
`ui9-ftue-progress-export-adc5a937cfbb`. The measured full profile is
**295,349 bytes**, `sizeof(UiProgress)` is **308,304**, and the serialized
tutorial field is **two bytes**. Persistence alone is not completion of the
interactive tutorial feature.

The optional `--export EXISTING_DIRECTORY` writes codec-produced legacy NSH7,
enabled/declined/full NSH8, and a valid raw v17 run for host integration tests.
`web_profiles_storage_test.js DIRECTORY` repeats its ordinary storage checks with
those exact exported records. The JavaScript host remains format-agnostic:
progress is an opaque base64 record independent of v17 runs and v4 preferences.
The real exported-record run passed at the same checkpoint, receipt
`ui9-ftue-web-valid-profiles-e711f101ff00`. It covers profile isolation, run-only
deletion, denied progress writes, preference fallback and failed preference
transactions, and deletion recovery across a new host instance without changing
the NSH8 bytes. The matching native profile regression passed **259 checks**,
receipt `ui9-ftue-ndless-profiles-a6b4bb661b47`, including current NSH8 summaries
and full tutorial-flag decoding beside its existing old-prefix and backup
recovery cases. All three journals recorded `source_changed: false`.

The exported full NSH8 record has SHA-256
`4c3628949e7de487aa449744e6b469de25ff2743630b77798a61158a18efe725` and was
handed to the separate ordinary-device capacity gate. It contains actual
codec-valid maximum counts; it is not used to claim a naturally played run or
completed UI interaction.

The final ordinary bilingual HTML passed its full browser script at the same
checkpoint, receipt `ui9-ftue-offline-browser-local-dcab07ff6e07`. The script
recorded **16 completed flow checks and 27 captures** in isolated Chromium
156.0.8078.47, with no page errors or external requests. Profiles one and two
declined the question; a separate fresh profile accepted it, resolved its real
Neow choices in five control steps, acknowledged the first-map tutorial, and
entered the first combat. The three-page lesson supported both pointer and
physical navigation, blocked ordinary shortcuts and End Turn, and recorded
completion only on the final Next release (`0x6007`: initialized, enabled,
acceptance, map, and combat seen). The resulting canonical v17 save remained on
turn one. Reload and Continue preserved that completion without repeating the
lesson or changing profile one's declined preference. The existing setting,
language, save, profile-isolation and deletion flows also passed.

The first attempt, `ui9-ftue-offline-browser-5cbda53f30a4`, failed before page
creation because the command sandbox denied Chromium's local socket setup.
That failure is retained. The successful rerun received automatic approval for
the same frozen script in an isolated local browser; all external requests
remained blocked. The visible ten-second library measurement recorded 301
renders and zero provider reads. It measures that browser's CPU/Canvas path,
not calculator display FPS.

## Modal rendering and actual operation ownership

The runtime is implemented in `ftue_presentation.hpp`, `ftue_input.inc`, and
`ftue_ui.inc`. `App` owns one small transient modal record; no framebuffer,
canonical State copy, saved command, or RNG stream is added. The seven original
textures are optional append-only auxiliaries in NSP2/33. A prior catalog does
not acquire an invisible modal when its tutorial artwork is unavailable.

The following single-player operation boundaries are implemented separately:

| Tutorial | Trigger and completion | Interaction after confirmation |
|---|---|---|
| AcceptTutorials | Actual Embark, before seed generation or new-run mutation; mark on either answer | Continue that selected run once; No disables ordinary tutorials |
| CombatRules | First observed or opted-in restored turn-one combat after prepared poses and dealt hand; three pages, mark only after final Right | Add the original CombatStart banner, then the first PlayerTurn banner |
| MapSelect | First-map camera sweep or its cancel guard finishes, then100ms; root-node anchor; mark on show | Enable route selection; unseen tutorial also forces the first sweep in Fast mode |
| CanPlayCards | EndTurn with a genuinely playable card; three no-playable-hand attempts silently retire it | Stay on the same player turn |
| CannotPlayCard | Original reason flags equal EnergyCostTooHigh alone; eighth successful manual play retires it once per session | Stay in combat, or use the separately lifted EndTurn hit |
| PowerCard | First Power in a real card-reward row, including nested reward sources | Preserve every offered card and pending selection |
| Shuffle | Actual empty-draw/nonempty-discard command boundary in combat playback | Resume the existing presentation queue after acknowledgement |
| CombatReward | Ordinary terminal Skip while loot remains; completed ordinary Proceed after ActFloor4 silently retires it | Restore the reward list; do not leave or discard rewards |
| RelicReward | A visible relic row in ordinary or nested rewards, or an opened treasure holder after1s | Preserve the unclaimed relic |
| ObtainPotion | A genuine successful PotionObtained command, not initialized/restored belt contents | Return to the same room and potion operation |
| Merchant | Attempt to Leave without opening inventory; inventory Open retires it | Stay in the room, or use the lifted merchant hit to open inventory |
| RestSite | Original500ms initial wait, then choices fade and tutorial opens | Preserve every rest option and permit the next deliberate choice |
| AscensionSingleplayer | Standard character selection with an actual unlocked ascension; SeenPopup independent of Enabled | Original1s delay and300ms entry; mark on dismissal |

The modal rebuilds its own hit list and blocks ordinary pointer, scroll,
drawing, auxiliary-button, navigation, and game-command dispatch. Releasing a
key held before the modal appeared cannot trigger Peek or a reward alternative
under it. Original Select confirms on release. CombatRules Left/Right uses the
source's non-repeating key-down path, while its visible arrows use pointer
release. The initial question remains on character selection, never startup.

The contextual popup uses the original579×257 geometry projected by4/21,
original HSV material, the separate keep-aspect confirm texture, pulsing outline,
and authored arrows. Its source22px body and−2px line separation use real4px
normal/bold glyphs and a4px line advance. Page-body and Ascension-body source
sizes use real5px glyphs. Text is laid out before its visible-character animation;
rendering does not shrink an8px bitmap or replace a long paragraph with a new
summary. The relic popup's9px source4:3 overhang is kept inside the320px viewport
without changing its child sizes. Root-node map positioning follows the current
map transform. Actual energy, end-turn, pile, potion-tray, merchant-top, rest
choice, reward-row, card-face, and treasure-holder components are drawn again
above the dark backstop; bright rectangular screenshot cutouts are not used.

The source300ms backstop fade also survives closing. CombatRules page images
use CUBIC_OUT alpha and EXPO_OUT translation; text alpha is linear and its
visible ratio SINE_OUT. CombatStart uses its original300ms delay,750ms EXPO_OUT
scale,1.3s label entry,1s CUBIC_OUT exit, PlayerTurn creation at2.6s, and final
background disappearance at5.1s. The banner uses original Spectral/Source Han
Serif display glyphs at18px, an actual2→1 node zoom, and a bounded outline
coverage scratch array rather than new permanent art or cache storage.

The 5.1s banner is visual and does not hold gameplay input. In the original
`CombatManager.StartCombatInternal`, adding it is followed by the ordinary
0.5s Fast/1s Normal preparation wait and `StartTurn`, not an await of the banner.
`StartTurn` clears `PlayerActionsDisabled`, unpauses the action executor and
emits `TurnStarted`, which enables the hand and End Turn. The third tutorial
page likewise creates the banner and immediately closes its blocking modal.
The port retains its actual preparation/dealing guards and does not add an
incorrect five-second action lock after tutorial completion.

### Canonical and renderer boundary

The existing core resolves commands synchronously; the combat UI presents
queued hand, pile, health, status, resource, and effect changes at their source
command times. Shuffle therefore waits at a **presentation barrier**, holding
the pre-shuffle pile counts and all subsequent animation ticks until the modal
is acknowledged. It does not pretend that the already-resolved canonical RNG
was suspended, and does not roll back or replay gameplay. This architectural
boundary is explicit and is covered by unchanged canonical-State comparisons.
The room's shared animation tick currently waits with that barrier; independent
UI clocks continue. It is not a claim of frame-exact original asynchronous task
scheduling or the original renderer's GPU typography/material pipeline.

### Regression and visual acceptance

`ftue_ui9_regression` has both a synthetic optional-catalog fixture and an
actual-private-pack mode. It exercises the13 distinct identities, pointer and
physical-key ownership, disabled/legacy profiles, page completion, source
operation gates, root-map registration, source clocks, and State purity. Its
actual mode requires complete localized paragraphs at their real scene sizes
and captures every tutorial plus all three combat pages and CombatStart.
The potion case uses the real reward-claim action and its scoped UiCoreObserver;
the shuffle case executes DrawOne through draw_cards with an empty draw pile and
compares the entire resulting State against the same unobserved command. It
also checks that neither the visible piles nor the newly drawn hand advance
before the modal barrier. Treasure exercises real Open and Take commands,
including immediate keyboard and direct-action attempts, the999/1000ms real
clock boundary in Fast, acknowledgement, and unchanged legacy behavior.
`ftue_progress_regression` independently validates NSH1–8 persistence and the
three-profile migration boundary.

The first combined49291534 execution retained three failed FTUE receipts:
synthetic initialization, and both actual locales at the first CombatRules
input check. The synthetic retained-cost table was positioned incorrectly;
the actual fixture opened its modal before explicit combat asset preparation
finished. After those corrections, a third fixture correction used the real
RewardCards action to start its350ms entrance clock. No production preparation
or input gate was weakened. Preparation also asserts unchanged State and
animation, and every existing modal assertion remains.

The corrected fixture linked against the unchanged frozen49291534 runtime and
headers passed59,564 synthetic /5,987 English /5,284 Chinese checks. Receipts:
`ui8-stage8e-ftue-probe2-synthetic-375bcf7a2a9d`,
`ui8-stage8e-ftue-probe2-eng-5146b67d530c`, and
`ui8-stage8e-ftue-probe2-zhs-98dfe88853e1`. Both final production packs passed
all required-glyph and complete-paragraph bounds; each locale produced19
reviewed captures, including real shuffle and treasure commands. Private
captures are under `ui9-stage8e-captures/ftue/{eng,zhs}`. The three combat-page
image rectangles are also pixel-identical between locales. The reviewed
Power-arrow position is authored in its original scene; NPowerCardFtue only
raises the actual Power holder's Z-index and does not reposition that arrow.

Visual review also identified a real Relic-arrow difference: its original
scene has `flip_v=true`. The renderer now supplies that one missing transform
flag. The actual-pack fixture checks the final framebuffer against opaque
pixels from the original47×39 texture in reversed row order, including pixels
whose opposite row is transparent so the unflipped direction cannot pass.
It excludes only the popup's actual overlap, not the arrow's visible tip.

The revised fixtures and this one runtime flag still require their own remote
checkpoint and formal matching rerun. These diagnostic passes do not overwrite
the first combined suite's failed result or imply renderer-wide95% fidelity.
