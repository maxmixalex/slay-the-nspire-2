# Source11 checkpoint evidence — 2026-10-21

Human-readable times use Asia/Shanghai (UTC+08:00); journals retain UTC. The pinned game reference remains v0.111.0 / 41cef1ea.

## Source10 remote preservation and formal results

At **2026-10-21 11:20:43**, the work branch read back as [`1d4133ff49c6a50720c9a8ffa29cdaa3f936d040`](https://github.com/zyf722/slay-the-nspire-2/commit/1d4133ff49c6a50720c9a8ffa29cdaa3f936d040), tree `f8ad9f8a63bf5fb58de12a0679c8de51ea870cd2`, parent `7efafe07e956bba6a49b2c9f0cca85100c7e9598`. All 439 file paths, modes and blob hashes, and all 15 uploaded file contents, matched the local tree. The equivalent original local commit `cff19391844521dd79e90d6da76d386ecb1be36f` remains on its named checkpoint. Main stayed at `efc50bc0b0cc6ca8e83461a2ff394cc7a56895e0` and CI stayed disabled.

Receipt `source10-remote-readback-20261021.json` SHA-256: `bb90e6385f0e6fe2b47eb72f31532fba17fb12f493d8a635dfdba7bb200a41e7`. The detached worktree `/tmp/nspire-stage-1d4133ff` was checked against all 439 committed blobs. Its `run_logged.py` source fingerprint is `a4c0157bfb939c477882268d4b401de6e3ea1d5c1753407034f322662fd73db7`.

| Formal source10 stage | Result | Seconds | Log SHA-256 |
| --- | --- | ---: | --- |
| Native configure | passed | 2.719 | `1d1aed1af72acb56eaeb388804643c2824eb13284fe702308dd85308fe13a5f8` |
| Native ASan/UBSan build | passed, 122/122 steps | 170.790 | `77a3283198e69020374ae876ecd32fe3c650ae6327711117801efea1db89a63d` |
| ARM build | failed at link, exit 1 | 63.513 | `2f796182530d7dc8df4dfc0ed8a82210a41ab51ab8d83b6e216bb7292cd30bde` |
| WASM compile/link | passed | 80.982 | `4b3a4c1ec17fe9a52eff3cdce363e81417c1fafe038b1488f02d8a008c83e4e1` |

All completed stages reported `source_changed: false`. LSan remained disabled because of the ptrace environment, while native ASan and UBSan remained enabled. The complete 46-test native batch started at **11:25:06** with four workers, a 1,200-second per-test limit and a 7,200-second journal limit. It is **still running at this checkpoint**, not a full-suite pass. Its eventual result must retain that exact source10 identity.

The WASM file is **4,795,012 bytes**, SHA-256 `97a40b6f5111ac9b4fbb429eceb752e2bf584f3867cb095fbf57321990f966d1`. This is a compile-only artifact; no matching embedded-content HTML has been packaged yet.

Python validation passed **75 distinct tests**: prefetch 2, raster 3, rest actors 9, room assets 5, room shaders 13, scene assets 18, UI assets 4, pack/cache 21. Seven generated files and the full directory plan also passed. Bare-Python discovery initially passed 52 cases but failed one import, while pack/plan failed to start because `lz4` was absent from that process's module path. Those failures remain recorded. Rerunning the affected groups with `PYTHONPATH=/tmp/nspire-python-deps:tools` passed; the first 52 checks are not additional coverage. Summary receipt `source10-python-profile-summary.json` SHA-256: `18c19b13a4c5fee4173cbc4261545f921e93794d01178eb870e1e1b3a6dd9867`.

## ARM926 unsigned 64-bit division

The source10 ARM link exposed `__aeabi_uldivmod` references from the new rest/treasure frame normalization. The current platform supplied only 32-bit division. Source11 adds an independent restoring-division implementation to `platform/ndless/entry.S`, preserving the existing C++ arithmetic and animation periods rather than narrowing elapsed time.

The calling convention follows [Arm RTABI32 section 5.2](https://github.com/ARM-software/abi-aa/blob/main/rtabi32/rtabi32.rst): the two input register pairs contain numerator and divisor; the returned pairs contain quotient and remainder. The implementation compares and subtracts complete unsigned words, including divisors above `2^63`, and preserves shift carry before comparisons replace the flags. Callee-saved registers are restored and the stack remains aligned. Zero division uses the documented local fallback of zero quotient and original numerator remainder; presentation callers require nonzero durations. Vector results establish output/ABI behavior, not internal branch coverage.

`tests/arm_division_regression.py` loads the real linked ELF and executes its actual instructions with Unicorn 2.1.2 in ARM926 mode. Python's arbitrary-precision `divmod` is the independent result oracle. The 5,249 vectors cover unsigned word boundaries, high-bit divisors, random 32-/64-bit divisors, the actual rest periods and extreme 32-bit animation ticks. Every vector also checks r4-r11 and stack preservation and successful return within a bounded instruction count.

The isolated short diagnostic passed before source11 preservation: assembly **2.476639 seconds**, minimal linking **0.363089 seconds**, instruction execution **3.678798 seconds**. The helper is **140 bytes**. Receipt `source11-arm-helper-short-20261021/receipt.json` SHA-256: `b9065e6347951d0dc6a478afc146857833679cbb08c5ab38afc7b5715db31d9c`; log SHA-256: `5667b347ff7429e362a880d803206827c799f961a9e429370b651ab4cf853e8c`. Input hashes were unchanged throughout. This isolated ELF is not the full production ARM runtime, Firebird execution or calculator performance evidence. After source11 is pushed/read back, build the complete ARM target and execute the same oracle against its linked production ELF.

## Extinguished rest actor modulation

The pinned private `NRestSiteRoom.ExtinguishFireIfAble` first hides flame glow and then assigns DarkGray to each character container; the single-player actor belongs to `Character_1`. The source9 actor converter selected the light-off track but omitted that inherited parent modulation.

Godot 4.5.1's [Colors.cs](https://github.com/godotengine/godot/blob/4.5.1-stable/modules/mono/glue/GodotSharp/GodotSharp/Core/Colors.cs) defines the color's byte value, and [Color.cs](https://github.com/godotengine/godot/blob/4.5.1-stable/modules/mono/glue/GodotSharp/GodotSharp/Core/Color.cs) defines byte-to-float conversion. Source11 uses RGB `169/255` in binary32 (`0x3f29a9aa`, approximately `0.6627451181411743`), with alpha unchanged at one.

The fix changes the scene container before traversal, so each ordinary draw inherits the tint before blending. Defect's additive emissions are individually modulated before saturation; darkening a completed saturated plane would produce the wrong result. The reviewed Necrobinder/Osty attached-fire fragments explicitly replace COLOR and therefore keep their authored output without an extra post-multiplication. All six actor chains retain their original transforms, periods, planes and alpha. Authored parent-color drift is rejected explicitly.

The rest-actor suite grows from 9 to **15 cases**, all passed in **4.559 seconds**. Added tests cover the exact float bits, all six ordinary/material-override chains with partial alpha, parent drift, single/double additive emission, both bone/slot attached-fire paths and cache invalidation. The first new synthetic fire fixture was missing its shader document; that fixture was corrected and the complete 15-case rerun passed. This was a fixture construction error, not an actual private scene conversion. Independent read-only review found no blocking issue.

The reviewed private `NRestSiteRoom.cs` SHA-256 is `54255f76d81709efb966a769fd0d7aab780ff384578de6bcbba22e8f60b02511`; `NRestSiteCharacter.cs` is `f7ca3edaa6b4be77a014e6be86f01b38214f8bd76a1a2b54d24611d29f70a47b`. No private source or pixels are included in Git. External receipt `source11-rest-darkgray-evidence-20261021.json` SHA-256 is `3e9f95e248fc620c35354bec7fc8aec3841d06f5ce4b9c88293c5b8e98864864`; file-check receipt SHA-256 is `1e5ea08c64d03afd11f300be292dccae74fb19d39e308efe1f1f337bf4f002d0`; test log SHA-256 is `2bdd81d7a7c3066b5179049d0b08f26805276c7f0f7081a3d17bb251b12ec7eb`.

Changing this renderer invalidates the **48 rest-actor clips / 8,340 frames** closure, including the 24 dark clips / 4,170 frames that need the corrected color. All **327 enemy bounds/reference/batched closures** and texture/scene/atlas/UI/map/room-image/treasure-clip categories remain identical. Existing enemy prefetch can continue without discarding its validated work. No cache checksum or source guard is weakened.

## Next concrete validation

Push/read back and freeze this source stage before complete ARM rebuilding or private asset conversion. Native and WASM C++/headers are unchanged from source10; their identities must be compared explicitly before reusing artifacts. Finish the already-running source10 46-test batch, run source11's affected asset/profile gate, build the complete NSP2/14 pack with its strict cache checks, then verify actual-pack native/WASM saves and pixels and inspect rendered UI. Planned counts remain 558 cards, 6,100 strings, 1,170 auxiliary images, 1,695 clips and 45,719 images; the estimated maximum directory is 1,345,360 bytes. Actual complete payload size remains unknown.

The completed 0.16 delivered pair remains retained. No full 0.17 resource pack, matching HTML/calculator ZIP, original-game GPU comparison, physical-device performance result or 95% visual similarity result is claimed by this source checkpoint.
