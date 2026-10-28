# Source8 evidence audit — 2026-10-21

Reference: **v0.111.0 / 41cef1ea**. Release readiness remains **false**.

This audit reread existing journals, checked completed log digests, validated
private cache receipts without rendering, and checked the source8a preservation
archive. It did **not** run a new build, regression suite, conversion, or original
game comparison. Writing this report does not upgrade any historical result.
The public report contains metadata only; private assets, cache payloads,
decompiled sources, generated packs, and embedded-asset launchers stay outside
the public repository.

## Frozen checkpoint evidence

Journal names below are relative to `build/journal/` in the corresponding frozen
checkout. Each `.json` has a same-stem `.log`. Completed journal log SHA-256 values
were recomputed and matched their recorded values; all completed records below
report `source_changed: false`. The existing hashes and source fingerprints are
retained in [validation-world-0.17.json](validation-world-0.17.json).

| Checkpoint | Raw journal ID (`.json`) | Audited result |
| --- | --- | --- |
| `0f750373` | `world17-configure-e4b76fe9b968` | Passed, exit 0. |
| `0f750373` | `world17-native-38ac92f463e1` | Passed, **118/118 build steps**. This is build success, not 118 tests. |
| `0f750373` | `world17-wasm-cc206ad066c8` | Passed; runtime **4,600,560 bytes**. |
| `0f750373` | `world17-converter-tests-abfc34e40dcc` | Passed; **38 Python tests**. |
| `0f750373` | `world17-arm-3d972bc3934c` | Failed, exit 1; zehn rejected constructor/TLS sections. |
| `0f750373` | `world17-ctest-63977c63cbca` | Interrupted; **16 of 44 suites reported passed**, no final result. Raw status remains `running`. |
| `0f750373` | `world17-new-flow-45af975d0019` | Interrupted; no completed result. Raw status remains `running`. |
| `0f750373` | `world17-new-flow-resume-de04f1fc2546` | Failed; **2 passed / 2 failed**, 138.77 seconds. |
| `14fcf1e4` | `world17-pack-5537cfd33eb3` | Interrupted; raw status remains `running`. No completed pack or final directory validation. |
| `84a6ebf3` | `world17-conversion-profile-108713afe98b` | Passed; completed conversion profiling only. |
| `84a6ebf3` | `world17-prefetch-fa14c6368be4` | Interrupted; raw status remains `running`. No completed prefetch claim. |
| `4b9dad96` | `world17-raster-compare-21be606aa5cc` | Passed; **5 clips / 69 frames / 0 differing RGBA bytes**. |

The resumed four-suite run passed `card_result_regression` and
`world_frontend_ui_regression`. It failed `enemies_regression` at the Gremlin
Surprise turn-order assertion and `world_game_over_ui_regression` at its
synthetic Proceed-art fixture. Subsequent source fixes and isolated diagnostics
do not change those frozen failures. An unfinished journal is not proof of a
currently running process and must not be converted into a pass.

The raster run measured **37.27548519199263 seconds** for the retained reference
renderer and **13.30180109000139 seconds** for the batched renderer, a
**2.8022885727867046×** speedup. Its complete clips were CorpseSlug.hurt (12
frames), SpinyToad.hurt (11), Inklet.idle_loop (8), CeremonialBeast.hurt (16), and
Zapbot.idle_loop (22). This is exact agreement between those two implementations
on those samples, not an exhaustive enemy-frame comparison or an original-game
display oracle.

The previously reported **51 passing Python tests on mutable source** are a
separate short check. No corresponding new frozen journal was established in
this audit. Source8 frontend/room changes and later repairs still need a new
formal frozen build and regression run.

Full checkpoint IDs:

- `0f750373da39b980726b543b993f3bca49c7cf2e`
- `14fcf1e4478a582d39024ba8cb96a466c0bb8dd4`
- `84a6ebf32899108920ee225abd0c71de13bc4aaf`
- `4b9dad9645f531b3cda3884c2a906bc425570ca6`

## Private cache completion, measured from receipts

Every counted receipt passed schema/key, payload length, and SHA-256 checks.
Clip payload framing was also parsed. Enemy job keys were independently
recomputed using the audited mutable source's renderer scope, complete job, and
measured layout; the cache constructor and cache migration methods were not
invoked, so this check wrote no cache records.

