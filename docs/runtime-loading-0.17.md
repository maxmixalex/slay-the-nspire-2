# Runtime loading and decoding in 0.17

## Policy and implemented behavior

The user's 2026-10-23 instruction supersedes the older integrity-preserving
startup design. The game no longer hashes a whole resource pack, checks split
header digests, computes decoded group CRCs, or walks every image, tile index,
glyph, clip and string before presenting the title. These were port-specific
checks and were not prerequisites for original gameplay.

Opening a resource now identifies its supported format/profile, bounds the
resident directory, reads that directory, and reads the actual three-letter
language marker plus its terminator. The only emitted startup stages are Header,
Directory and Ready. Historical progress enum slots remain for source stability;
there is no hidden Integrity, Images or TileIndex phase behind the loading label.
Loading failures use the built-in English/Chinese text without requiring a loaded
font atlas.

Selected resource reads retain the bounds needed to use the data. An image
request validates its dimensions, selected descriptor and encoded range before
decoding. An LZMA2 request requires the supported dictionary property, bounded
input/output, an end marker, complete input consumption and the declared output
size. A selected sparse frame checks its grid, geometry, output length and only
the tile IDs that supply visible span bytes. Unrequested descriptors and index
records, inactive tiles, tile padding and obsolete CRC fields are not scanned.
The existing RGB565, alpha, indexed and sparse output layouts are preserved.

Glyph lookup bounds the selected record and payload. Raw glyph drawing uses its
specified width and height. RLE drawing, including scaled turn-banner text,
consumes encoded pairs only while logical pixels remain and caps a run at the
remaining width × height. This is part of drawing the requested glyph, without
a second validation pass. String reads bound and check the requested terminator;
clip/card/skin lookups bound their selected targets.

## v20 storage and cache planning

NSP2/20 adds retained-picture groups without another decoded pool. A format-7
picture uses a relative group displacement, encoded group size, raw offset and
raw format/size. Relative addressing allows the common artwork payload to stay
identical between language packs. RGR1 groups use the existing LZMA2 decoder and
the existing 5 MiB tile/group pool. The selected picture is copied into the
existing picture cache after its byte range is bounded.

The resident directory also contains one decoded-page-cost byte per image. This
lets multi-card and animation preparation plan exact picture-cache capacity
without decoding pixels or reading descriptor payloads at startup. Older packs
use their geometry-based upper bound until a picture is decoded; the learned
exact cost is then retained when directory tail space is available. The v20
resident table also remains available if no optional tail space is free.

No additional pixel or group pool is introduced. Picture storage stays at
748 × 4096 bytes / 320 keys, the group pool at 40 × 128 KiB, the directory at
2 MiB, and the resource read cache at 64 × 4096 bytes. String-cache keys now
include the requested length as well as owner and offset, adding 32 bytes of
cache metadata so a short cached string cannot authorize a longer aliased view.

## Split resources and saves

The split adapter supports v17–v20 and maps the common-artwork and selected
language files into the same virtual byte stream. It bounds the three mapped
segments and verifies the actual language marker. Historical digest, checksum
and reserved words are ignored. Actual read failures still propagate through
the selected request; reopening clears cached owner data.

Save version remains v15. The four-byte historical footer is consumed but is
not calculated or compared. New saves write zero in that field. Current code
continues to read historical v3–v15 saves, subject to existing game-state and
format checks. This does not promise that old launchers, which required the old
checksum, can read new zero-footer saves. Current semantic roundtrips are the
compatibility criterion; old and new writer bytes are not claimed identical.

The codec still bounds counts and lengths, identifies the save version/reference,
uses the reusable scratch state, and publishes a loaded run only after its game
state is valid. Reserved padding is consumed without integrity checks. The
separate frontend changes remove repeated serialization and checkpoint FNV;
they are maintained with the title/frontend work.

## Hash audit and tests

Remaining production hashes serve functionality: original seed/RNG derivation,
localized-string lookup, button identity, map geometry cache keys and deterministic
creature appearance. They are not resource or save integrity gates. URL fragments
named `hash` are browser location state. Offline historical fixture generators
may still create old-format checksum words to exercise compatibility; no such
work occurs in game startup. Converter/build-tool cleanup is tracked separately
by the integration owner.

The runtime tests exercise directory-only startup, selected read failures,
structural decoder failure and exact output bytes, legacy checksum acceptance,
maximum directory capacity, cached aliases, cache eviction, failed allocation
release, independent resource owners and old save migration. The five-character
UI fixture advances and renders the actual clock until input is available;
it no longer clears `busy` to attempt a card action during a presentation tail.

The initial restoration stage had only syntax and whitespace checks. That
historical limitation is superseded by the frozen UI3 execution reports: all
73 native Release suites, all 73 GitHub ASan/UBSan suites, ARM/WASM builds and
actual bilingual backend comparisons passed. Ordinary Firebird title, battle,
collection, save/continue and normal-exit flows also passed. See
[runtime acceptance](validation-ui3-runtime-0.17.json) and
[Firebird acceptance](validation-ui3-firebird-0.17.json).
The recovered same-v18 host comparison and later resource measurements retain
their own source identities in [retained storage](retained-storage-0.17.md).
No obsolete hash or eager-validation work remains in the measured UI3 startup.
Host file callback counts measure requested reads; host or emulator timings do
not establish physical-calculator startup time. Cold multi-card rendering still
has a measured pause, explicitly retained in the final Firebird report.
