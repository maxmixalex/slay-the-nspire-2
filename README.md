# Slay the Nspire 2

Native TI-Nspire Ndless adaptation of **Slay the Spire 2**, pursuing original single-player rules and visual identity with 320×240 layout/input adaptations. Current build: **nSpire II 0.10**. The owner's **v0.111.0 / 41cef1ea** installation remains hash-pinned in `config/reference-release.json`.

**Audio, multiplayer and Timeline meta progression are excluded, not deferred.** Implemented content is available without replaying Timeline unlocks. Visual animation remains in scope.

## Current breadth

**58 card models, 25 relics, 16 potions and two playable character subsets.** Cards comprise 24 Ironclad pool models, 24 Silent pool models, nine colorless models and one generated Shiv. Neither character pool is complete. The playable expedition still uses a custom twelve-floor route, the existing bounded Nibbit encounter and three adapted events. The original regional-map subsystem remains a separate preview.

Version 0.10 adds seven shared cards: **Impatience, Master of Strategy, Shockwave, Scrawl, Purity, Hand of Greed and Panic Button**. Purity supports optional 0–3/0–5 captured-card exhaustion; Retain and card-source No Block are implemented. Scrawl costs 1 in both states: its pinned upgrade adds Retain, rather than reducing its cost. These cards enter the two colorless shop slots and both practice decks. Ordinary character rewards remain character-specific; complete original acquisition probabilities are not claimed.

Eight new relics are **Horn Cleat, Captain's Wheel, Mercury Hourglass, Art of War, Regal Pillow, Meal Ticket, Old Coin and Centennial Puzzle**. Their turn, room-entry, acquisition and first-HP-loss effects are persisted and tested within the supported combat system. See [shared-content rules and limits](docs/shared-0.10.md).

## Interface changes

Upgrade, removal and transformation now use a paged card grid, followed by explicit confirmation. Upgrade confirmation compares original-framed base/upgraded cards side by side; choosing or cancelling cannot spend gold, consume RNG or change the deck. Stale confirmations are cancelled when the gameplay revision changes.

Enlarged card details use the correct red/green/colorless frame and energy art. Relic and potion collections have real pagination and full detail overlays. The relic strip shows an overflow count instead of silently hiding additional items. Mandatory discard/exhaust and optional Purity choices share a modal grid with distinct minimum/maximum rules. The calculator's C shortcut now opens the compendium, matching the desktop host.

These are small-screen adaptations using the already converted original art, not a complete Godot renderer or pixel-identical PC layout. Existing actor/camp animations are retained without dropping frames.

## Build and test

Public source excludes original game assemblies/PCKs, calculator ROM/flash images, installable fonts and generated game-derived packs.

```sh
# LLVM clang++ and ld.lld; no network or SDK download at build time.
python3 tools/build_ndless.py

# Linux/X11 host and ten asset-free regression targets.
cmake -S . -B build/host
cmake --build build/host
ctest --test-dir build/host --output-on-failure
build/host/nSpire2 --pack dist/nSpire2.nsp.tns
```

Use `-DSPIRE_X11=OFF` for tests without X11. Historical fixtures in `tests/fixtures/` are saves from earlier project runtimes, not original-game saves. Selected content/UI tests also accept a generated private pack.

Full conversion is wired through the versioned stages; this release's executed conversion uses an existing 0.9 pack:

```sh
python3 -m pip install -r requirements.txt
python3 tools/build_shared.py /path/to/game /path/to/0.9/nSpire2.nsp.tns \
  --output dist/nSpire2.nsp.tns --font /path/to/local-font
python3 tools/build_web.py
```

`build_shared.py` validates the original installation, retains old image/glyph payloads and all clip records, and adds original shared-card/relic art to **NSP2/7**. The full `build_content.py /path/to/game --font ...` pipeline invokes the same final stage; a full skeleton conversion was **not rerun as 0.10 validation**. `build_web.py` additionally requires `wasm-ld`; its offline HTML embeds private game-derived content and is not a public-source artifact.

## Install, saves and controls

With Ndless r2022 installed, place `nSpire2.tns` and `nSpire2.nsp.tns` in the same calculator folder. **Replace both files and back up saves first.** The 0.10 launcher requires the 58-card NSP2/7 pack. Open the launcher, not the pack or save document.

The reader accepts project save versions 3–6 (0.3–0.9). New saves use **v7**, which older launchers cannot read. The browser writes a separate `nSpire2-save-v7` key and falls back to v6/v5/v4/v3 without overwriting those old keys. Import preserves an old run's current state; subsequent prototype behavior uses the new runtime.

Directions or 4/6/8/2 navigate; Enter or 5 confirms; Space ends a normal combat turn; Tab opens piles; I opens details; C opens the compendium; L opens the log; M opens the playable map; P potions; R relics; S saves; O loads; Esc returns or pauses. Touchpad taps, physical clicks and card dragging share the desktop dispatcher.

During combat card choices, **Enter/click selects, Space/the confirmation button commits**; Space does not end the turn. Purity permits selecting no cards. Saving retains the pending gameplay choice, not uncommitted UI checkmarks. During permanent deck operations, Enter first opens a confirmation, then confirms it; Esc cancels. Start Game/Practice open real Ironclad/Silent character selection. G opens the separate regional map preview.

## Evidence and remaining scope

The ten regression targets run with ASan/UBSan. The actual 0.10 ARM build is exercised in Firebird using a private working copy of the owner's CX II CAS image and supplied Ndless installer. Controlled fixtures are used for focused new-card/relic and permanent-upgrade tests; they are not unassisted complete runs. See [current validation](docs/development.md); prior 0.9 evidence is retained separately.

Original methods were inspected, not executed as a complete combat differential oracle. Physical hardware, sustained handheld performance, measured peak RAM, complete original encounter/room pools and a playable original seeded run remain unverified or unfinished. Audio, multiplayer and Timeline are not counted among those gaps.

## License

Project runtime/converter: **GPL-3.0-or-later** (`COPYING`). Go BSD, Sun and Cephes notices remain in `NOTICE.md` and `licenses/`. The original empty-repository MIT notice does not relicense imported code. Original game content and trademarks remain with their owners.
