# Single-player colorless audit — 0.14

## Evidence and scope

This audit uses the supplied **v0.111.0 / 41cef1ea** assembly, SHA-256
`0861bfa1df347538d932f22d580e75420f08082792eb914e53b4882764acdbe9`.
Constructors, canonical variables, upgrade methods, play methods and the relevant
power/command continuations were inspected statically. The method tokens below
identify the inspected original implementation without distributing its code.
This is an independent bounded implementation; the original game was not executed
for a frame-by-frame or seed-by-seed comparison.

All **53 single-player colorless models**, each in base and upgraded form, have an
explicit effect scenario in `tests/colorless_completion_regression.cpp`. Soul and
Sovereign Blade have separate generated-token scenarios. Multiplayer-only cards,
Timeline unlock progression, audio, the other three playable character systems,
and their full card pools remain outside the implemented scope. Generation rules
are applied to the represented character pools; their output distribution cannot
equal the complete original pools until those catalogs are implemented.

The table describes unmodified values. Strength, Vigor, Vulnerable, Dexterity,
No Block, relevant relics and replay effects are applied by the common engine.
`A / B` means base / upgraded. An unchanged cost is written once. The token is the
original card's asynchronous `OnPlay` continuation (`MoveNext`). Exhaust and
Retain are stated where they affect the result pile or end-of-turn behavior.

## Complete card matrix

