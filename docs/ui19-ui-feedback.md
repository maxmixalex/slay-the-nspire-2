# UI19 feedback fixes: layout, card inspection and version reset

Reference: StS II v0.111.0 / 41cef1ea. Current presentation label:
`nSpire 0.1.0 (StS II v0.111.0)`.

## Implemented behavior

- Upgrade selection reserves a separate bottom row for **View Upgrades**. Both
  Smith and the world upgrade selector clip their card viewport at y=190;
  the checkbox occupies y=195..209. Hovered, enlarged and selected cards use
  the same clipped `CardGridPainter` surface, and card hit regions are clipped
  independently. Other card grids retain their existing height.
- Treasure's common-banner label moved up three LCD pixels. Its text now sits
  on the ribbon, retaining the source locale font dispatch and horizontal
  treatment for long translations.
- Floor numbers use centered text within their fixed top-bar field. The
  field and boss icon positions do not change when a second digit appears.
- Orbiting Sovereign Blades now expose a transformed hit rectangle, a hover
  reticle and their actual live card preview. The preview shows the forged
  instance's damage and current enchantment, followed by its keyword tips.
  Attacking/removing swords and held targeting suppress this inspection.
  Clicking an orbiting sword is explicitly a no-op. A tooltip owns a copy of
  the instance facts; it does not keep a pointer into a temporary preview.
  Hovering freezes that sword's orbit, as in the original, and leaving resumes
  from the same phase. Forge, attack and removal animation clocks continue;
  cancelling the pointer or beginning an attack releases the orbit pause.
- Continuing from defeat fades the original death quote at its original
  coordinates. The subsequent encounter quote uses independent geometry,
  and its height is reserved from the start of the score animation. The
  outgoing quote and score list no longer jump during the transition.
  The banner, quote column and score list are more compact.
- The title's Continue information panel moved left by 23 pixels.
- Character descriptions gained four more pixels of left/top inset and a
  narrower text column. Random character retains the original unknown HP
  (`??/??`) and gold (`???`) display. The private reference's
  `NCharacterSelectScreen` explicitly supplies those values.
- Active VERSION, title label, web template title/help labels, package
  metadata and release filenames now use 0.1.0 / UI19. The short run-history
  version field remains within its established 16-byte capacity. Historical
  reports and save-codec version numbers were not renamed.

The private `NSovereignBladeVfx.UpdateHoverTip` reference creates a card hover
set while the focused sword is not attacking and target selection is idle.
The new implementation follows that behavior with the calculator's bounded
card/keyword layout. Its `_Process` advances `OrbitProgress` only while no
hover-tip set exists. `NGameOverScreen.AnimateInQuote` fades the existing quote
before replacing it; UI19 retains its position during that outgoing fade.

## Frozen native verification

These tests use explicit `CHECK` functions that exit on failure, so Release
`NDEBUG` does not remove their assertions.

- Checkpoint `a220d8cb86a64b04057c68ef43968ae47d302601`: frozen clang-18
  targeted build and all eight selected regressions passed.
- Checkpoint `0c39135`: rebuilt the locale oracle and additional visual cases;
  all eight selected regressions passed again. The selected targets were
  deck selection, world selection, game-over facts, combat UI14, five-character
  UI, world frontend, keyword tooltip and world game-over UI.
- With unchanged private NSP2/42 source assets, English and Simplified Chinese
  each passed deck selection, world selection, five-character UI, frontend UI
  and game-over UI. These runs render complete 320x240 frames. The floor
  screenshot reaches floor 11 through the world controller rather than
  changing a display string.
- Checkpoint `bc08f1c5058a5fb6a2344c0d523764bd4906fd5c`: the sword regression
  accepts the existing monolithic resource packs as well as the historical
  split fixture. Synthetic, English and Chinese runs passed. Original-art
  runs verify a forged 40-damage sword, bounded tooltip placement, no click
  action, attack suppression and current Sharp enchantment facts. The
  captured tooltip's copied amount stays unchanged when the live instance
  changes, and the next inspection observes the new amount. Matching private
  NSR/NSH device fixtures round-trip through the production codecs.
