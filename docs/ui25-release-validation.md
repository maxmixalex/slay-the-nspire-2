# UI25 release record

The three UI25 ZIPs are built from the final **ed5** ordinary launcher and
corrected **e98** resources. Complete package validation, a separate streaming member audit and an
independent archive/resource audit pass. The full English/Simplified-Chinese
installation expands to **69,839,495 bytes**, including every instruction
and license file, leaving **160,505 bytes** below the **70,000,000-byte** limit.
The final ordinary Firebird scenarios pass for both editions, and the complete
ed5 source CI passes. The [numeric/hash record](ui25-release-validation.json)
separates these gates and retains excluded intermediate results.

The game reference remains **v0.111.0 / 41cef1ea**. Original game data stays
private. Source publication does not redistribute the calculator resources.

## Three matching packages

| Filename suffix | ZIP bytes | All extracted members | Members |
| --- | ---: | ---: | ---: |
| `full-animated-bilingual.zip` | 68,968,610 | **69,839,495** | 23 |
| `no-character-animation-bilingual.zip` | 27,964,300 | **29,811,189** | 23 |
| `optional-languages.zip` | 29,467,909 | 32,244,564 | 29 |

The filename prefix is `Slay-the-Nspire-2-0.1.0-ui25-`. Both main packages
contain the launcher, the index and all eight artwork parts, English and
Simplified Chinese, instructions, BUILD-INFO, calculator licenses and an
empty `state/` directory. The optional package contains its instructions and
fourteen locales for each matching animation profile. No user saves, desktop
converter, desktop NOTICES, firmware, NAND, PCK or original assembly is included.

| Package | SHA-256 |
| --- | --- |
| Full animation | `68dadfe183638f8d5fcc31222d82d0c93aa3efb13d8769f676b277b79b6a1146` |
| Reduced actor animation | `bafe05980754fa70da1c2c1db5c7b3e00566b88189855044fada24752a1ecdee` |
| Optional languages | `7a4a5e02035d05525be4707d8a38b00d2584e0cc9811dd1e19b398f8c8e86bc4` |

The installation ceiling uses **all extracted member sizes**, not ZIP size or
only launcher/data bytes. The full launcher/data subtotal is 69,756,436 bytes;
the remaining 83,059 bytes of instructions, BUILD-INFO and licenses also count.
The empty state directory adds no payload bytes; later user saves are excluded.
The raw manifest retains its separately named launcher/data subtotal alongside
`extracted_bytes`; the latter is the complete installation measurement.

The completed packaging journal used immutable published source
`ed5dd2564625779313aa2c71046079dee7506e76`. Its first attempt passed content
validation but failed moving a ZIP across filesystems (`EXDEV`). That failed
journal is preserved. The unchanged source completed with `TMPDIR` under the
same filesystem as the delivery directory. All 75 archive members were then
streamed and checked for length, hash, unique safe paths, exact expected names,
resource/launcher input equality, license equality and usable bilingual
instructions, without importing the packaging module. A second independent
review confirms all 75 members and reconstructs every one of the 32 locale/
profile combinations from the actual ZIPs against the production receipts and
complete image-consumer directories.

## Reproduce the fixed packaging stage

Use the immutable published ed5 source and the final matching directories.
The temporary ZIP staging directory must share the output filesystem because
the packager uses an atomic rename. The successful production run set `TMPDIR`
to `/workspace/scratch/1c4ea656da6d/recovery/packaging/tmp`, on the same filesystem
as `/workspace/scratch/1c4ea656da6d/deliverables`. A portable equivalent is:

