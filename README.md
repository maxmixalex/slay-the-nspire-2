# Slay the Nspire 2

Experimental native TI-Nspire Ndless adaptation of **Slay the Spire 2 v0.111.0 / 41cef1ea**. The current prototype is nSpire II **0.3**.

## Implemented

One character, 14 cards and upgrades, resumable combat choices, ten functional relics, three potions, a shop, camps and three adapted events. The twelve-floor expedition and its random generator are still custom; **original maps, PC seed compatibility and complete game content are not implemented**.

The relic inventory preserves acquisition order and counters. Press **R** to inspect it. Supported relics: Burning Blood, Vajra, Anchor, Bag of Preparation, Lantern, Oddly Smooth Stone, Blood Vial, Happy Flower, Orichalcum and Pen Nib. Shops and treasure use the shared acquisition path and exclude already-owned relics.

Event choices use full-width rows with separate effect text. Event descriptions retain nested colors, bold text, sine and jitter effects. Eleven original animation clips are sampled at approximately 24 fps offline. Sparse RGB565 scanlines skip transparent pixels; presentation timing is independent of input and uses elapsed time on each backend. There is no audio.

**The 0.3 ARM launcher was launched inside Firebird with CX II CAS OS 6.4.0.74 and Ndless r2022.** Title, combat, relic inspection, suspended-choice save/load, return to the OS and subsequent USB save retrieval were exercised. This is emulator validation, not physical-handheld certification or an original-game parity guarantee.

## Build

No game files, assemblies, fonts, ROMs, flash images, installers or generated packs are tracked. The complete converter helpers are present in this checkout.

The launcher uses LLVM and a freestanding Ndless ABI adapter:

```sh
python3 tools/build_ndless.py
```

Requirements: Python, `clang++` with the ARM target, and `ld.lld`. For the desktop X11 host:

```sh
cmake -S . -B build/host
cmake --build build/host
# After content conversion:
build/host/nSpire2 --pack dist/nSpire2.nsp.tns
```

Create a private content pack from the exact supported installation, using a local Chinese-capable font:

```sh
python3 -m pip install -r requirements.txt
python3 tools/build_content.py /path/to/game --font /path/to/local-font
python3 tools/build_web.py
```

The converter reads `SlayTheSpire2.pck` and `release_info.json`; it does not execute the game. `build_web.py` needs `wasm-ld` and creates a self-contained desktop HTML using the same C++ core. That HTML embeds game-derived resources and is not a public source artifact.

The content format is NSP2/3. Version 0.3 cannot read 0.1/0.2 saves or older content packs. Use the launcher and content pack from the same build.

## Run

Put `nSpire2.tns` and `nSpire2.nsp.tns` in the same calculator folder, with Ndless r2022 installed. The save is written beside them. Test with copies of calculator images; do not use your only flash dump as an emulator working image.

Directions or 4/6/8/2 navigate; Enter or 5 confirms; Space ends the turn; Tab inspects piles; L opens the log; M previews the map; P opens potions; **R opens relics**; S saves; O loads; Esc returns or opens the menu. In events, up/down scroll text and left/right select the choice.

To compile the supplied Firebird source and gif-h submodule offline:

```sh
python3 tools/build_firebird.py --source /path/to/firebird --gif-source /path/to/gif-h
```

This builds the upstream headless target. It is not by itself a game launch test. See [development and verification](docs/development.md) for the exercised environment and remaining work, and [map/seed compatibility](docs/map-seed.md) for the verified original-code findings.

## License

The imported runtime and converter remain **GPL-3.0-or-later**, with the full license in `COPYING`. The pre-existing empty-repository MIT notice is preserved in `LICENSE`; it does not relicense the imported prototype. See [NOTICE.md](NOTICE.md) for attribution. Game assets and trademarks remain with their respective owners.
