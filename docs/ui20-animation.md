# UI20: hover artwork, registered action frames and reward identity

The reference remains the owner's v0.111.0 / `41cef1ea` installation. All
original images, sampled skeleton geometry, generated frame tables and packs
remain private. The completed source bake, production rendering checks and
independent full-frame pixel oracle are recorded below.

## Corrected behavior

- Power hover rows use the same source status image lookup as their combat
  badge. The shared helper is also used by merchant hover preparation.
- A hover containing only derived cards anchors its preview to its real owner
  on both screen edges. It no longer reserves an absent keyword column.
- A terminal core phase does not suppress the visual Osty death edge while
  queued combat feedback is still being presented. His original `die` clip
  now proceeds to the original `dead_loop` when the new assets are present.
- Card reward rows use the original rare, uncommon and special-card artwork.
  The rarity rule follows `CardReward.get_IconPath` (original token
  `0x06000f31`) and `TryGetSingleRarityInPool` (`0x06000ac6`): boss encounter
  rewards are rare; a source pool containing only one rarity uses the first
  generated card's rarity. A mixed source pool retains the ordinary icon.
  `SpecialCardReward.get_RewardIcon` (`0x06000fb8`) supplies its own row icon.

## Animation density and storage

The complete UI19 ordinary-enemy audit found nine action clips, 325 frames,
whose pixels were below the native four-pixels-per-21-source-units density.
The old per-clip fit included the entire travel path in one bounded bitmap.
For example, Ceremonial Beast's `plow` used 16,149 inverse-scale units versus
5,376 for its idle. The runtime had to enlarge those pixels about three times.
Other affected source models are Bygone Effigy, Frog Knight, Mecha Knight,
Thieving Hopper and Waterfall Giant. Vantom already has a separate continuous
registered-layer renderer and is not rebaked by this stage.

The new builder samples the official Spine runtime at the exact previous
frame counts and times. It rasterizes each frame at native density, trims its
transparent boundary and preserves signed world coordinates. Only a single
frame larger than 320×240 is split into bounded pieces. Invisible pieces are
culled before pixel decoding. The tiny registration list is copied before
loading pictures, so cache page replacement cannot invalidate its pointer.

The 325 old frame image IDs are reused for each frame's first piece. The bake
needs 23 extra pieces across 17 frames (11 Bygone Effigy, one Thieving Hopper,
five Waterfall Giant frames; the latter can need four pieces). Overflow pieces,
Osty's three dead-loop frames, 10 private word tables and three reward icons append
to the directory. Existing clip metadata, IDs, clocks and units are unchanged.
NSP2/44 remains the intermediate format; the separate storage stage reclaims
unused tile slots, applies lossless compression and creates locale editions.
The resource reader recognizes the complete 5,422-entry endpoint as well as
the previous 5,409-entry endpoint. No resident pixel pool grows.

The accepted prior 454a6 enemy / 555a6 player colour precision is retained;
this stage raises spatial resolution without lowering colour precision or
removing frames. Runtime pixels still use RGB565 and the existing alpha path.

## Completed validation

The complete intermediate English pack is `eng-ui20-complete44.nsp`,
71,028,106 bytes before the storage stage reclaims unreachable tiles. It has
58,099 images and 5,422 auxiliary entries. The adjacent JSON receipt lists all
333 replacement IDs (325 action frames plus the UI lane's eight corrected
320×240 Ancient backgrounds), 39 appended IDs and every frame registration.
All 1,783 existing clip descriptors remain exactly unchanged. Its SHA-256 is
`d823e669731c75bcf7f62f307f699c0212a211af0f236c36383a4b6777cf66b5`.

| Check | Result |
| --- | --- |
| Actual English power/card hover regression | 635,956 checks passed |
| Actual English world UI / deferred combat feedback | 35,225 checks passed |
| Actual English reward UI and source rarity policy | 4,352 checks passed |
| Production Canvas registered-frame regression | 770,035 checks; all 328 frames and 351 bounded pieces passed |
| Synthetic geometry, split pixels and signed tables | Four cases passed |
| Controlled device fixture save/progress round-trips | 239 checks passed |

The production Canvas test exercises every source-clock frame, off-screen
culling, loop/clamp endpoints and distinct ordinary/rare/uncommon/special row
icons. Osty's original short `dead_loop` resolves to one corpse pose at LCD
precision; all three source samples are retained and deduplicated by the
existing image grouping. The test correctly requires motion only for the
nine moving enemy clips, rather than inventing movement in this source pose.

`verify_framed_animation_ui20.py` independently resamples the official Spine
runtime, rasterizes each **whole frame without partitioning**, then decodes
and places the shipped pieces in source coordinates. Across all 328 frames:

- Maximum visible source RGBA byte difference: **0**; differing values: **0**.
- Difference after the established 454a6/555a6 mapping: **0 RGB565 pixels and
  0 alpha pixels**.
- Every shipped piece independently decodes to the exact sampled piece.
- Dimensions, signed registration, nonoverlapping piece rectangles and
  visible alpha coverage pass strict checks.

