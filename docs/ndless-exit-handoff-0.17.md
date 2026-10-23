# Ndless normal-exit handoff

## Observed failure and change

The actual `9d8cb1fd` ARM candidate on an isolated Firebird CX II CAS
OS 6.4.0.74 / Ndless r2022 session reopened its document after an 80 ms ENTER
confirmation at Quit. Confirming with the 5 key returned to the OS and allowed
the normal performance report to be downloaded. The alternate key is a control
observation; it does not make the ENTER regression pass.

The platform previously rendered another complete frame after `app.quit`
became true, then waited for release, wrote the optional report and implicitly
closed both resource handles. The normal-exit path now skips that discarded
frame, waits for release, explicitly finishes resource/report I/O, and performs
the existing bounded release gate again before restoring the display. Resource
destructors are idempotent, so no resource close remains after the final gate.
The gate still samples matrix rows and touchpad contact/press, requires 100 ms
of continuous release and stops after two seconds. It changes no keyboard
queue, interrupt configuration or OS clock.

The change was built and executed at
`61970600a8d3f7fb2833af13f82f09c86105ebec`. The 974,608-byte ARM launcher has
SHA-256 `6037aaac82d9a70daaad107008079e2bc2ff083624a77ff332f007b6f64c0cab`;
its complete USB readback matched the built file. In the isolated CX II CAS
session, the same **80 ms ENTER** confirmation retained the game foreground
while held and returned to the OS document browser after release. It remained
there seven simulated seconds later. The first final invocation and the later
direct Chinese and English Continue/save/exit routes all observed this result.
The final trigger was not replaced with the 5 key or a shorter press.

The first final-runtime journal, `final-firebird-runtime-d1c7b32eed5d.json`, is
still **failed / 4**: two initial report requests stalled and were explicitly
cancelled. After resetting the emulated USB link and listing the directory,
ordinary USB export succeeded. The failed transport journal is not a passing
whole-session gate, and the independent flash cross-check is not substituted
for normal USB acceptance.

The subsequent `final-firebird-save-continue-0158306e39a1.json` is closed
**passed / 0**, with unchanged frozen source. Clean Chinese and English routes
each used Continue to reach the same Neow event, saved through the normal game
controls, returned through Save and Quit, and confirmed title Quit with ENTER
held exactly 80 ms. Each remained in the OS after seven simulated seconds and
exported the save and performance report over ordinary emulated USB. Both
230,088-byte saves are byte-identical to the initial Chinese save and pass the
actual core's decode, validation and canonical re-encoding. Their SHA-256 is
`9965d829dd1231146cdc7ee75c1d0e6a0ce07556dc1819b4f9d3ed0c46911d4c`.

An earlier short-key menu sequence accidentally chose an event option; its
subsequent use of the game's O checkpoint reload is recorded separately and is
not counted as either clean Continue route. A long navigation hold also
repeated through menu items, so the final runs checked the selected item before
confirming. These observations do not establish an OS queue mechanism. The
cause remains a hypothesis, and the execution is emulator evidence rather
than a physical-device result. Exact screenshot identities, exported-file
identities, timing boundaries and every closed platform journal are recorded
in [the platform receipt](validation-platform-feedback-0.17.json).

## Why `get_event` is not used as an empty-queue drain

The fixed official Ndless source
[`9484d8da7c7a4dde9766138c2e42e1d1e3acfcd4`](https://github.com/ndless-nspire/Ndless/tree/9484d8da7c7a4dde9766138c2e42e1d1e3acfcd4)
declares `get_event(struct s_ns_event*)` as syscall 261 and describes the
28-byte ARM event structure in `ndless-sdk/include/nucleus.h`. Neither the
declaration nor the loader promises that repeated calls on an empty queue are
nonblocking. The loader itself uses `wait_no_key_pressed()` before restoring
the saved interrupt state; see `ndless/src/resources/ploaderhook.c` and
`ndless-sdk/libndls/wait_no_key_pressed.c` in that fixed source.

A read-only inspection of the supplied OS's call chain found a thread-dependent
queue wait, including an indefinite-suspend path. A private call probe returned
the visible display to the OS without producing its expected post-call result;
that observation does not prove the probe returned normally. A fixed iteration
count around a potentially blocking call would not bound its duration. The
product therefore adds no such syscall, guessed queue address or IRQ write.
The private probe and OS extracts remain outside the source repository.
