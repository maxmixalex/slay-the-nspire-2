# UI24 ASCII seed compatibility and portable import

Reference: Slay the Spire 2 **v0.111.0 / 41cef1ea**. This report distinguishes the text accepted by the UI, the resulting seed identity, random-stream behavior, and the configurations of the maps actually compared. It does not certify an entire PC campaign.

## What the two capacities mean

`MapDrawingCapacity = 2048` is the capacity of an optional hand-drawn annotation document. A point is a four-byte pen sample. This is not the number of generated map rooms, a map-generation limit, or a seed-space limit. The reference map stores at most `7 * 20 + 3 = 143` point records.

The original port's `char seed_text[65]` stores at most **64 UTF-8 bytes plus the NUL terminator**. The same field width is used by the run setup, world state, save codec, current history record and archived history records. It is a project-chosen representation, not a demonstrated TI OS or hardware limit. Multi-byte characters use more than one byte. An overflowing edit is rejected on confirmation; it cannot silently turn into the first 64 bytes of another seed.

The original game generates **12 characters by default**, using the actual 34-character alphabet `0123456789ABCDEFGHJKLMNPQRSTUVWXYZ`. These generated seeds fit directly. The original custom-run LineEdit has no configured maximum length, and the original seed/RNG methods accept ASCII strings beyond 64 bytes. Those longer custom strings therefore require the explicit import path below.

## Ordinary text and import codes

Ordinary seed text uses the original `SeedHelper.CanonicalizeSeed` rules: invariant uppercase, ASCII O to 0 and I to 1, then the original whitespace trim. It does not normalize NFC/NFKC. A blank setup field requests a random seed, whereas an explicitly entered whitespace-only field supplies the empty canonical string. Ordinary digits remain string seed input; they are not treated as the old calculator numeric bootstrap seed.

The normal Custom Run seed editor offers a distinct **Import seed code** mode. On the calculator, open the seed field and use Up/Down to switch mode, or select the import action below the field. Enter the converter's `SEED64-` followed by 16 hexadecimal digits, or just its 16 digits. The mode validates the complete code and preserves the chosen character, ascension, region and run modifiers. It does not run the code text through the seed hash again.

The same desktop converter provides these independent operations before any game-file discovery:

```
nspire2-convert --encode-seed "the complete ASCII seed"
nspire2-convert --encode-seed-file seed.txt
```

The file path streams the complete ASCII content in 64 KiB chunks. It applies original ASCII uppercase/O-I/trim and xxHash64, with constant-sized hash snapshots to discard trailing whitespace without buffering it. It has no arbitrary 64-byte or 65,537-byte limit. The file must contain the intended seed text; internal newlines and spaces are significant. This import encoder currently rejects non-ASCII text explicitly. Short Unicode seed input retains the previous UI17 behavior.

`raw:123`, `RAW:123`, `SEED64:123`, and `SEED64-0000000000000000` entered in ordinary text mode remain ordinary original-game strings. The import mode is selected by a separate UI state, never by detecting a string prefix. The developer console's explicitly requested `new CHARACTER raw:...` legacy override remains a separate developer facility.

The original long text is not stored on the calculator. The setup, save and history display its portable import code. An explicit origin flag is retained in the existing world/history alignment bytes and serialized flag bits, while the original 64-bit seed and all named RNG states are saved normally. No save or history record is enlarged. Normal old saves keep the same byte representation. Imported seed records require a runtime that supports this flag. End-of-run quote RNG uses the original seed hash for imported seeds, rather than hashing the display code.

Native measured sizes before and after this change are identical: App 2,970,856 bytes; State 200,368; WorldState 11,912; UiRunHistory 7,576; MapPreview 3,400. The ARM launcher still needs its separate strict allocation check because new executable code consumes memory too.

## Direct original-assembly verification

The optional tool `tools/seed_reference_oracle` checks the supplied assembly SHA-256 `0861bfa1df347538d932f22d580e75420f08082792eb914e53b4882764acdbe9` before invoking its actual methods. Only the isolated module initializer is disabled, to avoid telemetry and native Godot startup. The seed, hashing, RNG and standard-map methods retain their original IL. No assembly bytes, private IL or generated game assets are distributed with the source.

The completed UI24 comparison used .NET 9.0.4 on Ubuntu 24.04.3 with the original invariant culture initialization. It covered:

| Coverage | Executed cases |
|---|---:|
| ASCII input strings | 927 |
| All printable ASCII singleton characters | 95 |
| Every default-alphabet character at each of 12 positions | 408 |
| Seeds returned by the actual original random-seed method | 128 |
| Deterministic mixed printable ASCII strings | 256 |
| Named RNG state/draw comparisons | 13,905 |
| Original standard-map constructor comparisons | 7,416 |
| Save/load/save byte-exact round trips, including imported long seeds | 927 |
| C++ comparison assertions | 391,191 |

The input set includes 63/64/65-byte boundaries, longer inputs through 65,537 ASCII bytes, empty/whitespace strings, case/O/I mapping, numeric strings, punctuation, and ordinary text that resembles internal syntax. The desktop encoder also matched all 927 original normalized seed values. A separate normal UI/codec suite passed 1,369 checks, including import mode, invalid codes, overflow rejection, UTF-8 boundaries, character/ascension/region retention, all named RNG continuations, and history serialization.

The original `RunRngSet` contains **12** streams. The comparison tests those actual dictionary entries plus the original named `Rng` constructor for the port's three additional names (`rewards`, `shops`, `transformations`). For each of all 15 names it compares the initial four words, eight returned values, final four words and draw count. The last three cases do not assert that their full player lifecycle matches the original global `RunRngSet`.

