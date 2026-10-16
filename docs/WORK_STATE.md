# Work state — 2026-10-16

Continue on `work/0.14-colorless-potions`. Do NOT restart the earlier bulk relic recovery. Main remains `efc50bc0b0cc6ca8e83461a2ff394cc7a56895e0` (tested 0.13); CI remains disabled. Reference v0.111.0 / 41cef1ea. Audio, multiplayer and Timeline remain excluded.

## Current source stage

The extended engine has 53 single-player colorless card models, all 64 active potion models, and generated Soul/SovereignBlade tokens; 114 card IDs total, 30 relics and two incomplete characters. These are registry/implementation counts, not new successful test claims. Current source writes save v11 and requires NSP2/11 (114 cards, 407 strings, 185 auxiliary images).

The renderer/description mismatch is corrected. ExtendedChoice is displayed over combat; three-card offers use explicit model tokens, pile choices use instances. Hand and detail costs respect temporary/free overrides, and Blade descriptions use the selected instance. Practice compendium details now expose card/potion acquisition buttons; title, paused and non-practice runs cannot use those actions. Shop and victory potions now use the existing character-scoped pool helper instead of selecting every registered potion.

Source checkpoint `1a131ac64da005f81ae3437306d1e1c1baf090ef` preserved the renderer/input stage and was read back. The subsequent stage includes the lab buttons/guards and acquisition pools. No compilation has yet been performed in this session. First verify the work-branch ref and local input hashes, then run logged builds and per-model tests. Migrate the older regression expectations for save v11 and the enlarged practice pool without changing historical fixture bytes.

## Actual environment and assets

Container execution initially returned InvalidArgumentError but recovered. Current local working directory is `/mnt/data/nspire14`, reconstructed from the intact 0.13 source archive and actual fetched 0.14 sources. Core-model file Git blob hashes have been checked against the fetched branch; remaining build/test/metadata files still need reconciliation.

The retained private resource pack is the OLD NSP2/10, 6,057,527 bytes. It MUST NOT be distributed with the new launcher or claimed to exercise new art. The original game's currently accessible second split ZIP volume is truncated (130,023,424 bytes instead of 419,430,400). The PCK crosses the missing region. No complete verified original PCK or new NSP2/11 production pack is currently available. Do not weaken the release hash check, fabricate missing portraits, or call a synthetic test pack original-game content.

Next: finish source-input reconciliation; run C++ sanitizer regressions, repair failures, and persist results. Compile ARM/WASM independently of the missing art. Use synthetic profiles only for parser/UI integration tests, explicitly distinguished from production visuals. No new Firebird, physical-device, fresh verified content conversion or PC-parity result has been obtained.
