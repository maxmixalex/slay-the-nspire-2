# UI18 launcher compression and conservative code folding

Selected runtime: `44d7fb79f7d87d522038a074fa80d25e2bdeff31`.
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
| Level-9 zlib launcher | 1,780,919 |
| Selected Zopfli launcher | **1,719,223** |
| Packaging saving | **61,696** |
| Identical decompressed executable image | 7,268,736 |
| Identical metadata prefix | 157,612 |
| Total Zehn allocation | 22,289,380 |
| Execution/static memory | 22,131,768 |

The two prefixes are byte-identical except for their stored file-length word.
Both compressed streams restore exactly the same image, whose SHA-256 is
`0b3beb967148759e2b3456b7e01ee4d4d20382a6df57a8fbbc987d0553be82bb`.
The selected launcher SHA-256 is
`7d34c4163a61c5f4984915d08272c42a3b4b69e003ddb1c388826d21d9098be7`.
There are **39,361 absolute relocations**. The complete **13,369,344-byte
picture pool** is unchanged.

The ordinary Python build checks decompression and every relocation rebase
before writing the destination. Failed packaging preserves an existing output.
The seven focused packer cases cover valid packaging, both allocation bounds,
the rejected image/relocation paths and output preservation. The full integrated
Python gate subsequently passes all 345 tests, including the locale-binding fix
and the final suffix-alignment cases.

The complete final 29-object ARM rebuild at `44d7fb7` is byte-identical to
the ordinary `feb344b` launcher used for the final same-NAND device pair.
Their production `src/`, `include/` and `platform/` trees also match exactly;
`final-device-launcher-identity.json` records that relation. The earlier a7e3
launcher and its different 61,546-byte same-image comparison remain historical
controls and are not the final installation.

The selected launcher is **47,356 bytes smaller than UI17**, rather than the
full 61,696-byte same-image saving: the new runtime features have their own
code cost. Total allocation increases by **19,688 bytes** and execution/static
memory by **19,296 bytes** relative to UI17. These remain below the existing
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
| `ui18-final-ordinary-arm-86e313858b62` | Final 44d7 ordinary ARM and selected launcher |
| `ui18-final-warmup-zlib-control-045a5ee65756` | Same final 44d7 ELF, normal zlib control |
| `final-zehn-compression-identity.json` | Final complete prefix and decompressed-image equality |
| `ui18-final-python-suite-451ac753ab9c` | 345 integrated Python tests at 299ee, unchanged inputs |

The first integrated Python run failed on two undeclared Japanese calendar
literals. Their exact culture bindings were added and the complete run passed;
the original failure remains in the journal archive.
