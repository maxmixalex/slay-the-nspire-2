# Attribution and content boundaries

## Project source

nSpire II experimental native runtime, version 0.2 (2026-10-03).
SPDX-License-Identifier: GPL-3.0-or-later.

This project uses the offline-content / native-runtime approach explored by
zyf722's nZATO. The Ndless clock register setup in
`platform/ndless/main.cpp` follows nZATO's modern backend and retains GPL
compatibility:

https://github.com/zyf722/nZATO/blob/86a4785560e18d8230e4e7f9f9e18d9be7adc96e/cpp/platform/ndless/monotonic_clock.cpp

## Platform references

Ndless r2022's `zehn.h`, `genzehn.cpp`, `syscall-list.h`, `keys.h`,
`lcd_blit.cpp` and touchpad declarations were consulted for the Zehn loader,
stable syscall ABI, LCD function-pointer API and key protocol.
This distribution does not bundle the Ndless SDK or a Firebird binary.
The freestanding launcher still requires emulator execution validation.

https://github.com/ndless-nspire/Ndless/tree/r2022
https://github.com/nspire-emus/firebird

## Game data and animations

Slay the Spire 2, its text, visual assets, characters, logos and trademarks belong
to Mega Crit and the respective rights holders. This unofficial prototype is
not endorsed by or associated with Mega Crit. GPL licensing of this project's
code grants no additional right to redistribute original game content.

The private content pack and offline HTML are transformed from the installation
provided for this project. Public source archives exclude original game data,
assemblies, PCKs, ROMs, flash dumps, screenshots and font files.

`tools/animation.py` and `tools/setup_pose.py` implement a bounded converter for
the supplied Spine 4.2 data. Selected bone, slot, deform and draw-order tracks
are sampled offline, with limited IK and transform-constraint handling.
They do not bundle the Spine editor or a complete official runtime and do not
claim exact rendering parity. The official binary reader was consulted as a
format reference:

https://github.com/EsotericSoftware/spine-runtimes/tree/4.2

Only pre-rendered images and their animation-directory records are read on the
calculator. Some room layout, encounter substitutions, finite card pools,
selection highlights and presentation effects are specific to this prototype.

Fonts are read locally by the converter. No installable font file is included;
only rasterized glyphs needed by the selected interface enter the content pack.
