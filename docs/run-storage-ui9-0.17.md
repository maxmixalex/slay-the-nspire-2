# Calculator run storage and three-profile budget

The simulation writer remains canonical NSV1, save version 17. The calculator
host writes a smaller, lossless NSZ1 envelope when it reduces the complete
file length. The same bounded envelope now also stores optional calculator
histories. Desktop/Web files, canonical NSV/NSH bytes and simulation values are
unchanged; either raw or wrapped calculator files remain readable.

## Format and memory contract

The envelope starts with the four ASCII bytes `NSZ1` and the decoded byte length
as a four-byte little-endian integer. Each following control byte describes
1–128 decoded bytes: its low seven bits are length minus one, and its high bit
selects a zero run. A literal control is followed by that many original bytes.
The writer starts a zero packet only for a run of at least two zero bytes. Both
literal and zero packets stop at 128 bytes.

The first pass counts the encoded length. If the eight-byte header plus packets
is not strictly smaller than the canonical file, the host writes the canonical
file directly. Thus the format never increases a run's file size. The counting
and writing passes are linear, bounded by `SaveCapacity`. An all-literal stream
would need `N + ceil(N / 128) + 8` bytes; raw fallback prevents that expansion
from appearing on disk.

The writer and reader each use one 512-byte automatic I/O block. They reuse the
existing foreground `ResourceTransferUse::Save` buffer; no resident allocation,
additional large staging buffer, compression dictionary, or hash is introduced.
The decoder validates the decoded length, alignment, every output span, literal
availability, and exact end of input. It then passes the unchanged bytes to the
existing transactional `load()` validator. A short file, malformed packet,
trailing byte, invalid canonical save, or close failure cannot publish a State.

## Existing file and failure behavior

`save_game()` and `load_game()` are extracted into
`platform/ndless/run_storage.inc`, which is included directly by the calculator
host and its independent fault-injection test. Existing NSV1 files are detected
without a migration rewrite. A successful save writes and closes the temporary
file before rotating the primary into the backup; the prior run backup remains
available after success. Failed temporary writes and closes preserve committed
files. Failed final renames attempt to restore the previous primary and keep a
recovery copy if the rollback also fails.

Restore keeps the existing **primary-only** policy. The envelope does not
silently load a different run from the backup. All file closes that gate a
restore or rotation are checked. Optional history still follows its existing
successful-save cleanup policy, which removes its transactional backup.

## Exact raw-file bounds

The final v17 codec has this byte-length formula:

```
R = 230000 + 8D + 12C + 4S + 16E + 12L + 4Q + 36A
    + align4(4 + 2P)
```

`D` is deck count; `C` is combat-card count; `S` is the total of the five physical
pile counts; `E` is effect count; `L` is log count; `Q` is selection count; `A` is
automatic-play depth; and `P` is the reconciled live power-order count. Validated
limits are respectively 192, 512, 512, 512, 24, 512, 12, and 1,118. This gives a
strict conservative raw bound of **252,928 bytes**, distinct from the 262,144-byte
buffer capacity. It does not assert that every capacity and power can coexist
in a naturally played run.

The existing full NSH7 history fixture is 287,075 bytes. Filling its remaining
used-length data—2,048 map drawing points and four sets of ten discovery names—
adds 8,192 + 80 bytes. The actual encoded/decoded maximum fixture is therefore
**295,347 bytes**. A profile can retain 32 completed histories plus its current
run, each with 96 floors, 192 cards, 320 relics, and the shared 4,096-entry journal.

Three preferences files add 3 × 108 bytes, and global profile/language choices
add 8 + 8 bytes: **340 bytes** total. File-system allocation and metadata remain
additional device-dependent costs.

## Current measured compression, not a universal compression promise

A private matched-core size probe used the final 8c2 checkpoint
`a0df79d8347b76dc86804b0ab8d17f948c600204`. It reproduced the original valid
bounded fixture at 250,620 bytes and the fully populated variable-count variant
at 250,964 bytes. Both survived canonical save/load/save byte equality.

The new exact-host test adds a denser valid state: all 512 combat cards have
nonzero cost/retention/negative signed metadata and enchantments, all 320 relic
records have random nonzero values, all 32 hook frames contain 320 serials, and
all 128 reward records contain eight cards. It remains subject to `validate()`
and exact canonical reserialization. These are engineered capacity fixtures;
they are not presented as naturally played runs or proofs of the maximum
compression ratio of every valid combination.

