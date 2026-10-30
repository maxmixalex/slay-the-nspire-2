# UI17: original Unicode seed canonicalization and legacy hashing

Reference: **v0.111.0 / 41cef1ea**. Production checkpoint:
`c45d886735ac98cf0c1f43e8665b8876a0c61a48`, based on UI16
`1526ede2a84e168cd46d430338e5d5a540ecaf18`.

The previously documented ASCII-only seed difference is now implemented. This
change adds original Unicode casing and trimming, preserves the separate raw
legacy hashing path, and connects both to playable runs and save/continue.
It adds **no mutable static memory**, does not expand saved fields, and does not
include an ICU runtime or extra font assets.

## Original rules and the two entry paths

The privately supplied `sts2.dll` is 9,757,184 bytes and has the pinned SHA-256
`0861bfa1df347538d932f22d580e75420f08082792eb914e53b4882764acdbe9`.
Inspection and execution used these original methods:

| Method | Metadata token | Relevant behavior |
|---|---|---|
| `SeedHelper.CanonicalizeSeed` | `0x06004d71` | Invariant uppercase, replace ASCII `O` with `0`, replace ASCII `I` with `1`, then trim Unicode whitespace |
| `StringHelper.GetDeterministicHashCode` | `0x06004d7c` | UTF-8 encoding followed by xxHash64 with seed zero |
| `StringHelper.GetDeterministicHashCodeOld` | `0x06004d7d` | Alternating two-lane 32-bit hash of UTF-16 code units |
| `RunRngSet(string)` | `0x06000c82` | Test raw lowercase `old` prefix; remove exactly three UTF-16 units and use the old hash, otherwise use the modern hash |
| `StartRunLobby.BeginRunForAllPlayersIfAllReady` | `0x060015f3` | Canonicalize ordinary input; a debug seed override bypasses that canonicalizer |

The first four calls in `NGame._EnterTree` unconditionally set both default
thread culture and default thread UI culture to `InvariantCulture`. The
one-argument `StartsWith("old")` is therefore still a linguistic operation,
but its intended culture is established by the original game, rather than
inferred from this computer's language. The original runtime configuration
does not enable invariant globalization mode or request Windows NLS.

Ordinary input `oldA` becomes `0LDA` and uses the modern hash
`EC8011CFFF0DFCC3`. It must not accidentally activate legacy hashing. The setup
screen now retains the entered text until departure, like the original custom
run screen. An actually blank input requests a generated seed; whitespace-only
input is an explicit seed which canonicalizes to the empty string.

The raw path is available through `world_set_seed_raw` and the explicit existing
console shortcut extension `new IRONCLAD raw:oldA`. It produces
`00000000CDCAB7DD`; `raw:olda` produces `00000000CDCAB7BD`. The ordinary numeric
console fixture syntax, for example `new IRONCLAD 1104089578`, retains its old
behavior. The `raw:` adapter makes the original debug-override behavior usable
without changing normal setup input semantics.

### Unicode details

The original operation is simple invariant uppercase, **not NFC/NFKC**, and
does not perform full-string case expansions. `é` and `e` followed by combining
acute remain different seed strings. `ß` is not expanded to `SS`; ligatures are
not expanded; fullwidth letters are not converted to ASCII. Dotless `ı` remains
dotless under the original invariant mapping, while ASCII `i` becomes `1`.
Supplementary-plane letters use their scalar uppercase mapping, and emoji are
preserved. The implementation trims the same 25 Unicode whitespace scalars.

