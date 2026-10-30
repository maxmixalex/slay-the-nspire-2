# UI17 source-only delivery review

Reviewed production: `d4303baf9a720bbeed58c1bb1859ff63343afcd6`.
Incremental baseline: recovered UI16
`1526ede2a84e168cd46d430338e5d5a540ecaf18`.
Review date: 2026-10-30 UTC. This receipt records read-only source/history
inspection; it is not a runtime, original-game parity or device-performance gate.

## Scope and observed contents

The final production tree changes **98 paths** from that baseline: **40 added**
and **58 modified**. Separately, the seventeen UI17 source bundles listed below
expose **43 distinct tip commit IDs**. The union reachable from those tips,
excluding objects reachable from the UI16 baseline, contains:

| Git object type | Count |
| --- | ---: |
| Commits | 115 |
| Trees | 355 |
| File-content blobs | 397 |
| Total | **867** |

The 397 blobs total **8,051,054 bytes**. All decode as UTF-8 text and contain
no NUL bytes. The review checked their paths, content categories and binary
signatures. No original PCK/DLL, installable font, Boot1/ROM/NAND, derived
`.tns`/`.nsp`, private generated full oracle, screenshot or other binary payload
was found in this increment. Source tables, source-only converters and synthetic
fixtures were inspected as code/test material, not classified by a filename
substring alone. All seventeen bundles passed `git bundle verify` against the
available repository history.

The public seed fixture is **2,032 bytes / 30 input-output vectors** using
synthetic strings and deterministic numeric results. The optional
`tools/seed_reference_oracle` contains only its runner/project configuration;
the original assemblies, isolated execution copy and complete generated
scalar/prefix/vector outputs remain outside Git. Its source does not embed
original seed method bodies. This is an audit of the stated incremental
object set, not a new per-blob audit of the entire inherited project history.

## Notices and selected implementation

The source contains the project GPL text, existing Go BSD/Sun/Cephes notices,
Godot/OkHSV MIT notices, Noto OFL notices, the pinned public-domain LZMA SDK
source notices, and the complete Unicode License V3 at
`third_party/unicode/LICENSE`. The seed table headers point to that license.
`tools/package_release.py` includes it at the fixed calculator ZIP path
`data/licenses/Unicode-License.txt`. This documentation follow-up adds the
Unicode table provenance to `NOTICE.md` and removes an obsolete decoded-CRC
requirement from the LZMA2 wrapper README; the wrapper's exact decode/end-marker
requirements are unchanged.

At the reviewed production commit the selected artwork path is **e3's
`nuc_stat` plus deferred NSS1 header validation**. There is no production
`include/resource_directory.hpp`. The `a8f92db` directory-enumeration experiment
is retained only in its branch/bundle history and must not be described as the
selected launcher implementation.

The inspected UI17 reports distinguish original UI16 screenshots and timing
receipts from new component work. In particular, the 196-input UI route,
French/Dutch defaults and missing-part captures retain their historical
runtime identity; intermediate launcher totals and native microbenchmarks do
not become final combined calculator results. The e3 I/O report still records
its candidate-stage pending gates at this production checkpoint; the final
integration/device ledger owns its superseding acceptance and exact totals.

At the reviewed production checkpoint, the optional offline-HTML notice list
in `tools/build_web.py` had not yet added the Unicode license, although its
WASM source list already compiled the same `src/reference_rng.cpp` tables.
The authorized packaging-only follow-up adds that one license path to its
embedded notice list. Python syntax and the complete notice inputs were
checked; no HTML or WASM was built and no browser acceptance was performed.
The current delivery remains calculator-only, with the frozen runtime unchanged.

## Reviewed UI17 bundles

These names are relative to the recovered working area, not files embedded in
the public source tree:

- `ui17-resource-final-source.bundle`
- `ui17-integrated-source.bundle`
- `ui17-index-validation-source.bundle`
- `ui17-directory-source.bundle`
- `ui17-retained-source.bundle`
- `ui17-root-source.bundle`
- `ui17-storage-source.bundle`
- `private-ui17-device/ui17-io-diagnostic-source.bundle`
- `private-ui17-device/ui17-perf-validation-source.bundle`
- `private-ui17-device/ui17-deferred-artwork-source.bundle`
- `private-ui17-device/ui17-directory-startup-source.bundle`
- `private-ui17-reference/directory-table-final.bundle`
- `private-ui17-reference/directory-table-stage.bundle`
- `private-ui17-reference/seed-c45d886.bundle`
- `private-ui17-reference/seed-final-ui17.bundle`
- `private-ui17-reference/ui17-acceptance-doc.bundle`
- `private-ui17-reference/resource-failure-final.bundle`
