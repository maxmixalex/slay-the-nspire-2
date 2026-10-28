# Silent completion audit — 0.15

## Scope and source

The locked reference is **v0.111.0 / 41cef1ea**. The privately supplied assembly was checked against SHA-256 `0861bfa1df347538d932f22d580e75420f08082792eb914e53b4882764acdbe9` before static inspection. This document records derived rules and method identities; the repository contains no original assembly, IL, localization dump or game assets.

`SilentCardPool.GenerateAllCards` (`0x06004826`) declares 91 models. Five models are restricted to multiplayer: BladeSymphony, Concoct, Fade, Flanking and Sneaky. The complete single-player scope is therefore **86 cards**. The existing 29 remain at their old IDs, and 57 new models occupy append-only IDs **170–226**. Shiv remains the existing token **50**; Silent requires no additional token ID.

`config/silent-cards-0.15.json` is the per-model evidence index. Its 87 rows cover the 86 single-player cards and Shiv, with base/upgraded costs, keywords, canonical numeric variables, generation restrictions and reference method tokens. The complete permanent-deck pool includes Ancient Suppress and WraithForm. Both are excluded from ordinary rarity 2–4 rewards and generation. Nightmare and TheHunt are also excluded from combat generation by their own model rules.

## Implemented behavior

`src/silent_core.inc` handles every Silent model, including the original 29 and Shiv. It uses the existing serialized effect queue, so card choices, nested automatic plays and turn-start choices can pause and survive save/load. `src/silent_definitions.inc` adds the 57 new definitions. The module state and codec append to save version 13, preserving the older field prefix and IDs.

The new behavior includes active-discard/Sly play, temporary and permanent Sly/Retain marks, power acquisition order, per-play captured trigger amounts, dynamic damage and ordered cost reductions, delayed Nightmare snapshots, enemy Artifact/Strangle state, and additional fatal-card reward groups. Negative Dexterity is supported for WraithForm.

### Rules that differ from older game versions or need explicit timing

- Reflex and Tactician are playable **3-cost Sly Skills**. Sly is triggered by active discards. Normal end-of-turn hand flushing does not trigger it. Discard-and-draw captures the whole batch, discards it, performs the requested draw, then automatically plays the captured Sly cards in selection order.
- CalculatedGamble exhausts in both variants; upgrading adds Retain. BladeDance also exhausts. Upgraded Mirage removes Exhaust. WellLaidPlans retains the entire hand rather than opening a selected-card retention prompt.
- BulletTime applies a whole-turn free cost to cards currently in hand. A later temporary explicit cost can mask that modifier until the physical play ends, after which the whole-turn free cost remains. X-cost cards retain their special cost and consume the current energy when played manually.
- Pinpoint reduces its own cost after each completed Skill, including Skills already completed when a newly generated instance enters combat. Its reductions expire at the end of the turn. UpMySleeve reduces its own combat cost after creating Shivs. Explicit cost setters preserve modifier order instead of subtracting all historical discounts from a newer override.
- Nightmare snapshots the selected card immediately, and creates three copies before the next ordinary hand draw. The template is outside all combat piles, so end-of-turn cleanup does not mutate its captured local cost modifiers. Copies preserve captured upgrade, enchantment, temporary keywords, and supported instance damage/cost fields. The original card can change afterward without changing the snapshot. Exhaust-on-next-play is cleared by cloning.
- FanOfKnives changes Shivs to hit all enemies. Inky applies one Weak to the target, or to every enemy for an all-target Shiv. PhantomBlades grants Retain to existing and future Shivs and increases only the first completed Shiv play of each turn; Accuracy remains additive. Dynamic targeting and damage previews follow these rules.
- Afterimage, SerpentForm and Strangle use the amount captured before each play. A newly installed power does not retroactively trigger itself. Replays capture again, so a second play can observe the amount installed by the first.
- After-card history is recorded first. Player power listeners then run in their shared acquisition order, including Panache, Calamity, Rage, Rupture, Afterimage, SerpentForm and MasterPlanner. Player relics follow, then card listeners such as Pinpoint, then each enemy's captured Strangle. A serialized listener continuation preserves this order through nested effects and suspended choices. The original late-hook pass has no override among this build's three included card pools.
- Accelerant repeats each Poison trigger, decrementing Poison after each surviving hit. Outbreak applies Poison and triggers it immediately. It is not a poison-spread-on-death power. Envenom requires unblocked attack damage; Artifact can prevent its Poison application.
- GrandFinale requires an empty draw pile for manual play. The original automatic-play route does not use that manual `IsPlayable` restriction. Automatic GrandFinale can therefore resolve with a nonempty draw pile.
- ShadowStep installs DoubleDamage next turn. DoubleDamage always multiplies attack damage by two; extra layers extend its duration and decrement one per player-side end. Vulnerable/Cruelty, Tracking and DoubleDamage are combined before integer rounding.
- Blur preserves Block at the block-clear point and decrements at the following player-side start, including when Barricade independently preserves Block. Shadowmeld multiplies each Block gain, including unpowered gains, once.
- ToolsOfTheTrade increases the normal hand draw and then requires a discard. This choice is valid with no active card and is serialized. NoxiousFumes and WraithForm use their after-side-start timing; InfiniteBlades and Nightmare act before ordinary hand draw.
- KnifeTrap captures the Shivs in Exhaust and automatically plays them with the selected target; its upgrade upgrades the captured cards. EchoingSlash schedules another all-enemy strike for each kill. Finisher, Flechettes and Skewer use bounded continuations instead of allocating an effect for every possible hit.
- TheHunt adds a distinct three-option card reward when its attack kills an enemy. Each added reward can be selected or skipped, and the pending group count survives the combat transition and save/load.

