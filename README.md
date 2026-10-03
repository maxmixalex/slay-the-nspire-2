# Slay the Nspire 2

Native TI-Nspire Ndless adaptation of **Slay the Spire 2**, targeting original-game fidelity with 320×240 layout and input adaptations. Current build: **nSpire II 0.5**. Reference: the owner's **v0.111.0 / 41cef1ea** installation, pinned by assembly and PCK hashes in `config/reference-release.json`.

## Current scope

The playable expedition still contains one character, 14 cards and upgrades, ten relics, three potions and three adapted events. Its twelve-floor map and encounter substitutions remain custom. This is not a complete game port.

Version 0.5 adds an independent **seven-column reference map preview**. It ports the original explicit-room-quota constructor: path generation, room assignment, duplicate-path pruning, quota repair and layout processing. Ten seed/configuration cases matched restricted replay of the original CIL, including ordered edges, room types and final RNG state.

**The preview is not full PC seed compatibility.** Original Gaussian room-quota generation, legacy seed handling and the complete act/run pipeline remain unfinished. The preview uses fixed quotas and does not replace the playable expedition or alter its saves. See [map/seed scope](docs/map-seed.md).

Pointer paging now advances by complete ten-card pages. Scrolling cancels a held click; held menu controls retain their capture. Existing touchpad/mouse targeting, drag-to-play and card details remain available.

## Build

No original game files, assemblies, fonts, calculator images or generated content packs are tracked. Build the ARM launcher with Python, `clang++` and `ld.lld`:

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

`build_web.py` additionally needs `wasm-ld`; its offline HTML embeds game-derived assets and is not a public source artifact. During interface-only development, `tools/refresh_glyphs.py` can refresh glyphs in an existing private NSP2/3 pack without resampling animation images.

## Installation and controls

Place `nSpire2.tns` and `nSpire2.nsp.tns` together with Ndless r2022 installed. Version 0.5 retains version-3 saves; 0.3/0.4 saves remain readable. Back up saves before replacing files, and use the new content pack for the added glyphs.

The title menu's **参考地图**, or **G**, opens the reference preview. Up/down scrolls, left/right selects a neighboring demonstration seed, clicking a node shows its details, and Esc closes the preview. Seed entry and the original default quota distribution are not implemented.

Directions or 4/6/8/2 navigate; Enter or 5 confirms; Space ends a turn; Tab opens piles; I opens card details; L opens the log; M previews the playable map; P opens potions; R opens relics; S saves; O loads; Esc returns or opens the menu. Touchpad motion, short taps, physical clicks and drag targeting share the desktop mouse dispatcher. Physical touchpad calibration remains outstanding.

## Validation and limits

The final 0.5 ARM launcher ran in Firebird with the owner's CX II CAS working-image copy and Ndless r2022. Reference map generation, changing seeds, scrolling, return to the legacy game, suspended-choice save/load and normal OS exit were exercised. The retrieved USB save passed the desktop codec. The initial reference map matched the desktop RGB565 framebuffer exactly.

This is emulator validation, not physical-device certification. [Development notes](docs/development.md) separate original-CIL comparisons from internal stress tests and describe the remaining memory and browser limitations. [Fidelity contract](docs/parity.md) tracks the wider objective. [Recovery notes](docs/recovery.md) correct the unavailable earlier map-polish artifacts.

## License

Runtime and converter: **GPL-3.0-or-later** (`COPYING`). The original empty-repository MIT notice remains in `LICENSE` and does not relicense the imported code. See `NOTICE.md`. Game content and trademarks remain with their owners.
