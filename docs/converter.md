# Resource converter

Installation: [English](installation.md) · [简体中文](installation.zh-CN.md)

The standalone `nspire2-convert` (`nspire2-convert.exe` on Windows) builds
calculator data from your Slay the Spire 2 **v0.111.0 / 41cef1ea** installation.
It includes Python and its native helpers, so players do not need to install a
development toolchain. The source entry point is `convert.py`.

## Usage

Place the converter beside `SlayTheSpire2.pck` and `release_info.json`.
The original `sts2.dll` must be there or inside `data_sts2_*`.
Windows users can double-click the executable for the default English and
Simplified Chinese conversion, or run:

```powershell
.\nspire2-convert.exe
```

On Linux:

```bash
chmod +x ./nspire2-convert
./nspire2-convert
```

These commands write `nSpire2-converted/data/`. The output also includes
`converter-receipt.json`, which records the conversion. Copy the complete
`data/` beside a matching `nSpire2.tns`; the converter does not create a launcher.

| Option | Effect |
| --- | --- |
| `--game "PATH"` | Select the original installation |
| `--output "PATH"` | Select the output folder |
| `--no-animations` | Use reduced actor animation; default output is `nSpire2-converted-no-animations` |
| `--languages all` | Generate all sixteen languages |
| `--languages eng,zhs,jpn` | Generate selected languages, including English |
| `--work "PATH"` | Place the work directory on a chosen drive |
| `--workers 1` | Reduce conversion concurrency |
| `--key-font "PATH"` | Use an owner's installed `TINspireKeysCX.ttf` for key labels |

For example:

```powershell
.\nspire2-convert.exe --game "D:\Games\Slay the Spire 2" --output "D:\nSpire2"
.\nspire2-convert.exe --no-animations --languages all
```

Keep the artwork and locales from each conversion together. Full and reduced
animation share the same launcher but need their own matching data. To retain
both editions, run the conversions sequentially with separate output folders.

中文：将转换器放在原版目录中双击运行，默认输出中英双语完整动画版。
`--no-animations` 生成精简动画版，`--languages all` 生成全部语言。
转换器已自带依赖；生成的整个 `data/` 需要与另行下载的启动器配合使用。
两种版本请依次转换，并使用不同输出目录。

## Progress and resuming

Conversion displays its current stage and writes logs under the work directory.
Re-run the same command after an interruption. Completed stages are reused
when their inputs and settings still match; incomplete stages are rebuilt.

By default, the work directory has a stable name under the operating system's
temporary directory, based on the original game's path. Changing languages or
animation mode keeps that work directory, allowing expensive samples to be
reused. Moving the game or clearing the temporary directory can require a new
conversion. Do not run two conversions against one work directory at once.

Finished stages release intermediate files after their last consumer. The
converter keeps logs and selected expensive recovery data, which can still be
large. The success summary prints the actual work directory. Once the process
has exited and you have kept the final output, you can remove that dedicated
work directory if you no longer want its cache. There is no cleanup flag.

`--work` should point to a folder dedicated to this converter. Older unregistered
work files are left alone; a new work directory enables ownership tracking
from the start. `--keep-intermediates` retains all stages for debugging.

中文：中断后重跑同一命令即可复用已完成的步骤。默认缓存位于系统临时目录，
也可用 `--work` 指定其他磁盘上的专用文件夹。成功后会显示其位置；
确认转换器退出并保留最终结果后，可以手动删除该工作目录来释放空间。
不要删除整个系统临时目录、原版目录或最终输出。

## Seed codes

The converter can encode a complete ASCII seed without game files or a resource
conversion:

```powershell
.\nspire2-convert.exe --encode-seed "complete seed text"
.\nspire2-convert.exe --encode-seed-file .\seed.txt
```

The result is `SEED64-` followed by sixteen hexadecimal digits. Enter it through
**Import seed code** in Custom Run. The normal seed field treats text literally
and does not auto-detect codes. The file option streams long inputs; non-ASCII
encoder input is rejected. The code preserves the root seed, while character,
act, difficulty and run settings still affect the campaign.

## How conversion works

The pipeline in `tools/converter.py` orchestrates the individual
builders. Use the complete pipeline for a fresh installation; old numbered
builders consume specific predecessor formats and are not interchangeable.
`--plan` lists stages. `--stop-after STAGE` is a developer checkpoint, whose
output may not yet be installable.

Original images, scenes, text and fonts are read locally. Animation is sampled
offline; the calculator reads raster images and animation records. The full
profile preserves the established source samples, dimensions and action
clocks. The reduced profile keeps one representative loop pose and up to three
action poses while preserving clip timing and final poses. It still builds the
source catalog first, so it does not avoid first-conversion sampling time.

The pipeline then applies the bounded color policies described in
[porting notes](porting.md#rendering), deduplicates tiles and compresses resources.
Storage transforms preserve their selected pixels, glyph coverage, strings,
IDs and clocks. The [format reference](resource-format.md) explains the container
layers and decoder bounds.

Cache identity includes the actual renderer, supplied runtime, source inputs
and job settings. A changed sampler must not reuse old rendered pixels.
Consumers validate the dimensions, frame counts and ranges they use. Hashes
for cache selection and developer comparisons do not justify extra whole-file
integrity scans at game startup. See [AGENTS.md](../AGENTS.md) for this policy.

The work receipt records converted/reused stages, elapsed time and sampled
workspace size. Allocation is measured with `st_blocks` when available; the
fallback rounds file lengths to 512 bytes and is explicitly marked estimated.
Those measurements describe the work directory, not every temporary file on
the computer.

## Building from source

Install LLVM 18, a C++ compiler, .NET 8 and the Python dependencies:

```bash
python3 -m pip install -r requirements.txt -r requirements-release.txt
python3 tools/prepare_converter_dependencies.py --output build/converter-deps
python3 tools/build_converter_validation.py --output converter_helpers
```

The dependency preparation script obtains the public sources pinned by
`config/converter-dependencies.json`. Keep their licenses with any redistributed
helpers. `nSpire2.elf` and `arm-validation.json` are built from this checkout for
the converter's linked ARM decoder checks. Rebuild them after changing their
recorded source inputs.

For source conversion, provide the sampling dependencies through
`NSPIRE_SPINE_RUNTIME`, `NSPIRE_DOTNET` and `NSPIRE_FASTNOISE_HEADER`:

```bash
python3 convert.py --game "PATH TO ORIGINAL GAME"
```

`NSPIRE_SPINE_ADAPTER` and `NSPIRE_FASTNOISE_LIBRARY` select precompiled helpers.
To package a standalone converter, build target-specific helpers and pass
their folder to the bundle builder:

```bash
python3 tools/build_converter_helpers.py --help
python3 tools/build_converter_bundle.py --helpers PATH --output PATH --work PATH
```

The helper folder must also contain the matching ARM ELF and validation
receipt. Build on the target operating system: Linux and Windows executables
are separate outputs. The complete recipes for both platforms are in
[converters.yml](../.github/workflows/converters.yml), which publishes
`nspire2-converter-windows` and `nspire2-converter-linux`.

The bundle builder checks public locale bindings and indexes before freezing.
Its self-test exercises embedded imports, worker processes, locks, locale
metadata and the real linked ARM decoder. Actual conversion still needs the
owner's game files. Bundles include public recipes and required dependency
notices, while original resources and fonts remain private inputs.
