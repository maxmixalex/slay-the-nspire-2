# Card rendering and startup measurements — 2026-10-23

The rendering changes remove repeated small-card composition and glyph-pixel
reads during text measurement. The two static Chinese grid scenes made **zero
resource-source callbacks over their 180-frame warm windows**; both languages
also reduced measured host render time. All animation clocks continued to
advance. This report does not claim calculator FPS or a device cold-start gain.

## Source and method

The baseline runtime is `e8444322b494db67d905e4772ff95373d7bfd34a`; the remotely
preserved candidate is `069f6d51f37aef3f31d11a6282bd02a61d7afdef`.
Both static libraries were built with Clang 18.1.3, C++17, CMake Release
(`-O3 -DNDEBUG`). The unchanged `tests/card_grid_benchmark.cpp` was separately
linked against each library and its matching headers. Original NSP2/17 Chinese
and English inputs were held identical between baseline and candidate.

The driver opens the resource read-only through the production ranged-file
callback and cursor. Seed 781 practice fixtures provide eight-card library and
ten-card removal/selection pages. Removal uses `buy(ShopRemove)` after setting
up a shop fixture. Selection places an already owned Secret Technique instance
in the opening hand, preserves zones, then uses production `play()` to suspend
on its actual draw-pile choice. It does not fabricate a choice queue.

After preparation and the explicit settling interval, each scene receives 180
updates of `advance(33)` plus `render()`. Canonical save bytes remain identical
before and after every window, and no save callback is attempted. Observations
were sequential, with no display pacing and already warm OS file pages. Host
elapsed time is not an estimate of physical calculator performance.

## Measured warm windows

Time is mean host milliseconds per update plus render. Source bytes and calls
are totals across 180 frames, not per-frame counts. MB uses decimal units.

| Language | Scene | Baseline ms | Candidate ms | Time reduction | Source MB | Source calls |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| Chinese | library_8 | 1.047 | 0.549 | 47.53% | 49.315 → 0.000 | 12,060 → 0 |
| Chinese | remove_10 | 1.275 | 0.639 | 49.88% | 57.508 → 0.000 | 14,040 → 0 |
| Chinese | draw_choice_10 | 2.056 | 1.585 | 22.90% | 93.429 → 65.437 | 22,830 → 15,996 |
| English | library_8 | 1.894 | 0.615 | 67.54% | 0.000 → 0.000 | 0 → 0 |
| English | remove_10 | 2.075 | 0.655 | 68.41% | 0.000 → 0.000 | 0 → 0 |
| English | draw_choice_10 | 3.045 | 1.599 | 47.50% | 0.000 → 0.000 | 0 → 0 |

English already fit its glyph/text working set in the reader's page cache;
its zero source calls do not imply that its previous repeated layout and
composition were inexpensive. The larger Chinese working set previously
refetched pages during every static-grid frame. The animated combat choice
retains nonzero reads and less acceleration; its changing backdrop and live
scene remain visible and are not frozen to obtain a faster result.

## What changed and what was checked

- `Content::glyph_metrics` reads only resident descriptors. Full-catalog
  measurement leaves all resource-read statistics unchanged and does not evict
  the 64 glyphs prepared immediately before it.
- Card-title fitting and centering run once for all eight outline passes and
  the foreground pass. A pixel comparison preserves their prior paint order,
  including antialiased overlap.
- Eight bounded cache slots retain exact before/after RGB565 card regions.
  Cache hits compare every destination background pixel; changed backgrounds
  rebuild through the original renderer. Slots used earlier in the same frame
  are protected, so a ten-card page cannot cycle through and evict all eight.
  Dynamic rich text, clipping, modulation, unusually wide costs and oversized
  keys fall back to normal drawing.
- The focused hand outline derives from the actual 69 × 97 card face and
  follows the same pose scale and rotation. Geometry is checked at 0.5×,
  0.65625× and 1×, plus a 90-degree rotation.

The synthetic card/HUD test passed 385,178 checks. The same test passed 329,691
checks on Chinese resources and 318,361 on English resources. Its full-surface
oracle compares cached and uncached production rendering on eight/ten-card
loads, varied backgrounds, costs, stars, animations, clipping and tint. The
existing Canvas regression also passed 159,808 cases and 319,758 checks.

Private side-by-side captures show zero changed pixels on the entire library
screen in both languages. Removal-screen differences include the intended
replacement of literal markup/template fields with the correctly formatted
localized heading. The choice captures also include the concurrent HUD change;
they are not claimed to be pixel-identical to the old release.

The host cache symbol is 130,944 bytes plus a four-byte frame counter, below its
130 KiB compile-time ceiling. The final integrated ARM **16 MiB total allocation
gate remains separate** and must be checked after all agents' changes are linked.

## Full startup validation remains enabled

`resource_open_benchmark` opens each actual pack in a new process and measures
the existing progress phases. This is a host stage breakdown with warm OS
pages, not a flash-storage cold-start measurement. It does not bypass a check.

| Stage | Chinese ms | English ms |
| --- | ---: | ---: |
| Complete open | 766.178 | 722.788 |
| Header | 0.007 | 0.006 |
| Full-file integrity | 123.852 | 106.007 |
| Directory | 0.507 | 0.226 |
| Retained image validation | 62.986 | 59.566 |
| Tile-index validation | 577.504 | 556.592 |
| Glyph validation | 0.990 | 0.040 |
| String and final directory checks | 0.266 | 0.316 |

Full-file FNV accounts for **16.2%** of the observed Chinese open and **14.7%**
of English. Tile-index validation accounts for **75.4%** and **77.0%**. The complete
open reads 117,346,301 and 111,875,809 callback bytes respectively, including the
full 81,124,876 / 78,349,198-byte integrity ranges after their 40-byte headers.

These checks answer different questions. The full hash checks the stored
payload before play; structural validation constrains descriptors, offsets,
references and decoder inputs; group checks and decoders validate payloads as
they are used. The present measurement does not establish that removing one is
acceptable. It also shows that removing only the full-file hash cannot account
for the largest measured host stage. The next useful startup investigation is
the tile-index traversal and decompression work, followed by a device-specific
I/O/CPU breakdown. No validation policy was changed in this work item.

## Evidence and remaining integration

[Machine-readable results and closed journal receipts](validation-rendering-0.17.json)
record input identities, driver/library hashes, all warm-window observations,
startup phases and source-stability checks. The accepted synthetic receipt is
`perf-card-pixel-final-b44e32276ac9.json`: an earlier parallel invocation's
journal remained `running` on readback despite printing completion, so it was
not accepted and the identical command was rerun sequentially. All thirteen
accepted receipts are closed `passed`, exit 0, with unchanged frozen source.

The English event-reader scroll helper was separately diagnosed as counting
character wraps instead of complete-word wraps (Baths: 12 versus 13 lines).
Its follow-up keeps the original scroll assertions and is not relabeled as
part of this candidate's execution. Later feedback resources, split-container
hosts, the ARM allocation gate and any Firebird/device timing need their own
integration evidence.
