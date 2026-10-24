# Native static hover owners — pinned v0.111.0 / 41cef1ea

The remaining ordinary hover panels now use the shared `NHoverTipSet`
projection: a 360-unit column becomes 69 calculator pixels, with the existing
native 5px font, source title/body margins, normal/debuff artwork and one-pixel
projected shadow. This replaces the legacy 144–204px readers used by these
owners. Long bodies wrap through the same layout; they are not shortened to
the former arbitrary three or seven lines.

| Owner | Source contract | Calculator placement |
| --- | --- | --- |
| Energy counter | `NEnergyCounter.OnHovered`, offset `(-70,-200)` | Counter origin + `(-13,-38)` |
| Star counter | `NStarCounter.OnHovered`, offset `(-34,-300)` | Counter origin + `(-6,-57)` |
| Orb / empty slot | `NOrb.OnFocus`, bounds owner, default 75% alignment | Right of bounds; left above the 75% horizontal threshold |
| Gameplay/input setting | `NSettingsScreen.settingTipsOffset`, `(1012,-60)` | Setting origin + `(193,-11)` |
| Map legend | `NMapLegendItem.OnFocus`, containing column's bottom/right | Below the adapted six-row legend column, right-aligned |
| Draw / erase / clear | Drawing tool holder + `(10,-132)` | Toolbar origin + `(2,-25)` |
| Summary badge | `NBadge.OnFocus`, offset `(10,-132)` | Badge origin + `(2,-25)` |
| Summary discovery | `NDiscoveredItem.OnFocus`, offset `(-76,-200)` | Discovery origin + `(-14,-38)` |

Viewport overflow is corrected by the shared helper. The map column retains
its established 4:3 layout; its tooltip follows that adapted column rather
than a hard-coded wide panel spanning the map. Orb tips keep the real dynamic
Passive/Evoke values and the source orb header icon. Empty slots have their
own text and no orb icon. Settings now show both the original header and body,
including the distinct `SCREENSHAKE_HEADER` resource key. Scrolling a settings
row moves its hover origin with it.

Summary discovery bodies preserve the source's ten-name limit and trailing
four dots (`NRunSummary.GetDiscoveryBodyText`); this is not an adapter truncation.
The focus ornament and tooltip are independent. The actual-resource summary
fixture compares all differing pixels, excluding the focused owner's ornament,
against the projected tooltip column and shadow. Energy/star/orb checks exercise
both sides, empty slots and strict gameplay-save identity. Settings and map
captures use the ordinary drawing functions and both real locales.

These changes add no persistent state, gameplay fields or image/glyph pools.
Source syntax passes; matching frozen native, actual-resource and portable
execution is recorded separately in `WORK_STATE.md`. Source coordinates and
test counts do not establish a numerical whole-game fidelity percentage.