```bash
NSPIRE_PACKAGE_SOURCE=/path/to/immutable-ed5-source
NSPIRE_PACKAGE_OUTPUT=/path/to/delivery
NSPIRE_PACKAGE_LAUNCHER=/path/to/final/nSpire2.tns
NSPIRE_FULL_DATA=/path/to/final-full-all16/data
NSPIRE_REDUCED_DATA=/path/to/final-reduced-all16/data
mkdir -p "$NSPIRE_PACKAGE_OUTPUT/staging"
TMPDIR="$NSPIRE_PACKAGE_OUTPUT/staging" python3 "$NSPIRE_PACKAGE_SOURCE/tools/run_logged.py" \
  --checkpoint ed5dd2564625779313aa2c71046079dee7506e76 \
  --label ui25-three-zips --root "$NSPIRE_PACKAGE_SOURCE" \
  --output "$NSPIRE_PACKAGE_OUTPUT/journal" --timeout 7200 -- \
  python3 "$NSPIRE_PACKAGE_SOURCE/tools/package_variants_ui23.py" \
  --stage ui25 --launcher "$NSPIRE_PACKAGE_LAUNCHER" \
  --animated "$NSPIRE_FULL_DATA" --no-animations "$NSPIRE_REDUCED_DATA" \
  --output "$NSPIRE_PACKAGE_OUTPUT" \
  --runtime-checkpoint ed5dd2564625779313aa2c71046079dee7506e76 \
  --runtime-source-ref ed5dd2564625779313aa2c71046079dee7506e76 \
  --resource-checkpoint e98a6cb4ed29cd4e3bcaa597cff9ad089c6854f7 \
  --resource-source-ref e98a6cb4ed29cd4e3bcaa597cff9ad089c6854f7 \
  --resource-source-relation equivalent_tree \
  --documentation-ref ed5dd2564625779313aa2c71046079dee7506e76 \
  --maximum-animated-bytes 70000000
```

This describes the verified source/input family. ZIP container timestamps can
change on a later rerun; verify resulting member identities and measured sizes
instead of promising a newly generated container has the archived ZIP hash.

## 安装与使用 / Installation and use

只选一个中英主包。完整动画版是 `full-animated-bilingual`；精简人物/怪物动作
版是 `no-character-animation-bilingual`，保留必要姿态和原动作时长，启动器
自动识别。把 ZIP 根目录的 `nSpire2.tns` 和整个 `data` 文件夹放在计算器同一
游戏目录；首次安装可带上空 `state`，然后打开 `nSpire2.tns`。

升级前退出游戏并备份、保留已有的整个 `state`。不要用压缩包内空目录覆盖
存档。删除旧 `data` 后传入完整新 `data`，同时更换启动器；旧的可选语言也要
换为本轮文件。所有八个 artwork 分包都必须保留，英语建议保留作回退。

额外语言包按模式选目录：完整动画主包只用 `animated/data`，精简主包只用
`no-animations/data`。只把需要的 `nSpire2.<locale>.dat.tns` 放进游戏 `data`，
不要把两种模式混装，也不要将预制语言包与自行独立转换的数据混用。

Choose one bilingual main package. Copy `nSpire2.tns` and the complete `data`
directory into the same calculator game directory. Preserve the entire existing
`state` directory when updating; replace the old `data` directory and launcher
with the matching new set. Copy optional locale files only from the folder
matching the installed edition. All eight artwork parts are required.

The runtime reads historical v3–v17 runs and NSHA and earlier progress, and
writes **v18 / NSHB**. Older launchers may not read new writes. Rollback requires
the full pre-upgrade `state` backup with its matching old launcher and data.
The title-menu Timeline supports normal obtain/reveal progression, score,
timestamps and unlocks across all 57 source Epochs. Select a node and press
Enter to inspect, then Enter again to reveal an obtained node. Legacy profiles
retain labelled compatibility access without invented acquisition dates.

## Final runtime and first-action loading

| Item | Final identity |
| --- | --- |
| Runtime and published source | `ed5dd2564625779313aa2c71046079dee7506e76` |
| Launcher bytes | 1,737,025 |
| Launcher SHA-256 | `9c578a2cb0ca079a3aea523e8fcfadf0aecef3e4aee5ca557423c9c238932b1f` |
| Formal allocation / ceiling | **22,288,920 / 22,291,016 bytes** |
| Allocation headroom | 2,096 bytes |
| Linked ARM ELF SHA-256 | `33bd3b1060f8f70e75e78f5769ec2ead97e39dcf5002937f42b08bed75b1ef37` |

The earlier ordinary c814 control exposed cold first-Strike resource loading,
including 11 reads / 83,676 bytes and a 376.740 ms maximum combat render.
UI25 appends the player's attack and current living enemies' Hit reactions
after existing actor/enemy priorities, and prepares first-card targeting,
impact and damage text. It stays within the existing **288-image / 640-page**
preparation limits. Action clocks, image/cache pools and `.data`/`.bss` sizes
are unchanged. Linked allocation grows **1,076 bytes** from c814 through code,
alignment and relocation changes; the formal ceiling is not increased.