## Reference evidence for shared mechanisms

| Mechanism | Reference method identity | Implementation consequence |
| --- | --- | --- |
| Declared Silent pool | `0x06004826` | 91 declared, 86 single-player models |
| Discard-and-draw Sly batch | `CardCmd.DiscardAndDraw.MoveNext`, `0x0600b22f` | Discard full batch, then draw, then captured automatic plays |
| Manual GrandFinale condition | `GrandFinale.get_IsPlayable`, `0x06004007` | Empty draw pile required only for manual play |
| Automatic-play gate | `CardCmd.AutoPlay.MoveNext`, `0x0600b229`; `Hook.ShouldPlay`, `0x06004cf6` | Does not call the manual `IsPlayable` getter |
| Local modifier evaluation | `CardEnergyCost.GetWithModifiers`, `0x0600521d` | Ordered add/set modifiers |
| X energy payment | `CardEnergyCost.GetAmountToSpend`, `0x06005220` | X consumes current energy and bypasses ordinary free-cost hooks |
| Pinpoint entry / completion / discount | `0x06004304`, `0x06004305`, `0x06004306` | Current-turn history and `AddThisTurn` |
| Nightmare selection snapshot | `NightmarePower.SetSelectedCard`, `0x06002710` | Clone at selection time and clear affliction |
| Clone metadata | `CardModel.CreateClone`, `0x060018fb`; `DeepCloneFields`, `0x060018c0`; `CardEnergyCost.Clone`, `0x06005230` | Copy instance metadata and clear exhaust-on-next-play |
| End-of-turn card cleanup | `PlayerCombatState.EndOfTurnCleanup`, `0x060050d4`; `get_AllCards`, `0x060050c0` | Only cards in combat piles are cleaned |
| Captured before-play amounts | Afterimage `0x060023f1`, SerpentForm `0x06002843`, Strangle `0x060028f7` | No first-install self-trigger |
| After-card listener phases | `Hook.AfterCardPlayed.MoveNext`, `0x0600af13`; `CombatState.IterateHookListeners.MoveNext`, `0x0600b3f4` | Player power acquisition order, relics, cards, then enemy listeners |
| Poison repetitions | `PoisonPower.Trigger.MoveNext`, `0x06009fbc`; trigger count `0x06002798` | Accelerant repeats decreasing Poison |
| Phantom first-Shiv damage | `0x06002779` | First completed Shiv play in the turn |
| Inky values and play | `0x060039ed`, `0x0600a779` | One Weak, respecting Shiv target type |
| DoubleDamage multiplier / expiry | `0x0600254d`, `0x06009ede` | Fixed ×2, one duration layer removed per player-side end |
| Shadowmeld / Tracking | `0x0600284e`, `0x060029be` | ×2 per Shadowmeld layer; linear Tracking percentage |
| Blur expiry | `0x06009e6c` | Decrement on player-side start |
| Tools hand draw / choice | `0x060029b0`, `0x0600a0b7` | Extra draw followed by mandatory discard |
| TheHunt fatal reward | `0x0600ad3e` | Additional independent card-reward group |

## Verification status

**Verified source: `4df76b04e2a07e6be9a898793f03fd0ab8df4aae`.** The source was pushed and read back before verification. Native journal `native-final-c5b65fe7bbf4` completed with all 25 regression suites passing under AddressSanitizer and UndefinedBehaviorSanitizer.

| Verification | Result |
| --- | --- |
| Silent completion suite | **17,753 checks passed**, 174 base/upgraded variants, 86 single-player models plus Shiv |
| Complete native regression set | **25/25 suites passed**, including shared character, save, UI, potion and colorless regressions |
| Exact rounding reference | **864 combinations passed**, including high multipliers and saturation |
| ARM link | Device executable produced successfully after removing the unsupported 64-bit division dependency |

The preceding source checkpoint `433dc2c6920914d67e751417fdee7b93a441cf7f` had already passed 16,811 Silent checks. The final result above additionally covers shared post-play listener ordering and the device arithmetic replacement. Actual-asset UI results are maintained separately by the content/UI verification workflow.

`tests/silent_completion_regression.cpp` covers **174 base/upgraded variants** across all 86 single-player models and Shiv. Its canonical table is a factual reference snapshot rather than a copy of values read from the runtime Definition array. Each model is played and checked for an observable effect: damage, Block, Poison, drawing/discarding, creation, power amount, modifier, delayed snapshot or status. Save/load round trips are exercised at choices and completed states.

Focused cases cover Sly batch order and turn flushing, temporary/permanent Sly, Fan/Inky/Phantom interactions, captured-power replays, integer rounding, Poison repetitions, draw-only triggers, Tools choices with no playing card, negative Dexterity, DoubleDamage duration, ordered costs, Nightmare immutability and cloning, automatic GrandFinale, large hit counts, fatal reward groups, and malformed module state.

The verified source includes SerpentForm/Panache acquisition-order cases and 864 combinations checked against a host-side exact integer rounding reference. The device implementation performs no 64-bit division: it bounds the product, then uses a 32-step shift/subtract quotient so the ARM runtime does not require `__aeabi_ldivmod`.

The engine remains a bounded single-player implementation: two enemies, ten hand cards, 512 combat card instances, 16 simultaneous Nightmare templates, and 192 permanent deck cards. Multiplayer-only cards are outside this scope. These passing deterministic regressions establish the tested implementation contracts and save/continuation behavior. They do not establish complete original-game RNG, encounter, or arbitrary cross-model differential parity.
