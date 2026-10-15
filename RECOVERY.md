# Recovery handoff — 2026-10-15

## Durable checkpoints

- Stable main observed: `89a18a55bc330ab1bcd7b8dc9054c5e5984cd5ed` (0.12 combat UI).
- Earlier incomplete bulk work: `work/bulk-recovery` at `ac0ebce48ffda54e1308d46f4ee41fc8ede26c1b`.
- Latest recovered bulk fragments: `1571b77275480316eb00a9f65bc040e0e0fdbeff`, tree `6267cd1bfe4ac09eeebbc6ab6e250d422b00ef21`, now reachable through `checkpoint/bulk-20261015`.
- This branch is NOT a buildable release. Header/registry counts (112 cards, 116 relics, 55 potions) are NOT verified completed-content counts.

## Complete local baseline located

`/mnt/data/nSpire2-0.13-source.zip` exists, passes ZIP integrity inspection, and has SHA-256 `7627a99a52c171b81b50a555562656bb5b617a0c3a3e4215583a9ad8f41cfeeb`. It contains 124 files including source and patch manifests. Extraction for this session: `/mnt/data/persistence/source-0.13`.

The matching 0.13 Ndless and offline HTML packages are also available locally. They contain private game-derived content: DO NOT publish them, the original game installation, fonts, calculator images, or private disassembly to this repository.

## Why the earlier recovery failed to publish

Creating Git trees did not advance a branch. A prior attempt also used locally computed blob hashes as if the corresponding objects had already been uploaded; GitHub rejected a missing blob with HTTP 422. Do not repeat that request unchanged. Read an existing tree to verify objects, or upload the actual text, then create a commit, update/create a named branch and read the branch back.

`src/bulk_effects.inc` was not accepted by the earlier write tool and is absent from this checkpoint. Core integration, historical save migration, new test files, and the matching bulk converter/profile remain incomplete. Do not merge this tree into main merely because some files exist.

## Requested scope and workflow

Prioritize original UI fidelity and breadth of relics, potions and single-player colorless cards. Keep reference `v0.111.0 / 41cef1ea`. Audio, multiplayer and Timeline are explicitly excluded. Keep `.github/disabled-workflows/ci.yml` disabled.

Persist source before long compile/conversion/emulator steps. Record each command and outcome in versioned, concise work notes. Mark started/running work as such; never reuse historical success prose as evidence for a new tree. Preserve incomplete work on a checkpoint branch and keep main usable. Complete small stages and verify their remote refs before starting the next stage.

## Immediate next work

Publish the complete 0.13 source from the actual archive, preserving existing historical repository files. Reconcile its manifest with the remote tree; then run local CMake/CTest and the ARM build. Integrate bulk changes against that complete baseline rather than repeatedly synthesizing half-built trees.

No compilation or emulator test has been run yet in this persistence-first session.
