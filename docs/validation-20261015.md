# Persistence-first recovery validation — 2026-10-15

Source checkpoint: `5e739910fe2487eb6c239e8908de4a26872c1bc9`, read back from `work/0.13-persisted` BEFORE compilation. This report concerns the recovered 0.13 baseline, not the unfinished bulk registry.

## Source equality

Local compiler/test input subtrees matched the remote checkpoint:

| Subtree | Git tree SHA |
|---|---|
| src | 42db66dc5dfcdfcce750ac8387647d37e7527228 |
| include | 3791dabcdcea09af2c8abbc9b8cc6701d2414a62 |
| tests | 43d1218d8811854a65eea62afe89c815a11d47c4 |
| tools | 1c0ac86bb5e5d016c7a39d9584952f0524ffbbf8 |
| platform | d4f68b04f41d92cd9bb374df0182db53b1c3692f |

CMakeLists.txt: `8a5b6ba62232bee25ac01298fba18cad8949f6c1`. Historical documentation outside these inputs differs from the compact original source archive and was not represented as a full-tree match.

## Executed commands

Every command below ran through `tools/run_logged.py` with the source checkpoint above. All returned zero; every journal reports `source_changed: false`.

- `python3 tests/logged_command_regression.py`: four tests passed, covering a running receipt before child execution, failed exit code, timeout, and changed-source rejection.
- CMake configured with Clang, ASan/UBSan, Debug and SPIRE_X11=OFF; `cmake --build build/host --parallel 4` passed.
- `ctest --test-dir build/host --output-on-failure`: all 17 tests passed.
- `python3 tests/event_pack_regression.py`: six tests passed.
- `python3 tools/build_ndless.py`: ARM launcher compiled and packed.
- `python3 tools/build_web.py`: WASM and private embedded-content HTML built.

CTest log SHA-256: `39a5bdad9b4b82c2d8cffdb4a43686bb35dfe0947b58dfeec3a6883b7766bf79`.
ARM build log SHA-256: `4083ba9589c891167a693b5e52a5aca7732dc09f98906862795debf66f5cb56e`.
WASM build log SHA-256: `7104e452d218ecfddea59e5d49fa900ee92029d187664b665f239d7d08dff7c4`.

## Outputs

| Output | Bytes | SHA-256 |
|---|---:|---|
| nSpire2.tns | 120505 | 2cef028366a77c1bc92884a5e4335284199377f681f5757a9291adab83510897 |
| nSpire2.wasm | 410262 | 809972b638fc5485bd447cad77a07fae6710e60e1aef146417f29a2b3c888e09 |
| retained private NSP2/10 pack | 6057527 | e157ee346a4eb1fc2849bc2d18e5dc1f4ccf364b85386d92dc7306a1430908a5 |

The ARM packer reports 8,341,736 bytes static loader allocation and 1,543 relocations. Static allocation is not measured peak runtime RAM. The private content pack was reused, not freshly converted.

## Limits

No new Firebird, physical-device, original-game differential, fresh asset-conversion or browser DOM/persistent-storage test is claimed. This recovery does not increase completed content beyond 68 card models, 30 relics, 16 potions and five events. Remote CI remains disabled. The incomplete 112/116/55 registry is separately checkpointed and is not a release.
