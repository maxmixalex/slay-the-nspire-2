# UI21 restored artwork and font-storage evaluation

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

## Osty's complete death and revival sequence

The pinned original `Osty.GenerateAnimator` maps `Dead` to `die`, whose next
state is the looping `dead_loop`; `Revive` selects `revive` and then returns to
the looping idle state. The three newly supplied downed samples complete the
last part of the death presentation. They are not a replacement for the death
motion itself.

The matching pack retains the existing 17-frame `AnimOstyDie` schedule
(17 x 78 ms), adds the three-frame `dead_loop` schedule (3 x 56 ms), and retains
the 12-frame revival schedule (12 x 81 ms). The short original downed loop has
the same corpse pose at LCD precision. `App::sync_character_feedback` triggers
the death clip at the visible positive-HP to zero-HP edge even when queued
combat feedback outlives the canonical combat phase. `combat_actors` finishes
that clip, holds the original downed loop while HP is zero, and follows the
existing revival/idle path when HP becomes positive. The world UI regression
covers the normal death edge and lethal owner-overflow timing; the production
framed regression also covers the downed loop. The private source-symbol and
pack-descriptor receipt is `artwork/osty-death-semantics.json`.

## Font-stream candidate rejected after ARM measurements

The UI15 font pipeline already removes obsolete glyph tails and identical
payloads. Its small-label groups remain bounded to 4 KiB for cold access;
large display groups retain their existing bounds. UI21 does not remove
characters or fitted-label sizes, change glyph ink/metrics, or merge groups.

The private experiment added two font encoder candidates,
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

The **29,614-byte figure belongs to this private candidate, not the delivered
installation**. Subsequent linked ARM measurements found that small stream
savings could increase decoder work. The release therefore keeps the previous
font/text streams; the two added default font candidates were removed from
`retune_resource_streams.py`. No glyphs, sizes or native ink were sacrificed to
recover that space. The candidate's exact-byte proofs remain useful for
explaining the rejected tradeoff, but do not turn it into an accepted speed or
storage improvement.

## Acceptance of the final selected streams

The runtime used for these gates is checkpoint
`7b0cd8b4f2fd824eea781e0e0a853011c3474b1c`. The final offline policy is
`9d5f9966bb061cec48f7f6efeef131aa243540c6`; it does not change the production
decoder. It restores dense groups above 60% occupancy to their original
streams and retains the original font/text streams. The following are complete
logical validation files; installation files share the common artwork rather
than storing two complete copies:

| Locale | Logical bytes | SHA-256 |
| --- | ---: | --- |
| English | 68,726,154 | `36e7c560558617053081bc4c731d838a3916801f6f9ce3cc8747e1c77dc94e40` |
| Simplified Chinese | 73,150,868 | `6094d63ef015d691252e5bb788aed356f7b252d1cd77b5fc6d322646f59ce2ee` |

The full native image oracle first checked the retuned candidate against the
matching repaired NSP2/44 artwork: **58,099 images and 1,170,062,350 decoded
bytes**, byte for byte through memory, ranged IO and incremental preload. Ten
bounded ranges cover the catalog exactly once. The candidate's SHA-256 is
`c28917a1f93de76208fbba9a6b6a410d52b9089251f6e691a499837b52abd11f`.

The final policy changes compressed stream selection, so its verification
explicitly connects the final files to that complete proof:

- All 1,030 unchanged tile/index streams are byte-identical to the verified
  candidate. All 58,099 image dimensions and references remain unchanged;
  the 1,865 retained streams and direct image bytes are also identical.
- The 107 restored dense streams are byte-identical to the original dense
  reference. An independent comparison checks all **95,118,870 complete
  decoded tile bytes**, including hidden RGB and padding, against the
  candidate. The 338 remaining sparse groups satisfy the 60% limit.
- A coverage set of **42 actual images** reaches every restored group. The
  production reader checks these against the repaired artwork through memory,
  ranged IO and incremental preload. This is a targeted final check, not a
  second claimed run of the entire catalog.
- Final English and Chinese artwork is byte-identical over 66,385,036 bytes.
  Their original font/text tails remain byte-identical over 503,314 and
  4,282,700 bytes respectively. Complete production metadata checks pass for
  **13,299 English and 53,632 Chinese glyph records**, 8,704 strings per locale,
  all 1,783 clips, and 40 memory/ranged/reference UI frames per invocation.
- The final production Canvas run again passes all **328 repaired frames and
  351 pieces**, loop/clamp/culling and distinct reward icons. Ten captures,
  totaling 2,304,150 bytes, are byte-identical to the repaired NSP2/44 artwork
  rendered before the storage changes.

The private receipts are `final-oracles/images/exact-image-receipt.json`,
`final-oracles/final-policy-mapping-eng.json`,
`final-oracles/final-changed-native/receipt.json`,
`final-oracles/final-chinese-art-font-binding.json` and
`final-oracles/final-framed-capture-equivalence.json`. Final journals
`ui21-final-policy-mapping-71c44371790b`,
`ui21-final-changed-native-e5ca413a4376`,
`ui21-final-policy-zhs-metadata-5ddac5ef9bb3` and
`ui21-final-policy-framed-f9331cf305ca` all pass without source changes.

## Continuing representation limits

The eight Ancient rooms now use their correct full viewport and original
source pixels, but the complete room remains a static actor/background
composite under the recorded storage exception. Dialogue, choices, healing
and the separately implemented card materials still animate. Osty and other
missing creature motion do not inherit that exception.

Output remains 320x240 RGB565/LDR with integer rasterization, bounded CPU
particles and sampled materials. Existing Affliction material sampling has
eight phases, Infection has 30 source frames, and Ancient card fire has ten;
this release does not reduce those representations. These are not continuous
Godot GPU materials. The exclusions for audio, multiplayer and Timeline, and
the existing cosmetic RNG, hosted-service, RTC/timezone, history, seed/input
and allocation limits remain listed in
[the retained-differences record](ui20-retained-differences.md).

Native pixel checks and linked ARM instruction measurements do not establish
normal-OS or physical-calculator frame rates. This round has no new Firebird
or physical-device timing result.

Final installed sizes must include the launcher, all common concern files and
the selected language files. See the current work-state/release record for the
integrated result and the continuing platform limits.
