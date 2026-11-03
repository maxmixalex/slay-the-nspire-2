# UI24 Ancient room acceptance

The accepted ordinary launcher improves the measured Tezcatara room from
**6.389 to 23.915 FPS**. None of its **717 warm measured frames exceeded
50 ms**, compared with all 192 baseline frames exceeding 100 ms. Neow and
Nonupeipe also improve on the same formal full-animation resource family.
These are actual Firebird guest-counter measurements. They do not establish
physical-calculator FPS or a universal 30 FPS target.

The user authorized simplifying Ancient decorative effects. This change
preserves the original converted actor frames and clocks, while reducing
procedural illumination and background particle work. The final presentation
policy is frozen at the settings documented below.

## Machine, program and measurement identity

The supplied Firebird checkpoint identifies **TI-Nspire CX II CAS**, running
**OS 6.4.0.74**, with **64 MiB RAM**. The retained normal cold-boot log reports
**CPU 396 MHz, AHB 198 MHz and APB 99 MHz**. No additional overclock setting
was applied. These recorded settings are not an independent certification of
factory frequencies, and the result must not be generalized to an older
132 MHz CX. Host turbo was enabled; frame rates are calculated from the
guest's **32,768 Hz SP804** counters, never host wall time.

The final Tezcatara measurement uses actual source
`c8142bd99423a19e82da7aae9b4e33de4ad7c939`, source-equivalent to published
`73e1213810bc28672202a3ca83d87fe32aad149c`, tree
`23c96872fbe9b580efa694b03fe633b70baf6e4b`. The normal LLVM 18.1.3 / zopfli
0.4.3 launcher has **no profiling build flags**:

| Item | Value |
| --- | --- |
| Launcher size | 1,736,530 bytes |
| Launcher SHA-256, including ordinary USB readback | `1c455b5f630dee02750f0b54767850770d05e6643ec670427bf535312447eeb1` |
| Linked allocation | 22,287,844 bytes |
| Unchanged strict allocation ceiling | 22,291,016 bytes |
| Remaining allocation headroom | 3,172 bytes |

The baseline is the ordinary UI23 `d9350b94c234f443c29f12f7efb862f3595a6a57`
launcher, SHA-256
`9a00d1f6e3e795839914c0f65929b60eafe856c27c726157b055423e9e60dfae`.
No heap, resident framebuffer, arena or pool was enlarged.

Each measurement starts two sessions from the same closed, paired NAND/RAM
snapshot. Session B runs 30,000 additional guest milliseconds before the same
normal pause/save/exit sequence. Subtracting counters removes the shared
entry and exit path. Installation, launcher readback and counter retrieval use
ordinary OS/Ndless and USB operations. No guest memory or snapshot header is
edited. Nonupeipe and Tezcatara are controlled encounters established through
the normal developer console, then saved and continued; they are not claims
of a complete naturally played run to those rooms.

## Same-resource warm comparisons

All rows in this table use the original **formal full-animation bilingual
bad0168 resource family**, whose 11 required data files total 68,155,584 bytes.
They do not reuse the earlier `full7229` controlled package as if it were the
formal one. The newer Neow/Nonupeipe rows use the first ordinary UI24 `b690`
launcher; only the final Tezcatara row uses the later complete `c814` launcher.

| Room / program | Frames | Guest FPS | Mean render ms | Intervals >50 ms | Intervals >100 ms | Resource reads |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Neow, UI23 d935 | 337 | 11.201985 | 88.817200 | 337 | 1 | 0 |
| Neow, UI24 b690 | 880 | **29.312073** | **33.736766** | **0** | **0** | 0 |
| Nonupeipe, UI23 d935 | 684 | 22.797750 | 43.505351 | 0 | 0 | 0 |
| Nonupeipe, UI24 b690 | 848 | **28.283556** | **34.981098** | 0 | 0 | 0 |
| Tezcatara, UI23 d935 | 192 | 6.388653 | 155.985037 | 192 | 192 | 0 |
| Tezcatara, UI24 c814 | 717 | **23.915010** | **41.456747** | **0** | **0** | 0 |

