# Source selection restoration — implementation stage

Pinned reference: v0.111.0 / 41cef1ea. This stage follows the completed
stage8e build and its retained validation receipts. It does not claim a
percentage of original-game parity or a completed final release.

The source selector command audit is in
[combat-selection-source-0.17.md](combat-selection-source-0.17.md). It separates
the command's all-eligible shortcut from the independent confirmation rules
of hand, pile, simple-grid and generated-offer screens. A singleton fixed
hand/pile request should not introduce a modal. A hand screen that actually
opens still retains its independent Confirm control. Generated offers have
their own `canSkip` flag. The core implementation applies automatic choices
within the existing effect loop, without recursively entering a public
choose/resolve call or mutating gameplay from a renderer.

## Private append-only resource extension

`tools/selection_ui9_assets.py` consumes the complete stage8e v33/2058 catalog.
It retains the 8704-string directory, 1765 animation registrations, all old
artwork and old glyphs. The 32 original card/relic/potion/power selection
prompts use verified empty slots 8378–8399 and 8689–8698. Both original locales
are read from the user's private PCK, never embedded in public source.
`src/selection_text_index.inc` contains only source keys and indices.

Two appended image references form the optional v33/2060 catalog. Slot2058
is the source upgrade-preview arrow, projected to13×12 and shared by the
three source arrow nodes. Slot2059 stores a shader-input image, not a visible
replacement sprite. The pinned512×512 RGBAH card-frame SDF is bilinearly
sampled at the145×181 projected texel centers with edge clamp. Each texel is
stored as two little-endian words: alpha UNORM16, then RGB565. The directory
therefore reports290×181, format0 after optional lossless LZ4 decoding.
The complete104980-byte decoded input uses the existing image cache; it adds
no resident pixel pool. RGB is retained because the original shader uses
both RGB and alpha. This projected fixed-point shader does not claim to be
bit-identical to Godot's GPU implementation.

Only the missing original glyphs at the actual scene sizes are appended.
The pre-conversion directory check requires10 additional English keys and52
Chinese keys for the currently specified selection, pause and potion text,
leaving both below the existing32768-entry limit. Conversion readback must
verify every required glyph and byte preservation before accepting output.
These counts are planning checks, not yet a production-conversion receipt.

## Overlay layout corrections in progress

The original pause menu uses five372×80 solo buttons in a centered VBox.
At320/1680 projection, its controls are71×15 with6px original bold labels;
the source64px title projects to12px. The VBox minimum height includes the
64px header and4px default separation. The shared backstop comes from
`scenes/run.tscn`: black at.85 with the source500ms fade. The hidden blue
Backstop in the pause scene must not be substituted. Its original Back
control resumes the live run. The Give Up material uses the original half
turn in YIQ chroma, with a separately authored label outline.

The potion popup's259×239 source rectangle projects to49×46. Use/Discard
controls project to45×16, and the32px labels to6px. The popup is centered
under the actual holder and retains its100ms opacity and150ms position
entrance. Disabled state darkens the source background rather than changing
the authored label colors. Existing throw targeting and automatic placement
in the first available belt slot are retained.

The deck, combat and world-choice screens, actual-resource interaction tests,
ordinary ARM/WASM builds, browser flows and calculator execution must all be
run against the subsequent complete UI checkpoint. Passing converter/unit
checks or source diagnostics alone is not functional completion.
