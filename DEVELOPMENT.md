# Developing Slay the Nspire 2

Player setup is in [English](docs/installation.md) and [简体中文](docs/installation.zh-CN.md).
Read [AGENTS.md](AGENTS.md) and the current section of [WORK_STATE.md](docs/WORK_STATE.md)
before changing the project. The [documentation index](docs/README.md) links the
current release and historical evidence.

## Repository layout

| Path | Purpose |
| --- | --- |
| `src/`, `include/` | Native game, rendering, input, storage and content code |
| `platform/ndless/` | Calculator entry point, OS I/O and persistence |
| `platform/web/` | Browser host source and compile target |
| `convert.py`, `tools/` | Desktop conversion, packaging, build tools and evidence helpers |
| `config/` | Public conversion recipes and content metadata |
| `tests/` | Native, Python and asset-free fixtures; optional private-resource checks |
| `docs/` | Player guides, current release receipts and historical technical records |
| `licenses/`, `third_party/` | Dependency source and license notices |
| `.github/workflows/` | Native/sanitizer checks, portable builds and Windows converter build |

The current UI25 delivery uses the pinned **v0.111.0 / 41cef1ea** reference.
`VERSION` and the in-game label still identify `0.1.0-ui24`; the delivery stage
and exact runtime/resource identities are recorded separately in
[UI25 release validation](docs/ui25-release-validation.md). Do not infer new
runtime acceptance from a documentation or packaging commit.

## Native build and regression tests

The public CI uses Ubuntu 24.04, LLVM 18, CMake, Ninja and Python. A native
headless build does not need original game resources or the X11 development
libraries. Install Python dependencies with:

```bash
python3 -m pip install -r requirements.txt -r requirements-release.txt
```

Configure the same sanitizer settings as CI. Use the exact committed source
identity in long-command journals:

```bash
NSPIRE_CHECKPOINT="$(git rev-parse HEAD)"
cmake -S . -B build/host -G Ninja -DSPIRE_X11=OFF \
  -DCMAKE_CXX_COMPILER=clang++-18 -DCMAKE_C_COMPILER=clang-18 \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-O1 -fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_C_FLAGS="-O1 -fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
python3 tools/run_logged.py --checkpoint "$NSPIRE_CHECKPOINT" --label native-build -- \
  cmake --build build/host --parallel 2
ulimit -s 32768
python3 tools/run_logged.py --checkpoint "$NSPIRE_CHECKPOINT" --label native-tests -- \
  ctest --test-dir build/host --output-on-failure --parallel 2
```

The stack allowance is for host sanitizer fixtures; it does not change the
calculator allocation. For an interactive Linux host, configure a separate
build with `SPIRE_X11=ON` and the X11 development libraries installed.

Converter and packaging regressions run without private assets:

```bash
PYTHONPATH=tools:tests python3 tools/run_logged.py --checkpoint "$NSPIRE_CHECKPOINT" \
  --label converter-tests -- python3 -m unittest discover -s tests -p 'test_*.py'
PYTHONPATH=tools:tests python3 tools/run_logged.py --checkpoint "$NSPIRE_CHECKPOINT" \
  --label pack-tests -- python3 -m unittest animation_regression logged_command_regression \
  character_pack_regression all_character_pack_regression event_pack_regression world_pack_regression
python3 -m compileall -q tools
```

The authoritative public check commands remain in [ci.yml](.github/workflows/ci.yml).
Tests that take a real resource path are separate from these asset-free gates.

## Calculator and WebAssembly builds

The calculator builder uses LLVM `clang++` and `ld.lld`; ensure LLVM 18 binaries
are available on `PATH`. The release compression option needs the pinned
`zopfli` dependency from `requirements-release.txt`.

```bash
python3 tools/run_logged.py --checkpoint "$NSPIRE_CHECKPOINT" --label arm -- \
  python3 tools/build_ndless.py --compression zopfli
```

The generated launcher is `dist/nSpire2.tns`. Build success and its linked
allocation do not establish startup or performance in an emulator or handheld.
The formal program ceiling is **22,291,016 bytes**; heap, cache pools and arenas
must not be enlarged to pass acceptance.

The browser runtime can be compiled without embedding private resources:

```bash
python3 tools/run_logged.py --checkpoint "$NSPIRE_CHECKPOINT" --label wasm-compile -- \
  python3 tools/build_web.py --compile-only
```

This requires `wasm-ld`. It is a compile gate, not a browser UI/persistence test
or a current offline-game deliverable. Current interactive acceptance uses
ordinary Firebird; see the scope in [AGENTS.md](AGENTS.md).

## Desktop converter

The standalone converter bundles Python and native helpers. Users need no
separate Python, .NET or C++ installation; source developers do need the
toolchain for the helpers they build.

[The converter guide](docs/converter-ui23.md#building-from-source) covers the
linked ARM validation helper, dependency preparation, Spine/noise sampling
helpers and PyInstaller bundle. It is the full source-build reference.
[converter-windows.yml](.github/workflows/converter-windows.yml) provides the
reproducible Windows pipeline and artifact naming. Use the appropriate target
OS for a standalone executable.

The converter generates `data/` and a receipt, not a launcher. Independent
conversions must keep their own matching locale files, including when version
and animation labels agree. Original data, assemblies, fonts, generated packs
and embedded-content HTML stay outside the public repository.

## Release and evidence workflow

Preserve and publish a coherent source checkpoint before long compilation,
conversion or emulator work, then use `tools/run_logged.py` for the executed
commands. A journal left running is unfinished; a source-changed journal is
not an unchanged-source pass. Preserve failures and record their later fixes
separately.

For the accepted UI25 family, [the release record](docs/ui25-release-validation.md)
provides the packaging command, exact source/resource relationship, three ZIP
hashes, member audit and ordinary full/reduced Firebird results. Existing
verified ZIPs are not rebuilt for documentation-only changes. The
[numeric companion](docs/ui25-release-validation.json) records exact values.

The complete full-animation bilingual installation must fit **70,000,000
decimal bytes**, counting every extracted member, instructions and licenses,
before saves. ZIP size is a different measurement. Optional languages must
match their full or reduced resource family.

Report evidence at its actual level: source tests, actual-resource comparisons,
ARM builds, ordinary Firebird execution and physical-device measurements.
Do not turn model counts into fidelity percentages or warm emulator windows
into a universal handheld FPS claim. Update the current WORK_STATE entry with
what passed, what failed and the next concrete step.

## Historical records

Versioned reports under `docs/` retain their original source identities. For
the former README's detailed UI3 and 0.16 history, see the
[README at the verified UI25 source checkpoint](https://github.com/maxmixalex/slay-the-nspire-2/blob/ed5dd2564625779313aa2c71046079dee7506e76/README.md).
The file named `docs/development.md` is specifically a historical 0.12 recovery
report; it is not the current build guide.
