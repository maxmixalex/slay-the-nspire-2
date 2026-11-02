# UI25 converter correction and release record

UI25 corrects a resource-production regression and prepares first-card action
resources within the existing cache limits. The reference remains locked to
**v0.111.0 / 41cef1ea**. This checkpoint records the diagnosed defects and the
current runtime candidate; it does not approve the final release.
Corrected full/reduced resource acceptance, final ZIP sizes and their hashes
are pending. The [numeric record](ui25-release-validation.json) identifies
each completed and pending gate separately.

## Resource correction and provenance

The complete Linux conversion from an empty work directory at public
`5a21f14752dba19e4ceaa43d1b41639dd27d4826` completed all **84 stages**.
Independent decoding then compared all **60,294 images** with the accepted
full-animation family. Exactly twelve images, IDs **2418–2429** in
`necrobinder.attack` (clip 28), had passed through `bounded-a5` again.
Their geometry, nonzero-alpha masks and 83 ms frame duration were retained,
but RGB565/alpha values changed and their decoded storage group grew from
**83,585 to 1,044,120 bytes**. These are real pixel and storage differences;
that output is not accepted as equivalent. The other 60,282 decoded images,
the checked English/Simplified-Chinese strings and glyphs, and clip metadata
matched.

Public `e98a6cb4ed29cd4e3bcaa597cff9ad089c6854f7` preserves the authored
Necrobinder attack through both actor dicing and color/storage reduction,
including its FR1 pieces. The correction does not change C++, clocks, pools
or the sampled source action. Its synthetic policy checks are separate from
actual-resource acceptance. The frozen Linux producer is **90,673,448 bytes**,
SHA-256 `35bc284f38a221eb6dad7ff5b0285b3327f41f78dafa88232dbd600422db42c7`.
Its actual source archive is local `e89d8e7b8cc0c4bfa17b6584c52e3af768de426a`,
whose tree exactly matches the published `e98a6cb` checkpoint.

The corrected production tail starts from a read-only verified copy of the
completed `5a21` paired44 boundary and extracted fonts. Source publication
and copied-file hashes are recorded before execution. The corrected frozen
producer then runs the production stages from `native-framed20` onward.
**This is a verified-prefix conversion tail, not another empty-work cold
conversion.** The earlier completed cold timing and temporary-space samples
belong only to their original producer and output. Full and reduced outputs
must complete their own final publication, locale and decoder gates before
packaging.

## Ordinary launcher candidate and first-action preparation

A new matched ordinary Firebird control reproduced a cold first-Strike stall
with the accepted c814 launcher and accepted full data: the action added
11 resource reads / 83,676 bytes and about 567 ms of resource time. Maximum
combat render increased from 60.486 to 376.740 ms in that comparison. The
accepted slash's 83,585-byte retained group alone did not prevent this loading
stall. This observation is separate from the converter's unwanted redicing.

Runtime `f37d38ea6adbcd01065705b66bfdf7fac60e3f3d` adds the player's attack
after the existing actor/enemy preparation priorities, within the same
288-image / 640-page limits. It also prepares first-card targeting, impact
and damage text resources. Action clocks and storage pools are unchanged.
Full Regent plus Nibbits does not fit an added attack within the old page
ceiling, so universal zero-read attacks are not claimed.

| Candidate item | Identity or limit |
| --- | --- |
| Runtime source | `f37d38ea6adbcd01065705b66bfdf7fac60e3f3d` |
| Test fixture correction, same runtime | `73bf452e1c8b82855ab464a892f671af0258fb36` |
| Launcher bytes | 1,736,666 |
| Launcher SHA-256 | `9b50a50eb15d5c8f22dcc8984464517df6d76a29ed53fa351476df1b7bc47aca` |
| Formal allocation / ceiling | **22,288,328 / 22,291,016 bytes** |
| Allocation headroom | 2,688 bytes |
| Full bilingual installation ceiling | **70,000,000 decimal bytes**, including instructions and licenses |

The candidate's linked allocation is 484 bytes above c814 through code,
alignment and relocation changes. `.data`, `.bss`, heap/cache pools and the
ceiling remain unchanged. Independent launcher/ELF parsing gives 125,200
loader bytes plus 22,163,128 bytes of loaded ELF memory. This is linked
allocation, not measured physical-device peak memory.

The focused native regression passes **971,633 checks**. Separate actual
accepted-full and accepted-reduced native traces use normal card selection
and Confirm, require enemy HP 44→38 and energy 3→2, and record one 4,096-byte
frame-descriptor read each. They contain no attack, targeting-arrow, hit or
damage payload reads. These host traces do not supply calculator FPS.

