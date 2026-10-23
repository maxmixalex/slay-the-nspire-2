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
- StaticHoverTip.Transform is enum value 4; it must not be confused with Upgrade.

The fixed reference assembly SHA-256 is
`0861bfa1df347538d932f22d580e75420f08082792eb914e53b4882764acdbe9`.
Private disassembly is inspection input only and is not checked in.

## Validation status

New keyword tooltip regression cases cover explicit associated/empty options,
dynamic prepared cards, potion/relic/enchantment text, pointer and keyboard
ownership, modal suppression, paging, state/save-byte stability and real-pack
screenshots. Short syntax validation passed. Formal runtime receipts belong in
the final work-state update after the joint checkpoint is persisted.
