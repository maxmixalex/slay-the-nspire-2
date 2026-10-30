# UI17 deferred artwork I/O — measured implementation

This work addresses the measured UI16 startup cost of opening and reading all
eight artwork headers before any scene uses them. It does not alter image
bytes, animation timing, resource layouts, the image arena, or the four-slot
artwork handle budget.

## Measured cause and baseline

The restored user CXII CAS runs OS 6.4.0.74 with Ndless r2022 and the supplied
unmodified Firebird core `b10f3b51a9ab8e27d2703444b5e6bd278d4c5e6e`
at 396 MHz. The ordinary UI16 launcher matches SHA-256
`cf83955453bc6046e57895a94efdf68ecf00a6c6e4ba2eec5e3a172c94f36158`.

In this restored installation the unchanged launcher reports 49,754 startup
ticks (1,518.372 ms, 32,768 Hz). An optional per-syscall trace attributes
796.265 ms to the eight eager artwork probes: each first `nuc_fopen` costs
about 61 ms, and five first 32-byte reads each cost another 61 ms. All probe
seeks/tells combined take less than 0.5 ms. The common index open before the
existing startup timer costs another 61.43 ms and must not be omitted when
discussing complete launch latency.

This restored baseline differs from historical UI16 timings despite the
identical launcher and resources. Candidate claims therefore require the same
saved ordinary-OS snapshot, matching NAND, file install and input schedule;
the historical measurements are context only.

## Production change

The Ndless host first calls the official r2022 `nuc_stat` syscall for each
required part. The exact 36-byte OS structure (size at byte 20) is declared
separately from host/newlib `stat`. Missing files and sizes outside the valid
32-byte-header/maximum bounds still fail at startup. Index/span bounds are
still validated before a virtual artwork source becomes available.

Each callback-backed part validates its actual NSS1 header immediately before
its first payload read. This checks magic, version, reference ID, header size,
physical length, part identity and reserved fields. No payload is returned
on a failed header. Validated immutable headers survive normal LRU eviction;
reinitialization and failed payload streams reset the relevant validation.
Multipart's deferred API rejects direct providers, which could bypass that
contract. Eager callers retain the original validated-header API.

Both the first Ndless render and later frames check `Content::healthy()`
before presentation. A failure closes resources and displays the existing
built-in English resource error. Independent review found a separate window
when preparation fails in `advance()` before queued input is dispatched.
The final integration must also include the resource-failure/write guard;
render-only checks do not establish that queued Save cannot write after a
preparation failure.

Metadata timing remains inside the original startup scope. Resource timing
includes metadata, cold opens, header checks and requested reads. Read/byte/
seek counts describe actual provider calls, including the first header read;
an open or seek failure cannot invent a physical read. Input collection stays
before/after metadata and handle operations and at existing read boundaries.

## Completed isolated validation

The short focused regression passes 3,547 assertions. It covers eager
compatibility, metadata failures, every header field, short/failed header
reads, later short reads/retry, reused FILE addresses, LRU eviction,
reinitialization, complete virtual bytes, simultaneous old/new locale and
state streams, balanced timing observation and input-poll boundaries. The
native replay driver now offers a deferred-provider mode and both English
and Chinese full-frame/canonical-state comparisons.

The same 3,547 assertions pass AddressSanitizer and UndefinedBehaviorSanitizer.
LeakSanitizer itself cannot run under this environment's ptrace restriction;
that failed invocation remains in its journal, followed by the explicit
ASan/UBSan run with only leak scanning disabled.

The isolated ordinary ARM launcher is 1,761,665 bytes (+1,048 versus UI16),
with a 22,524,620-byte allocation (+1,136). Its `.data` and `.bss` sizes are
unchanged. The main-function stack decreases from 1,400 to 1,248 bytes.

