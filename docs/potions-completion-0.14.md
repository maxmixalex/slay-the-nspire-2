# Potion behavior audit — 2026-10-18

Reference remains **v0.111.0 / 41cef1ea**. This audit starts from restored source commit `50a58c9b876c3038e549b97e7e7d245224d717d8`. It separates executable behavior, deterministic regression coverage, and original-game parity. The new regression source has not yet been run at this document's initial checkpoint; the current test receipt belongs in `WORK_STATE.md` after verification.

## Inventory and acquisition

There are 64 active registered potion models, excluding DeprecatedPotion. Every active ID reaches either the direct dispatcher in `potions.cpp`, the continuation dispatcher in `potions_extra.cpp`, or the automatic revival hook. Merely having a dispatch case is not sufficient evidence of a complete original-game implementation.

| Current acquisition group | Count | Models / behavior |
| --- | ---: | --- |
| General pool | 45 | Shared models accepted by both currently playable characters. |
| Ironclad pool | 3 | Blood Potion, Soldier's Stew, Ashwater. |
| Silent pool | 3 | Poison Potion, Ghost in a Jar, Cunning Potion. |
| Other character models | 9 | Bone Brew, Pot of Ghouls, Potion of Doom; Focus Potion, Essence of Darkness, Potion of Capacity; Star Potion, Cosmic Concoction, King's Courage. Explicit practice acquisition is available. Necrobinder, Defect and Regent are not playable characters yet. |
| Special acquisition models | 4 | Ambergris, Foul Potion, Glowwater Potion and Potion-Shaped Rock. Explicit practice acquisition is available; their original event/acquisition routes are not implemented. |

Each currently playable character consequently has a 48-model ordinary potion pool. Shop shelves, expedition rewards, Alchemize and Entropic Brew use the shared pool helper. Explicit `procure_potion` and practice acquisition accept every active model, allowing deliberate acquisition without putting off-character or special bottles into ordinary drops.

The expedition retains two potion slots. A full inventory rejects purchases and reward claims without charging gold, marking the shelf sold, marking the reward claimed, or overwriting a bottle. Discarding one slot allows the same transaction to proceed. Entropic Brew consumes its own bottle before filling empty slots, and preserves an occupied second slot.

The random helper currently samples uniformly from the available models with the expedition RNG. Original rarity weights, potion-generation RNG streams and source-dependent generation restrictions are **not** claimed. In particular, the retained assembly audit identifies `FruitJuice::get_CanBeGeneratedInCombat`; its return value and the factory's filtering must be recovered before certifying combat generation. The original shop and reward probability systems remain outside the current adapted expedition.

## Behavior coverage

`colorless_potions_regression` supplies initial numerical assertions for all 64 active models. `potions_completion_regression` supplements that enumeration with the following state transitions and interactions. These are assertions to execute, not a statement that they have already passed.

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
| Droplet of Precognition, Liquid Memories | Selection moves the requested card instance, retaining its upgrade and the other pile entries. Liquid Memories grants the temporary free cost. |
| Gambler's Brew | Exact selected discard/redraw count, no-draw interaction, and persistence while choosing. |
| Ashwater, Glowwater Potion | Exhaustion snapshots the original hand/selection. Cards newly drawn by Dark Embrace are not accidentally included; ordinary exhaustion hooks still run. |
| Touch of Insanity | The chosen card becomes free immediately even if an earlier same-turn effect gave it a nonzero temporary cost; it remains free on subsequent turns. |
| Cunning Potion, Pot of Ghouls | Token identity, upgraded Shivs, generation despite No Draw and full-hand overflow into discard. |
| Orobic Acid, Cosmic Concoction | Attack/skill/power generation and free cost, or three upgraded colorless instances, respectively. These checks do not certify an unimplemented character's complete generation pool. |
| Clarity, Radiant Tincture | Immediate benefit, the next three turn-start benefits and expiry on the fourth turn. Multiple-bottle stacking still needs original-source confirmation. |
| Regen Potion, Mazaleth's Gift | Regeneration's diminishing healing and persistent Ritual growth over six turns. |
| Ship in a Bottle, Heart of Iron, Stable Serum | Current/next-turn block, retained Plating behavior and exact two-turn hand retention/expiry. |
| Beetle Juice, Shackling Potion | Damage reduction and duration, temporary enemy Strength restoration. |
| Powdered Demise, Potion of Doom, Poison Potion | End-of-enemy-turn HP loss, Doom threshold, poison damage and decay. |
| Lucky Tonic, Ghost in a Jar | Buffer survives fully blocked damage and is consumed by the first HP loss; Intangible reduces damage and expires. |
| Fairy in a Bottle | Cannot be manually consumed; the correct slot empties on lethal damage, a second bottle survives until the next lethal hit, and death occurs after both are spent. Suspended state remains serializable. |
| Gigantification Potion, Duplicator, Soldier's Stew | Multi-hit attack damage, replay without a second energy payment, turn expiry and Strike-tag instance replay. |
| Distilled Chaos | Automatic plays ignore energy payment and resume all remaining plays after a generated-card selection, preserving the potion continuation and auto-play stack. |
| Bottled Potential, Snecko Oil | Hand/draw/discard redistribution, draw count, temporary randomized hand costs and clearing at the next turn. |
| Bone Brew | Summon HP, absorbing an attack and increasing an existing summon's current and maximum HP. The full Necrobinder actor behavior is still absent. |
| Focus Potion, Potion of Capacity, Essence of Darkness | Orb-slot growth, slot bound, dark-orb creation, Focus growth and eviction damage. Only dark orbs are currently modeled. |
| Star Potion, King's Courage | Star accumulation; one generated Sovereign Blade, additional Forge updating that same blade, and actual attack damage from its forged instance. Regent's full star-spending card pool remains absent. |
| Entropic Brew | Fills the consumed slot, preserves occupied slots, respects current character pool selection. |

All 64 models additionally receive availability and inventory tests: valid/invalid slots, manual/automatic use, target boundaries, dead targets, empty slots, choice and busy-state rejection, out-of-combat scope, full-inventory failure, discard/reacquire and save/load.

## Corrections and remaining source questions

One definite descriptor violation was found during this audit: Touch of Insanity set the combat cost but left a previously randomized turn cost with higher lookup priority. Its selected card could still cost energy during the turn in which the potion was used. The selection handler now clears the earlier turn override when granting the combat-wide free cost. The new regression fixes the order explicitly: nonzero temporary cost, Touch of Insanity selection, immediate zero cost, next-turn zero cost.

These original rules require fresh access to the pinned assembly before changing behavior:

1. Whether Bottled Potential includes the exhaust pile, and precisely which active or removed cards qualify as “all cards.” Current source shuffles hand, draw and discard.
2. Multiple-instance stacking for Clarity, Radiant Tincture and Duplicator. Current source stores a shared power amount; the distinction between more duration, more output and more replay must come from the pinned status hooks.
3. Whether a later generated Strike-tag card inherits Soldier's Stew, or only instances present at use receive its replay enchantment.
4. Snecko Oil's treatment of X-cost and non-playable cards; the ordering of later randomized costs versus an earlier permanent free effect.
5. Fairy in a Bottle outside combat. Both `revive` and adapted event HP loss currently prevent that path. This requires checking the original death-prevention hook and event semantics, not copying another game's rules.
6. Full-hand behavior for targeted draw/discard selection and the reference factory's special potion restrictions.

The source tests cannot close these questions by agreeing with the implementation. There is no new original-game replay oracle, original asset pack validation, Firebird execution, or physical-calculator performance result in this audit.
