# NSP2/33 lossless grouped strings

This storage extension retains every localized UTF-8 byte and string index.
It shares resource version 33 with the optional reversible TGR2 tile filter;
either feature may occur independently. Existing raw string directories remain
valid, and runtime versions 3–32 remain readable. Glyph grouping still begins
at version 32. No State, save or history format changes are involved.

## On-disk contract

A string directory remains two little-endian unsigned 32-bit fields: offset and
length. Bit `0x80000000` of the length identifies grouped storage. The low bits
retain the original UTF-8 length excluding its terminal NUL. The string remains
strictly below 16,384 bytes; the longest accepted payload includes 16,383 text
bytes plus its NUL.

The offset of a grouped string points to the same 16-byte descriptor shape used
by grouped glyphs:

| Offset | Field | Required meaning |
| --- | --- | --- |
| 0 | signed relative group offset | RGR1 start relative to this descriptor |
| 4 | group byte length | Header and complete raw LZMA2 stream |
| 8 | decoded slice offset | First byte of the exact UTF-8 payload |
| 12 | decoded slice length | Original string length plus its NUL |

The descriptor must not overlap its group. Groups use the existing RGR1 header,
property 14 and 512 KiB codec dictionary, with at most **32 KiB decoded bytes**.
The item count is positive and no greater than 16,384; the final three header
words are zero. The runtime checks the small decoded bound and header before
loading a group, including when another record has already warmed that group.
A slice must fit wholly within the decoded group and end with a NUL.

The locale marker at string index 6110 always remains literal, including any
identical aliases. `Content::open`, split-resource probing and boot error text
therefore require no string decompression or extra preload pass. The latest
version gates accept 33, and future-version fixtures now test 34.

## Runtime ownership and lifetime

A cold request first checks the existing 102 KiB, 1,024-entry string cache. A
miss borrows the existing transfer workspace, loads through the existing decoded
resource-group cache and copies only the requested slice into the ordinary
string cache. It does not allocate another static buffer or cache. Cached text
continues to work during a Save/Codec lease. A cold request during an incompatible
lease returns an empty result without reading, evicting text, changing the save
workspace or poisoning Content health; a later request can retry.

Returned views retain their prior string-cache lifetime: unrelated glyph and
image loads, including eviction of the original compressed string group, do not
move or invalidate a surviving string entry. String-cache eviction or reopening
its Content owner can invalidate it, as before. Direct-memory packs use this same
stable cache for grouped values; their literal values still point directly into
the input. Read or decode failures publish no new valid text entry and mark the
Content unhealthy; reopening clears that owner's caches.

## Converter and verification

`group_strings.py` walks existing physical string order, deduplicates exact
payloads and uses a group only when compressed data plus all descriptors and
alignment saves bytes. It preserves the complete non-string prefix, including
all artwork, TGR1/TGR2 streams, already grouped glyphs and model/animation tables.
Only string offsets/storage flags and the string tail are rebuilt. Its output
is atomically published after exact readback, and input/output/receipt paths
must be distinct private paths.

`PackView.string_payload(index)` returns the exact UTF-8 payload **including**
NUL; `string_length(index)` returns the original length without NUL or flags.
Raw directory tuples retain their storage flags so unrelated relocation tools
can copy them without losing the representation. The split writer checks grouped
string bounds and keeps every text group outside the common artwork range.

The new Python regressions independently decompress and compare every string,
cover rich text, Unicode, maximum length, aliases, literal boot probing,
incompressible fallback, preserved grouped glyphs, idempotence, bilingual split
roundtrips, reordered indices, malformed bounds/headers and corrupted streams.
Five new tests pass together with eight glyph-group and three prior split tests.

The native `grouped_string_regression` exercises raw/grouped and direct/ranged
readers, the 16,383-byte maximum, exact aliases, busy Save leases, failure/reopen,
malformed bounds and NUL, and borrowed small/maximum string pointers after eleven
512 KiB image-group loads evict their original group. Its actual-pack mode
compares every indexed string and then reuses 128 strings for 64 warm frames
without new provider reads.

## Executed production and runtime checks

Production conversion used remotely saved checkpoint
`4e51f970133a3fa9625c76eceb1838a851dc86c8`, after appending the original card
resource overlay. Both locales use 42 groups and retain their literal language
marker. These are measured converter results, before the subsequent TGR2 pass:

| Locale | Input bytes | Output bytes | String saving |
| --- | ---: | ---: | ---: |
| English | 91,313,773 | 90,606,342 | 707,431 |
| Chinese | 94,312,354 | 93,601,654 | 710,700 |

The converter compared all 8,704 indexed strings in each locale and preserved
the complete glyph, art and animation payloads. Exact receipts are retained
with the private build journal; superseded intermediate packs were retired only
after checking their hashes against a verified exact successor.

The frozen native UI build
`474e592f112ab64b9116f80e5aa20cba4a9231c7` subsequently ran the complete native
string regression against the original v32 and final v33 packs. Each locale
passed 26,133 checks, including all 8,704 strings through direct and ranged
readers, maximum-length text, aliases, malformed inputs, busy Save leases and
pointer stability after compressed-group eviction. Reusing 128 strings for
64 warm frames caused zero additional provider reads. Opening read 1,049,110
bytes for English and 1,498,918 for Chinese, including the existing metadata
directory; no compressed string was needed for the language probe.

After the independent TGR2 pass and 24 missing original Chinese hover-title
glyphs were appended, exact bilingual splitting produced 84,719,566 common
bytes, 2,153,050 English bytes and 5,149,970 Chinese bytes. Their combined
92,022,586 bytes exclude the launcher and user profiles. Device installation
headroom must therefore be assessed using the final launcher and profile-growth
budget, not just whether the resource files fit.