Native replay mode 3 matches mode 2 for both English and Simplified Chinese:
all 182 complete RGB565 frames, all canonical serialized States and every
logical resource request are byte-identical. The first harness combined
buffered stdout with unbuffered stderr, placing a numeric summary inside one
Chinese CSV row. Its failed comparison is retained; a separate verifier
removes only that explicitly recognized summary and validates every complete
request row. Pixel and State bytes were already identical in the original
outputs; no frames were rerun or selected.

| Same restored environment | UI16 ordinary | Deferred headers `e3d8858` |
| --- | ---: | ---: |
| Selected validation + App init, ticks | 49,754 | 39,652 |
| Selected validation + App init, ms | 1,518.372 | 1,210.083 |
| Document Enter to first fully visible menu, ms | 13,380 | 13,060 |
| Six-second five-Dazed action frames | 169 | 169 |
| Action resource ticks | 8,931 | 8,929 |
| Action provider reads / bytes / seeks | 19 / 113,070 / 19 | 19 / 113,070 / 19 |

Startup improves by 308.289 ms (20.30%). The end-to-menu observation uses
20 opaque menu-glyph pixels and the unchanged core's 10-ms event cadence;
it includes the original intro presentation and loader, and subsequent Down
navigation is confirmed on the LCD. The 320-ms improvement therefore is not
work hidden outside the existing startup timer. No combat speedup is claimed
for this I/O change: the action frame count and resource work are unchanged.

Private evidence is under `private-ui17-device/`: `baseline-ui16-*`,
`deferred-ui17-*`, the corresponding `*-launch-watch/` receipts, and
`deferred-native-oracles/verified-receipt.json`. Baseline and candidate use
the same prelaunch RAM snapshot, matching NAND, resource bytes, CPU frequency
and keypad schedule. The optional trace is diagnostic only and is excluded
from the ordinary release launcher.

## Rejected directory-enumeration follow-up

An independently frozen candidate `a8f92dbaad2fcc0a4ecf17020f4e4c8710270832`
replaced eight `nuc_stat` calls with official `nuc_opendir/readdir/closedir`
filename discovery, derived expected lengths from the validated NSM spans,
and proved actual file length plus header at first use. It passes 12,179
focused checks, ASan/UBSan and the 10,075 existing storage-source checks.
Both actual UI16 and final retained-artwork installations have 89 spans and
all eight derived maximum physical ends exactly equal their actual NSS1 file
sizes; no padding exception was hidden.

It is **not selected for production**. Ordinary startup takes 41,515 ticks,
56.854 ms slower than `e3d8858`. Per-syscall attribution shows required
directory entries themselves taking about 60.46 ms each; enumeration totals
545.532 ms. Discovery saves time in a later locale-name probe but adds a cold
read in `Split.open`, so reduced API count does not improve elapsed startup.
After the eighth required name, the only remaining entry takes 0.0305 ms and
EOF takes zero ticks. Stopping there cannot offset the measured regression,
so no further enumeration experiment is warranted. Its source, bundle and
raw evidence remain separate from the selected production path.

Ordinary-OS Firebird error exercises on that branch use the same render-error path:
a wrong cards-part ID gives English `RESOURCE LOADING FAILED` on first use;
a missing shared-part name gives English `RESOURCES NOT FOUND`. ESC returns
to ordinary OS, immediate no-USB reopening reproduces the appropriate error,
and ESC returns again. Exact post-exit RAM and matching NAND are then reopened
with a fresh frontend USB connection for readback: the 230,352-byte run and
17,591-byte progress equal the original fixture byte for byte. The initial
automation attempts that selected the uploaded data document, used Enter
instead of the displayed ESC action, or reused a stale USB host session are
retained as failed/inapplicable UI gates. These render-failure exercises do
not replace the final guard's queued-Save/preparation tests.

Remote branch updates were rejected by automatic approval review in this
session; the root agent owns that publication boundary. The source checkpoint
and a local source bundle are preserved before long validation. Private ROM,
NAND, original game resources, snapshots and recordings remain outside git.