All six windows also have zero intervals above 250 ms and zero resource bytes
read. Tezcatara FPS is **3.743 times** the baseline and mean render time falls
**73.423%**. This remains about 24 FPS, with enough stability to remove the
previous repeated 100–160 ms warm stalls. It is not described as 30 FPS.

Cold work remains visible. The final Tezcatara session's cumulative counters,
which include entering the room, contain a maximum interval of **2,035.248 ms**
and maximum render of **1,954.498 ms**. These are maxima from a lifetime that
includes loading, not the warm interval maximum or an isolated causal loading
timer. Baseline and new room-entry paths differ, so they are not used to claim
a percentage reduction in cold-entry time.

## Latest Neow with the new NWF3 package

A separate actual `c814` run installed the unified NWF3 full candidate, played
a new profile through its first combat death, revealed Neow through normal
Timeline UI, saved, and continued into Neow. Its matched subsequent 30-second
window has **872 frames / 29.055081 FPS / 33.228428 ms mean render**.

That window still performs **24 reads / 511,552 bytes** while traversing its
first subsequent animation cycle. It contains **12 intervals above 50 ms,
5 above 100 ms and none above 250 ms**. It must not be relabeled as the fully
prewarmed zero-I/O result in the preceding table. The cold-containing A
session records a maximum render of **1,604.065 ms**, with 113 room-lifetime
reads totaling 1,480,195 bytes. Cold animation/resource access therefore
remains a source of occasional stalls even though the steady rendering cost
is much lower.

The complete full/reduced installation and ZIP gates subsequently passed,
as recorded in [UI24 release acceptance](ui24-release-validation.md) and the
recovered Timeline/animation receipts. Complete standalone converter execution
is a separate gate. This report identifies the resource candidate actually
measured without claiming that a later converter artifact produced that earlier
guest run.

## What changed in the rendered rooms

### Neow and smooth illumination

Neow retains all actor frames and all **546 transmission-mask frames** at
their converted cadence. Its procedural radiance field is held at clock zero
and cached inside existing room-state space. Transmission, actor motion and
the remaining dynamic lights/fog continue at their normal clocks. The
expensive whole-screen refraction/chromatic-dispersion pass is omitted.
Smooth lighting and fog use a **four-pixel sample grid**; the actor, text,
foreground masks and final destination are not downscaled. Content/room/phase
invalidation and the existing shared scene storage are covered by regression
checks. Nonupeipe uses the shared smooth-light/fog improvement.

For new packs, NWF3 computes this same held radiance field offline. A direct
production C++ shader oracle compared **all 17,485 words / 34,970 bytes** with
the converter, yielding SHA-256
`00aee7c569c4fafd53166fa175188cd6c6abbd1f018fda5d6ab03925f6f229c4`.
Six Neow-only input pages are replaced; the transmission IDs, parameter ID,
body animation and textures shared with Tezcatara are preserved. Retained
payload compaction actually removes the unreachable bytes. NWF1/2 packs
remain supported by the runtime fallback, as exercised by the old-resource
warm tests above.

The verified unified conversion tail saves **162,740 bytes** in the full
profile and **113,558 bytes** in the reduced profile at this stage. Its 11
required data files total 68,052,158 and 27,878,510 bytes respectively. These
are resource subtotals, excluding the launcher, instructions, licenses and
saves; they are not presented as complete installer sizes.

