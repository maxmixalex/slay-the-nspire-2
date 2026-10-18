# Ironclad completion — 0.15

## Scope and execution status

The pinned original declares 90 Ironclad cards. Excluding its five multiplayer-only models leaves **85 single-player cards**: 29 retained IDs and 56 appended at IDs 114–169. Primal Force adds the generated **Giant Rock token at ID 227**. All 85 base/upgraded variants now have explicit engine effect contracts in `tests/ironclad_completion_regression.cpp`. The first source checkpoint, `575b3fce9e12aaa455efc9156b3f904f08780b85`, was pushed and read back with the exact local tree before validation. Its first native validation is in progress; **no passing 0.15 execution result has yet been established in this document**. A subsequent correction distinguishes other-instance Block previews during a suspended choice and will be validated against its next source checkpoint.

This audit is based on the supplied original assembly and PCK, fixed to **v0.111.0 / 41cef1ea**. It does not claim to have run the original game as a differential oracle. Original disassembly, text, image payloads and assembly data remain private. Public config stores model facts, resource paths and method-token references only.

| Input | SHA-256 |
| --- | --- |
| Original assembly | `0861bfa1df347538d932f22d580e75420f08082792eb914e53b4882764acdbe9` |
| Original PCK | `c60f672ee7804e6aefa1e19a582fa1c80b126a7b0eef4d084d3abf110df2eab7` |

`IroncladCardPool.GenerateAllCards` (`060047f8`) supplies the declaration order. `CardFactory.FilterForPlayerCount` (`06004f08`, single-player predicate `0600affe`) removes constraint 1. The excluded models are Blaze, DemonicShield, Midnight, Outrage and Tank. The recursively inspected card/power references require GiantRock; they do not require the STS1 Burn, Wound, Dazed, Void or Slimed cards.

## Catalog and original facts

`config/ironclad-cards-0.15.json` records all 85 models in original declaration order, including rarity, target, cost, canonical base/upgraded variables, keyword changes, Strike tags, combat-generation restrictions and evidence tokens. The 29 retained definitions match the pinned numeric/upgrade rules; Whirlwind uses the original X flag even though its original constructor's numeric base is zero. Dark Embrace applies a fixed one-card draw trigger; upgrading lowers its cost.

Break and Corruption have Ancient rarity and are available in the complete practice/compendium pool; ordinary random generation and rewards exclude that rarity. Feed (`06003f0f`) and NotYet (`0600426d`) explicitly disallow combat generation. AshenStrike and SetupStrike have the Strike tag, which participates in Perfected Strike and Hellraiser. GiantRock belongs to the original colorless token frame/energy presentation (`TokenCardPool` methods `06004831`–`06004834`).

The following table lists energy and canonical variables. `a → b` means base → upgraded; `X` captures current energy. Runtime calculated damage, block and hit counts are detailed below rather than treated as fixed canonical numbers.