- Checkpoint `66a99a0dfdba99e06446b90a0cf589ee156f8d5e` exercises the real
  Sharp-enchantment transaction, including the runtime revision update. The
  tooltip reports 43 damage for Sharp 3 and 47 damage for Sharp 7. This avoids
  treating a test-only mutation without cache invalidation as a valid game
  transaction. Synthetic and both original-art locale runs passed.
- Checkpoint `c783dcf04d5865d2a1b1754891fc88155ac25c7e`: the frozen targeted
  build and synthetic/English/Chinese sword suites passed. Tests hold orbit
  position and phase across successive hover frames, resume continuously,
  verify that forging still enlarges and settles the sword, and check attack
  completion and pointer cancellation. A real forged sword also stays fixed
  across a 660-ms UI update while the canonical run state remains unchanged.

The Chinese View Upgrades pixel test initially failed because its old oracle
hardcoded authored face 5, while the runtime correctly applies the original
Chinese default-font override. The oracle now chooses the same documented
locale font family and still independently decodes and compares every label
pixel. No mismatch threshold was relaxed.

The initial default GCC Release build hit an existing compiler ICE in
`world_relics.cpp`; clang-18 completed the frozen validation. An early build
continued while a later test edit was made and its journal correctly reports
`source_changed`; it is not acceptance evidence. One early English capture
was invoked with an incorrect checkpoint string and is likewise excluded.
The frozen runs above supersede both.

## Ordinary calculator UI verification

Firebird uses the unchanged controller/core and the ordinary Ndless launcher.
Only a copied working NAND is written. Snapshot relocation changes its copied
header's flash path; the decompressed payload after the standard 1032-byte
header was verified unchanged. A game snapshot taken before flushing the
working flash failed to resume; the successful capture sequence flushes first
and then records the snapshot. No CPU, memory, scheduling or game patches were
used to make the emulator run.

Initial UI launcher checkpoint
`3fd43e977bcf7d84c593627a9e9f8b1c05926b60`, SHA-256
`3f9fdc1644bfbce68e103d8bafc422107bf51ff24aa48362fdd5663b5ac16e25`,
was installed and read back with the same hash against existing NSP2/42 data.
Ordinary 320x240 LCD capture and visual review confirmed the Continue panel,
complete version label, Smith checkbox separation, treasure ribbon alignment,
compact defeat summary and stable outgoing quote position after Continue.
These controlled room/death states exercise presentation; they are not
claims of a naturally completed run.

The integrated ordinary launcher
`8411656dc3899a3b7f32e29785ca39e720217f11`, SHA-256
`8f5763afb0dc40736d1c1d54a8b80eb783bf7f058b11ac1a2948123638dc124d`,
was separately installed and read back. With NSP2/42, the saved Cosmic
Indifference choice resumed, the selected Wound visibly flew left toward Draw,
and the settled pile counts were Draw 6 / Discard 2. Save and Quit visibly
faded the pause screen out and the title screen in. After an OS exit and
relaunch with no USB operation between them, Continue restored those pile
counts. The ordinary file-backed resource reader reported no read failures.

Console commands deliberately detach their state from an existing run
checkpoint. Consequently, console `die` is suitable for inspecting defeat
presentation but cannot establish natural-death checkpoint deletion. That
storage behavior is covered by integrated lifecycle regressions; its ordinary
enemy-damage device check is still pending at this report checkpoint. Further
integrated behavior/resource/performance receipts belong in the final UI19
device/storage report.

## Evidence and limits

Private LCD files and logs remain outside the source tree. Key captures are
`upgrade-pointer`, `treasure-open`, `defeat-intro`, `defeat-continue-90ms`,
`defeat-summary`, `random-character`, `hud-two-digit-floor`,
`sovereign-blade-hover`, `sovereign-blade-enchanted-hover`,
`cosmic-return-150ms`, `cosmic-return-250ms`, `save-quit-0800ms`,
`save-quit-1100ms`, `save-quit-1600ms` and `reopened-cosmic-combat`.
All are 320x240 frames; the test oracle, native screenshots and Firebird runs
have distinct purposes. This report does not claim physical-device frame
rates, all-locale visual review or a manual browser acceptance run.

Source stages are local commits with source-only backup checkpoints. Automatic
approval review rejected publication of the accumulated source history, so
no remote push or read-back is claimed for this UI19 work.
