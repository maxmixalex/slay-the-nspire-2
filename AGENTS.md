# Development continuity

## Current calculator acceptance direction

The current direction is readable 320×240 layouts, a complete English/Simplified Chinese full-animation installation no larger than 70,000,000 decimal bytes, optional support for all original locales, and the existing strict 22,291,016-byte formal allocation ceiling subject to ordinary Ndless startup. Count the launcher, data, instructions and licenses in the installation total. Do not enlarge the heap, cache pools or arenas for tests, and do not reinterpret the older 24 MiB statement as permission to relax this ceiling. Use the normal 64 MiB Firebird firmware/OS/Ndless path for interactive UI, gameplay and performance validation; do not edit guest memory or snapshot headers to bypass constraints. Do not run manual WASM/browser validation unless the user asks for it again; native regressions remain useful for correctness and do not establish calculator performance. Keep platform adaptations and measured limits explicit.

Read `docs/WORK_STATE.md` before doing work. It records the active user feedback;
`DEVELOPMENT.md` contains build commands and target budgets. Earlier UI25
acceptance describes its own runtime and resources, not a later source tree.
Prioritize the current requested input, progression and tutorial repairs.
Preserve verified single-player content, five-character/card/potion behavior,
clear full-animation actor sampling and world generation.

The repository is `maxmixalex/slay-the-nspire-2`. Inspect actual current remote
refs and local changes, use isolated task branches appropriate to the active
work, then synchronize verified progress to `main`; do not assume a historical
integration checkout is current. Preserve source persistence BEFORE long
validation. The user requests parallel sub-agents with smaller independent goals.

## Required checkpoint cycle

1. Read the current remote branch and inspect local changes. Preserve incomplete source on a named checkpoint/work branch; do not put a non-building tree on main.
2. Commit and push each coherent source stage BEFORE long builds, content conversion, or emulator work. Creating a blob/tree/commit alone is not a push: update/create a branch and read the ref back. Do not claim completion without that receipt.
3. Verify uploaded file/subtree hashes against the actual local files. A locally computed SHA is not evidence that the blob already exists remotely. Never repeat an unchanged missing-blob HTTP 422 request.
4. Use `tools/run_logged.py --checkpoint <verified-commit> --label <stage> -- <command>` for long commands. It writes a running journal before starting. A journal left running is unfinished/unknown, not passed. Supply only non-secret command arguments; logs stay local unless deliberately reviewed and uploaded.
5. Publish the result summary and next actionable step in WORK_STATE, including failures and untouched work. Update the source checkpoint before rerunning changed code. Keep a source-only archive as secondary backup.

Do not rely on an uninterrupted conversation, hidden working context, or an unsaved local tree. Do not restart bulk expansion from memory while intact source archives or checkpoint branches are available.

## Scope and evidence

Keep the reference locked to v0.111.0 / 41cef1ea. The current request removes
the Timeline interface and its gameplay gates while retaining independent
character progression and existing-save compatibility. Do not invent dates
for old progress. Audio, multiplayer and hosted services remain unimplemented;
this status does not establish a user waiver. Record genuine platform limits
and remaining differences accurately, and prioritize concrete user feedback.
The user explicitly authorized GitHub Actions and synchronization of verified
progress to `main`. Keep active workflows in `.github/workflows/`; run native
regressions and portable builds without private assets. Clean up branches only
after their history is reachable from main or preserved by an archive tag;
do not discard unique history.

The user explicitly requested removal of runtime integrity/hash checks that are neither original-game requirements nor functional prerequisites. Do not retain full-file FNV scans, integrity-only CRC verification, or eager resource-wide validation passes as a default policy. Keep the bounds, format identification, exact decoding and indexing needed to render/play correctly. Functional RNG/lookup/cache hashes are separate. Developer build/test comparisons do not add work to game startup; report executed performance and compatibility rather than silently changing this policy.

The user authorized saving the completed calculator ZIP and offline HTML to their ChatGPT Library. This permission also resolves the earlier upload approval block; verify that files still exist or reconstruct them accurately before saving. Original assets remain private and must never be pushed to this public repository.

Never publish original PCK/assembly data, fonts, calculator images, private disassembly, game-derived packs or embedded-content HTML in this public source repository. Runtime source HTML templates are allowed; generated HTML containing private assets is not.

Count executable and verified content, not registry declarations or art. Native/WASM agreement is not original-game parity. ARM compilation is not Firebird execution or physical-device performance evidence. Keep reusable technical contracts in the development guides. Superseded stage reports and generated receipts belong in Git history, not a growing documentation archive.
