# UI16 selected-hand raster sharing

This is an isolated runtime-memory change based on `def1db3`, with final
production checkpoint `38e21991a24b8b442bec3ee1c5a2cc40c001540f`. It does not
change the storage adapters, resource bytes, image pool or animation timing.
The integration's final storage/performance and device checks remain separate.

## Lifetime and implementation

The selected long hand-card description formerly retained a 53,384-byte ARM
object: one complete card face, the description's unobscured background and
coverage, plus its small identity and scroll state. Only `combat_screen`
requests `selected_hand_tile`, for a focused or dragged hand card. Character
selection and the shop render through mutually exclusive room branches.

`SceneRasterStorage` now has a third `SelectedHand` owner for those pixels.
Its 230,412-byte size remains determined by the merchant raster. The selected
card's source pointer, built identity, start time, selected instance, current
scroll and maximum scroll remain in a separate **24-byte ARM record**. Claiming
the shared pixels after Random or Shop consequently does not reset the existing
1,400 ms pause or 125 ms-per-pixel text movement. A reclaimed raster rebuilds
its exact background, invalidates only the rendered scroll window, and draws
the original current age.

All three existing `instance = -1` paths still affect only the small record:
general hand-cache invalidation, a short description, and a combat frame with
no focused or held card. They cannot overwrite an active merchant or Random
raster. Initialization and language reload still invalidate hand entries;
their next selected face starts the same new identity as before. Returned
pixel references are consumed by the current draw and do not escape a render.

The ten ordinary hand-card rasters remain independent. They are also used by
shop card-purchase flights, transformations, enchantments and smithing; sharing
them with the merchant would be incorrect. The map's drawing raster also
remains independent because map overlays can cover a live shop or combat.

The original `HandTile` array defaults are preserved for constant initialization
on the freestanding ARM target. A newly claimed selected slot starts only an
inactive union byte. Its builder then copy-constructs the complete `HandTile`
from the original source, starting the correct member lifetime without an
extra 42 KiB default clear. The background/coverage ranges are completely
written before use. No dynamic allocation or constructor table is introduced.

## Actual-pack before/after comparison

The independent old renderer is built from `eb3e990`, before the production
sharing change. Both renderer executables include their respective actual
`src/app.cpp` and link the same frozen `def1db3` native core archive. Resource
inputs are the full private English and Simplified Chinese logical packs,
accessed through a counting range provider rather than the direct-memory path.

The test drives real long-card faces and full 320 by 240 frames through:

- Every text-scroll endpoint, a one-pixel move, and the reverse leg.
- Real pointer press/movement to drag the selected hand card, cancellation,
  loss of focus and refocusing.
- Selected card → Random character screen → open Shop → the same selected
  card, with its original source identity and scroll start retained.
- Developer-console open/close over combat.
- Serialized save readback and `App::init` with the same App/Content addresses.
- English → Simplified Chinese → English resource reload.

Each execution records **174 complete frames**. Old and new RGB565 capture
files are byte-identical across **26,726,400 bytes**, SHA-256
`141ad772524c8e1c2c637cc0121d046506cad342aeb588851a50282d9651d938`.
Every frame also has identical canonical save bytes, physical provider call
counts, requested bytes, offset/length trace, and selected-reader
`started`, `instance`, `skip` and `maximum` values. This includes the first
return frame after another scene owns the raster, not only warmed frames.

Four alternating old/new execution pairs reproduce all **696 matching frame,
state, I/O and clock rows**. Process CPU changes range from −10.318% to +7.567%,
with a median paired change of −5.275%. Three pairs improve and one regresses
under host noise. This short native measurement is not a calculator FPS claim
or a claimed additional rendering speedup. The concrete result is lower static
storage with unchanged executed pixels, clocks and resource reads.

The final regression passes 837 checks without capture-file assertions, or
1,013 checks with captures. Additional ownership checks invalidate the hand
while the merchant owns the shared slot, then render an unfocused combat
frame; both operations leave all merchant color and alpha bytes intact.

