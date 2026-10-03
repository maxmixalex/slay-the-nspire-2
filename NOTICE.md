# Attribution and content boundaries

## Project source

nSpire II experimental native runtime, version 0.6 (2026-10-04).
SPDX-License-Identifier: GPL-3.0-or-later.

This project uses the offline-content / native-runtime approach explored by
zyf722's nZATO. The original clock adapter followed nZATO's modern backend. Version 0.3
uses a foreground SP804 timer instead of resetting the OS timer. The
project retains its existing GPL compatibility and attribution:

https://github.com/zyf722/nZATO/blob/86a4785560e18d8230e4e7f9f9e18d9be7adc96e/cpp/platform/ndless/monotonic_clock.cpp

## Platform references

Ndless r2022's `zehn.h`, `genzehn.cpp`, `syscall-list.h`, `keys.h`,
`lcd_blit.cpp` and touchpad declarations were consulted for the Zehn loader,
stable syscall ABI, LCD function-pointer API and key protocol.
This distribution does not bundle the Ndless SDK or a Firebird binary.
The 0.3 launcher has been exercised in Firebird on CX II CAS OS 6.4.0.74
with Ndless r2022. Physical-device validation remains outstanding.

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

## Software math added in 0.6

`src/reference_math.cpp` adapts Go runtime/softfloat64.go and math/{log,sqrt,sin}.go.
Copyright 2009-2011 The Go Authors. All rights reserved.
The BSD-3-Clause license is preserved in `licenses/Go-BSD-3-Clause.txt` and must
also accompany binary redistributions. The C++ adaptation uses explicit-width
arithmetic, normal/subnormal rounding and restricted map-quota domains.

The logarithm and square-root algorithms also retain this notice:
Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
Developed at SunPro, a Sun Microsystems, Inc. business.
Permission to use, copy, modify, and distribute this software is freely
granted, provided that this notice is preserved.

Sine coefficients: Cephes Math Library Release 2.8, June 2000.
Copyright 1984, 1987, 1989, 1992, 2000 by Stephen L. Moshier.
May be used freely, without support or guarantee.

Reference implementations: https://go.dev/src/runtime/softfloat64.go and
https://go.dev/src/math/ . These notices do not grant rights in the original
game or imply exact last-bit equivalence to its Windows math library.
