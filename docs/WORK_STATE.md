# Work state — 2026-10-15

## Branches and authoritative inputs

`checkpoint/bulk-20261015` preserves the interrupted bulk source plus RECOVERY.md. Code checkpoint: `1571b77275480316eb00a9f65bc040e0e0fdbeff`; handoff commit: `20d788a94b58c478190079c731f19ea05fb49807`. It is incomplete and NOT a release. The 112/116/55 declarations must not be presented as verified playable-content totals.

`work/0.13-persisted` is the working recovery branch. Runtime, core, room tests and converter/browser restoration have been committed in stages; `376a82d9599c5566d31d35d6c31512f8f763266b` includes the complete conversion chain. The original main at recovery start is `89a18a55bc330ab1bcd7b8dc9054c5e5984cd5ed`. Do not force-push either branch.

## Source recovery

The intact local source archive `/mnt/data/nSpire2-0.13-source.zip` has SHA-256 `7627a99a52c171b81b50a555562656bb5b617a0c3a3e4215583a9ad8f41cfeeb`; all entries in its source manifest matched. The work directory is `/mnt/data/persistence/work`. Runtime core is split into four include units whose bytes exactly match consecutive slices of the archived core.cpp. The optional X11 host retains the prior window-title string; gameplay is unchanged by that metadata.

Recovered content remains 68 card models, 30 relics, 16 potions, five events and two incomplete character pools. New source requires NSP2/10 and writes project save v10, reading v3-v9. The existing private 0.13 pack is 6,057,527 bytes. It must not be uploaded to public source.

## Current stage

Source is persisted before compiling. No new build or test success is claimed yet. `tools/run_logged.py` and its four state-transition tests are now included. Next: verify remote source subtree hashes, run the journal tests, build the native sanitizer targets, run CTest and ARM/WASM builds. Record actual results before advancing main.

Then continue UI fidelity work on this usable baseline. Keep the interrupted bulk branch intact until its executor, full source integration, migration, profiles and tests are genuinely completed. Do not overwrite a working core with larger declarations alone.

Remote CI stays disabled. No new Firebird, physical-device, original-PC differential, fresh asset conversion or sustained frame-rate test has been performed during recovery.
