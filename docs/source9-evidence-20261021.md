# Source9 evidence — 2026-10-21

All human-readable times in this report use **Asia/Shanghai (UTC+08:00)**. Original UTC journal values are reproduced separately and have not been edited. The reference remains **v0.111.0 / 41cef1ea**. Evidence was checked on **2026-10-21 at 10:44 Asia/Shanghai**, after both aggregate and post-review short receipts and the completed older 24-suite journal were available.

## Status and source identities

Source9 adds reviewed rest-site fire/light evaluation, the original single-player rest-character scene recipes and ordered blend planes, and treasure presentation timing. It also repairs two stale regression fixtures exposed by a completed run of an older frozen checkpoint. These are source changes with bounded diagnostics. **No source9 native/WASM/ARM build, complete source9 engine regression, complete new private pack, matching HTML/ZIP pair, or 95% visual-fidelity result is established by this report.**

| Identity | Evidence boundary |
| --- | --- |
| Local source8 parent `81a0dd913afc55ee7176fa8f3a37408155e509b4` | Locally preserved source; its complete runtime changes have not been formally rebuilt. |
| Source9 worktree | Uncommitted at the time these diagnostics were captured. The final source checkpoint and any archive are to be recorded separately after this evidence document is integrated. |
| Latest verified remote work-branch ref `4b9dad9645f531b3cda3884c2a906bc425570ca6` | GitHub branch writing remains unresolved. Neither a local commit nor a remote unattached commit object establishes a successful branch push/readback. |
| Older frozen runtime `0f750373da39b980726b543b993f3bca49c7cf2e` | The 24-suite result and the two isolated old-library fixture diagnostics below belong to this older runtime. |

The source-preservation gate in [AGENTS.md](../AGENTS.md) still applies: commit, push, read back the branch/tree/changed blobs, and then run long work from a frozen checkpoint using `tools/run_logged.py`. This stage performed only short source checks and a few private render samples; it did not start a new long source9 build or bulk conversion. The already-frozen older regression run was allowed to finish, and its failures remain recorded.

## Source changes and their limits

### Rest-site fire and light

[world_room_shaders.py](../tools/world_room_shaders.py) evaluates four specifically reviewed shader programs. [world_scene_assets.py](../tools/world_scene_assets.py) exposes a default-no-op shader hook; [world_room_assets.py](../tools/world_room_assets.py) opts the rest renderer into it. Ordinary scene rendering retains the existing path. Known shader paths must match their reviewed code fingerprints, required texture inputs must exist, and unsupported parameter/resource shapes fail explicitly. Other shaders retain explicit omission diagnostics rather than displaying a noise or distortion mask as substitute art.

The read-only scene inventory covered **48 light nodes, 19 background fire nodes (4 normal and 15 additive), and 2 Necrobinder/Osty dark-fire nodes**. This is an inventory of source inputs, not 69 rendered or visually verified nodes.

The implementation preserves the following distinctions:

- Light uses the original curve texture and color ramp, including floating-point curve values above 1 until the shader calculation is complete. Curve baking and generated-texture sample positions follow the reviewed Godot 4.5.1 conventions. Noise uses repeat sampling; masks use clamp sampling. Bilinear lookup includes the half-texel offset.
- Light warping uses the scene model-transform scale, excluding the camera transform. Its opacity multiplies original texture alpha, warped texture alpha, and node alpha; color-ramp alpha is not substituted. Existing quad geometry is preserved.
- Fire combines the two authored noise textures, noise mask, two distortion textures and distortion mask before evaluating the authored inner/outer thresholds. The dark variant retains its separate color equation and global offset. Its explicit shader color output is not multiplied by node tint a second time. Sprite flipping changes all shader UV sampling consistently.
- Shader color is clamped/quantized to RGBA8 before the existing LDR compositor. This is distinct from prematurely clipping curve values. The evaluator does not invent procedural GPU noise.
- Rest-background sampling is a **static time-zero sample**. Actor-associated fire also uses an explicit zero shader time. Neither establishes a fire/noise loop period, and no arbitrarily shortened dynamic loop is registered.

The reviewed shader fingerprints are metadata; original shader source and texture pixels remain private:

