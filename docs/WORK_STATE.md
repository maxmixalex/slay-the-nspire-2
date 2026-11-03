# Maintainer notes

Read [AGENTS.md](../AGENTS.md) for checkpoint and device constraints;
[DEVELOPMENT.md](../DEVELOPMENT.md) contains build commands and target budgets.
The original game reference remains **v0.111.0 / 41cef1ea**.

## Current behavior

- Touchpad center presses click the visible pointer or confirm keyboard focus.
  Tap to Click and Mouse Speed are saved per profile; defaults are off and 100%.
- Character access is independent of Timeline. New profiles start with Ironclad
  and unlock the other characters through completed standard runs. The Timeline
  interface and its gameplay gates have been removed.
- The new-profile tutorial follows Neow, map selection, combat and rewards.
  Older saves retain their progress and previously available characters.
- Windows and Linux standalone converters are built by `converters.yml`.
  Player instructions are in the bilingual installation guides.

## Maintenance

Keep `state/` when updating or changing animation editions. Replace the launcher
and complete `data/` together, including optional locales. Resource formats,
cache ownership and source provenance are documented in
[resource-format.md](resource-format.md) and [porting.md](porting.md).

Verify changed input and progression through normal controls, save/relaunch
and OS exit. Use ordinary 64 MiB Firebird/OS/Ndless startup with the existing
allocation limits. Native tests, emulator execution and physical handheld
measurements are separate. Manual browser validation still needs a renewed request.

Original assets, firmware, converted packs and raw captures remain private.
Keep current instructions and reusable technical knowledge here; historical
reports and generated receipts remain in Git history or external build records.
