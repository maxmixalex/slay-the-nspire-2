# Defect completion audit — 0.16

## Locked source and scope

The reference is **v0.111.0 / 41cef1ea**. The supplied assembly was read privately after verifying SHA-256 `0861bfa1df347538d932f22d580e75420f08082792eb914e53b4882764acdbe9`. Card, power, orb and character text was checked against the same supplied PCK. Only derived implementation facts and method identities are committed; original assemblies, IL, localization dumps and game images remain outside Git.

`DefectCardPool.GenerateAllCards` (`0x060047d8`) declares **91** models. EnergySurge, Hibernate, Ignition, ImitationLearning and OneForAll require multiplayer and are excluded. The remaining **86 single-player cards** have append-only IDs **228–313**, in original pool order. `StrikeDefect` and `DefendDefect` use C++ identifiers `DefectStrike` and `DefectDefend`; their original localization keys remain unchanged. BiasedCognition and Quadcast have Ancient rarity and stay available to the full pool and practice mode while ordinary rewards and combat generation exclude them.

`config/defect-cards-0.16.json` is the complete per-model evidence index. It records base/upgraded cost, keywords, numeric and calculated variables, rarity, targeting, generation restrictions and the assembly methods inspected. All 86 models have an executable branch in `src/defect_core.inc`; none relies on a registration-only fallback.

The canonical character starts at **75 HP**, **99 gold** and **three orb slots**, with four Strikes, four Defends, Zap, Dualcast and CrackedCore. The character methods are `0x06003a36`, `0x06003a37`, `0x06003a3c`, `0x06003a3d` and `0x06003a43`; CrackedCore's initial Lightning trigger is `0x06001d52`.

## Orb rules

The oldest orb is index zero and corresponds to the original rightmost orb. The newest orb is the last occupied index and corresponds to the leftmost orb. Dualcast, Quadcast, MultiCast and Loop use the oldest orb; ConsumingShadow evokes the newest. Filling an occupied capacity evokes and removes the oldest orb before appending the new one. Reducing capacity drops the newest excess orbs **without an evoke**.

| Orb | Natural passive | Evoke | Focus and stored value |
| --- | --- | --- | --- |
| Lightning | End of turn: 3 damage to a random living enemy | 8 damage to a random living enemy | Add Focus and floor at zero; unpowered damage |
| Frost | End of turn: 2 Block | 5 Block | Add Focus and floor at zero; unpowered Block |
| Dark | End of turn: increase stored charge by 6 | Damage the enemy with least current HP for stored charge | Starts at 6; Focus modifies each increase, not the final stored-charge evoke |
| Plasma | Start of turn: 1 energy | 2 energy | Ignores Focus |
| Glass | End of turn: 4 damage to every enemy, with a decaying local base | Twice the current passive amount to every enemy | Add Focus before multiplying evoke by two; each positive passive reduces the local base by one, floored at zero |

Glass's passive does not decay its local base when Focus reduces the result to zero. Dark target ties use the first living enemy. A repeated evoke removes the orb only for the final repeat; Thunder runs after each Lightning evoke and damages the same surviving target. Natural Plasma triggering inspects the queue after start-of-turn powers have finished channeling and evoking, so a removed Plasma orb cannot trigger at a stale index.

`DOrb::Dark` is zero so older saves with only `expansion.dark[]` map naturally to the complete queue. `expansion.dark[]` stores Dark charge or Glass's local base, and zero for the other orb types. `defect.orb_kind[]` supplies the type. Non-Defect characters with zero base/current slots gain one slot when a positive channel command runs; a Defect whose slots were reduced to zero does not regain a slot automatically.

## Cards and continuation behavior

The implementation uses the existing bounded, serialized effect queue. Multi-hit attacks, channel loops, repeated evokes and random attacks resume through explicit operations. Hologram and Scavenge reuse existing choice machinery. Scrape additionally stores its explicit draw results per automatic-play frame, so an automatic Headbutt choice can suspend a Scrape without losing its captured cards.

### Instance and resource rules

