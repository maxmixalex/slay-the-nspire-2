# Current work

Repository: `maxmixalex/slay-the-nspire-2`.
Read [AGENTS.md](../AGENTS.md) for checkpoint and device constraints, and
[DEVELOPMENT.md](../DEVELOPMENT.md) for build commands.

## Active player feedback

The current work responds to the user's requested touchpad/settings repairs,
removal of the Timeline interface and its gameplay gates, independent character
unlocking, the new-profile tutorial, and a simpler public project presentation.
The earlier UI25 scope is a baseline, not a substitute for these requests.

The input and campaign changes need their own ordinary-device checks. Verify
physical touchpad clicks, touch movement, direction-key/center transitions,
Tap to Click, pointer speed, profile persistence, new-run tutorial progression,
character unlocking, save/relaunch and return to the OS. Record the actual
remaining failures; a prior successful route did not establish the root cause
of the older touchpad exit failure.

README and installation guides now focus on playing the game. Reusable resource
contracts and porting knowledge are in `resource-format.md` and `porting.md`.
Historical stage reports and generated receipts have been removed from the
working tree; their originals remain in Git history. Do not restore them as a
second archive or grow this file into a transcript.

## Baseline and constraints

The original game reference remains **v0.111.0 / 41cef1ea**. Preserve the
five-character/card/potion foundation, world generation and sampled animation
while changing the requested progression and input behavior.

The [previous release record](https://github.com/maxmixalex/slay-the-nspire-2/blob/2c5144fe69a0a6fb76711866eec8e367a8f08dc8/docs/ui25-release-validation.md)
retains the old runtime/resource identities, package measurements and scoped
Firebird results. Those results do not validate a newer build. Cold startup and
some transitions stalled in that baseline, and physical handheld performance
was not measured. Corrected conversion tails were tested; a new corrected
empty-work conversion and all-language warm-run measurements remained open.

The formal allocation and installed-size budgets remain in DEVELOPMENT. Keep
the ordinary 64 MiB Firebird/OS/Ndless path. Do not enlarge arenas or alter
emulator memory/clock/snapshot headers to make a test pass. Manual WASM/browser
validation still requires a renewed request.

Original game data, firmware, private captures, converted packs and embedded
HTML remain private. Audio, multiplayer, hosted services, desktop timezone/DST
data and calculator IME remain unimplemented.

## Next checkpoint

Replace this section with the integrated source checkpoint, concise test
results, outstanding issues and the next action after the current repairs are
verified. New GIFs should show the final ordinary input paths: `controls.gif`
and `journey.gif` in `docs/media/`. Add them to the README only after recording
and inspecting the actual files; the existing `gameplay.gif` is an older
Firebird capture with its provenance in `media/README.md`.