| ID | Model | Type | Rarity | Energy | Canonical variables | Keywords base → upgraded |
| ---: | --- | --- | --- | --- | --- | --- |
| 114 | Aggression | Power | Rare | 1 | — | — → Innate |
| 3 | Anger | Attack | Common | 0 | Damage 6 → 8 | — |
| 115 | Armaments | Skill | Common | 1 | Block 5 | — |
| 116 | AshenStrike | Attack | Uncommon | 1 | CalculationBase 6; ExtraDamage 3 → 4 | — |
| 117 | Barricade | Power | Rare | 3 → 2 | — | — |
| 2 | Bash | Attack | Basic | 2 | Damage 8 → 10; VulnerablePower 2 → 3 | — |
| 22 | BattleTrance | Skill | Uncommon | 0 | Cards 3 → 4 | — |
| 118 | BloodWall | Skill | Common | 2 | HpLoss 2; Block 16 → 20 | — |
| 14 | Bloodletting | Skill | Uncommon | 0 | HpLoss 3; Energy 2 → 3 | — |
| 18 | Bludgeon | Attack | Uncommon | 3 | Damage 32 → 42 | — |
| 21 | BodySlam | Attack | Common | 1 → 0 | CalculationBase 0; ExtraDamage 1 | — |
| 119 | Brand | Skill | Rare | 0 | HpLoss 1; StrengthPower 1 → 2 | — |
| 120 | Break | Attack | Ancient | 1 | Damage 20 → 30; VulnerablePower 5 → 7 | — |
| 121 | Breakthrough | Attack | Common | 1 | Damage 9 → 13; HpLoss 1 | — |
| 122 | Bully | Attack | Uncommon | 0 | CalculationBase 4; ExtraDamage 2 → 3 | — |
| 6 | BurningPact | Skill | Uncommon | 1 | Cards 2 → 3 | — |
| 123 | Cascade | Skill | Rare | X | — | — |
| 124 | Cinder | Attack | Common | 2 | Damage 18 → 24 | — |
| 125 | Colossus | Skill | Uncommon | 1 | Block 4 → 7; Colossus 1 | — |
| 126 | Conflagration | Attack | Rare | 1 | Damage 2; Repeat 4 → 5 | — |
| 127 | Corruption | Power | Ancient | 3 → 2 | Power 1 | — |
| 128 | CrimsonMantle | Power | Rare | 1 | CrimsonMantlePower 7 → 10 | — |
| 129 | Cruelty | Power | Uncommon | 1 | CrueltyPower 25 → 50 | — |
| 13 | DarkEmbrace | Power | Rare | 2 → 1 | — | — |
| 1 | DefendIronclad | Skill | Basic | 1 | Block 5 → 8 | — |
| 62 | DemonForm | Power | Rare | 3 | StrengthPower 3 → 4 | — |
| 130 | Dismantle | Attack | Uncommon | 1 | Damage 8 → 10 | — |
| 131 | Dominate | Skill | Rare | 1 | VulnerablePower 1 → 2; StrengthPerVulnerable 1 | Exhaust |
| 132 | DrumOfBattle | Skill | Uncommon | 1 | Cards 2; Energy 2 → 3 | — |
| 133 | EvilEye | Skill | Uncommon | 1 | Block 8 → 11 | — |
| 134 | ExpectAFight | Skill | Uncommon | 3 | CalculationBase 15 → 16; CalculationExtra 5 → 8 | — |
| 135 | Feed | Attack | Rare | 1 | Damage 10 → 12; MaxHp 3 → 4 | Exhaust |
| 12 | FeelNoPain | Power | Uncommon | 1 | Power 3 → 4 | — |
| 23 | FiendFire | Attack | Rare | 2 | Damage 7 → 10 | Exhaust |
| 136 | FightMe | Attack | Uncommon | 2 | Damage 5 → 6; Repeat 2; StrengthPower 3 → 4; EnemyStrength 1 | — |
| 61 | FlameBarrier | Skill | Uncommon | 2 | Block 12 → 16; DamageBack 4 → 6 | — |
| 137 | ForgottenRitual | Skill | Uncommon | 1 | Energy 3 → 4 | Exhaust |
| 138 | Havoc | Skill | Common | 1 → 0 | — | — |
| 58 | Headbutt | Attack | Common | 1 | Damage 9 → 12 | — |
| 139 | Hellraiser | Power | Rare | 2 → 1 | — | — |
| 15 | Hemokinesis | Attack | Uncommon | 1 | HpLoss 2; Damage 15 → 20 | — |
| 140 | HowlFromBeyond | Attack | Uncommon | 3 | Damage 18 → 24 | — |
| 16 | Impervious | Skill | Rare | 2 | Block 30 → 40 | Exhaust |
| 141 | InfernalBlade | Skill | Uncommon | 1 → 0 | — | Exhaust |
| 142 | Inferno | Power | Uncommon | 1 | InfernoPower 6 → 9 | — |
| 9 | Inflame | Power | Uncommon | 1 | StrengthPower 2 → 3 | — |
| 10 | IronWave | Attack | Common | 1 | Damage 5 → 7; Block 5 → 7 | — |
| 143 | Juggernaut | Power | Rare | 2 | JuggernautPower 6 → 8 | — |
| 144 | Juggling | Power | Uncommon | 1 | — | — → Innate |
| 145 | Mangle | Attack | Rare | 3 | Damage 20 → 26; StrengthLoss 10 → 15 | — |
| 146 | MoltenFist | Attack | Common | 1 | Damage 10 → 14 | Exhaust |
| 147 | NotYet | Skill | Rare | 2 | Heal 10 → 13 | Exhaust |
| 17 | Offering | Skill | Rare | 0 | HpLoss 6; Energy 2; Cards 3 → 5 | Exhaust |
| 148 | OneTwoPunch | Skill | Rare | 1 | Attacks 1 → 2 | — |
| 149 | PactsEnd | Attack | Rare | 0 | Damage 18 → 24; Cards 3 | — |
| 60 | PerfectedStrike | Attack | Common | 2 | CalculationBase 6; ExtraDamage 2 → 3 | — |
| 150 | Pillage | Attack | Uncommon | 1 | Damage 6 → 9 | — |
| 4 | PommelStrike | Attack | Common | 1 | Damage 9 → 10; Cards 1 → 2 | — |
| 151 | PrimalForce | Skill | Rare | 0 | — | — |
| 152 | Pyre | Power | Rare | 2 | Energy 1 → 2 | — |
| 153 | Rage | Skill | Uncommon | 0 | Power 3 → 5 | — |
| 154 | Rampage | Attack | Uncommon | 1 | Damage 10; Increase 5 → 10 | — |
| 155 | Rupture | Power | Uncommon | 1 | StrengthPower 1 → 2 | — |
| 156 | SecondWind | Skill | Uncommon | 1 | Block 5 → 7 | — |
| 157 | SetupStrike | Attack | Common | 1 | Damage 7 → 9; StrengthPower 3 → 4 | — |
| 5 | ShrugItOff | Skill | Common | 1 | Block 8 → 11; Cards 1 | — |
| 158 | Spite | Attack | Uncommon | 0 | Damage 5; Repeat 2 → 3 | — |
| 159 | Stampede | Power | Uncommon | 2 → 1 | Power 1 | — |
| 160 | Stoke | Skill | Rare | 1 | — | — |
| 161 | Stomp | Attack | Uncommon | 3 | Damage 12 → 15 | — |
| 162 | StoneArmor | Power | Uncommon | 1 | PlatingPower 4 → 6 | — |
| 0 | StrikeIronclad | Attack | Basic | 1 | Damage 6 → 9 | — |
| 59 | SwordBoomerang | Attack | Common | 1 | Damage 3; Repeat 3 → 4 | — |
| 163 | Taunt | Skill | Common | 1 | Block 6 → 7; VulnerablePower 1 → 2 | — |
| 164 | TearAsunder | Attack | Rare | 2 | Damage 5 → 7; Repeat 1; CalculationBase 0; CalculationExtra 1 | — |
| 165 | Thrash | Attack | Rare | 1 | Damage 4 → 6 | — |
| 19 | Thunderclap | Attack | Common | 1 | Damage 4 → 7; VulnerablePower 1 | — |
| 166 | Tremble | Skill | Common | 1 | VulnerablePower 3 → 4 | Exhaust |
| 7 | TrueGrit | Skill | Common | 1 | Block 7 → 9 | — |
| 8 | TwinStrike | Attack | Common | 1 | Damage 5 → 7 | — |
| 167 | Unmovable | Power | Rare | 2 → 1 | — | — |
| 168 | Unrelenting | Attack | Uncommon | 2 | Damage 14 → 20 | — |
| 20 | Uppercut | Attack | Uncommon | 2 | Damage 13; Power 1 → 2 | — |
| 169 | Vicious | Power | Uncommon | 1 | Cards 1 → 2 | — |
| 11 | Whirlwind | Attack | Uncommon | X | Damage 5 → 8 | — |
| 227 | GiantRock | Attack | Token | 1 | Damage 20 → 24 | — |