The ARM command returned zero, but its original journal correctly remains
`source_changed`: the synthetic glyph fixture changed during that build.
The separate input bridge verifies that all **650 compiled ARM inputs** and
both build/pack scripts were unchanged; only that test fixture changed.
This bridge must be retained with the original journal, not described as
a clean unchanged-source journal pass. Candidate ordinary guest acceptance
and corrected-family acceptance remain pending here.

Ordinary guest acceptance uses the supplied Firebird **TI-Nspire CX II CAS**,
**OS 6.4.0.74**, **64 MiB** firmware/OS/Ndless environment. Its recorded
CPU/AHB/APB clocks are **396/198/99 MHz** and its guest timer is 32,768 Hz.
No memory or snapshot-header changes are permitted. These measurements do
not establish physical-handheld performance or performance on an older CX.

## Delivery gate

The final delivery consists of exactly three calculator ZIPs: full-animation
English/Simplified Chinese, reduced-animation English/Simplified Chinese,
and the other fourteen original locales for both matching families:

| Filename suffix | Content | Optional-language folder |
| --- | --- | --- |
| `full-animated-bilingual.zip` | Full sampled actor animation; English and Simplified Chinese | `animated/data` |
| `no-character-animation-bilingual.zip` | Essential actor poses with original action durations; English and Simplified Chinese | `no-animations/data` |
| `optional-languages.zip` | Fourteen additional original locales for each matching family | Select one of the folders above |

All eight artwork parts remain required. Optional locales must match their own
family; independent conversions must use their own locale output.

The installation limit counts every extracted full-package member, including
the launcher, resource index and parts, both default locales, BUILD-INFO,
instructions and licenses. ZIP compression size is reported separately.
Windows/Linux standalone converters are separate executables and are not
counted as calculator-installed files. ZIP member hashes and normal USB
readback must identify the same final data before the release gate passes.

The recovered UI24 archives remain historical, separately accepted artifacts:
their full and reduced installations expand to 69,869,324 and 29,695,944 bytes.
Those counts do not pre-approve newly generated UI25 packages. See the
[UI24 release record](ui24-release-validation.md) for their original sources,
hashes, guest scenarios and recovered evidence.

## Retained features and measured boundaries

The runtime retains the implemented 57-Epoch Timeline with normal
obtain/reveal progression, score, timestamps and persistent unlocks; v18/NSHB
save writing and legacy readers; long-ASCII seed import; and paged history.
Upgrades must preserve the entire `state` directory. Legacy compatibility
does not invent acquisition dates. The installation instructions retain the
Timeline and explicit **Import seed code** steps. Ordinary ASCII seeds remain
usable; 64 bytes is the direct input field capacity, and 2,048 points is only
the optional drawn-map annotation buffer.

安装时只选一个中英主包，把根目录的 `nSpire2.tns` 和完整 `data` 放进计算器
同一游戏目录。升级前退出游戏，备份并保留整个已有 `state`；不要用包内空目录
覆盖存档。删除旧 `data` 后传入完整新 `data`，可选语言也换成同轮对应模式的
文件。第三包只取 `animated/data` 或 `no-animations/data` 中需要的语言；
不要把两种模式混装。

单独下载的转换器放在原版游戏目录，零参数运行默认生成英语和简体中文；
`--no-animations` 生成精简动作版，`--languages all` 生成全部十六种语言。
工具内置所需 Python/.NET 等运行时，无需另装；内部仍使用 Python，并非纯
C++ 转换器。包内说明给出 Windows/Linux 的可直接执行命令。更长 ASCII 种子
用 `--encode-seed-file` 编码后，在计算器自定义模式中明确选择 **Import seed
code / 导入种子码**，不要把输出码当普通文本输入。

Historical accepted UI24 measurements remain scoped to their recorded scenes
and windows: final Tezcatara measured **23.915010 FPS** over a warm 717-frame
window with zero resource I/O; Neow has a separate **29.055081 FPS** first-cycle
window including reads and stalls. Cold-containing rendering still reached
about 2.0 seconds for Tezcatara and 1.604 seconds for Neow. These are retained
results, not fresh corrected-resource measurements. See
[Ancient performance](ui24-ancient-performance.md),
[Timeline](ui24-timeline.md), [Necrobinder/Osty](ui24-necrobinder-osty.md) and
[ASCII seeds](seed-ascii-ui24.md) for precise evidence.

Audio, multiplayer, cloud saves and hosted leaderboards remain unimplemented.
Desktop timezone/DST data and calculator IME are not supplied. Source Early
Access Timeline placeholders remain unavailable. Port-authored history/archive
errors and Timeline status labels use English or Simplified Chinese; other
locales use English for those labels. The seven-Affliction extreme has not
received a new optimization or final performance measurement in this stage.
Physical-device measurements and exhaustive original-game fidelity remain
unestablished. These boundaries are not a user waiver or a claim of complete
desktop feature equivalence.
