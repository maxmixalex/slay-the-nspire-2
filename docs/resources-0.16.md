# Five-character resource profile — 0.16

## Scope and reference

The asset converter reads the owner's pinned **v0.111.0 / 41cef1ea** installation after verifying both hashes in `config/reference-release.json`. Public files contain original asset paths, numeric metadata, sampling parameters and the converter. Original images, Spine data, localization, fonts, generated resource packs and manifests remain private.

`tools/build_all_characters.py` appends the three complete 86-card pools and eleven shared generated/status cards after the existing 228 cards. The public single-player registries supply each card's identity, base/upgraded numeric values, cost, keywords and calculated-variable names. All 258 character cards have a final portrait in the pinned PCK. Dazed, Wound and Debris have no original description field, so their displayed text consists of their original keyword titles. The prior Sidestep portrait and every earlier image remain unchanged.

The converter distinguishes static numeric text from live damage, Block, Osty damage, delayed Block, orb counts, Focus, channel counts, hit counts, Forge and Doom. Live markers retain their canonical number for collection previews and a named runtime hook for combat previews. Unknown variables and unsupported expressions are errors. The original Sacrifice conditional is rendered through the live Block hook, which returns zero while Osty is dead. NoEscape and TimesUp retain a separate target condition, shown only in combat with a valid living target; catalog and permanent-deck views omit it. Standalone energy icons render their amount; a zero-cost prefix remains zero. Burn's unpowered two damage stays a static number.

## Append-only contract

| Directory | Immutable NSP2/12 prefix | NSP2/13 total |
| --- | ---: | ---: |
| Card models | 228 | 497 |
| Localized strings | 749 | 1,556 |
| Auxiliary images | 233 | 339 |
| Animation clips | 17 | 38 |
| Image payloads | 1,622 | 2,638 |
| Glyph payloads | 6,265 | 7,215 |

Every earlier image record's dimensions, encoding and compressed payload bytes are preserved. Earlier card, string, auxiliary and animation records retain their exact values and order. Existing glyph dimensions, metrics, encoding and payload bytes are retained; only missing glyphs are rasterized. Physical payload offsets may move because directories grow. Reopening and encoding the new pack again must produce identical bytes, and both pre-encoding and post-decoding checks compare the historical prefixes.

The parser retains independent historical profiles: NSP2/12 always means 228 cards, 749 strings, 233 auxiliary records and 17 clips. It never inherits the current character or animation count. The current application requires the full NSP2/13 profile. The Python reader and C++ parser permit at most 8,192 images, 16,384 glyphs for NSP2/13, 2,048 strings and 256 frames per clip. The earlier formats retain their 8,192-glyph limit.

## Added original visuals

| Auxiliary range | Contents |
| --- | --- |
| 233–237 | Defect selection background, portrait, card-energy icon, HUD-energy layers, map marker |
| 238–242 | Regent equivalents |
| 243–247 | Necrobinder equivalents |
| 248–256 | Blue, orange and pink attack/skill/power frames using original materials |
| 257–259 | CrackedCore, DivineRight and BoundPhylactery |
| 260–263 | Lightning, Frost, Plasma and Glass orbs; original Dark remains at 180 |
| 264–265 | Osty portrait and the original layered star counter; original small star remains at 179 |
| 266–289 | Defect powers in `DPower` order |
| 290–311 | Regent powers in `RPower` order |
| 312–333 | Necrobinder powers in `NPower` order |
| 334–338 | Conqueror, Debilitate, Hang, Oblivion and SicEm |

The pinned PCK lacks a separate HyperbeamFocusDown power texture. Its explicit temporary negative Focus status uses the original common Focus icon; this adaptation is recorded in the generated manifest. Each other new power has its corresponding original texture. Regent's HUD composite applies its original material and repeats layer 5 as specified by the original scene's Layer4/Layer5 nodes.

The appended animation IDs are Defect idle/attack/hurt/cast/camp at 17–21, Regent at 22–26, Necrobinder at 27–31, and Osty idle/attack/hurt/die/revive/camp at 32–37. Original full cycles are sampled at 8 fps for new idle clips, 12 fps for new action clips and 4 fps for new camp clips. Their duration is retained to integer frame timing. The first 17 clips and every historical frame remain byte-identical. This sampling budget and the existing offline Spine/material renderer are calculator adaptations, not a claim of pixel parity with Godot.

### Observed Spine branches

