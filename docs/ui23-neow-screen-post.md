# Neow final screen-water post

These six source files add the active `Neow/water effect` source command49.
The two explicitly hidden sibling water posts remain hidden. This pass sees
the screen **after** body, stepped fire, bone lights, fog/stars, static lights,
vignette and final three lights, and runs before dialogue/HUD and reduced-scene
cache publication. It must not replace those layers with an old baked screen.

## Integration contract

- Input: exactly46 /7224 auxiliary entries /59729 images /1783 clips.
- Output: exactly46 /7225 auxiliary entries /59730 images, with aux7224→59729.
- Add `tools/neow_post_assets_ui23.py` after the common trajectory stage and
  before final quality/reduction/pruning. Arguments are `--input`, `--output`,
  `--pck`, `--noise-header`, `--work`, and optional `--no-animations`.
- Include `neow_post_ui23.hpp` with the renderer headers and include
  `neow_post_ui23.inc` where Canvas is available. The sole hook is
  `if(e.id==OriginalEventId::Neow)neow_post::draw(c,reduced?0u:a.map_clock_ms);`
  after all Neow scene layers, before `ancient_still_store`.
- Add the exact endpoint to existing ancestor schema checks. Validate the new
  typed root with `neow_post_assets_ui23.validate(source,image)`; final release
  calls it with `final=True`. Before generic reduction, call
  `require_reduced_source` so the full-profile noise approximation cannot be
  relabeled as a directly sampled original-noise static field.
- Register `neow_post_ui23_regression` like the other standalone header tests;
  this test defines its own foreground workspace and does not link spire_core.
  Include paths need the normal `include` directory. Python tests are
  `tests/test_neow_post_assets_ui23.py`.

Root owns the central Content/guard/Recipe edits while this isolated piece is
reviewed. No changes to those concurrently owned files are included here.

## Data and math

The source-bound producer verifies original scene/shader/texture hashes and
the genuine bundled `nspire_neow_noise` export. Neow uses Godot's default
OpenSimplex2S, non-normalized R8 and original seamless blending; the explicit
OpenSimplex2 Random/Orobas helper is not substituted.

Full mode carries a source-filtered128×128R8 noise image, the complete original
100×100 alpha mask and the mip lookup/typed parameters in one28672-byte view.
The complete continuous piecewise-bilinear noise comparison has maximum error
1.875R8 levels, corresponding to at most0.03530 /0.02648 LCD pixels of screen
coordinate shift. The original512² noise and the candidate's periodic
derivative bounds prove rho<1.214 andrho<1.138 respectively for every clock;
therefore mip0 plus mip1 cover the source `filter_linear_mipmap` footprint.

Reduced mode directly samples the original512² noise at clock0 at every native
2×2 derivative-quad position, retaining R8-in-Q8 values. It has no noise
resolution approximation; Q8 rounding is at most1/512 of an R8 level. This
51200-byte view is a separate exact-source field, not a full-profile sampler
alias. Both modes retain one actual image leaf, with strict payload hashes,
profile/mode checks and zero padding.

Rendering uses native RGB565 bilinear interpolation, a source-derived mip
lookup and explicit2×2 finite derivatives. RGB565 intermediate rounding and
fixed-point math are distinct from a claim of bit-identical GPU floating-point
execution. The actual source-body comparison at0/499/999ms had at most one
RGB565 level per channel for the noise approximation; native499ms and reduced0
also differed from the floating reference by at most one level per channel.
These screenshots used the actual deferred-vignette source body and do not
claim that still-pending fog/fire layers were already present.

## Ownership and measured cost

All content views are acquired before the pass. A generation check binds the
single view to that Content instance. The pass then borrows the existing
`ResourceTransferLease::Bytes` workspace for a153600-byte full screen and
38400-byte mip1, exactly192000 transient bytes. It makes no content requests,
callbacks or image-cache changes while the lease is held. A competing
Bytes/Save/Codec/Map lease causes an unchanged-screen refusal; RAII releases
only this pass's ownership. No persistent framebuffer, cache or particle pool
is added.

Actual retained group payloads are10608 bytes full and31376 bytes reduced,
before final whole-pack storage. Their decoded views occupy7 and13 image-cache
pages. The actual full/static source records passed935272 UBSan checks,
including independent full-copy output equality, phase wrap, inactive regions,
lease collisions and failure release. The same optimized host kernel measured
about0.974 /0.902ms including screen copy and mip construction on the source
body. This is neither calculator timing nor a full-App frame-rate claim.

Combined production allocation, complete final-layer Canvas output, actual
linked decoder cold reads, guest performance, final original-folder CLI modes
and the70,000,000-byte extracted installation gate remain required.
