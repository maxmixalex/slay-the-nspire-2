# Original-install converter / 原版资源转换器

Player installation: [English](installation.md) · [简体中文](installation.zh-CN.md).
This guide retains its existing filename so earlier technical references remain valid.
当前玩家安装步骤见上述双语指南；保留原文件名，确保已有技术文档的链接继续有效。

`convert.py` is the source entry point for the same converter that can be
packaged as `nspire2-convert` (`nspire2-convert.exe` on Windows). The executable
embeds Python and native sampling helpers. It is **not** a native C++ rewrite.
An executable built with the complete helper bundle needs no separately
installed Python, .NET SDK, or C++ compiler on the player's computer.

## Quick start / 快速使用

The supported original release is **v0.111.0 / 41cef1ea**. Put the downloaded
desktop converter in the original game directory and open a terminal there.
That directory must contain `SlayTheSpire2.pck`, `release_info.json`, and the
original `sts2.dll` either directly or in its `data_sts2_*` directory. The game
executable is never run.

Choose the command for the edition you want. The examples create bilingual
animated, bilingual reduced, all-language animated and all-language reduced
output, respectively. If producing several editions, run them sequentially;
these examples keep their output directories separate.

将下载的桌面转换器放在上述原版游戏目录中，并在该目录打开终端。Windows
可直接双击 `nspire2-convert.exe` 执行默认中英双语转换。
以下每组转换命令分别生成中英双语完整动画版、中英双语精简版、全部 16 种语言
的完整动画版和全部 16 种语言的精简版。按需选择；若要生成多种版本，请依次
运行，等待前一条完成。这些示例分别写入不同的输出目录。

Windows PowerShell:

```powershell
.\nspire2-convert.exe
.\nspire2-convert.exe --no-animations
.\nspire2-convert.exe --languages all --output .\nSpire2-converted-all-languages
.\nspire2-convert.exe --no-animations --languages all --output .\nSpire2-converted-no-animations-all-languages
```

Linux: run `chmod` once after downloading, then choose a conversion command.
Linux 下载后先执行一次 `chmod`，再选择所需转换命令：

```bash
chmod +x ./nspire2-convert
./nspire2-convert
./nspire2-convert --no-animations
./nspire2-convert --languages all --output ./nSpire2-converted-all-languages
./nspire2-convert --no-animations --languages all --output ./nSpire2-converted-no-animations-all-languages
```

Without `--output`, results are written inside the original game directory:
`nSpire2-converted/data` for animated and
`nSpire2-converted-no-animations/data` for reduced. The examples use the upstream
standalone names `nspire2-convert.exe` and `nspire2-convert`; if a download adds
a version suffix, substitute its actual filename.
未指定 `--output` 时，默认输出位于原版目录中的上述文件夹。上述命令使用通用文件名；
若下载的程序带版本后缀，请相应替换命令中的文件名。

The calculator detects the animation profile from the generated data. Both
profiles use the same launcher. The desktop converter is a separate download
from the three calculator installation ZIPs; run it on your computer and copy
its complete generated `data/` directory as a matching set beside the calculator's
`nSpire2.tns` launcher described in the [installation guide](installation.md).
The converter generates data only; it does not generate or download the launcher.
Optional locale files must come from that same conversion and common-data output.
Do not add languages from the prebuilt release's
optional-language ZIP or from another conversion: version and animation
profile labels alone do not guarantee matching resource addresses. The provided
prebuilt optional-language folders match the provided prebuilt game packages.

计算器会从数据文件自动识别动画模式，两种模式共用同一个启动程序，无需在
设置中切换。转换器与三个计算器安装 ZIP 分开提供：转换器在电脑上运行，
转换器只生成数据，不生成或下载启动程序；应将生成的 `data/` 目录整套放在
[安装指南](installation.zh-CN.md)所述的配套 `nSpire2.tns` 旁边。额外语言文件也必须
来自同一次转换、与这套公共数据匹配；不要混用预构建发行包的可选语言或另一次
转换的语言文件。版本号和动画模式相同并不保证资源地址相同。

可执行版内部嵌入了 Python 和原生辅助程序，因此使用者无需另行安装
Python、.NET SDK 或 C++ 编译器。这是打包后的完整转换流程，**并非 C++
重写版**。原版游戏素材不包含在转换器中，需读取你本机的原版安装。

