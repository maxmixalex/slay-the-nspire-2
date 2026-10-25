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