| Reviewed shader | SHA-256 | Fingerprint input |
| --- | --- | --- |
| `rest_site_light_shader.gdshader` | `468d72309e2f8e2d7ae29458e4d453e094c731b136a5c8fe94d2dbd7c008642e` | Complete source bytes |
| `vfx_stepped_shader_fire_flat.tres` | `10365f5ad4c7cf285634a92558b35b4c13f1c2aa188502cef3859a4493dc7198` | Decoded generated shader code |
| `vfx_stepped_shader_fire_add.tres` | `020ed35c9ac3f2b3c5312d36c4fbe98d5d456023270a94c6fd44e2c40aaef721` | Decoded generated shader code |
| `vfx_stepped_shader_fire_dark.tres` | `6fae0954ba6d4ae43947d8ad19be2a2f8a04252e9969b6d5faa0cc0bc16ed537` | Decoded generated shader code |

Seven authored Underdocks reflection materials have reversed inner smoothstep edges. GPU behavior for those edges is formally undefined; the CPU evaluator uses the usual polynomial and records `authored_reversed_smoothstep_edges_cpu_polynomial_gpu_undefined`. This is a declared limitation, not verified GPU parity. Remaining gaps include **11 foliage-wind instances**, particle/smoke simulation, and the existing static Underdocks water approximation. GPU derivatives, mipmaps, anisotropic filtering, MSAA, exact GPU float/filter precision and per-draw framebuffer quantization are not emulated. There is no independent original-game GPU image oracle.

### Five characters and Osty at the rest site

[world_rest_actors.py](../tools/world_rest_actors.py), [world_rest_actor_assets.py](../tools/world_rest_actor_assets.py), and [world_rest_actors.hpp](../include/world_rest_actors.hpp) describe six scene components: the five playable characters plus Osty. They preserve the original single-player parent/child transforms, the three act-specific animations, and the secondary light-off track for the dark variants. Transparent actor planes use the same 320 × 218 room camera and screen origin as the background, so they are not independently centered by their visible bounds.

| Component | Authored period | Ordered blend planes | Planned clips | Planned frames |
| --- | ---: | --- | ---: | ---: |
| Ironclad | 6.666667 s | Normal | 6 | 804 |
| Silent | 8 s | Normal | 6 | 960 |
| Defect | 8 s | Normal → Add → Normal | 18 | 2,880 |
| Regent | 8 s | Normal | 6 | 960 |
| Necrobinder | 21.333334 s | Normal | 6 | 1,536 |
| Osty | 10.000001 s | Normal | 6 | 1,200 |
| **Total** | Independent periods | Three acts × lit/dark | **48** | **8,340** |

The nominal 20 Hz sampler caps a clip at 256 frames by lowering sample density while preserving its complete period. Authored floating-point durations are normalized to milliseconds before frame budgeting; Osty's 10.000001 s becomes 10,000 ms. Necrobinder and Osty retain separate periods and the original Necrobinder-before-Osty composition order. Osty is part of the authored single-player rest scene regardless of combat HP.

Defect's four contiguous glow slots form an additive middle plane. Its stored pixels contain the already alpha-weighted emission, and runtime addition must not apply alpha a second time. Regent's and Necrobinder's actual empty `CanvasItemMaterial` multiply overrides use default normal MIX; the skeleton slot enum alone would produce the wrong blend. The converter validates those source overrides. This does not establish generic multiply-blend support.

The original `SpineSlotNode` follows the bone transform without inheriting the slot tint/alpha. The dark light-off track therefore does not imply hiding the attached Necrobinder/Osty fire. The focused dark samples retained that fire. Animation sampling starts at phase zero; the original nondeterministic presentation phase and independently advancing global shader time remain unimplemented.

### Treasure presentation

The presentation source adds the original chest fade toward 0.5 over 400 ms and restoration over 300 ms, collection fade-in/out over 400/300 ms, and the 400 ms banner with EXPO_OUT opacity and BACK_OUT position. Presentation updates use 33 ms quantization. Early collection keeps interpolation continuous, Dolly selection does not delay closing, loaded state uses terminal presentation, and an empty chest has a one-second presentation-only close.

