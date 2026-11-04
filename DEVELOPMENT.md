# Development

The game and converter are built separately. A normal run on the calculator uses the native launcher and the data made on the desktop; it does not need Python, .NET or an emulator.

## Repository layout

| Path | Contents |
| --- | --- |
| `cpp/src`, `cpp/include` | Game rules, resource reader, renderer and public C++ interfaces |
| `cpp/platform` | Ndless entry point, shared input code and the portable host |
| `cpp/tests` | Native regression tests and the small fixtures they exercise |
| `cpp/third_party` | Dependencies compiled into the runtime and notices for the shipped components |
| `python/src/nspire_converter` | PC installation reader, animation sampling, localization and resource packing |
| `python/tests`, `python/tools` | Converter tests and build commands |
| `python/dotnet` | Build-time adapters for assembly data and animation sampling |
| `assets/screenshots` | The nine gameplay clips used by both READMEs |

## Desktop converter

Use Python 3.11 or 3.12 and Poetry. From the repository root:

```sh
poetry install --with dev,build
poetry run poe check
poetry run nspire-converter --help
```

The Poetry lock file pins the Python environment. Ruff handles linting and formatting; mypy checks the typed interfaces listed in `pyproject.toml`. The converter's tests cover binary formats, resource addressing, language packs and command-line behavior.

The distributable converter is compiled from the same Python entry point with **Nuitka**. Its build also prepares the native helpers used to read or sample assets. Building those helpers requires a desktop C++ compiler and the .NET SDK; users of the packaged converter need neither a separate Python installation nor a source checkout. For the exact commands on each platform, follow the converter job in [the build workflow](.github/workflows/converters.yml).

## Native game

A host build needs CMake, a C++ compiler and a build tool such as Ninja. X11 is optional. The following builds the core and tests on Linux without a window system:

```sh
cmake -S cpp -B build/native -DSPIRE_X11=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build/native --parallel 2
ctest --test-dir build/native --output-on-failure
```

Install clang-format and clang-tidy before configuring to enable these targets:

```sh
cmake --build build/native --target format-check
cmake --build build/native --target tidy
```

The C++ source list is shared by the host and calculator builds. Compile modules as `.cpp`, put shared declarations in `.hpp`, and use private headers for definitions that must be visible in the including translation unit. Files that consist of generated initializer or enumeration entries live alongside their consumers' data rather than masquerading as translation units.

For the calculator, put Clang and LLD on your path, install the Poetry build group, and run:

```sh
poetry run python python/tools/build_ndless.py --compression zopfli --output dist/nSpire2.tns
```

Move that launcher and a complete converted `data/` directory into the same folder. In Firebird use your own supported OS, Ndless installation and normal calculator memory/clock configuration. A host test can exercise game rules; it does not exercise the calculator touchpad, the TI filesystem or the ARM frame budget.

## Distribution

The [native workflow](.github/workflows/ci.yml) builds the calculator launcher. The [desktop workflow](.github/workflows/converters.yml) builds the Windows and Linux converters. Original game files, generated game data, firmware and player saves are not repository inputs. To make the optional languages or reduced edition, use the converter switches in the [README](README.md#quickstart).
