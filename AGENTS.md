# Development continuity

Read `docs/WORK_STATE.md` before doing work. The 0.15 single-player Ironclad and Silent card pools and retained 0.14 potion/colorless content have passed the recorded validation at `4df76b04e2a07e6be9a898793f03fd0ab8df4aae`. Continue from the verified work branch and its handoff record. Preserve original UI fidelity and source persistence BEFORE long validation. The user requests parallel sub-agents.

## Required checkpoint cycle

1. Read the current remote branch and inspect local changes. Preserve incomplete source on a named checkpoint/work branch; do not put a non-building tree on main.
2. Commit and push each coherent source stage BEFORE long builds, content conversion, or emulator work. Creating a blob/tree/commit alone is not a push: update/create a branch and read the ref back. Do not claim completion without that receipt.
3. Verify uploaded file/subtree hashes against the actual local files. A locally computed SHA is not evidence that the blob already exists remotely. Never repeat an unchanged missing-blob HTTP 422 request.
4. Use `tools/run_logged.py --checkpoint <verified-commit> --label <stage> -- <command>` for long commands. It writes a running journal before starting. A journal left running is unfinished/unknown, not passed. Supply only non-secret command arguments; logs stay local unless deliberately reviewed and uploaded.
5. Publish the result summary and next actionable step in WORK_STATE, including failures and untouched work. Update the source checkpoint before rerunning changed code. Keep a source-only archive as secondary backup.

Do not rely on an uninterrupted conversation, hidden working context, or an unsaved local tree. Do not restart bulk expansion from memory while intact source archives or checkpoint branches are available.

## Scope and evidence

Keep the reference locked to v0.111.0 / 41cef1ea. Audio, multiplayer, and Timeline are excluded. Keep remote CI disabled; `.github/disabled-workflows/ci.yml` must not be moved back into workflows without explicit permission.

Never publish original PCK/assembly data, fonts, calculator images, private disassembly, game-derived packs or embedded-content HTML in this public source repository. Runtime source HTML templates are allowed; generated HTML containing private assets is not.

Count executable and verified content, not registry declarations or art. Native/WASM agreement is not original-game parity. ARM compilation is not Firebird execution or physical-device performance evidence. Historical reports must be identified as historical.
