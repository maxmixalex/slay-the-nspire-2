# Installation and updates

**English** | [简体中文](installation.zh-CN.md) · [README](../README.md)

If you already have `nSpire2.tns` and a complete `data/` folder, [transfer them
together](#transfer-and-launch) and keep `state/` when updating. The conversion
steps below are for creating data from your own PC game installation.

## Requirements

- A color-screen TI-Nspire with a working [Ndless](https://github.com/ndless-nspire/Ndless) installation.
- Your own Slay the Spire 2 **v0.111.0 / 41cef1ea** game files.
- A computer for conversion and transfer, with enough temporary space for the converter.

The device setup used for emulator testing is TI-Nspire CX II CAS, OS 6.4.0.74
and Ndless r2022 in Firebird. Other models and OS versions need their own testing.

## Download the programs

Open a successful workflow run and download from its **Artifacts** section:

| Program | Workflow | Artifact |
| --- | --- | --- |
| Calculator launcher | [Native tests and portable builds](https://github.com/maxmixalex/slay-the-nspire-2/actions/workflows/ci.yml) | `nspire2-launcher` |
| Windows converter | [Desktop converters](https://github.com/maxmixalex/slay-the-nspire-2/actions/workflows/converters.yml) | `nspire2-converter-windows` |
| Linux converter | [Desktop converters](https://github.com/maxmixalex/slay-the-nspire-2/actions/workflows/converters.yml) | `nspire2-converter-linux` |

Extract `nSpire2.tns` and the converter for your computer:
`nspire2-convert.exe` on Windows or `nspire2-convert` on Linux. Keep their
license notices.
The launcher needs the data produced by the converter. For an expired
build artifact, follow the [source build instructions](../DEVELOPMENT.md).

## Convert your game

The original game directory must contain `SlayTheSpire2.pck`, `release_info.json`
and `sts2.dll` (directly or inside `data_sts2_*`). Put the converter there.
On Windows, double-click it or run:

```powershell
.\nspire2-convert.exe
```

On Linux:

```bash
chmod +x ./nspire2-convert
./nspire2-convert
```

The default output is `nSpire2-converted/data/`, with full animation, English
and Simplified Chinese. These options change the output:

| Option | Use |
| --- | --- |
| `--no-animations` | Reduced actor animation; writes to `nSpire2-converted-no-animations/` |
| `--languages all` | Include every supported language |
| `--languages eng,zhs,jpn` | Include only the selected languages; keep `eng` |
| `--game "PATH"` | Read a game installed elsewhere |
| `--output "PATH"` | Choose the output directory |
| `--work "PATH"` | Use a dedicated work directory on another drive |
| `--workers 1` | Use less conversion memory |

Both animation editions use the same launcher. If you create both, run them
one after the other with separate output directories. Repeating a command
resumes completed stages. The [converter guide](converter.md) explains cache
reuse and Linux usage.

## Transfer and launch

Place `nSpire2.tns` and the complete `data/` folder in the same calculator
directory. The data contains:

| Files inside `data/` | Purpose |
| --- | --- |
| `nSpire2.dat.tns` | Resource index |
| `nSpire2.interface.dat.tns`, `nSpire2.cards.dat.tns` | Interface and cards |
| `nSpire2.characters.dat.tns`, `nSpire2.enemies.dat.tns`, `nSpire2.bosses.dat.tns` | Actors |
| `nSpire2.rooms.dat.tns`, `nSpire2.effects.dat.tns`, `nSpire2.shared.dat.tns` | Rooms and shared artwork |
| `nSpire2.eng.dat.tns`, `nSpire2.zhs.dat.tns` | English and Simplified Chinese |
| `nSpire2.<locale>.dat.tns` | Any extra languages you selected |

Keep all eight artwork parts and the English file. Every file must come from
the same conversion, including optional languages. Open `nSpire2.tns`, then
choose an installed language in Settings.

## Languages

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

Generate new languages with the rest of the data. Files from separate
conversions can have different resource addresses even when their version and
animation settings match. Some labels added by the port use English outside
the English and Simplified Chinese interfaces.

## Saves and updates

The game creates `state/` beside the launcher. It contains all three profiles,
runs, settings, history and backups. Keep it when updating either edition or
switching between full and reduced animation.

1. Exit the game and back up the entire `state/` folder to your computer.
2. Keep `state/` on the calculator and replace the launcher and complete `data/` together.
3. Replace optional languages with files from the new conversion too.
4. Open the game, choose your profile and select Continue.

Older saves are migrated when loaded, retaining character access and recorded
progress. For a rollback, restore your pre-update `state/` backup with its old
launcher and data; an older launcher may not read a newly written save.

## Starting and unlocking characters

A new profile starts with Ironclad. Complete a standard run to unlock Silent,
then complete a run with Silent to unlock Regent, Regent to unlock Necrobinder,
and Necrobinder to unlock Defect. Winning is not required: defeat or abandoning
a run also counts. Saving and returning to the title does not count, and
Custom/Daily runs do not advance character unlocks. Random appears after all
five characters are available.

Locked characters can be browsed to see their requirements. Unlocks apply
directly; there is no Timeline screen. Existing profiles keep characters that
were already available or played.

When a new profile accepts the tutorial, it starts with Neow, teaches map
selection and introduces combat in the first Nibbits encounter. The run then
continues through rewards to the map.

## Long ASCII seeds

For a seed longer than the calculator's input field, save the complete text in
`seed.txt` and run:

```powershell
.\nspire2-convert.exe --encode-seed-file .\seed.txt
```

In Custom Run, open the seed field, choose **Import seed code** with Up/Down,
and enter the printed `SEED64-` code. Use this import option instead of the
ordinary seed text field. Encoding does not require a game conversion.

## Troubleshooting

| Problem | Try this |
| --- | --- |
| The launcher does not start | Check that Ndless works with the installed OS. |
| A resource is missing or invalid | Re-copy the complete `data/` from one conversion. |
| A language is unavailable | Copy its matching locale file, then choose it in Settings. |
| The transfer runs out of space | Back up `state/`, remove old data and transfer the complete replacement. |
| Conversion was interrupted | Repeat the same command with the same work directory. |

For bug reports, include the build, calculator model, OS/Ndless version,
language and steps to reproduce. Keep game data and firmware out of public issues.
