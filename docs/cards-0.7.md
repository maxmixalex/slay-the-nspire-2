# Card expansion — 0.7

Reference: the supplied v0.111.0 / 41cef1ea assembly and PCK, verified against the existing SHA-256 lock. `config/card-audit-0.7.json` records the inspected method tokens. All ten additional models occur in the pinned Ironclad `GenerateAllCards` registry. Registry membership does not mean they are unlocked on every profile.

| Card | Implemented base / upgraded behavior |
|---|---|
| Bloodletting | 0 energy; lose 3 HP, gain 2 / 3 energy. |
| Hemokinesis | 1 energy; lose 2 HP, deal 15 / 20 damage. |
| Impervious | 2 energy; gain 30 / 40 card block; exhaust itself. |
| Offering | 0 energy; lose 6 HP, gain 2 energy, draw 3 / 5; exhaust itself. |
| Bludgeon | 3 energy; deal 32 / 42 damage. Its pinned rarity is uncommon. |
| Thunderclap | 1 energy; deal 4 / 7 to all enemies, then apply 1 Vulnerable. |
| Uppercut | 2 energy; deal 13, then apply 1 / 2 Weak and Vulnerable. |
| Body Slam | 1 / 0 energy; derive base damage from current block, then apply attack modifiers. |
| Battle Trance | 0 energy; draw 3 / 4, then prevent further draws for the player turn. |
| Fiend Fire | 2 energy; snapshot and exhaust the other hand cards, then deal 7 / 10 per captured card; exhaust itself. |

The constructor, dynamic values, upgrades, keyword declarations and asynchronous `OnPlay` state machines were inspected. Fiend Fire explicitly captures `ToList()` before iterating exhaustion. Battle Trance applies No Draw after its own Draw command. The corresponding status-removal hook was inspected. This work did **not** execute the complete original game's combat system, so the ten cards are recorded as implemented and internally/target tested, not fully reference-trace matched.

## Engine behavior

Card IDs 0–13 are unchanged; ten IDs are appended. `card_definitions.inc` holds the checked values and traits. Effects for HP loss, energy, Weak, No Draw and exhaustion by instance ID are appended after the existing effect IDs. Fatal self-damage stops queued gameplay commands and leaves a valid card-zone partition without granting the remaining energy or draws.

Self-exhaust reaches the same ordered Feel No Pain/Dark Embrace hooks as hand exhaustion. Newly drawn cards do not join Fiend Fire's snapshot. No Draw prevents both ordinary draws and exhaust-triggered draws, and clears at player-turn end. Weak modifies enemy attack previews and resolution, and decrements after the enemy acts. Pen Nib remains scoped to all hits of one attack card; potion damage remains separate.

The existing scalar/stat limits and bounded custom combat core remain. New effect support does not reproduce every relic interaction, enemy power, multiplayer target, cost modifier, unlock condition or original reward probability.

## Content and save formats

NSP2/4 uses header offset 32 as the card-record count. After image, glyph, string and clip directories, each 16-byte card record contains four little-endian indices: image, title, base description and upgraded description. Payload bounds, references and expected 41×30 art dimensions are validated. Card visuals no longer depend on `A_Card0 + kind` or on localization slots that collide with events. Gameplay IDs are still eight-bit in this release; complete content coverage will require another reviewed expansion.

Save version 4 appends No Draw and two Weak fields after the existing version-3 state fields. The old decoder path supplies zero defaults. The 0.6-generated suspended-exhaust fixture checks real backward reading, resaving as v4, and continuation. An old program cannot read a new v4 save; the browser therefore writes a separate storage key.

The new content builder preserves original color markup, resolves HP/energy variables and adds canonical self-exhaust keywords. Card descriptions use rich-text layout; enlarged details provide the complete text when the two-line combat summary is insufficient. The tested incremental pack retains all old animation bytes.
