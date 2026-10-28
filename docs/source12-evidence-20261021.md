# Source12 checkpoint evidence — 2026-10-21

Human-readable times use Asia/Shanghai (UTC+08:00); journals retain UTC. The game reference remains v0.111.0 / 41cef1ea. This document records the prior completed gates and the source12 source/short-diagnostic stage. It does not declare a complete 0.17 release or a 95% original-UI result.

## Source11 remote identity and completed gates

At **2026-10-21 11:32:24**, the work branch read back as [`713ab3be32c5e1331da132581d6ecad3c9d31e0e`](https://github.com/zyf722/slay-the-nspire-2/commit/713ab3be32c5e1331da132581d6ecad3c9d31e0e), tree `3c3cf590656ccbdf91030bb85214ce44bc85a34e`, parent `1d4133ff49c6a50720c9a8ffa29cdaa3f936d040`. All **441 paths, modes and blob hashes**, and all **seven changed contents**, matched. The equivalent local `9b0c15e64938253843c3f75d8281f0c11a3f6caf` remains on `checkpoint/0.17-arm-rest-tint-source11`. Main stayed at `efc50bc0b0cc6ca8e83461a2ff394cc7a56895e0`; CI stayed disabled.

Remote receipt `source11-remote-readback-20261021.json` SHA-256: `89745a0f57766452b644a6f3092eed8b6e0e6d0d4474be74a45470a202a6b9a0`. All 441 frozen files in `/tmp/nspire-stage-713ab3be` were checked. The source fingerprint is `246a413b8ff48e3900115088a4335ccf209404c0d5ace85b9d86670007d94e4e`.

| Formal frozen source11 gate | Result | Seconds | Log SHA-256 |
| --- | --- | ---: | --- |
| Complete ARM compile/link/package | passed | 153.396 | `dd487e9fd94522e54df49a87d1c111e889d8faa49286e5677dba40bea7b6b61c` |
| Actual production ELF division/ABI oracle | 5,249 vectors passed | 2.732 | `c5d2b03215f5446b1e673399365bc0e9d1f6c9e6bbb898cb218d7c5ac880babc` |
| Python world asset discovery | 60 tests passed | 7.872 test time | recorded in the asset receipt |
| Python pack/cache | 21 tests passed | 1.363 test time | `2e4e9ee40f3aed275c4f1c3cd25a3077f7bc4673b4dd6515b5040feec218612d` |
| Generated tables and complete directory plan | passed | 3.600 journal time | `2de631156255b6db363f1b36b0c8978a20bd0960821702d43341c7964734241d` |

All completed journals report `source_changed: false`. ARM produced `nSpire2.tns`, **839,822 bytes**, SHA-256 `1423d1f1250ce36f0c256bc482372f2c7678a201a9155cafdcb48863d3484b10`. Allocation is **10,417,060 bytes**, with 28,471 absolute relocations. Its linked ELF is **5,148,380 bytes**, SHA-256 `251ef2b21d3244598ec7d432cae5016de57dd7086d0a4ced9891cfe3bf35ed2d`. Unicorn 2.1.2 executed the actual 140-byte helper in that ELF against Python integer division, checking the result, callee-saved registers, stack and return. It does not measure physical-device execution or performance.

Formal ARM receipt `source11-arm-formal-20261021.json` SHA-256: `cb1cf32b7d9cd0a1369a246496b2a736c73c2ca4c9040628ccaa2a6465030776`. Asset receipt `source11-assets-python-20261021/receipt.json` SHA-256: `d67aff8fc76e9a6eb2ddbf2f939480d40f3ee1f64150d70aa3c0ece00d028b0d`; it lists all **81 distinct test names** and all three journal/log hashes. The earlier failed source10 ARM journal and missing-lz4 Python attempts remain retained as historical failures.

The source-only secondary archive `slay-the-nspire-2-source11-recovery-20261021.zip` is **1,571,863 bytes**, SHA-256 `9f7d16a708eb14562457723c63ff8d69717cc75a29c407987679a54628694add`. It includes the complete committed source tar, per-file manifest, remote receipt and Chinese recovery instructions. All 441 tar entries were compared by blob/mode and the ZIP was read back after atomic publication. An initial tar permission assertion rejected the default group-writable archive; rebuilding with explicit `tar.umask=0022` produced the recorded verified archive. No private game assets are in it.

## Source10 native batch and current private conversion

The complete 46-suite batch still belongs to frozen source10 `1d4133ff49c6a50720c9a8ffa29cdaa3f936d040`, not source12. At this source stage **43 suites have passed and three are still running**. Confirmed complete repaired suites include enemies (202.18 s), world map/Dolly (35.90 s), room UI (41.42 s), frontend UI (103.83 s) and presentation blending (0.06 s). The relic-combo, events and character-UI suites remain unfinished under the original 1,200-second per-test limit. No result is inferred from elapsed time or partial logs.

An explicit comparison found **298 identical** native/WASM runtime/build/test inputs between source10 and source11; receipt SHA-256 `5020c163c976ab861519c0abdadfc98c35172f5288d170d407c02d965a7fdb75`. Source12 changes the frontend and therefore needs new linking and affected regression/backend execution. Earlier native/WASM results are retained under their exact checkpoint identities.

The complete source11 pack journal `source11-full-world-pack-b9eb4555f6aa.json` began at **11:34:13**. It and the two-worker reverse enemy prefetch are still running. The latter retains its source9 identity because all 327 enemy rendering closures are identical. Both jobs use the same strictly hashed cache and distinct production paths. Running logs and cache counts are not completion evidence. Planned NSP2/14 counts remain **558 cards, 6,100 strings, 1,170 auxiliary images, 1,695 clips and 45,719 images**, with a conservative directory bound of 1,345,360 bytes. Actual complete payload size remains unknown until canonical assembly finishes.

## Original treasure holder behavior

The pinned single-player holder constructs a cosmetic delay of **0.2 + 0.2 × Chaotic.NextFloat seconds**. Its parent starts 150 original pixels below the target and moves over **600 ms with BACK_OUT**, while delayed parent RGB fades from black to white over **200 ms**, keeping alpha one. The child relic independently fades transparent-black to white over **250 ms**. The parent's RGB and child's RGB multiply before the child's alpha composites over the collection/chest.

The holder ignores pointer input until delay plus movement has completed. Original focus assignment and keyboard/controller select bypass that MouseFilter gate, so focused confirmation can claim earlier. Source12 omits the pointer hit region while gated and leaves the Confirm path intact. This also prevents an early captured press from acquiring on release after the gate opens. The unrelated first-time tutorial can disable interaction separately; that tutorial flow remains unfinished.

Source12 implements this in App presentation fields and methods. A fresh nonempty holder takes one draw from a session-local xorshift32 stream, seeded on first use from a fixed mixed constant and the presentation clock. The stream survives room resets and loads; restoring an already-open offer shows its terminal appearance without another draw. Empty chests take none. This is an explicit deterministic platform adaptation of original Chaotic wall-clock/MegaRandom behavior, not an assertion of the same random sequence. No State, save-codec or named/legacy gameplay RNG field changes.

The 150-pixel entry distance projects into the 218-pixel room viewport as 30 pixels. The existing 33 ms animation clock samples the original timing; Q12 evaluates BACK_OUT without a floating-point runtime. Among 8,040 independent float-reference delay/tick samples, 14 round to a position one pixel different; maximum error is one pixel. RGB565 source modulation introduces the already required color quantization. These limitations are explicit instead of being hidden inside a similarity percentage.

The 44-pixel icon's final y is 84, with its full entry extent reserved above the fixed title at y 158 and two 12-pixel description lines starting at y 178. This layout permits readable text on the calculator screen and leaves the Proceed hit starting at y 205 unobstructed. Source RGB is darkened before Canvas normal/additive composition, with independent alpha and transparent holes. A white multiplier preserves the old default path.

## Source12 short diagnostics and prepared execution

Independent read-only review found no blocking issue in one-draw ownership, parent/child tween ordering, pointer/keyboard separation, state/save purity, hidden claimed holders or Dolly's continuation and uninterrupted chest clock. Eight short diagnostic commands completed with unchanged inspected source:

| Short diagnostic | Executed result |
| --- | --- |
| Three changed C++ translation-unit syntax checks | passed, 22.662 s |
| Persistent RGB565 blend helper under UBSan | 2,773,149 checks passed, 0.088 s |
| Extracted real treasure timing helpers under UBSan | 28,331 checks passed, 0.023 s |
| Extracted real Canvas paint/picture paths under UBSan | 6,209,280 checks passed, 0.233 s |
| Whitespace/diff check | passed |

The Canvas diagnostic includes normal/additive composition, optional coverage, dense/palette/sparse and scaled image paths, alpha holes and neutral-brightness byte comparison against source11. These are helper and syntax diagnostics; they do not execute the complete changed room or engine suites. Receipt `source12-treasure-short-20261021/attempt1.json` SHA-256: `9ba41a702421a267ebf2db0a0d03bd82d8fd38fcf9414c5638666691e8308a2c`; log SHA-256: `53cfcaecdf40546aa62941cccf85b3e0ab1179729b05a447c2ef2102db996c48`. The receipt includes source/helper/binary hashes and every command.

Persistent room cases now compare controller-only and App opening save bytes, hold named RNG unchanged, cover three delay boundaries and color/alpha pixels, reject a press crossing the gate, allow a 66 ms keyboard claim, retain the random stream across rooms/loads, and preserve Dolly/empty/wrap behavior. They need the source12 frozen build before a formal pass can be recorded.

`world_backend_driver` grows from six to **16 controlled actual-pack scenarios**. Five rest scenarios cover all characters, all four regions and all three acts, with lit/dark animation, save/load and exit. Five treasure scenarios cover early keyboard claim, split pointer-down/release across the gate, open-offer restoration, Dolly's pending canonical save/load and one duplicate, and Silver Crucible's actual empty chest. The JS runner replays the same separate pointer steps through real WASM exports and compares complete RGB565 frames and saves. C++ syntax and JS parse checks passed; runtime execution awaits the complete actual pack and rebuilt source12 native/WASM. Fixed relic model injection isolates acquisition and does not claim Dolly is a normal chest drop.

## Remaining work

Preserve source12 on a remotely read-back branch and freeze it before long builds. Finish the source10 46-suite journal without relabeling checkpoints. Rebuild source12 native/WASM/ARM, execute affected complete UI regressions, finish the existing private conversion, validate the assembled pack and inspect actual screenshots, then run all 16 exact native/WASM scenarios and package the matching deliverables.

Original hover/press scale, rarity glow/particles, empty SPRING text movement, relic flight to inventory and first-time tutorial remain unfinished. Map entry/circle/marker transitions and GPU noise/filter/particle differences also remain documented. Backend agreement is not original-game parity, and neither ARM compilation nor the instruction oracle proves physical-device performance. Retain the completed 0.16 delivery pair until the new runtime and complete matching pack are verified.
