# UI16 language recovery and original font roles

Reference: **v0.111.0 / 41cef1ea**. This is a source and native-resource change;
calculator timing and physical-device language coverage require separate runs.

## Startup and errors

An installed, saved language still needs only one ordinary functional load.
If that load fails, startup queries the current calculator language and tries
that installed edition, English, then the first remaining installed edition.
Missing, truncated, unreadable or functionally invalid candidates are removed
from this session's available-language mask. The successful language alone is
stored. No full-pack hash, CRC or eager asset scan is added.

The calculator language decoder already maps the original 16 language editions.
Unrecognized tags, including Danish, Dutch, Finnish, Norwegian and Swedish,
select English when its pack is installed. A saved explicit language selection
continues to take priority over the system default. No usable language pack
produces an English recovery screen, independent of game fonts and images.

All built-in startup labels and technical load/save/profile/language errors now
use English. The embedded boot-only CJK font is no longer linked. Normal game
text and successful-action messages retain their selected language.

## Font roles checked against the original

`MegaLabel.RefreshFont` applies `FontType.Regular` to its normal font whenever
`FontManager.NeedsFontSubstitution` returns true. `MegaRichTextLabel.RefreshFont`
does the same for its normal font, but independently selects the locale's Bold
and Italic fonts for explicit rich spans. An authored Bold or Italic **normal
label** therefore becomes Regular in the seven substituted languages.

| Language group | Regular / normal label | Rich bold | Rich italic |
| --- | --- | --- | --- |
| English, German, both Spanish editions, French, Indonesian, Italian, Brazilian Portuguese, Turkish | Authored Kreon Regular/Bold, Spectral Bold or Bitter Italic, according to the scene | Kreon Bold | Bitter Medium Italic |
| Simplified Chinese | Noto Sans Mono CJK SC Regular | Source Han Serif SC Bold | Source Han Serif SC Medium |
| Traditional Chinese | Noto Sans Mono CJK TC Regular | Source Han Serif TC Bold | Source Han Serif TC Medium |
| Japanese | Noto Sans CJK JP Regular | Noto Sans CJK JP Bold | Noto Sans CJK JP Medium |
| Korean | GyeonggiCheonnyeonBatangBold | Same family | Same family |
| Polish, Russian | Fira Sans Extra Condensed Regular | Fira Sans Extra Condensed Bold | Fira Sans Extra Condensed Italic |
| Thai | CSChatThaiUI | Same family | Same family |

Thus the original Chinese UI intentionally mixes a sans normal face with serif
bold/italic rich emphasis. A blanket replacement with one family would lose
that original distinction.

Corrected calls cover Proceed, End Turn, potion popup verbs, View Upgrades,
card-selection banners/skip/inspect/hand headings, map-history headings,
credits major headings, card energy/star labels and ancient dialogue
Continue/keyboard labels. Measurements and cached-card bounds use the same
selected face as their pixels. Rich emphasis is retained.

The evidence is in the pinned private source's
`MegaCrit.Sts2.Core.Localization.Fonts/FontManager.cs`, the language-specific
`*FontPathSet.cs` files, `MegaLabel.cs`, `MegaRichTextLabel.cs`, and these scene
families: `scenes/ui/proceed_button.tscn`, `common_banner.tscn`,
`card_reward_alternative_button.tscn`, `scenes/potions/potion_popup.tscn`,
`scenes/screens/card_selection`, `scenes/events/ancient_event_layout.tscn`,
`scenes/screens/credits_screen.tscn`, `scenes/ui/map_point_history_hover_tip.tscn`,
`scenes/cards/card.tscn`, `scenes/combat/end_turn_button.tscn` and the player-hand
scene. These are references only; original scenes, fonts and strings stay private.

## Validation gates

`boot_locale_regression` exercises system tag/XML/UTF-16 result decoding,
installed-language subsets, saved preference precedence and failure retry
ordering. Every built-in recovery string and actual error-screen pixel buffer
must be identical across all 16 language selections and Unknown.

`locale_editions_regression` has four actual-resource recovery cases for each
non-English edition: absent, truncated, bad magic and a provider read failure
after the split container opens. Each must reopen the real English pack and
render a healthy title screen. Its source-font oracle compares the actual
View Upgrades label's pixels with the original normal-face selection, while
checking that explicit rich bold/italic remain distinct roles.

`tools/audit_locale_fonts_ui16.py` reads the actual private editions and requires
the exact font/size/codepoint records for changed controls and all four deck
sort labels at **8 px**. It rejects hidden `?` and neighbouring-size fallback.
The optional repair pass adds only missing exact source glyphs and preserves
the complete old payload, compression cohorts, artwork and strings. It adds no
runtime work or resident memory.

Execution results are recorded after the source checkpoint, in the integration
report and private command journals. A declared gate is not a passed run.
