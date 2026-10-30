# UI17 integrated calculator delivery — 2026-10-30

Reference: the owner's pinned **v0.111.0 / 41cef1ea**. Dates in this ledger
are reported for Asia/Shanghai. The restored, complete UI16 checkpoint
`1526ede2a84e168cd46d430338e5d5a540ecaf18` is the baseline; the older public
UI14 branch is not used as a substitute baseline.

## Frozen program and resources

The ordinary release launcher is built from
**`d4303baf9a720bbeed58c1bb1859ff63343afcd6`**. It contains the retained
UI16 feedback fixes, exact constant-color rendering, hover-list reuse,
resident-directory compaction and address-table reuse, the TGI2 decoder,
Unicode/raw legacy seed semantics, the selected `e3d8858` deferred artwork
headers, and the final resource-failure write/input guard. It does **not**
contain the rejected `a8f92db` directory-enumeration experiment.

The complete sixteen-language resource conversion is recorded at
**`44b239e5f41208929661ca70c96545ff65903e08`**, following the verified TGI2
stage. Later source commits add acceptance records and license notices;
they do not alter the tested C++/ARM program or resource bytes. The final
source archive records its separate documentation/history tip.

| Program identity / budget | Final value |
|---|---:|
| Launcher | 1,766,579 B |
| SHA-256 | `1a9a0fb8de2ba7f99a43ccbef3a21d915e72536c9eb62782634a3abdb6f83459` |
| Zehn total allocation | 22,269,692 B |
| Loader metadata | 157,220 B |
| Contiguous execution/static memory | 22,112,472 B |
| Absolute relocations | 39,263 |
| Complete picture pool | 13,369,344 B |
| Ndless main static stack | 1,256 B |

Total allocation is **253,792 B lower** than UI16. Both the 22,450,000-byte
execution budget and the 24 MiB total budget pass. These are loader requests,
not a physical peak-RAM measurement or a guarantee for arbitrary OS heap
fragmentation.

## Actual installed storage

Counts include the program and matching required resources; saves and notices
are excluded consistently. Decimal bytes and MiB are not interchangeable.

| Installation | UI16 | UI17 | Actual reduction |
|---|---:|---:|---:|
| Default English + Simplified Chinese | 81,022,714 B | **79,957,013 B** | **1,065,701 B (1.3153%)** |
| All sixteen languages | 99,576,281 B | **98,499,241 B** | **1,077,040 B (1.0816%)** |
| Common index + eight artwork concerns | 74,026,106 B | 72,957,060 B | 1,069,046 B |

The default installation is **76.252950 MiB**. The earlier **65–70 MB**
installation target remains unmet. No smaller estimate, compressed ZIP size,
or intermediate launcher is substituted for this installed total.

The common index is 1,520 bytes and maps 89 spans. The eight required parts
have distinct concerns:

| Data file | Bytes |
|---|---:|
| `nSpire2.interface.dat.tns` | 10,786,410 |
| `nSpire2.cards.dat.tns` | 2,904,812 |
| `nSpire2.characters.dat.tns` | 1,219,174 |
| `nSpire2.enemies.dat.tns` | 34,059,934 |
| `nSpire2.bosses.dat.tns` | 8,696,798 |
| `nSpire2.rooms.dat.tns` | 4,654,788 |
| `nSpire2.effects.dat.tns` | 10,500,882 |
| `nSpire2.shared.dat.tns` | 132,742 |

English is 779,148 B; Simplified Chinese is 4,454,226 B. The other fourteen
language files total 18,542,228 B and are optional. All eight artwork parts
are required, regardless of the selected locale.

### Implemented compression and memory changes

- **TGI2 frame indices:** 496 of 500 index groups use a smaller reversible
  predictor, saving 807,004 artwork bytes. Existing pixel streams, group
  ordering, decoder workspace and provider boundaries are retained. The
  format advances to NSP2/41; the runtime still accepts its supported older
  versions.
- **Previously unfiltered retained images:** 608 of 956 eligible complete
  RGR1 groups now use the existing reversible RGR2 filter, saving another
  262,042 artwork bytes. This stage adds no decoder or runtime pool. Complete
  encoded spans, including their headers and alignment, must actually shrink.
- **Resident image directory:** streamed image rows use 12 bytes instead of
  16. Direct-memory packs retain their original layout. Existing foreground
  scratch handles 64 KiB chunks; prepared costs reuse owned directory memory.
  The final 1,835,012-byte resident budget fits every actual edition and the
  existing 65,535-image maximum fixture. Some arbitrary old synthetic 2 MiB
  layouts with fewer images and excessive unused group rows may be rejected.
