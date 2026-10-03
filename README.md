# Slay the Nspire 2

Experimental native TI-Nspire Ndless adaptation of a bounded **Slay the Spire 2 v0.111.0 / 41cef1ea** rule and content subset. The imported prototype is named nSpire II 0.2.

## Import status

The 0.2 C++ core, ARM/WASM/X11 backends and conversion sources are being preserved here before further development. **The repository import is not complete:** the upload tool blocked `tools/setup_pose.py`; that file is retained in the original `nSpire2-0.2-source.zip` delivered to the owner. Consequently, **content conversion from this checkout will fail until the missing helper is restored through an authorized upload**. Native compilation from the checked-in C++ sources does not use this helper. Do not report this as a complete source import.

No original game data, assemblies, PCK files, fonts, calculator ROM/flash images, Ndless installers or generated content packs are tracked.

## Current scope

- One character, 14 card definitions and upgrades, one enemy type, bounded combat resolution with resumable exhaust selection.
- Custom twelve-floor, three-column expedition with shop, camp, treasure and three adapted events. This is **not the original map or seed behavior**.
- Three hardcoded optional relics and two potion slots. A complete relic subsystem is not yet present.
- Original art and offline-sampled animation supplied by a separately generated private content pack. No audio.
- Same C++ game core on ARM926 Ndless, offline WebAssembly and Linux X11.
- **Firebird and physical-device execution have not been verified.** Successful compilation is not a launch test.

## Build

The launcher uses LLVM and a freestanding Ndless ABI adapter, not a bundled SDK. With `clang++`, `ld.lld` and Python on PATH:

```sh
python3 tools/build_ndless.py
```

A Linux host can be compiled with a C++17 compiler and X11 development headers:

```sh
mkdir -p dist
c++ -std=c++17 -O2 -Iinclude src/core.cpp src/run.cpp src/content.cpp src/app.cpp src/host_x11.cpp -lX11 -o dist/nSpire2
```

After generating a private `dist/nSpire2.nsp.tns`, `python3 tools/build_web.py` creates the self-contained desktop HTML using `wasm-ld`. It embeds game-derived content and is not a source artifact for public redistribution.

The content converter requires Pillow, NumPy, a local Chinese-capable font, and the exact supported game installation. See the import warning above before attempting it:

```sh
python3 tools/build_content.py /path/to/game --font /path/to/local-font
```

## Controls

Directions or 4/6/8/2 navigate; Enter or 5 confirms; Space ends the turn; Tab inspects piles; L opens the log; M previews the map; P opens potions; S saves; O loads; Esc returns or opens the menu.

Place `nSpire2.tns` and `nSpire2.nsp.tns` beside each other on the calculator. The 0.2 save format is incompatible with 0.1. Use copies of calculator images when testing.

## License

The pre-existing repository MIT notice remains in `LICENSE` and is not an attempt to relicense the imported prototype. **Imported prototype source remains GPL-3.0-or-later**, as documented in `NOTICE.md`; the 0.2 distribution includes the GPL license text. The imported Ndless clock implementation follows GPL-licensed nZATO code. Game content and trademarks remain with their respective owners.