A fresh immutable archive verified all 1,732 source files against commit blob
identities. The final ARM journal is a clean pass. The focused native regression
passes **1,059,848 checks**. Both actual final-resource native traces perform
normal card selection and Confirm after 180 idle frames, verify enemy HP
44→38 and energy 3→2, and record **zero resource reads/bytes**. These host
traces are separate from calculator measurements.

Final ordinary Firebird acceptance verifies all twelve launcher/data files for
each edition by USB readback, then starts the actual ed5 launcher fresh from
the OS. The explicitly console-prepared Necrobinder seed-1 / Nibbits fixture
uses normal card 6 plus Enter, compared with idle over matched 5,420 ms commands.
Both editions' first Strike adds **zero combat reads, zero resource bytes and
zero combat resource time**. Action and idle retain identical cumulative
combat maxima. These counters retain the shared setup/preparation history;
they are not isolated attack-only peak measurements:

| Edition | Maximum combat render | Maximum combat interval |
| --- | ---: | ---: |
| Full animation | 60.272217 ms | 70.037842 ms |
| Reduced actor animation | 61.340332 ms | 71.228027 ms |

This is a scoped action-versus-idle comparison, not a universal latency or
whole-game frame-rate claim. Final full/reduced ordinary controls verify Strike
44→38 enemy HP, Osty's Unleash 44→37, joint owner/Osty death, both corpses still
visible after 10.1 seconds, normal save/exit/relaunch/Continue and OS return.
The continued fixture retains 66 player HP, two energy and 38 enemy HP; OS
screens remain visible at 20/30 seconds before USB reads. Dense full-animation
death captures show leaning, upward rotation, falling, changed limbs during
landing and the settled corpse. These capture offsets follow sampled input;
they are not substituted for the authored clip timestamps.

Both editions also use normal Settings to switch from Simplified Chinese to
English. The English title, Ironclad screen and OS exit captures were visually
reviewed, and the saved language reads back exactly as `NSL1eng` plus newline.
Emulator language smoke is limited to English and Simplified Chinese; all
sixteen locale/profile combinations in each family have separate resource and
archive validation.

## Final scene windows and retained cold costs

These are paired guest-counter windows on the final ed5 launcher and corrected
data. The B run adds 30 seconds before the same normal exit; counts subtract
common setup/exit. Ancient fixtures use Necrobinder seed 1 and Simplified Chinese.
Random selection uses an explicitly synthetic production-codec legacy-compatible
profile with **zero obtained Epochs and timestamps**, so it does not establish
natural Timeline progression.

| Edition and scene | FPS | Mean render | Reads / bytes | Intervals >50 ms |
| --- | ---: | ---: | ---: | ---: |
| Full: Ironclad | 30.000153 | 14.273207 ms | 0 / 0 | 0 |
| Full: Random | 27.754131 | 35.181968 ms | 0 / 0 | 1 |
| Full: Neow | 29.752381 | 33.085839 ms | 0 / 0 | 1 |
| Full: Tezcatara | 26.927660 | 36.743730 ms | 0 / 0 | 1 |
| Reduced: Ironclad | 30.002197 | 13.702393 ms | 0 / 0 | 0 |
| Reduced: Random | 30.006227 | 11.444092 ms | 0 / 0 | 0 |
| Reduced: Neow | 30.007326 | 16.876221 ms | 0 / 0 | 0 |
| Reduced: Tezcatara | 30.007326 | 16.906738 ms | 0 / 0 | 0 |

All eight reported windows have zero resource I/O. Full Random, Neow and Tez
have one interval above 50 ms each and none above 100 ms; the other rows have
none above 50 ms. Reduced Random uses its static profile background, while full
Random retains animated water, light and dots. The reduced Ironclad row is its
initial 5-second-settle window, which recorded zero I/O; the full Ironclad row
is the **same fixture after an additional 60-second settling period**.

The earlier full Ironclad **5-second settle + 30-second first-cycle window is
retained separately**: **28.204384 FPS**, 16.548130 ms mean render,
**17 reads / 1,199,150 bytes**, and **five intervals above 250 ms**. It is not
labelled warm, discarded or replaced by the longer-settled row. Cold startup
and transitions still have larger stalls; the table does not promise universal
30 FPS or eliminate loading costs.

One initial Random USB attempt stalled before game launch; a wake-OS retry
passed. A later host ENOSPC interruption left a failed warm-preparation journal
and partial snapshot/image, which remain preserved. Recovery removed only 422
incomplete synchronization copies verified byte for byte against their retained
originals, then the same-source final short steps passed under distinct retry
journals. No ordinary evidence, final data or ZIP was replaced. This host
synchronization incident is separate from the converter's dedicated-TMP
measurements below.