## Sanitizer and ARM evidence

Clang 18 `-O1 -fsanitize=address,undefined` passes the same **1,013 checks** and
174 captured frames. Its RGB565 file is byte-identical to both native versions.
The changed renderer and test are directly sanitizer-instrumented; the frozen
unchanged core archive is not. LeakSanitizer alone is disabled in the executor's
ptrace environment. AddressSanitizer and UndefinedBehaviorSanitizer remain
enabled, with undefined-behavior failures configured to stop the test.

The actual ARM926 `app.cpp` object is recompiled with the production flags,
then linked with the 28 unchanged, hashed `def1db3` objects and the production
linker script. The ordinary Zehn builder's no-constructors and allocation gates
pass. The integration owner performs the final complete ARM rebuild.

| Isolated launcher | `def1db3` | Shared selected raster |
| --- | ---: | ---: |
| Compressed launcher | 1,759,300 B | 1,759,227 B |
| Zehn header allocation | 22,568,868 B | 22,515,672 B |
| Header/relocation/flag/extra prefix | 157,048 B | 157,076 B |
| Contiguous executable/BSS request | 22,411,820 B | 22,358,596 B |
| Complete hot picture pool | 13,369,344 B | 13,369,344 B |

The pure `.data + .bss` reduction is **53,360 bytes**. Accounting for added
instructions and alignment, the actual executable request falls **53,224
bytes** and the Zehn header allocation falls **53,196 bytes**. The old raster
contained nonzero defaults, so most of its storage was `.data`; it would be
inaccurate to label this entire saving as BSS. The selected-hand function's
ARM stack frame changes from 2,144 to 2,152 bytes. Its large buffers remain
outside the stack.

Private artifacts reside under `../private-ui16-selected-reader`, including
the paired captures, frame transcripts, `comparison.json`, immutable ARM
object manifest and final linked size result. They contain no emulator
execution shortcuts. The native save/reopen check is not ordinary-OS device
restart acceptance.

## Warm OS boundary

The device agent supplied the exact final `def1db3` failure before/after
snapshots from the warm French OS plus USB workflow. Read-only traversal of
the system heap checks every forward/backward link. Immediately before launch,
137 blocks contain 28,797,068 free bytes in total, but the largest free block
is only **22,145,408 bytes**. The original 22,411,820-byte executable request
already exceeds it by 266,412 bytes before loader metadata allocation.

After failure and the error UI, the largest block is 21,851,248 bytes; the
additional change is not all attributed to the loader. The approximately
53 KiB saving here, even combined with the separate 22,528-byte addressing-table
reduction, is insufficient to guarantee a launch from that specific warm
snapshot. This change increases ordinary startup headroom; it does not remove
arbitrary OS heap fragmentation or establish a universal warm-restart fix.

## Verification record

Successful frozen journals:

- `selected-raster-baseline-build-dc241f107a66`
- `selected-raster-baseline-frames-f9a166abc19b`
- `selected-raster-final-native-build-b89e870db92a`
- `selected-raster-final-frame-ab-36e70d3eeb33`
- `selected-raster-final-sanitizer-build-bcb9011c91d1`
- `selected-raster-final-sanitizer-c9171d36435c`
- `selected-raster-final-arm-build-85b8b1886fd1`

Earlier failed attempts remain visible. The baseline fixture initially used a
save-size member as a function, assumed a shuffled card ID was visible, opened
Pause after cancelling a drag, and omitted combat preparation during native
reopen. All were corrected on the unchanged production baseline before the
before/after gate. The first ARM candidate caught a dynamic initialization
table after removing the permanent hand array's explicit pixel defaults; the
final inactive-union/copy-construction design restores constant initialization.
The first transcript comparator also encountered interleaved stdout/stderr;
the final comparison captures the streams separately and compares clean rows.
These failed probes are not reported as successful feature implementation.