## Mechanics and ordering

### Attack and HP histories

Attack starts and finishes are separate counters. Stomp receives a local cost reduction on every started Attack (`0600458c`, reduction `0600458d`); a newly created non-clone Stomp starts with this turn's finished-attack reductions (`0600458b`). Juggling copies the third started Attack before that card resolves (`06009f66`). Copies preserve the original instance's damage/cost/enchantment modifiers; they do not repeat the non-clone history initialization.

Every actual positive HP-loss event increments the combat history used by Tear Asunder and the turn history used by Spite. Buffer prevention and an Osty interception do not fabricate a player HP-loss event. Inferno and Rupture act only on the player's side. Rupture registers an active card when it begins, accumulates the Strength associated with damage from that card, then delivers it when that card finishes (`0600281d`, `06009ffd`, `06009ffb`). Hemokinesis therefore resolves its own attack before receiving the Strength earned by its HP cost. Per-frame pending values preserve this distinction during nested automatic plays and save/resume.

Crimson Mantle and Inferno maintain a separate self-damage amount for the number of applied copies. Two Mantles produce one two-HP loss at turn start, not two one-HP losses; the resulting Inferno/Rupture trigger is a single event. Their positive Block/Damage amounts still stack. Colossus always halves powered attack damage from Vulnerable enemies; stacked amounts extend enemy-side duration (`06009e8e`) instead of repeatedly dividing damage.

