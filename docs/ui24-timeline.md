# UI24 Timeline and unlock state

The target remains v0.111.0 / 41cef1ea. Numeric Epoch metadata and localization
keys are in `config/timeline-0.111.0.json` and `config/timeline-text-ui24.json`.
The latter appends 257 original strings at 8704..8960 for every original locale.
Original text and images are private converter inputs, never public sources.

## State and migration

The 57 Epochs use the original None / NoSlot / NotObtained / ObtainedNoSlot /
Obtained / Revealed states. Obtain stores the first actual UTC timestamp;
reveal expands slots and makes associated content available to the next run.
Only Revealed contributes to a fresh profile's frozen two-word run mask.
The score bar grants at most one agnostic Epoch at each Standard run end;
remaining score is clamped below the next threshold. Finished-run accounting
retains its existing outcome guard, including save/reload and retry.

NSHB appends bounded Timeline data to profile saves. Existing NSH1..A profiles
receive compatibility access to previously available content, with no invented
Epoch dates, reveals or accumulated score. Their observed character starts and
statistics remain actual facts. Canonical v18 appends the frozen run mask and
first-run flag; v3–v17 retain the historical all-open run contract.

## Normal UI and generation

The title's Timeline entry opens a chronological LCD list. Inspect shows the
original title, story, obtain time, condition and revealed description/unlocks.
A separate Reveal action applies the original expansion graph. This 320x240
list/scroll presentation adapts the PC's large spatial Timeline and reveal
staging; it does not reproduce its portrait choreography. Pending reveals have
a visible title-menu marker. Legacy access is labelled explicitly.

Fresh profiles gate characters, Random, Custom/Daily and Epoch model pools.
Random appears only with all five characters available. Daily uses the original
all-unlocked policy. Custom portraits and randomization also obey character
locks. Standard first-run and unseen-boss ordering is applied after random room
generation, before the second A10 boss is selected. Without revealed Neow, the
first act opens on the existing map and its root children are reachable.

The ordinary profile records an act when that act is entered, including entry
before any combat or boss. This persisted fact controls original GetRandomList:
unlocked unseen Underdocks is forced without a random draw. Otherwise its
bucket is sampled; Hive and Glory each consume their original one-item draw.
Random character selection then uses the same independent act_selection stream.
Low-level explicit-region map APIs remain available for isolated seed tests.

The [machine-readable acceptance record](ui24-timeline-validation.json) keeps
source identities, readback hashes and the separate pending release gates.

## Executed native and original-reference evidence

The final focused native run passes all twelve selected suites: seed/world,
profile codec, Timeline foundation and integration, original startup vectors,
FTUE/maximum legacy migration, run modes, character departure, power ordering,
world map/relics and world frontend. It contains **297 foundation checks,
275 integration checks and 483 startup checks against 40 original-DLL rows**.
This is a selected affected-suite pass, not a claim of a new full-repository CI
run. The source was unchanged during the recorded run at actual task commit
`7aa74d59640cde3dcf4ed03f4b8772677a1c83b8`. The journal names the published
integration checkpoint `73e1213810bc28672202a3ca83d87fe32aad149c`, which contains
these tested changes. The entire native task tree is not identical to that
integration tree: the latter also includes subsequent Ancient rendering work.
The exact c814 production runtime identity is recorded separately below.

An additional final-c814 continuous firmware session starts a normal legacy
Standard run as Necrobinder and reaches stable Neow at66HP, then verifies the
repaired Strike slash and Offering/Osty death through ordinary card play.
Only the combat fixture uses the built-in console; it is closed during every
action sequence. The blue slash is visible, Offering kills the1HP owner and
living Osty, and both corpses remain on GameOver after10.1 seconds. This is
separate from the console-free fresh Timeline obtain/reveal evidence.

A failed active-character-selection snapshot restore is retained as an excluded
observation: its error page appears before Menu is pressed. Repeating the entire
ordinary OS-launch/selection/start sequence without an intermediate active-game
restore succeeds twice. This establishes the observed boundary; it does not
claim a root cause or repair of that emulator snapshot failure.

The integration tests exercise score boundaries, the one-grant-per-run limit,
Obtained versus Revealed availability, save/reload accounting, actual act-entry
persistence before a boss, old-profile compatibility and frozen run masks.
Custom portraits and randomization obey character locks. Physical Confirm/Menu
starts Custom/Daily on its matching release; Select/Enter retains field meaning.
Seed input and confirmation modals cannot be bypassed by that release, and a
changed form context cancels it.

`tools/timeline_reference_oracle` executes the owner's pinned original
GetRandomList, canonicalization/hash and Random selection. Its 40 numeric rows
cover eight ASCII seeds and five fresh/all-open/unseen-Underdocks states, both
first-act results and all five Random characters, including each RNG state word
and call count. The published optional tool reproduces the fixture byte for
byte. Only the original telemetry module initializer is disabled for isolated
loading; original method bodies are retained and receipt-hashed. Four Acts and
five Characters use their original constructors and ModelDb registration; a real
ProgressState is supplied through a SaveManager mock. The assembly, IL, original
text, game resources and saves remain private.

```
dotnet run --project tools/timeline_reference_oracle -- /private/reference /private/timeline-output
```

This startup comparison complements the explicit-region map vectors in
[ASCII seed validation](seed-ascii-ui24.md). Neither test establishes every
possible whole-run interaction with the original application.

## Actual ordinary firmware flows