A different game location can be selected explicitly. In PowerShell:

```powershell
.\nspire2-convert.exe --game "PATH TO ORIGINAL GAME" --output "PATH TO OUTPUT"
```

On Linux:

```bash
./nspire2-convert --game "PATH TO ORIGINAL GAME" --output "PATH TO OUTPUT"
```

English and Simplified Chinese are the default output. Use `--languages all`
for all sixteen original language editions, or a comma-separated selection
including `eng`, for example `--languages eng,zhs,jpn`. The requested languages are carried through every locale-writing stage. The
default English/Chinese conversion therefore avoids generating the other
fourteen locale directories and their temporary glyph/text packs. Shared
artwork and all source clips remain identical across language selections.

## Long ASCII seed import codes

The same executable can encode a complete original-game ASCII seed without
converting resources or requiring the game files:

```powershell
.\nspire2-convert.exe --encode-seed "the complete original seed"
.\nspire2-convert.exe --encode-seed-file .\seed.txt
```

On Linux, use `./nspire2-convert` with the same options, for example
`./nspire2-convert --encode-seed-file seed.txt`.

The second form streams a text file and keeps its complete seed identity even
when the text is longer than the calculator entry field. Both commands print
`SEED64-` followed by sixteen hexadecimal digits. Enter that code through the
calculator's explicit seed-code import mode. Ordinary seed entry continues to
treat prefixes such as `raw:` and `SEED64:` as ordinary text. The encoder uses
the pinned original game's ASCII casing, O/I normalization, whitespace trimming
and xxHash64 rules; non-ASCII input is rejected explicitly. The code identifies
the root seed; character, act, Ascension and unlock settings still select their
normal gameplay rules.

同一个转换器也能在没有原版文件的情况下，把完整 ASCII 种子或种子文本文件
转换为 `SEED64-` 导入码。计算器端使用明确的种子码导入模式；普通种子输入
仍按普通文字处理 `raw:`、`SEED64:` 等前缀。文件方式采用流式处理，保留
完整长种子，非 ASCII 输入会明确报错。

## Animated artwork policy

The full animated edition keeps the port's established source sample counts,
individual frame sizes and action clocks. Actors are rasterized at their
displayed density, then stored as cropped registered pieces, rather than fitted
into one fixed small sprite size. The UI24 material and Ancient-effect repairs
do not further downscale those actor rasters or remove their samples. This does
not imply that the sampled calculator animation contains every frame rendered
by the original PC engine. Its inherited actor color policy uses bounded RGB565 reduction and at most four
alpha units of error for animated actors. Character-selection artwork uses a
separate policy that preserves dark channel values; Ironclad, Defect and
Regent selection art retains exact alpha and only reduces the green
channel by at most one RGB565 code. Static interface/card art and typed parameter
tables remain exact. Neow/Tezcatara scene loops keep all 1,052 samples at the
original 33 ms cadence and 320×240 resolution, holding a color only while its
red/blue channel error stays within one RGB565 code and green within two.
Existing retained-plane encoding uses full checkpoints every 32 samples.
The reduced edition takes the exact source-body path and retains its original
first poses instead of applying this temporal color policy. Scene shader clocks
are held at zero. Both profiles share the reviewed half-resolution basic/triangle
noise used by Tezcatara fire. Neow's static held-clock light field is now baked
once on the computer as NWF3 data, replacing its six former procedural input
pages while preserving image IDs and all transmission/body frames. The baked
field is exact to the port's former held-clock field. Tezcatara's calculator
material interpolation is an explicit approximation, separate from actor/body
colors; it does not claim original-noise pixel equality.

After color selection, the converter performs exact tile deduplication, bounded
sparse-tile encoding, and literal-context retuning. These steps preserve the
selected pixels and use the existing calculator cache and decoder bounds.
The final index cleanup executes the real linked ARM decoder before and after
each proposed removal. It requires both exact restored bytes and no increase
in decoder instruction count; the bundled ELF is tied to the converter's
frozen public runtime source. Lossless font storage then preserves every
coverage byte, glyph metric, text string, and common artwork byte. Receipts
record these checks; they do not imply that physical-calculator performance
has been measured.