These changes add no State or save-codec fields and use presentation state rather than gameplay RNG. New room regression assertions are source-ready, but the integrated source9 room engine suite has **not** run. Missing effects include the holder's original random 0.2–0.4 s delay, black/white treatment, 0.6 s entrance and associated input wait, the empty-chest spring, particles, and the relic flight to the backpack.

## Planned resource capacity and cache scope

The short `tools/build_world.py --plan` result is a resource plan, not a completed pack or a measured encoded payload:

| Planned quantity | Value |
| --- | ---: |
| Card models | 558 |
| Strings | 6,100 |
| Auxiliary images | 1,170 |
| All animation clips | 1,695 |
| All images, including animation frames | 45,719 |
| Enemy variants / frames | 1,600 / 33,210 |
| Map clips / frames | 3 / 540 |
| Room opening/shine clips / frames | 6 / 99 |
| Rest actor clips / frames | 48 / 8,340 |
| Maximum directory size | 1,345,360 bytes |
| Directory / file limits | 2,097,152 / 268,435,456 bytes |

The encoded payload size remains unknown until the selected original frames are losslessly encoded and assembled. Independent source review found that all prior **1,609 world-clip keys, 831 world-image keys and 4,361 world-text keys retained their IDs**. Room and actor cache dependencies include their new renderer/source inputs; the enemy scopes remain separate.

For 109 enemy models and three cache-scope kinds, all **327 scope comparisons** retained the source8 scoped file/function fingerprints. The enemy manifest/planner was unchanged. This preserves eligibility of the previously audited enemy cache; it does not constitute a new payload audit. The historical source8 audit covered **122/1,600 clips, 2,639/33,210 frames and 14,167,819 payload bytes**, with 1,144 valid distinct bounds receipts and 456 jobs sharing Kaiser layout. See [source8-evidence-20261021.md](source8-evidence-20261021.md) for that earlier audit.

## Current-source short checks

### Recorded aggregate diagnostic

The first aggregate receipt is outside the repository at `../source9-short-diagnostics-20261021/receipt.json`, with `run.log` beside it. It ran at **2026-10-21 10:36:58.226674–10:37:12.963525 Asia/Shanghai**. Original UTC fields are `2026-10-21T02:36:58.226674+00:00` and `2026-10-21T02:37:12.963525+00:00`.

- Kind: `short_source_diagnostic_unverified_checkpoint`.
- Parent: `81a0dd913afc55ee7176fa8f3a37408155e509b4`.
- Source fingerprint: `1c8b92118308c689d892da1b7c2351271149732bae766dff0ae004bc641dc867`.
- Receipt SHA-256: `986800692605fd744458748fd1e53e49e0091c5e0f5fc457b50ed14d060d142d`.
- Log SHA-256: `e648065f17c2736977746a7cc4f7e3ff8a08dcae500dadc66c770b1c6e5520d6`.
- All eight commands returned 0; the receipt reports `passed: true` and `source_changed: false` **during that diagnostic**.

| Command/check | Observed result | Boundary |
| --- | --- | --- |
| `python3 tools/world_ui_profile.py` | Header/profile validation passed | No private rendering |
| `python3 -m unittest discover -s tests -p 'test_world*.py'` | **54 tests passed** | Asset-free converter/planner tests |
| `python3 tests/world_pack_regression.py` | **21 tests passed** | Synthetic pack tests |
| C++17 `-fsyntax-only` on five translation units | Exit 0 | `src/app.cpp`, room UI, presentation blend, Ironclad completion and five-character UI tests; no link/runtime check |
| Compile `presentation_blend_regression.cpp` with UBSan | Exit 0 | Pure presentation helper |
| Execute that helper | **2,622,597 additive RGB565 checks passed** | No engine/content linked |
| `python3 tools/build_world.py --plan` | Counts above | Planning only |
| `git diff --check` | Exit 0 | Whitespace check |

The Python total is **75 tests (54 + 21)**. Earlier individual shader/scene/room test runs overlap this total and are not added again. The RGB565 check count is 2,622,597; “565” identifies the pixel format and is not an extra numeric prefix.

