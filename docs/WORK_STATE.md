# UI21 integration — complete private resources recovered

The active continuation is `work/ui21-integration`, based on UI20
`f4875c2eee50ecdee4793d7ecb365a20d34857dc`. The owner's supplied conversation
attachment has been read in full. The accepted UI19 bilingual and optional
language installers were recovered, and the supplied original four-part game
archive matches the pinned v0.111.0 / `41cef1ea` reference.

- Recovered UI19 bilingual installation: **74,538,517 bytes** including its
  1,724,027-byte launcher; all 16 language data files are available.
- Rebuilt complete UI20 English artwork: **71,028,106 bytes**, SHA-256
  `d823e669731c75bcf7f62f307f699c0212a211af0f236c36383a4b6777cf66b5`;
  independently matches the historical repaired artwork. Source sampling and
  production Canvas acceptance pass 328 frames / 351 pieces.
- Baseline 158 native suites have actually passed. Independent host processes
  exercise the real Ndless storage implementation through abandon/death and
  reopen, retiring sav/bak/tmp without deleting settings or progress.
- This source stage adds occupied-pixel sparse restoration and two exact
  font-stream encoder candidates. Linked final ARM acceptance, complete
  resource compaction, all-locale readback and the two installer ZIPs remain
  in progress; this checkpoint is not a completed player release.
- No current Firebird BootROM/NAND has been recovered. Host rendering, linked
  ARM instruction measurements and historical emulator results must not be
  relabelled as a new ordinary-OS or physical-device frame-rate measurement.

Next: verify this source checkpoint remotely, build its ordinary ARM launcher,
finish the exact compaction chain, validate the matched final data and publish
the bilingual installer and optional-language ZIP. See
[ui21-art-and-font-storage.md](ui21-art-and-font-storage.md) for current art and
font evidence. The preceding UI20 handoff is retained below for provenance.

# UI20 continuation — historical source and launcher checkpoint

Use the current `work/ui20-integration` history, not the archived UI19 handoff.
The source was recovered from remote UI20 `39fc6f1`; no private resource pack,
PCK, generated UI20 artwork or Firebird image was recovered in this continuation.
Full authorization covers commits, pushes and synchronization to `main`.
There was no callable subagent interface in the continuation session; parallel
subprocesses/CI were not represented as agents.

## Frozen implementation and completed checks

- Production runtime: **`f1d5fcdb702806fb3a0c447b46d3120466e89e8c`**.
- Final native test correction checkpoint:
  **`9342f4f8086df7525fddec3c50a9c9e47aa25088`**.
- Final launcher: **1,675,061 bytes**, SHA-256
  `7946c9225cdf94b668f2709f91044c4f9f0e41492535cdac0f140be0b347c1cb`.
- Same-toolchain baseline launcher: 1,724,397 bytes; **49,336 bytes saved**.
- Absolute relocations: 39,394 -> 32,539. Total Zehn allocation:
  22,289,604 -> 22,274,096. Execution/BSS alone grows 11,912 bytes; both
  independent allocation guards still pass. Do not claim all RAM shrank.
- **158/158 native suites**, **399 converter tests**, **54 additional tests**,
  **4,799 linked-ARM memory vectors**, and ordinary ARM / WASM compile-only
  builds pass. Native suites use public fixtures, not original artwork.
- Packed source-key indices preserve all 6,855 entries; their 57,137-check
  production comparison is part of the native suite, not extra game content.

Details, exact journals and the private rebuild chain are in
[ui20-storage.md](ui20-storage.md). The source-plus-launcher archive is a
checkpoint, not a full game installer. No new whole-installation size or
ordinary-device frame rate has been measured. No Library write was verified.

## Concrete additions in this continuation

`packed_content_index.hpp` / `world_content.inc` remove absolute key pointers
and repeated first-dot domains at compile time, without runtime allocation,
resource reads or catalog scans. The release Zehn encoder chooses the smaller
of its existing and one additional fixed profile, independently verifying the
loaded bytes. Its isolated 2,592-byte gain is included in the total above.

`compact_retained_slices.py` is an implemented offline converter for obsolete
and duplicate retained slices within existing groups. It preserves codecs,
live pixels, group identity and 4 KiB phases, verifies every selected locale,
and declines a candidate unless the actual installation gets smaller. Its
nine synthetic test cases pass; real-game resource savings remain unmeasured.
The earlier pointer-only index candidate and failed test journals remain
historical, not accepted production artifacts.

The two final test changes correct initialization and indeterminate padding
comparisons; they do not alter game rules or relax render immutability. The
raw padding failure also reproduces against the previous runtime, with the
same cards/RNG and identical encoded outcomes.

## UI20 work carried forward, not newly completed a second time

The previous source implements bottom-right version text, status hover icons,
card-only preview anchoring, native-density registered enemy frames, Osty
terminal/dead-loop presentation, rarity-specific reward art, full Ancient
viewport, black standalone next-act maps, compact defeat layout, checkpoint
retirement, Continue information anchoring and the menu fade. The source and
historical private-art records are in [ui20-ui.md](ui20-ui.md) and
[ui20-animation.md](ui20-animation.md). The art-dependent repairs cannot be
activated by replacing only the launcher on an old resource installation.

## Remaining acceptance prerequisites

Recover the owner's complete repaired UI20 resource input (or the accepted
UI19 inputs plus pinned original PCK and official private Spine runtime), run
the documented repaired-art/locale/tile/sparse/retained/stream chain, and
measure the final installation. Verify every image through the production
reader and replay the ordinary Firebird feedback/performance scenarios on
that exact matched data and launcher before describing a full release.

The historical bilingual UI19 total was **74,538,517 bytes**; it is not a
new UI20 measurement and did not meet the 65–70 MB default target. Preserve
`state/` when trying a launcher-only checkpoint; do not apply full-package
instructions that delete `data/` unless a complete replacement is available.
Full static Ancient scenes, sampled LCD materials and platform capacities
remain in [ui20-retained-differences.md](ui20-retained-differences.md).

The prior long handoff is preserved verbatim as
[WORK_STATE-ui19.md](WORK_STATE-ui19.md), including its older source identities,
private paths and measurements. Do not treat those paths as currently mounted.
