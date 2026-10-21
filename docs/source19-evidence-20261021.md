# Source19 evidence and next gates — 2026-10-21

The reference remains **v0.111.0 / 41cef1ea**. This stage follows remotely
verified source18 `5147ca97b377f662f363c61124e5022fd0277143` on
`work/0.17-world-parity`. Original assets and private reference material are
excluded from the public source tree.

## Completed source18 validation

All **471 source files and 12 changed file contents** matched the remote tree
and read-back. The source-only archive is **9,072,640 bytes**, SHA-256
`10bb7f4ac1ff7a6073579f7d3c4eddb5ca368e620c541e624011bdcf8e7d1968`.
The frozen runtime source fingerprint is
`39711020aa45d8439d84912feae79e5e9cfc7355d81aaa4e823e46d36534c9d1`.
See [validation-source18-0.17.json](validation-source18-0.17.json) for individual
journals, hashes and exact scope.

| Gate | Executed result |
| --- | --- |
| Native build | All 134 steps passed with ASan/UBSan enabled |
| ARM packaging | 863,848-byte launcher; 10,456,808-byte allocation; 28,566 relocations |
| WASM link | 4,840,209 bytes |
| Linked ARM division and ABI | 5,249 vectors passed |
| Six affected native suites | All passed; counts recorded in the machine-readable report |
| Actual original world text | 1,759 checks passed |
| Existing backend replay | Source18 WASM matched source17 native captures: 18 scenarios, 357 RGB565 frames and 25 saves |
| Actual Colossal Flower captures | Parameterized rainbow word rendered without markup; raw 0/1,000/10,000 ms captures and canonical saves checked |

This is a focused source18 verification, not another complete 52-suite run.
The full 52-suite baseline remains source17
`26a9097521a971b69a4b58494985c6062a24017e`. The older 18-case backend set does
not contain Colossal Flower. Its new case is part of this source19 stage.
The first source18 backend launch failed because a deliberately restricted
cross-compiler PATH lacked Node. That failed journal remains intact; a separate
corrected invocation passed. Ten successful journals and this one launch failure
are retained. The private Colossal capture's sine/jitter frame was fixed to
isolate rainbow time, and it is not an original-PC differential capture.

## Source19 implementation

### Act-entry camera and banner

The map now owns a bounded presentation state for the original first-open
camera sweep, its interruption window and independently composed act banners.
The ordinary camera holds for 1,000 ms and sweeps for 3,000 ms using the pinned
Godot EXPO_IN_OUT equation. Scroll can interrupt after 1,750 ms, followed by a
separate 200 ms input guard. Closing advances the normal close tail; a completely
hidden map pauses the node-bound camera/banner and reopening resumes them.
Top-bar opening consumes the introduction without the sweep. Fast and FTUE,
later-act duplicate banners, input distinctions, save/load conventions and
calculator typography are explicitly documented in
[world-map-act-intro-0.17.md](world-map-act-intro-0.17.md).

The pre-checkpoint pure test passed **61,072 checks**, with maximum error
**444 / 4,096 original source pixel** against continuous equations before
projection. The existing pure map-window suite passed **106,533 checks**.
Short C++17 syntax checks passed. These are not a complete App build or a
sanitizer run. Two new registered suites and four real fresh-run backend
scenarios await execution against the frozen source19 build.

The retained-event Proceed path now also reopens an already completed room's
map, even when no new `room_open` 1-to-2 transition occurs. Map cursor selection
happens after the old room's cursor clamp, including before the first render.

### Escapes and real deaths

Thieving Hopper, Fat Gremlin and Battleworn Dummy exits previously changed HP
to the removed-slot sentinel, which a shared death observer incorrectly counted
as an AfterDeath event. The escape path now consumes that observation edge
without firing death listeners, using the existing saved mask. A dead actor's
Escape command is a no-op, preserving pending death rewards and revival.
New-identity slot resets still allow a later real death to fire exactly once.

