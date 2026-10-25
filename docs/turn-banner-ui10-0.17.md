# Turn banner stroke and actual-resource acceptance — UI10

Pinned reference: v0.111.0 / 41cef1ea. This stage changes the existing turn
banner's outline without changing its source font bands, labels, color tweens,
duration, combat waits or canonical state.

The original `player_turn_banner.tscn` and `enemy_turn_banner.tscn` use a
24px outline around their 96px titles. The player's 36px turn counter uses a
16px outline. Both outline colors have alpha 0.501961. Under the existing
1680→320 UI projection these become 5px title and 3px counter strokes. The
enemy label's original 2→1 scale tween also scales its outline, from 9px to
5px. Previously all three labels used a fixed 1px outline throughout.

The wider stroke is combined on a bounded 320-byte coverage scanline before
compositing. It preserves the source half-opacity outline when neighboring
glyph strokes overlap: repeated offset blits would incorrectly accumulate
the same half-opacity black until it was effectively opaque. This also avoids
repeating complete text parsing and glyph lookup for every outline offset.
The painter retains at most 64 codepoint/position pairs, supports ordinary and RLE
alpha records, and uses no allocation, persistent cache, image surface or
additional resource. Each streamed pixel view is consumed before the next
glyph read, so cache eviction cannot invalidate a retained pointer. Its circular bitmap stroke remains a software
approximation of the original font renderer; it is not claimed to reproduce
every MSDF edge pixel.

The timing regression passes 34 assertions, including the independent source
24px/16px projection and enemy initial scale. Final combined runtime and
actual-resource acceptance belongs to the next coherent checkpoint.

## English compound-intent test correction

The preserved stage-D actual-resource combat gate passed all 15 Chinese
modules and 14 of 15 English modules. English `world_ui_regression` failed
while locating Vantom's visible 26 damage label. A short diagnostic against
the unchanged frozen stage-D renderer confirmed that the 26 and its separate
3-card status count were both present.

The failure was in the independent expected-pixel compositor. At 5px, the
original English Kreon Bold digit 6 has a 4-column bitmap and a 3px advance.
The old test allocated a row from advances alone, so the final digit's
overhang and outline wrapped into the next expected row. The compositor now
reserves the actual glyph extent and outline while preserving each glyph's
advance for placement. It still compares all sampled pixels exactly,
including background holes and visible ink; no tolerance or count check was
weakened. The production intent painter is unchanged.

With that test correction, the complete world UI short run against the frozen
stage-D library and original stage-D 37 packs passes 7,665 assertions in each
language without capture output. The original failed receipt remains a failed
receipt; the next coherent checkpoint must rerun the formal acceptance gate.
