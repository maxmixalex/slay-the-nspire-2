# Ancient dialogue layout and interaction — UI8

Reference: **Slay the Spire 2 v0.111.0 / 41cef1ea**. This change implements the
original single-player Ancient conversation presentation. It is not a claim
of an overall fidelity percentage or completed calculator acceptance.

## Source contracts

The implementation follows `NAncientEventLayout`, `NAncientDialogueLine`,
`NAncientDialogueHitbox`, `NEventLayout`, `NEventRoom`, `AncientDialogueSet`
and the eight Ancient models, together with `ancient_event_layout.tscn`,
`ancient_dialogue_line.tscn` and `ancient_event_option_button.tscn`. The locked
4:3 viewport is 1680×1260, projected by 4/21 into the calculator surface.
Original source, scenes, locale content and artwork remain private inputs.

| Original control | 320×240 implementation |
| --- | --- |
| 1160×720 content clip at (260,500) | x50, y95, width221, bottom232 |
| Short content clip before the final line | bottom217 |
| Centered 1000×100 option | x65, width190, height19 |
| Option separation −4 source pixels | starts at 0,18,36 pixels |
| Centered 800-wide dialogue line | x84, width153 |
| 56×56 Ancient/character speaker image | 11×11, on its source side |
| 634-wide text, source 24px font | width121, explicit 6px minimum |
| Dialogue separation −4 source pixels | −1 pixel |
| Content movement, Expo Out | one second |
| New line / stale-line hover fade | 100ms |
| Stale line opacity | 25%, restored to full on history hover |
| Final-line keyboard focus | enabled after 800ms |

The 6px minimum is a declared low-resolution adaptation. The text's complete
height grows; the viewport clips and can review the history rather than
silently dropping old text. Source bubble colors, per-model button RGBA,
source speaker images, nine-patch dialogue and mirrored tails are retained.
The option image has no HSV shader in its Ancient scene, so ordinary-event
HSV shading is not applied to its original color.

The old 292×40 option buttons, enlarged speaker portraits, two-line-only
speech paragraph and static duplicated title were removed. The separate
source banner helper draws last in the scene hierarchy. Source relic icons
are 11×11 in each option, followed by a single gold/red bold title and its
description. At the explicit 6px minimum, an overlong option ellipsizes inside
its source rectangle; the existing full reader exposes every option's rule.

## Actual dialogue and controls

All displayed dialogue refers to keys already present in the installed
original locale. Each line is resolved as `.ancient` or `.char`; its source
`.next` text drives Continue. The fake-next label, original little arrow and
original keyboard keycap use the current remapped Confirm key. No new story
text is fabricated.

Before the last line, the source whole-room dialogue hitbox or Confirm
advances the conversation. Options are absent. On the last line the clip
expands, Continue disappears and the source options become available to the
pointer; keyboard selection waits 800ms. Vertical focus wraps through the
options and horizontal neighbors stay on the same option, as `NEventLayout`
specifies. The full reader provides the complete revealed conversation,
identified speakers and, only at the final line, available options.

Previously revealed lines remain in the VBox. Hovering any old visible line
restores all stale lines to full opacity, and leaving fades them back to a
quarter opacity. Wheel review moves this same content within its clip. During
an unfinished conversation, vertical keys also review it. The existing full
reader supports larger-font keyboard-only review of all revealed lines.

Refreshing the event's actual page/options calls the equivalent of
`ClearDialogue`. A completed event therefore shows its Proceed option without
inventing a completion speech bubble. Returning from an in-room card selector
keeps the conversation binding so a modifier chain does not replay a welcome.

## Historical selection and bounded state

The original uses profile AncientStats to select first-ever, character-visit
and repeatable conversations. The port uses only real retained UI run/floor
records. The locked models have character-specific sequences at prior visit
counts 0, 1 and 4, and original repeatable character/ANY lines. Tezcatara's
ANY-line exclusion for Defect is preserved.

The existing history stores at most 32 runs. If older records have rolled off,
the current run is partial, or the prior profile history was unavailable,
retained counts are treated as lower bounds rather than exact visit numbers.
The implementation then selects only eligible original repeatable material;
when no prior encounter is recorded it uses the canonical original first-ever
line as an explicit historyless fallback. This does **not** reconstruct a
missing lifetime AncientStats counter. No new progress/save version is added.

The presentation has eight bounded key slots (the locked data currently uses
at most three lines). It binds run seed, profile, UI history serial, character,
act, map point and event identity. Its small private random stream only chooses
among eligible source conversations. Advancing, hovering, reading and scrolling
never consume gameplay RNG or change the simulation save. A 30-pixel temporary
surface mirrors the existing original tail; no permanent bitmap pool is added.

## Verification status

The integrated source passes Clang 18 syntax checks. The updated native
regression adds the original source option geometry and a real Darv visit-4
conversation fixture. It checks keyboard and pointer advance, mixed speakers,
no early relic choice, the final keyboard delay, vertical focus, history
opacity/review, unknown-history fallback and clearing after completion, with
byte-for-byte simulation saves around presentation actions.

At this source checkpoint those new behavioral tests and English/Chinese
original-art captures have not yet run. They require the frozen stage-five
source and its matching auxiliary/font pack; stage-four packs cannot be used
as evidence for the later 2013-entry asset directory. Final ARM allocation and
ordinary Firebird interaction remain subsequent gates.
