# Calculator run storage and three-profile budget

The simulation writer remains canonical NSV1, save version 17. The calculator
host now writes a smaller, lossless NSZ1 envelope when it reduces the complete
file length. Desktop and Web saves, old raw-file migration, optional NSH history,
and simulation values are unchanged.

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

## Verification status

`ndless_run_storage_regression` exercises the exact host implementation with
bounded packets, raw fallback, original v3–v13 fixtures, current v17 files,
canonical equality, source immutability, short reads/writes, header/literal
truncation, output overflow, trailing input, close failures, rotation/rollback,
and shared-transfer conflicts. It can export its valid dense and capacity files
with `--export <existing-directory>` for calculator installation tests.

At source-ready time, the two changed translation units passed short native
syntax checks and both capacity/density measurements passed. Full logged native,
portable, and device validation must use the next remotely preserved checkpoint;
this source-ready note does not claim those future gates have run.