Short pre-checkpoint measurement of the new host implementation:

| Fixture | Canonical bytes | Stored bytes | Write calls | Largest write |
|---|---:|---:|---:|---:|
| All variable counts, 16 Nightmare instances, 8 enemies with 15 status types | 250,964 | 15,456 | 31 | 512 |
| Dense nonzero/signed metadata, full relic hooks and rewards | 250,964 | 121,503 | 238 | 512 |

Let `H = 295347`, `C = 121503` for the measured dense file, and `R = 252928` for
the unconditional canonical bound. Ordinary three-profile history-save peaks
are then:

| Storage state | Peak including settings | Bytes |
|---|---|---:|
| All run primaries and backups use the measured dense envelope | `4H + 6C + 340` | 1,910,746 |
| Three compressed primaries, three old raw backups | `4H + 3C + 3R + 340` | 2,305,021 |
| Unconditional raw-fallback upper bound | `4H + 6R + 340` | 2,699,296 |

The migration line matters: the first new save can retain a raw predecessor,
and the second successful save naturally replaces it with the previous
compressed primary. Nothing proactively rewrites old files or removes their
backup. If an already-full installation attempts a history write before its
run has migrated, that existing I/O failure remains possible. A rounded device
display of “2.4 MB free” cannot establish exact available bytes.

Keeping three *additional permanent history backups* would be a different
retention policy. The current successful history path does not do that; the
unconditional peak for that policy would be `7H + 6R + 340 = 3,585,337` bytes.

### Subsequent tutorial metadata

The combined tutorial checkpoint
`492915341106b129f730e46ed84d152637a24b7d` adds two bytes to each encoded history
for NSH8; it does not change NSV1/v17 or NSZ1. Its matching C++ export passed
6,413 persistence checks and measured **H8 = 295,349 bytes**, with a full
decode/re-encode comparison. Receipt: `ui9-ftue-progress-export-adc5a937cfbb`.
That valid file, SHA-256
`4c3628949e7de487aa449744e6b469de25ff2743630b77798a61158a18efe725`, is the
input supplied to the separate calculator capacity test.

The ordinary history-update peak now uses `4H8`, increasing each previous
three-profile figure by eight bytes:

| Storage state | Peak including settings | Bytes |
|---|---|---:|
| Measured dense compressed primaries and backups | `4H8 + 6C + 340` | 1,910,754 |
| Three compressed primaries and three conservatively bounded old raw backups | `4H8 + 3C + 3R + 340` | 2,305,029 |
| Unconditional raw-fallback upper bound | `4H8 + 6R + 340` | 2,699,304 |

Using the actual dense fixture's 250,964-byte raw predecessor instead of the
conservative 252,928-byte bound yields a migration peak of **2,299,137 bytes**.
These figures still exclude filesystem overhead. Successful native codecs or
exact byte budgets do not by themselves establish a successful device write;
that operation has its own ordinary-USB receipt.

## Calculator history envelope and recovery

`platform/ndless/storage_codec.inc` contains the shared byte-preserving codec.
Run reads retain their minimum of16 bytes and four-byte alignment. History
reads use their separate512KiB capacity, an eight-byte minimum and no alignment
requirement: the complete NSH8 fixture has an odd length of295,349 bytes.
Both readers reject output overflow, truncated literals, excess packets and
trailing bytes. The canonical run and history decoders remain responsible for
their own schemas. The packet writer is unchanged, including raw fallback
when compression would not reduce the file.

History loads, saves and profile summaries use the existing shared foreground
`ResourceTransferUse::Bytes` lease. The reader/writer has one512-byte automatic
I/O block; there is no new resident buffer, hash, dictionary or second
`UiProgress`. File length is obtained through seek/tell so a physical malformed
file can be distinguished from a short I/O read. This is bounded file metadata
and the normal decode, not an additional body integrity pass. Profile summaries
still decode only the canonical summary fields; they do not certify the late
history payload or update recovery facts.