After this receipt, independent review found that new actor clips in the synthetic room fixture still pointed at opaque image 0, which would cover the lit/dark room. The fixture was corrected by appending four actual RGBA frames: an explicit transparent ordinary-actor frame and Defect's ordered normal/add/normal diagnostic planes. New assertions cover lit/dark Defect Camp pixels, opaque/partial foreground, and unchanged canonical save bytes and RNG. A separate artificial clock-jump fixture now resets presentation deadlines before testing natural unsigned wrap; runtime behavior was not altered for that fixture. The rest-renderer `character_layer` diagnostic text was also updated. The earlier receipt does not silently extend to later source changes.

### Post-review targeted receipt

`../source9-short-diagnostics-20261021/post-review.json` records the subsequent **2026-10-21 10:42:31.943908–10:42:34.784985 Asia/Shanghai** check. Its original UTC fields are `2026-10-21T02:42:31.943908+00:00` and `2026-10-21T02:42:34.784985+00:00`. Comparison against the first receipt found exactly two changed source files: `tests/world_room_ui_regression.cpp` and `tools/world_room_assets.py`.

- Source fingerprint: `81ba0ff0687be546ece2b8f79506abd4321d477aa286d19329ff1046bca910b8`.
- Receipt SHA-256: `8d373772f329ff543726c34dadb1934ed18f8ff22e8f97efa362068ccfde808f`.
- Log SHA-256: `69f4a43464af249242dd3e762a9fd34e53f395c7398968d6e85181d5dfb9c228`.
- All four commands returned 0; `passed: true`, `source_changed: false` during the check.
- Room asset tests: **5 passed**. Rest actor tests: **9 passed**. These **14 are reruns within the original 75-test set**, not 14 additional independent tests.
- C++17 syntax-only check of `src/app.cpp` and `tests/world_room_ui_regression.cpp`: exit 0, no diagnostics. `git diff --check`: exit 0.
- Final room fixture SHA-256: `b36cdd1b0dfc5581dd2e034b77040acb21009507173a76d7a22642e2db5c9164`.

The new room pixel, RNG/save, and clock-wrap assertions are **source-ready and syntax-checked, not executed engine regressions**. Neither this receipt nor the pure helpers below is a completed room engine suite or a linked source9 runtime.

### Independent bounded helper checks

The treasure presentation helper was compiled with UBSan and reported **8,003 assertions passed**, with no engine or content linked. Its compile took 0.8426 s; both compile and execution returned 0. This was a short tool diagnostic, not a `run_logged.py` frozen engine run. `/tmp/treasure_transition_helper.cpp` has SHA-256 `7e93f0fb3b865f486c08402a33d495fd9307a3151b79238996581c3d73c205de`; its executable has SHA-256 `27e5d0b830c3ed1a10754fe43c97bf168c4f1efecabf784a39fd38cb3a1f6d5c`.

A separate review compiled the current production `Canvas::paint`/`picture` implementation and direct dependencies beside an extraction from source8. UBSan execution reported **73,824 assertions passed**, covering sparse, scaled, opaque, dense and palette dispatch, plus byte equality for the unchanged source8 default path. Both compile and execution returned 0; compile took 0.6912 s. It linked neither engine nor content. `/tmp/source9_canvas_dispatch_review.cpp` has SHA-256 `9e87884ce3aa18d00ea1973420a5638ee42faffd6e7ceccb0b20d21813a6a0fe`; its executable has SHA-256 `a85248f91ffcec524a551b4d2f6646c5a5dc57a0f95dbce3acee32790cc5b897`.

The review's consolidated receipt is `../source9-integration-review-20261021/receipt.json`, 12,863 bytes, SHA-256 `b969d7b93d4f3b30088c17af4037fc366e0b9802bc9b58a63d73d1a4dde52143`. It was written at **2026-10-21 10:42:47.782487 Asia/Shanghai** (`2026-10-21T02:42:47.782487+00:00`) from completed tool observations. The directory retains copies of both helper sources and binaries with their original hashes. This is an after-the-fact review receipt, not a prestarted `run_logged.py` frozen execution journal.

Earlier two- and three-TU syntax checks, followed by the five-TU aggregate and two-TU post-review checks, overlap. They are not additional runtime tests.

## Private render samples