A focused ASan/UBSan diagnostic passed **683 checks**, including actual exit
sequences and a suspended Ovicopter/minion death cascade. The unchanged frozen
library reproduces the erroneous escape death. See
[enemy-exit-boundaries-0.17.md](enemy-exit-boundaries-0.17.md). The complete
174-variant enemy suite remains a source19 execution gate.

### Image and glyph cache identity

An image cache hit now requires the complete immutable 16-byte descriptor to
match, including geometry, stored format and encoded length. An offset alone
could previously alias distinct valid images or bypass malformed descriptor
validation. The existing key word holds the directory index, so descriptor
comparison needs neither additional cache storage nor range I/O. Exact
descriptor aliases continue sharing their decoded payload.

Glyph cache hits now require the same payload offset **and actual raw/RLE byte
length**. A shorter cached glyph can no longer supply an incompletely populated
longer prefix alias. Different code points and layouts still share identical
byte ranges. String-cache semantics are unchanged.

The short combined sanitizer diagnostic passed **392 checks**, including both
load orders, exact alias reuse, read failures, independent owners and reopening
changed payloads. Seven cache/buffer symbol sizes are unchanged; the memory
budget increase is **zero bytes**. Formal complete synthetic and actual-pack
content validation still requires the frozen source19 build.

### Public status and backend coverage

The README, parity document, development version and source HTML template now
describe the active five-character, 558-card original three-act work. Historical
0.12/0.16 counts are no longer presented as the current implementation. The
backend driver adds Colossal Flower plus ordinary, interrupted, top-bar and Fast
fresh-run introductions, for **23 planned scenarios**. Fresh runs use existing
production title/settings/character/Neow inputs and existing host exports.
No new scenario result is claimed before execution.

## Storage work remains separate from a release

The valid complete pack remains **231,312,495 bytes**, SHA-256
`d9adddc61c4f4f61b2ce50ee90960b112d752a694595ffa5f0feb2a6d9b204fb`.
It does not fit the documented calculator storage. Previous full lossless raw
group candidates measured 117,396,313 and 107,167,402 bytes, also too large.

A new exact-tile sample reconstructs **71,792,715 pixels** in sequential and
deterministically shuffled order for each of 8/16/32-pixel tile sizes. It covers
1,295 canonical frames from 57 raw groups, including complete Kaiser clips and
distributed large actors and rooms. With all sampled metadata accounted for,
the 16-pixel candidate is **5,203,414 bytes**, compared with the same sample's
9,035,482-byte raw-group baseline. This is **a sample, not a full-pack size**.

The full 28,910-canonical-frame trial is running separately. Its final receipt
must include all directory entries and timing aliases, actual compressed reads,
byte-exact reconstruction and bounded-cache costs. The provisional dense image
representation changes cache size classes, so a compact file by itself will
not establish acceptable multi-enemy runtime behavior. No production format or
reader is changed by these measurements.

## Required next actions

1. Push source19, read back its branch, every tree entry and all changed contents,
   then freeze the verified source and retain a source-only archive.
2. Build native, ARM and WASM; run linked ARM helpers and the affected complete
   App/map/enemy/content suites with the existing sanitizer configuration.
3. Render the actual pack through the new act-entry suites and all 23 backend
   scenarios, compare native/WASM frames and saves, and inspect the new captures.
4. Complete the full lossless size/read-cost measurement before choosing any
   production encoding. Keep any failed/unfinished journal as such.

Native/WASM equality is not original-game visual equality. There is still no
physical-calculator or Firebird execution result. Shared rich-text phase,
dynamic original effects and temporal animation sampling remain fidelity gaps;
8 fps loops / 12 fps actions are performance adaptations, not readability-only
exceptions. **The requested 95% UI similarity remains an unverified target.**
Main, disabled remote CI and the verified 0.16 delivery pair remain unchanged.
