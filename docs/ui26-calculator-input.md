# Calculator input repair after physical-device feedback

This change follows the physical-device report after UI25. It does not change the
fixed three UI25 resource packages or establish a new physical-device performance
measurement. The existing allocation limit, caches and arenas remain unchanged.

## Hardware contract and settings

The upstream references inspected were Ndless
[`e52415fb568e92cd36ed049067b3e9c2727b84af`](https://github.com/ndless-nspire/Ndless/blob/e52415fb568e92cd36ed049067b3e9c2727b84af/ndless-sdk/libndls/touchpad.c)
and nZATO
[`86a4785560e18d8230e4e7f9f9e18d9be7adc96e`](https://github.com/zyf722/nZATO/blob/86a4785560e18d8230e4e7f9f9e18d9be7adc96e/cpp/platform/ndless/main.cpp).
Ndless reads page 4 registers 0x00 through 0x0A inclusive: byte 0 is finger contact,
bytes 2–5 are big-endian X/Y, and byte 10 is the mechanical switch. Its
[`keys.h`](https://github.com/ndless-nspire/Ndless/blob/e52415fb568e92cd36ed049067b3e9c2727b84af/ndless-sdk/include/keys.h)
routes CX arrows and center through touchpad arrow classification, not a second
independent matrix key. nZATO also scans the pad once and interprets mechanical
pressed/arrow; it does not implement pointer taps or read an OS tap preference.

No supported OS tap-preference getter was found in the inspected Ndless
[`syscall-decls.h`](https://github.com/ndless-nspire/Ndless/blob/e52415fb568e92cd36ed049067b3e9c2727b84af/ndless-sdk/include/syscall-decls.h)
or [`syscall-list.h`](https://github.com/ndless-nspire/Ndless/blob/e52415fb568e92cd36ed049067b3e9c2727b84af/ndless-sdk/include/syscall-list.h).
The game therefore exposes its own **Tap to Click / 轻触点击** toggle, default
off, and **Mouse Speed / 鼠标速度**, with 50/75/100/125/150% choices. 100% retains
the existing 448×336 relative gain. This is an explicit platform adaptation; it
does not claim to inherit TI OS settings. The defaults make mechanical activation
available without requiring a touch contact to activate anything.

Both values use the remaining bits of the existing 108-byte per-profile NSPF
record, version 7. Versions 1–6 retain their original meaning and migrate to tap
off / speed 100%. Versions predating the new bits still reject them. General
Settings exposes both controls to keyboard and pointer, and Reset to Default
restores them. Run saves and historical run records do not contain these values.

## Gesture ownership

- The raw collector classifies a held side press once. Replay resolves a center
  press after earlier queued keyboard events: visible cursor means left mouse
  button; hidden cursor means confirm the current keyboard focus. That decision
  lasts until release, including when the finger slides toward an edge.
- A mechanical down/up report reanchors the finger without moving the cursor on
  that report. Thus the press acts at the preceding cursor location. Subsequent
  held motion still supports dragging.
- A direction or keyboard action owns an already touching finger until lift.
  Residual motion cannot immediately switch back to mouse mode. A fresh contact
  needs six normal-speed pixels of net displacement before restoring the cursor;
  stationary oscillation does not accumulate intent. The threshold is independent
  of the speed preference. Tap recognition is restricted to a visible cursor.
- A short pointer click keeps the tilted cursor and a fading 180 ms outline
  visible after release. It uses the presentation clock and no gameplay RNG or
  image/cache allocation.

The existing 32-entry combined matrix/touch FIFO still preserves complete gestures
across bounded resource reads and CPU rendering. Tests must exercise that replay
path as well as normal foreground polling; the collector must never dispatch App
callbacks itself.

## Targeted acceptance

`calculator_pointer_ui26_regression` covers the 11-byte report layout, physical
center ownership in both binding configurations, pressure coordinates, brief
click feedback, tap on/off, every speed, residual contact and jitter, actual matrix
Enter ownership, drag/cancel, settings persistence/defaults, and unchanged run
save bytes. `foreground_pointer_regression` retains the queued cold-render/decode,
side-pad card selection, bounded repeat, invalid report, and nested capture cases.
Execution receipts belong to the checkpoint validation record; adding tests alone
does not establish that they passed.

Ordinary Firebird and physical-device acceptance should use actual relative
touchpad events: center press/release at a visible control, directional press and
release with the finger still down, drift without lift, deliberate fresh sliding,
center confirmation while keyboard focus is visible, dragging while held, and
tap/speed changes surviving exit/relaunch. Verify both English and Simplified
Chinese labels against the unchanged matching packs. Finish with Custom Start →
Neow → Pause → Save & Quit → title → Quit → Yes → TI OS, then observe at least
30 seconds with input released. Firebird results must remain distinct from
physical-handheld evidence.