All original pixels, scene parameters, extracted data, and diagnostic PNG/JSON files remain under the private `../reference-inputs/` tree. Only audit metadata is recorded here. There was no bulk conversion.

### Overgrowth fire/light sample

`../reference-inputs/source9-rest-diagnostics/` contains two images and a summary produced in **2.7215385 s**. The new renderer drew **23 nodes versus 10 previously**, adding ten light and three fire draws; all thirteen added draws had nonzero alpha. Every pre-existing drawn node retained its transform and local rectangle. Both composed images were opaque, and **22 other omissions remained**. This checks nonempty shader output and unchanged existing geometry for one time-zero scene; it does not compare against an original-game screenshot.

| Private evidence | Bytes | SHA-256 |
| --- | ---: | --- |
| `overgrowth-light-fire.png` | 72,110 | `7c56b574e51bd4a91d6f45233fa2509f031dde54cecfa691778774047ba2ac6a` |
| `overgrowth-previous-omissions.png` | 71,072 | `9d9ffd255d37e3b77537d71377fc0c4cae06b0a8d963b95b4ff42888be409f7e` |
| `overgrowth-summary.json` | 11,881 | `b3d23474a5daec8099a5bf4d12a68aaf6bf859c9bbfb87749f8652275b4ce45c` |
| `receipt.json` | 866 | `d5302e319eb15a7bea180cfa3eaf2ee3dfc9c2974e3ac42e8c14861d88af75e6` |

The receipt binds the renderer/test source at sample time. Later diagnostic wording changes do not make it a newly rendered sample.

### Actor samples

`../reference-inputs/diagnostics/rest-actors-source9/` contains **11 retained single-frame PNGs**, each with a JSON diagnostic. They cover six initial actor samples and five focused samples from progressive implementation, without a frozen-checkpoint/cache receipt binding. The first Regent sample predates the correct scene material override and contains only the earlier partial plane; the later `regent-a1-lit-p0` sample supersedes it with the complete normal-MIX actor. The other samples have no known later pixel-meaning correction, but their earlier JSON files do not contain all subsequently added diagnostic fields. These are not 11 fully converted clips or a complete final-version image set.

| Single-frame PNG | SHA-256 | Scope |
| --- | --- | --- |
| `ironclad-p0.png` | `c35062a848338b6997685fde90d02be89530ae0c1df8bc3392fe2274b10d68ef` | Initial normal plane |
| `silent-p0.png` | `ab7d4113f2f40f505c0e37e7a7960c29984f4b4ad8d43f76ad65abf66db9ee1c` | Initial normal plane |
| `defect-p0.png` | `6f075001b20b0f825410e29085a67795ac7a1c83520010551fcc81feb5771701` | Initial first normal plane |
| `regent-p0.png` | `839ba2cd7697b704d87b95f389c262fc809f7ef14bcb4bb6d6a1f3ccf11b01aa` | Earlier diagnostic; superseded |
| `necrobinder-p0.png` | `b51f8a93ac0d6677309a7ffc36e1e1475953c2e064f4cdf05368782f974da3c6` | Initial normal plane |
| `osty-p0.png` | `f0b8884d3c64287105fae7e59b464dfbed47b218b7490755f621ed590a9fec38` | Initial normal plane |
| `defect-a1-lit-p1.png` | `ece06266770670829c26b7f5071b13958d79228dc085bdc71b52d8e2fc6b9d0b` | Additive middle plane |
| `defect-a1-lit-p2.png` | `462208244b364f567ffae11c5f50e00e0631925cef58836d8f576547989c1ec8` | Following normal plane |
| `regent-a1-lit-p0.png` | `15c07c1e790acec6c9cd8d945b8200c9425b06c527bec0d812cc3106a1060f23` | Correct normal-MIX override |
| `necrobinder-a2-dark-p0.png` | `b865e5d7d4dd6516684762ca461888b175ae7f2807b16b38097d435601daa53d` | Dark fire retained; 102 nonzero-alpha fire pixels |
| `osty-a3-dark-p0.png` | `29075e0836e35c4919e6d054b7327a40ebd134aa83b4f041d2008410ba37c12d` | Dark fire retained; 247 nonzero-alpha fire pixels |

