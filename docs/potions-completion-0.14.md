# Potion behavior audit — 2026-10-18

Reference remains **v0.111.0 / 41cef1ea**. This audit starts from restored source commit `50a58c9b876c3038e549b97e7e7d245224d717d8`. It separates executable behavior, deterministic regression coverage, and original-game parity. The original assembly was recovered again and independently verified as SHA-256 `0861bfa1df347538d932f22d580e75420f08082792eb914e53b4882764acdbe9`. Public method-token references and behavioral conclusions are recorded in `config/potion-audit-0.14.json`; the assembly and private disassembly are not repository content.

At pushed checkpoint `8adeaeebe4a0380b92ceceff154e30ab5069696c`, `potions_completion_regression` passed **33,831 checks across 64 model policies** with AddressSanitizer and UndefinedBehaviorSanitizer. This includes the rarity/context fixes, temporary-cost lifetime, X-cost handling, Cosmic Concoction's distinct generation, Plating timing, Fairy event revival and source-backed stacking/pile assertions. LeakSanitizer was disabled because the execution environment restricts the required process/thread inspection. These are deterministic engine results, not an original-game differential result. The earlier `b0945483e4f50ca065c0b7c3a0d9f917a2b375e6` run passed 7,573 checks in this suite; it is superseded by the larger run.

The following source stage fixes full-hand pile choices, Touch of Insanity's eligibility filter, Distilled Chaos's reserved batch and Doom/Demise ordering. Its new assertions await execution after the next verified source checkpoint. The successful `8adeaeeb` run does not certify these later changes or the old expectations that further original-method inspection corrected.

## Inventory and acquisition

There are 64 active registered potion models, excluding DeprecatedPotion. Every active ID reaches either the direct dispatcher in `potions.cpp`, the continuation dispatcher in `potions_extra.cpp`, or the automatic revival hook. Merely having a dispatch case is not sufficient evidence of a complete original-game implementation.

Freshly inspected rarity, usage and single-player enemy-target scopes match the descriptors for all 64 active models. The per-model method tokens and comparison values are included in the audit JSON. This metadata check is separate from effect and interaction validation.

| Current acquisition group | Count | Models / behavior |
| --- | ---: | --- |
| General pool | 45 | Shared models accepted by both currently playable characters. |
| Ironclad pool | 3 | Blood Potion, Soldier's Stew, Ashwater. |
| Silent pool | 3 | Poison Potion, Ghost in a Jar, Cunning Potion. |
| Other character models | 9 | Bone Brew, Pot of Ghouls, Potion of Doom; Focus Potion, Essence of Darkness, Potion of Capacity; Star Potion, Cosmic Concoction, King's Courage. Explicit practice acquisition is available. Necrobinder, Defect and Regent are not playable characters yet. |
| Special acquisition models | 4 | Ambergris, Foul Potion, Glowwater Potion and Potion-Shaped Rock. Explicit practice acquisition is available; their original event/acquisition routes are not implemented. |

Each currently playable character consequently has a 48-model ordinary potion pool. Shop shelves, expedition rewards, Alchemize and Entropic Brew use the shared pool helper with an explicit generation context. Alchemize excludes Fairy in a Bottle, Fruit Juice and Regen Potion, leaving 45 eligible models. Entropic Brew explicitly uses the original factory's out-of-combat policy even when consumed during combat, so all 48 models remain eligible. Explicit `procure_potion` and practice acquisition accept every active model, allowing deliberate acquisition without putting off-character or special bottles into ordinary drops.

The expedition retains two potion slots. A full inventory rejects purchases and reward claims without charging gold, marking the shelf sold, marking the reward claimed, or overwriting a bottle. Discarding one slot allows the same transaction to proceed. Entropic Brew consumes its own bottle before filling empty slots, and preserves an occupied second slot.

The random helper now chooses rarity first: 65% common, 25% uncommon and 10% rare, then a model within that rarity and the permitted pool. This replaces the earlier uniform model selection. The probabilities and combat exclusions come from the pinned factory and potion methods. Entropic Brew calls the factory separately for each empty slot with no exclusions; duplicate models and another Entropic Brew are permitted. Deterministic regressions cover those exceptions with concrete seeds and broad rarity-distribution bounds. The expedition retains its own integer RNG and does not reproduce the original floating-point random sequence, unlock progression, shop generator or reward probability system.

## Behavior coverage

`colorless_potions_regression` supplies initial numerical assertions for all 64 active models. `potions_completion_regression` supplements that enumeration with the state transitions and interactions below. The `8adeaeeb` result covers the then-frozen suite; full-hand choices, Touch eligibility, reserved Distilled Chaos batches and Doom/Demise ordering are newer assertions awaiting their first checkpoint run.

