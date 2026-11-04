# Slay the Nspire 2

[![CI](https://img.shields.io/github/actions/workflow/status/maxmixalex/slay-the-nspire-2/ci.yml?branch=main&label=CI&logo=githubactions&logoColor=white)](https://github.com/maxmixalex/slay-the-nspire-2/actions/workflows/ci.yml)
[![License](https://img.shields.io/badge/license-GPL--3.0--or--later-blue.svg?logo=gnu&logoColor=white)](LICENSE)
![Codex vibe coding](https://img.shields.io/badge/vibe_coded_with-Codex-412991)

**English** | [简体中文](README.zh-CN.md)

An unofficial port of *Slay the Spire 2* to the TI-Nspire Ndless platform. Choose a character, accept an Ancient’s gift, and build a deck as you climb the Spire. A desktop converter prepares the artwork and text from your own copy of the PC game. [Slay the Spire 2 on Steam](https://store.steampowered.com/app/2868840/Slay_the_Spire_2/)

<table width="100%" align="center">
  <tr>
    <td width="33%" align="center" valign="center">
      <strong>Main menu and animated title</strong>
    </td>
    <td width="33%" align="center" valign="center">
      <strong>Characters and unlocks</strong>
    </td>
    <td width="33%" align="center" valign="center">
      <strong>Battle with Osty</strong>
    </td>
  </tr>
  <tr>
    <td width="33%" align="center" valign="center">
      <img src="assets/screenshots/menu.gif" alt="Main menu and animated title" width="100%">
    </td>
    <td width="33%" align="center" valign="center">
      <img src="assets/screenshots/characters.gif" alt="Characters and unlocks" width="100%">
    </td>
    <td width="33%" align="center" valign="center">
      <img src="assets/screenshots/battle.gif" alt="Battle with Osty" width="100%">
    </td>
  </tr>
  <tr>
    <td width="33%" align="center" valign="center">
      <strong>Compendium</strong>
    </td>
    <td width="33%" align="center" valign="center">
      <strong>The merchant</strong>
    </td>
    <td width="33%" align="center" valign="center">
      <strong>Events and choices</strong>
    </td>
  </tr>
  <tr>
    <td width="33%" align="center" valign="center">
      <img src="assets/screenshots/compendium.gif" alt="Compendium" width="100%">
    </td>
    <td width="33%" align="center" valign="center">
      <img src="assets/screenshots/shop.gif" alt="The merchant" width="100%">
    </td>
    <td width="33%" align="center" valign="center">
      <img src="assets/screenshots/event.gif" alt="Events and choices" width="100%">
    </td>
  </tr>
  <tr>
    <td width="33%" align="center" valign="center">
      <strong>An Ancient’s gift and the map</strong>
    </td>
    <td width="33%" align="center" valign="center">
      <strong>Developer console</strong>
    </td>
    <td width="33%" align="center" valign="center">
      <strong>Sixteen languages</strong>
    </td>
  </tr>
  <tr>
    <td width="33%" align="center" valign="center">
      <img src="assets/screenshots/ancient.gif" alt="An Ancient’s gift and the map" width="100%">
    </td>
    <td width="33%" align="center" valign="center">
      <img src="assets/screenshots/console.gif" alt="Developer console" width="100%">
    </td>
    <td width="33%" align="center" valign="center">
      <img src="assets/screenshots/languages.gif" alt="Sixteen languages" width="100%">
    </td>
  </tr>
</table>

Captured from the game running in Firebird at the calculator’s native resolution. Some scenes were prepared with the in-game console. The final clip shows the language menu and battles in several languages.

## Key features

- Single-player runs with the Ironclad, Silent, Regent, Necrobinder and Defect, including cards, relics, potions, enemies, events and Ancient encounters.
- Animated characters and monsters, title-screen effects, combat effects and transitions adapted to the calculator screen.
- Full-animation and reduced-animation content packs. The reduced edition keeps the essential poses and uses the same launcher and saves.
- Touchpad and keyboard controls, card and item inspection, an interactive map, and adjustable pointer speed and tap-to-click behavior.
- A Compendium with the card library, relic collection, potion lab and bestiary, plus profiles, run history and a developer console.
- Sixteen languages from the original game. English and Simplified Chinese are included by default, and the remaining languages can be installed individually.

## Software and Hardware Support

### Supported Handhelds

The game is designed for color TI-Nspire calculators with Ndless. Development and gameplay recording use a TI-Nspire CX II CAS in Firebird with its normal memory configuration. A computer is needed to convert the original game and transfer files to the handheld. [Firebird](https://github.com/nspire-emus/firebird)

### OS and Ndless versions

The tested combination is TI OS 6.4.0.74 with Ndless r2022 and the Zehn launcher. Install the Ndless release that matches your calculator’s OS before launching the game. [Ndless](https://ndless.me/)

The current port is **nSpire 0.1.0**, developed against **StS II v0.111.0**. The converter reads the original game’s version from the installation and includes it in the content pack, so the title screen identifies the data you are using. The current release supports single-player play. Audio, multiplayer and online services are unavailable.

## Quickstart

### Download

Open a successful workflow run and download the relevant ZIP from its **Artifacts** section. The launcher and converters are built separately.

| Download | Workflow | Contents |
| --- | --- | --- |
| `nspire2-launcher` | [CI](https://github.com/maxmixalex/slay-the-nspire-2/actions/workflows/ci.yml) | `nSpire2.tns`, the program that runs on the calculator |
| `nspire-converter-windows` | [Desktop converters](https://github.com/maxmixalex/slay-the-nspire-2/actions/workflows/converters.yml) | `nspire-converter.exe` for Windows |
| `nspire-converter-linux` | [Desktop converters](https://github.com/maxmixalex/slay-the-nspire-2/actions/workflows/converters.yml) | `nspire-converter` for Linux |

The converter is packaged with Nuitka and includes its Python dependencies and native helpers. You can run it directly after extracting the archive.

If you already have a complete installation pack, continue with **Transfer to Handheld** below.

### Create Content Pack

In Steam, choose **Slay the Spire 2 → Manage → Browse local files**. Copy the converter into that directory, next to `SlayTheSpire2.pck` and `release_info.json`, then run `nspire-converter.exe` on Windows or `./nspire-converter` on Linux.

With no arguments, it creates a full-animation English and Simplified Chinese pack in `nSpire2-converted/data/`. Conversion extracts the game’s resources, samples animation, prepares fonts and compresses the result. The progress display follows each stage.

To select another edition or additional languages, use the command line.

```powershell
.\nspire-converter.exe --no-animations
.\nspire-converter.exe --languages eng,zhs,jpn
.\nspire-converter.exe --languages all
```

`--no-animations` writes a separate pack to `nSpire2-converted-no-animations/data/`. Language packs contain their own text and fonts, so you can select just the languages you want. Run `nspire-converter --help` for the complete options.

For an ASCII seed that is too long to type, use `nspire-converter --encode-seed-file seed.txt`, then choose **Import seed code** in the custom game’s seed editor.

### Transfer to Handheld

Use your usual TI file-transfer software to copy `nSpire2.tns` and the complete matching `data/` directory into the same calculator folder. Open `nSpire2.tns` from My Documents.

| File or folder | Contents |
| --- | --- |
| `nSpire2.tns` | Launcher shared by both editions |
| `data/` | Game resources, animations, selected languages and original game version |
| `state/` | Profiles, runs, history, settings and backups created during play |

The game starts in the system language when its pack is present, otherwise in English or the first installed language. You can change languages from the main menu’s **Settings** screen.

When updating, exit the game and keep the entire `state/` folder. Replace the launcher and `data/` together. To add languages from the optional ZIP, copy files from `full/data/` for the full-animation edition or `reduced/data/` for the reduced edition into your existing `data/` folder.

## Controls

| Input | Action |
| --- | --- |
| Touchpad stroke | Move the pointer |
| Touchpad center press | Click at the visible pointer, or confirm the focused control when it is hidden |
| Arrow keys | Move focus. Lift your finger and make a fresh stroke to return to the pointer |
| `1`–`9`, `0` | Select a card in your hand |
| `Enter` | Confirm a selection or target |
| `Esc` | Go back or pause |
| `Menu` | End the turn |
| `Tab` / `Doc` | Open the map / deck |
| Flag key | Open the developer console |

In **Settings**, enable **Tap to Click** to use light taps and adjust **Mouse Speed** to suit your touchpad. Key bindings are available under **Settings → Input**. These preferences are saved on the calculator.

## Development

The handheld game is written in C++, and the desktop converter uses Python with Poetry and Nuitka. Repository layout, build commands and code checks are covered in the development guide. [Development](DEVELOPMENT.md)

## License & Disclaimer

The port is distributed under GPL-3.0-or-later. Slay the Spire 2 and its assets belong to Mega Crit and their respective owners. Creating a content pack requires your own copy of the PC game. [License](LICENSE)

This project was developed with Codex and is an unofficial fan project, independent of Mega Crit.

## Acknowledgements

- Mega Crit, for Slay the Spire 2. [Official game](https://store.steampowered.com/app/2868840/Slay_the_Spire_2/)
- The Ndless contributors, for native applications on the TI-Nspire. [Ndless](https://github.com/ndless-nspire/Ndless)
- The Firebird contributors, for the TI-Nspire emulator used during development. [Firebird](https://github.com/nspire-emus/firebird)
- nZATO, for the converter-and-launcher approach and project organization. [nZATO](https://github.com/zyf722/nZATO)
- The authors of the third-party code and fonts used by the port. [Credits and license notices](cpp/third_party/notices/ATTRIBUTIONS.txt)
