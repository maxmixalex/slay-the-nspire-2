# UI5 card details: implemented changes and measured limits

Reference: v0.111.0 / `41cef1ea`. The accepted resource change orders Chinese
glyph payloads by localized character frequency **within each native font
size**. English retains its previous physical payload order. All glyph
geometry, advances, encoded coverage bytes, images, animation groups and
localized strings remain exact. This change adds no installation bytes,
runtime buffer, loading phase or startup integrity scan.

The measured improvement is fewer cold resource callbacks and bytes in every
tested Chinese card grid and first-open detail. Native CPU samples are mixed;
this report does not claim that all frames become faster or that the reported
milliseconds predict calculator performance. The full source-only receipt is
[`validation-ui5-card-performance-0.17.json`](validation-ui5-card-performance-0.17.json).
Private packs, screenshots and full execution logs are kept outside this
repository.

## Implemented runtime work

`Content::prepare_images` batches only the missing images requested by the
currently visible cards. It uses a bounded set of 32 descriptors, the existing
encoded-image workspace and the existing image pool. Identical descriptor
aliases are deduplicated; adjacent ranges with tiny padding are read together.
Distant images and unsupported batch cases keep the ordinary path. The batch
does not load another card page or reserve a second image buffer. Collection,
deck, removal, card-choice and detail-preview callers pass their visible images.
The content regression checks aliases, compressed and raw images, small gaps,
distant selections, read failure/reopen behavior and zero warm reads.

`Canvas::glyph` advances encoded runs by row and column, replacing division and
remainder for each visible glyph pixel. Clipping, run coverage, paint order and
RGB565 blending retain the earlier output. The pixel oracle and actual paired
page captures cover this change; no font is rerasterized or simplified.

Detail-word hover rejects positions outside the description before formatting
the text. Inside it, hit testing requests glyph metrics rather than glyph
payloads and stops after the visible rows. It therefore does not fetch coverage
bytes just to locate a word. This change preserves the earlier hit-test layout;
the remaining mismatch between English paragraph word wrapping and the
hit-test's character wrapping is a follow-up. The benchmark exercises the
production pointer path rather than inferring its cost from a probe counter.

Inline star formatting now follows the pinned original sprite-font formatter:
`singleStarIcon` and `starIcons()` produce original star imagery, including the
original count/zero behavior. A rendering marker has the same square metrics
in width calculation, wrapping and drawing; outline and bold passes do not
duplicate the sprite. Actual English and Chinese v22 resources passed the
sprite-pixel comparisons at 8, 10, 12 and 16 px. This is separate from the
Regent combat counter's presentation. Inline energy imagery still needs its
own original sprite-font assets; the card-cost gem was not substituted for it.

## Why the resource change was needed

The initial image batching and glyph-loop changes did not materially reduce
first-open detail reads. The frozen stage-2 comparison still read the same
Chinese collection/removal detail payloads as the baseline. An offset-only
trace classified 23 of 28 collection-detail callbacks as glyph pages, including
13 for the 12 px description font. This was a callback-range classification,
not a new runtime integrity walk or an attribution of calculator time.

`tools/reorder_glyph_payloads.py` addresses that physical layout. The converter
counts Unicode characters in the existing localized strings and orders glyph
payloads using those frequencies. It keeps the directory's keys and ordering
unchanged, changes only glyph offsets and their dedicated payload region, and
preserves two-byte alignment and all existing aliases. It rejects interleaved
unknown regions rather than moving unidentified content. Independent tests
compare decoded antialiased pixels, geometry, non-glyph bytes, shared aliases,
idempotence, malformed layouts and deliberate corruption.

Two policies were measured. Putting all native sizes of each character next to
one another reduced Chinese detail reads, but increased cold grid calls in both
languages. For example, the Chinese removal grid rose from 72 to 81 calls and
the English one from 27 to 33. That candidate was rejected and its evidence
was retained. Keeping font sizes separate improved every tested Chinese grid
and detail. In English it did not improve grids and added one page to two
details, so the English output of that experiment was also excluded from the
release pair.

