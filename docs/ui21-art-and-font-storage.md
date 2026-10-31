# UI21 restored artwork and exact font-stream optimization

Reference remains the owner's StS II v0.111.0 / `41cef1ea` installation.
Original PCK bytes, skeleton samples, glyphs, screenshots and generated packs
remain private. The supplied conversation attachment records UI20's last
delivery as a source/launcher checkpoint without a complete replacement data
set; UI21 restores the matching artwork before running the storage pipeline.

## Rebuilt matching artwork

The accepted UI19 English NSP2/44 logical input has 58,060 images, 1,783 clips
and 5,409 auxiliary entries. The current source converters rebuild eight full
320x240 Ancient room backgrounds with the pinned 1680x1260 scene viewport,
325 action frames in nine enemy clips, Osty's three `dead_loop` samples, and
the original rare/uncommon/special card-reward icons.

The resulting complete English NSP2/44 file is **71,028,106 bytes** with
SHA-256 `d823e669731c75bcf7f62f307f699c0212a211af0f236c36383a4b6777cf66b5`.
It reproduces the complete historical UI20 artwork file byte for byte, with
58,099 images and 5,422 auxiliary entries. All 1,783 prior clip descriptors
remain unchanged. There are 333 image replacements and 39 appended entries.

The action-frame audit covers all ordinary enemies. Nine action clips need
the native-density registration path; no shared-group creature has another
under-density clip. Vantom retains its existing independent layered renderer.
Ceremonial Beast's `plow` gains a 3.004-fold spatial sampling density without
changing its 35 samples or source clock. This is a sampling measurement,
not a frame-rate claim.

The fresh source oracle independently samples the official pinned Spine
runtime and rasterizes each whole frame. Its comparison with the decoded
registered pieces passes all **328 frames and 351 pieces**:

- Maximum visible RGBA byte difference: zero.
- Differences after the existing RGB565/alpha mapping: zero color pixels and
  zero alpha pixels.
- All 17 frames needing multiple pieces preserve coverage and registration.
- No frame count, duration, palette precision or runtime pixel pool changes.

The production Canvas test also passes all 328 frames, off-screen culling,
loop/clamp endpoints and four distinct reward-icon paths. Its 770,035 checks
include capture-write assertions and are not a gameplay-content count.
Selected two-piece/four-piece comparisons, the full Neow background and the
Osty corpse were opened for visual inspection.

Artwork conversion and the source oracle ran against immutable source
`f4875c2eee50ecdee4793d7ecb365a20d34857dc`. The rebuilt official Spine runtime
uses its existing pinned source commit with .NET SDK 8.0.427. Private journals
`ui21-ancient-art-4535b9590dfb`, `ui21-framed-art-0f79cb19aad4`,
`ui21-frame-source-oracle-82d2bba09140` and
`ui21-framed-native-art-9f652b9e4fb4` pass without source changes. The earlier
`ui21-framed-art-3cf0ffa71bec` dependency failure is retained separately; it
failed before producing artwork because Python `lz4` was absent.

## Additional font-stream saving

The UI15 font pipeline already removes obsolete glyph tails and identical
payloads. Its small-label groups remain bounded to 4 KiB for cold access;
large display groups retain their existing bounds. UI21 does not remove
characters or fitted-label sizes, change glyph ink/metrics, or merge groups.

`retune_resource_streams.py` adds only two font encoder candidates,
`(lc, lp, pb) = (0, 1, 0)` and `(1, 1, 0)`, to the existing five. A literal
position context can distinguish the count/alpha bytes of the existing glyph
RLE representation. The offline selection keeps the prior stream unless a
candidate is smaller and independently decodes exactly. These contexts are
within the calculator decoder's existing `lc + lp <= 4` probability bound.

All **511 actual English/Chinese font groups** were measured, comparing the
best existing candidates with the expanded candidate set. These are stored
group spans including the existing even-byte alignment:

| Locale | Groups | Improved groups | Previous best bytes | New best bytes | Saved bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| English | 32 | 25 | 197,421 | 195,177 | 2,244 |
| Simplified Chinese | 479 | 158 | 3,446,662 | 3,419,292 | 27,370 |
| Total | 511 | 183 | 3,644,083 | 3,614,469 | **29,614** |

Every candidate in this measurement round-tripped exactly. Decoded group
sizes, glyph descriptors and cold-access membership are unchanged. The
private receipt is `font-context-full.json`; journal
`ui21-font-context-full-9ae9de596f25` passes without source changes.

The 29,614-byte figure is the incremental font-stream saving over the prior
retuning profiles. It is not the final installation saving or a substitute
for the complete locale, stream, native-reader and package checks. Final
installed sizes must include the launcher, all common concern files and the
selected language files. See the current work-state/release record for that
integrated result and the continuing platform limits.