| Models | Additional contract exercised |
| --- | --- |
| Fire Potion, Explosive Ampoule, Potion-Shaped Rock, Foul Potion | Damage respects block; ordinary attack Strength, Vigor and Vulnerable do not amplify potion damage. Foul Potion affects the player as well as enemies, resolves revival before victory, and has its separate merchant use. |
| Block Potion, Fortifier | Flat potion block and tripling do not pick up Dexterity or card-only No Block. Finite block bounds do not overflow. |
| Blood Potion, Fruit Juice, Ambergris | Integer healing, maximum-HP growth, bounds, out-of-combat availability, and Ambergris's one extra player turn. |
| Energy Potion, Strength Potion, Dexterity Potion, Flex Potion, Speed Potion, Fysh Oil | Actual resource gains and caps; temporary gains remove only the amount successfully granted. |
| Swift Potion, Cure-All | No Draw suppresses drawing while retaining an independent energy gain. |
| Blessing of the Forge | Only combat hand instances upgrade; an already upgraded instance and the permanent deck remain coherent. |
| Liquid Bronze | Retaliation occurs on a blocked attack and is not mistaken for playing a card. |
| Vulnerable Potion, Weak Potion, Potion of Binding | Target rejection, living-enemy scope, stack bounds and no mutation of a dead target. |
| Attack Potion, Skill Potion, Power Potion, Colorless Potion | Three distinct offers, appropriate type/color, mandatory selection, invalid-selection atomicity, free selected instance and suspended save/resume. |
| Droplet of Precognition, Liquid Memories | Selection moves the requested card instance, retaining its upgrade and the other pile entries. Liquid Memories grants the temporary free cost. A full hand still permits selection; the selected instance goes to discard, where Liquid Memories's temporary free cost remains effective. |
| Gambler's Brew | Exact selected discard/redraw count, no-draw interaction, and persistence while choosing. |
| Ashwater, Glowwater Potion | Exhaustion snapshots the original hand/selection. Cards newly drawn by Dark Embrace are not accidentally included; ordinary exhaustion hooks still run. |
| Touch of Insanity | Only cards with a positive payable cost are eligible. The chosen card becomes free immediately even if an earlier same-turn effect gave it a nonzero temporary cost; it remains free on subsequent turns. |
| Cunning Potion, Pot of Ghouls | Token identity, upgraded Shivs, generation despite No Draw and full-hand overflow into discard. |
| Orobic Acid, Cosmic Concoction | Attack/skill/power generation from the current character's eligible pool and temporary free cost, or three distinct upgraded colorless instances, respectively. These checks do not certify an unimplemented character's complete generation pool. |
| Clarity, Radiant Tincture | Immediate benefit, the next three turn-start benefits and expiry on the fourth turn. Source inspection confirms a fixed one-card/one-energy benefit per turn; two bottles extend the duration to six turns. |
| Regen Potion, Mazaleth's Gift | Regeneration's diminishing healing and persistent Ritual growth over six turns. |
| Ship in a Bottle, Heart of Iron, Stable Serum | Current/next-turn block, Plating's player-turn-end block and following-turn decrement, and exact two-turn hand retention/expiry. Plating no longer grants a second block payment at the next player turn. |
| Beetle Juice, Shackling Potion | Damage reduction and duration, temporary enemy Strength restoration. |
| Powdered Demise, Potion of Doom, Poison Potion | End-of-enemy-turn HP loss, Doom threshold, poison damage and decay. Doom checks before Demise's HP loss, preventing a premature kill when Demise first lowers HP below Doom. |
| Lucky Tonic, Ghost in a Jar | Buffer survives fully blocked damage and is consumed by the first HP loss; Intangible reduces damage and expires. |
| Fairy in a Bottle | Cannot be manually consumed; the correct slot empties on lethal damage, a second bottle survives until the next lethal hit, and death occurs after both are spent. The same prevention now handles lethal Baths/Vegetation event damage, then continues the event's remaining effects. Nonlethal damage preserves the bottle. State remains serializable. |
| Gigantification Potion, Duplicator, Soldier's Stew | Multi-hit attack damage, replay without a second energy payment, turn expiry and Strike-tag instance replay. |
| Distilled Chaos | The available top-three batch is reserved before any card plays. Automatic plays ignore energy payment and resume after a generated-card or shuffle selection, preserving the potion continuation and reserved instances. Early victory clears any unplayed reservations. |
| Bottled Potential, Snecko Oil | Hand/draw/discard redistribution without recovering exhausted cards or installed powers; shuffle-choice pause/resume; randomized costs excluding X cards, expiring after their first completed play or at turn end. |
| Bone Brew | Summon HP, absorbing an attack and increasing an existing summon's current and maximum HP. The full Necrobinder actor behavior is still absent. |
| Focus Potion, Potion of Capacity, Essence of Darkness | Orb-slot growth, slot bound, dark-orb creation, Focus growth and eviction damage. Only dark orbs are currently modeled. |
| Star Potion, King's Courage | Star accumulation; one generated Sovereign Blade, additional Forge updating that same blade, and actual attack damage from its forged instance. Regent's full star-spending card pool remains absent. |
| Entropic Brew | Fills the consumed slot, preserves occupied slots, respects current character pool selection. |