The supplied ordinary guest is Firebird **TI-Nspire CX II CAS**, **OS 6.4.0.74**,
**64 MiB**, with recorded CPU/AHB/APB clocks **396/198/99 MHz** and a 32,768 Hz
guest timer. No memory or snapshot-header edits are used. Linked allocation
and these emulator counters do not measure physical-device peak memory or
physical-handheld FPS. Saturated rooms, including the measured Regent/Nibbits
case, can leave optional actions cold within the unchanged preparation cap.

## Complete source CI

The final [ed5 core workflow](https://github.com/maxmixalex/slay-the-nspire-2/actions/runs/41495670719)
passes native, conversion and portable jobs. The
[native job](https://github.com/maxmixalex/slay-the-nspire-2/actions/runs/41495670719/job/125450117968)
passes **183/183 suites, zero failures**, with ASan and UBSan enabled; CTest
took **345.87 seconds** and the log contains no sanitizer failure report.
Python discovery passes **652 tests**; a separate command passes **56 tests**
in six supplemental modules. These executed command counts are reported
separately, not advertised as an aggregate independent-test count.

The [ed5 Windows workflow](https://github.com/maxmixalex/slay-the-nspire-2/actions/runs/41495670730)
also passes both ARM validation and Windows converter jobs. Portable ARM and
WebAssembly compilation passes; no new interactive browser/WASM validation is
claimed. The reviewed final documentation changes affect only README/docs;
`src`, `include`, `tools`, `tests`, `platform`, `config`, `third_party` and
`.github` remain exact to tested ed5. Earlier cancelled native jobs and the
historical f37 `source_changed` ARM journal are not used as passing ed5 evidence.

## Corrected resource production and independent comparison

The actual production build/tail/publication receipts identify public **e98**:
`e98a6cb4ed29cd4e3bcaa597cff9ad089c6854f7`. Its frozen source archive originated
at local `e89d8e7b8cc0c4bfa17b6584c52e3af768de426a`; both have the exact tree
`ad517844fd666fd32c94cb5246b779b19ea510ff`. Archive origin and execution checkpoint
are recorded separately. The producer is not relabelled as the later ed5
standalone delivery executable.

| Resource family | Bilingual data, 11 files | All 16 locales, 25 files | Extra 14 locale files |
| --- | ---: | ---: | ---: |
| Full | 68,019,411 | 85,979,742 | 17,960,331 |
| Reduced | 27,990,788 | 42,273,212 | 14,282,424 |

The all-language publication preserves every one of the eleven bilingual files
byte for byte. Independent reconstruction checks the actual final published
data, including **8,961 strings per locale**. The pre-locale `final-index46`
artwork boundary has 8,704 strings and is not used as final-delivery evidence.

Full compares all **60,294 decoded image-directory entries** exactly with the
accepted UI24 family. Reduced compares **60,293 exactly**; the remaining entry,
FR1 parameter table 58117, uses equivalent retained-image aliases with identical
displayed pixels, clocks, signed positions, layer order and scale/reference
units. All **1,783 clip records**, image alias partitions and checked action
clocks remain unchanged. Each English and Chinese locale compares all strings
and respectively **12,902 / 53,351 glyph** metrics and alpha planes. Exact common
artwork and normalized directories bridge all Chinese image consumers as well.

The original-input 5a21 cold output had unnecessarily reapplied bounded-a5 to
twelve `necrobinder.attack` frames and put them in a **1,044,120-byte** decoded
tile group. The accepted group had **83,585 decoded / 36,814 stored bytes**.
The corrected full group has **83,445 decoded / 36,438 stored bytes**, with its
140-byte FR1 parameter table stored separately. All twelve authored frame
pixels and the **12 × 83 ms** schedule are preserved. This resolves the concrete
production difference; it does not assert arbitrary Spine/GPU equivalence.

## Standalone conversion, seeds and measured scope

将单独下载的 `nspire2-convert-ui25.exe`（Windows）或
`nspire2-convert-ui25-linux`（Linux）放在原版游戏目录，在该目录打开终端。
零参数运行默认生成英语和简体中文，Windows也可双击 EXE。各次转换依次运行。
工具内置需要的 Python、.NET 和采样组件，无需用户另装这些运行时；内部仍用
Python，并非纯 C++ 转换器。桌面 `NOTICES.txt` 是独立交付文件，不放进计算器。

PowerShell:

```powershell
./nspire2-convert-ui25.exe
./nspire2-convert-ui25.exe --no-animations
./nspire2-convert-ui25.exe --languages all --output ./nSpire2-converted-all-languages
./nspire2-convert-ui25.exe --no-animations --languages all --output ./nSpire2-converted-no-animations-all-languages
```

Linux:

```bash
chmod +x ./nspire2-convert-ui25-linux
./nspire2-convert-ui25-linux
./nspire2-convert-ui25-linux --no-animations
./nspire2-convert-ui25-linux --languages all --output ./nSpire2-converted-all-languages
./nspire2-convert-ui25-linux --no-animations --languages all --output ./nSpire2-converted-no-animations-all-languages
```

The converter writes `data/` and a receipt, not a launcher. Use its matching
launcher and keep optional locales from that same conversion. Default outputs
are `nSpire2-converted` and `nSpire2-converted-no-animations`. `--workers 1` or
`--workers 2` controls the bounded parallel jobs; two is the default, with one
native numerical thread per worker. Fewer workers can lower simultaneous host
compression work and increase elapsed time.

普通 ASCII 种子可直接输入。**64字节**只是直接文本框容量，不是游戏或操作
系统种子上限；**2,048点**只是可选手绘地图笔迹的采样容量，不是地图房间数。
更长 ASCII 文本可在电脑运行
`./nspire2-convert-ui25.exe --encode-seed-file ./seed.txt`，Linux用对应文件名。
在计算器 Custom Run 打开种子框，按上下切换到 **Import seed code / 导入种子码**，
输入输出的 `SEED64-` 加16位十六进制码。普通文本模式始终按原版文本语义哈希，
不要把导入码直接当普通文本使用。

The completed historical empty-work 5a21 conversion took **13,087.111953 s**
(3 h 38 min 7.112 s) of CLI wall time for 84 stages. Its dedicated whole-TMP
peak was **2,279,501,824 allocated bytes**, sampled every 0.5 s including
one-file extraction. Its subsequently rejected twelve-frame output prevents
using that run as corrected-output acceptance, although the measurements remain
factual for that executable. Two planned whole all-language warm runs did not
complete and are not reported as passed.

The corrected full **29-stage resumed tail** took **3,863.386037 s**; the reduced
**22-stage resumed tail** took **1,016.730632 s**. They start from verified
original-derived boundaries, not empty directories, and overlap in wall time.
Their workspace-only allocation peaks are respectively **1,658,974,208** and
**1,771,462,656 bytes**. These are not new whole-TMP cold benchmarks or expected
fresh conversion durations. The reduced retained workspace includes pre-existing
copied inputs that the ownership-safe collector deliberately preserves.

The final ed5 Linux standalone binary has passed its frozen self-test and locale
gate, uses the actual final ARM helper, verifies equivalent ordinary reduced
recipe declarations, and exercises game discovery/fresh fonts through an
intentional `--stop-after fonts` entry-point test. That partial command is not
called a new full original-input conversion. Windows frozen validation likewise
does not establish Windows original-game conversion.

## Retained features and remaining limits

The implemented Timeline, long-ASCII seed import, paged history, Necrobinder/Osty
death/slash handling and five-character foundation remain in scope. Source Early
Access Timeline placeholders remain unavailable. Audio, multiplayer, cloud saves
and hosted leaderboards remain unimplemented; desktop timezone/DST data and
calculator IME are not supplied. This is not a user waiver.

Original locale content uses the selected source language, with source English
fallback for missing keys. Port-authored history/archive errors and Timeline
status labels use English or Simplified Chinese; other locales use English for
those labels. The seven-Affliction extreme has no new optimization or final
performance measurement in this stage. Cold loading stalls remain outside the
scoped first-action result; historical warm Ancient results are not silently
relabelled as final ed5 results. Physical-handheld measurements and exhaustive
original-game fidelity remain unestablished.

The [UI24 release record](ui24-release-validation.md),
[Ancient measurements](ui24-ancient-performance.md), [Timeline](ui24-timeline.md),
[Necrobinder/Osty](ui24-necrobinder-osty.md), and [ASCII seed record](seed-ascii-ui24.md)
retain their original source, resource and execution-window identities.
