# 0.12 source recovery: executed evidence

The published baseline was `1667fb2cdeb8e7260f0327571ab534e8450c72a9` (0.11 plus disabled CI). The previously unreferenced recovery commit was `22b9daab5ca998c6dbf43200b81c7500fadac0c3`. A checkpoint branch and draft PR were created before further changes. The recovered C++ source and include subtrees were reconstructed from the retained 0.11 archive and connector-read changes; their Git tree hashes exactly match `d5d5e619e201ea911172bc7a59b7298ffd6e85d5` and `ef475b968d72f00826ff648804f72959330f47e3`.

Completed missing publication pieces: `tools/build_relics.py`, its final-stage call in `tools/build_content.py`, 0.12 Zehn metadata, and the current-save-version assertion in `tests/tactics_regression.cpp`. Restored historical regression changes without deleting failing tests. Source excludes all private game data, calculator images and fonts.

## Checks executed during this recovery

All fourteen CTest targets passed under AddressSanitizer and UndefinedBehaviorSanitizer. The final verbose run includes 15,443 relic-combination checks / 2,873 controlled transitions and 221 asset-free relic UI checks. Invalid actions, pending choices, persisted counters, treasure reroll/duplicate prevention and Old Coin at the gold cap are covered.

The ARM launcher rebuilt to 121,354 bytes with 8,068,044 bytes reported static loader allocation and 1,500 absolute relocations. WASM rebuilt to 413,363 bytes. Static allocation is not a measured peak-RAM result.

The provided installation was re-extracted locally and passed the existing assembly/PCK SHA-256 lock. Incremental conversion from the retained 0.11 private pack produced NSP2/9: 5,687,197 bytes, 1,324 images, 3,836 glyphs, 68 cards and 17 clips. Running the same converter on that output produced identical bytes. No animation resampling or frame deletion was performed.

Asset-aware tests passed: content 535 checks; shared UI 106; tactics UI 353; relic UI 223. The generated browser host embeds the rebuilt WASM and private pack. Browser DOM/persistent-storage and Firebird tests were not rerun at this checkpoint.

## Evidence boundary

Earlier staged prose contained a completed 0.12 Firebird report. Its private captures/drivers and 0.12 output package were not present in this workspace, so those claims are not presented as newly verified results. The retained 0.11 reports remain historical evidence only. No full original-game combat differential run, full fresh skeleton bake, sustained handheld frame-rate test or peak-RAM measurement was executed in this recovery.

Remote CI stays disabled. The playable expedition remains custom; additional characters, original encounter/event pools and original seeded run integration are still missing. Audio, multiplayer and Timeline remain explicitly excluded.
