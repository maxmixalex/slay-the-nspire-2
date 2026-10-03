# Slay the Nspire 2

A native TI-Nspire Ndless adaptation of **Slay the Spire 2**, targeting the closest practical reproduction of the original game. Screen layout and input may be adapted for 320×240; game rules and content must not be silently simplified to fit a prototype.

The current build is **nSpire II 0.4**. Its reference is the owner's **v0.111.0 / 41cef1ea** installation, pinned by assembly and PCK SHA-256 in `config/reference-release.json`. This is not yet a complete game port.

## Current build

One implemented character, 14 cards and upgrades, ten relics, three potions, a shop, camps and three adapted events. The twelve-floor expedition, encounter substitutions and RNG are still custom. **Original map/seed compatibility, the other characters, complete content and original UI parity remain open.** See the [fidelity contract and work plan](docs/parity.md).

Version 0.4 adds a relative **touchpad cursor**, physical clicks, short taps, card selection followed by enemy targeting, and drag-to-play. Keyboard input remains available. Existing menus, rooms, rewards, potions and relics share hit regions with their drawn controls. Pile inspection now uses a card grid with detail overlays; inspecting the draw pile never reveals its order. These screens are adaptations, not a claim of completed original UI reproduction.

The 0.4 ARM launcher was exercised in Firebird on CX II CAS OS 6.4.0.74 with Ndless r2022: touchpad navigation, targeting, drag, short-tap card details, save and return to the OS. A save retrieved through emulated USB also loaded in the desktop core. **Physical-handheld touchpad behavior has not been tested.**

## Build

Public source contains no original game data, assemblies, font files, calculator images, installers or generated content packs. The standalone ARM build needs Python, `clang++` with the ARM target and `ld.lld`:

```sh
python3 tools/build_ndless.py
```

The Linux X11 host uses CMake and X11 development headers:

```sh
cmake -S . -B build/host
cmake --build build/host
build/host/nSpire2 --pack dist/nSpire2.nsp.tns
```

Verify the reference and convert private content using the exact supported installation and a local Chinese-capable font:

```sh
python3 -m pip install -r requirements.txt
python3 tools/reference_inventory.py /path/to/game
python3 tools/build_content.py /path/to/game --font /path/to/local-font
python3 tools/build_web.py
```

The converter reads `release_info.json`, `SlayTheSpire2.pck` and `sts2.dll` (at the game root or inside `data_sts2_windows_x86_64`). It does not run the game. A different or modified build fails the reference check rather than being reported as compatible. The inventory counts metadata definitions, not obtainable content or implemented behavior.

`build_web.py` also needs `wasm-ld`. It creates an offline HTML host with the same C++ core and embedded game-derived content; that HTML is not a public source artifact.

## Controls and installation

Put `nSpire2.tns` and `nSpire2.nsp.tns` in the same calculator folder with Ndless r2022 installed. Use copies of calculator images for emulator testing.

Move a finger on the touchpad to move the cursor; lift and reposition to continue. Press the pad or tap briefly to click. Select an attack card and then its enemy, or drag the card onto the enemy. Non-targeted cards can be confirmed on the battlefield or clicked a second time. Dropping elsewhere cancels. Desktop mouse input follows the same interaction paths.

Directions or 4/6/8/2 navigate; Enter or 5 confirms; Space ends a turn; Tab opens piles; **I opens the selected card's details**; L opens the log; M the map; P potions; R relics; S saves; O loads; Esc returns or opens the menu. In events, up/down reads text and left/right selects an option. The top-right `...` also opens the menu.

Version 0.4 retains the **NSP2/3 content and version-3 save formats**. Existing 0.3 saves remain readable; 0.1/0.2 saves are not. Use the newly converted pack for all new interface glyphs. The browser keeps the existing `nSpire2-save-v3` key.

## Verification and development

[Development notes](docs/development.md) distinguish executed tests from outstanding acceptance work. [Map/seed notes](docs/map-seed.md) document the reference findings, not an implemented map generator.

```sh
python3 tools/build_firebird.py --source /path/to/firebird --gif-source /path/to/gif-h
```

This compiles the upstream headless emulator. Compilation alone is not an Ndless launch test.

## License

The runtime and converter remain **GPL-3.0-or-later** (`COPYING`). The pre-existing empty-repository MIT notice remains in `LICENSE`; it does not relicense the imported prototype. See [NOTICE.md](NOTICE.md). Original content and trademarks remain with their owners.
