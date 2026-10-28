# Silent subset and interface update — 0.9

## Reference and count semantics

The original reference remains v0.111.0 / 41cef1ea. The supplied assembly and PCK hashes were verified before inspection/conversion. `config/card-audit-0.9.json` records the inspected model/method tokens, including Silent's starting-state methods and the card-pool registry. It does not contain proprietary method bodies.

This iteration adds **24 Silent pool models and one generated Shiv**, taking the project total from 26 to 51. The 24 ordinary models occur in the pinned 91-entry Silent pool registry; Shiv is a token, not a normal reward. Remaining original cards are not implicitly supported. Some old localization keys lack a corresponding current model and were not used as implementation evidence.

The reference constructor, canonical variables, upgrades, keywords and relevant asynchronous play methods were inspected. Poison/status callbacks and the first-turn-only starting relic were inspected as well. **This iteration did not run the original game or add an original-CIL combat execution oracle.** Tests below are implementation/invariant checks, not proof of complete original-game parity.

## Implemented cards

| Model | Base / upgraded behavior within the supported engine |
|---|---|
| StrikeSilent | 1 energy; 6 / 9 damage. |
| DefendSilent | 1 energy; 5 / 8 block. |
| Neutralize | 0 energy; 3 / 4 damage, then 1 / 2 Weak. |
| Survivor | 1 energy; 8 / 11 block, discard 1. |
| Acrobatics | 1 energy; draw 3 / 4, discard 1. |
| Backflip | 1 energy; 5 / 8 block, draw 2. |
| DaggerThrow | 1 energy; 9 / 12 damage, draw 1, discard 1. |
| DeadlyPoison | 1 energy; apply 5 / 7 Poison. |
| PoisonedStab | 1 energy; 6 / 8 damage, then 3 / 4 Poison. |
| CloakAndDagger | 1 energy; 6 block and create 1 / 2 Shivs. |
| BladeDance | 1 energy; create 3 / 4 Shivs; exhaust itself in this reference version. |
| DaggerSpray | 1 energy; deal 4 / 6 to every enemy twice. |
| Dash | 2 energy; 10 / 13 block and 10 / 13 damage. |
| LegSweep | 2 energy; apply 2 / 3 Weak before gaining 11 / 14 block. |
| Footwork | 1 energy power; gain 2 / 3 Dexterity. |
| NoxiousFumes | 1 energy power; apply 2 / 3 Poison to enemies at player-turn start. |
| BouncingFlask | 2 energy; apply 3 Poison to a random living enemy, 3 / 4 times. |
| Prepared | 0 energy; draw 1 / 2, then discard 1 / 2. |
| Slice | 0 energy; 6 / 9 damage. |
| Deflect | 0 energy; 4 / 7 block. |
| SuckerPunch | 1 energy; 8 / 10 damage, then 1 / 2 Weak. |
| Adrenaline | 0 energy; gain 1 / 2 energy, draw 2, exhaust. |
| Accuracy | 1 energy power; Shivs deal an additional 4 / 6 damage. |
| LeadingStrike | 1 energy; 3 / 6 damage and create 2 Shivs. |
| Shiv | Generated card; 0 energy; 4 / 6 damage, exhaust. |

## Starting state, pools and combat boundaries

Silent starts with 70 HP, five Strikes, five Defends, Neutralize, Survivor and Ring of the Snake. Ring of the Snake gives two additional cards at the first player turn, not every turn. It is not offered as a random generic shop/treasure relic. The Ironclad starting relic remains separate, so Silent no longer inherits unconditional post-battle healing.

Rewards, the five colored shop slots and transformations use the chosen character's modeled pool. Colorless slots remain the two implemented colorless cards. Shiv is excluded from ordinary pools. This is a corrected subset selection boundary, not the complete original reward factory or rarity/unlock distribution. All implemented content is available without Timeline progression, which is explicitly out of scope.

Poison damage resolves before enemy actions, bypasses block and does not use Strength, Vulnerable or Pen Nib multipliers. Living affected enemies then lose one Poison; lethal Poison prevents their action. Noxious Fumes applies on the next player-turn start rather than immediately upon playing the power. Accuracy is part of Shiv damage before attack modifiers. New statuses clear on a fresh combat.

Card creation is not drawing and remains possible under No Draw. When a created Shiv does not fit the hand, it goes to the discard pile, rather than disappearing. This branch was checked against the original card-pile addition logic. The engine still has fixed card/effect/stat capacities and lacks many original enemy and relic interactions.

## Mandatory card choices

Discard and exhaust are distinct gameplay phases but share an original-informed card-selection overlay. A choice captures actual card IDs, not unstable hand positions. Multi-discard validates required count, uniqueness and membership before changing any pile. The UI requires explicit confirmation and cannot activate an enemy behind the overlay.

Enter/click toggles selection, Space confirms the selected set, and I opens details. A detail view returns to the same partial selection. Saving records the pending game effect and required discard count; loading intentionally clears presentation-only selected checkmarks. Keyboard input, touchpad capture and modal transitions share cancellation rules.

## Assets and UI

The original Silent character-selection skeleton is rendered to a static small-screen background. Portrait, starting relic icon, green energy imagery, original frame material parameters, Poison/Accuracy/Noxious icons and the original Silent map marker are included. These are converted original assets, not invented stand-ins for other characters.

Six additional clips cover Silent idle, attack, hurt, cast, Shiv and camp presentation. Combat clips are sampled at approximately 24 fps; the eight-second Silent camp loop is sampled at approximately 16 fps to stay within the resident-pack budget. These sample rates are **not measured device frame rates**. All 999 baseline image payloads, including the original 11 clips/706 frames and 160 campfire frames, are preserved after decoding.

The compendium supports All / Ironclad / Silent / Colorless filters, default/cost/type sorting, base/upgraded viewing and the seventeenth relic. The Silent filter includes the generated Shiv; the implementation count distinguishes it from the 24 ordinary pool models. Collection inspection uses canonical card values rather than leaking the current combat's temporary buffs into catalog descriptions.

## Formats

The content version is NSP2/6: existing card/clip IDs are preserved, with 51 card records, 17 clips and 74 auxiliary image entries. The converter validates the pinned installation and retains the existing lossless image/glyph codecs. The App rejects an older content pack that lacks the required card directory.

Project save version 6 adds character, Poison, Noxious Fumes, Accuracy, pending-discard data and the expanded relic representation. Old v3/v4/v5 saves use their historical fixed array sizes and default to Ironclad. The new v5 migration fixture also covers Bronze Scales' historical signed-log representation. The browser uses a separate v6 key and leaves old keys intact. Old runtimes cannot read a v6 save.

Audio, multiplayer and Timeline are excluded product features. Parsing past an audio/event field in the original skeleton format is necessary to read the file and does not introduce audio playback or Timeline progression.