Zero omissions in these selected actor-component diagnostics do not imply that all room scene effects are supported. Full-loop output, all acts/lighting combinations, actual-pack compositing, animation continuity and original-game visual comparison remain unverified.

## Completed older frozen regression: retain the failures

The older checkpoint's remaining 24-suite run is complete. It ran from **2026-10-21 10:19:30.357214 to 10:37:18.008402 Asia/Shanghai**, with `ctest -j 2 --timeout 600`. Original UTC journal fields remain `2026-10-21T02:19:30.357214+00:00` and `2026-10-21T02:37:18.008402+00:00`.

**Result: 22 passed, 2 assertion failures, 0 timeouts; total CTest elapsed time 1,067.58 s; exit code 8; journal status `failed`; frozen source unchanged.** `character_ui_regression` passed in 498.97 s. No timeout rerun was necessary. This is a subset of the older checkpoint's tests, not a full current-source test result.

- Frozen source: `0f750373da39b980726b543b993f3bca49c7cf2e`.
- Journal: `/tmp/nspire-stage-0f750373/build/journal/world17-remaining-63e43055a928.json`.
- Journal SHA-256: `5546f799b86cbcea726b5b4661cdea6f5f30059d7646930cdef95206fd8a8daa`.
- Log SHA-256: `8f592a6b64e9b5f3b0f07ba73f96eeccc66e8eeccf31568363cf179bf07fb9ae`.
- Journal source fingerprint: `98db3cd8d5432c5034b3b5a8c215cad7408f8573b42d5d9a250cae2ba8ac601f`.

| Suite | Result | Seconds |
| --- | --- | ---: |
| `core_regression` | Passed | 227.29 |
| `relic_combo_regression` | Passed | 470.51 |
| `events_regression` | Passed | 495.25 |
| `colorless_completion_regression` | Passed | 26.10 |
| `potions_completion_regression` | Passed | 3.47 |
| `expansion_ui_regression` | Passed | 28.25 |
| `ironclad_completion_regression` | **Assertion failed at old line 206** | 2.30 |
| `silent_completion_regression` | Passed | 8.19 |
| `defect_completion_regression` | Passed | 6.70 |
| `regent_completion_regression` | Passed | 11.51 |
| `necro_completion_regression` | Passed | 11.53 |
| `character_ui_regression` | Passed | 498.97 |
| `characters_integration_regression` | Passed | 0.10 |
| `five_character_integration_regression` | Passed | 0.43 |
| `five_character_ui_regression` | **Assertion failed at old line 56** | 0.07 |
| `autoplay_status_regression` | Passed | 0.25 |
| `original_events_regression` | Passed | 1.55 |
| `world_map_regression` | Passed | 5.47 |
| `world_relics_regression` | Passed | 0.54 |
| `localized_template_regression` | Passed | 0.03 |
| `world_ui_regression` | Passed | 2.10 |
| `world_event_ui_regression` | Passed | 6.49 |
| `world_map_ui_regression` | Passed | 2.16 |
| `world_reward_ui_regression` | Passed | 1.93 |

### Havoc → Armaments: fix the assertion boundary

The old Ironclad fixture expected `forced_exhaust[arm]` to remain set while Armaments was waiting for an upgrade choice. The pinned original call order is Havoc setting `ExhaustOnNextPlay`, then autoplay, then `CardModel.OnPlayWrapper` obtaining the result location **before** the card's `OnPlay` opens that choice. Obtaining the result consumes the one-use modifier; the saved physical-play continuation retains Exhaust as the destination. The relevant source paths were already correct and byte-identical to the frozen core. No gameplay/core change was needed.

[ironclad_completion_regression.cpp](../tests/ironclad_completion_regression.cpp) now asserts the pending nested choice and Limbo card, checks that the transient flag is cleared and both continuation results are captured, and performs three repeated result previews. Each preview must keep every canonical save byte unchanged: Armaments still resolves to Exhaust/bottom and the outer Havoc to Discard/bottom. The fixture retains save/load continuation, invalid-flag rejection, upgrade completion and final exhaustion. Existing `card_result_regression` source also covers repeat preview and saved-resume non-consumption; it was inspected for this diagnosis, not rerun or added to the short-test count.

