# Shared cards, relics and deck UI — 0.10

Reference: the owner's hash-verified **v0.111.0 / 41cef1ea** assembly and PCK. `config/shared-audit-0.10.json` records original model/method tokens. The seven added cards occur in the inspected colorless registry. Evidence is read-only metadata/CIL inspection, not execution of the complete original game. Full combat differential parity is not claimed.

## Added card behavior

| Card | Base / upgraded behavior |
|---|---|
| Impatience | 0 energy; draw 2 / 3 only when the remaining hand contains no Attack cards. |
| Master of Strategy | 0 energy; draw 3 / 4; Exhaust. |
| Shockwave | 2 energy; apply 3 / 5 Weak and Vulnerable to each living enemy; Exhaust. A colorless pool member in this pinned sequel. |
| Scrawl | 1 energy in both states; draw to the hand limit; Exhaust. The upgrade adds Retain. |
| Purity | 0 energy; optionally exhaust 0–3 / 0–5 other hand cards; Retain and Exhaust in both states. |
| Hand of Greed | 2 energy; 20 / 25 attack damage; gain 20 / 25 gold when that hit kills its target. |
| Panic Button | 0 energy; gain 30 / 40 card block, then 2 No Block; Exhaust. |

Existing two colorless cards remain, giving nine shared pool models. Character-specific normal rewards are unchanged; the two colorless shop slots sample distinct cards from the nine implemented shared models. Both practice decks contain the additional shared cards. Original complete stock generation, acquisition odds and prices remain unfinished.

### Ordering and state

Purity captures instance IDs rather than mutable hand positions. Required range, uniqueness and current hand membership are validated before changing any pile. Exhaust commands execute in selection order, with each supported Feel No Pain/Dark Embrace hook before the next captured card. Newly drawn cards do not join the captured set. Zero selection still exhausts Purity itself and invokes its supported exhaust hooks. An empty remaining hand completes without displaying an empty modal.

Purity and upgraded Scrawl stay in hand at turn end. Their retained instances count toward the next draw's hand capacity. Generation and drawing remain distinct under No Draw. No Block suppresses card-source block, not the existing unpowered Feel No Pain gain, relic block or potion block. It decrements at the end of the enemy side. Panic Button gains its own block before applying No Block, but an already active No Block prevents that card block too.

Hand of Greed uses ordinary attack modifiers and block handling before evaluating a living-to-dead transition. Its bonus gold is distinct from the later normal loot claim. The bounded encounter model lacks original minions and death-prevention powers; those interactions are not advertised as supported. Fixed stat, deck and effect bounds remain.

## Added relic behavior

| Relic | Supported effect |
|---|---|
| Horn Cleat / 船夹板 | 14 block on player turn 2. |
| Captain's Wheel / 舵盘 | 18 block on player turn 3. |
| Mercury Hourglass / 水银沙漏 | 3 unpowered damage to each living enemy at player-turn start. |
| Art of War / 孙子兵法 | One extra energy next turn when the previous player turn played no Attack; not on turn 1. |
| Regal Pillow / 皇家枕头 | 15 extra healing when resting. |
| Meal Ticket / 餐券 | Heal 15 on merchant-room entry. Buying it inside a merchant does not replay entry. |
| Old Coin / 古钱币 | Gain 300 gold on acquisition, once. |
| Centennial Puzzle / 百年积木 | Draw 3 upon the first nonfatal HP loss of each combat. |

Art of War persists the completed-turn attack flag, including a zero-hit X-cost attack. Hourglass damage does not use Strength, Vulnerable or attack multipliers. Relic block is granted after the next-turn block clear and is not suppressed by No Block. Rest UI and resolution share the same healing calculation.

Puzzle's first-loss flag is set before drawing. Fully blocked hits do not trigger it; direct self-HP loss does. No Draw can suppress its draw without leaving the relic armed for another attempt. Lethal loss does not draw after death. The flag is serialized and resets on new combat. Tests cover the supported engine's hook ordering, not every original enemy, relic or death-prevention combination.

## Interface and input

Permanent upgrade/removal/transformation choices now use a ten-card grid with page controls, five-column keyboard navigation and details. Choosing opens an explicit confirmation. Upgrades show two larger original-framed cards and their base/upgraded descriptions, including Scrawl's added Retain. Longer descriptions are summarized with an ellipsis; the full card detail remains available before choosing.

The confirmation captures deck index, phase and gameplay revision. Cancellation changes no deck, gold or RNG. A changed revision invalidates the pending confirmation; pointer releases cannot activate underlying cards through a modal. The transformation's already-selected first card stays unavailable for the second selection.

Large details use the card's actual red/green/colorless frame and energy image. Collections page relics/potions in groups of eighteen and open full item details. The 25-relic second page contains seven real items. The HUD exposes excess relics through a count rather than silently truncating them. The calculator's C key now invokes the same compendium as X11/Web.

These are adapted original-asset layouts, not pixel-identical PC screens. No audio, multiplayer or Timeline features are added.

## Formats and resource boundary

**NSP2/7** has 58 card records, 17 clips and 83 auxiliary image records. Existing IDs remain stable; seven shared portraits, eight relic icons and the No Block icon are appended. All 1,286 old image records retain their encoded payloads, so no prior animation frames are discarded or resampled. Existing glyph records retain their coverage too. The new converter accepts a v6 or v7 private pack and verifies both source identity and final size.

**Save v7** keeps historical decoding sizes for v3–v6 and appends optional-exhaust state, No Block duration, the turn attack flag and eight new relic slots/counters. The v6 fixture was produced by the retained 0.9 runtime before this work, not by editing v7 bytes to look old. Readers reject incompatible values and corrupt checksums. New saves cannot be opened by old launchers.

The six-MiB resident pack, bounded decompression caches and fixed gameplay capacities remain. Larger future content will require further resource design rather than treating this release's small remaining headroom as unlimited.
