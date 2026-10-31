# UI21 completed matched calculator release

The active release is `work/ui21-integration`, based on UI20
`f4875c2eee50ecdee4793d7ecb365a20d34857dc`. The owner's supplied conversation
attachment was read in full. The accepted UI19 bilingual and optional-language
installers were recovered, and the supplied four-part original game archive
matches the pinned v0.111.0 / `41cef1ea` reference. Both complete UI21 player
ZIPs are built with the matching repaired artwork and all sixteen locales.
Original assets, derived packs, private captures and calculator firmware remain
outside the public repository.

## Frozen source and complete artifacts

- Runtime: **`7b0cd8b4f2fd824eea781e0e0a853011c3474b1c`**.
- Permanent filesystem regression: `c8dbcd16739762b389858e912742dd384f301f96`.
- Final offline selection policy: `9d5f9966bb061cec48f7f6efeef131aa243540c6`.
- Packaging instructions: `01176770c7bcb29c63e03f0bdf302fcd701669ac`.
- Final launcher: **1,679,979 bytes**; formal allocation **22,291,016 bytes**,
  below 24 MiB. The same-toolchain UI20 launcher is 1,679,626 bytes: the new
  sparse inverse adds 353 bytes, not a launcher-size reduction. The older
  delivered UI19 launcher was 1,724,027 bytes.

| Private release ZIP | ZIP bytes | SHA-256 |
| --- | ---: | --- |
| Slay-the-Nspire-2-0.1.0-ui21-bilingual-calculator.zip | 72,473,950 | `172789594bfa5509b4f1567db5843abb90e6402e657ab1b4f4dda359d5f10cef` |
| Slay-the-Nspire-2-0.1.0-ui21-optional-languages.zip | 17,035,233 | `752f0b338622f0f1917cbbd947c26f83ddac9badd4bdf0743b53d6d9e777a7e7` |

Default installation is **73,295,445 bytes** for the launcher and required
`.tns` resources, before saves; the complete extracted bilingual ZIP including
README, build information and licenses is **73,367,990 bytes**. The fourteen
optional locale files add **18,507,815 bytes** if all are installed; users can
select only the languages they need. Preserve `state/` and replace the complete
matching launcher/data set, including any previously installed optional locale.

The comparable UI19 default `.tns` installation was **74,538,517 bytes**.
UI21 is **1,243,072 bytes smaller (1.67%)** while adding **1,105,032 bytes** of
repaired source-derived artwork. Against the same complete artwork, the data
pipeline saves **2,304,056 bytes**. The historical 65-70 decimal-MB target is
not reached: 73.30 MB is 69.90 MiB, and the unit difference is not an extra gain.

## Completed functional and resource acceptance

- Rebuilt complete English artwork is 71,028,106 bytes, SHA-256
  `d823e669731c75bcf7f62f307f699c0212a211af0f236c36383a4b6777cf66b5`,
  reproducing the historical repaired artwork exactly. Eight complete Ancient
  backgrounds, nine native-density enemy clips, Osty's terminal pose and
  rarity-specific reward icons are present in the actual delivered data.
- All **159 native targets** have executed and passed. The new public test
  runs four lifecycle scenarios in twelve independent processes using the
  production Ndless run callbacks and synthetic resources.
- Final English/Chinese resources pass **16/16 production-Canvas UI runs** and
  **20/20 filesystem/icon subprocesses**, including title abandonment, pause
  abandonment, death, act-2/act-3 abandonment and independent restart. Active
  sav/bak/tmp files retire; unrelated profiles and preferences remain intact.
- **58,099 images** pass exact memory/ranged/preload reader comparison. The
  107 groups changed by the final density decision additionally pass complete
  95,118,870-byte record comparison and production-reader image coverage;
  unchanged group streams and image references are byte-identical. Final
  glyphs, strings and clip descriptors retain the matching source semantics.
- The source oracle and final Canvas pass **328 frames / 351 pieces**. Ten
  frame captures remain byte-identical after the final compression and runtime
  changes. Osty's existing death animation completes before its source corpse
  loop, with the existing revive action retained.
- Linked ARM passes **44 sparse vectors**, **4,799 memory vectors** and exact
  ABI/bounds checks. Sparse inverse instruction work falls 10.39-66.92% in the
  twelve controlled windows. Complete decoder samples include low/mid/high
  occupancy improvements of 22.77%, 12.25% and 8.65%; these are instruction
  counts, not device frame rates.

Dense low-benefit sparse conversions and font/text retuning were rejected
after complete ARM decoder measurements showed extra work. The release caps
new sparse selection at 60% occupancy and preserves prior font/text streams.
The 29,614-byte font-profile candidate, color-plane alternatives and cold-TU
compiler candidates are not shipped gains. There are no added resident pixel
buffers or startup-wide integrity scans.

## Evidence and retained platform boundaries

See [ui21-storage.md](ui21-storage.md),
[ui21-ui-acceptance.md](ui21-ui-acceptance.md) and
[ui21-art-and-font-storage.md](ui21-art-and-font-storage.md) for measured
acceptance, rejected candidates and exact resource identities. Continue from
these complete UI21 artifacts rather than the historical launcher-only UI20
checkpoint. No currently recovered Firebird BootROM/NAND was available in this
session, so no new ordinary-OS or physical-calculator FPS is claimed. Static
Ancient compositing, sampled LCD materials, the established display/input
capacities and the existing audio/multiplayer/Timeline exclusions remain
explicit platform boundaries. The preceding UI20 handoff is retained below
for provenance.

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
