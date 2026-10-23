# Shared artwork and language storage

`tools/split_content.py` separates a paired, private Chinese/English NSP2/17 or
NSP2/18 build into `nSpire2.common.tns`, `nSpire2.eng.tns` and
`nSpire2.zhs.tns`. Both source packs must have byte-identical contiguous artwork.
The converter checks the complete original checksums, actual language markers,
all image/group ranges and font/text boundaries. It reconstructs and compares
every original byte before publishing the three files and a private receipt.

The common file stores compressed animation groups and static image payloads
once. Each language file stores that language's original resource directory,
frame descriptors, glyphs and strings. The prefix and suffix retain all original
offsets. Runtime `SplitResourceSource` translates a virtual read through at most
three spans: localized prefix, shared artwork, localized suffix. It requires no
allocation, resident mapping table or full-pack buffer; a compile-time assertion
limits the adapter to 256 bytes. Both underlying sources and the adapter must
remain alive while the returned source is in use.

## Container fields

All integers are unsigned little-endian 32-bit. Reserved bytes must be zero.

| Offset | NSC1 common header (64 bytes) | NSL1 language header (96 bytes) |
| --- | --- | --- |
| 0 | `NSC1` magic | `NSL1` magic |
| 4 | Version 1 | Version 1 |
| 8 | Reference `0x41cef1ea` | Reference `0x41cef1ea` |
| 12 | Header bytes, 64 | Header bytes, 96 |
| 16 | Whole common-file bytes | Whole language-file bytes |
| 20 | Artwork bytes | Reconstructed NSP2 bytes |
| 24 | Reserved zero | Localized prefix bytes |
| 28 | Header FNV-1a, excluding this field | Shared artwork bytes |
| 32 | Artwork SHA-256, 32 bytes | Localized suffix bytes |
| 36 | — | Language: 1 English, 2 Simplified Chinese |
| 40 | — | Original NSP2 body FNV-1a |
| 44 | — | Header FNV-1a, excluding this field |
| 48 | — | Bound artwork SHA-256, 32 bytes |
| 80 | — | Reserved zero, 16 bytes |

NSC1 payload is the exact shared artwork span. NSL1 payload is the exact
original prefix immediately followed by the exact suffix. The converter's
receipt records complete source/output SHA-256 and sizes. Its SHA-256 check
binds each language to one artwork payload; runtime compares that identity and
checks header integrity, length arithmetic, expected language and the actual
NSP2 locale marker before handing a source to `Content`.

## Validation and cold startup

`SplitResourceSource::open` reads only the two small headers, the original NSP2
header and the language-marker directory/payload. It does **not** add another
full artwork scan. `Content::open` continues to read and validate the complete
virtual NSP2 checksum, image formats, tile directories, glyphs and strings.
Compressed blocks retain their existing checksums and bounded decode checks.
Opening the adapter alone therefore is not equivalent to opening validated
game content: callers must still call and check `Content::open`.

This change avoids duplicate installation bytes when both languages are
present. It does not claim single-language compression, a faster full integrity
pass or a 30–40 MB installation. Removing or caching the full integrity pass
would be a separate measured design decision. Legacy single-file NSP2 sources
continue to use `Content::open` unchanged.

When new artwork is added, the resource builder must place it before glyph/text
payloads and use the same order in both language builds. The split converter
rejects mixed or differing artwork instead of silently duplicating it or
dropping pixels/frames. Runtime consumers should retain the two handles for
one selected language; changing language requires reopening the content.