| Model | Cost | Base / upgraded behavior | Play token |
| --- | ---: | --- | --- |
| Alchemize | 1 / 0 | Obtain one random combat-eligible potion from the owner's pool. Exhaust. | `0x0600a796` |
| Anointed | 1 | Move random Rare cards from Draw to available hand slots; no draw trigger. Exhaust; upgrade adds Retain. | `0x0600a7a1` |
| Automation | 1 / 0 | Each 10 actual card draws grants 1 Energy; counters persist between turns and are independent per application. | `0x0600a7b7` |
| Beat Down | 3 | Capture and automatically play up to 3 / 4 distinct Attack instances from Discard. | `0x0600a7d5` |
| Bolas | 0 | Deal 3 / 4 damage; return this instance before next turn's hand draw. | `0x0600a80c` |
| Calamity | 3 / 2 | Each completed Attack play creates a random Attack from the owner's ordinary combat pool. | `0x0600a844` |
| Catastrophe | 2 | Automatically play a random card from Draw 2 / 3 times, selecting again after each play. | `0x0600a857` |
| Dark Shackles | 0 | Target loses 9 / 15 Strength until the enemy side ends. Exhaust. | `0x0600a8c7` |
| Discovery | 1 | Choose one of three distinct cards from the owner's ordinary combat pool; add it with temporary zero cost. Base Exhausts; upgrade does not. | `0x0600a908` |
| Dramatic Entrance | 0 | Deal 11 / 15 damage to every enemy. Innate, Exhaust. | `0x0600a925` |
| Entropy | 1 | After each normal hand draw, choose one hand card to transform for combat. Upgrade adds Innate. | `0x0600a94d` |
| Equilibrium | 2 | Gain 13 / 16 Block and retain the hand through this turn's discard. | `0x0600a951` |
| Eternal Armor | 3 | Gain 9 / 12 Plating: end-of-turn Block, then decrement at the next player turn start. | `0x0600a957` |
| Fasten | 1 | Cards with the Defend tag gain 4 / 6 additional powered Block. | `0x0600a971` |
| Finesse | 0 | Gain 4 / 7 Block, then draw one card. | `0x0600a98c` |
| Fisticuffs | 1 | Deal 7 / 9 damage, then gain powered Block equal to that hit including blocked and overkill damage. | `0x0600a99a` |
| Flash of Steel | 0 | Deal 5 / 8 damage, then draw one card. | `0x0600a9a6` |
| Gold Axe | 1 | Deal damage equal to completed card plays in this combat before the current play. Upgrade adds Retain. | `0x0600a9f3` |
| Hand of Greed | 2 | Deal 20 / 25 damage; a qualifying kill grants 20 / 25 Gold. | `0x0600aa0c` |
| Hidden Gem | 1 | Enchant a random eligible Draw instance to replay 2 / 3 extra times each time it is played this combat. | `0x0600aa40` |
| Impatience | 0 | If no Attacks remain in hand, draw 2 / 3 cards. | `0x0600aa68` |
| Jack of All Trades | 0 | Create 1 / 2 distinct eligible colorless models, excluding itself. Exhaust. | `0x0600aa86` |
| Jackpot | 3 | Deal 25 / 30 damage; create three owner-pool cards whose canonical cost is zero. Upgrade also upgrades the generated cards. | `0x0600aa8b` |
| Master of Strategy | 0 | Draw 3 / 4 cards. Exhaust. | `0x0600aada` |
| Mayhem | 2 / 1 | After normal draw and turn-start effects, reserve and automatically play the top card; stacked amount reserves a complete batch first. | `0x0600aae3` |
| Mind Blast | 1 / 0 | Deal damage equal to the current number of cards in Draw. Innate. | `0x0600ab01` |
| Nostalgia | 1 / 0 | The first Attack or Skill started each turn returns to the top of Draw if its ordinary result would be Discard. Stacking increases the number. | `0x0600ab45` |
| Omnislice | 0 | Deal 8 / 11 damage to the target; copy that full hit including overkill as unpowered damage to every other enemy. | `0x0600ab54` |
| Panache | 0 | Every fifth subsequent completed card play in a turn deals 10 / 14 unpowered damage to all enemies. Each application has its own counter. | `0x0600ab6d` |
| Panic Button | 0 | Gain 30 / 40 Block, then 2 turns of No Block from powered cards. Exhaust. | `0x0600ab6f` |
| Prep Time | 1 | Gain 4 / 6 Vigor at each player turn start; unspent Vigor stacks. | `0x0600aba5` |
| Production | 0 | Gain 2 / 3 Energy. Exhaust. | `0x0600abac` |
| Prolong | 0 | Snapshot current Block and grant that amount next turn. Base Exhausts; upgrade does not. | `0x0600abae` |
| Prowess | 1 | Gain 1 / 2 Strength and 1 / 2 Dexterity. | `0x0600abb7` |
| Purity | 0 | Choose and Exhaust up to 3 / 5 hand cards, including choosing none. Retain, Exhaust. | `0x0600abc2` |
| Rend | 1 | Deal 10 / 12 damage plus 5 / 8 for each eligible Debuff type on the target; stack amounts do not multiply the type count. | `0x0600ac03` |
| Restlessness | 0 | If no other cards remain in hand, draw 2 / 3 cards then gain 2 / 3 Energy. Retain. | `0x0600ac07` |
| Rolling Boulder | 3 | At each player turn start deal 5 / 10 unpowered damage to all enemies, increasing this application's future damage by 5. | `0x0600ac16` |
| Salvo | 1 | Deal 12 / 16 damage and retain the hand through this turn's discard. | `0x0600ac23` |
| Scrawl | 1 | Draw until the hand is full. Exhaust; upgrade adds Retain, with no cost reduction. | `0x0600ac2e` |
| Secret Technique | 0 | Choose a Skill from Draw and move it to Hand. Base Exhausts; upgrade does not. | `0x0600ac3f` |
| Secret Weapon | 0 | Choose an Attack from Draw and move it to Hand. Base Exhausts; upgrade does not. | `0x0600ac44` |
| Seeker Strike | 1 | Deal 9 / 12 damage, then choose one of up to three sampled Draw instances and move it to Hand. | `0x0600ac48` |
| Shockwave | 2 | Apply 3 / 5 Weak and Vulnerable to all enemies. Exhaust. This reference places it in the colorless pool. | `0x0600ac6c` |
| Splash | 1 | Choose among three distinct Attacks from other characters' ordinary combat pools; add the choice with temporary zero cost. Upgrade upgrades the offer. | `0x0600acbb` |
| Stratagem | 1 / 0 | After each shuffle, choose one card from Draw and move it to Hand before the interrupted operation continues; stacking increases the selection. | `0x0600ace4` |
| The Bomb | 2 | After three player turn ends, deal 40 / 50 unpowered damage to every enemy. Each bomb has its own countdown. | `0x0600ad35` |
| The Gambit | 0 | Gain 50 / 75 Block; an incoming powered Attack with unblocked damage triggers the applied death effect. | `0x0600ad37` |
| Thinking Ahead | 0 | Draw two cards, then choose one hand card to put on top of Draw. Base Exhausts; upgrade does not. | `0x0600ad46` |
| Thrumming Hatchet | 1 | Deal 11 / 14 damage; return this instance before next turn's hand draw. | `0x0600ad4f` |
| Ultimate Defend | 1 | Gain 11 / 15 Block; carries the Defend tag. | `0x0600ad72` |
| Ultimate Strike | 1 | Deal 14 / 20 damage; carries the Strike tag. | `0x0600ad74` |
| Volley | X | Deal 10 / 14 damage to a random living enemy X times; choose a target for each hit. X=0 still completes a card play. | `0x0600ad99` |

## Shared rules and repairs

### Generation, movement and costs