- **Functional address table:** retaining at most 256 bytes avoids repeatedly
  reading NSL2 rows through the NSL3 page cache. On the unchanged UI16 English
  inputs, split/content initialization changes from 42 reads / 246,312 B /
  17 seeks to 36 / 218,380 / 13. These component I/O counts are not device
  timing claims.
- **Exact drawing and hover preparation:** large constant-color dim rectangles
  compute their RGB565 blend map once; inactive hover slots are no longer
  repeatedly reconstructed. The full picture pool and existing frame/pixel
  content are retained.

The retained duplicate audit and local 8×8 subtile experiment are preserved
as investigations, not delivered savings. All 24 weighted subtile samples
were larger; replacing them would add 99,132 B. The directory-enumeration
startup experiment also passed correctness but was rejected for worse actual
OS latency. See the [artwork investigation](ui17-artwork-investigation-0.17.md)
and [selected I/O report](ui17-deferred-artwork-io-0.17.md).

## Correctness and original behavior

All final named gates below have `status: passed` and `source_changed: false`.
Earlier failures, interrupted commands, unfinished work and the invalid
checkpoint receipts remain separately identifiable in the source verification
index; none is counted as a final pass.

| Gate | Result / journal |
|---|---|
| Full native build | 317 integration steps, then all 8 final affected steps; `ui17-final-native-build-5ee5ea070466` |
| Full native tests | **152/152**, 75.72 s; `ui17-final-native-ed0aab32d458` |
| Full ordinary ARM build | `ui17-final-arm-26754362f1c9` |
| Changed retained/package Python tests | **21 tests**; `ui17-final-storage-python-112772d4e431` |
| Actual sixteen languages | **89,816 checks / 53,072 descriptions / 0 failures**; `ui17-combined-actual16-023472249d7c` |
| All actual images and catalogs | **169,595,016 checks**; `ui17-combined-all-images-095d01a5edd3` |
| Final ENG/ZHS provider replay | `ui17-final-bilingual-provider-oracles-1f6c91fbfa97` |
| Final calculator/optional ZIPs | `ui17-final-calculator-package-9341796a15da` |

The complete image gate compares **58,060 images / 1,167,930,140 decoded
bytes** through both new memory and ranged paths against the independent old
UI16 output, plus 13,299 glyph records, 7,124 kerning pairs, 56,019 exact-size
samples, 8,704 strings, 1,783 clips and 40 complete UI frames. It runs at
`1ec2707`; the relevant native core and data bytes are unchanged at final
`d4303ba`. A sequential byte spool avoids interleaved cache-owner thrashing;
it does not replace byte comparison with hashes.

At exact final production `d4303ba`, each of English and Chinese replays 182
full frames. New eager/deferred providers match every 27,955,200 RGB565 byte
and every 41,966,288 canonical-State byte, then compare directly with the old
UI16 result. English also matches the earlier independent old-runtime replay.
All 182 frame labels and State boundaries are checked. The new providers'
logical sequences match; relocated old/new resource offsets are not expected
to match. See [resource acceptance](ui17-resource-acceptance-0.17.md).

The final native boot suite passes **137,145** checks. It retains saved
preference precedence, system-language selection, English fallback, other
usable editions and built-in English technical errors. Unsupported calculator
languages use English when available; a healthy explicit saved choice still
wins. Keep English installed. Actual French/Dutch OS coverage is recorded in
the final device section rather than inferred from this native matrix.

The [feedback ledger](ui17-feedback-acceptance-0.17.md) reconciles all nine
reported items. It preserves the original pointer-only upgrade preview,
Tablet/Messy versus rest/Smith effects, and map control sizes. The 8 px deck
sort labels, upper-left tucked card at (18,78), original locale font roles and
treasure ribbon title correction remain implemented.

The previously missing [Unicode and raw legacy seed behavior](seed-unicode-ui17-0.17.md)
is now connected to input, preview, run creation, RNG streams, save and
Continue. Original methods supply 31,209 vectors and all 1,112,063 non-NUL
valid Unicode scalars; native and focused sanitizer checks each pass
3,354,588 assertions. The existing field remains 64 UTF-8 bytes. Earlier
monster-command/Vantom omissions were already closed in UI15 and are retained,
not counted again as newly implemented UI17 features.

### Failure handling and scoped sanitizers

The selected stat/deferred-header path validates each required part's presence
and size at startup and its functional header before first payload use. A live
resource failure is latched before the next input sample; the actual run,
history, clear, settings, profile, language and feedback write callbacks reject
the failed session even when called inside the same App operation. Normal
optional-language failure can still roll back fully to the prior locale.

The new real-App/actual-host-callback regression passes 170 checks, as does
its focused ASan/UBSan build. Existing run/history/profile transactions pass
1,397 / 9,463 / 259 checks. Fault detection prevents subsequent state writes;
it does not undo a normal language-preference write made before a later fault.
Diagnostic performance output may still be written on exit. See
[the failure guard report](ui17-resource-failure-0.17.md).