RGB is ignored only where both source and assembled eight-bit alpha are zero.
Such source RGB was already omitted by the established sparse encoder and
cannot affect the LCD. No visible holes, alignment errors or size tolerances
are accepted. The verifier permits a one-byte source floating-point threshold
for general raster arithmetic, but **none occurred in these original frames**.
This comparison is separate from the storage lane's byte-exact 44→45 codec
verification; it does not claim that the platform's colour quantization is
lossless relative to the original full-colour textures.

Private evidence is under `private-ui20-animation/source-pixel-oracle/`:
`pixel-oracle.json` plus 17 full-source/decoded-piece PNG pairs covering all
23 extra pieces. Native 320×240 draws, the original Die For You tooltip icon,
Osty's corpse and reward rows are under `private-ui20-animation/captures/`.
The Bygone Effigy two-piece, Thieving Hopper two-piece and Waterfall Giant
four-piece comparisons were opened and visually inspected.

Completed journals include `ui20-animation-final-art-2d5439f06500`,
`ui20-native-all-registered-frames-70d6528781bd`,
`ui20-full-source-pixel-oracle-783dc0a32c4c` and
`ui20-production-device-fixtures-5beb35db3092`. The final source-pixel oracle
ran against verified remote checkpoint
`d8bec9c2e33f02a8cedac4eb84814e5f3cbdebc7`.

The integration tree names the separate controlled exporter
`tests/ui20_animation_fixtures.cpp` / `ui20_animation_fixtures`, preserving the
UI lane's independent `ui20_device_fixtures` target. Its six saved scenarios
cover live Ceremonial Beast Plow, Osty's nonterminal and terminal death,
already-dead Osty, merchant Accuracy's card-only hover and the boss's golden
card reward. They use the production run/history codecs, mark the unobserved prefix
partial and are explicitly controlled test prerequisites. Ordinary Firebird
performance and final 45-package checks remain owned by the integration lane;
native wall time is not presented as calculator FPS.

## Required private rebuild stage

The historical `build_content.py` entry point does **not** automatically run
the complete later UI conversion chain. A current UI20 rebuild must include
the following source-art repair before locale grafting or compression. The
runtime's deliberate compatibility with the previous 5,409-entry artwork
directory does not make that old artwork a complete UI20 release.

Start with the accepted complete UI19 NSP2/44 English logical pack (5,409
auxiliary entries), the user's pinned original `v0.111.0 / 41cef1ea` PCK, and
the official private Spine runtime built with `build_private_spine_runtime.py`.
The runtime source revision used here is
`e7dc1435fa4a0083ab431f1b28e083c14a1f5c68`; the private runtime DLL and its
build receipt must accompany the work directory. Set `UI20_SOURCE_44`,
`UI20_PCK`, `UI20_RUNTIME`, `UI20_DOTNET` and `UI20_WORK` to those private paths
outside the public source checkout. After publishing and verifying a source
checkpoint, run these commands through `tools/run_logged.py` as required by
the repository's build policy:

```sh
python3 tools/rebake_ancient_ui20.py \
  --source "$UI20_SOURCE_44" --pck "$UI20_PCK" \
  --runtime "$UI20_RUNTIME" --dotnet "$UI20_DOTNET" \
  --output "$UI20_WORK/ancient"

python3 tools/framed_animation_ui20_assets.py \
  --input "$UI20_SOURCE_44" --pck "$UI20_PCK" \
  --runtime "$UI20_RUNTIME" --dotnet "$UI20_DOTNET" \
  --work "$UI20_WORK/framed" \
  --extra-replacements "$UI20_WORK/ancient/ancient-ui20.json" \
  --output "$UI20_WORK/eng-ui20-complete44.nsp"

python3 tools/verify_framed_animation_ui20.py \
  --content "$UI20_WORK/eng-ui20-complete44.nsp" --pck "$UI20_PCK" \
  --runtime "$UI20_RUNTIME" --dotnet "$UI20_DOTNET" \
  --cache "$UI20_WORK/framed/sampled" \
  --output "$UI20_WORK/source-pixel-oracle"
```

The second command is the mandatory integration point: it replaces the 325
action frames and eight Ancient backgrounds, appends the bounded overflow
pieces, Osty dead-loop frames, original reward icons and ten registration
tables, and emits the adjacent JSON receipt. It rejects an unexpected source
version or auxiliary endpoint and never overwrites its input. Omitting its
`--extra-replacements` argument also omits the separately baked Ancient
background fixes; the production command above deliberately includes them.

The resulting logical English file must have 58,099 images, 1,783 unchanged
original clip descriptors and 5,422 auxiliary entries. Feed **this repaired
file** to `refresh_artwork_resources.py --artwork`, then run the documented
`compact_tile_slots.py`, `compact_sparse_tiles.py` and
`retune_resource_streams.py` pipeline in [ui20-storage.md](ui20-storage.md).
Those stages preserve the repaired pixels, registration tables, animation
timing and locale contents while reclaiming unused slots and choosing smaller
byte streams. Running compression directly on the historical UI19 file skips
the visual fixes and is not the UI20 production chain.

The UI20 packaging gate checks the decoded registration tables and the
required artwork identities before release packaging. This is a regression
guard in the converter/packager, with no extra whole-file scan added to the
calculator's runtime. The source-art conversion and the original-frame pixel
oracle above remain required; the gate is not a replacement for either.
