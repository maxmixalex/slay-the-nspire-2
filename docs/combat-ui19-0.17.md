# UI19 combat, end-of-run and transition corrections

Reference: the privately supplied **StS II v0.111.0 / 41cef1ea**. Source-only
implementation through `bd8d228243e2bad0a9ed906aa7716d985947ae59`.
The ordinary integrated calculator build and Firebird acceptance are recorded
by the UI19 delivery work; the host checks below do not establish device FPS.

## What changed

- **Returning cards target their actual destination.** A genuine Discard →
  Draw transition now redirects its real selected holder to the draw pile.
  Cosmic Indifference and Headbutt share this observation path, including the
  original one-card selection shortcut. A card still finishing its previous
  discard flight changes destination instead of retaining the old flight.
  Selecting from the pile preserves the grid holder's visible origin. Count
  badges add the arriving card only when its flight reaches the destination.
- **All observed newly created draw/discard cards receive the same preview.**
  The path no longer depends solely on `EnemyGenerateCard`. It also covers
  Necrobinder's `NMakeSoul`, `EventCardMaterialize`, and creation in other
  observed card/relic commands. The original single shuffled-card back remains
  separate: observing Shuffle normalizes the old discard membership before
  drawing, so a shuffle does not create one return animation per old card.
- **A resumed pending choice rebinds its continuation presentation.** Loading
  a valid pending pile selection previously discarded the command trace and
  could resume its result without the usual transfer feedback. The current
  state is now observed before accepting the answer. A same-zone transformation
  releases its temporary selected holder, preventing a stranded card above the
  battlefield. Neither change writes presentation data into the save format.
- **Draw/discard count labels are centered by visible glyph ink.** The
  15 × 15 original red badge measures glyph alpha bounds, bearings and kerning
  instead of assuming a fixed ascent offset. Raw glyphs scan by row and RLE
  glyphs by run; neither does an ARM software division for every visible pixel.
  Other health labels retain their prior layout. No glyph cache or resident
  pixel buffer was added.
- **Save & Quit performs a real transition.** A successful save starts the
  original 800 ms simple fade out; the main menu is entered at black, followed
  by its existing 800 ms fade in. Outgoing game input and gameplay clocks stop
  during this transition. Save failure keeps the pause menu and current run.
  The outgoing shade combines the source threshold with the source QuadIn
  simple-transition alpha. The existing departure state stores the new phase.
- **A finished run retires its own checkpoint before result buttons.** On the
  first terminal presentation binding, progress/history are recorded and
  persisted, then the associated current-run save is removed. Repeated frames
  do not repeat this I/O or append duplicate outcomes. A real deletion failure
  remains retryable from Main Menu. Save identity and association checks still
  preserve another run's checkpoint. Removing the checkpoint does not remove
  run history or profile progress.
- **Osty uses the source's final position and exposes Die for You.** His
  position now follows the immediate `OstyScaleToSize` placement after spawn,
  with the owner's center, half of the owner's hitbox width, and the source
  HP-dependent offset. The old implementation retained an earlier temporary
  formation position. The already shipped original power icon and localized
  title/description appear on Osty's status strip and body hover, including
  his retained dead body.

All changes use existing artwork. The card-motion record remains **7,888
bytes**, below its existing 8 KiB bound. There is no new canonical card field,
Osty power counter, save revision, decoded-image pool or second saved State.

## Original behavior checked

The following method identities were inspected in the supplied assembly;
private assembly bytes and IL dumps are not source deliverables.

