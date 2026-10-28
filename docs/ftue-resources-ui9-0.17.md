# Original singleplayer tutorial resources

The source UI audit uncovered thirteen singleplayer tutorial presentations that
the earlier frontend treated as already seen. They are not part of the audio,
multiplayer or Timeline exclusions. Their trigger, input and profile behavior
are implemented separately from the resource append described here.

## Append-only catalog

`ftue_ui9_assets.py` appends seven source images to the existing NSP2/33 layout:
popup, confirm, confirm outline, pointer arrow and three combat illustrations.
Auxiliary IDs 2051–2057 follow the existing card-rule overlay. Earlier profiles
with 2050 or 2051 auxiliaries remain supported. A complete tutorial profile has
2058 auxiliaries; the format, 1765 animation clips and 8704 string slots do not
change. The existing vertical popup, Ascension icon and settings arrows are
reused.

The popup is retained at its largest source-derived projection, 146×65 pixels;
the remaining ordinary scenes place it at 110×49 or 122×54 according to their
authored bounds. The arrow is 47×39 with its transparency retained. The confirm
texture is 56×18, and its authored controls use the source aspect-fit behavior.
The three 671×512 control projections are 128×98. Their pinned 930×728 source
illustrations are the same in both source locales. No old image is resized,
recolored, re-encoded or omitted by this append.

Thirty-one original locale values fill previously reserved slots 8347–8377.
The profile catalog beginning at 8400 is unchanged. Generic confirm/cancel
continue to use the existing original values at 6098/6099. Localization keys
are public; localized text and generated glyph/pixel bytes remain private.
The converter accepts an empty slot or an already identical tutorial value
from a fresh full build, but rejects an unrelated occupied value.

## Fonts and exact preservation

The append uses the pinned original English Kreon and Chinese Noto/Source Han
faces. It adds only absent normal 4px and explicit-bold 4px glyphs for the ten
standard 22px-source popup bodies, normal/bold 5px for the other tutorial
fields, the Ascension 6px heading, and the reused confirmation popup's
larger body/title bands. Existing glyph keys, metrics, advances, encoded bytes
and alpha masks remain exact. The full original-font rebuild follows the same
tutorial heading rules so later asset rebuilds cannot reintroduce missing
small bold glyphs.

The ordinary battle-start banner following CombatRules uses its existing
`gameplay_ui.BATTLE_START` value at slot3426, with the original display face:
English Spectral Bold and Chinese Source Han Serif Bold. Only this field's
missing12/14/16/18px glyphs are appended; no text slot or image is added for it.

Old grouped glyph and string payloads are relocated opaquely. New text may
replace a grouped empty-string descriptor; other aliases of that empty string
remain untouched. The generalized append helper now recognizes the decoded
empty value, rather than incorrectly interpreting its grouping flag as a text
length. Synthetic bilingual tests compare every string, old glyph and old
image and verify that an occupied grouped slot is still rejected.

Production readback compares all old image/model/clip IDs, immutable payloads,
decoded-cost entries, existing locale values and glyphs, and verifies every
new image and text. It checks the unchanged 32768-glyph directory bound and
complete required tutorial-font coverage. No resident runtime image pool,
glyph cache, startup resource scan or integrity-only hash check is added.

The first production append passed with seven images, all thirty-one original
texts and zero missing required glyphs. Runtime layout review then found that
standard bodies require the original projected 4px face to fit their authored
bounds. The next production attempt correctly failed the tool's older 5px
minimum; the v33 validator now accepts 4px without changing earlier formats.
Three focused converter regressions pass, including both 4px faces, old glyph
equality and rejection of the new size under v32. The corrected production
append at f8701249 passed with3181 English and31675 Chinese glyphs, zero missing
required characters and exact old payloads. The final battle-banner append,
composed runtime/profile/input checks, actual
bilingual tutorial frames and ordinary calculator/browser flows must be
completed before the tutorial feature is considered accepted.
