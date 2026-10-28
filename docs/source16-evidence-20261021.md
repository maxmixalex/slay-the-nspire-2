# Source16: retained rooms, map window and death boundaries

Reference: v0.111.0 / 41cef1ea. This source stage follows the remotely verified source15 commit `7225c880decfb7ff635f960d03ebecd4b0cdd1e2`. It must be pushed/read back and frozen before its long builds and actual-pack reruns. Main, disabled CI and the verified 0.16 delivery pair remain unchanged.

## Implemented source

- The original map is an App overlay over the current room. Opening and closing it preserves the room, choices and canonical save bytes. Original map/point/drawing entry layers, close tail and scroll ordering are separate presentation clocks. Delayed destination submission remains guarded by the complete departure save fingerprint. See [world-map-window-0.17.md](world-map-window-0.17.md).
- Proceed marks a retained room complete once; actual destination entry clears the old room. `world_can_travel` centralizes eligibility for the initial map, merchant, finished event/reward, rest and treasure. Existing serialized `room_open` keeps its width and gains value 2 for a completed, retained room; historical 0/1 values remain accepted. Native event completion retains its terminal page, and repeat Proceed does not recount rooms. Dolly/card pickup decisions still block travel and survive saves.
- Treasure eligibility follows `NTreasureRoom`, rather than the earlier inaccurate unopened-chest assumption: travel is enabled after relic selection or explicit Skip. An unopened chest can display the map but cannot submit a route. Skip resolves an offered relic without acquiring it. Empty-collection presentation timing remains under review.
- Reviving enemies now remove the original shared debuffs on death without erasing the buffs that Illusion retains. A final primary enemy queues living minion deaths through the real death dispatcher, including their hooks and saveable continuation. Five revival types, four minion encounters, suspended-death saves and a surviving-primary negative are covered. See [enemy-death-boundaries-0.17.md](enemy-death-boundaries-0.17.md).
- Ancient buttons now use the eight original colors, the authored additive outline and a border-preserving layout. The two existing static resource entries change without changing IDs or clip definitions. The actual-pack Crystal Sphere driver chooses an enabled cell through its real hit regions rather than assuming cell zero is available. See [actual-pack-and-ancient-ui-20261021.md](actual-pack-and-ancient-ui-20261021.md).

## Evidence recovered and independently checked before this stage

Source15 completed all 126 native build steps and its complete synthetic character UI suite: **38,001 checks / 860 acquired variants**, 445.77 seconds. Its separate 47-suite batch was interrupted after 15 complete passes; that is not a full-suite result.

The earlier source14 conversion journal remains `running`, so its execution is still unfinished/unknown. A complete output file was nevertheless found and validated independently under a new journal: **231,314,177 bytes**, SHA-256 `342983cbb824f1c858375c7cb72193f7ea68778321cd430d1815f060625b7f42`, NSP2/14, **45,724 images / 1,695 clips / 11,915 glyphs / 6,100 strings / 558 cards**. The actual directory is **1,011,812 bytes**. Full canonical decoding/reencoding, retained 0.16 prefixes, clip dimensions and complete text glyph coverage passed.

The unchanged source14/source15 resource/runtime inputs were compared before using the pack with source15. Actual memory/range loading passed **317,981 checks / 40 identical frames**. The actual event UI suite passed **3,655 checks**, while screenshots still revealed Ancient-button contrast and unresolved conditional text; a passing behavioral test is not visual acceptance. The 18-case native capture batch failed at Crystal Sphere frame 2 because its driver clicked a disabled cell. That failure remains recorded; source16 corrects the driver and requires a complete rerun.

Private receipts and logs remain outside this public source tree. No game files, derived pack or original images are included here.

## Short pre-checkpoint checks

Changed root and agent translation units passed syntax checks, and `git diff --check` passed. The extracted map-window formulas passed **106,533** bounded UBSan checks. RGB565/coverage primitives passed **2,921,053** checks, and all seven targeted Python Ancient component tests passed. These focused checks are not a new complete runtime build.

All 336 render scopes were compared with source15: only the static `ui-component` scope changes. All 327 enemy scopes and all 1,657 appended clip jobs remain identical. Of 836 appended image jobs, only the two Ancient button entries change. Existing validated enemy/rest frame caches can therefore be reused under the converter's normal strict checks.

## Next gates

1. Push/read back this source, verify complete tree and changed blobs, and freeze a source-only archive.
2. Build native ASan/UBSan, run the complete affected room/map/death suites and validate historical v15 and retained-room save round trips.
3. Reassemble the complete private pack using checked caches; verify actual parsing and Ancient screenshots.
4. Build ARM/WASM, run the actual 18-case backend comparison with exact frames and saves, and inspect room/event/combat screenshots.
5. Finish original conditional text rendering and the remaining map/room presentation boundaries in a subsequent preserved stage.

No original-game behavioral oracle, 95% visual acceptance, Firebird execution or physical-calculator performance result is claimed by this checkpoint.
