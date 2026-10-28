# UI12 original startup logo and skip preference

This closes the previously explicit missing startup-logo feature. The source
contract is `NGame.LaunchMainMenu`, `NLogoAnimation._Ready/PlayAnimation`,
`NMainMenu._Ready`, `NIntroLogoTickbox` and `SettingsSave.SkipIntroLogo`, pinned
to v0.111.0 / `41cef1ea`. Audio remains outside the port's agreed scope.

## Implemented behavior

Interactive hosts explicitly begin the sequence after resource/preferences
initialization. On Ndless this is after release of the Enter key that opened
the document. The first-launch guard survives `App::init`, locale replacement,
presentation reset and returning from a run. Ordinary initialization in a
native test or a headless fixture does not synthesize a new host launch.

| Source stage | Time from launch |
| --- | --- |
| Initial fade and original wait | 0–1,800 ms |
| Original nonlooping Spine `animation` | 1,800–4,800 ms |
| Background Cubic.Out to `#074254` | 4,800–6,800 ms |
| Original final hold | 6,800–7,800 ms |
| Fade out | 7,800–8,600 ms |
| Main-menu Cubic.In fade | 8,600–10,600 ms |

The logo uses the original setup-bounds scaling and centering, with the
800-source-pixel Back.Out entrance projected to 152 LCD pixels and a 500 ms
linear alpha entrance. Select, Cancel and pressed mouse buttons skip at the
source cancellation points. A cancel during the initial one-second wait
finishes that wait; during the Spine track it freezes the entrance tween and
fades its container with 250 ms Expo.Out. The source final background tween
and hold are not interrupted. Held skip keys remain captured until release,
so they cannot activate Continue when the menu appears.

General Settings now includes the original Skip Intro Logo checkbox after
Text Effects. It defaults to off; resetting Settings restores that default.
NSPF6 uses bit 11 without growing the 108-byte record, masks the independent
shake value correctly, preserves custom keys, reads old NSPF1–5 with the new
option off and rejects assigning the new bit to an old schema. Toggling the
setting affects the next process launch. Its header, description and two
toasts use the original sixteen language tables.

## Bounded resource/runtime changes

The optional UI40 extension contains exactly 37 sampled source Spine images
at 12 Hz, including the final pose at 3 seconds. It keeps the existing 1,783
animation clips, gameplay IDs and version 40. The 160×160 transparent canvas
preserves the complete animated ink and splashes; the final logo occupies
approximately 94×79 LCD pixels. Original Spine input files and generated
pixels remain private.

The converter writes one common file and sixteen NSL2 language files, with
no complete logical locale files written to disk. Every existing image/tile
payload and clip clock is copied. The new shared artwork adds **165,511
bytes**. The final common file is **61,666,883 bytes**; common + English +
Simplified Chinese resources total **67,913,133 bytes**, before the launcher
and saves. Traditional Chinese needs two added glyphs; Thai needs ten shaped
graphemes in four font bands, forty glyphs in total. Other editions require
no added glyphs. Existing locale glyphs and string IDs remain intact.

The presentation state is 16 bytes. Drawing uses the existing image cache and
LCD buffer, with no new pixel buffer, resident Spine runtime, full-file scan
or gameplay RNG use. Old UI40 directories without the optional catalog still
open and proceed directly to the menu. The boot language probe now accepts
the current UI40 marker; its direct/ranged metadata reads remain 52 bytes.

## Executed verification and boundaries

Runtime `1310a5e` plus preferences `ef3e13c` and pure-black fade correction
`5eed72c` were checked against the actual generated UI40 resources.

| Gate | Result | Local journal suffix |
| --- | --- | --- |
| Complete matching native core/startup/boot build | Passed | `44c6a97d026e` |
| Independent source-clock oracle and bounded boot probe | 2/2 CTest targets passed | `a0799d759d03` |
| Actual startup, skips, host lifetime and sixteen locales | 12,531 checks passed | `7e2bc39e7cf3` |
| Source Spine sampling and all sixteen incremental packages | Passed | `3c833edb5241` |

Every listed journal closed with `source_changed=false`. The actual gate
renders all 37 frames, checks the final background and both transitions,
retained Enter presses, pointer skips, persisted skipping, no replay on
language reinitialization, all four locale keys and their required font bands.
Canonical gameplay State remains byte-identical throughout. Native captures
of the animated logo, final source background and menu were visually reviewed.

The first converter attempt used the wrong adapter metadata field and stopped
before writing any output pack; the second lacked the already installed
private `regex` dependency. Both failures are retained. The first actual
rendering gate caught a real dark-blue-versus-black fade error, which was
fixed in runtime rather than weakening its pixel assertion.

The source skeletal geometry is sampled at the documented 12 Hz LCD cadence;
runtime GPU material/raster equality with Godot is not asserted. Transition
coverage is a bounded CPU black veil. Native checks establish behavior and
resource correctness, not device latency. Ordinary Firebird startup, setting
toggle/restart and locale switching remain release-owner integration gates
against the final launcher and these matching resource files.
