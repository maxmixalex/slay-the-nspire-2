# Work state — 2026-10-15

## Durable source, not conversational memory

`checkpoint/bulk-20261015` preserves the interrupted bulk fragments and RECOVERY.md. Code checkpoint: `1571b77275480316eb00a9f65bc040e0e0fdbeff`; handoff commit: `20d788a94b58c478190079c731f19ea05fb49807`. This branch is incomplete, NOT a release. The 112/116/55 declarations are not verified playable-content totals. Its executor/core integration, migration, assets/profile and tests are unfinished.

`work/0.13-persisted` contains the recovered usable baseline. Source checkpoint `5e739910fe2487eb6c239e8908de4a26872c1bc9` was pushed and its ref read back before all validation below. The runtime/input/test/tool/platform subtrees were compared against local files. Source provenance and commands are in `validation-20261015.md`.

The original main at recovery start was `89a18a55bc330ab1bcd7b8dc9054c5e5984cd5ed`. It can now be fast-forwarded to this tested recovery branch; check its actual current ref before doing so. Do not force-push.

## Executed validation

All 17 C++ CTest targets passed with ASan/UBSan. All four journal-runner tests and six event-converter/configuration tests passed. ARM and WASM compiled successfully. All journaled commands returned zero and reported unchanged source inputs.

Actual NSP2/10 pack tests also passed: content 535; shared UI 106; tactics UI 363; relic UI 223; combat UI 815; event/shop UI 388 checks.

Four room sequences were rerun through the native driver and final WASM: Aroma transform, Silent Aroma upgrade, Doors removal and shop purchase/removal. All 84 full RGB565 frames and eight saves matched. This uses explicit storage callbacks, not browser DOM or persistent localStorage sessions, and is not original-PC parity.

ARM launcher: 120,505 bytes, SHA-256 `2cef028366a77c1bc92884a5e4335284199377f681f5757a9291adab83510897`; reported static loader allocation 8,341,736 bytes, not measured peak RAM. WASM: 410,262 bytes. Existing private pack: 6,057,527 bytes, SHA-256 `e157ee346a4eb1fc2849bc2d18e5dc1f4ccf364b85386d92dc7306a1430908a5`.

## Next source stage

Fix hand presentation so a focused card is raised only while deliberately selected/hovered; neutral pointer position should not leave an oversized card blocking the battlefield. Preserve the ten-card stable lower rail, instance-based drag capture, keyboard targeting and modal isolation. Add explicit regression cases. Persist this source stage and read back the remote ref BEFORE compiling it.

Content baseline remains 68 card models, 30 relics, 16 potions, five events and two incomplete characters. Requires NSP2/10; writes project save v10 and reads v3-v9. Recovering old code is not new content completion.

## Recovery inputs and boundaries

Intact source archive: `/mnt/data/nSpire2-0.13-source.zip`, SHA-256 `7627a99a52c171b81b50a555562656bb5b617a0c3a3e4215583a9ad8f41cfeeb`. Working directory: `/mnt/data/persistence/work`. All source manifest hashes matched. Four split core units match the archived monolithic core byte for byte. Optional X11 window title still says 0.12; this metadata does not affect the tested runtime.

Keep original reference v0.111.0 / 41cef1ea. Audio, multiplayer and Timeline are excluded. Remote CI remains disabled. No new Firebird, physical-device, original-game differential, fresh asset-conversion or sustained frame-rate test has been run. Private game data, packs, fonts, calculator images and embedded-content HTML must not enter this public repository.