| Category | Validated receipt count | Payload bytes | Scope of evidence |
| --- | ---: | ---: | --- |
| Schema 2 enemy clips | **122 / 1,600** | **14,167,819** | **2,639 / 33,210 frames**; all 122 matched the current retained reference renderer scope. |
| Schema 2 enemy bounds | **1,144** | **0** | All required measured jobs matched; another **456 jobs** use the shared Kaiser layout. |
| Schema 2 images | **884** | **5,811,958** | 816 texture, 54 scene, 14 atlas-tres receipts; payload integrity checked, individual current image-job scope eligibility not rederived. |
| Legacy schema 1 images | **77** | **739,720** | Valid old receipts; not an additional current output-directory count. |

There are **1,478 enemy clips still missing**. All inspected receipt directories
had zero orphan `.bin` files, zero temporary files, and zero invalid receipts.
The bounds count covers all required measurements; the 456 shared-layout jobs
must not be described as missing measurements. Legacy image receipts must not
be added to the schema 2 count as if they were distinct current pack entries.

These are **validated cache receipts**, not a completed resource pack. Cached
enemy clips use the retained `enemy-rgba-1` reference pipeline. They are eligible
for the checked reference-cache reuse path; this audit did not create batched
pipeline receipts or certify future renderer changes. A changed scope must be
checked again. None of these counts measures enemy behavior parity or UI visual
similarity.

## Source8a preservation artifact

The audit checked `checkpoint-20261020-stage8a.json` against
`source-stage8a-20261020.tar`. All **15 changed Git blobs** matched the manifest;
their combined size was **334,107 bytes**. The tar contained **425 regular
files**, totaling **8,232,960 archive bytes**. This identifies that preserved
snapshot, not edits made after it.

| Field | Audited value |
| --- | --- |
| Parent | `4b9dad9645f531b3cda3884c2a906bc425570ca6` |
| Tree | `fbc6fe1a2db3819c02232ffcd147aa16c8e25d03` |
| Manifest SHA-256 | `fe5eb448e362e2e832b91468eee31923b9e576fde46fb4237d69c2cf8d08aea5` |
| Tar SHA-256 | `42a5cd9aa192a4da06701c7180c497ffd81677d163b9bb46d9e840537542b9e6` |

At audit time, local push receipts ended at stage7b / `4b9dad96`; no
`stage8a-push-receipt.json` existed. This local source archive check does not prove
a remote branch update. An existing tree or commit object is not a verified
branch reference. Remote preservation requires a successful ref update followed
by ref and blob readback.

## Deliverable status

No completed **0.17 NSP2/14 pack**, matching **0.17 offline HTML**, or matching
**0.17 calculator ZIP** was present in the audited outputs. The intended
`world-0.17` output directory was absent, and frozen7's `dist/` was empty.

The frozen7 compile-only `build/web/nSpire2.wasm` exists: **4,600,560 bytes**,
SHA-256 `ee1763f2897786b415593dc879c841a16116604786c0ae785232e151a90c9d0b`.
The earlier frozen6 ARM launcher is **820,236 bytes**; it is not a successful
source8/frozen7 ARM build or a validated match for a completed new pack.

The existing `Slay-the-Nspire-2-offline.html` identifies itself as **0.16**.
The existing `Slay-the-Nspire-2-calculator.zip` contains `validation-0.16.json`,
a 284,458-byte launcher, and a 12,910,778-byte pack. Preserve that historical
delivery; do not rename or present it as the new 0.17 result.

## Next gate

1. Preserve the complete reviewed source checkpoint through a remote ref update,
   then verify that ref and its blobs before starting long work.
2. Freeze that exact source; run logged native, ARM, and WASM builds, the repaired
   and new frontend/room suites, and the complete regression suite. Retain
   historical failed and interrupted journals.
3. Resume private resource conversion from eligible verified receipts; assemble
   the complete pack and validate actual sizes, directory bounds, and reading.
4. Check native/WASM rendering and save consistency with the completed pack,
   then generate and verify matching launchers and delivery bundles.

Until those gates finish, neither full resource completion nor a new verified
playable bundle is established. Native/WASM consistency is not an original-game
comparison; successful ARM linking is not emulator or physical-device timing.
**No measured 95%+ UI fidelity claim is supported.**
