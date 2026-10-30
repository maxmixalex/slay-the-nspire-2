# UI18 launcher compression and conservative code folding

Selected runtime: `a7e3e9441c0dbd1c0372475769f0ea2a92c0cb81`.
This report separates same-image packaging savings from the net size of the
whole UI18 program, which also adds incremental preparation and locale dates.

## Final executable bytes and packaging

The final ordinary ARM executable was packaged twice from the same frozen
ELF. Release packaging uses `zopfli==0.4.3`, five iterations and a maximum of
15 split blocks. The existing level-9 zlib result remains a candidate and is
retained whenever it is smaller. This is an offline build-time choice; the
calculator uses the same existing Zehn zlib decoder and receives the same
executable bytes, relocations, flags and allocation request.

| Same final ELF | Bytes |
|---|---:|
| Level-9 zlib launcher | 1,780,918 |
| Selected Zopfli launcher | **1,719,372** |
| Packaging saving | **61,546** |
| Identical decompressed executable image | 7,268,600 |
| Identical metadata prefix | 157,612 |
| Total Zehn allocation | 22,289,244 |
| Execution/static memory | 22,131,632 |

The two prefixes are byte-identical except for their stored file-length word.
Both compressed streams restore exactly the same image, whose SHA-256 is
`d12ce7ccae82244c4e845013fdf633f3626b876b13c884defa81a92d8239ef85`.
The selected launcher SHA-256 is
`909bfbb0501b6828916e69324c8f44d3fbe33ce95e32db1d379abb217bd20e13`.
There are **39,361 absolute relocations**. The complete **13,369,344-byte
picture pool** is unchanged.

The ordinary Python build checks decompression and every relocation rebase
before writing the destination. Failed packaging preserves an existing output.
The seven focused packer cases cover valid packaging, both allocation bounds,
the rejected image/relocation paths and output preservation. The full integrated
Python gate subsequently passes all 340 tests, including the locale-binding fix.

The selected launcher is **47,207 bytes smaller than UI17**, rather than the
full 61,546-byte same-image saving: the new runtime features have their own
code cost. Total allocation increases by **19,552 bytes** and execution/static
memory by **19,160 bytes** relative to UI17. These remain below the existing
24 MiB total and 22,450,000-byte execution limits. Neither figure is a physical
peak-RAM measurement or a guarantee for arbitrary OS heap fragmentation.

## Safe identical-code folding

The ARM link now uses LLD's `--icf=safe` with the existing function/data
sections, Clang address-significance metadata and explicit emitted relocations.
It does not use `--icf=all`. A matched link of the same 29 component objects
established this change separately from the final integrated features:

| Isolated same-object control | Before | Safe ICF |
|---|---:|---:|
| Level-9 zlib launcher | 1,767,185 | 1,764,759 |
| Total allocation | 22,269,992 | 22,257,788 |
| Absolute relocations | 39,264 | 39,157 |

That control saves **2,426 launcher bytes** and **12,204 allocation bytes**.
It is a component comparison, not another amount to subtract from the final
installed total. Final ARM execution, resource loading, saving and reopening
are identified separately in the integrated delivery and device reports.

## Rejected initialization experiment

Moving the large static App to BSS and constructing it in place really reduced
the loaded image by 2,934,608 bytes. It also grew the compressed launcher by
12,039 bytes and the allocation request by 35,844 bytes. It was rejected and
is absent from the selected runtime. The isolated source and unsuccessful
decision are preserved in [the BSS record](ui18-app-bss-rejected-0.17.md).
The earlier opaque-blend factoring experiment was likewise withdrawn after
its native regression; the selected renderer retains only the independently
verified exact coverage-division optimization.

## Reproduce the release build

```sh
python3 -m pip install -r requirements-release.txt
python3 tools/build_ndless.py --compression zopfli --output build/ndless/nSpire2.tns
```

Omitting `--compression zopfli` keeps the previous zlib developer-build path
and does not require the optional compression package. The portable workflow
is configured to install the pinned release dependency and request release
compression. No remote CI execution is claimed while publication is blocked.

## Evidence identities

| Gate or receipt | Scope |
|---|---|
| `ui18-zehn-python-720d30f11f3f` | Seven focused packer cases |
| `ui18-zehn-real-elf-2ba6c48ddefc` | Earlier same-image component compression control |
| `ui18-safe-icf-real-link-5c7023f03625` | Same 29 objects, safe ICF comparison |
| `ui18-integrated-arm-8c5fa6aae6ea` | Final a7e3 ordinary ARM and selected launcher |
| `ui18-final-zlib-control-f1b135ce2015` | Same final ELF, normal zlib control |
| `final-zehn-compression-identity.json` | Final complete prefix and decompressed-image equality |
| `ui18-integrated-python-fixed-91b3bf539185` | 340 integrated Python tests, unchanged inputs |

The first integrated Python run failed on two undeclared Japanese calendar
literals. Their exact culture bindings were added and the complete run passed;
the original failure remains in the journal archive.
