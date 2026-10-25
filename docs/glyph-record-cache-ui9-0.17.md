# Bounded glyph directory memo — UI9

The rendering path measures and draws the same characters repeatedly while
fitting text, centering it and painting its outline. The glyph alpha cache
already avoids repeated payload reads, but `Content::glyph_record` still
performed a full binary search of the resident directory for each request.

A 256-entry exact-key memo now remembers the resolved directory index. It
shares no alpha/pixel storage and adds only 2,052 static bytes on 32-bit ARM
(2,056 on a 64-bit host). Both the key and index use 32 bits; increasing a
format's glyph count therefore does not create a 16-bit index boundary here.
Hash collisions replace an entry and always compare the complete requested
size/face/codepoint key before using its index.

The first lookup retains the existing record/shape/encoding checks and the
same recursive scene-family, regular-face and question-mark fallback order.
The metadata is the immutable resident directory already used by the image
cache. Reopening or failing a Content owner invalidates its record memo.
Switching between distinct owners clears the memo before either can reuse
an index; it cannot borrow a different language's metrics. Cached misses are
also scoped to that owner. A malformed fallback poisons the owner and is
never published as a successful lookup.

No source file is scanned at startup, no payload checksum is added, and no
new range read or decode is introduced. The public Content and save formats
are unchanged. Existing glyph alpha caches, sparse scene-font decisions,
text-fitting order, RGB565 rounding and animation clocks are unchanged.

A short independent public-API check adds 1,024 generated glyph keys, repeated
queries under pressure, distinct metrics for the same keys in two owners,
missing-codepoint fallback, absent size bands, reopening and malformed-record
recovery. The current content regression passes 69,398 checks. This is a
pre-checkpoint correctness result, not a timing or final integration claim.
Actual-resource whole-frame/state/I/O comparisons, matched portable builds
and the final linked ARM allocation are separate acceptance gates.

## Frozen v35 actual-frame comparison

Checkpoint `3e1cabc190dcea2d1971309d719091b0147d6b36` was tested with its
matching native archive and both privately generated v35 packs. The baseline
uses that same frozen source and archive, disabling only the three memo
lookup/remember calls in `Content::glyph_record`. It does not substitute the
older UI or change its glyphs, card geometry, animation clocks or drawing.

The normal card-grid driver exercises the known Ironclad and Rainbow libraries,
deck removal, draw choice, each second-level card inspector, keyword hover,
and both idle/hovered seven-card merchant views. Separate instrumented runs
record every rendered frame's RGB565 fingerprint, all hit-region fields and
canonical State fingerprint. All867 paired frames in each language match:
1,734 frame pairs and133,171,200 pixels per side. The unmodified driver's
canonical before/after byte comparisons also pass, with zero save callbacks.
All24 measured runs have matching stage counts, range calls, bytes, seeks,
visible cards and first/settled/last frame observations.

Six serial AB/BA repetitions per language measured180 advancing frames in
each of11 windows. The complete wall-clock means are2.5503→2.3260ms English
and2.2915→2.3296ms Chinese. They include substantial scheduling outliers during
concurrent development activity: one English candidate inspector window rises
to15.35ms/frame while its other repetitions range2.41–3ms. The corresponding
sums of per-window medians are2.4109→2.0416ms and2.2662→2.1753ms. These noisy
elapsed-time results are retained in full; neither the first promising run nor
the median is promoted to a final performance claim. A focused final
comparison must record thread CPU time as well as elapsed time for a multi-card
page and its inspector after heavy conversion/build work has stopped.

The eight library/choice/inspector warm windows and keyword window perform no
resource reads. Merchant animation continues its deterministic group reads;
those calls and bytes also match. The memo changes resident directory lookup
work, and cannot claim a reduction in image or file I/O. Linked ARM allocation
and physical calculator timing remain separate from this host measurement.
