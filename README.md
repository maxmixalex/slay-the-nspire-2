# Slay the Nspire 2

**nSpire II 0.11** is a native TI-Nspire Ndless adaptation of Slay the Spire 2. It pursues original single-player rules and visual identity, with 320x240 layout/input adaptations. The owner's **v0.111.0 / 41cef1ea** installation remains hash-pinned in `config/reference-release.json`.

**Audio, multiplayer and Timeline progression are excluded, not deferred.** Visual animation remains in scope. Public source does not contain original game assets, assemblies, calculator images or installable fonts.

## What is playable

**68 card models: 29 Ironclad, 29 Silent, nine colorless and one generated Shiv.** There are 25 relics, 16 potions and two playable character subsets. Both character pools remain incomplete. The playable expedition still uses a custom twelve-floor route, a bounded Nibbit encounter and three adapted events. The original regional-map subsystem is a separate preview, not a complete original seeded run.

Version 0.11 adds **Headbutt, Sword Boomerang, Perfected Strike, Flame Barrier, Demon Form, Dodge and Roll, Infinite Blades, Backstab, Escape Plan and Predator**. They enter their real character's implemented reward/shop pool and practice deck. Headbutt selects an actual discard-pile instance for the top of draw. The other additions cover random multi-hit attacks, Strike tags, retaliation, recurring strength/Shivs, Innate and next-turn block/draw. The pinned Demon Form grants 3/4 Strength; upgraded Infinite Blades adds Innate. See `config/tactics-audit-0.11.json` for inspected method identifiers and evidence limits.

The UI now supports paged discard-pile choices, including save/load while a choice is pending. A compact original-icon status strip opens a full **Powers and Statuses** view with actor tabs, amounts, rich descriptions and clickable page arrows. Permanent deck operations retain the explicit confirmations and upgrade comparisons introduced in 0.10. These are small-screen adaptations, not pixel-identical PC screens.

## Build and test

```sh
# Native runtime: LLVM clang++ and ld.lld. No SDK download at build time.
python3 tools/build_ndless.py

# Twelve asset-free regression targets; Linux X11 host is optional.
cmake -S . -B build/host -DSPIRE_X11=OFF
cmake --build build/host --parallel
ctest --test-dir build/host --output-on-failure
```

Leave `SPIRE_X11` enabled to build the Linux desktop host, then run `build/host/nSpire2 --pack dist/nSpire2.nsp.tns`. GitHub Actions builds the source-only regression suite and ARM launcher without private game assets.

Convert the owner's pinned installation and an existing 0.10 pack:

```sh
python3 -m pip install -r requirements.txt
python3 tools/build_tactics.py /path/to/game /path/to/0.10/nSpire2.nsp.tns \
  --output dist/nSpire2.nsp.tns --font /path/to/local-font
python3 tools/build_web.py
```

The converter adds original portraits/icons, retaining all existing decoded image data and glyph coverage. Host-side LZ4 high compression reduces the pack without deleting animation frames or changing the calculator decoder. `build_content.py` also chains into this final stage; a complete fresh skeleton bake was not rerun for 0.11. The offline HTML embeds private game-derived content and is not a public source artifact.

## Install and controls

With Ndless r2022 installed, put **both** `nSpire2.tns` and `nSpire2.nsp.tns` in the same calculator folder. Replace both when updating and back up saves first. Open the launcher, not the pack or save document. Version 0.11 requires the 68-card NSP2/8 pack.

New saves use **v8**; the reader accepts project save versions 3-7. Old launchers cannot load v8. The browser uses a separate `nSpire2-save-v8` key and falls back to v7 through v3 without overwriting those older keys.

Directions or 4/6/8/2 navigate; Enter or 5 confirms; Space ends a normal combat turn; Tab opens piles; I details; C compendium; **V powers/statuses**; L log; M playable map; G reference-map preview; P potions; R relics; S save; O load; Esc returns or pauses. Touchpad taps, physical clicks and card dragging use the shared dispatcher.

Inside a card-choice modal, Enter/click selects and Space/the confirmation button commits; Space does not end the turn. Purity permits zero selections. Headbutt uses a paged discard-pile grid. Saving preserves the pending game choice, not uncommitted checkmarks. Permanent deck operations require a second confirmation; Esc cancels it.

## Validation and limits

All twelve regression targets passed with ASan/UBSan. The final ARM launcher booted in Firebird; touchpad compendium/Headbutt paging, selection, save/load, Demon Form's next-turn strength and status details were exercised. Two OS-side USB-retrieved saves matched the native core byte for byte. The actual embedded WASM matched the same sequences using a supplied storage adapter. These compare backends of this project, not the full original game.

The 5,674,998-byte pack leaves 616,458 bytes below the six-MiB resident limit. The launcher is 117,183 bytes. Static loader allocation is not measured peak RAM. Physical-handheld performance, complete original encounters/acts, remaining characters and complete combat differential parity remain outstanding. Detailed current and historical evidence is in [development notes](docs/development.md) and [fidelity scope](docs/parity.md).

## License

Runtime/converter: **GPL-3.0-or-later** (`COPYING`). Go BSD, Sun and Cephes notices remain in `NOTICE.md` and `licenses/`. Original game content and trademarks remain with their owners. Code licensing grants no additional right to distribute those assets.