All 64 models additionally receive availability and inventory tests: valid/invalid slots, manual/automatic use, target boundaries, dead targets, empty slots, choice and busy-state rejection, out-of-combat scope, full-inventory failure, discard/reacquire and save/load.

## Source-backed corrections and confirmed boundaries

One definite descriptor violation was found during this audit: Touch of Insanity set the combat cost but left a previously randomized turn cost with higher lookup priority. Its selected card could still cost energy during the turn in which the potion was used. The selection handler now clears the earlier turn override when granting the combat-wide free cost. The new regression fixes the order explicitly: nonzero temporary cost, Touch of Insanity selection, immediate zero cost, next-turn zero cost.

The recovered original methods settle several rules that descriptions alone left ambiguous:

1. **Temporary costs expire after the first physical play, including its replay group.** `CardModel.SetToFreeThisTurn` itself calls `SetThisTurnOrUntilPlayed`. This applies to potion offers, Liquid Memories and generated temporary-free cards as well as Snecko Oil. The existing serialized `turn_cost` field can represent this shared lifetime; no new save field is needed. Same-turn return tests verify that a card charges its original cost again. If Touch of Insanity predates Snecko, the random override expires back to the permanent free cost; if Touch follows Snecko, it takes effect immediately.
2. **X-cost is preserved.** The original cost getter bypasses local modifiers for X/negative canonical costs, and Snecko's predicate excludes X before consuming random draws. Free setters do not convert X into a zero-energy full-strength play.
3. **Clarity and Radiance stack duration.** Their hooks grant exactly one extra card or energy, then decrement the power. Two bottles grant the immediate benefit twice and the recurring benefit once for six turns.
4. **Duplicator stacks future triggers.** Its power adds exactly one replay and decrements one layer after modifying the play count. Two bottles therefore duplicate each of the next two cards once; they do not both amplify the next single card. Unused layers expire at turn end.
5. **Soldier's Stew updates existing Strike-tag instances.** The original enumerates all current combat cards and increments their replay count. A Strike generated afterward does not inherit the previous dose. The regression creates a new Strike through the generation opcode after drinking the potion and checks its actual single play.
6. **Bottled Potential preserves exhaust and installed powers.** It moves the hand into the draw pile and invokes the ordinary draw/discard shuffle before drawing five. A Stratagem choice may interrupt the shuffle; accepting it resumes the potion's remaining draw and completion marker.
7. **Cosmic Concoction samples three distinct models.** One batch of the original combat-card factory performs the selection before upgrading/adding them. The new batch generation path removes each selected candidate before the next draw. Ordinary single-card generation retains its existing behavior.
8. **Plating has one block payment per round.** The relevant player hook grants block before the player's turn ends. The next player turn removes ordinary block and decrements Plating. Heart of Iron now protects against the intervening enemy attack without receiving an extra start-turn payment.
9. **Fairy participates outside combat.** The run-state hook iterator includes carried potions when no combat state exists. Lethal event HP loss now goes through the shared prevention path; a recovered player continues the original event. Original decimal HP storage truncates positive fractional healing, matching 24 HP at a maximum of 81.
10. **A full hand does not cancel pile selection.** Both Droplet of Precognition and Liquid Memories first select from their source pile, then invoke the ordinary card-add command. When the hand is full, that command redirects the card to discard. Droplet therefore still removes the selected draw-pile card. Liquid Memories still gives its selected discard-pile card the temporary free cost. Explicit pile moves are not draws and work under No Draw. This correction and its regressions follow the `8adeaeebe4a0380b92ceceff154e30ab5069696c` checkpoint and await a later checkpoint's execution.
11. **Touch of Insanity filters its candidates.** The original predicate checks for a positive energy or star cost with local or full modifiers. The currently supported cards have no star costs or extra global cost hooks, so a positive current energy cost is required. X cards, inherent zero-cost cards and currently free cards are not offered. A randomized positive cost can make an otherwise zero-cost card eligible.
12. **Distilled Chaos reserves the batch before playing.** The original draw-pile auto-play command first removes up to three cards into Limbo, including any necessary shuffle while selecting them, then plays those reserved instances. The first card's draw effect therefore cannot draw the next card scheduled for auto-play. Reservation, shuffle-choice suspension and the potion completion marker survive save/load together.
13. **Doom checks before Demise on enemies.** The original Doom power uses the enemy's before-turn-end hook; Demise uses its after-turn-end hook. An enemy at 40 HP with 33 Doom and 9 Demise survives the first end phase at 31 HP. Doom can kill it at the following enemy turn end. The previous reversed order killed it one turn too early.

The four follow-up corrections above still need the next checkpoint's sanitizer execution. Full original character generation pools, the original acquisition events for four special bottles, all non-dark orb behavior and the full Necrobinder/Regent actors remain outside this two-character expedition. No original-game replay oracle, Firebird execution, or physical-calculator performance result is implied by this audit.
