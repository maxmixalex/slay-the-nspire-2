# Breadth and interface update — 0.8

Reference: v0.111.0 / 41cef1ea. The supplied assembly and PCK were checked against the existing SHA-256 lock. `config/expansion-audit-0.8.json` identifies the inspected new model families and method tokens; `config/ui-audit-0.8.json` identifies original scene/material inputs by path and hash. These are inspection records, not a full original-game execution oracle.

## Gameplay additions

| New potions | Implemented behavior |
|---|---|
| Energy, Strength, Dexterity | +2 energy, +2 Strength, +2 Dexterity respectively. |
| Swift | Draw three, respecting No Draw and the hand cap. |
| Explosive Ampoule | 10 unpowered damage to every living enemy, respecting block but not attack modifiers. |
| Vulnerable, Weak | Three stacks to one selected living enemy. |
| Fruit Juice | +5 maximum and current HP, once; usable outside combat. |
| Blessing of the Forge | Upgrade current combat hand instances, not the persistent deck. |
| Flex, Speed | +5 temporary Strength/Dexterity; remove the actually granted amount at player-turn end. |
| Liquid Bronze | +3 Thorns for combat. |
| Fortifier | Triple current block without Dexterity multiplying the gain. |

The existing Fire, Block and Blood potions remain, bringing the total to 16. New bottle art comes from the pinned installation. Canonical values, target/use scopes and relevant status hooks were inspected. Potion tooltips paraphrase the supported behavior; they are not a claim of exact original localization formatting or all status interactions.

New relics: **Strawberry +7 max HP, Pear +10, Mango +14; Bag of Marbles applies one Vulnerable at combat start; Akabeko grants eight Vigor on the first turn; Bronze Scales grants three Thorns.** Acquisition effects run once, ownership order is preserved, and high-bit relic IDs survive the legacy signed log field. Vigor scopes to the first actual powered attack command and all its hits, not potions or a zero-hit X-cost card. Thorns runs before the incoming hit resolves; a hit already in progress is not erased merely because its attacker dies to Thorns. Simultaneous death and queued-effect edges are covered by internal tests.

New colorless cards: **Finesse** costs zero, grants 4/7 block and draws one; **Flash of Steel** costs zero, deals 5/8 damage and draws one. These are the pinned sequel values, not values assumed from the first game. They enter practice and the colorless shop shelf; class rewards/transforms continue using the 24-card Ironclad subset.

Original full item/card pools and selection odds are not implied. The legacy shop still uses simplified price sampling, relic pricing and unlocked subset stock. Its new category counts follow the original inventory scene, but its stock is not seed-compatible with an original merchant. The legacy potion-drop meter uses integer percent values on the existing custom RNG; it is not the original floating-point reward factory.

## Interface changes and boundaries

The new menu, loot, merchant, rest, pause and top bar are reflowed from the supplied scene structure and use original textures. The title background and Ironclad selection background are converted **stills** from original scene animation data; no full original main-menu animation is claimed. The catalog explicitly lists only implemented content and does not create placeholder playable characters.

Victory now stages loot. Gold and a potion are claimed independently; a card opens a separate three-card screen. Claims are idempotent and saved. A full potion bar leaves the dropped bottle available; opening the potion panel to discard a bottle then returning allows collection. Proceeding is explicit. This intentionally changes the old prototype's automatically granted-gold flow; imported legacy reward states mark their gold already claimed to avoid duplication.

Card portraits are 82×60 (twice the old dimensions), separately sampled for small hand/grid cards and larger details/rewards. Rarity banners use the original common/uncommon/rare material parameters. The authored shader operates in YIQ despite its HSV filename; the converter evaluates its bounded hue/saturation/value controls offline. Text gets a contrasting outline. Upgrade preview does not upgrade a persistent card. Original dynamic cost modifiers, every card keyword tooltip and every rarity-specific animation remain outside this release.

The rest view uses original heal/smith pictures and a sampled campfire layer. `camp_flames.py` evaluates the pinned Overgrowth material's texture/noise controls into 160 frames at 8 fps over a 20-second cycle. Only one blend-mix flame layer is evaluated; full Godot lighting, extra layers/particles and exact color-space equivalence are not asserted. The previous eleven actor/room animation clips are retained without dropping any of their frames.

The shop has five class cards above; two colorless cards below left; three relics and three potions below right, plus removal. Items support hover descriptions and an I/detail overlay. The top bar shows original health/gold/floor/potion/map/deck/settings icons; relics use a separate strip on relevant screens rather than covering pile tabs. Pause uses the original textured controls rather than generic framed rows.

## Content and save formats

**NSP2/5** adds 56 auxiliary image indices after the card directory and requires 26 card records. Card images must be 82×60. Image count is bounded at 8192; the current pack has 999 images. New image format 5 wraps an inner format byte, decoded uint32 size and an LZ4 block. Two fixed decode caches (230400 and 32768 bytes) bound temporary image storage. A returned image is a borrowed view until the next image in the same cache tier; rendering consumes it immediately. This is not general animation streaming.

Glyph records retain full eight-bit alpha. Their formerly reserved field can hold a compressed byte count; nonzero selects `(count, coverage)` runs. The decoder checks exact coverage totals. Image offsets, dimensions, decoded lengths, clip references, card references and all auxiliary references are checked before opening. All content stays in the existing six-MiB resident pack buffer.

**Save v5** keeps the exact legacy v3/v4 prefix and appends expanded shelf/inventory/reward/status fields. v3/v4 imports initialize new statuses, translate sold-item bits and mark unavailable added shelf positions sold, preserving old stock instead of drawing replacements. New v5 saves cannot be read by older launchers. The browser writes a separate v5 key and preserves the previous keys.

LZ4 reference format: https://github.com/lz4/lz4/blob/dev/doc/lz4_Block_format.md . The encoder/decoder in this repository are independent implementations; the upstream source code is not bundled. No game assembly, scene file, original shader, texture atlas or font file is shipped in public source.
