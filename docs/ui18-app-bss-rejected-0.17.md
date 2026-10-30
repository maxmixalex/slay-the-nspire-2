# UI18: rejected App BSS construction candidate

**Decision: do not integrate.** This isolated candidate reduces the loaded
image but increases both the installed launcher and continuous execution
allocation. No calculator startup improvement was measured. It is not a
completed space or performance optimization in the selected UI18 release.

The control is `a7e3e9441c0dbd1c0372475769f0ea2a92c0cb81`.
The implementation candidate is `0a702684f81b8c32a1e0524007950e2462a67623`.
The executed source checkpoint is
`bc5126896aa8a0bc519ab52146a13990ef3f6a81`; its two later commits only correct
the Japanese calendar literal catalog and its Python regression, with no
additional ARM implementation change.

## Actual implementation and comparison

`App::init` resets selected UI/resource state and does not replace C++ object
construction. The candidate therefore replaces the Ndless host's initialized
`static App` with a same-size, same-alignment union initially containing zero
bytes. Before any App access in `main`, `value_construct_at<App>` starts its
lifetime and executes all existing nested default member initializers. The
normal App resource initialization and language/profile reloads are unchanged.
No second App, image pool, heap allocation or persistent bookkeeping is added.

The actual ARM compiler does **not** emit another three-megabyte constant
template. It does emit 35,768 additional text bytes for default construction.
The compact existing zero-rich initializer compresses better than that code.

| Quantity | Frozen control | Candidate | Difference |
|---|---:|---:|---:|
| Loaded image end | 7,268,600 B | 4,333,992 B | −2,934,608 B |
| `.text` | 2,511,336 B | 2,547,104 B | +35,768 B |
| `.rodata` | 665,452 B | 665,516 B | +64 B |
| `.data` | 4,091,808 B | 1,121,368 B | −2,970,440 B |
| `.bss` | 14,863,032 B | 17,833,476 B | +2,970,444 B |
| Execution/static end | 22,131,632 B | 22,167,468 B | +35,836 B |
| Zehn allocation including metadata | 22,289,244 B | 22,325,088 B | +35,844 B |
| Default zlib launcher | 1,780,918 B | 1,794,369 B | +13,451 B |
| Selected Zopfli zlib launcher | 1,719,372 B | 1,731,411 B | +12,039 B |

The comparison rebuilt the changed ARM main translation unit, reused the
other 28 frozen control objects with recorded SHA-256 identities, and linked
the complete candidate using the same `--icf=safe` setting. Both encodings
passed the ordinary Zehn decode/allocation/relocation round trips. The extra
two relocation entries account for the additional eight metadata bytes.

The source-identified journal is
`ui18-app-bss-arm-candidate-f7ada1ad1e1f`: **passed**,
`source_changed:false`. The private `arm-comparison.json` records the object
identities and all section/launcher measurements. The control Zopfli launcher
is SHA-256
`909bfbb0501b6828916e69324c8f44d3fbe33ce95e32db1d379abb217bd20e13`;
the rejected candidate is
`e1ed1ff47918f475a4b4340893dc61eb7f8a54f3b33416595e33c4ce000fb05a`.

The branch also contains a proposed native default-byte and resource-lifecycle
regression. It was **not executed**: after the completed actual ARM comparison
established the storage/allocation regression, the candidate was rejected and
further testing was stopped. C++ initialization semantics and a successful ARM
link are not reported as an executed default-byte oracle. No Firebird, NAND,
physical calculator, startup timing or frame-rate test was run for this
candidate. Its isolated branch and bundle preserve the negative result without
changing the selected implementation.
