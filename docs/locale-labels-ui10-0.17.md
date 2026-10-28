# Original locale font substitution — UI10

Reference: the owner's pinned v0.111.0 / 41cef1ea source and scenes. Original
source, fonts, text and derived resource packs remain private.

`MegaLabel.RefreshFont` substitutes `FontType.Regular` for the ordinary label
font. `MegaRichTextLabel.RefreshFont` independently substitutes Regular, Bold
and Italic for its three rich-text roles. `FontManager` does this for `zhs` but
does not list `eng` as a substitution locale. Consequently a scene-authored
Kreon Bold, Spectral Bold or Bitter Italic **normal** label still uses the
original Noto regular font in Chinese. Explicit rich bold and italic spans keep
their separate roles. Changing all text globally would erase this distinction.

The runtime now chooses the normal face at each confirmed source label. This
does not change the shared RichReader, card text styles or glyph fallback rules.

| Source surface | Implemented correction |
| --- | --- |
| Profiles, Card Library and Bestiary | Ordinary source labels use the locale normal face, including labels whose English authored font is bold/display. |
| Compendium and Statistics | Original normal-label substitution and the separate rich-text bold role are preserved. |
| Shared hover title | The `NHoverTipSet` title follows the normal-label rule; the body retains independent bold spans. |
| Event, rest, merchant and turn banner | Source-sized labels and Chinese normal faces are selected together. |
| Reward banner and card-reward alternatives | Their normal labels follow the runtime substitution instead of selecting a CJK serif bold face from the English scene metadata. |
| Treasure banner, contextual tutorials and Combat Start | Normal labels, including the direct animated-glyph path, use the same locale face for width, metrics and drawing. |
| Ancient name and epithet | Both animated and settled names use the locale normal face. The source22px epithet projects to4px; its final18px size uses the documented4px readability floor. |
| Run-end score, buttons, banner and quotes | Normal source fonts are locale-substituted. The English victory-damage prose also uses the authored Bitter Italic24px projection at5px, matching the death-quote normal family. |

The exact appended bands are declared by localization **keys**, sizes and
replacement alphabets in `profile-ui-fonts.json`, `room-ui-fonts.json` and
`common-ui-fonts.json`. No original localized prose is copied to those public
manifests. The converter reads the private pack, retains every old payload,
collects every reachable event page and formatter branch, and rejects a missing
source-font codepoint or an overflow of the unchanged2MiB directory. The v35
count extension remains bounded and does not allocate another resident pool.

The rest tutorial now calls the same option-picture renderer as the ordinary
rest page. Its lifted controls share the47px icon width,66px column stride,
source hover sample, gold outline and5px label; a second old50px/71px/10px
projection no longer remains under the tutorial overlay.

At the source checkpoint, app syntax and the nine font/split converter unit
tests pass. Both actual input packs have complete original-font cmap coverage
for the requested additions. Matching final-font rendering, composed bilingual
review and ordinary-host execution remain the integrated acceptance gates.
These source findings do not establish a numerical original-game fidelity.
