# Attribution and content boundaries

## Project source

nSpire II experimental native runtime, version 0.17 development (2026-10-23).
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
Versioned validation documents record the exact source and binary identities
used for each platform result, including Firebird execution on an isolated copy
of the supplied CX II CAS working image with Ndless r2022. Compilation,
emulator execution and physical-device measurements are separate evidence;
one must not be presented as another.

https://github.com/ndless-nspire/Ndless/tree/r2022
https://github.com/nspire-emus/firebird

## Bounded resource decompression

The seven unchanged decoder source/header files under `third_party/lzma2` come
from Igor Pavlov's official LZMA SDK in 7-Zip 26.04, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`. The SDK is in the public domain.
Verified original file hashes and the narrow decoder integration are documented
in that directory. The runtime uses neither the archive parser nor SDK heap or
filesystem support.

https://github.com/ip7z/7zip/tree/9128b80e3a471108678e2b3ec3c985861c8d7b0f/C
https://www.7-zip.org/sdk.html

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

Game fonts are read locally by the converter. No installable font file is
included; only rasterized glyphs needed by the selected interface enter the
content pack.

The small Chinese bitmap subset used before a content pack is available is
derived from Noto Sans CJK SC, independently of the original game fonts.
Copyright 2014-2021 Adobe and 2010-2012 Google Corporation. This boot subset
retains the SIL Open Font License 1.1 in `licenses/Noto-Boot-OFL.txt`, which
must accompany binary redistributions and is embedded in the offline HTML.

https://github.com/notofonts/noto-cjk

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

## Interface/content conversion added in 0.8

Original scene layouts, menu/selection artwork, UI textures and material controls
are inspected only from the owner's pinned installation. Private outputs include
converted scene stills, card-banner material colors and a bounded sampled campfire
layer. These are presentation adaptations, not the complete Godot renderer.
The independent LZ4 block encoder/decoder and glyph RLE retain image/coverage
values losslessly; no upstream LZ4 implementation source is bundled.
No original scene, shader, atlas or installable font is included in public source.

## Silent content and product scope added in 0.9

The added Silent portraits, character-selection still, card/power/relic/energy
textures, colored frame materials and six sampled clips are converted from the
owner's pinned local game installation. Existing payloads remain intact. No
audio, multiplayer or Timeline progression implementation is included; these
three areas are excluded by the requested product scope. Reading/skipping binary
skeleton event metadata does not provide audio playback.

## Shared content added in 0.10

Seven additional colorless portraits, eight relic icons and a No Block power
icon are converted from the same pinned local installation. Previously converted
image payloads and all seventeen clip records are retained. Upgrade comparison,
permanent deck choices and paged item details reuse the original-derived UI
textures already present in the private pack. No original game code or assets
are included in the public source.

## Character tactics and status UI added in 0.11

Ten original character card portraits and seven power icons are converted from
the same pinned local installation. Existing decoded images, glyph coverage and
clip timing are retained. Host-side python-lz4 performs high-compression encoding;
the calculator keeps this project's independent LZ4 block decoder. No python-lz4
or upstream LZ4 source/binary is bundled in the runtime. Public source excludes
all original assets, private disassembly, calculator images and font files.

## Relic combinations and treasure UI added in 0.12

Five original relic icons are converted from the same pinned user installation.
All prior encoded image payloads, glyph coverage and clip records are retained.
Treasure presentation adapts inspected original room/relic-holder scene structure,
using existing original-derived panels and banners. Prototype acquisition odds,
prices and the exhausted-pool fallback are not original-game parity claims.
No original assembly bodies, assets, font files or calculator images are published.

## Original rich-text color math added in 0.17

`include/rich_rainbow.hpp` adapts the rainbow phase calculation from Godot
4.5.1 and samples Björn Ottosson's OkHSV color mathematics. The fixed-point
adaptation and numerical table retain the MIT notices in that header and in
`licenses/OkHSV-Godot-MIT.txt`; the latter must accompany binary redistributions.
The offline HTML packager includes it with the existing notices.
The pinned public sources, table reconstruction procedure and measured error
limits are documented in `docs/rich-rainbow-0.17.md`. These mathematical tables
contain no original game image, font or localization payload.