- AdaptiveStrike creates a full combat clone in Discard and sets the clone's combat cost to zero. The clone has no permanent deck binding.
- AllForOne returns eligible zero-cost Attack, Skill and Power instances from the actual Discard order, up to hand capacity. X-cost and Status cards are excluded even if another display path reports zero.
- Claw increases every existing combat Claw, including exhausted instances and itself. The base card adds two damage; the upgraded card adds three. Later newly created Claws start with their own base value. A full clone preserves its captured local value.
- GeneticAlgorithm blocks for its current local value, then adds three/four to that instance and its associated permanent deck card. Per-deck values and combat-instance deck bindings survive saves and subsequent battles. Creating a temporary clone clears the deck binding. Removing, transforming or copying a permanent deck card has an explicit metadata hook.
- Modded adds one slot, draws one/two, then adds one to its own combat cost. MomentumStrike sets its own combat cost to zero. RocketPunch subtracts one until its next physical play for each Status generated in combat. Explicit cost setters retain the relative order of prior and later additive modifiers.
- HelixDrill uses actual energy-spent history for the current turn, subtracting its current modified cost while the card is in PlayPile. FTL uses the count of finished card plays. EchoForm uses the count of physical series started in the current turn and gives each of the first N series one extra play.
- SignalBoost adds one repeat to the next Power series per charge. Synthesis installs a late free-Power cost only in Hand/PlayPile. Feral changes the final result location of the first N zero-energy, non-duplicate Attacks to Hand, including an Attack which normally Exhausts; it also consults zero-energy attacks played before the power was acquired.

### Status creation, exhaust and draw rules

- BoostAway creates Dazed, FightThrough creates two Wounds, GunkUp creates Slimed, Overclock creates Burn and Turbo creates Void in Discard. Each created Status produces one generation event after it enters its pile. Smokestack and TrashToTreasure run in power acquisition order, followed by RocketPunch's card listener.
- Compact changes current hand Status instances into Fuel/Fuel+, clears the old instance modifiers, and preserves hand positions. It does not announce a newly generated Status.
- FlakCannon captures all non-exhausted Status cards in the original `AllPiles` order: Hand, Draw, Discard, then PlayPile. It exhausts each captured instance with normal exhaust hooks before performing one random attack per captured Status. Its snapshot includes a suspended Slimed in PlayPile if Slimed drew a Hellraiser-triggered automatic FlakCannon. End-of-play cleanup must respect an instance already moved to Exhaust.
- Iteration triggers on the first Status drawn in a turn, including whether such a draw already happened before Iteration was acquired. Scrape discards only its explicit draw results with a nonzero cost or X cost; nested Iteration draws do not become part of that captured batch. It moves the whole discard batch before Sly callbacks run.
- Reboot moves Hand directly into Draw, combines and shuffles the draw/discard cards, and draws four/six. It does not activate active-discard listeners. The current Reboot, exhausted cards and installed Power cards remain outside the shuffle.
- Uproar makes its two attacks, then automatically plays an eligible random Attack from Draw. Voltaic captures the number of Lightning orbs previously channeled in the combat before beginning its own channel loop.

The shared status/token definitions are maintained centrally. In this pinned build, **Slimed costs one and draws one before Exhausting**, Fuel gives one/two energy for zero cost and Exhausts, Burn deals two blockable unpowered damage at the end of a turn in Hand, Void loses one energy when drawn, Dazed/Void are Ethereal and Unplayable, and Wound/Burn are Unplayable. Status cards have no upgrade; Compact's upgrade creates Fuel+.

## Power timing

| Boundary | Defect work |
| --- | --- |
| Physical play starts | Capture paid energy and EchoForm/SignalBoost repeat count; advance resource history once |
| Before each repeated play | Capture existing Storm/Subroutine amounts and consume applicable free-Power charge |
| After each repeated play | Run captured Storm/Subroutine in the shared acquired-power listener order; update finished history |
| Final physical result location | Apply Feral once, clear RocketPunch's until-played reduction, clear the frame's captures |
| After energy reset | LightningRod channels one and decrements its duration; Spinner channels its amount; natural Plasma phase follows |
| Before ordinary hand draw | CreativeAi generates its random Power cards; MachineLearning increases ordinary draw count |
| After player turn starts | Loop repeats the oldest orb's passive |
| After player side starts | BiasedCognition loses Focus, Coolant gains Block per distinct orb type, Feral resets its remaining-turn allowance |
| Before player side ends | Hailstorm damages all enemies if any Frost remains |
| Natural orb end phase | Lightning, Frost, Dark and Glass passives run in queue order |
| After player side ends | ConsumingShadow evokes the newest orb; each temporary Focus source expires in its own acquisition order |

FocusedStrike, Hotfix, Hyperbeam and Synchronize use four separate temporary-Focus power entries. Their aggregate is exposed for presentation, but expiry preserves acquisition order relative to ConsumingShadow. For example, a Glass orb evoked by ConsumingShadow can observe or miss temporary Focus depending on which power was installed first. Negative Focus is supported for BiasedCognition and Hyperbeam.