`CardFactory.FilterForCombat` (`0x06004f0e`, predicate `0x0600b002`) excludes Basic,
Ancient and Event rarities and models whose `CanBeGeneratedInCombat` is false.
Alchemize (`0x06003b18`), Hand of Greed (`0x06004038`) and Hidden Gem
(`0x0600408a`) explicitly opt out. Generated tokens also stay out of ordinary
combat offers. Discovery, Calamity and Jackpot use the owner's character pool;
Splash uses foreign character pools. Jack of All Trades excludes itself and
samples a distinct batch, while Jackpot independently generates each of its three
cards and may repeat a model. Cosmic Concoction shares the distinct-batch helper.

Entropy uses the source card's pool, or the colorless pool for a token, and excludes
the original model. `GetDefaultTransformationOptions` (`0x06004f0f`),
`GetFilteredTransformationOptions` (`0x06004f12`) and
`CreateRandomCardForTransform` (`0x06004f10` / `0x06004f11`) create a fresh model;
the old upgrade, transient cost, replay enchantment and return marker do not carry
over. These rules are tested for both represented character colors and tokens.

General pile selections remain available with a full hand. `CardPileCmd.Add`
(`0x0600b258`) redirects an attempted Hand destination to Discard at the hand cap;
`FromCombatPile` (`0x0600b2b8`) does not suppress the choice. This applies to Secret
Weapon/Technique, Seeker Strike, Stratagem, Droplet and Liquid Memories. Anointed
has its own explicit hand-capacity limit (`0x0600a7a1`) and leaves excess rares in
Draw. Direct moves and generated cards do not count as draws for Automation or
other draw-triggered effects.

`SetToFreeThisTurn` (`0x060018c5`) and Snecko's temporary cost assignment use expiry
on the side's turn end **or completion of the first physical play**. Player-side
cleanup follows hand flush, before enemy actions (`0x0600b3c0`, `0x0600b3c4`,
`0x060050d4`); retained cards and cards outside Hand are also cleaned. Replays finish before
cleanup (`CardModel.OnPlayWrapper`, `0x06009b06`). Canonical X/negative costs are
returned before these overrides (`CardEnergyCost.GetWithModifiers`,
`0x0600521d`). Snecko skips X and unplayable costs; Touch of Insanity considers only
eligible positive-cost cards (`0x0600a186`, `0x060018db`) and clears an older turn
override when applying combat-long free cost.

### Automatic play, replays and result piles

Beat Down captures its distinct discard instances before any selected attack
plays (`0x0600a7d5`); a returned card cannot be selected twice by the same Beat
Down. Catastrophe deliberately chooses afresh after each play (`0x0600a857`).
Hidden Gem excludes already replay-enchanted instances (`0x0600aa40`). Replays use
one physical instance and one energy payment, each count as a started/completed
card play, and the result pile is handled after the entire group.

`AutoPlayFromDrawPile` (`0x0600b26e`) first reserves the requested batch in Limbo.
The first card's draw cannot steal another card that was reserved for that batch.
A needed shuffle can suspend reservation at Stratagem's choice; all reserved
instance ownership and the remaining count are serialized. Capacity is checked
before moving instances. Tests reject duplicated queue references and orphaned
Limbo cards, and check save/resume at this precise boundary. Mayhem and Distilled
Chaos use this shared batch implementation.

Nostalgia's result-pile decision is made before generating the replay count and
before the replay loop (`0x06009b06`, `ModifyCardPlayResultLocation` before
`GeneratePlayCount` and `History.CardPlayStarted`). Its power (`0x06002725`,
predicate `0x06002728`) counts earlier Attack/Skill starts this turn and only
changes an ordinary Discard destination. Consequently, the return decision is
fixed for a physical group while every replay advances the count. Exhausting
cards still Exhaust. Bolas and Thrumming Hatchet's return hooks (`0x0600a80a`,
`0x0600ad4d`) also recover their instance from Exhaust if it was played on the
previous turn; an already-in-Hand instance is left there.

### Turn timing and damage

Normal hand draw and `AfterPlayerTurnStart` effects precede Mayhem's auto-pre-play
phase: `SetupPlayerTurn` (`0x0600b3d0`), `RunAutoPrePlayPhase` (`0x0600b3cc`) and
Mayhem's hook (`0x06009f72`). The suite exercises Entropy's saved choice before
Mayhem begins.

