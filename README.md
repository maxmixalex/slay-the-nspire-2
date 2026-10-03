# Slay the Nspire 2

Native TI-Nspire Ndless adaptation of **Slay the Spire 2**, targeting original-game fidelity with 320×240 layout and input adaptations. Current build: **nSpire II 0.6**. Reference: the owner's **v0.111.0 / 41cef1ea** installation, pinned by assembly and PCK hashes in `config/reference-release.json`.

## Current scope

The playable expedition still contains one character, 14 cards and upgrades, ten relics, three potions and three adapted events. Its twelve-floor map and encounter substitutions remain custom. This is not a complete game port.

Version 0.6 replaces fixed quotas in the independent **reference map preview** with the inspected regional quota generation. It supports Overgrowth, Underdocks, Hive and Glory; consumes quota draws before generating paths; and adds a custom seed editor and elite-quota switch. Software binary64 keeps this calculation independent of the target FPU/runtime. The sampler matches original-method replay on 20,016 tested inputs; see [map/seed scope](docs/map-seed.md) for the math-library and coverage boundaries.

**This is still not a playable original run or certified full PC-seed compatibility.** The preview does not resolve complete run/unlock/modifier state or bind original encounters. Unicode and legacy seeds remain unsupported. The existing expedition and its saves are unchanged.

## Build

No original game files, assemblies, font files, calculator images or generated content packs are tracked. Build the ARM launcher with Python, `clang++` and `ld.lld`:

```sh
python3 tools/build_ndless.py
```

Build the Linux X11 host with a C++17 compiler and X11 headers:

```sh
cmake -S . -B build/host
cmake --build build/host
build/host/nSpire2 --pack dist/nSpire2.nsp.tns
```

Generate private content from the exact reference installation and a local Chinese-capable font:

```sh
python3 -m pip install -r requirements.txt
python3 tools/reference_inventory.py /path/to/game
python3 tools/build_content.py /path/to/game --font /path/to/local-font
python3 tools/build_web.py
```

`build_web.py` also needs `wasm-ld`; its offline HTML embeds game-derived assets and is not a public source artifact. `tools/refresh_glyphs.py` can update interface glyphs in an existing private NSP2/3 pack without resampling images. The delivered 0.6 pack was produced this way.

## Installation and controls

Place `nSpire2.tns` and `nSpire2.nsp.tns` together with Ndless r2022 installed. Version 0.6 retains version-3 saves; 0.3–0.5 saves remain readable. Back up saves before replacing files, and use the new pack for added interface glyphs.

Open **参考地图** from the title menu or press **G**. Click the seed field to edit it; the calculator's on-screen keyboard supports touchpad clicks or directional selection and Enter. Browser/X11 hosts also accept typing. Generate applies ASCII normalization; empty, overlong, Unicode and raw lowercase `old` legacy seeds are rejected. Click the region button to cycle through four models and the elite button to switch quotas 5/8. Up/down or the on-screen arrows scroll, nodes show details, and Esc returns. This view remains isolated from gameplay.

Directions or 4/6/8/2 navigate; Enter or 5 confirms; Space ends a turn; Tab opens piles; I opens card details; L opens the log; M previews the playable map; P opens potions; R opens relics; S saves; O loads; Esc returns or opens the menu. Touchpad motion, short taps, physical clicks and drag targeting share the desktop mouse dispatcher. Physical touchpad calibration remains outstanding.

## Validation and limits

The final 0.6 ARM launcher ran in Firebird using a copy of the owner's CX II CAS image and Ndless r2022. Touchpad seed entry, regional maps, elite quotas and the legacy suspended-choice save/load were exercised. Six map captures matched desktop RGB565 output exactly. After normal exit to the OS, a USB-retrieved save passed the desktop codec.

This is emulator validation, not physical-device certification. [Development notes](docs/development.md) distinguish original-CIL comparisons from internal tests and browser/storage limitations. [Fidelity contract](docs/parity.md) tracks the wider objective; [recovery notes](docs/recovery.md) document the earlier unavailable artifacts.

## License

Runtime and converter: **GPL-3.0-or-later** (`COPYING`). The software-math adaptations retain their Go BSD, Sun and Cephes notices; see `licenses/Go-BSD-3-Clause.txt` and `NOTICE.md`. The original empty-repository MIT notice remains in `LICENSE` and does not relicense the imported code. Game content and trademarks remain with their owners.
