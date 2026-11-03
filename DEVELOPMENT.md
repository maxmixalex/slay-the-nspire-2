# Development

Player setup: [English](docs/installation.md) · [简体中文](docs/installation.zh-CN.md).
Read [AGENTS.md](AGENTS.md) and [WORK_STATE.md](docs/WORK_STATE.md) before making
changes. The latter records active work; older stage reports live in Git history.

## Project layout

| Path | Purpose |
| --- | --- |
| `src/`, `include/` | Game logic, rendering, input, resources and saves |
| `platform/ndless/` | Calculator entry point, OS I/O and persistence |
| `platform/web/` | WebAssembly host |
| `convert.py`, `tools/` | Conversion pipeline, builds and packaging |
| `config/` | Conversion recipes and content metadata |
| `tests/` | Native/Python regressions and fixtures |
| `licenses/`, `third_party/` | Dependency code and notices |
| `.github/workflows/` | Native tests, portable builds and Windows converter |

Generated `*.inc` files and the numbered conversion recipes are source inputs.
Do not remove them simply because their names refer to an earlier stage.
Build outputs, generated game data and test receipts belong outside tracked
source. Documentation keeps reusable instructions and contracts; test logs and
superseded delivery reports need not accumulate under `docs/`.

## Build and test on Linux

Install CMake, Ninja, LLVM 18 and Python dependencies:

```bash
python3 -m pip install -r requirements.txt -r requirements-release.txt
cmake -S . -B build/host -G Ninja -DSPIRE_X11=OFF \
  -DCMAKE_CXX_COMPILER=clang++-18 -DCMAKE_C_COMPILER=clang-18 \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-O1 -fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_C_FLAGS="-O1 -fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
```

After preserving the source checkpoint as required by AGENTS, run long commands
through the journal wrapper:

```bash
NSPIRE_CHECKPOINT="$(git rev-parse HEAD)"
python3 tools/run_logged.py --checkpoint "$NSPIRE_CHECKPOINT" --label native-build -- \
  cmake --build build/host --parallel 2
ulimit -s 32768
python3 tools/run_logged.py --checkpoint "$NSPIRE_CHECKPOINT" --label native-tests -- \
  ctest --test-dir build/host --output-on-failure --parallel 2
PYTHONPATH=tools:tests python3 tools/run_logged.py --checkpoint "$NSPIRE_CHECKPOINT" \
  --label converter-tests -- python3 -m unittest discover -s tests -p 'test_*.py'
PYTHONPATH=tools:tests python3 tools/run_logged.py --checkpoint "$NSPIRE_CHECKPOINT" \
  --label pack-tests -- python3 -m unittest animation_regression logged_command_regression \
  character_pack_regression all_character_pack_regression event_pack_regression world_pack_regression
```

These gates use synthetic public fixtures. Tests that accept a real resource
path run separately with private inputs. The larger host stack is for
sanitizers and does not change calculator memory. An interactive Linux build
uses `SPIRE_X11=ON` and X11 development libraries. The host can load a converted
directory with `--resources DIR --language eng` or `--language zhs`.

The exact CI commands are in [ci.yml](.github/workflows/ci.yml).

## Calculator and WebAssembly builds

Put LLVM 18 on `PATH`, including `clang++`, `ld.lld` and `wasm-ld`:

```bash
python3 tools/run_logged.py --checkpoint "$NSPIRE_CHECKPOINT" --label arm -- \
  python3 tools/build_ndless.py --compression zopfli
python3 tools/run_logged.py --checkpoint "$NSPIRE_CHECKPOINT" --label wasm -- \
  python3 tools/build_web.py --compile-only
```

The calculator output is `dist/nSpire2.tns`. Zopfli is pinned in
`requirements-release.txt`. Portable builds need no original game assets.
Interactive acceptance uses ordinary Firebird; manual browser validation needs
a renewed user request as specified in AGENTS.

## Converter

[docs/converter.md](docs/converter.md) covers source conversion, helper builds,
standalone bundles and cache reuse. Follow the complete pipeline for fresh data;
individual historical builders expect particular predecessor formats.
The [Windows workflow](.github/workflows/converters.yml) includes its
ARM helper, target-native sampler, PyInstaller bundle and self-tests.

Original game data, fonts, assemblies, private reference runners, converted
packs and HTML with embedded game content stay outside the public repository.
The supported original release is pinned in `config/reference-release.json`.

## Target budgets and verification

| Constraint | Budget or setup |
| --- | --- |
| Display | 320×240 RGB565 |
| Formal calculator allocation | At most 22,291,016 bytes |
| Complete bilingual full-animation installation | At most 70,000,000 decimal bytes, including launcher, data, instructions and licenses |
| Ordinary emulator validation | 64 MiB CX II CAS, OS 6.4.0.74, Ndless r2022 |

Do not enlarge the heap, cache pools, arenas or preparation limits to pass a
test. Use ordinary OS/Ndless startup without guest memory, clock or snapshot
header changes. Linked allocation is distinct from measured peak RAM. Cold
loading and warm scene windows should be measured separately.

For a release, test changed behavior through normal input, save/relaunch and
OS exit, and record the actual launcher/data pair. Native agreement, a successful
ARM build and Firebird execution answer different questions. Physical handheld
performance needs physical measurements. Keep failures and unresolved cases
visible in the active work state; do not inherit old acceptance after changing
code or resources.

`tools/package_release.py` packages private calculator data and, when requested,
the offline player. Preserve all required notices. The public CI artifact is a
launcher with licenses and instructions; it contains no game data.

## Technical references

- [Resource formats and ownership](docs/resource-format.md): storage layers, codecs, cache lifetimes and format checks.
- [Porting notes](docs/porting.md): gameplay/presentation boundaries, seeds, calendar data and color-math provenance.
- [NOTICE.md](NOTICE.md): third-party source and distribution obligations.