There are five existing native sizes: 8, 10, 12, 16 and 20 px. The accepted
policy changes physical locality within each size; it does not add fonts,
pre-read the full character set or increase the 64-page reader cache.

## Frozen source and measurement method

| Role | Source identity |
| --- | --- |
| Original runtime baseline | `a2dc843cdef68768c203bc0a427fa0d2b81bb952` |
| Isolated image-batch/glyph-loop stage | `3253b22f8dcd50f354f1b7c094a00e5cb80b279f` |
| Stage-2 runtime | `eb6f9564c6f3c88bc47b73829036af21d0489072` |
| Runtime used for the final resource comparison | `55fdd6e703cd921220740f41e1537f864a02b300` |
| Final sampling driver and converter | `1ad378862fae54fc8c8b346c2680b5f448e26883` |
| Stage-4 release candidate, before split/boot compatibility correction | `1ad378862fae54fc8c8b346c2680b5f448e26883` |

The resource experiment uses **one runtime and one driver** for original and
reordered packs. It must not be described as a measurement of the later release
runtime. For the separate baseline/current hover comparison, the exact same
driver was compiled against each frozen runtime's own headers and static
library. Both used Clang 18, C++17, Release optimization and `NDEBUG`.
Later v22 split-source and boot-language acceptance corrections do not change
which runtime these samples measured. The final delivery manifest identifies
the corrected runtime; the stage-4 candidate above is not that final receipt.

The driver opens the actual private pack through the production ranged `FILE`
source. Every grid reopens `Content` and its reader caches; the operating
system's file cache is uncontrolled. Fixtures use seed 781 and the existing
Ironclad practice run: an eight-card collection, a real ten-card shop-removal
selection, a real draw-pile choice entered through Secret Technique, and the
eight-card collection page containing Rainbow.

The first-open detail sample includes the ordinary Detail input command,
pointer movement outside its description and the complete rendered frame.
It cannot activate combat preparation. The parent page has already been
displayed and animated, as in normal navigation; its naturally resident images
and glyphs are not discarded just to create an artificial detail benchmark.
Each grid, detail and keyword window then advances and renders 180 frames at
33 ms of simulated time per update. No wall-clock pacing is applied.

The keyword sample visits all 472 positions in an 8-by-59 pointer sweep,
including blank rows and text after the first gold word. All pointer movement,
hit tests and the final tooltip frame are included in the sample. A previous
driver stopped at the first match, which happened to be the first position in
both languages; those older samples are not used as evidence for a full sweep.

## Accepted Chinese resource I/O

These are production source callbacks, not operating-system syscall counts or
physical-device storage timings. The table's grid row is the first complete
visible render; the detail row is input through its first complete render.

| Entry | Calls before → after | Bytes before → after |
| --- | ---: | ---: |
| Collection, 8 cards | 71 → 65 | 392,280 → 371,800 |
| Its first detail | 28 → 16 | 125,070 → 66,254 |
| Removal, 10 cards | 72 → 63 | 405,548 → 364,588 |
| Its first detail | 27 → 16 | 125,070 → 67,726 |
| Draw choice, 10 cards | 155 → 121 | 702,102 → 554,646 |
| Its first detail | 46 → 23 | 202,781 → 96,285 |
| Rainbow collection page, 8 cards | 116 → 91 | 590,364 → 483,868 |
| Rainbow's first detail | 34 → 23 | 145,550 → 92,302 |
| Complete pointer sweep and first keyword frame | 0 → 0 | 0 → 0 |

Opening the Chinese resource and fixture adds the same four callbacks and
1,129,006 bytes on both sides. The draw-choice scene also has 23 preparation
updates: its 15 callbacks and 178,351 bytes are unchanged. Including those
preparation costs, that entry falls from 170 to 136 callbacks and 880,453 to
732,997 bytes before the first full page, in addition to the unchanged open
cost. Work was not moved into preparation to improve the visible-frame row.

