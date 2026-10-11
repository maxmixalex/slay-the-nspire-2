# Slay the Nspire 2

Native TI-Nspire Ndless adaptation of Slay the Spire 2, pursuing original single-player rules and visual identity with 320x240 adaptations. Current source: **0.12, combat UI revision**. Reference: the owner's hash-pinned **v0.111.0 / 41cef1ea** installation.

**Audio, multiplayer and Timeline are excluded, not deferred.** Animation remains in scope. Original assets, assemblies, calculator images and installable fonts are not published in this repository.

## Current content and interface

**68 card models, 30 relics and 16 potions.** Cards comprise 29 Ironclad, 29 Silent, nine colorless and one generated Shiv. Both character pools are incomplete. The playable expedition is still a custom twelve-floor route with bounded Nibbit encounters and three adapted events; the reference regional map remains a separate preview.

The completed 0.12 content adds Kunai, Shuriken, Ornamental Fan, Letter Opener and Nunchaku, with once-per-completed-card counters, suspended choices and persistence. Owned relic icons open their own details. Treasure has separate open, inspect, claim and proceed/skip steps; saves cannot reroll or duplicate the revealed reward. See [relic rules](docs/relics-0.12.md).

The combat UI now uses a centered integer-rendered card fan and a raised, enlarged focused card. Titles move to the upper banner, descriptions onto the card; long text remains available through I. Energy is left, end turn right, and draw/discard piles occupy opposite bottom corners. Hero HP is shown beneath the actor. Stable lower hit regions keep all ten cards selectable without hover oscillation. This follows inspected scene structure with explicit small-screen adaptations, not exact PC layout or motion. See [layout reference and validation](docs/combat-ui-0.12.md).

## Build and local tests

Remote CI remains disabled in `.github/disabled-workflows/ci.yml`. Do not re-enable it without an explicit request.

```sh
python3 tools/build_ndless.py
cmake -S . -B build/host -DSPIRE_X11=OFF
cmake --build build/host --parallel
ctest --test-dir build/host --output-on-failure
```

LLVM clang++ and ld.lld are required for ARM. Enable SPIRE_X11 for the Linux desktop host. Fifteen source-only regression targets are included.

Incremental private content conversion:

```sh
python3 -m pip install -r requirements.txt
python3 tools/build_relics.py /path/to/game /path/to/0.11/nSpire2.nsp.tns --output dist/nSpire2.nsp.tns --font /path/to/local-font
python3 tools/build_web.py
```

The converter accepts NSP2/8 or NSP2/9 and preserves old payloads. The full build_content.py pipeline is wired to this final stage; a full fresh skeleton bake was not executed during recovery. No animation frames were deleted for the combat UI revision.

## Installation and controls

Put **both** nSpire2.tns and nSpire2.nsp.tns in the same calculator folder, with Ndless r2022 installed. Back up saves first. This launcher requires NSP2/9 (68 cards, 95 auxiliary images); matching card count alone is insufficient. New saves use v9; project v3-v8 saves remain readable. Old launchers cannot read v9. Browser storage uses an independent v9 key.

Directions or 4/6/8/2 navigate; Enter or 5 confirms; Space ends a turn (or confirms a pending card choice); Tab piles; I details; C compendium; V statuses; L log; M playable map; G reference-map preview; P potions; R relics; S save; O load; Esc return/pause. Touchpad taps, clicks and dragging share the native dispatcher. The bottom portions of fanned cards provide stable selection regions.

## Evidence and limits

All 15 ASan/UBSan CTest targets and five actual-pack test targets passed. ARM and WASM built successfully. Both characters' six native/WASM frame comparisons and post-play saves matched exactly. These are local and cross-backend results, not a new Firebird, browser DOM/persistent-storage or physical-device certification.

The pack is 5,687,197 bytes; the launcher is 118,154 bytes. Reported static Ndless allocation is 8,337,248 bytes, not measured peak RAM. The bounded browser runtime uses nine MiB including its pack buffer, caches and stack. Sustained handheld frame rate remains unmeasured.

Complete original runs, remaining characters/encounters, full original combat differential parity and other screens' fidelity remain unfinished. Current evidence is separated from older reports in [recovery notes](docs/development.md) and [combat UI notes](docs/combat-ui-0.12.md).

## License

Project code: GPL-3.0-or-later (COPYING). Keep Go BSD, Sun and Cephes notices in NOTICE.md and licenses/. Original content and trademarks remain with their owners; code licensing grants no additional asset redistribution rights.
