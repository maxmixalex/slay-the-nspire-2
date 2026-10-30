# UI17 live resource failure and state-write interlock

## Fixed behavior

Production checkpoint: `5e150f4e9a92e73918f7b1b3e8c656be7f808c11`.
Final focused-test checkpoint: `cde0c8a2ba817784de0e8391bcd412407069cb71`.
The latter changes only the generated test catalog; production is identical.
The base is `ec423e4a6a8bc2082a9b2bf54c6c5d26eb8afd2b`, containing the
selected `e3d8858` stat/deferred-header implementation. The slower experimental
directory-enumeration candidate `a8f92db` is not included.

Previously, the calculator host checked resource health after rendering.
Combat or merchant preparation in `App::advance` can perform the first actual
read of a deferred artwork part. If that read failed, the same host iteration
still dispatched already-captured input. A Save action could therefore write
state before rendering noticed the failure; Confirm Quit could persist
progress and skip that final render altogether. Resource reads and a state
write can also occur inside a single App operation, before control returns
to the host.

The calculator now owns a one-byte logical lifecycle in
[`ResourceRuntimeGuard`](../include/resource_runtime_guard.hpp):

- **Starting:** initial locale candidates may fail and be retried. State
  mutation callbacks remain disabled until one complete initialization succeeds.
- **Active:** the host checks Content health before and after update, each
  captured input sample, repeat dispatch, host services and rendering.
- **Failed:** the first observed live resource failure permanently closes the
  write gate for this process. Reopening Content or changing profile cannot
  reset it. The host stops input and presents its built-in English resource
  error once, including when the same App operation already requested Quit.

[`platform/ndless/main.cpp`](../platform/ndless/main.cpp) owns this lifecycle.
The checks neither read resource files nor scan payloads. They add no image
pool, cache or heap allocation. Final launcher allocation and target timing
belong to the combined build/device receipt, not this isolated main-TU gate.

## Mutation boundaries and rollback

The actual calculator entry points reject an unhealthy or latched-failed
session before opening any mutable file:

| State | Production entry points |
| --- | --- |
| Run and its recovery files | `save_game`, `clear_save` in [`run_storage.inc`](../platform/ndless/run_storage.inc) |
| History/progress | `save_progress` in [`progress_storage.inc`](../platform/ndless/progress_storage.inc) |
| Settings | `save_preferences` in [`preferences_storage.inc`](../platform/ndless/preferences_storage.inc) |
| Profile selector and deletion | `store_profile_selector`, `erase_profile_files`, `request_profile`, `service_profile` in [`profile_storage.inc`](../platform/ndless/profile_storage.inc) |
| Global language preference | `store_language`, `request_language` in [`language_storage.inc`](../platform/ndless/language_storage.inc) |
| Local feedback export | `write_feedback` in [`feedback_storage.inc`](../platform/ndless/feedback_storage.inc) |

`clear_save` moved from the main translation unit into the existing run-storage
include without changing its healthy transaction. The small language functions
moved together so the same production preference reader/writer is exercised by
the regression.

A normal manual language change remains transactional: if its optional
candidate fails, the entire previous resource/App state may be restored before
the host observes the completed operation. A successful rollback remains live.
In contrast, an advance/input failure is latched before language/profile service
can run. A later successful `reload_language` cannot reopen that failed gate.

The guarantee is **no subsequent state mutation after the resource failure is
observed**, not absence of all prior writes in the session. Initial successful
selection still writes its language preference before the first render, as it
did before this change. A later first-render failure does not retroactively
undo that preference. Optional performance/diagnostic reports may still be
written during host shutdown; they are not run, progress, settings or selector
state. This change does not scan unused parts or promise detection before their
first actual use.

## Executed verification

[`resource_failure_regression.cpp`](../tests/resource_failure_regression.cpp)
uses a real App, synthetic resource providers and the actual Ndless storage
includes with an in-memory fault-observing filesystem. It is a normal registered
native test, not a source-text search or a parallel implementation of storage.

The **170 checks** cover real combat-preparation reads failing in `App::advance`
before pending Save/Confirm Quit, failure between input samples, and an actual
Settings Skip Intro action that reads a bad localized toast before invoking its
settings callback. They also cover run/progress/clear as the first observers
inside an App call, all guarded mutation entries, unchanged existing file bytes,
zero temporary-file opens after failure, failed-gate persistence through actual
language reload/profile refresh, successful optional-language rollback and
normal cold initialization/save/settings/progress/profile/quit behavior.

| Gate | Journal and result |
| --- | --- |
| Focused native | `ui17-resource-failure-focused-4527a68a7c9c`: resource failure **170**, existing run storage **1,397**, history storage **9,463**, profile storage **259** checks, all passed |
| Focused ASan/UBSan | `ui17-resource-failure-asan-b2d70d14318f`: **170** checks passed; the new lifecycle, actual Ndless storage includes and fixture are instrumented; the unchanged core is the frozen native library, so this is not a whole-core sanitizer claim |
| ARM926 main translation unit | `ui17-resource-failure-arm-main-2e2208dccf5f`: passed with production freestanding/soft-float/no-unaligned-access flags and the 8,192-byte frame warning gate |

All successful journals have `source_changed:false`. Focused native reuses
`ui17-directory/build/native/libspire_core.a`; its `src`/`include` inputs at
`c747a5024575cbc4d43ee95cfc84d6af058ea1cf` are identical to this branch's base.
Only the changed host/header/test code is rebuilt here. LeakSanitizer is disabled
for the restricted execution environment; address and undefined-behavior
instrumentation remain enabled for the focused scope.

The first focused journal, `ui17-resource-failure-focused-aabd63bbd2ec`, is a
recorded failure. Its v14 synthetic catalog did not include the later startup
toast, so that settings action did not issue the intended failing string read.
The test-only follow-up uses a generated UI40 startup catalog and separates the
cold text page from the locale-marker page. The unchanged assertion now observes
the real provider failure and settings callback short-circuit. No production
behavior or failure assertion was relaxed.

No NAND, original assembly, private text, font or artwork is part of this
change. Full combined ARM/native gates and ordinary-OS Firebird execution are
reported separately by the integration/device owners.
