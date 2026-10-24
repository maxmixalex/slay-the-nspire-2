# UI7 Bestiary behavior and ordering

Reference: **v0.111.0 / 41cef1ea**. The screen is implemented, but complete
original preview effects and final bilingual platform acceptance remain open.

## Source-backed screen

`NBestiary.CreateEntries/AddAct/AddEvents`, `BestiaryEntry`,
`MonsterModel.GenerateBestiaryMoveList` and the default, Kaiser Crab and
Decimillipede layouts define the current catalogue and interaction contract.
There are 101 entries grouped into 29 Overgrowth, 22 Underdocks, 22 Hive,
23 Glory and five event entries. Five headers produce 106 rows when every act
is discovered. Model identifiers are `EnemyId`; encounter entries use
`EncounterId`, not current battle slots.

The ordinary entry gate requires a discovered act and an actual recorded
enemy victory. Unknown entries stay disabled. Clicking selects an entry;
hovering does not. Keyboard focus and confirmation remain distinct. Actions
preview detached actors and do not use the current battle as their context.
The actions/statistics preference persists; character filters require
observed wins or losses. A Decimillipede entry uses the encounter result,
not the first individual segment reaching zero HP.

Nibbit's original `ShouldShowMoveInBestiary` explicitly hides `SLICE_MOVE`.
Its buttons are therefore Butt, Hiss, Hurt and Die. The checkpoint-6 runtime
already had those four buttons; the old regression incorrectly expected at
least five. The corrected fixture requires exactly four and triggers each
button, checking the actual attack, hiss, hurt and die clip in order.

## Correct culture, less work on opening

The original `NBestiary.AddEntries` compares room type, boss encounter title
and then entry title with CurrentCulture. `NGame._EnterTree` sets the default
thread culture to **InvariantCulture**. This differs from the locale-specific
card-library collation contract.

The old port used lowercased ASCII/UTF-8 ordinal insertion sorting. Its
Chinese Overgrowth order differed. `generate_bestiary_order.py` now emits two
101-byte index tables. An independent .NET helper applies the original
comparer; the generator independently uses ICU root collation and rejects
any difference. The helper passed under checkpoint 6 in journal
`ui7-bestiary-invariant-reference-formatted-0f3c6dbdb4c1`. Only numeric entry
indices are published. The first helper attempt rejected the dynamic
TestSubject title before running its comparer; its failed journal remains
`ui7-bestiary-invariant-reference-22b7799b0904`.

`TestSubject.Title` supplies the saved death counter plus eight. Six numeric
boundary values were formatted and compared in each language; none changes
the resulting order. This establishes the ordering metadata, not a persisted
TestSubject death counter that the port does not yet store. Enemy victory
counts must not be substituted for this separate original fact.

Opening the screen now makes one pass over 101 indices, adding visible act
headers and entries. It performs no title insertion sort or repeated locale
lookups. The tables occupy 202 read-only bytes and add no runtime cache or
allocation. This is a removed-work statement, not a measured latency claim.

## Acceptance boundary

The checkpoint-6 native suite stopped at the incorrect Nibbit count. The
corrected action fixture and complete 106-row source-order oracle are
included in checkpoint 8 and await its formal native result. Actual ENG/ZHS
resources are required for final bilingual screen review. Remaining gaps
include phase-dependent preview contexts, complete move-specific VFX,
scene-specific English fonts and the dynamic TestSubject death-count title.

Original prose, source, textures and screenshots remain private.
