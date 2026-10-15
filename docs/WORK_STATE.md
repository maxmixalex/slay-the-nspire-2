# Work state — 2026-10-15

## Completed and durable

The complete 0.13 recovery reached main at `203c9dd004f3c674a8650b65d92a192b0469792f`, replacing the old 0.12 head without force-pushing. The next SOURCE checkpoint, `8d3738561ebf1919f24a43bbaec673bd13d9c697`, fixes always-raised hand cards. It was committed to `work/0.13-persisted` and its ref read back BEFORE building. Validation is recorded in `hand-focus-20261015.md`, committed at `6245f9c6dad6e44b4f3826dd8f46168872e96ab0`.

Hand focus now follows real hover/keyboard selection, preserves target navigation, collapses neutral/overlay states, and keeps stable instance-based selection/drag regions. All 17 ASan/UBSan CTest targets and six actual-pack targets passed. Combat UI: 1,218 checks. ARM and WASM compiled. Four native/WASM room sequences matched all 84 frames and eight saves. Every validation journal reported unchanged source inputs. Source files are preserved independently of this conversation.

Final launcher: 121,162 bytes, SHA-256 `1d29edf67de845e8387cc5181eb00e5c9f540f702ff676f4077cc0fb047036e3`. WASM: 411,727 bytes. The unchanged private pack is 6,057,527 bytes, SHA-256 `e157ee346a4eb1fc2849bc2d18e5dc1f4ccf364b85386d92dc7306a1430908a5`.

## Baseline and unfinished work

Playable baseline: 68 card models, 30 relics, 16 potions, five events, two incomplete characters. Requires NSP2/10; writes project save v10 and reads v3-v9. This recovery/focus revision does not add to those completed-content counts.

`checkpoint/bulk-20261015` preserves interrupted expansion fragments. Code commit: `1571b77275480316eb00a9f65bc040e0e0fdbeff`; recovery-notes commit: `20d788a94b58c478190079c731f19ea05fb49807`. It is NOT buildable. The 112/116/55 declarations are not implementation-completion evidence. Its continuation executor, core integration, historical migration, asset profile/converter and new regressions remain unfinished. Do not overwrite a working main with that incomplete tree.

Next: inspect the saved bulk branch and integrate coherent, buildable mechanisms against this persisted 0.13 baseline, with real per-model coverage. Keep original UI refinement in scope. Persist each source stage to a named branch, read the ref back, run logged validation, then record the result before the next stage. AGENTS.md and tools/run_logged.py define this cycle. Never re-create an intact source archive by manually guessing old file contents or by referencing unuploaded blob hashes.

## Recovery inputs and limits

Original intact source archive: `/mnt/data/nSpire2-0.13-source.zip`, SHA-256 `7627a99a52c171b81b50a555562656bb5b617a0c3a3e4215583a9ad8f41cfeeb`. Working directory during this session: `/mnt/data/persistence/work`. Local paths are convenience hints, not durable prerequisites; use the repository when starting a new environment.

Reference remains v0.111.0 / 41cef1ea. Audio, multiplayer and Timeline are excluded. Remote CI stays disabled. No new Firebird, physical-device, original-game differential, fresh asset-conversion, browser DOM/persistent-storage or sustained frame-rate test was run. Native/WASM agreement does not establish original-game parity. Public source excludes private assets, packs, embedded-content HTML, fonts and calculator images.
