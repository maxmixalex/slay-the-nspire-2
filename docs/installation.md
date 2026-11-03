# Installation and updates

**English** | [简体中文](installation.zh-CN.md) · [Back to README](../README.md)

This guide covers the UI25 calculator installation. It uses the original
**v0.111.0 / 41cef1ea** game files and a working Ndless installation. The
[release record](ui25-release-validation.md) identifies the tested launcher,
resources and Firebird environment. UI25 is the delivery stage; the in-game
version label may still read `0.1.0-ui24`.

## Get the program files

Download programs from successful GitHub Actions runs for the intended source
checkpoint, using the run's **Artifacts** section:

| Required program | Workflow | Artifact / contained file |
| --- | --- | --- |
| Calculator launcher | [Native tests and portable builds](https://github.com/maxmixalex/slay-the-nspire-2/actions/workflows/ci.yml) | `nspire2-launcher-<commit>` / `nSpire2.tns` |
| Windows converter | [Windows original-install converter](https://github.com/maxmixalex/slay-the-nspire-2/actions/workflows/converter-windows.yml) | `nspire2-converter-windows-<commit>` / `nspire2-convert.exe` |

The launcher artifact contains program and license files; it does not contain
`data/`. The converter runs on your computer and creates `data/` from your own
original installation. Keep the supplied dependency notices with the programs.
For a Linux converter or an unavailable workflow artifact, use the
[source build instructions](../DEVELOPMENT.md).

The public repository has no complete game-data download. Do not mistake a
source ZIP, a converter executable or a launcher-only artifact for a complete
calculator installation.

## Create your data

The original directory must contain `SlayTheSpire2.pck`, `release_info.json`,
and `sts2.dll` either directly or under its `data_sts2_*` directory. The converter
reads these files without launching the original game.

Put `nspire2-convert.exe` in that directory. Windows users can double-click it
for the full-animation English/Simplified-Chinese edition. In PowerShell,
choose **one** of these commands for the desired output:

```powershell
.\nspire2-convert.exe
.\nspire2-convert.exe --no-animations
.\nspire2-convert.exe --languages all --output .\nSpire2-converted-all-languages
.\nspire2-convert.exe --no-animations --languages all --output .\nSpire2-converted-no-animations-all-languages
```

For a Linux standalone build, first run `chmod +x ./nspire2-convert`, then
use `./nspire2-convert` with the same options. Versioned downloads may have a
different filename; substitute the filename you actually received.

| Options | Output directory, unless overridden |
| --- | --- |
| No options | `nSpire2-converted` under the original game directory |
| `--no-animations` | `nSpire2-converted-no-animations` under the original game directory |
| `--languages all` | All sixteen locale files in the selected output |
| `--languages eng,zhs,jpn` | Only the requested locales; English must be included |
| `--game "PATH"` / `--output "PATH"` | Explicit original-game / output directory |
| `--work "PATH"` / `--workers 1` | Dedicated working directory / reduced worker concurrency |

Conversion may take a substantial amount of time and temporary space. Re-run
the same command to resume verified completed stages. If producing both
editions, run them sequentially and keep separate output directories. The
[converter guide](converter-ui23.md#progress-and-resuming) explains cache reuse,
progress records and cleanup. The converter produces `data/` and
`converter-receipt.json`; the launcher is obtained separately.

## Transfer to the calculator

Choose a game directory in the calculator's document storage. Place the
launcher at its root and preserve the complete generated `data/` directory.
Do not flatten its files into the root.

| Relative path | Contents |
| --- | --- |
| `nSpire2.tns` | Ndless launcher |
| `data/nSpire2.dat.tns` | Index for all eight artwork parts |
| `data/nSpire2.interface.dat.tns` | Interface artwork |
| `data/nSpire2.cards.dat.tns` | Card artwork |
| `data/nSpire2.characters.dat.tns` | Character artwork and animation |
| `data/nSpire2.enemies.dat.tns` | Enemy artwork and animation |
| `data/nSpire2.bosses.dat.tns` | Boss artwork and animation |
| `data/nSpire2.rooms.dat.tns` | Rooms and their effects |
| `data/nSpire2.effects.dat.tns` | Shared visual effects |
| `data/nSpire2.shared.dat.tns` | Other shared artwork |
| `data/nSpire2.eng.dat.tns` | English text and glyphs; keep for fallback |
| `data/nSpire2.zhs.dat.tns` | Simplified Chinese text and glyphs |
| `data/nSpire2.<locale>.dat.tns` | Optional locale files from the same conversion |
| `state/` | Created by the game for every profile, save, preference and backup |

**All eight artwork parts are required in either animation mode and for every
language.** Keep the index, parts and locale files from one matching conversion.
Open `nSpire2.tns` to play, then choose an installed language in Settings.
The launcher uses a saved usable language first, followed by a supported system
language, English, then another installed usable locale.

The recorded UI25 full bilingual package extracts to **69,839,495 bytes** and
the reduced package to **29,811,189 bytes**, including instructions and licenses.
Saves and additional locales need more space. A transfer can also need temporary
free space if the OS keeps an old file while receiving its replacement.

## Optional languages

Generate the desired languages together with the rest of your data. Version
and animation labels alone do not guarantee that two conversions have matching
resource addresses. Never combine locale files from an independent conversion
with another installation's common data.

| Code | Language | Code | Language |
| --- | --- | --- | --- |
| `eng` | English | `zhs` | Simplified Chinese |
| `zht` | Traditional Chinese | `deu` | German |
| `esp` | Spanish — Latin America | `fra` | French |
| `ind` | Indonesian | `ita` | Italian |
| `jpn` | Japanese | `kor` | Korean |
| `pol` | Polish | `ptb` | Portuguese — Brazil |
| `rus` | Russian | `spa` | Spanish — Spain |
| `tha` | Thai | `tur` | Turkish |

If you already have the three matching UI25 installation archives, choose only
one main archive. The optional archive is an add-on for those exact main data
sets; it is not a standalone installation:

| Main package filename suffix | Optional locale folder |
| --- | --- |
| `full-animated-bilingual.zip` | `animated/data/` |
| `no-character-animation-bilingual.zip` | `no-animations/data/` |

Copy only the wanted `nSpire2.<locale>.dat.tns` files from the matching folder
into the installed `data/`. English and Simplified Chinese are already included
in both main archives. The archive prefix is `Slay-the-Nspire-2-0.1.0-ui25-`.
These names document the verified packages, not public game-data download links.

Original text uses the selected locale with the original English fallbacks.
Port-authored history/archive errors and Timeline status labels currently use
English or Simplified Chinese; other locales show those labels in English.

## Update and preserve saves

1. Exit the game normally and back up the **entire `state/` directory** to your computer.
2. Keep the existing `state/` on the calculator. Do not replace it with an archive's empty folder.
3. Remove the old `data/`, then transfer the matching new launcher and complete new `data/`.
4. Replace any optional locales with the new matching files before launching.
5. Open the game, select the intended profile and use Continue.

The three profiles share the same installation but have separate progress and
runs. First-profile files include `nSpire2.sav.tns`, `nSpire2.progress.tns` and
`nSpire2.history.tns` under `state/`. Profiles two and three insert `.p2` or `.p3`
before `.tns`, such as `nSpire2.sav.p2.tns`. Keeping the whole directory also
preserves settings, backups and temporary recovery files.

UI25 reads v3–v17 run saves and NSHA/earlier progress, and writes v18/NSHB.
Older launchers may not read newly written files. To roll back, restore your
pre-upgrade `state/` backup together with its matching old launcher and data.
Earlier unpublished directory layouts are not imported automatically.

## Timeline and profiles

Use the top-left title control to choose one of three profiles. Open Timeline
from the title menu; Enter inspects the selected node, and a second Enter reveals
an obtained node. New profiles follow the unlock chain. Older profiles preserve
explicit compatibility access without invented obtain dates or reveal history.
The calculator presents a chronological list with scrolling details; original
Early Access placeholders remain unavailable.

## Long ASCII seeds

The calculator's direct seed field holds 64 UTF-8 bytes. For a longer ASCII
seed, save the complete text in `seed.txt` and run:

```powershell
.\nspire2-convert.exe --encode-seed-file .\seed.txt
```

On Linux, use `./nspire2-convert --encode-seed-file seed.txt`. You can also
pass `--encode-seed "complete ASCII seed"`. Encoding needs no game-data conversion.

In Custom Run, open the seed field, use Up/Down to select **Import seed code**,
then enter the printed `SEED64-` code. Do not enter the code as ordinary seed
text. The [seed reference](seed-ascii-ui24.md) describes exact identity handling
and the explicit rejection of non-ASCII encoder input.

## If something goes wrong

| Symptom | Check |
| --- | --- |
| Ndless does not launch the program | Confirm Ndless works with the installed OS; review the recorded model/OS scope before assuming support. |
| Missing or invalid resource error | Re-copy the complete `data/`, including its index, all eight parts and English locale. Check that the files came from one conversion. |
| A language is unavailable or fails to load | Use that language's file from the same conversion and animation family, then select it in Settings. |
| Transfer reports insufficient space | Back up `state/`, exit, remove the old `data/`, and finish transferring the complete replacement before playing. |
| Converter was interrupted | Repeat the same command and work location; failed or incomplete stages are not counted as finished. |

When reporting a problem, include the program source identity or `BUILD-INFO`,
calculator/OS/Ndless versions, animation edition, language and steps to reproduce.
Do not attach original game files, firmware, resource packs or private save data
to a public issue. The [release evidence](ui25-release-validation.md) is the
reference for what has actually been tested.