The map comparison invokes the actual `StandardActMap` constructor, including its quota consumption, generation, assignment, pruning/repair, centering, spreading and path straightening. It checks all four regions, explicit solo mode, elite quotas 5/8, treasure replacement and second-boss configurations. It compares requested quotas, quota draws, total draws, final RNG words, active coordinates, room types and directed connections. Character identity is not an argument to this constructor. The suite supplies the relevant configuration explicitly; it does not infer the PC player's Timeline, selected modifiers or act choices.

Selected original normalized seed values are:

| Ordinary original input | Canonical text | Original 64-bit value |
|---|---|---|
| `raw:123` or `RAW:123` | `RAW:123` | `899C9710E43EE920` |
| `raw:oldA` | `RAW:0LDA` | `AAB6291187E3C3EB` |
| `SEED64:123` | `SEED64:123` | `2AFD7DFA93C73EFD` |
| `SEED64-0000000000000000` | unchanged | `4A3065D9A1F23113` |
| `1234567890` | unchanged | `A9D4D4132EFF23B6` |
| `0000012345` | unchanged | `73DA7FD0F2A8D281` |
| `oldA` | `0LDA` | `EC8011CFFF0DFCC3` |
| 65 lowercase `a` characters | 65 uppercase `A` characters | `B1E5A6467A3F54D6` |
| 65,537 lowercase `a` characters | 65,537 uppercase `A` characters | `95D0D953D29500E1` |

The complete private TSV SHA-256 is `75d95d4e998a3859878e8a160cf6edd513bd5d28d290092a26e70ac2d567051c`. The final oracle and comparison journals both completed with unchanged source. An earlier first-run oracle journal was invalidated by .NET generating `obj` files inside its source inventory; the final run moved all intermediate/build files outside the frozen source tree and reran the expanded set.

## Executed calculator UI and persistence

These checks used the ordinary **2fbe141c0fd447bdafa212b0bcc622732f3534fc** candidate, published as `e513342da7404cd85c1c334038cfdcdaff3cf3db`, with launcher SHA-256 `809c905858dee6e4eaa62c1b84d8b6f5cbaa0aab02da85ecee35fb3601a9fd44`. It was a normal LLVM18.1.3/zopfli build without profiling flags. This identifies the exact tested seed candidate; it does not claim that every later UI24 launcher was rerun through this entire suite.

On the supplied 64 MiB Firebird calculator, normal key/touch input opened Custom Run, rejected a 65-byte ordinary edit, rejected a non-hex import code, selected `SEED64-B1E5A6467A3F54D6`, and started the run. This code represents the entire 65-character lowercase-`a` original-DLL vector. The same overflow, invalid-code and selected-code screens were checked in Simplified Chinese and in English after changing language through normal Settings. Neither language displayed the other language's hardcoded error message.

The run was saved, the program was restarted, Continue loaded it, and it was saved again. Both files were obtained through ordinary guest USB and decoded by the production storage envelope and canonical `load`/`validate` implementation. Each was **7,074 bytes**, with the same SHA-256:

```
9e76229d07cc4f6d3dd7cd32802a79462ed96e7f7b12dfca7480531977ed4d0a
```

Both retained `run_seed = B1E5A6467A3F54D6`, `seed_imported = 1`, and the full portable display code. All 15 saved RNG states and draw counts were unchanged by this Continue/save cycle. The current history record also retained its 64-bit identity, import flag and display code; elapsed/profile bookkeeping was allowed to change. The save file itself was byte-identical.

A separate exit limitation was observed on the initial touchpad Custom Run path: its first confirmed exit immediately started the program again. A subsequent ordinary keyboard exit reached the OS, after which the recorded restart/Continue/save sequence exited normally. The unsuccessful USB reads attempted before reaching the OS were retained as failed attempts, not counted as successful persistence tests. A plain title exit on the same seed candidate and an old UI23 touchpad-Continue/save/exit control both reached the OS. The precise trigger remains a separate platform/input investigation; these observations do not establish a hashing or save-corruption defect. No guest memory or snapshot header was edited to obtain the successful round trip.

A later narrow control used the final ordinary `c8142bd99423a19e82da7aae9b4e33de4ad7c939` launcher, SHA-256 `1c455b5f630dee02750f0b54767850770d05e6643ec670427bf535312447eeb1`. Normal keyboard input selected Custom Run, entered ordinary ASCII `a`, and used the restored Menu-release Start action. The run reached Neow, saved to the title, and exited to the OS. The OS file list remained visible at both 20 and 30 seconds after exit confirmation. No touch commands were used. This verifies the current keyboard path without claiming that the old touch-start trigger was identified or that the entire earlier import suite was repeated on c814.

## Scope of the result

A PC seed can be reused directly when its entered text and canonical result fit the 64-byte input representation and its characters can be entered with the calculator's physical text keys. The explicit portable code supports complete ASCII seeds of arbitrary practical length, including punctuation/control characters unavailable on those physical keys. For the tested pinned-version configurations, normalized seed identity, the stated RNG calls and standard map results match the original methods.

That statement is narrower than “the whole run will always be the same.” Original version, solo configuration, selected character, ascension, modifiers, act choices, unlocked card/relic/event/boss/ancient pools, initial progression and subsequent player choices can affect what happens. The earlier port opened all pinned solo pools instead of reproducing Timeline/unlock history. UI24's separate Timeline work must document its own evidence; these map tests do not certify that integration or full encounter/reward call order. The arithmetic test runtime is Linux .NET, not the shipped Windows math library, and the existing explicit half-integer rounding guard remains relevant.