The first reduced NWF3 candidate subsequently failed ordinary guest resource
open: its correctly transferred files had lost the required `NAF1` profile
header word in generic storage writers. The NWF3 adapter now preserves that
word at replacement and compaction boundaries, with a real-writer synthetic
regression. The earlier reduced subtotal above identifies that historical
conversion tail. Corrected reduced was subsequently rebuilt from the proper
pre-NWF3 input with actual producer `cd0d5a7dfbef4238abe1da701c82b7874f9a7b14`,
published whole-tree equivalent `4d75c55f7561711ced681806d924f24c65ca4959`.
Its eleven bilingual files total **27,878,523 bytes**; all 25 files total
**42,163,250 bytes**. All sixteen locales retain the required profile marker.
The ordinary final c814 launcher and eleven required data files passed exact
USB readback, Chinese/English Timeline/Continue/save and normal OS exit.
See [corrected reduced acceptance](ui24-timeline.md#corrected-reduced-edition).
Full-resource Neow/Tez performance and their image comparisons are unaffected.

### Tezcatara's flame, glow and cinders

The three source flame materials share one **9×9 UV grid each**, so a frame
requires 243 full shader evaluations. Bilinear color/alpha interpolation
retains the live source clock, all **237 fire geometry nodes** and their
**506-frame timeline**. The grids borrow 972 bytes from existing foreground
scratch. They approximate the material, not the actor geometry.

Both foreground cinder emitters retain all 24 particles. The 72 background
emitters use staggered one-in-12 original indices, retaining **192 of the
original 1,776 particles in total**. Each selected particle keeps its original
seed/index, phase, inheritance and **60 Hz trajectory**. The simulation does
not slow all particles or invent a substitute random stream. Its density
discriminator uses existing padding; particle State remains 56,844 bytes.

The 90 decorative glow planes are accumulated through the existing four-pixel
soft field after the fire pass. This simplifies their former interleaving
with normal fire layers. Consequently the resulting glow is softer and can
be brighter, and the final lit actor pixels can change. The claim of
unchanged actor source frames is not a claim of byte-identical final lighting.

The first 33×33-grid candidate measured only 8.454 FPS and was rejected as
insufficient. A separate 17×17 diagnostic build then measured about 44.389 ms
of particle simulation, 19.013 ms of cinder drawing and 16.757 ms of glow per
warm frame; the UV grid itself cost only 2.122 ms. Those diagnostic counters
identified the larger work to simplify. They are not the ordinary final FPS
result and are not included in the shipped build.

## Appearance and regression evidence

The native production App and room renderers compared 13 identical clocks
between the original material path and 336/264/192-particle candidates. The
complete 320×240 views at 9,009 and 50,000 ms were also independently inspected.
At those views the final density keeps the actor and UI readable, with a
small reduction in background sparks and softer flame/glow edges. No obvious
grid blocks or extra actor blur were seen. This is finite visual coverage,
not a claim about every possible animation phase.

All 13 isolated actor-input frames are byte-identical. The complete image is
intentionally different. For example, the final App at 9,009 ms changes
13,422 pixels, with mean absolute 8-bit RGB error **0.7601 / 255** and maximum
channel error **214** at a localized effect difference. The unobstructed room
at that clock changes 22,265 pixels, with mean error **1.3909 / 255**. The
maximum is retained rather than implying that a small mean bounds every pixel.

Executed native checks include **28,414** selected-particle trajectory cases,
**76,843** scratch/room-lease checks and **616,971** checks preserving Vakuu's
unrelated full-density default particle path. Production App callbacks confirm
that the sampled glow path is actually active while nested scratch usage is
10,854 bytes, and that the scratch top returns to zero after rendering. The
room union and resident allocations are not enlarged.

## Resource identity bridge and retained limits

Final Tezcatara FPS was measured with the old formal full pack. To establish
exact input continuity with the unified new pack, both physical 11-file
families were independently reconstructed. All **520 relevant decoded image
headers and 2,288,957 payload bytes** compare directly equal: every actor/delta
frame and timeline, all fire parameters and eight source pages, the light
parameters/shared texture, and the cinder parameters/sprite. Their combined
SHA-256 is
`520c6b13a79884b06ac4bacf1bdc55279db2c7b2d9ecf205937a5d14a86e9de0`.
This proves equality of Tezcatara's decoded inputs. It does not assert
identical whole-pack physical I/O or unchanged unrelated NWF3, slash and
Timeline data.

The [machine-readable receipt](ui24-ancient-validation.json) includes both
families' complete required-file sizes and hashes, logical/common hashes,
all measured counter deltas, exact launcher identities and private evidence
receipt hashes. No original asset pixels, assembly bytes or calculator image
are published with it.

The final result is limited to the tested rooms and machine. Other Ancient
rooms are not assigned an unmeasured FPS by sharing a helper. The historical
seven-family Affliction case remains a separate heavy workload and was not
claimed fixed by this task. Cold entry, first-cycle reads and the exact UI24
seed/exit boundaries remain explicit in this report and
[the seed acceptance report](seed-ascii-ui24.md).