Buffer uses the existing shared Buffer mechanism. BulkUp applies slot loss and ordinary Strength/Dexterity, Capacitor adds slots, Defragment adds permanent combat Focus, ChargeBattery and Scavenge use delayed energy, and the other attacks/Block/draw cards use the shared damage and resource pipeline. Orb damage and Frost/Coolant Block remain unpowered while still notifying applicable shared Block/damage listeners.

## Integration and serialized facts

`include/defect_state.inc` contains 24 ordered powers, typed orb metadata, turn/channel/resource counters, per-instance values and ordered costs, permanent GeneticAlgorithm metadata, and per-automatic-frame capture state. `src/defect_codec.inc` appends these to save version 14 with the established four-byte field encoding. `src/defect_validation.inc` rejects invalid orb kinds/values, missing or duplicate power-order entries, malformed temporary-Focus totals, wrong-kind instance metadata, invalid deck bindings and illegal queued operands.

Shared engine hooks are declared in `src/defect_forward.inc`. The four permanent-deck hooks are `defect_deck_card_created`, `defect_deck_removed`, `defect_deck_replaced` and `defect_deck_copied`. Combat creation/reset/copy hooks remain separate from `notify_card_generated`: building the opening deck or transforming a card must not accidentally announce player-created Status cards. Nightmare snapshots must copy `value`, `cost_delta`, `turn_origin` and `combat_origin`, then clear a clone's `deck_origin`.

### Mechanism evidence index

| Mechanism | Assembly methods |
| --- | --- |
| Five random orb models | `OrbModel..cctor`, `0x06001b0f` |
| Orb capacity loss / zero-capacity channel | `OrbQueue.RemoveCapacity`, `0x060050dd`; `OrbCmd.Channel.MoveNext`, `0x0600b327` |
| Dark / Frost / Glass / Lightning / Plasma rules | `0x06002c73–0x06002c9b` |
| Focus permits negative values | `FocusPower.get_AllowNegative`, `0x060025bd`; orb modifier `0x060025be` |
| EchoForm series count | `0x06002564`, `0x06002569` |
| Feral history and result location | `0x0600259b–0x0600259f`, `0x060025a1` |
| Synthesis free-Power cost / decrement | `FreePowerPower`, `0x060025d5`, `0x06009f01` |
| Storm / Subroutine capture | `0x060028f0–0x060028f1`, `0x06002907–0x06002908` |
| First Status draw | `IterationPower`, `0x060026a1`, `0x060026a3` |
| Status generation listeners | Smokestack `0x060028b0`; TrashToTreasure `0x060029c2` |
| Lightning evoke follow-up | `ThunderPower.AfterOrbEvoked`, `0x060029ac` |
| Temporary Focus expiry | `TemporaryFocusPower.AfterSideTurnEnd`, `0x0600296f`, state machine `0x0600a098` |
| Newest-orb end trigger | `ConsumingShadowPower.AfterSideTurnEnd`, `0x060024ab` |
| Full combat pile ordering | `PlayerCombatState.get_AllPiles`, `0x060050bf`; `get_AllCards`, `0x060050c0` |
| Flak snapshot/filter/exhaust | `0x06003f4f`, `0x0600a99f`, `0x0600a9a0` |
| HelixDrill spent-energy calculation | `0x0600aa28–0x0600aa2b` |

## Verification status

**Source implementation and regression cases are prepared; checkpoint integration and execution are pending.** No passing build, test count, resource conversion, original-runtime comparison or hardware result is claimed by this document at this point. The source must be pushed and read back before the gated validation runs.

`tests/defect_completion_regression.cpp` contains an independent literal canonical table and **172 base/upgraded play scenarios**. Every model has a numeric or state-changing assertion. Focused scenarios additionally cover all five passive/evoke formulas, negative Focus, capacity overflow/removal, zero capacity, Dark targeting, Thunder, Shatter, Voltaic history, start/end power timing, temporary Focus ordering, EchoForm/Feral/SignalBoost, instance costs and Claw growth, permanent GeneticAlgorithm growth, immutable Nightmare copies, Status creation/transform/exhaust, nested Scrape choices, suspended Slimed, character starting facts, legacy orb potions and malformed serialized state. Save/load round trips are exercised at ordinary and suspended choice states.

The implementation keeps the project's explicit bounds: two enemies, ten hand cards, ten orb slots, 512 combat card instances, 192 permanent deck cards and twelve nested automatic frames. Multiplayer-only models remain outside scope. Deterministic regression results will establish these bounded implementation contracts, not the original game's seed sequence, full encounter progression or arbitrary differential parity.