A read-only structure audit covered the eight new combat/camp skeletons and three selection skeletons. The combat/camp inputs are Spine 4.2.43; the Defect and Regent selection inputs are 4.2.40. Only Regent uses the additional `NoRotationOrReflection` inheritance mode, on four bones. Its head-bubble constraint also enables relative-world rotation and translation near the end of attack, hurt and cast. Two Regent constraints and three Necrobinder camp constraints enable one-bone IK stretching. Necrobinder's attack activates one six-vertex clipping contour through its authored end slot. None of these eleven inputs contains path or physics constraints; selected clips use only the already decoded bone, slot, IK, transform, deform and draw-order tracks. Three Defect sequence attachments remain detached throughout all four selected combat clips, and there are no selected sequence timelines.

The new mathematical branches use the official Spine 4.2 reference: [Bone.updateWorldTransformWith](https://github.com/EsotericSoftware/spine-runtimes/blob/4.2/spine-ts/spine-core/src/Bone.ts) (blob `ac5dcb4d661348ba90a3cfef8b4daa0e53d7d47f`), [TransformConstraint.applyRelativeWorld](https://github.com/EsotericSoftware/spine-runtimes/blob/4.2/spine-ts/spine-core/src/TransformConstraint.ts) (`76b51df19751d92f7c8922f4612aa961f9fc8d80`) and [IkConstraint.apply1](https://github.com/EsotericSoftware/spine-runtimes/blob/4.2/spine-ts/spine-core/src/IkConstraint.ts) (`4f8f0837741b6c17242567d8e0c96b4731d25657`). The sampler's skeleton scale is one; output pixel scaling remains a separate final operation. Mode 2 removes parent rotation/reflection while retaining its stretch, including the collapsed-axis fallback. Relative-world constraints add target transforms and respect reflected angular offsets. One-bone stretch/compression uses the target distance in the appropriate local space and the original uniform-scale flag. Binary frame switches preserve their step timing.

Clipping partitions the authored concave contour into disjoint triangles and intersects textured mesh triangles against those pieces. Each generated intersection retains linearly interpolated UV coordinates. Clipping includes its end slot, then stops even if that slot has no attachment. Five asset-free regression methods use literal transform matrices, local-space reach, texture coordinates, a known concave area and explicit slot boundaries; they do not use converted game art as their expected oracle. The historical image and frame payloads still enter this stage directly from the immutable supplied NSP2/12 prefix.

## Memory and execution status

The application and versioned pack encoder use a **13 MiB resident resource limit**. The fixed WebAssembly memory remains **16 MiB**, with the remaining space reserved for code data, game state, frame buffers, image caches and the stack. The linker must validate that budget. ARM/Ndless allocation and the final compressed pack size must be checked against the built artifact; neither is inferred from the registry count. The former 12 MiB limit was increased after the actual complete encoding exceeded it by 327,866 bytes; sampling rates, dimensions and all historical payloads are retained. The encoder already uses LZ4's maximum high-compression level 12.

The complete resource conversion, all 30 Python tests, the actual C++ pack parser and independent on-disk payload/font inspection **passed at the pushed, read-back checkpoint `8c6977d3cc1dc9f17926a480127bc1067abd0286`**, tree `db96da21a4f39ae8612857fab5c2ea1b4e1295a3`. The actual pack occupies **12,910,778 bytes** and leaves **720,710 bytes** within the 13 MiB resource allocation. Its SHA-256 is `4b83320460798a5dd57d4faeee3cdf9613bef7413259ac57b61fb74d109a5917`. Actual character UI, backend and device evidence is maintained separately; resource counts and successful decoding alone do not establish original-runtime parity. Failed commands retain their journals.

The first frozen conversion attempt at `2eabce11a5ad5e5f2460e67eb5f9713c8ea424eb` stopped at NoEscape's unsupported `IsTargeting` localization condition before producing a pack. Journal `pack16-preservation-34ae4b4084d1` records exit 1 and `source_changed: false`. Its source repair added explicit target conditions for NoEscape and TimesUp and matching Python/UI regression cases.

At `865a92783a825d95d05c20689aca90eeceee8723`, all eight new resource tests passed within a 25-test Python run; the run's sole failure was the historical browser test still expecting save writes to v13 instead of v14. Journal `python16-all-fc7c7b030c5b` retains that failure. The same checkpoint's conversion passed the three character pools' text processing and then stopped at Debris's absent original description, before producing a pack (`pack16-preservation-6ef55eadb633`). The next source revision explicitly permits Debris's keyword-only description and requires v14 writes while preserving reads of v3–v13.

Checkpoint `7b61bed6326639d137137af453451820b973310e` passed all **25 Python tests** (`python16-all-acd085dbd0ce`, log SHA-256 `d7a049640131560b4d386fc413ecc224e66fa8f182f0f26e20ff8c4dfc234994`). Its conversion passed all new card text/portraits and 106 auxiliary visuals and sampled all five Defect clips, then stopped at Regent's previously unsupported inheritance mode 2. Journal `pack16-preservation-342a84f747db` retains exit 1, unchanged source and log SHA-256 `e2bdfb2398079ec7f290b4ef92280e8e2e34fd5bfee736047c7416de2630d5d2`. No pack was produced. The subsequent complete structure audit motivated the four observed mathematical additions above.

At `79b17017674cbbc02e8255e9bdc714c87472dfe6`, all **30 Python tests passed**, including the five new geometric cases (`python16-all-42539a43c098`). The complete conversion then passed all 21 appended animation clips and font construction but failed its final resident-size gate: the encoded data occupied **12,910,778 bytes**, 327,866 bytes above the former 12 MiB limit. No partial pack was written. Journal `pack16-preservation-058304a1e28b` retains exit 1, `source_changed: false` and log SHA-256 `40c4827f38a967ca4e0a90c7b5299ee58419b617f7ed2eae29a6bf6cb7ab7ecd`. The successor checkpoint increased only the resource allocation to 13 MiB, retained 16 MiB fixed WASM memory and repeated the complete conversion successfully. Failed attempts remain preserved and are not counted as successful pack evidence.

### Final resource verification

All four final journals below completed with `source_changed: false` and common source fingerprint `9fadd7740d7abde1d3144e293f22a6846476d84b6cefe74927f3065182a5ea2e`.

| Verification at `8c6977d3` | Result | Journal |
| --- | --- | --- |
| Python regressions | 30 tests passed, including eight resource and five geometric cases | `python16-all-c3aa036bdc09` |
| Full original-resource conversion | 497 card records, 1,556 strings, 339 auxiliary records, 2,638 images, 7,215 glyphs and 38 clips | `pack16-preservation-f67bf8c7225a` |
| Independent on-disk inspection | Exact historical prefixes, canonical byte-for-byte re-encoding, zero missing new font code points | `pack16-readback-4bc9ac39bda7` |
| Actual C++ content parser | 3,736 checks passed, including real card portraits/text, new auxiliary and clip references, character frame/energy mapping and application opening/rendering | `content16-actual-d0b8e6389395` |

The new image records consist of **269 card portraits, 106 auxiliary visuals and 641 sampled animation frames**. Every one of the previous 1,622 image payloads and 6,265 glyph payloads retains its original bytes. The 228 card records, 749 strings, 233 auxiliary references and 17 clips also remain exact. All 2,638 images use the existing independently decoded LZ4 wrapper. The maximum image dimensions are 320 × 240, and the largest clip in the complete pack has 161 frames, within the 256-frame bound.

The 950 new glyph records cover **190 additional Unicode code points** at all five sizes, 8/10/12/16/20. Independent inspection of the supplied original font's Unicode `cmap` found no missing new code points. Every character in the final localized strings has a glyph at all five sizes. The maximum glyph is 20 pixels wide and 25 pixels high, within the runtime's 40 × 40 bounds. The original font file remains private and is not included in the source or output pack.

Final log SHA-256 values are `7d0de73b3e33cc7ef2d3c4d32678aafd8cc5761646a5a2214f84c38c15754089` (Python), `5081436f06c7ec4b0c52e5408679c4aba039c7cd21d9cb52983fae6ce01ef28d` (conversion), `1c2191f3a069afb18a2a6ec269afd35932ccc73a810b37df0e048561ef8bd534` (readback) and `b720952aaeee36e90a95b4382df66eed68397a7304f1a5b82a85f4229dd32495` (actual parser). The private output also contains `content-manifest.json` and `content-readback.json` with exact counts, allocation, hashes and source checkpoint.

For an existing verified 0.15 pack, invoke `tools/build_all_characters.py` with the reference installation, that pack, a private output path and a Chinese-capable font. The full `tools/build_content.py` pipeline also invokes the new stage after its historical stages. Only rasterized missing glyphs enter the resource pack; the font file is never copied into it.