The read-only casing data comes from the pinned [Unicode 15.1 data file](https://www.unicode.org/Public/15.1.0/ucd/UnicodeData.txt),
with the invariant dotless-i exception confirmed against the [.NET 9.0.7 native casing implementation](https://github.com/dotnet/runtime/blob/v9.0.7/src/native/libs/System.Globalization.Native/pal_casing.c)
and [managed ICU casing caller](https://github.com/dotnet/runtime/blob/v9.0.7/src/libraries/System.Private.CoreLib/src/System/Globalization/TextInfo.Icu.cs).
The generated table and retained Unicode license are distributed with the
source; the upstream data file is not required at runtime.

The linguistic legacy prefix has two easily missed details. Ignorable
characters can participate in a match, but the constructor still removes the
first **three UTF-16 code units of the original string**, rather than the end
of the matched prefix. A combining accent immediately following `old` can
prevent a prefix match. An intervening soft hyphen changes that case again.
The implementation uses a bounded table specific to this one prefix and the
executed invariant-culture oracle. The hash alternates UTF-16 units between two
lanes initialized to 352654597, applies `lane * 33 XOR unit` with 32-bit wrap,
and combines them with multiplier 1566083941. The constructor zero-extends the
final 32-bit result to its 64-bit seed. Astral pairs and a substring boundary
which splits a pair are covered by original-constructor vectors.

## Production integration and retained boundaries

`reference_rng.cpp` validates UTF-8 and handles casing, hash selection and
complete-scalar append/backspace. `world_rng.cpp` separates ordinary
canonicalization from raw seed identity, then initializes the existing 15
named random streams. `map_preview.cpp`, setup/departure input and the debug
shortcut use the appropriate path. The X11 seed editor now accepts the same
UTF-8 IME commit flow already used by card search. Seed clipping and deletion
preserve scalar boundaries.

The existing field still holds at most **64 UTF-8 bytes plus its terminator**.
That is a byte limit, not a promise of 64 Unicode characters. Case conversion
which would exceed it fails without a partially accepted seed. Invalid,
overlong, truncated, surrogate-encoded and out-of-range UTF-8 is rejected. The
C-string interface cannot represent an embedded NUL, and malformed .NET
strings containing unpaired surrogates are outside this UTF-8 interface.
Save format, `State` size and history field sizes are unchanged.

The calculator still has its physical keyboard; this change does not claim to
add a calculator CJK input method. Rendering uses installed font coverage, so
arbitrary seed Unicode support does not imply every scalar has artwork. The
API and existing Unicode-capable host input adapters accept valid text.

The executed oracle environment is **Linux, .NET 9.0.22, ICU 74.2.0.0,
InvariantCulture**, with invariant globalization and NLS disabled. The
original supplied assembly targets .NET 9 and came with runtime 9.0.7. This
report does not claim that every Windows NLS mode or future ICU version has
identical collation. Regenerating the fixed prefix table requires the reviewed
oracle digest; a different environment cannot silently replace it. Scalar and
mixed-context coverage below is measured evidence, not a proof of every
possible ICU collation context.

## Executed verification

The optional source-only runner in `tools/seed_reference_oracle` checks the
original assembly digest and its culture initialization. Direct assembly load
would invoke its Sentry/Godot module initializer. The runner preserves the
original and creates a private copy with **only the two bytes at file offsets
584 and 585 changed**, turning that initializer into a return. All actual
canonicalization, hash, constructor and RNG method bodies and metadata remain
unchanged. Its isolated assembly digest is
`ad22c60604a9da57c0dfe46acb0b44f34b10679f70f79ba13e7bed5e17cba961`.
No assembly, disassembly or private generated full oracle is included in Git.

The runner executed the actual original canonicalizer over all **1,112,063
non-NUL valid Unicode scalars**. It also executed the original constructor and
helpers for **31,209 seed strings**, including 30 curated cases, seven
placements for every classified prefix scalar, and 20,000 deterministic mixed
strings. Native and ASan/UBSan comparison each passed **3,354,588 checks**.
The table reproducer independently regenerated byte-identical outputs:

| Output | SHA-256 |
|---|---|
| Scalar canonicalization oracle | `ac71b0002606cb113b7972d444b1f38183e1009983999c97963dcd0d18d8b13c` |
| Invariant legacy prefix classifier | `17a88c40b595ca7c3d7d025f30a9fe13f683b3fa802348e41b3320180b546329` |
| 31,209 original seed vectors | `324f8bfc78bf7012c0c1f4600f858591a5e69bb8d4e03ac12587e81c23c45fec` |

The nine targeted CTest registrations all passed. The new seed text test has
951 regular checks including the 30 public factual vectors; the new world/UI/
save test has **1,161 checks**, including raw and ordinary seeded playable maps,
all 15 stream states and next draws, byte-identical save round trips,
save/continue, history encoding, empty-versus-blank input, IME commits,
console entry and atomic invalid-input failures. Existing run modes, character
departure, debug console, world map, state codec, UI progress and device input
feedback regressions also passed.

Formal production-checkpoint journals, all with `source_changed:false`:

| Gate | Journal | Result |
|---|---|---|
| Native target compilation | `ui17-seed-native-build-b61fdfa04936` | Passed |
| Nine native integration suites | `ui17-seed-nine-native-56b3bd3ecd81` | 9/9 passed |
| Full oracle ASan/UBSan | `ui17-seed-text-asan-interpreter-15e46c0ee9f1` | 3,354,588 checks passed |
| Complete Ndless ARM build | `ui17-seed-arm-1aaf3b716b88` | Passed |

An earlier sanitizer launch reached the container's unsupported LeakSanitizer
ptrace condition, and another launch hit stripped generated-file execute
metadata. The recorded successful sanitizer run disables **LeakSanitizer
only**, retaining ASan and UBSan, and launches the unchanged ELF through its
declared interpreter. These environment failures are not represented as
passing executions. This seed component has not separately claimed Firebird
or physical-calculator performance evidence.

## Storage and memory cost

The case table contains 87 ranges and 111 pairs, totaling **1,488 bytes**. The
legacy prefix table contains 237 ranges, totaling **1,896 bytes**. Both are
read-only. There are no new permanent conversion buffers or heap dependencies.
The following section sizes compare actual complete ARM ELFs against the
byte-reproduced UI16 launcher, not estimates from host object files:

| Measurement | UI16 | UI17 seed component | Difference |
|---|---:|---:|---:|
| `.text` | 2,490,636 | 2,493,040 | +2,404 B |
| `.rodata` | 659,612 | 663,036 | +3,424 B |
| `.data` | 4,353,104 | 4,353,104 | 0 B |
| `.bss` | 14,863,028 | 14,863,028 | 0 B |
| Zehn executable allocation | 22,366,388 | 22,372,212 | +5,824 B |
| Relocation metadata | 157,096 | 157,072 | −24 B |
| Total loader allocation | 22,523,484 | 22,529,284 | +5,800 B |
| Compressed launcher | 1,760,617 | 1,765,339 | +4,722 B |

ARM static stack reports are 48 bytes for canonicalization and 56 bytes for
seed hash selection, excluding callees. The tables and their searches are
used during seed operations; this change adds no per-frame rendering work or
startup asset scan. These figures isolate this component and are not the final
combined UI17 memory measurements.

## Reproduction

Use the privately owned pinned `sts2.dll`, `GodotSharp.dll` and
`System.IO.Hashing.dll`, an external private output directory, .NET 9 and the
same ICU version to reproduce the exact collation data:

```sh
dotnet run --project tools/seed_reference_oracle -- "$ASSEMBLY_DIR" "$ORACLE_DIR"
python3 tools/generate_seed_case_tables.py "$UNICODE_15_1_DATA" --check
python3 tools/generate_seed_legacy_prefix.py "$ORACLE_DIR/prefix-invariant.tsv" --check
cmake -S . -B build/seed-check -G Ninja -DSPIRE_X11=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build/seed-check --target seed_text_regression seed_world_regression
build/seed-check/seed_text_regression "$ORACLE_DIR/seed-original-vectors.tsv" "$ORACLE_DIR/scalar-original.tsv"
ctest --test-dir build/seed-check -R '^seed_(text|world)_regression$' --output-on-failure
```

The exact Unicode input SHA-256 is
`2fc713e6a31a87c4850a37fe2caffa4218180fadb5de86b43a143ddb4581fb86`.
The runner records original/isolated hashes, changed offsets, runtime, OS,
culture, sort version and globalization switches in its private receipt.
The ordinary CTest targets need no original game files. Public-source tests
cover seed implementation and integration; they do not certify every random
call in a complete original-game campaign.