## Reduced actor animation profile

```powershell
.\nspire2-convert.exe --no-animations
```

On Linux, use `./nspire2-convert --no-animations`.

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
in this profile. The reduced edition skips actor/body color quantization and
retains the original RGB565 and alpha values in every representative actor/body
pose. Its scene shader effects use clock zero with the shared reviewed noise
exception above. Neow foreground transmission is generated from the original
first pose, rather than reducing the full profile's held transmission.

The reducer runs after the complete source catalog has been generated. It does
not currently save the original-source sampling time of the first conversion.

## Original Timeline text

Every requested locale receives the same 257 Timeline string slots, from the
original Epoch and Timeline tables, with required original-font glyphs. Missing
source translations use the original English fallback; optional absent unlock
text stays empty. The locked Thai source lacks 185 of these translations, and
all locales have 37 absent optional values. These source omissions are retained
explicitly rather than advertised as fully translated content.

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
temporary directory, named from the resolved original-game directory path.
Changing `--languages` or `--no-animations` does not select a different work
directory. The same game path and temporary root therefore find the same
available cache automatically; `--work` is optional for normal sequential
reuse. Matching receipts and inputs still determine which stages are reusable.
Moving the game, changing the temporary root or having the system remove it
can require conversion again. The converter releases owned intermediate packs and
sampling scratch after their final successful or verified consumer. This includes
the positional work directories of fresh actor dicing, codec45, final quality,
sparse conversion and literal-context retuning. Shared light-sampling data stays
available until its last reader. It keeps diagnostic
receipts and selected expensive recovery boundaries, including the source
language catalog and the artwork before profile-specific scene work. An
interrupted stage keeps its required inputs, and a later run verifies its
available boundary before rebuilding missing dependencies. Existing legacy
work directories are preserved; reclamation applies only to directories owned
by the lifecycle manifest. The collector does not retroactively claim a
pre-existing directory that an older version failed to register. Use a new
`--work PATH` to apply the corrected ownership tracking from the beginning,
or `--keep-intermediates` for developer inspection of every generation. A later
default run can reclaim previously registered scratch after verifying its
consumers, without rerunning the resource transformations.
The success summary prints the actual workspace path and sampled retained
logical and allocated bytes. Recovery boundaries can be large; this is not a
promise that the directory contains only small receipts.
Animated and reduced runs can reuse the same
work directory sequentially; never run two conversions against that directory
at the same time. Use separate output directories to retain both profiles.
The converter keeps these inputs separate from the final output and never modifies the original game installation's files.

After the converter has exited and you have preserved the final output and its
`converter-receipt.json`, you may manually remove the dedicated work directory
printed in the success summary if you no longer need its resume/reuse data.
If you supplied `--work`, that directory must be dedicated to this conversion
and contain no unrelated user files. Remove only that work directory, not the
system temporary directory, original game directory or final output. There is
no cleanup command-line flag. A later conversion rebuilds any required stages
whose retained data was removed.

Conversion uses at most two worker processes for independent sampling jobs and
one native numerical thread inside each worker. `--workers 1` selects one
worker when memory is more limited. This setting does not reduce frame counts,
resolution, action clocks or language coverage. The packaged `--self-test`
starts two actual interpreter workers, checks their numerical environment and
shared locks, and verifies that their temporary files can be removed.

默认工作目录位于系统临时目录，名称与解析后的原版安装路径绑定，切换
`--languages` 或 `--no-animations` 不会改变该目录。原版位置和临时目录不变时，
可自动找到保留的缓存，无须额外指定 `--work`；具体阶段仍须通过身份和内容核对。
同一命令再次运行时，
程序会核对并复用已完成的阶段；失败或中断的阶段不会被标记为完成。已被后续
阶段用完且已登记为本程序所有的临时包和采样目录会自动清理，保留诊断记录和
部分较昂贵的恢复边界。这些恢复数据仍可能较大。旧版本未登记的既存目录不会
被事后认领并删除；可用新的 `--work PATH` 从头启用修正后的登记和回收。
成功摘要会打印实际工作目录以及采样到的保留字节数，便于复用和管理。若系统
已清空临时目录，下次会重新转换。完整动画版和精简版可顺序复用同一工作目录，
但应写入不同输出目录。可用 `--work PATH` 指定其他磁盘的工作目录；
`--workers 1` 可降低并发内存占用。

