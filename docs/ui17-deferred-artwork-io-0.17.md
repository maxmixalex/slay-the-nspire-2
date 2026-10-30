# UI17 deferred artwork I/O — candidate validation

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
before presentation. A failure closes resources, displays the existing
built-in English resource error and stops input dispatch. This host error path
does not invoke mutable-save callbacks.

Metadata timing remains inside the original startup scope. Resource timing
includes metadata, cold opens, header checks and requested reads. Read/byte/
seek counts describe actual provider calls, including the first header read;
an open or seek failure cannot invent a physical read. Input collection stays
before/after metadata and handle operations and at existing read boundaries.

## Validation checkpoint

The short focused regression passes 3,547 assertions. It covers eager
compatibility, metadata failures, every header field, short/failed header
reads, later short reads/retry, reused FILE addresses, LRU eviction,
reinitialization, complete virtual bytes, simultaneous old/new locale and
state streams, balanced timing observation and input-poll boundaries. The
native replay driver now offers a deferred-provider mode and both English
and Chinese full-frame/canonical-state comparisons.

Sanitizers, ARM layout/stack and matched ordinary-OS execution are pending
at this source checkpoint. This document does not claim performance success
until those measurements are recorded. The optional trace is diagnostic only
and is excluded from the ordinary release launcher.

Remote branch updates were rejected by automatic approval review in this
session; the root agent owns that publication boundary. The source checkpoint
and a local source bundle are preserved before long validation. Private ROM,
NAND, original game resources, snapshots and recordings remain outside git.
