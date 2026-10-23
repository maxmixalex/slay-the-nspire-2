# Event option hover contract

Reference: v0.111.0 / 41cef1ea. The source repository contains model identifiers,
association metadata and independent rendering code, without original text or art.

## Ownership and interaction

`EventOption.HoverTips` is distinct from colored text in an option. The 57 static
association rows in `event_hover_catalog.inc` follow the option constructors,
including page-specific Trial choices and the later Colossal Flower choice.
Dynamic choices read saved event values for Slippery Bridge, Doll Room, Relic
Trader, potions, Tinker Time, Endless Conveyor and Wongo. Ancient FromRelic
options exclude their own relic description while retaining ExtraHoverTips.
147 relic association rows include canonical mechanic keys. Archaic Tooth,
Dusty Tome and Touch of Orobas read already prepared or owned models.
Potion options distinguish the single FromPotion tip from full get_HoverTips;
39 extra association rows cover the latter, including upgraded Shiv, summon,
orb and power details. Dusty Tome collects base-card extras before its upgraded
card preview; its localized StringVar remains the base title as in the original.

Pointer hover and the keyboard-selected option share an owner. `V` opens the
readable paged view; Up/Down or wheel changes pages; Back or V returns. Event
reader and other modal surfaces suppress underlying hover. Narrow calculator
screens use the left column; full-width ancient rows use a compact text panel
above the selected row, with the full card preview available through V.
No hover path writes State, prepares an offer, draws RNG or serializes new fields.

## Primary reference evidence

- `EventOption.get_HoverTips` / setters: method tokens 0x06004f36 / 0x06004f37.
- `EventOption.FromRelic`: 0x06004f4f reads HoverTipsExcludingRelic.
- `Amalgamator.GenerateInitialOptions`: 0x060034f2 links Ultimate Strike/Defend.
- `AromaOfChaos.GenerateInitialOptions`: 0x060034f7 links Transform only to Let Go.
- `GraveOfTheForgotten.GenerateInitialOptions`: 0x0600358f links Souls Power/Decay.
- `Trial.Accept`: 0x06003705 has distinct page/choice card associations.
- `ArchaicTooth.UpdateHoverTips`: 0x06001c29 uses stored source/result cards.
- `DustyTome.set_AncientCard`: 0x06001da1; TouchOfOrobas setters 0x060022de/0x060022e0.
- `HoverTipFactory.FromCard`: 0x06004c58 upgrades the preview when its second argument is true.
- `PotionModel.get_HoverTips`: 0x06001b2b includes ExtraHoverTips; `FromPotion` 0x06004c50 returns only the potion tip.
- StaticHoverTip.Transform is enum value 4; it must not be confused with Upgrade.

The fixed reference assembly SHA-256 is
`0861bfa1df347538d932f22d580e75420f08082792eb914e53b4882764acdbe9`.
Private disassembly is inspection input only and is not checked in.

## Executed validation

The frozen, remotely verified checkpoint
`9d8cb1fd58d2ea73900449415c925592776a5584` passes the keyword-tooltip suite:

| Resource profile | Checks | Result |
| --- | ---: | --- |
| Synthetic | 21,734 | Passed |
| Chinese NSP2/18 | 402,524 | Passed |
| English NSP2/18 | 402,632 | Passed |

The regression covers explicit associated and empty options, prepared dynamic
cards, every potion/relic hover-text route, enchantments, pointer and keyboard
ownership, modal suppression, paging and unchanged save bytes. Both languages'
compact event hover and readable detail screenshots were inspected at 320×240.
The screenshots demonstrate the implemented event surfaces, not whole-game
visual parity. Source, pack, binary, journal and private screenshot hashes are
recorded in [validation-event-tooltips-0.17.json](validation-event-tooltips-0.17.json).
All five configure/build/test journals closed with exit code 0 and unchanged
source.

The earlier failures remain in that receipt. At `573cb816`, the synthetic
Slippery Bridge fixture had an energy offset without its required Tezcatara's
Ember enchantment. The corrected fixture retains the save-byte assertion.
At `6d2b78ac`, the full potion catalog exposed `DARK` instead of `DARK_ORB`.
The canonical key was corrected; unresolved-title/body assertions were kept.
