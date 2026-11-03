# Slay the Nspire 2

[![CI](https://github.com/maxmixalex/slay-the-nspire-2/actions/workflows/ci.yml/badge.svg)](https://github.com/maxmixalex/slay-the-nspire-2/actions/workflows/ci.yml)
[![License: GPL-3.0-or-later](https://img.shields.io/badge/license-GPL--3.0--or--later-blue.svg)](COPYING)

**English** | [简体中文](README.zh-CN.md)

An unofficial port of **Slay the Spire 2** to TI-Nspire calculators running
Ndless. Build a deck, choose your route and climb the Spire on a 320×240 screen.

<p align="center">
  <img src="docs/media/gameplay.gif" alt="Necrobinder and Osty in combat" width="320">
</p>

*Necrobinder and Osty in combat. [Recorded in Firebird](docs/media/README.md).*

## Features

- The three-act single-player campaign, with five characters to unlock.
- Battles, events, shops, rest sites and the original regional maps.
- English and Simplified Chinese, with optional support for the other game languages.
- Full animation and a smaller edition with reduced actor animation.
- Three save profiles, saved runs, a card library and run history.
- Touchpad and keyboard controls, with remappable keys.

Audio and multiplayer are not implemented.

## Quick start

You need a color-screen TI-Nspire with [Ndless](https://github.com/ndless-nspire/Ndless)
and your own supported Slay the Spire 2 installation. See the
[installation guide](docs/installation.md) for the game version and device setup.

### Download

Download the artifacts from a successful GitHub Actions run:

| Download | What to extract |
| --- | --- |
| [Calculator launcher](https://github.com/maxmixalex/slay-the-nspire-2/actions/workflows/ci.yml) | `nSpire2.tns` |
| [Windows converter](https://github.com/maxmixalex/slay-the-nspire-2/actions/workflows/converters.yml) | `nspire2-convert.exe` |
| [Linux converter](https://github.com/maxmixalex/slay-the-nspire-2/actions/workflows/converters.yml) | `nspire2-convert` |

The converter includes its dependencies. Original game data is created locally
from your game installation.

### Convert the game

Put `nspire2-convert.exe` in the original game directory and double-click it.
The default conversion creates English and Simplified Chinese data in
`nSpire2-converted/data/`.

For reduced animation, run `nspire2-convert.exe --no-animations`. Use
`--languages all` to include every language. Conversion can take a while;
running the same command again resumes completed stages.

The [converter guide](docs/converter.md) covers other folders, Linux and building
the converter from source.

### Transfer and play

Copy the launcher and the **entire** generated `data/` folder into one directory
on the calculator:

```text
Slay the Nspire 2/
  nSpire2.tns
  data/
```

Open `nSpire2.tns` to play. The game creates a `state/` folder for saves and
settings. Back up and keep that folder when updating; replace the launcher and
data together. Choose your language in Settings.

## Controls

| Input | Action |
| --- | --- |
| Touchpad movement / press | Move the pointer / click |
| Direction keys | Navigate menus |
| `1`–`9`, `0` | Select a card in your hand |
| `Enter` | Confirm or aim |
| `Esc` | Back or pause |
| `Menu` | End turn |
| `Tab` | Map |
| `Doc` | Deck |

Settings includes remappable keys, Tap to Click and pointer speed.
Tap to Click is off by default.

## Development

Build instructions, tests and the project layout are in
[DEVELOPMENT.md](DEVELOPMENT.md).

## License and thanks

Project source is [GPL-3.0-or-later](COPYING). See [NOTICE.md](NOTICE.md) for
third-party credits. Slay the Spire 2 and its assets belong to Mega Crit and
their respective owners. This project is unofficial and includes no original
game data.

Thanks to [Ndless](https://github.com/ndless-nspire/Ndless),
[Firebird](https://github.com/nspire-emus/firebird) and
[nZATO](https://github.com/zyf722/nZATO).
