# UI6 developer commands

Developer mutations are compiled only when `SPIRE_DEBUG_COMMANDS=1`.
The ordinary calculator launcher and browser runtime omit the console,
transaction storage and mutation exports. Legacy practice/reference-map menu
entries are removed from the production interface; historical saves still load.

## Build and open

```sh
cmake -S . -B build/debug -DSPIRE_DEBUG_COMMANDS=ON
cmake --build build/debug
python3 tools/build_web.py --debug-commands --compile-only
python3 tools/build_ndless.py --debug-commands
```

The Python builds use separate `build/web-debug` and `build/ndless-debug`
directories. The default debug calculator output is `dist/nSpire2-debug.tns`.
It uses the same three matching resource files as the ordinary launcher.

Open the developer menu with **F1** in the browser/Linux host or **G** on the
calculator. Up/down chooses a command. Left/right chooses a specific encounter,
event, character or act for that row. Enter executes; Esc closes. Browser/Linux
users can press **I** to edit the command line. The debug menu pauses updates;
the calculator uses the menu without text entry and does not show or activate
the **I** editor.

The calculator menu offers fourteen presets: defeat all enemies, heal to full,
gain 1000 gold, set energy to 99, ordinary/elite/boss/shop/rest/treasure/event/map
rooms, a new run, and an act. Left/right enumerates specific encounters/events,
all five characters for a new run with seed 1, or acts 1–3. The browser and Linux
text editor additionally allow targeted kills, custom amounts and seeds, potion
IDs and the `help` command. The command table below describes that complete
text syntax; it does not imply calculator free-text support.

`SPIRE_DEBUG_TEXT_INPUT` defaults to 1. The Ndless builder explicitly sets it to
0; it only controls editor input, its hit region and the platform hint. A native
menu-only regression build uses `-DSPIRE_DEBUG_COMMANDS=ON` and
`-DSPIRE_DEBUG_TEXT_INPUT=OFF`. The ordinary debug-disabled build has no menu or
mutation code regardless of this capability.

## Commands

| Command | Effect |
| --- | --- |
| `kill all` or `kill` | Force all current enemy phases to die, then execute actual death hooks and victory rewards. Also resolves a suspended combat selection. |
| `kill 1` | Force the first enemy to die at an idle player-action boundary. Indices are one based. Other living enemies remain. |
| `room monster` | Enter the next ordinary encounter for the current act. `combat` is an alias. |
| `room elite`, `room boss` | Enter the corresponding encounter type. |
| `room monster ENCOUNTER_ID` | Enter a specific implemented encounter, including encounters from another act. |
| `room event EVENT_ID` | Enter a specific implemented event or Ancient. Omitting the ID uses the next event. |
| `room shop`, `room rest`, `room treasure`, `room map` | Enter the named room. `camp` is an alias for `rest`. |
| `act 1`, `act 2`, `act 3` | Regenerate that act's map and discard its later route history, retaining the current deck and inventory. |
| `new silent 12345` | Start a new temporary run. Character keys are `ironclad`, `silent`, `defect`, `regent`, `necrobinder`; character and numeric seed are optional. |
| `heal 20` | Heal through the actual healing API. With no amount, heal to full. |
| `gold 1000` | Gain gold through the actual gold/relic hooks. |
| `energy 99` | Set current combat energy. |
| `potion 1` | Obtain a potion by its one-based public model ID, if an inventory slot is available. |
| `help` | Show syntax. |

Room commands from the title create a temporary Ironclad run. A room replacement
uses the current map coordinate instead of inventing a new map visit or granting
the previous room's rewards. Pending old combat/event/relic selections are
cancelled. A failed core command restores the complete State and RNG words.
None of these commands autosaves; use the ordinary Save action explicitly if
you want to retain the modified run.

`kill all` also answers combat card choices newly opened by death-triggered
draw/shuffle hooks, using eligible cards through the normal selection API, so
those hooks can finish and the real reward queue can appear. It is bounded;
failure rolls back the transaction. A targeted `kill N` leaves any resulting
interactive selection to the player.

## Browser console and Linux automation

Only the debug HTML installs these helpers:

```js
nSpire2DebugList('encounters')  // Exact model keys for this pinned catalog
nSpire2DebugList('events')
nSpire2Debug('room shop')       // { ok: true, message: 'OK' }
nSpire2Debug('kill all')
```

The Linux debug host accepts repeatable commands before ordinary `--actions`:

```sh
build/debug/nSpire2 --pack /private/ui6-eng.nsp --headless \
  --debug-command 'new regent 12345' \
  --debug-command 'room monster' \
  --debug-command 'kill all'
```

The commands are developer facilities, not claims about original-game console
syntax. Normal gameplay and the historical save codecs are unchanged by the
default-off build option. UI adaptation and original-game parity remain separate
from test-fixture mutations.

## Input-capability verification

Frozen checkpoint `16507a5aaf12ccb344be0347408707f9e1c6dd92` was built with
Clang 18 Release in two independent native directories. Both enable debug
commands; their compiler flags explicitly select text capability 1 or 0.

| Capability | Executed checks | Result |
| --- | --- | --- |
| 1, browser/Linux editor | Preferences, core debug and debug UI CTest cases; actual ENG/ZHS debug UI including typed room commands | Three CTest cases and both actual-language runs passed |
| 0, calculator menu | Debug UI CTest; actual ENG/ZHS runs rejecting I editing, command-line editing hits and text mutations while retaining left/right enumeration, up/down selection and Enter room execution | CTest and both actual-language runs passed |

The menu-only English and Chinese footer captures were inspected. The text
capability's six captured menu/room images remain byte-identical to stage 6.
The new menu hint is the only new inline locale row and is compiled only when
debug commands are enabled. All 935 legacy indexes remain byte-identical.
Ordinary `app`, `content`, `core` and `run` preprocessed translation units also
remain byte-identical to stage 6; this check is separate from portable binary
comparison and emulator acceptance.

Private journals are under `ui6-debug-stage7-journal`: text build
`094d485dd8ed`, text CTest `7aee44eabf70`, menu build `8a5485ea8a02`, menu
CTest `4ac1efcdabf6`, actual text ENG/ZHS `f8d113758d42`/`1b7ebea8a88a`, and
actual menu ENG/ZHS `30b589b72541`/`462f7aec4e41`. All completed commands
reported unchanged checkpoint source. The actual-run logs identify their
input mode as `text input passed` or `menu-only input passed`.
