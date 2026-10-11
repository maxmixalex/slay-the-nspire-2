# Slay the Nspire 2

Native TI-Nspire Ndless adaptation of Slay the Spire 2, pursuing the original single-player rules and visual identity with 320x240 layout and input adaptations. Current source: **0.12 prototype**. Reference remains the owner's hash-pinned **v0.111.0 / 41cef1ea** installation.

**Audio, multiplayer and Timeline progression are excluded, not deferred.** Animation remains in scope. Original assets, assemblies, calculator images and installable fonts are not in this source repository.

## Current content

68 card models (29 Ironclad, 29 Silent, nine colorless, one generated Shiv), 30 relics and 16 potions. Both playable character pools remain incomplete. The expedition is still a custom twelve-floor route with bounded Nibbit encounters and three adapted events. The reference regional map remains separate from the playable route.

This release completes Kunai, Shuriken, Ornamental Fan, Letter Opener and Nunchaku, with once-per-completed-card counters, suspended-choice handling and persistence. Owned relic icons open their own details; relic inventory is paged and displays counters. Treasure has separate open, inspect, claim and proceed/skip steps; saving and loading cannot reroll or duplicate a revealed reward. See `docs/relics-0.12.md` and the existing reference audits for rule boundaries.

## Local build

Remote CI is disabled by request. Its definition stays in `.github/disabled-workflows/ci.yml`, outside workflow discovery. Do not re-enable without an explicit request.

```sh
python3 tools/build_ndless.py
cmake -S . -B build/host -DSPIRE_X11=OFF
cmake --build build/host --parallel
ctest --test-dir build/host --output-on-failure
```

LLVM clang++ and ld.lld are required for ARM. Enable `SPIRE_X11` for the Linux desktop host. Fourteen source-only regression targets are included.

Incremental private content conversion:

```sh
python3 -m pip install -r requirements.txt
python3 tools/build_relics.py /path/to/game /path/to/0.11/nSpire2.nsp.tns --output dist/nSpire2.nsp.tns --font /path/to/local-font
python3 tools/build_web.py
```

The converter accepts NSP2/8 or NSP2/9 and preserves existing payloads. The full `build_content.py` pipeline is wired to the same final stage; a full fresh skeleton bake was not executed during this recovery.

## Installation and controls

Put **both** `nSpire2.tns` and `nSpire2.nsp.tns` in the same calculator folder with Ndless r2022 installed. Back up saves first. This launcher needs NSP2/9 (68 cards, 95 auxiliary images); matching the card count alone is insufficient. New saves use v9; project v3-v8 saves remain readable. Old launchers cannot read v9. Browser storage uses an independent v9 key.

Directions or 4/6/8/2 navigate; Enter or 5 confirms; Space ends a turn (or confirms a pending card selection); Tab piles; I details; C compendium; V statuses; L log; M playable map; G reference-map preview; P potions; R relics; S save; O load; Esc return/pause. Touchpad taps, clicks and dragging share the native dispatcher.

## Evidence

The recovered source was rebuilt locally: all 14 ASan/UBSan CTest targets passed, ARM and WASM compiled, the private pack was regenerated and four asset-aware tests passed. Repeating the incremental conversion produced identical bytes. These are current local results, not a new Firebird or physical-device certification. Historical emulator reports do not establish that the newly rebuilt converter output has been exercised on hardware. See `docs/development.md`.

Complete original runs, remaining characters/encounters, full combat differential parity, sustained handheld performance and peak runtime RAM remain unfinished or unverified. UI fidelity is still an active development task, not claimed complete.

## License

Project code: GPL-3.0-or-later (`COPYING`). Keep Go BSD, Sun and Cephes notices in `NOTICE.md` and `licenses/`. Original content and trademarks remain with their owners; code licensing grants no additional asset redistribution rights.
