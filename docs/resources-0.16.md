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
| Image payloads in the supplied 0.15 pack | 1,622 | Measured during conversion |
| Glyph payloads in the supplied 0.15 pack | 6,265 | Measured during conversion |

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

## Memory and execution status

The application and versioned pack encoder use a **12 MiB resident resource limit**. The fixed WebAssembly memory is **16 MiB**, with the remaining space reserved for code data, game state, frame buffers, image caches and the stack. ARM/Ndless allocation and the final compressed pack size must be checked against the built artifact; neither is inferred from the registry count.

Source implementation is prepared. **No successful 0.16 conversion, parser regression, render check or pack-size result is claimed yet.** Execution follows a source checkpoint that has been pushed to GitHub and read back with a matching tree. The intended verification includes eight asset-free Python test cases, the versioned C++ parser suite, the retained-payload comparison, actual five-character UI rendering, clip/image bounds and final resident size. Failed commands retain their journals. Original-runtime differential testing and physical-device execution remain separate evidence.

The first frozen conversion attempt at `2eabce11a5ad5e5f2460e67eb5f9713c8ea424eb` stopped at NoEscape's unsupported `IsTargeting` localization condition before producing a pack. Journal `pack16-preservation-34ae4b4084d1` records exit 1 and `source_changed: false`. Its source repair added explicit target conditions for NoEscape and TimesUp and matching Python/UI regression cases.

At `865a92783a825d95d05c20689aca90eeceee8723`, all eight new resource tests passed within a 25-test Python run; the run's sole failure was the historical browser test still expecting save writes to v13 instead of v14. Journal `python16-all-fc7c7b030c5b` retains that failure. The same checkpoint's conversion passed the three character pools' text processing and then stopped at Debris's absent original description, before producing a pack (`pack16-preservation-6ef55eadb633`). The next source revision explicitly permits Debris's keyword-only description and updates the browser test to require v14 writes while preserving reads of v3–v13. These repairs await a new pushed checkpoint before execution; failed attempts remain preserved and are not counted as successful pack evidence.

For an existing verified 0.15 pack, invoke `tools/build_all_characters.py` with the reference installation, that pack, a private output path and a Chinese-capable font. The full `tools/build_content.py` pipeline also invokes the new stage after its historical stages. Only rasterized missing glyphs enter the resource pack; the font file is never copied into it.