### Block source and Unmovable

The shared engine distinguishes card-powered Block from unpowered Block produced by powers, relics and potions. Both can trigger Juggernaut. Only card-powered Block enters Unmovable's count. Dexterity and the card-specific modifiers apply before its multiplicative stage.

Unmovable counts **BlockGained history entries**, excluding those belonging to the current CardPlay (`060029d3`, predicate `0600a0c4`). It does not count distinct cards. Every Block operation of the current play receives the same benefit while previous plays' entry count is below the stacked amount. With two stacks, an eligible Evil Eye can gain doubled Block twice; those two history entries then exhaust the threshold for a subsequent Defend. The regression covers this exact two-stack boundary, Second Wind's repeated Block and unpowered Juggernaut damage. During an Armaments choice, the preview of another card includes the Block already gained by Armaments instead of subtracting that active play's history. Live Block descriptions also include Silent's Shadowmeld multiplier when mixed card sources are present.

### Local cost modifiers, free powers and X

The original cost engine applies ordered local `Add` and `Set` entries (`CardEnergyCost.GetWithModifiers`, `0600521d`). Set-this-turn-or-until-played (`06005223`), Set-this-combat (`06005225`) and Add-this-turn (`06005228`) retain their relative insertion order. Stomp stores cumulative reductions and the reduction count at each surviving Set. This preserves a later Snecko Set overwriting earlier reductions, subsequent attacks lowering that Set, and expiry revealing surviving combat modifiers. The same origin model is shared with Silent's Pinpoint.

X cards bypass local and global cost hooks. `GetAmountToSpend` (`06005220`) immediately returns the player's current Energy for X, and `SpendEnergy` (`06009b0a`) stores the captured amount. Corruption (`060024bd`) and FreeAttack (`060025d0`) therefore cannot make Cascade or Whirlwind cost zero. FreeAttack still consumes its next-Attack allowance when an X attack begins. Regression cases assert the captured effects, spent Energy and resulting exhaust/replay state.

### Automatic play and draw continuations

Cascade uses the shared reserved batch auto-play path. Havoc chooses one draw-pile card, shuffles if necessary, plays it automatically and forces that physical card to exhaust after all of its replays (`0600aa17`). The force flag is serialized during a suspended Armaments choice and cleared after the group. Original cloning explicitly clears that flag (`CardModel.CreateClone`, `060018fb`).