One byte of process-only state records the result of the existing full history
load. It resets when profile paths change. A complete valid primary load or a
successful save certifies the primary. A fully read malformed primary followed
by a valid backup load records that specific recovery. Open, seek, tell, read or
close failures leave validity unknown. This state is neither simulation State
nor a new saved field.

Saving first writes and closes the new temporary file. A certified valid
primary follows the existing primary-to-backup transaction. If the primary
was certified malformed and the backup recovered successfully, the writer
removes only the malformed primary and preserves the valid backup until the
new primary commits. A failed final rename leaves that backup loadable; a
subsequent load/save can complete normally. A missing primary with a valid
backup follows the same non-destructive commit path. A failed normal rollback
also leaves the backup available for the next load. Once a new primary commits,
a failed optional backup cleanup does not invalidate the successful save.

With two existing files and uncertain I/O validity, the writer never rotates
the unverified primary over the recovery copy. It attempts only an atomic
replacement supported by the host filesystem. If replacement is refused, both
old files remain untouched and a later complete load can establish the safe
normal path. This keeps a transient I/O failure distinct from a certified bad
file without permanently locking histories that retain an ordinary backup.

### Concrete history measurement

The exact host test builds the complete valid NSH8 fixture, writes it through
`save_progress`, reads it through `load_progress` and compares canonical bytes.
Its current short-check result is **295,349 canonical bytes →74,001 stored
bytes**, with writes no larger than512 bytes. The separately decoded candidate
has SHA-256
`4294557707e07bce1e8830166d8eff3d0d2a79bb2b644b5f1e2f8a168a318067`.
The committed-source/full ARM and ordinary filesystem gates remain separate;
this measurement alone is not a successful device installation claim.

Let `P = 74001` for this concrete packed history, retaining the measured dense
run file `C = 121503` and conservative raw run bound `R = 252928` above:

| Storage state | Atomic update peak including settings | Bytes |
|---|---|---:|
| Three packed histories and dense compressed run primaries/backups | `4P + 6C + 340` | 1,025,362 |
| Three packed histories, compressed run primaries and bounded old raw run backups | `4P + 3C + 3R + 340` | 1,419,637 |
| First history update with three old raw full histories and those mixed run backups | `3H8 + P + 3C + 3R + 340` | 2,083,681 |

These figures exclude filesystem allocation/metadata and describe the stated
fixtures. A valid history with less compressible values can use more space;
the writer falls back to unchanged raw bytes when necessary. Old raw histories
are read directly and migrate only through ordinary successful writes. The
all-raw conservative peak remains2,699,304 bytes. Disk-full failures must retain
the last valid files, and actual installation/peak-write proof is still required.

`ndless_history_storage_regression` compiles the real profile/history callbacks
against a fault-injecting filesystem. It covers raw and wrapped NSH1–8, complete
NSH8 equality and summary routing, raw fallback, every prefix truncation of a
packed fixture, extra trailing bytes, malformed lengths/packets, guarded output
bounds, shared-lease reentry, all three profiles, temporary write/close failures,
primary/backup rename and rollback failures, and successful saves after backup
recovery. It also verifies that an uncertain backup close cannot authorize
deletion of a malformed primary. `--export <existing-directory>` exports both
canonical and real-host stored versions for the separate calculator capacity
test; an exported synthetic fixture is not an ordinary played run.

## Run verification status

`ndless_run_storage_regression` exercises the exact host implementation with
bounded packets, raw fallback, original v3–v13 fixtures, current v17 files,
canonical equality, source immutability, short reads/writes, header/literal
truncation, output overflow, trailing input, close failures, rotation/rollback,
and shared-transfer conflicts. It can export its valid dense and capacity files
with `--export <existing-directory>` for calculator installation tests.

The frozen storage checkpoint
`d938a661c7a69e46b829a4a879ce6cd411be0f2b` passed the exact-host test with
**1,410 checks**. Build receipt: `ui9-stage8d-run-storage-build-faa297b71185`;
execution receipt: `ui9-stage8d-run-storage-b7eee0b6d2b6`. It reproduced both
measurements above and exported the dense file with SHA-256
`83bc4ed5be2fb3016f4080ede942668aef764cf46bfd2b4d015058a027f107d7`.
These are native fault-injection results; final ARM/device and combined-build
verification remain separate gates.