Every callback and byte count above repeated identically across three serial
trials. All 180-frame warm windows had zero source callbacks and zero source
bytes. All observed canonical saves were unchanged, and the save callback was
never invoked. All 18 paired real-resource captures matched pixel for pixel,
including the eight grid/detail scenes and the keyword frame in both languages.
The converter additionally compares every encoded glyph and every unaffected
section, not just the characters visible in these captures.

The isolated stage-1 image-batch comparison also retained six exact paired
captures. It reduced collection reads by seven calls in each language on the
same v21 input. Stage 2 extended batching to removal, reducing its calls by
nine. Those older figures use different resources/runtime stages and are not
subtracted from the final table as though they were one controlled experiment.

## Native time samples and their limits

Builds, peer tests, compression and the emulator were paused for an approximately
44-second timing window. Each process exited before the next began; the order
alternated across three trials per condition. These are unpaced host CPU
intervals, not display FPS or input latency on a calculator.

| Chinese cold detail | Original median, ms | Reordered median, ms |
| --- | ---: | ---: |
| Collection | 1.407 | 1.403 |
| Removal | 1.561 | 1.425 |
| Draw choice | 1.882 | 1.821 |
| Rainbow | 1.355 | 1.480 |

The collection grid's median rose from 1.987 to 2.112 ms despite fewer resource
reads. CPU samples therefore do not support a general speedup claim. The full
receipt retains ranges and all trial identities, including increases.

The separate 472-position baseline/current sweep on identical v21 resources
had Chinese medians of 3.531 and 2.618 ms; the baseline ranged from 2.687 to
5.084 ms. English medians were 2.857 and 3.032 ms with overlapping ranges.
These samples establish that the actual pointer route ran; they do not justify
an exact or cross-language acceleration percentage. Earlier stage-2 timing
processes overlapped with builds and each other, so their CPU measurements are
explicitly excluded from conclusions.

The final split files reconstruct the selected full packs byte for byte, but
the table above measures complete NSP2 sources, not the split adapter's physical
file accesses. New physical-calculator or ordinary Firebird-route measurements
remain necessary before claiming that the reported detail stall is resolved
on-device. ARM compilation is not that evidence. Existing allocation bounds
are retained; overall release allocation checks are reported by the release
validation rather than inferred from this benchmark.

## Final resource selection and reproducibility

The selected English full pack is unchanged from the final v22 locale build.
Only the Chinese full pack uses `size-frequency`. The three split resources
total **81,249,303 bytes**; glyph reordering adds zero bytes. The common artwork
file remains shared. Split reconstruction passed, and runtime integrity checks
remain disabled. File hashes and exact source identities are in the JSON
receipt; private input/output files are not included in source control.

Run conversion only after saving and verifying its source checkpoint. Example
commands below use owner-provided private paths:

```sh
python3 tools/run_logged.py --checkpoint "$CHECKPOINT" --label glyph-frequency \
  -- python3 tools/reorder_glyph_payloads.py \
  --eng "$ORIGINAL_ENG" --zhs "$ORIGINAL_ZHS" \
  --output "$PRIVATE_CANDIDATE" --order size-frequency

# Preserve the original English order; select only the measured Chinese output.
python3 tools/run_logged.py --checkpoint "$CHECKPOINT" --label release-split \
  -- python3 tools/split_content.py --eng "$ORIGINAL_ENG" \
  --zhs "$PRIVATE_CANDIDATE/glyph-locality-zhs.nsp" --output "$PRIVATE_RELEASE"
```

The source-only unit suite is `python3 -m unittest discover -s tests -p
test_glyph_locality.py`. Actual sampling uses `card_grid_benchmark REAL_PACK
180 PRIVATE_CAPTURE_DIRECTORY`; capture output is optional. Any new layout
must be compared with the same frozen driver/runtime, including preparation,
first-detail input, warm I/O, canonical state and exact pixels. A rejected
candidate remains rejected even when it improves one selected detail.
