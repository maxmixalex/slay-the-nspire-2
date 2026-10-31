# Original-install converter

`convert.py` is the source entry point for the same converter that can be
packaged as `nspire2-convert` (`nspire2-convert.exe` on Windows). The executable
embeds Python and native sampling helpers. It is **not** a native C++ rewrite.
An executable built with the complete helper bundle needs no separately
installed Python, .NET SDK, or C++ compiler on the player's computer.

The supported original release is **v0.111.0 / 41cef1ea**. Put the executable in
the original game directory and run:

```text
nspire2-convert
```

The directory must contain `SlayTheSpire2.pck`, `release_info.json`, and the
original `sts2.dll` either directly or in its `data_sts2_*` directory. The game
executable is never run. A different game location can be selected explicitly:

```text
nspire2-convert --game "PATH TO ORIGINAL GAME" --output "PATH TO OUTPUT"
```

English and Simplified Chinese are the default output. Use `--languages all`
for all sixteen original language editions, or a comma-separated selection
including `eng`, for example `--languages eng,zhs,jpn`. The converter's early
catalog stages build a common multilingual source; choosing fewer final
languages reduces the delivered files, not all intermediate conversion work.

## Reduced actor animation profile

```text
nspire2-convert --no-animations
```

The reduced profile retains one exact source pose for looping clips and at
most three exact source poses for action clips. Frame counts, action clocks,
clip identifiers, registered positions, and final death/transition poses
remain intact. Aliased frames share cached image data. The calculator detects
the profile from a reserved metadata word; no separate launcher or settings
toggle is required. Character selection and its Random background are static
in this profile.

The reducer runs after the complete source catalog has been generated. It does
not currently save the original-source sampling time of the first conversion.

## Progress and resuming

Each stage writes a log and a crash-readable receipt under the work directory.
Successful outputs are checked before reuse. Interrupted or failed stages are
never marked complete. Re-running the same command resumes verified stages;
changed recipe inputs or altered stage outputs are rebuilt.

The default work directory is a stable directory under the operating system's
temporary directory. It contains extracted original fonts, sampled frames, and
intermediate resource packs and can be large. Use `--work PATH` to place it on
a disk with sufficient space. The converter keeps these inputs separate from
the final output and never modifies the original game installation's files.

`--plan` prints every source, storage, and finalization stage. Developer option
`--stop-after STAGE` stops at a named checkpoint and explicitly reports that
the result is not yet an installable release. A successful plan or early
checkpoint is not evidence that a complete original conversion has passed.

## Fonts and calculator keys

All game text fonts are extracted from the owner's PCK using the Godot
`FontFile.data` resource property and verified with a font parser. Their source
files are not included in the public repository or converter executable.

Calculator key labels use the port's public outlined keycaps and arrows, with
labels rendered using the original Source Code Pro face. These are identified
as calculator interface glyphs in the conversion receipt; they are not
advertised as TI's official key font. An owner who has `TINspireKeysCX.ttf` can
select its exact historical shapes using `--key-font PATH`.

## Building from source

Install the pinned Python packages in `requirements.txt`. A developer can
supply the Spine runtime, .NET executable, and pinned FastNoiseLite header
through `NSPIRE_SPINE_RUNTIME`, `NSPIRE_DOTNET`, and
`NSPIRE_FASTNOISE_HEADER`, then run:

```text
python convert.py --game "PATH TO ORIGINAL GAME"
```

For an executable bundle, first run `tools/build_converter_helpers.py` to
produce the self-contained Spine sampler and the native noise library.
`NSPIRE_SPINE_ADAPTER` and `NSPIRE_FASTNOISE_LIBRARY` can select precompiled
helpers during source development. Build the final executable on the target
operating system with PyInstaller and `tools/build_converter_bundle.py`:

```text
python tools/build_converter_bundle.py --helpers PATH --output PATH --work PATH
```

The bundle builder includes public conversion recipes, metadata, and helper
binaries. It excludes game PCKs, original game fonts, prior release packs, and
private raw spools. Redistribution of the supplied Spine runtime must comply
with that runtime's license. A Linux executable is not a Windows executable;
Windows requires a Windows Python/PyInstaller build and Windows helpers.

The bundle receipt records the platform, embedded Python, helper requirements,
binary hash, and `--help` smoke test. Original-data conversion and physical
calculator validation require their own results and are not implied by that
receipt.
