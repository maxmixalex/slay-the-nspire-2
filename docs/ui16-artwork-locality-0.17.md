# UI16: retain artwork file locality for short concern islands

## Why this change is needed

The UI16 multipart format preserves the old NSC1 bytes, but its initial concern
assignment introduced additional physical reads when a normal 4 KiB resource
page crossed between short runs in different files. In the fixed native
Haunted Ship / five-Dazed End Turn route, multipart artwork alone changed
17 physical reads / 103,971 bytes / 17 seeks into 20 / 103,971 / 19 and one
cold `rooms` file open. That open evicted the otherwise warm `enemies` handle.
This is a file-layout issue separate from the NSL3 locale suffix alignment.

At action frame 147, the original layout handled these requests:

| Logical NSP2 request | Virtual NSC1 request | Original physical reads |
| --- | --- | --- |
| 62,218,240 + 4,096 | 60,377,072 + 4,096 | interface 2,401,080 + 3,296; rooms 2,631,748 + 800 |
| 62,214,144 + 4,096 | 60,372,976 + 4,096 | interface 2,398,698 + 1,020; rooms 2,630,034 + 1,714; interface 2,399,718 + 1,362 |

The room fragments do not represent the current combat's room. They are
unrelated small retained artwork blocks sharing the resource reader's page.
Original zero-based span 84 contains the 1,714-byte retained group and slice
for image 3430 (`background_front.KAISER_CRAB_BOSS`). Span 86 contains 2,900
bytes covering the retained-group/slice records of images 3436 and 3437
(`events.ANCIENT_OPTION` and `events.ANCIENT_OPTION_OUTLINE`). Both room spans
are surrounded by interface spans.

## General offline rule

After assigning complete decoder groups by their image references and merging
adjacent equal concerns, fold any contiguous A/B/A run when the middle B span
is at most 4,096 bytes. The complete middle span belongs to A. The rule is
independent of scene names, image IDs, locale, and a locale's virtual-page
alignment. It removes two existing boundaries and introduces none. Every
preexisting decoder span remains a whole interval in the result; large
compressed groups retain their concern. No runtime code, cache size, handle
count, image, animation, or original logical byte changes.

Actual results for the complete reference common file:

| Original span | Original concern | New concern | Bytes reassigned |
| --- | --- | --- | ---: |
| 67 | interface | cards | 1,512 |
| 84 | rooms | interface | 1,714 |
| 86 | rooms | interface | 2,900 |

There are 6,126 reassigned payload bytes, with no payload duplication or
removal. The number of spans falls from 95 to 89, and the NSM1 index falls from
1,616 to 1,520 bytes. Total common artwork storage, including all eight part
headers and the index, falls from 74,026,202 to **74,026,106 bytes**. It remains
1,712 bytes larger than the 74,024,394-byte original NSC1 file because of the
multipart metadata. The complete reconstructed NSC1 SHA-256 stays
`24f81a1f67e89ccb6da39c4dd98031a16e951662c10083dda6a088bc6afe5d6d`.

All 16 current locale prefix alignments have fewer artwork page crossings:
94 misaligned file boundaries become 88. The number of distinct affected
4 KiB pages falls from 92–94 to 88, depending on locale. This is a static
layout count, not a claim about every route's physical I/O or frame rate.

## Same-input native result

Checkpoint `373e5abfc157e77410ffda95cd1934dbc1399c0a` was committed with a
source-only bundle before conversion and replay. The diagnostic binary was
copied once from the independent locale I/O investigation; both runs use
that exact executable, the same old NSL2 English locale, and the same actual
saved Haunted Ship encounter. Only the NSM1/part bytes differ. The replay
warms 900 title frames and 240 combat frames, then executes End Turn and
renders 182 frames at fixed 33 ms input steps. It checks five new Dazed cards.

| Action window | Before | Folded layout |
| --- | ---: | ---: |
| Physical reads | 20 | **17** |
| Bytes read | 103,971 | **103,971** |
| Seeks | 19 | **17** |
| Cold artwork opens | 1 | **0** |
| Compared rendered frames | 182 | **182 identical** |

Both complete frame-hash transcripts have SHA-256
`6aed40cf0ab6841dd856bae3b3828d1f891e5d6a87a3243df967a76aa07c7e22`.
The two affected requests now read one contiguous interface range each:
2,401,282 + 4,096 and 2,397,186 + 4,096. The enemies handle remains open.
These are native correctness/I/O results. They do not establish calculator
FPS; the combined NSL3 and multipart correction still needs the ordinary
Firebird device pair. The earlier def1db3 multipart device regression remains
an observed failure until that new pair completes.

The separate locale alignment production change is
`e95f804d2922a3b51567cec6626439ca519fce03`. Its full 16-locale output was then
combined with this 89-span artwork layout and replayed with the same frozen
diagnostic binary:

| Same 182-frame action window | Reads | Bytes | Seeks | Cold opens |
| --- | ---: | ---: | ---: | ---: |
| Original NSC1 + NSL2 | 17 | 103,971 | 17 | 0 |
| Initial NSM1 + NSL3 | 26 | 116,124 | 19 | 1 |
| Folded NSM1 + aligned NSL3 | **19** | **102,505** | **17** | **0** |

All three complete frame transcripts have the same SHA-256 recorded above.
The remaining two additional reads arise in the independent-page locale
wrapper; the combined layout reads 1,466 fewer bytes than the original
container on this native route. This combined check is recorded by the
locale investigation's `locale-io-aligned-combined-full-b05638423c7a` journal,
which completed with return code zero and unchanged diagnostic source
checkpoint `afbde329214e2a188c084fd5d17f979bf8b9ee05`. The source checkpoint is
for a diagnostic driver, not a different production runtime. Device timing
acceptance remains separate.

Seven public synthetic tests cover the exact 4 KiB boundary, larger groups,
end spans, unequal neighbors, all eight concern owners, adjacent same-owner
groups, invalid gaps/overlaps, and deterministic randomized partitions. The
randomized cases check full interval coverage, preservation of each original
span, idempotence, and that every tested request crosses no more boundaries.
The actual converter verifies byte-identical reconstruction of the complete
74 MB common data before writing the candidate.

Local journals in the independent `ui16-artwork-layout` worktree:

- `artwork-fold-synthetic-cbd0c4f48a9f`: passed, 7 tests.
- `artwork-fold-actual-build-0ef44511f490`: passed, complete common identity.
- `artwork-fold-baseline-trace-26a39462abc9`: passed, original multipart layout.
- `artwork-fold-candidate-trace-9732e45025b9`: passed, folded layout.

Private trace binary, input identities, candidate files, and the two frame
transcripts are retained outside the public source tree in
`private-ui16-artwork-layout`. Original assets remain private. An initial
journal invocation using an abbreviated checkpoint was rejected before any
test ran; it was rerun with the full commit ID. No remote publication is
claimed.