| Question | Pinned evidence | Consequence |
|---|---|---|
| When is a lost run deleted? | `RunManager.OnEnded`, token `0x06000c67`, updates map history, produces the save/history entry and progress, then calls `SaveManager.DeleteCurrentRun` for solo play. The call occurs before result-screen confirmation. `SaveManager.DeleteCurrentRun` is `0x0600073d`. | Deletion belongs to terminal handling, with history retained; waiting for Main Menu was incorrect. |
| Does returning to the menu fade? | `NGame.ReturnToMainMenu` asynchronous body `0x0600b5b3` awaits a 0.8 s `FadeOut` using the original fade material before cleanup/load. | Save & Quit now performs the outgoing fade rather than switching scenes immediately. |
| Which Osty placement survives spawn? | `NCombatRoom.PositionLocalPlayerOsty` (`0x06007ce4`) supplies the earlier formation position; `AddCreature` immediately calls `NCreature.OstyScaleToSize` (`0x06008981`). Its target is owner position plus `GetOstyOffsetFromPlayer` (`0x06008982`). `Osty.MinOffset`/`MaxOffset` (`0x060030c4`/`0x060030c5`) are `(150,-75)` / `(250,-75)` source units. | Use the final owner-relative placement, including half the player's hitbox and clamped max-HP interpolation. |
| Was Die for You also missing mechanically? | `DieForYouPower` methods `0x06002531`–`0x06002538` redirect unblocked powered attack damage to a living Osty and retain his body/power after death. Existing Necrobinder core and completion regressions already implement damage absorption, overflow, death and revival. | Add the missing visible power and tooltip without inventing a second mitigation rule or changing damage semantics. |

## Executed host verification

All commands used the frozen local source SHA recorded by `tools/run_logged.py`.
A public source push was rejected by automatic approval review in this session;
these are **local** checkpoint receipts, not remote publication or CI receipts.
The first CTest launch encountered the known workspace executable-bit transport
issue. The successful rerun launched the unchanged ELF files through their
existing system interpreter.

| Check | Successful result / receipt |
|---|---|
| Seven targeted regressions | `ui19-combat-functional-pass-b9b48f534666` at `f0987e2`: world UI 8,279; frontend 37,035; card movement 1,111; layout 2,794; departure 1,256,703; Necrobinder 19,433 over 174 canonical variants; combat UI12 1,236 checks. |
| Actual original ENG42 world UI | 34,971 checks; `ui19-combat-actual-world-6066583231bd`. Includes original-resource Cosmic return flights and Osty living/dead power hovers. |
| Actual original ENG42 frontend | 70,389 checks; `ui19-combat-actual-frontend-213e1b7b325d`. Includes successful/failing Save & Quit, both transition boundaries, deletion at the initial terminal screen, retry, protected unrelated saves and history retention. |
| Actual font rendering after final badge optimization | 58,941 checks; `ui19-badge-actual-fonts-745f3b7ff342` at `bd8d228`. Independently measures rendered alpha-ink bounding boxes for `1`, `9`, `10`, `19`, `99`, `100`, `599` in both normal/authored label faces; both axes are centered within half a pixel. |
| Controlled device-save export | Four valid native combat fixtures and matching NSH7 progress; 148 checks including production codec roundtrip. Cosmic before-play/pending and living/dead Osty fixtures were supplied privately to the Firebird verifier. They are controlled fixtures, not natural-run evidence. |

The new integration tests compare complete canonical save bytes against a
separate direct-core execution. Both normal/fast settings exercise Headbutt and
Cosmic through a one-card shortcut, an explicit multi-card selection and a
saved pending selection. They observe the actual selected sprite's origin,
Draw destination, flight and delayed counter arrival. Generated-to-Draw tests
exercise three distinct commands; Seance checks same-zone holder cleanup.

Two failed actual-font attempts used the old harness's legacy single-common
root (first a wrong file argument, then the current multipart manifest). The
harness now accepts the complete private `.nsp` file as an alternative; the
successful result above actually renders and checks the original font masks.
The preliminary frontend failure tested an already expired notice after the
whole intro; the final test checks the failure notice at its onset and then
checks the persistent retry behavior separately.

## Remaining boundaries

The UI18 delivery record supersedes older missing-feature lists: monster
command programs, Vantom's root transition/tail, Smith effects, healing and
upgrade notifications, and secondary card trails were already implemented.
They were not counted again as UI19 work. This stage additionally closes the
pending-selection continuation and general generated-to-pile observations
found during the actual feedback audit.

The remaining recorded boundaries are RGB565/LDR CPU rendering and sampled
GPU materials, bounded cosmetic queues, offline service behavior, the UTC/RTC
adapter without a desktop timezone database, 64 UTF-8 seed bytes without a new
calculator IME, and finite 96-floor/32-run history/journal/drawing capacity.
They are platform differences, not a claim of complete Godot equivalence or
blanket user-approved exceptions. The agreed static full Ancient scene and
excluded audio, multiplayer and Timeline remain as previously specified.
