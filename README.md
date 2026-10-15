# Slay the Nspire 2

Native TI-Nspire Ndless adaptation of Slay the Spire 2. Reference: the owner's hash-pinned **v0.111.0 / 41cef1ea** installation. Current source: **0.13 recovery**.

**Audio, multiplayer and Timeline are excluded.** Original UI fidelity, animation and single-player content breadth remain in scope. Original game data, fonts and calculator images are not included in public source.

## Current implemented baseline

68 card models (29 Ironclad, 29 Silent, nine colorless, one generated Shiv), 30 relics, 16 potions and five adapted events. Both playable characters are incomplete subsets. The twelve-floor playable route and Nibbit encounters remain adapted; the reference regional map is still separate from a complete original run.

0.13 includes Aroma of Chaos and Doors of Light and Dark, pending event deck choices, outcome narratives, free event removal, full-text event reading, and separate merchant-room/rug interaction. It retains the fanned-hand combat UI and relic/treasure work. These layouts are adaptations, not complete original-PC fidelity.

Large bulk-expansion fragments are preserved on `checkpoint/bulk-20261015`, not counted as completed content. Read [current work state](docs/WORK_STATE.md) before resuming development.

## Build

Remote CI is disabled by request. Its definition stays in `.github/disabled-workflows/ci.yml`.

```sh
python3 tools/build_ndless.py
cmake -S . -B build/host -DSPIRE_X11=OFF
cmake --build build/host --parallel
ctest --test-dir build/host --output-on-failure
python3 tests/event_pack_regression.py
python3 tests/logged_command_regression.py
```

ARM requires LLVM clang++ and ld.lld. The X11 host is optional. Install converter dependencies from requirements.txt. Convert an existing private 0.12 NSP2/9 pack:

```sh
python3 tools/build_events.py /path/to/game /path/to/old/nSpire2.nsp.tns --output dist/nSpire2.nsp.tns --font /path/to/local-font
python3 tools/build_web.py
```

`build_content.py` also chains through this final conversion stage. New launchers require NSP2/10 (68 cards, 105 auxiliary images, 269 strings). Replace both launcher and pack, and back up saves. New saves use v10; project v3-v9 saves remain readable. Old launchers cannot read v10.

## Persistence and validation

Commit source to a named work branch and read the remote ref back **before** long compilation or conversion. `tools/run_logged.py` records the supplied checkpoint, input hashes, command, running/completed state, exit code and log hash. It does not itself verify or push a remote commit. A running journal after interruption is not a passed test.

Current execution results belong in WORK_STATE, not inherited historical reports. Compilation, cross-backend agreement and actual target execution are different evidence levels. Physical performance, remaining characters, complete original encounters/runs and exact UI parity remain unverified or unfinished.

## Controls and license

Directions navigate; Enter confirms; Space ends a turn or confirms a pending selection; Tab piles; I details; C compendium; V statuses; M map; P potions; R relics; S save; O load; Esc return/pause. Touchpad clicks and dragging share the native dispatcher.

Source: GPL-3.0-or-later (COPYING). Preserve NOTICE.md and licenses/. Original assets and trademarks remain with their owners; source licensing grants no additional asset redistribution rights.