Power application order also matters within that phase. `Creature.ApplyPowerInternal`
(`0x060051e9`) appends new power instances. `CombatState.IterateHookListeners`
(`0x0600b3f4`) visits the creature's powers in that order, then its relics;
`Hook.IterateCombatHookListeners` (`0x0600af9d`) preserves that ordering and
`AfterPlayerTurnStart` (`0x0600af55`) awaits each listener. Entropy stacks in its
original place, while each Rolling Boulder remains an independent instance.
The one-byte `entropy_boulder_prefix` records how many Boulders existed when
Entropy was first applied. Each Boulder damages and then increases its own amount;
an Entropy choice can suspend between earlier and later Boulders. Tests exercise
both application orders, duplicated Boulders, later Entropy stacks, and save/resume
at the choice. This ordering state is written explicitly in save v12. Importing
v11 sets the prefix to all existing Boulders, preserving the former engine's
behavior; the unrecorded original application order cannot be recovered.

Mercury Hourglass's `AfterPlayerTurnStart` (`0x06001fc8`) follows those power
listeners. Blood Vial uses `AfterPlayerTurnStartLate` (`0x06001c97`), after Mercury.
The represented early effects remain before hand draw: Horn Cleat / Captain's
Wheel use `AfterBlockCleared` (`0x06001ecc` / `0x06001d17`), Art of War uses
`AfterEnergyReset` (`0x06001c38`), and Happy Flower / Akabeko / Lantern use
`AfterSideTurnStart` (`0x06001eb6` / `0x06001c07` / `0x06001f3a`). Prep Time, Demon
Form and Noxious Fumes also use `AfterSideTurnStart`; Infinite Blades uses
`BeforeHandDraw` (`0x0600268d`). No global power-order system is needed for this
bounded set of turn-start effects.

Plating gives unpowered Block at the player turn's end (`0x06009fb2`) and decreases
at the next player turn start (`0x06009fb0`); it does not grant a second start-of-
turn Block amount. Prolong snapshots current Block into next-turn Block
(`0x0600abae`), so subsequent Block changes do not change that snapshot. No Block
decrements at enemy-side end (`0x06009f84`); skipped enemy turns do not decrement it.

Fisticuffs (`0x0600a999`) and Omnislice (`0x0600ab54`) use `TotalDamage` plus
`OverkillDamage`. The engine now preserves the whole hit for these effects.
Fisticuffs applies the resulting powered Block modifiers; Omnislice's secondary
hits are unpowered and still encounter each other enemy's Block. Tests cover
blocked damage and overkill instead of assuming damage equals HP removed.

The Gambit observes actual unblocked HP loss, not the pre-prevention attack amount:
`Creature.LoseHpInternal` (`0x060051d6`) constructs `DamageResult.UnblockedDamage`
from the HP difference. Osty redirection and Buffer's
`ModifyHpLostAfterOstyLate` (`0x06002451`) run before that result. If the player
loses no HP after those protections, the Gambit death effect does not trigger.

Rend's original predicate (`0x060043de`) counts Debuff powers excluding temporary
helper powers. The represented enemy Debuffs are negative Strength, Weak,
Vulnerable, Poison, Beetle, Demise and Doom. No extra count is added for Dark
Shackles' temporary restoration helper. Full interactions with unimplemented
character/enemy powers are outside this catalog.

## Generated tokens

Soul (`0x0600ac96`) costs zero, draws 2 / 3 and Exhausts. Sovereign Blade
(`0x0600aca8`) costs 2 / 1, starts at 10 damage and Retains; Forge strengthens its
combat instance or creates one if none is eligible. Their base/upgraded behavior,
replay and save state are exercised separately from the 106 colorless variants.
Regent-specific Seeking Edge, Parry and related powers are not implemented by
this two-character engine and are not claimed as covered.

## Validation record

The explicit 106-variant suite passed with AddressSanitizer and
UndefinedBehaviorSanitizer at checkpoint
`8adeaeebe4a0380b92ceceff154e30ab5069696c`, together with generation/transform pool,
choice save/resume, modifier and return-from-Exhaust scenarios present there.
That whole-project run passed 20 of 21 suites; the older combined suite still had
obsolete Fisticuffs, Omnislice and Plating expectations. The next source stage
replaces those with full original-rule numbers and adds batch Limbo reservation,
full-hand selection, Touch eligibility and Doom/Demise ordering coverage. The
following stage adds explicit Entropy/Boulder ordering, separates early and late
turn-start relic hooks, and checks cost expiry before an enemy-phase choice.

The newly added scenarios are **awaiting a verified source checkpoint and test
run** at the time this audit was written. Source tests are not passing evidence.
The integration owner records the later checkpoint results in `docs/WORK_STATE.md`.
Native/WASM agreement, ARM compilation and original-game parity are distinct
claims; this audit establishes static rule evidence and bounded engine coverage.
