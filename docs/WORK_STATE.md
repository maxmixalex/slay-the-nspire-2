# Work state — 2026-10-18

## Recovery receipt and active work

The accessible scratch workspaces no longer contained the previous checkout. All 163 source files were restored from the current work-branch head `50a58c9b876c3038e549b97e7e7d245224d717d8`, with every Git blob checked and the complete tree verified as `2fb04a1bb26b50d96d6741cbe2cba17f4a9366cc`. The original commit and shallow local Git checkout were reconstructed as well. Current directory: `/workspace/scratch/5af9d4bf40b9/slay-the-nspire-2`. A source-only baseline archive and isolated test copy are retained locally.

The user again prioritizes GitHub persistence, then completion of potions and single-player colorless cards, with parallel agents. Work is split between potion behavior, colorless behavior, and UI/acquisition regression coverage. The integration owner handles shared state, save compatibility, build wiring and checkpoints. Baseline validation has not yet run in this session. Historical success counts below must not be treated as fresh validation.

Next: run the isolated recovered baseline, repair stale profile/save/practice expectations without changing historical fixture bytes, and verify persistent/queued/automatic effects rather than counting dispatch-only smoke tests. Persist every coherent stage before long validation. Reference assets must be recovered and their locked hashes checked before any original-game parity or production-art claim.

Continue on `work/0.14-colorless-potions`. Do NOT restart the earlier bulk relic recovery. Main remains `efc50bc0b0cc6ca8e83461a2ff394cc7a56895e0` (tested 0.13); CI remains disabled. Reference v0.111.0 / 41cef1ea. Audio, multiplayer and Timeline remain excluded.

## Current source stage

The extended engine has 53 single-player colorless card models, all 64 active potion models, and generated Soul/SovereignBlade tokens; 114 card IDs total, 30 relics and two incomplete characters. These are registry/implementation counts, not new successful test claims. Current source writes save v11 and requires NSP2/11 (114 cards, 407 strings, 185 auxiliary images).

The renderer/description mismatch is corrected. ExtendedChoice is displayed over combat; three-card offers use explicit model tokens, pile choices use instances. Hand and detail costs respect temporary/free overrides, and Blade descriptions use the selected instance. Practice compendium details now expose card/potion acquisition buttons; title, paused and non-practice runs cannot use those actions. Shop and victory potions now use the existing character-scoped pool helper instead of selecting every registered potion.

Source checkpoint `1a131ac64da005f81ae3437306d1e1c1baf090ef` preserved the renderer/input stage and was read back. The subsequent stage includes the lab buttons/guards and acquisition pools. No compilation has yet been performed in this session. First verify the work-branch ref and local input hashes, then run logged builds and per-model tests. Migrate the older regression expectations for save v11 and the enlarged practice pool without changing historical fixture bytes.

## Actual environment and assets

Container execution initially returned InvalidArgumentError but recovered. Current local working directory is `/mnt/data/nspire14`, reconstructed from the intact 0.13 source archive and actual fetched 0.14 sources. Core-model file Git blob hashes have been checked against the fetched branch; remaining build/test/metadata files still need reconciliation.

The retained private resource pack is the OLD NSP2/10, 6,057,527 bytes. It MUST NOT be distributed with the new launcher or claimed to exercise new art. The original game's currently accessible second split ZIP volume is truncated (130,023,424 bytes instead of 419,430,400). The PCK crosses the missing region. No complete verified original PCK or new NSP2/11 production pack is currently available. Do not weaken the release hash check, fabricate missing portraits, or call a synthetic test pack original-game content.

Next: finish source-input reconciliation; run C++ sanitizer regressions, repair failures, and persist results. Compile ARM/WASM independently of the missing art. Use synthetic profiles only for parser/UI integration tests, explicitly distinguished from production visuals. No new Firebird, physical-device, fresh verified content conversion or PC-parity result has been obtained.