The exact Canvas/Content sanitizer run passes **330,242 cases / 2,951,469
checks**, including all RGB565 destination colors and call alpha values.
Directory, addressing-table, provider and seed stages have their own scoped
sanitizer receipts. These are not presented as one unexecuted full-game
sanitizer run. LeakSanitizer alone is disabled for the environment's ptrace
limitation; address and undefined-behavior checks remain enabled.

## Final ordinary-OS Firebird evidence

Configuration is **CX II CAS / 396 MHz / OS 6.4.0.74 / Ndless r2022** with
the unchanged upstream Firebird core
`b10f3b51a9ab8e27d2703444b5e6bd278d4c5e6e` and private owner-supplied images.
The same restored installation provides the new UI16 baseline. Historical
UI16 figures from a different installation state are not substituted here.

The final measured launcher and transferred data match the frozen identities.
The controlled six-second window is the same Haunted Ship / five-generated-
Dazed action; it is not a natural three-act run.

| Matched measure | UI16 baseline in this session | Final UI17 |
|---|---:|---:|
| Selected-resource validation + App init | 49,754 ticks / 1,518.372 ms | **36,191 / 1,104.462 ms** |
| Action frames / accumulated interval ticks | 169 / 196,433 | 171 / 196,905 |
| Whole-window FPS | 28.19176 | **28.45701** |
| Mean elapsed render stage | 24.88862 ms | 24.63090 ms |
| Resource elapsed time | 272.55249 ms | **213.16528 ms** |
| Physical reads / requested bytes / seeks | 19 / 113,070 / 19 | 19 / 113,079 / 19 |
| Intervals over 50 / 100 / 250 ms | 4 / 3 / 0 | 4 / 2 / 0 |

Startup's selected-resource stage improves **413.910 ms / 27.2601%**. It
excludes complete Zehn launch and the intentional logo sequence. The action
window improves **0.9409% FPS**; resource elapsed time falls 59.387 ms / 21.7893%.
Render elapsed time can include synchronous resource work; it is not a CPU
profiler. Finite exit-boundary frames may differ, and maxima cannot be
subtracted between paired aggregate reports. Native provider counts are kept
separate from these actual OS counts.

Final cold/warm lifecycle, French, Dutch and corrupted-part checks are being
completed with this frozen program. Their final receipts will replace this
paragraph before the delivery is closed. Cold scene preparation can still
stall; the measured warm window does not establish universal 30 FPS or zero
cold stalls. No physical-handheld timing claim is made.

## Packages, recovery and publication

| Archive | Bytes | SHA-256 |
|---|---:|---|
| `Slay-the-Nspire-2-0.17-ui17-bilingual-calculator.zip` | 79,127,539 | `750caa91e50efc8f9c0f7234b28806c6d47995676f326dc5fa4c9bf2776466a5` |
| `Slay-the-Nspire-2-0.17-ui17-optional-languages.zip` | 17,072,413 | `b4ec488a309742d103eba8429985949ebbf5b726b05eedc0d6cceaca2de7f801` |

The main ZIP has 23 members and the optional ZIP 15. Every archive member is
read back against its input. The only state member is an empty `state/`
directory: no test save, profiling marker or preference is distributed.
The complete Unicode seed-table notice is included. Optional HTML source
packaging also includes the notice, but no HTML/WASM player was built or
manually validated in this round.

Preserve the user's existing `state`. Install the matching launcher, index,
all eight concern files and desired languages before launching. For an older
monolithic version, replace its large `data/nSpire2.dat.tns` with the small
index first to release its occupied storage. Keep English for fallback;
installing all sixteen languages is unnecessary.

The source recovery ZIP contains committed source, the complete all-ref Git
bundle, source identity and source-only verification records. It preserves
failed and abandoned experiments with their original statuses. The separate
device evidence archive carries actual measurements and screenshots without
ROM, NAND, original assemblies, fonts or game packs. Original game assets
remain private and are not part of the public source.

Public source synchronization remains **blocked by automatic approval review**.
The historical repository authorization was recovered this turn, but the new
review still rejected the outbound source/history push as insufficiently
authorized for the current payload/destination. No alternate transport was
used to bypass that decision. No new remote ref, main synchronization or CI
pass is claimed. The final reviewable source archive preserves the work.

Accepted Ancient static-scene and excluded audio/multiplayer/Timeline scope
remain explicit. Other disclosed boundaries include LCD RGB565/material
sampling, bounded particles, independent cosmetic RNG, UTC calendar and
finite history. These are not recast as user-approved universal equivalence.
The delivery does not claim a measured restoration percentage, exhaustive
desktop-Godot equivalence, arbitrary heap-fragmentation support or a naturally
completed three-act victory.
