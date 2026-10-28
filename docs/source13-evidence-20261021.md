# Source13 map selection and resource prefetch — 2026-10-21

Human-readable times use Asia/Shanghai (UTC+08:00); journals retain UTC. The reference remains v0.111.0 / 41cef1ea. This is a source checkpoint with separately identified prior formal results and short diagnostics. It does not establish a complete 0.17 release, original-GPU pixel agreement or 95% UI similarity.

## Preserved source12 and completed builds

At **11:49:58**, `work/0.17-world-parity` read back as [`6ec81e3b6b6aac1198d168623ae557b4fe01deb0`](https://github.com/zyf722/slay-the-nspire-2/commit/6ec81e3b6b6aac1198d168623ae557b4fe01deb0), tree `172c6d9ad227fed0339f9d96a99e1921cd6ff6ea`, parent `713ab3be32c5e1331da132581d6ecad3c9d31e0e`. All **442** committed paths, modes and blob IDs, and all **11** uploaded contents, matched. Main remained `efc50bc0b0cc6ca8e83461a2ff394cc7a56895e0`; CI remained disabled. Remote receipt SHA-256: `840481bc79efba3ab576331fd609bd6d2bc50394afcea14494a2d6b82a5f4b2d`.

The source-only recovery ZIP is **1,515,616 bytes**, SHA-256 `8a7c6b04ad89ba535c3934a17fa3576a1a3e09fe6d98a21c106a0edd5734e85d`; every archived source entry and ZIP CRC was read back. The equivalent original local commit remains on its named checkpoint. Frozen source12 has source fingerprint `b1b446b4dd79829256e8dde821c2f59a9dad296ad9a8b8c8d943700bfda2c78e`.

| Frozen source12 gate | Completed result |
| --- | --- |
| Native ASan/UBSan build | All 122 steps passed in 449.286 seconds |
| ARM compile/link/package | Passed in 149.821 seconds; 840,339-byte launcher |
| WASM compilation/link | Passed in 146.912 seconds; 4,796,029 bytes |
| Affected complete UI regression batch | Running at this cutoff; 19 of 20 suites have passed, including the complete changed room and blend suites |

All completed build journals report unchanged source. The ARM launcher SHA-256 is `1a682c645ec3ca4990c0c1b7309f9b33bc399e3cfdff141e283e0f2c7391995b`; allocation is **10,417,972 bytes**, with 28,471 relocations. WASM SHA-256 is `47ea3d01cf728375be957f868bb5c017d3d4af832bad14ce36ae478600b225e0`. Cross-target receipt SHA-256 is `a44e1d78402f34e7f24c47e6c5967fa00c6259de6c803fc4992a59539bbd7b4f`. Native build journal SHA-256 is `4a7f89695d68372b317dc5fec343c3cb91cc2927bd00e574faa2b17e628875f2`; its log SHA-256 is `1a167b2a731f96a054d95c20ff3271b5d06defa4b83f7bd1247d41452cbfa5fd`.

`world_room_ui_regression` passed in **64.24 seconds** against the full source12 sanitizer library. The new holder timing, one cosmetic draw, early keyboard claim, pointer press crossing the gate, reload, Dolly continuation and clock-wrap fixtures therefore have a complete runtime execution, separately from source12's earlier extracted helpers. The room suite reported **7,637 checks** and the blend suite **2,773,149 checks** in actual CTest stdout. The complete batch result and its final JUnit receipt remain pending; these are not borrowed short-helper counts. These source12 results do not certify source13's new map/input changes.

## Retained source10 timeouts and conversion restart

The original source10 46-suite native batch finished with **44 passed and two timed out**, exit code 8, after 1,773.99 seconds. `events_regression` reached 1,200.15 seconds and `character_ui_regression` reached 1,200.13 seconds; neither produced a completed assertion count. No assertion or sanitizer failure was observed. The initial failed journal remains failed. The two suites are being rerun sequentially from the same frozen source, with a 2,400-second per-test limit; the events suite has now passed in **1,071.96 seconds**, while the character-UI suite remains running. There are **45 distinct passed suites** at this exact source, not yet 46/46. Initial complete receipt SHA-256: `39e38ef742a4e8eec6cac3b5619578d26892fd1aa227e8417f68af763daa7e8f`.

The completed full enemies suite checked **46,415 assertions across 174 encounter/ascension variants**, and the complete world-map suite passed **1,063** checks including the Dolly continuation repair. These are executable regression evidence, not a claim that registry or art counts prove complete original behavior.

A bounded actual Crusher frame diagnostic compared the same renderer under default BLAS threading and `OPENBLAS_NUM_THREADS=1`. The output was byte-identical: 320 × 218 × 4 bytes, SHA-256 `6882162587e12cd508ddfe241ea981cc7685cd4e8f316f44e784eb983869c170`. In this one sample, wall time changed from 4.409 to 3.394 seconds and CPU time from 6.814 to 2.832 seconds. This is a scheduling measurement for one CPU raster frame, not a general bake-speed or GPU-fidelity result. Diagnostic receipt SHA-256: `544dfce100557a878c0cc0dc243d4a5ed277d413cbc059550b351fb307631afc`.

After checking the exact owned process commands, the old serial pack and two-worker enemy-prefetch parents received a controlled interrupt. Their journals retain `interrupted`, exit 130 and unchanged source; no cache was deleted. The resumed strict scan confirmed **784 reusable enemy clips of 1,600**. The serial source11 assembler resumed with one BLAS thread; the source9 reverse enemy prefetch resumed with four single-BLAS-thread workers. All enemy render closures remain identical. Restart receipt SHA-256: `7e17aa3ee9f2daa77dcbe053ff22c7f03cbc227fbc54b4cfc532495e606e04cf`. Both jobs are still running; partial counts are not a completed pack.

## Original map selection reticle

The map now uses the original 48 × 48 `combat_reticle.png`, preserving its color and alpha. Four 30 × 30 scene corners mirror independently, each with the original black shadow at alpha 0.35000002 and projected offset (+3,+2). Original control bounds are 84 × 84 for ordinary nodes, 240 × 208 for the ancient, and 374 × 306 for a boss; the second-boss parent applies its authored 0.75 scale. The common calculator projection is 0.22, with RGB565 and pixel rounding. Icon-hover scaling is not incorrectly inherited by the reticle.

Original selection is immediate white/scale one, followed on the next tween update by explicit 0.9 → 1 scaling over **500 ms EXPO_OUT**. Deselecting captures the currently displayed scale and alpha, then moves toward scale 1.05 and alpha zero over **200 ms SINE_OUT**. Q12 tables sample these transitions at the existing 33 ms clock. An eight-byte motion record per map node retains simultaneous fades during rapid focus changes. Full 64-bit run seed and act identify the presentation cache; reset/load clears motion without changing global input mode. State, save codec and gameplay RNG are unchanged.

Original `IsUsingDirectionalNavigation` includes keyboard navigation as well as a controller. Arrow keys enter that mode here; pointer movement, buttons and scrolling leave it. Repeated stationary pointer samples preserve keyboard focus. The calculator API has no Godot mouse-velocity measurement or PC keyboard-mode preference, so actual coordinate change and always-available arrow navigation are explicit adaptations. Ordinary mouse hover does not draw a selected reticle.

The original selection class hash is `fe49e30868ce87ec2551e8784849795d34f238fbc8b8096c4a9dd56b0101b407`; scene hash is `70f26cd6f73123e7bcb9c466b25b728bcf728cedeab8ce5cc7c1baa27409b561`. No private source or art is committed. Independent read-only review checked the original scene, controller behavior, tween start semantics, geometry, clipping and lifecycle without blocking findings.

The new image is appended at auxiliary index **1170**, after all 831 existing world rows. Their complete key/index sequence is pinned by SHA-256 `7a2c7313947e81306646a10d4584faf3a3ca3a57f90de6e7a04731d107f9c546`; earlier IDs do not move. Source13 plans **1,171 auxiliary images / 45,720 total images**, with 1,695 clips, 558 cards and 6,100 strings unchanged. The conservative directory bound is **1,345,380 bytes**. All 335 checked rendering scopes, including 327 enemy closures, and all 1,657 appended clip jobs remain identical to source11; the 38 legacy clips are unchanged. Closure receipt SHA-256: `0b3674dfdd063370a67f51e62f24c5b10f610bce2bfe7bde42af68b9b5b13506`.

## Source13 short diagnostics and prefetch tool

| Short check | Executed result and boundary |
| --- | --- |
| Three changed reticle C++ translation units | Syntax passed, 3.728 seconds |
| Extracted production reticle helpers under UBSan | 29,556,899 assertions passed, including 384 complete synthetic frames against an independent geometry oracle; no engine linked |
| New image/index tests | Two passed; original 831 indexes pinned and asymmetric 48 × 48 RGBA preserved |
| Generated profile and complete directory plan | Passed |
| Actual private source texture probe | Original decoded RGBA and component output byte-identical; 0.465 seconds |
| Rest-prefetch protocol tests | 12 asset-free mock tests passed, 0.638 seconds |
| Expanded native/WASM replay driver | C++ syntax and JS parse passed; actual-pack execution remains pending |

The reticle helper covers four mirrors, dense/palette texture paths, alpha holes, partial alpha, the shadow, map clipping, every node dimension and the smaller second boss. Its explicit four-candidate fixture verifies A → B → C → A independent fading and reselection, seed-high-bit identity and clock wrap. The persistent full-App test also covers save purity, overlays, input-mode switching and reset/load, but awaits its frozen build; its generated-route multi-candidate branch is conditional on that route. Reticle receipt SHA-256: `ee6791b45233d8002aeba70d369c5f83b1510845d358902dc008317a40ba6562`; log SHA-256: `c251f6b07191b1a1f9fd528fa40e794d276ed027e95844f50133ca4d667e39b9`.

Asset short-check receipt SHA-256: `215a91a3a93d368561070c393254d5da007d7cbc336c59ae66e4dfced47d42fc`. Actual texture probe receipt SHA-256: `43a5e72358010ea9e665bc87df302e9cf6cc81dc349f320cfe59b33ed9341a1b`. An initial probe harness used a plain dict where the existing texture cache requires an OrderedDict and failed; that failed attempt is retained. The corrected harness passed without changing production renderer code. This one texture does not verify the complete pack or original-GPU rendering.

`tools/prefetch_world_rest.py` prepares the existing **48 rest clips / 8,340 frames** in reverse order, using one worker by default and at most two. It invokes the unchanged sampler, encoding, complete job identity, rendering scope and cache protocol. It verifies metadata, frame count/dimensions and payload hashes; checks frozen source before/after jobs and at completion; compares observed concurrent metadata and payloads; and reads completed records again before reporting success. The complete jobs and rest/enemy scopes match source11 byte-for-byte. Receipt SHA-256: `caece659bbbbcc3cb61ff301bb3fe841646220eca65ee1cfbfb9182dad900ce5`.

Prefetch workers cooperate through per-job locks, but the existing serial converter does not acquire those locks. Before/after/final readback detects observed disagreements; it is **not** a transaction guarantee against a noncooperating external writer. The actual owned producers use the same frozen rendering closure. No renderer or serial cache code is altered. Independent read-only review found no blocking issue. Actual rest prefetch must start only after source13 is committed, pushed, read back and frozen; its current mocks do not claim that original clips have been converted.

The 16-case actual-pack backend fixture now includes directional focus changes, timed deselection, real pointer movement and save/load in its map case. The WASM runner replays those pointer moves through its actual exported API. Complete frames and saves still need comparison after a matching full pack exists.

## Next gates and explicit remaining effects

Push/read back and freeze source13, then build its native/ARM/WASM runtimes and execute affected input/map/frontend suites and the complete Python asset tests. Start the one-worker rest prefetch from that frozen source. Finish canonical full-pack assembly and historical payload retention checks. Source11/source12's 1,170-auxiliary profile is not interchangeable with source13's 1,171-auxiliary profile; assemble the matching final pack using the retained compatible clip cache before actual-pack native/WASM capture, screenshot inspection and packaging.

Map-circle entry, marker entry/exit, path travel and map-opening transitions remain separate unfinished slices. For future work, original path-dot/map-close easing is CUBIC_OUT, while the fast room fade is QUAD_OUT. Treasure hover/press, rarity glow/particles, empty text spring, relic flight and first tutorial remain unfinished. Dynamic room shaders, particles and GPU filtering still need original comparison. Do not claim 95% from internal backend agreement, or calculator performance from ARM linking. Retain the complete 0.16 delivery pair until matching new artifacts are verified.