转换器进程退出、最终输出与其中的 `converter-receipt.json` 已保留后，若不再
需要续跑或复用缓存，可手动删除成功摘要中显示的专用工作目录。若自行指定了
`--work`，该目录应专门用于转换，不能包含其他用户文件。仅删除该工作目录，
不要删除整个系统临时目录、原版游戏目录或最终输出。程序没有清理命令行参数；
下次转换会重新构建所需但已删除的阶段数据。

`receipts/pipeline.json` records complete pipeline elapsed time, converted and
reused stages, reclaimed bytes, and measured work-directory logical and
allocated-byte peaks. Space is sampled every half second and at stage
boundaries, so this is an observed peak rather than a mathematical maximum.
Allocated bytes use filesystem `st_blocks` when available. Without that field,
including on Windows, the compatible counter estimates by rounding each unique
file's logical size to 512 bytes. `allocation_bytes_estimated`,
`allocation_measurement` and the printed `estimated allocated bytes` label make
that distinction explicit; this fallback does not measure filesystem allocation
granularity. These counters cover the workspace, not frozen-executable extraction
or every other temporary directory on the computer.
The final converter receipt includes that summary; an individual stage
`elapsed_seconds` is not the total conversion time.

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

Install the pinned Python packages in `requirements.txt`, including the desktop
Unicorn 2.1.4 dependency. A source developer also needs LLVM 18 to build the
real ARM validation helper from the same checkout. This build step uses the
normal production decoder and does not change calculator memory limits:

```text
python tools/build_converter_validation.py --output converter_helpers
```

The generated `nSpire2.elf` and `arm-validation.json` are mandatory. A missing,
modified, or source-mismatched helper is rejected; there is no unchecked
fallback. Rebuild them after changing their recorded runtime source inputs.
Then supply the Spine runtime, .NET executable, and pinned FastNoiseLite
header through `NSPIRE_SPINE_RUNTIME`, `NSPIRE_DOTNET`, and
`NSPIRE_FASTNOISE_HEADER`, and run:

```text
python convert.py --game "PATH TO ORIGINAL GAME"
```

For an executable bundle, run `tools/build_converter_helpers.py` to produce
the self-contained Spine sampler and native noise library. Add the matching
`nSpire2.elf` and `arm-validation.json` to that target's helper directory.
`NSPIRE_SPINE_ADAPTER` and `NSPIRE_FASTNOISE_LIBRARY` can select precompiled
sampling helpers during source development. Build the executable on the target
operating system with PyInstaller and `tools/build_converter_bundle.py`:

```text
python -X utf8 tools/build_converter_bundle.py --helpers PATH --output PATH --work PATH
```

The bundle builder first checks every reviewed UI locale binding and generated
locale index using only public source/configuration. Missing bindings or stale
indexes stop the build before PyInstaller, helper loading or output creation,
instead of waiting for the later original-game localization stage. After
freezing, it also runs `--internal-stage locale_profile` against the executable's
embedded source/configuration with developer search paths removed. These checks
need no original game assets and do not substitute for an actual conversion.

The Windows workflow builds the public ARM helper on Linux, transfers its
source-bound ELF and receipt to the matching Windows job, and embeds Unicorn
with it. The executable's self-test actually executes literal and predicted
index streams through that linked decoder; it does not accept a cached test
result. End users need neither this toolchain nor separately installed Python,
.NET, or Unicorn.

The bundle builder includes public conversion recipes, metadata, and helper
binaries. It excludes game PCKs, original game fonts, prior release packs, and
private raw spools. Redistribution of the supplied Spine runtime must comply
with that runtime's license. A Linux executable is not a Windows executable;
Windows requires a Windows Python/PyInstaller build and Windows helpers.

The bundle receipt records the platform, embedded Python, helper requirements,
binary hash, Unicode/module/native-helper checks, and real linked-ARM self-test. Original-data conversion and physical
calculator validation require their own results and are not implied by that
receipt.