The final tested launcher was built from
`c8142bd99423a19e82da7aae9b4e33de4ad7c939`, published equivalent
`73e1213810bc28672202a3ca83d87fe32aad149c`. It is **1,736,530 bytes**, SHA-256
`1c455b5f630dee02750f0b54767850770d05e6643ec670427bf535312447eeb1`.
Formal allocation is **22,287,844 / 22,291,016 bytes**, with no extra heap,
framebuffer, decoded cache or actor reduction. It is an ordinary release build.
The initial Timeline integration exceeded the cap; cold-code optimization
restored compliance before any final-launcher guest acceptance.

Tests used ordinary USB transfer/readback, the existing 64 MiB Firebird
CX II CAS firmware/OS/Ndless path and its 396 MHz configuration. No guest RAM,
snapshot header or CPU setting was modified. The console remained closed in
these normal gameplay flows. Full data and launcher readbacks were byte equal.
The earliest first-death/reveal captures use the preceding strictly passing
`17ec4ca` launcher; persistence, actual Silent selection/start, both configured
Menu starts and normal exit use final `c8142bd`. These identities are not merged
into a claim that every capture was rerun on the last launcher.

| Actual full-edition flow | Observed result |
| --- | --- |
| Legacy profile opens Timeline | Compatibility access is labelled; zero invented Epoch obtains, unknown dates and zero fabricated score. Existing content remains available. |
| New profile, normal first Standard run | Only Neow is listed initially; only Ironclad is available, Random is hidden, Custom/Daily are locked. The first run enters the map without a Neow room. |
| Normal combat defeat and return | Ten ordinary end turns cause the first loss; the summary awards 10 points. Neow becomes obtained with the actual clock time. |
| Inspect and reveal Neow, then Silent1 | Original localized story/condition/unlock text appears. Neow expands the source graph and obtains Silent1; revealing Silent1 unlocks Silent. |
| Normal save, OS exit, relaunch and Continue | NSHB keeps score 10 and the two real dates/states. The v18 saved run resumes at its Neow room. |
| Unlocked Silent portrait and Menu start | Actual selection shows Silent 70HP and the started run retains 70HP. USB save readback loads and validates as character1, Standard mode, with only Neow/Silent1 frozen into its unlock mask. |
| Legacy Custom and Daily physical Menu | A 100ms press remains on the form; release starts the actual Neow room. Daily save readback is mode2 with the dated challenge and all 57 unlock bits. |
| Final save/title/quit | Ordinary OS file list returns. The earlier specific touch-start/first-exit relaunch observation is retained separately; these keyboard passes do not claim to identify or fix it. |

The ordinary fresh-profile progress readback contains Neow and Silent1 as
Revealed, their actual UTC obtain timestamps, score 10 and observed-character mask0x03.
The final Silent run mask is `00000000/00020002`; the separate Daily mask is
`ffffffff/01ffffff`. Production save decoding and game validation accept both
normal USB-readback files. The private captures include first/intermediate/final
screens, original performance counters, command logs and readback receipts.
Normal Settings switching also verifies English Timeline list/detail text and
actual-date display, then Continue to the same Silent70HP run and normal OS exit.
Those later captures follow the actual abandonment of the second Standard run:
score is20, while the same two dates/reveal states are unchanged. The earlier
initial-chain readback's10 points and this later state are separate observations;
no save/reload is counted as an additional finished run.

The full resources are the explicit unified new-slash/NWFv3/Timeline candidate,
actual producer `39d28d60273a62a722a2f1b0efeaf3083d8b7272`, public equivalent
`6ea8bdfc504fdceb2363ce66854ec1990ad0ece4`: eleven bilingual data files total
68,052,158 bytes. This is a tested candidate identity, not a statement that the
final standalone converter and three-ZIP delivery have passed.

The first reduced unified candidate failed ordinary startup with
`RESOURCE LOADING FAILED`. Its NWF rewrite had cleared the NAF1 animation-profile
marker before locale publication; runtime correctly rejected the reduced-only
Neow dimensions when interpreting it as full mode. ENG/ZHS direct and streamed
production-content probes isolate those four bytes. The private diagnostic
copies were not installed. Producer correction and a rebuilt same-family reduced
installation remain pending at this checkpoint, so this failure is not counted
as a reduced Timeline pass.

## Presentation and remaining limits

All57 source nodes are implemented; actual firmware gameplay above exercises
the initial normal chain and legacy/Daily boundaries, not 57 complete campaigns.
Native/source comparisons cover the remaining trigger/state and entitlement
cases. Act2B and Act3B remain the original Early Access placeholders, with no
invented available regions.

The257 appended original strings bring the catalog to 8961 across 16 locales.
Original missing keys follow English fallback; optional absent unlockText keys
remain empty as in the source. The original Thai Epoch table lacks 185 keys.
Port-authored Timeline status labels, like history/archive diagnostics, use
Simplified Chinese or English; other locales use English for those new labels.
The UI list, seven-row paging, scrollable details and text reveal adapt the
source spatial board/portrait presentation to 320x240. UTC timestamps retain the
existing RTC/timezone boundary. Audio, multiplayer and hosted services remain
unimplemented, without an inferred user waiver.

Actual NWFv3 Neow timing is recorded separately in
[Ancient performance](ui24-ancient-performance.md). Its first 30s window after
Continue includes 24 reads and long frames; it is not a fully preheated zero-read
or physical-handheld result. Final converter identity, both final resource
families, the complete70,000,000-byte ZIP gate and all optional-language pairing
remain independent release gates.

The initial six outdated fixture assumptions, subsequent maximum-size legacy
fixture failures, the initial over-budget ARM and the reduced resource marker
failure remain in the journal history. The later focused native pass does not
turn those failed executions into successes.
