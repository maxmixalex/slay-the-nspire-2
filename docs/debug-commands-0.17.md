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
calculator commands are all usable through its menu without text entry.

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
