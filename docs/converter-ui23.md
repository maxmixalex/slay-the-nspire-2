# Original-install converter

`convert.py` is the source entry point for the same converter that can be
packaged as `nspire2-convert` (`nspire2-convert.exe` on Windows). The executable
embeds Python and native sampling helpers. It is **not** a native C++ rewrite.
An executable built with the complete helper bundle needs no separately
installed Python, .NET SDK, or C++ compiler on the player's computer.

## Quick start / 快速使用

Run the desktop converter in your original game folder. These three commands
create the default bilingual animated edition, the exact-color reduced edition,
and an animated edition with all sixteen languages, respectively:

将桌面转换器放在原版游戏目录中运行。Windows 可直接双击 EXE 执行默认
转换；在 PowerShell 中将下方程序名写成 `./nspire2-convert.exe`，Linux
则写成 `./nspire2-convert`。
以下三条命令分别生成中英双语完整动画版、保留必要动作姿势的无动画版、
以及包含全部 16 种语言的完整动画版：

```text
nspire2-convert
nspire2-convert --no-animations
nspire2-convert --languages all
```

The calculator detects the animation profile from the generated data. Both
profiles use the same launcher. The desktop converter is a separate download
from the three calculator installation ZIPs; run it on your computer and copy
its generated `data/` directory beside the calculator's `nSpire2.tns` launcher.

计算器会从数据文件自动识别动画模式，两种模式共用同一个启动程序，无需在
设置中切换。转换器与三个计算器安装 ZIP 分开提供：转换器在电脑上运行，
生成的 `data/` 目录应放在计算器的 `nSpire2.tns` 旁边。

可执行版内部嵌入了 Python 和原生辅助程序，因此使用者无需另行安装
Python、.NET SDK 或 C++ 编译器。这是打包后的完整转换流程，**并非 C++
重写版**。原版游戏素材不包含在转换器中，需读取你本机的原版安装。

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

## Animated artwork policy

The full animated edition keeps every source sample, frame size and action
clock. Its UI23 color policy uses bounded RGB565 reduction and at most four
alpha units of error for animated actors. Character-selection artwork uses a
separate policy that preserves dark channel values; Ironclad, Defect and
Regent selection art retains exact alpha and only reduces the green
channel by at most one RGB565 code. Static interface/card art and typed parameter
tables remain exact. Neow/Tezcatara scene loops keep all 1,052 samples at the
original 33 ms cadence and 320×240 resolution, holding a color only while its
red/blue channel error stays within one RGB565 code and green within two.
Existing retained-plane encoding uses full checkpoints every 32 samples.
The reduced edition takes the exact source-scene path and retains its exact
original first poses instead of applying this temporal color policy.

After color selection, the converter performs exact tile deduplication, bounded
sparse-tile encoding, and literal-context retuning. These steps preserve the
selected pixels and use the existing calculator cache and decoder bounds.
Receipts record each conversion and its verification; they do not imply that
physical-calculator performance has been measured.

## Reduced actor animation profile

```text
nspire2-convert --no-animations
```

The default reduced output is `nSpire2-converted-no-animations`, separate from
the animated output. An explicit output folder containing language files that
were not requested is rejected before conversion, so a new common resource
cannot silently leave incompatible optional languages alongside it.

The reduced profile retains one exact source pose for looping clips and at
most three exact source poses for action clips. Frame counts, action clocks,
clip identifiers, registered positions, and final death/transition poses
remain intact. Aliased frames share cached image data. The calculator detects
the profile from a reserved metadata word; no separate launcher or settings
toggle is required. Character selection and its Random background are static
in this profile. The reduced edition skips color quantization entirely and
retains the original RGB565 and alpha values in every representative pose.

The reducer runs after the complete source catalog has been generated. It does
not currently save the original-source sampling time of the first conversion.

## Progress and resuming

Each stage writes a log and a crash-readable receipt under the work directory.
Successful outputs are checked before reuse. Interrupted or failed stages are
never marked complete. Re-running the same command resumes verified stages;
changed recipe inputs or altered stage outputs are rebuilt.

World sampling also has its own verified cache keyed by the actual renderer
code, model specification, original inputs, and job settings. A later change to
unrelated packaging or runtime code can require reassembling a stage while
retaining its unchanged, expensive original-frame samples.

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
