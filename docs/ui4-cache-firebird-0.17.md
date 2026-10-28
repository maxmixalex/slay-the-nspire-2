# UI4 isolated card-cache acceptance on Firebird

The isolated source `ebf1212e71eb39f85e7fe282851b5ac05fc510f8` ran through ordinary OS/USB/Ndless and the same 18 gameplay inputs as the UI3 baseline. Both runs used the unchanged Chinese v20 hybrid pack and the same saved run. The candidate launcher was downloaded over USB and matched the 984,515-byte build exactly. The old performance file was deleted before launching; the accepted report contains 376 collection renders, rather than the stale 67-sample report from an excluded attempt.

| Collection measurement | UI3 baseline | Isolated UI4 cache |
| --- | ---: | ---: |
| Render samples | 368 | 376 |
| Longest render | 1,646.820 ms | 1,508.392 ms |
| Longest resource interval | 1,600.342 ms | 1,462.280 ms |
| Scene read calls | 151 | 76 |
| Scene requested bytes | 620,673 | 353,817 |
| Scene seeks | 135 | 68 |

The longest render improved by 8.4%, while the whole collection scene issued approximately half as many reads. These I/O totals include entry, focus and tooltip transitions; they are not the counters of the single longest frame. The first-entry pause remains about 1.5 seconds and still needs attention. The render mean also includes transitions and is not a steady-state FPS estimate.

The four collection, settled collection, focused keyword and card-detail screenshots are byte-identical. The 230,520-byte saved game is unchanged. The room capture differs in 246 bytes because its background animation is at a different phase; it is not counted as an exact-frame match. No image, text, frame or cache pool was enlarged or removed. The ARM allocation decreased by 2,960 bytes, to 16,302,240 bytes. Both reports contain zero read failures and zero integrity, image, tile-index, glyph or string pre-scan samples.

The accepted candidate cold-booted, acknowledged every ordinary OS USB notification, installed Ndless normally and launched in that same process without restoring a snapshot. The baseline resumed its previously prepared ordinary OS/Ndless pre-app snapshot. Their game inputs and durations match, but identical OS cache history is not claimed. There is one accepted route per variant; this is emulator evidence, not a physical-handheld performance claim.

The closed passed journal is `ui4-cache-candidate-controlled-flow-e9cab12feec4`. Earlier OS-only/stale-report, startup-only and failed USB-dialog routes are explicitly excluded in the companion JSON. A passed controller wrapper alone was not accepted as a passed game run. Final integrated UI4 title and combat acceptance is reported separately.