Hellraiser auto-plays a drawn card carrying the Strike tag. Pillage continues based on the card returned by its own one-card Draw (`0600ab89`); nested draws caused by that card must not replace the outer result. A per-frame captured card and queued continuation keep the result stable. The tests include Pillage → Pommel Strike → skill draw and Havoc → Armaments → saved choice.

Stampede chooses a random Attack from the **current** hand on each iteration (`0600a04b`), so newly created attacks are eligible for later iterations. It runs before Howl From Beyond's exhausted-card listener, following the original creature-power-before-card listener hierarchy (`CombatState`, `0600b3f4`). A manually played Howl discards normally; its exhaust-pile trigger also returns it to its ordinary result location unless another modifier forces exhaust (`0600aa48`, `0600aa4a`, base result location `060018f4`).

### Snapshot operations, generation and persistent instances

Second Wind snapshots non-Attack cards in hand, then exhausts one and gains Block before proceeding to the next (`0600ac3a`). Cards drawn by Dark Embrace during those exhausts are not retroactively added. Drum of Battle grants its upgraded or base Energy when exhausted. Stoke records the current hand, exhausts that snapshot and generates that many cards from the character's eligible pool, upgraded if appropriate (`0600acd8`).

Aggression chooses distinct discard-pile Attacks before hand draw, moves each to the hand and upgrades it (`06009e4d`). Full-hand overflow follows shared pile insertion rules. Primal Force transforms current hand Attacks to base/upgraded Giant Rock (`0600abaa`) and clears both character modules' instance modifiers. The shared clone helper preserves both character modules' properties, so Juggling does not lose Silent enchantments or costs on a mixed attack.

Rampage's increase and Thrash's absorbed damage belong to a physical card instance. Thrash attacks twice, selects a random hand Attack, computes that selected card's current damage against no enemy target using all original damage-hook phases, adds it to Thrash, then exhausts the selected card (`0600ad4b`; `ModifyDamageHookType.All = 14`). It excludes consumable attack modifiers already spent by the playing Thrash. Clones preserve the bonus; transforms reset it.

## State, choices and compatibility

The isolated module contains 18 positive powers with explicit acquisition order, bounded attack/HP/exhaust/block counters, per-auto-frame Rupture and Pillage continuations, and per-instance damage/cost/forced-exhaust state. It adds 26 serialized effect operations and one Armaments upgrade choice after the frozen pre-0.15 enums. Variable-length card effects use bounded continuations or preflight the shared queue/card capacities before moving their snapshot.

Save v13 appends the Ironclad and Silent modules to the historical prefix. Old v3–v12 files have no new-module history and load those fields as zero. The validator rejects duplicate or absent power-order members, illegal amount/instance bounds, reductions on non-Stomp cards, damage bonuses on unrelated models, impossible per-frame captures, malformed operation operands and Armaments choices that no longer reference eligible hand cards. The tests check rejected choices leave the serialized state unchanged and legitimate suspended choices produce identical results after loading.

## Regression coverage and bounded claims

The suite includes a concrete numeric or state-transition contract for all **170 Ironclad base/upgrade variants**, plus both Giant Rock variants and interactions for:

- Corruption/FreeAttack with X energy; ordered Snecko/Touch cost overrides and side-end cleanup.
- Deferred Rupture, combined Mantle HP loss, Unmovable multi-Block and stacked event thresholds.
- Havoc's forced exhaust through save/resume; Hellraiser's nested draws and Pillage's draw result.
- Snapshot exhaust operations, automatic/generated cards, instance cloning, transform reset and Thrash's consumed Vigor exclusion.
- Illegal choices and corrupted module state.

The repository's adapted encounters, two-enemy limit, queue/card/auto-frame capacities and deterministic custom RNG remain engine constraints. This audit does not claim original seed parity, full original-run parity, original-game replay comparison, or device execution. Cross-module ordering outside the explicitly represented shared hook phases must be assessed through its concrete regression coverage; per-module acquisition order alone is not proof of a universal original hook-listener order.
