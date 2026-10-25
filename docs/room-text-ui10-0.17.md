# UI10 room text and turn banners

Reference remains **v0.111.0 / 41cef1ea**. The private scenes and recovered
methods were read directly. This document records implemented differences;
it does not turn source coverage, native/WASM agreement or synthetic fixtures
into an original-game visual percentage.

## Default events

The previous right column used a16px title,10px body and186×37px buttons.
Those sizes came from the older calculator layout rather than the original
1680×1260 UI. `default_event_layout.tscn` defines a800px VBox at
`(viewport/2-38,255-39)`, a64px title, a280px minimum body, a24px spacer and
800×100px options. The implementation now projects that column to
`(153,41)`,152px wide, with a12px title and19px option rows separated by1px.

`NEventLayout.SetDescription` gives the original26px description to a
RichTextLabel with `fit_content=true`; it grows before the options are placed.
The native body therefore stays at5px and uses at least53px, increasing its
height when the complete wrapped prose requires more space. Four/five-option
pages retain all their rows. The source has no fixed three-option pagination.
Only a list exceeding six options uses the existing bounded calculator page
fallback. Prose that would push controls outside the240px viewport remains
available in the existing complete event reader.

`NEventOptionButton._Ready` builds one rich label: the gold/red `[b]` title,
a newline and the description. Its source24px normal font becomes5px and may
shrink to4px, in the projected `(8,2,138,14)` inner rectangle. Four pixels is
the calculator readability floor; source desktop autosizing can go down to
12px. If that floor still cannot fit a particular translated option, the
complete reader remains the deliberate overflow path. The compact painter
uses the same word wrapping, rich colors and animation semantics, with the
source quarter-alpha shadow and no new bitmap/cache storage.

## Rest and merchant dialogue

`rest_site_room.tscn.ChoicesScreen` is a full-screen Control. It does not use
the artwork's218px room camera. The header now occupies `(65,46,190,15)` with
the projected36px/7px face; the source247px option controls are47px wide,
31px high atY66, with19px separation. Names begin atY97 and use5px; the
description begins at `(82,115)` in156×75px and also uses5px. Hover and click
regions follow those controls. Existing entry/press/focus clocks, disabled
choices, Tent paging, canceled selectors, completion waits and first-Proceed
gates are unchanged.

`merchant_rug_dialogue.tscn` uses normal24px and a18px minimum. At the
inventory's already documented uniform2/9 zoom, the text now uses5→4px in
the actual62×29px rectangle. The former8px label clipped ordinary speech.
The existing bubble animation, text reveal, stock, prices and purchase
operations retain their original owners and saved-state boundaries.

## Locale substitution and turn banners

The private `MegaLabel._Ready` calls `RefreshFont`, which applies
`FontType.Regular`. The Chinese font substitution therefore replaces an
authored English Spectral/Kreon Bold **plain label** with Noto Regular.
`MegaRichTextLabel.RefreshFont` independently selects Regular for normal
text and Bold for explicit `[b]` spans. `NIntent.UpdateIntent` sets only the
formatted value string, so its Chinese damage/repeat value also uses Regular.
The room, banner and intent call sites now follow those actual runtime rules.
The config in `config/room-ui-fonts.json` records only keys, sizes, font roles
and dynamic alphabet families. No localized original text or font bytes are
stored in the public repository.

Player/enemy turn titles use the scene96px/18px Spectral face in English and
Regular substitution in Chinese. The36px counter uses7px. Scene anchors and
the opposite50px motion project to10px, rather than the previous8px from
the obsolete display scale. `NPlayerTurnBanner.Display` exits with300ms
SINE_OUT after its1500ms motion and400ms hold. The presentation now uses that
curve instead of a linear fade. The independent turn-banner test compares
the exit against scalar sine samples. Enemy/player scene lifetimes remain
independent; this change does not delay any actual combat command.

## Repeated enemy attack animation

`AttackCommand.Execute` invokes the attacker animation before every hit unless
`OnlyPlayAnimOnce` was selected. The earlier port retained the damage wait for
each hit but queued the enemy sprite trigger only once at the move's start.
Chomper's second bite and Byrdonis's later pecks therefore showed a stale or
idle sprite while the health bar correctly changed again.

Every observed living-target hit now queues its repeated source trigger at
the end of the preceding Damage wait, before its own animation/callback wait.
The existing millisecond accumulator preserves the source timing across33ms
display ticks. Axebot's OneTwo and other `OnlyPlayAnimOnce` commands retain one
trigger. Explicit no-animation commands stay suppressed; Cubex's valid zero
delay Attack remains supported. Prefix commands such as Louse's Uncurl do not
repeat. The existing24-entry visual queue uses stable chronological insertion
so a newly observed repeat cannot be blocked by a previously queued later
AttackEnd. Capacity, authoritative State, saves and RNG are unchanged.

The regression now checks both cue clocks and the actual sprite start for
Chomper's two hits and Byrdonis's three hits in Normal/Fast, with Axebot as the
once-only counterexample. This extends the former health-only check; syntax
passes and execution remains pending the current preserved checkpoint.

## Verification status and remaining limits

Short Clang syntax checks pass for the changed app and room/event/text/world
and canvas regressions. The independent turn-banner check passes31 checks.
At the earlier stage-A immutable library, the repaired intent fixture passes
the full synthetic world-UI7549 and world-text6811 checks. That repair fixes
two fixture errors: a public pack intentionally has no original hover skin,
and Mecha Knight's Artifact must be removed through the real command before
applying the Weak used by the hover test.

The current room/font stage still requires the parent's remotely preserved
checkpoint, matching converted resources and actual English/Chinese render
and interaction regressions. Its tests assert the source projected control
geometry, fit-content ordering, visible option coverage, real interactions
and unchanged canonical saves. They are not an unmodified-PC screenshot
comparison. Existing event texture stretch/nine-patch adaptation, exact
Godot font rasterization, source-width outline kernels, independent particle
effects and all unrelated custom-event scenes are outside this acceptance.
