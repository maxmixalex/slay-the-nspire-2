# Regenerating content after an original-game update

The existing architecture can be reused: inspect a private installation, extract
original assets and localization, bake the supported scenes, construct a logical
NSP2 pack, apply lossless storage stages, split shared artwork from locales, and
verify the matching calculator build. It is not currently a one-command importer
for an arbitrary new Slay the Spire II release. Replacing the PCK alone does not
implement changed rules or guarantee compatible image and string identifiers.

This document describes the implemented entry points as of the UI19 work. It does
not claim that a complete current pack was rebuilt from a fresh installation in
this round. UI19's executed resource conversion starts with the complete UI18
NSP2/42 installation and produces NSP2/43 resources.

## Private inputs and release identity

`tools/reference_inventory.py` reads the matching `sts2.dll` metadata and
`release_info.json` against `config/reference-release.json`. The current supported
reference is **v0.111.0 / 41cef1ea**. Its `verify_release(cache_identity=True)` path
also hashes the DLL and PCK for offline cache identity; a model inventory is not a
proof that every model's behavior is implemented.

The input set includes `SlayTheSpire2.pck`, the matching DLL and release metadata,
the original locale/font inputs, and any stage's original scene catalogs. The
world and later scene bakers also use a private, separately obtained official
Spine 4.2 C# runtime and the reviewed Godot FastNoiseLite header. Build that
conversion dependency with `tools/build_private_spine_runtime.py --source ...
--output ... --dotnet ...`. It verifies the source manifest, the reviewed
`e7dc1435fa4a0083ab431f1b28e083c14a1f5c68` revision, 47 C# files, and the license.
It is an offline dependency and is not linked into the calculator executable.

Keep these inputs, baked original artwork, caches and generated packs outside the
public source tree. Record input identities and source checkpoints with the
conversion receipts so that an old cache cannot silently supply a changed scene.

## What the existing entry points actually cover

| Stage | Existing implementation and required predecessor |
| --- | --- |
| Historical base | `build_content.py GAME --output ... --font ...` checks the pinned installation, builds the original base, then calls the card/expansion/character stages through `build_all_characters.py`. Its final scope is the historical 0.16 stage, not the complete UI19 resources. |
| World content | `build_world.py GAME SOURCE --output ... --font ... --runtime ... --dotnet ... --fastnoise-header ... --cache ...` extends the corresponding base with world scenes and content. |
| Earlier UI/resource additions | The retained/tile builders and UI6–UI10 append scripts add their recorded catalogs, rooms, history, selection, tutorials and glyphs. Their expected input stages, manifests and ordering remain in the stage documents and `WORK_STATE.md`; rerunning them on a final pack is not a supported substitute for this chain. |
| Locales | `build_locale_editions.py --input ... --pck ... --fonts ... --output ... --languages all` uses the original 16 locales and reviewed font roles. |
| Consolidated later scene stages | `ui12_resources.py`, `ui13_resources.py`, `ui14_resources.py` and `ui15_resources.py` take their complete predecessor editions and reviewed original catalogs. UI15 requires `--source-common`, `--source-editions`, `--pck`, `--fonts`, `--cache`, `--output`, plus the complete current artwork catalogs. `--partial-art` is a named test mode, not a final delivery mode. |
| Existing storage stages | The later chain includes UI15 font/string grouping, `split_content.py` and `storage_parts.py` (NSM1/NSS1 concern files and NSL3 locale pages), `predict_index_resources.py` (NSP2/41 TGI2), and `complete_retained_resources_ui17.py`. The completed UI17 and UI18 resource reports record the accepted stage inputs. |
| UI18 completion | `calendar_fonts_ui18.py`, followed by the accepted tile coalescing/slot-compaction/suffix-alignment stages in `docs/ui18-storage-0.17.md`, yields the complete NSP2/42 installation. Coalescing is explicitly a pre-slot-compaction operation. |
| UI19 lossless conversion | `compact_retained_planes.py --input COMPLETE_UI18_DATA --output NEW_DATA --work PRIVATE_WORK --workers 2` emits NSP2/43. It preserves all images, frames, glyphs and strings, compares complete decoded English retained groups, and requires every locale to share that proven artwork. This is a storage conversion, not a new-game-version importer. |
| Delivery | `package_release.py --calculator-only ...` checks locale format agreement, reconstructs the shared installation, creates the archives and reads their actual members back. Runtime and resource checkpoints are recorded separately. Use `--local-checkpoints` when the source has not been published; do not invent a remote receipt. |

For the accepted storage chain, use the exact commands and receipts in
`docs/ui17-retained-completion-0.17.md` and `docs/ui18-storage-0.17.md`. Stage
builders are intentionally narrower than general readers. UI19 centralizes the
current readable maximum in `tools/content_version.py`; individual codec
introduction versions and historical builders' output versions stay explicit.
`tests/test_retained_planes_pipeline.py` exercises an actual synthetic RGR3 pack
through conversion, concern partition/join, NSL3, ZIP packaging and readback, and
checks the Python maximum against the C++ runtime's `MaximumContentVersion`.

## Procedure for a later original release

1. Preserve the accepted reference and its complete private pack. Inventory the
   new installation separately and compare release metadata, model IDs/enums,
   texture/scene paths, animation topology, locale keys and template syntax.
2. Adapt the affected extraction profiles and handwritten game/UI behavior.
   New cards, powers, targets, encounters or save semantics require code review
   and tests. Changing the reference lock or header version does not perform
   this work. Reassess the Spine/Godot dependencies if the original changes them.
3. Regenerate a complete set from the matching input stages, with new source and
   cache identities. Do not splice new locale directories into old artwork or
   combine an old index with newly compressed concern files. Preserve receipts
   and intermediate manifests needed to reproduce the append chain.
4. Verify source behavior against the new reference, then run lossless
   before/after proofs for each storage transform using that same new input.
   Changed original artwork should match the new original; it is not expected
   to match the prior game's pixels. Run catalog/font coverage, every-image
   memory/ranged/incremental readback, native/sanitizer suites, and ordinary
   Firebird cold-scene and combat checks with the 24 MiB allocation limit.
5. Build and package the matching launcher, shared index, all eight concern
   files, and desired locales. Preserve player state only where the reviewed
   save migration/compatibility contract permits it. Publish a concrete
   versioned source and artifact receipt after all required release checks.

Public CI runs synthetic converter regressions, native sanitizer tests and
ordinary ARM/WASM compilation. It has no private original-game input set and
cannot prove fresh full-resource regeneration or original behavior for a future
release. A fully automated, current, from-installation orchestration entry point
remains unimplemented; the existing stage tools and accepted receipts are the
reusable workflow today. The display version `nSpire 0.1.0` is separate from the
NSP2 resource format and the pinned original game's `v0.111.0` version.
