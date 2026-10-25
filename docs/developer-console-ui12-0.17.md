# UI12 original developer console on the calculator

Reference: original v0.111.0 / 41cef1ea `NDevConsole` and the individual
`DevConsole.ConsoleCommands` classes. Replaces the fourteen-item DEBUG preset
menu. Private reference source is not embedded in this repository.

## Input and presentation

- The console is enabled by default in CMake. The calculator host supplies the
  Flag key and ASCII keyboard text; `App::debug_input(Key::Debug)` toggles it.
- It renders a translucent output surface over the current scene, with an
  editable command line. Closing it restores that scene. It owns all input,
  including when a tutorial, credits or inspection modal was previously open.
- Enter executes without closing. Up/Down retrieve eight recent distinct
  commands and preserve an unfinished draft. Left/Right move the caret;
  Backspace/Delete edit at the caret. Home/End and the original line editing
  controls Ctrl-A/E/K/U/W/L are accepted as their control codes.
- Tab completes command names, model IDs, piles and room types. Multiple
  matches produce a selectable list; arrows/Tab move, Enter or a pointer click
  accepts, Escape cancels. An accepted completion does not execute the command.
- Command output wraps into a bounded 24-line history. Long input scrolls with
  the caret. There is no ellipsis concealing a model ID or an error message.
- Buffers add about 3 KiB of fixed App storage. Existing static transaction
  rollback storage is included by the console build; no State is put on the
  calculator stack. Completion reads existing model registries without loading
  private art or allocating one string per model.

## Implemented original command families

| Command | Behavior and index rules |
| --- | --- |
| `help [command]` | Lists implemented commands; specific help reports syntax. |
| `card ID [pile]` | Creates an original card in Hand (default), Draw, Discard, Exhaust or Deck. Full hands fail. |
| `remove_card ID [Hand\|Deck]` | Removes the first matching card through actual pile/deck hooks. |
| `upgrade [index]` | Upgrades a combat hand instance; default index 0, zero based. |
| `relic [add\|remove] ID` | Actual obtain/remove including acquisition choices. Name resolution prefers exact ID, then prefix, then substring, as the source does. |
| `potion ID` | Obtains a named potion using the actual belt capacity and procurement hooks. Numeric port-only potion IDs were removed. |
| `fight ID` | Enters a named encounter with its original Monster/Elite/Boss class. |
| `event ID` | Enters a named event with its actual event logic. |
| `ancient ID [offering]` | Enters an Ancient; a supplied offering substring replaces its first option and retains its other generated options and prepared facts. |
| `room TYPE [ID]` | Uses real room constructors for Monster, Elite, Boss, Shop, RestSite, Treasure, Event or Map. The optional encounter/event ID is a retained port shortcut. |
| `act 1..3` | Jumps to an act and regenerates its map, retaining earlier visits and inventory. |
| `act ID` | Replaces the current act's region with OVERGROWTH, UNDERDOCKS, HIVE or GLORY and generates its actual model pools/map. The serialized region remains explicitly validated against those pools. |
| `kill [index\|all]` | Default is the first enemy; indices are zero based. Actual death hooks and terminal rewards run. |
| `win` | Force-kills all enemies and enters actual combat rewards. |
| `die` | Uses the actual forced player death pipeline. |
| `heal amount [ally]` | Heals the player (default/0); ally 1 is the Necrobinder's Osty. |
| `gold amount` | Adds signed gold through the actual gold hooks, bounded to 0..9999. |
| `energy amount` | Adds energy, rather than setting it; source rejects negative amounts. Maximum engine energy is 999. |
| `stars amount` | Adds stars and runs the character resource hooks; source rejects negatives. Maximum 9999. |
| `draw [count]` | Draws through actual pile/hand listeners, default 1. |
| `block amount [actor]` | Grants unpowered block; actor 0 is player, 1.. are enemies. |
| `damage amount [actor]` | Applies unpowered blockable damage. No target means every enemy; actor 0 is player. |
| `power ID amount actor` | Applies/changes implemented combat powers and listener registration; IDs use the original `_POWER` suffix. Target/family compatibility is validated. |
| `enchant ID [amount] [hand-index]` | Applies a supported original enchantment to the selected combat card; both optional arguments default to 1 and 0 respectively. |
| `afflict ID [amount] [hand-index]` | Applies Bound, Entangled, Galvanized, Hexed, Tainted, Ringing or Smog through actual card-affliction rules. |
| `godmode` | Toggles original Strength/Regen/Buffer powers and reapplies them at each combat setup. The calculator uses the engine's 9999 bound instead of decimal 999999999. It does not bypass damage listeners. |
| `travel` | Toggles free selection of map points, including revisits, without changing graph links or consuming limited travel relic charges. Revisited debug points replace the existing visit rather than inventing extra canonical floors. |
| `bestiary` | Opens the actual Bestiary. |
| `unlock TYPE [IDs...]` | Unlocks local cards, relics, potions, monsters, events, regions, character ascensions, or all supported categories in the active profile. No Timeline or platform achievement record is invented. |

## Explicit platform adaptations

`instant` selects the port's actual Fast Mode. The port does not have the
original Godot global time/wait scheduler. `dump [category] [page]` lists model
IDs in twelve-row console pages instead of writing an unbounded desktop model
dump. `clear` clears console output. `new [character] [seed]` retains the useful
Nspire shortcut for making a test run from the title screen.

`card` does not expose transient None/Play piles. Generic powers that require
an independent card snapshot or a creature family the engine does not model
on that target fail atomically; they are not turned into decorative status
icons. Generic actor indices use 0=player and 1..=enemy slots; the separately
addressable Osty ally is supported by `heal`, not by arbitrary monster-power
injection. Existing model bounds and selection/capacity restrictions remain.

Desktop/platform-only `achievement`, `cloud`, `getlogs`, `leaderboard`, `log`,
`multiplayer`, `open`, `sentry`, `art` and `trailer` produce an explicit
unavailable result. They do not pretend to upload data, unlock platform
achievements, change logging, open an OS file browser or run capture tools.

## Mutation, persistence and acceptance

All gameplay mutations use a bounded State transaction, including validation
and RNG rollback on failure. Actual model hooks, acquisition choices, death
continuations and reward constructors run. Console commands do not autosave
over the player's prior run checkpoint. `unlock` explicitly updates the local
profile, which is the command's requested purpose.

The previous DEBUG-menu regression is replaced with command semantics,
rollback, real card/relic/potion/power hooks, godmode persistence, Ancient
forcing, region replacement, free travel, line editing, history, completion
and no-autosave checks. Native checks establish those contracts only. Final
Flag-key typing and presentation/performance acceptance belong to the
integrated Firebird build.

### Frozen native result

Checkpoint `ee95fa6f085f2e5c360047314790988db610a7f5` passed the focused
console/core build, 80 synthetic command/UI checks, 498 existing debug-core
checks, and 87 checks each with actual logical English and Simplified Chinese
resource packs. The real-asset checks include rendering and pointer closure.
All five logged stages completed with unchanged source fingerprints. They do
not establish calculator keyboard behavior or emulator speed.

The following host-only checkpoint also wires X11 arrows, Tab/Shift-Tab,
Home/End, Backspace/Delete and control-code editing ahead of all other text
modals. F1/grave toggle the console on desktop; calculator uses Flag. No manual
WASM or desktop performance run was performed.
