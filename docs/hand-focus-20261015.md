# Hand focus revision — validated 2026-10-15

Usable 0.13 recovery was promoted to main at `203c9dd004f3c674a8650b65d92a192b0469792f`. The subsequent hand-focus SOURCE checkpoint is `8d3738561ebf1919f24a43bbaec673bd13d9c697`. Its work-branch ref was read back before any compilation. This stage changes presentation/input focus only; game rules, registry counts, save v10 and NSP2/10 profiles are unchanged.

## Behavior

At rest the hand stays entirely on the bottom arc. Hovering a real hand region or navigating cards with left/right raises the focused card. Moving the unheld pointer off the hand, completing a game action, cancelling a pointer capture or opening an overlay clears focus. Keyboard target selection preserves deliberate focus. Raised preview hit regions exist only while active. The stable lower rail and upper collapsed card faces remain selectable. Captured drags retain their instance identity.

This corrects an always-raised-card defect; it does not claim original-PC pixel or animation-curve parity.

## Verified source and executed results

Local subtrees matched the published source checkpoint: src `bcefb5bb7eb9e451a4fda774eba524700b5f6a5d`, include `16d10f79f072e46bfc425866945d985bc7b3ec80`, tests `bfc319de4a845a76ee2adacdaa429edb11b1a45a`. Tools and platform remain identical to the previously validated recovery checkpoint.

All 17 CTest targets passed with ASan/UBSan. All six actual-pack targets passed; the extended combat UI target passed 1,218 checks without counting optional screenshot pixel-write assertions. Focus cases cover 1-10 cards, hover, keyboard target changes, overlays, cancellation and drag completion. Optional screenshots were rendered by the actual native C++ runtime, not an emulator or image generator.

ARM and WASM rebuilt successfully. The final native driver and WASM reran four room sequences (Aroma transform, Silent Aroma upgrade, Doors removal and merchant purchase/removal): all 84 full RGB565 frames and eight saves matched. This compares project backends using storage callbacks, not original-PC parity or browser DOM/persistent storage.

Every build/test above was journaled with checkpoint `8d3738561ebf1919f24a43bbaec673bd13d9c697`, returned zero and reported unchanged source inputs. A preliminary local Git commit attempt needed a local-only author identity and did not run compilation; no remote code was lost or changed by that failed local command.

## Final outputs

| Output | Bytes | SHA-256 |
|---|---:|---|
| nSpire2.tns | 121162 | 1d29edf67de845e8387cc5181eb00e5c9f540f702ff676f4077cc0fb047036e3 |
| nSpire2.wasm | 411727 | de6be33d9e6b2b78a6ff3a0dbffb9e784ad2b4d48dd1ac8ea236bb18f252b70a |
| generated private desktop HTML | 8641264 | 48a41c763e9da2ec8e3635b29f6aef238988182749ea4fb9e47f2fedfc1305df |
| retained private NSP2/10 pack | 6057527 | e157ee346a4eb1fc2849bc2d18e5dc1f4ccf364b85386d92dc7306a1430908a5 |

ARM static loader allocation is 8,343,120 bytes with 1,545 relocations. This is not measured peak RAM. No images, glyphs or animation frames were changed or removed from the pack.

CI remains disabled. No new Firebird, physical-device, original-game differential, fresh content conversion or sustained frame-rate measurement has been run. No new content additions are counted in this UI stage; the incomplete bulk source remains separately checkpointed.
