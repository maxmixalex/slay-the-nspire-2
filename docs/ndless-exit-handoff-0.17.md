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

This source change has passed ARM syntax checking. Its acceptance condition is
the same 80 ms ENTER confirmation in the built game, remaining in the document
browser at least five simulated seconds later, followed by ordinary USB report
readback. That execution is pending the next persisted checkpoint. The OS queue
mechanism is a hypothesis, not a proven root cause or a physical-device result.

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