The revised test translation unit was compiled with the **old frozen headers and `libspire_core.a`**, ASan and UBSan enabled. Its first execution exited 1 because LeakSanitizer reported that it cannot operate under ptrace. That failed attempt is preserved:

- `/tmp/nspire-source9-havoc/attempt1-lsan-failure.log`, SHA-256 `19dbeccb9219387d01ff5a97152a8e5f22ce87a399fc97d772a50b4a4fcb9918`.
- `attempt1-receipt.json`, SHA-256 `a83be39861929c4a3a86bf1f40844c0ff26e67564426a8c632f88618e8ef97da`; return code 1, 2.555531 s.

With only leak detection disabled (`ASAN_OPTIONS=detect_leaks=0:abort_on_error=1`, `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`), the same isolated old-core/new-fixture diagnostic passed **4,407 checks across 170 base/upgrade variants plus Giant Rock and interaction contracts**, in 2.628528 s. ASan and UBSan remained enabled; leak checking did not run.

- Passing `run.log` SHA-256: `40f55980b6d97112339d742a1b42f32d5afa5541952ab0048f9dd04cde22d0ef`.
- Passing `receipt.json` SHA-256: `f2d569cffba669f264264d350e05b218633a186e943bea38c553b7041f2be6f6`.
- Revised test SHA-256: `3a19057689c97a2bbe60ed16dc367c7d301cb34625573dc128a175c1d5a2a1f3`.
- Diagnostic executable SHA-256: `748b6d75a52c61992be6c3adbe56964d1ff7fced3f4c0f6a5b10db1e81d78423`.

This diagnoses a stale fixture against the older core. It neither changes the original failed journal nor establishes a source9 runtime pass.

### Five-character UI: enter practice through Settings

The old fixture selected a fixed title row and immediately expected character selection. In the current title layout that row is Quit when no save is present and Settings when a save is present. Practice is exposed through Settings. The pinned original frontend distinguishes new single-player entry from Continue/Abandon; practice itself is this project's retained helper rather than an original menu feature.

[five_character_ui_regression.cpp](../tests/five_character_ui_regression.cpp) now enters the save-dependent Settings row, checks the settings state, and uses the rendered `SettingsPractice` hit target before selecting a character. No frontend behavior was changed to accommodate the old assertion.

Compiled as one revised test translation unit against the old frozen headers and library, it passed **2,703 checks using a synthetic profile**, in 5.075576 s. ASan and UBSan were enabled; LeakSanitizer was disabled for the ptrace environment.

- Directory: `/tmp/nspire-source9-five-character-ui/`.
- `run.log` SHA-256: `0585d3760b8fbe8ed560d0513cc7acdf2f3a548d1f4e24eedf0479fae8b12ea4`.
- `receipt.json` SHA-256: `e47085fd3b0b649bc73524655c7194e11cdfd0ffd2f80de7511a3393d4bf78dc`.
- Revised test SHA-256: `df1661f4551d255bc54525f4d1b0392efec15ae615a6fbcf9af90cf9eaff8c84`.
- Diagnostic executable SHA-256: `6d6a536fd1e70b8bea1577c85cfcde057cc55237dc321b0812c43c81bdad9917`.

This diagnostic does **not** test source8's new save guards, the new source9 actor/treasure runtime, or original image fidelity. The frozen run's original failure remains a failure in its own journal.

## Remaining validation and continuation

The next source stage must be preserved on a verified remote ref before long work resumes. Then build the exact frozen source across native, WASM and ARM; execute the new/changed engine suites and complete regression set; resume conversion from eligible checked caches; assemble and validate the actual private pack; and compare original room/actor/UI output and native/WASM behavior using that matching pack. New rendering still needs full periods and lighting/act combinations, actual runtime layer ordering, original-image review, and performance evidence on the intended calculator path.

The two old fixture diagnoses, 75 asset-free Python tests, syntax checks, pure presentation helpers, 11 actor frames and two Overgrowth images are useful bounded evidence. They do not close the missing dynamic shader/particle/foliage effects, treasure holder/flight effects, complete enemy conversion, final runtime validation, or requested visual-fidelity target. Main, disabled remote CI and the completed 0.16 delivered artifact pair remain outside this source-stage change.
