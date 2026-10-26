# Architect original attack VFX — UI13 implementation and resource acceptance

Reference: the owner's v0.111.0 / 41cef1ea PCK and its matching C# command
behavior. This stage adds the actual thirteen character attack scenes and the
terminal `NFireBurstVfx`, alongside the existing hit sparks and damage numbers.
Original inputs, sampled images and conversion caches remain private. The full
conversion and actual-resource native acceptance are complete; the final
section records their identities. These results do not claim Firebird or
physical-calculator acceptance.

## Converter and runtime directory

`tools/architect_attack_assets.py` exports `source_assets(pck, cache, runtime=...,
dotnet=...)`. It returns auxiliary rows in directory order, compatible with the
UI13 parent append. All images are sparse RGBA/RGB565 and 320×240. Common density
is 4/21 source pixels. Normal images retain ordinary alpha; additive emission and
screen displacement have their own planes. Existing auxiliary IDs are unchanged.

| Effect | Normal first | Frames | Other plane | Lifetime ms | Sampling |
| --- | ---: | ---: | --- | ---: | --- |
| Blunt | 2507 | 38 | — | 1250 | 30 Hz |
| HeavyBlunt | 2545 | 66 | warp 2611 | 2200 | 30 Hz |
| Slash | 2677 | 4 | — | 268 | 67 ms per original texture |
| BloodyImpact | 2681 | 90 | — | 3000 | 30 Hz |
| RockShatter | 2771 | 90 | — | 3000 | 30 Hz |
| DaggerSpray | 2861 | 30 | — | 1000 | 30 Hz |
| FlyingSlash | 2891 | 10 | — | 334 | 30 Hz |
| DramaticStab | 2901 | 60 | — | 2000 | 30 Hz |
| DaggerThrow | 2961 | 60 | — | 2000 | 30 Hz |
| Lightning | 3021 | 5 | — | 334 | 15 Hz, original keys |
| Scratch | 3026 | 5 | — | 167 | 30 Hz |
| StarryImpact | 3031 | 60 | warp 3091 | 2000 | 30 Hz |
| Thrash | 3151 | 60 | add 3211 | 2000 | 30 Hz |
| FireBurst | 3271 | 60 | — | 2000 | 30 Hz |

Total: **824 rows**, `[2507,3331)`. Every plane is contiguous within its effect.
Use source lifetime to expire a sequence; `frames / fps` is not the Blunt free
time. The 334/167 ms entries round the original one-third/one-sixth-second end
up to the runtime's integer clock. Empty source tail samples are retained and
may be deduplicated by the pack builder.

The ordinary pivot is **(160,120)**: draw at creature center minus that pivot.
FlyingSlash alone uses **(245,120)** because it enters from the left and targets
only the Architect at x245. This preserves its visible incoming trajectory
within the existing 320-pixel image limit. Lightning keeps the common pivot
because its effect can also target the player. FireBurst uses the common pivot
at `GetBottomOfHitbox()`, with the original default `#ff8b57` tint and scale 1.

## Source behavior preserved

- `NVfxParticleSystem` starts descendant emitters and frees at its scene
  `_lifetime`. The converter reads their original count, lifetime randomness,
  speed, explosiveness, transforms, texture, color ramps and scalar/XYZ curves.
- HeavyBlunt starts its anticipation at zero and its exported impact emitter
  list at 200 ms. Its original command frees at 2200 ms. StarryImpact and
  FireBurst restart their exported emitter arrays and free after 2000 ms.
- Slash is the four original `NSpriteAnimator` textures; its 15 Hz command
  rounds the delay to 67 ms. A fixed cosmetic seed chooses an integer angle
  from the source [0,25) range. Lightning preserves all five AnimationPlayer
  texture and position keys, 1/15 second apart.
- FlyingSlash and Scratch use the owner's reviewed Spine 4.2 runtime with the
  original clips, skins, transforms, texture pages and geometry. Their source
  durations are 0.33333334 and 0.16666667 seconds.
- GPU material evaluation covers the authored grayscale LUT, general/row
  flipbooks, heavy impact erosion, polar ring, poof distortion and screen-warp
  shaders. CanvasItemMaterial flipbooks retain their separate quad sizing.
- DaggerSpray retains the dagger→trail→spark subemitter chain, source capacities,
  inherited velocity and end emissions. A full child pool rejects a new spawn
  instead of killing an older particle and losing its end spark. Its trail
  maps one original texture continuously across the source history strip.
- RockShatter retains the original invisible LightOccluder floor polygon,
  particle collision radius, friction and bounce. Thrash's hidden `fist2`
  remains hidden. Rendering follows source z and behind-parent order.

The independent CPU process equations were checked against the public Godot
4.5.1 sources:
[ParticleProcessMaterial](https://github.com/godotengine/godot/blob/4.5.1-stable/scene/resources/particle_process_material.cpp),
[GPUParticles2D](https://github.com/godotengine/godot/blob/4.5.1-stable/scene/2d/gpu_particles_2d.cpp),
and [particle allocation](https://github.com/godotengine/godot/blob/4.5.1-stable/servers/rendering/renderer_rd/shaders/particles.glsl).
This includes speed-squared friction, additive emission intensity, current-age
animation/rotation curves, source align-to-velocity and inactive-slot allocation.

## Explicit CPU/LCD mappings

The converter integrates at 60 Hz and stores 30 Hz samples, while keeping every
authored sprite frame switch. A separate fixed cosmetic seed replaces unknown
GPU/System.Random state. The GPU turbulence vector is a deterministic CPU curl
field; authored strength, scale, influence/life curve and velocity response are
retained. A history strip and polygon-circle collision evaluate the original
trail/occluder geometry on CPU. These adaptations do not claim desktop GPU pixel
equality or consume the canonical RNG.

The screen-warp plane stores **R5 − 16 = dx, G6 − 32 = dy**, in source framebuffer
pixels; B5 is zero. Alpha marks a nonzero displacement. Draw order is warp,
normal, add. The runtime samples an immutable source pixel at `(x+dx,y+dy)`.
Offsets already include the source distortion texture, color alpha, radial
direction and SCREEN_UV scale. They are rounded to integer LCD pixels. Local
normal/add layers preceding the source warp are displaced during baking too.
The converter asserts `abs(dy) <= 15` for the runtime's 32-row readback window;
it never silently truncates a larger vertical warp.

## Executed bounded checks and remaining gate

- Python compilation passed for both independent converter modules.
- All fourteen effects passed a two-frame PCK conversion: **34 sparse image
  rows**, including the additional warp/add planes, with the original Spine
  runtime. The first attempt exposed Blunt's CONSTANT color gradient; the
  corrected sampler retains that source interpolation mode.
- Two later sample times for all ten GPU families produced nonempty distinct
  effects and real child emissions. Extra focused two-frame checks exercised
  DaggerSpray's continuous strip and RockShatter's original floor collider.
- Measured warp in these bounded checks was at most **8 pixels horizontally /
  6 vertically** for StarryImpact, and **4 / 3** for the sampled HeavyBlunt
  impact. These are sampled observations; full conversion still validates all
  frames and records maxima in its private `source-report.json`.

Full sampling must run from a remotely verified source checkpoint with
`tools/run_logged.py`. Invoke `architect_attack_assets.source_assets` from the
unified append, or use its CLI with `--pck`, `--cache`, `--runtime` and `--dotnet`.
The development `--max-frames 2` output is partial evidence and must not be
packaged as the complete 824-row family. The final pack, actual-resource gates
and Firebird behavior are owned by the parent integration stage.

## Completed full conversion and actual-resource acceptance

The full source conversion closed at `143d510`, generating all 824 attack rows
alongside 199 reading and 80 trail rows. It did not package the two-frame probes.
The final `6e40566` sixteen-edition conversion includes this complete family and
the eight subsequent Doom textures, ending the auxiliary directory at 3339.
Model IDs, source clip frame counts and clocks remain intact.

The production `d5f9a3c` runtime, final UI13 resources and independent `b96bd0d`
test oracle pass **84,365 checks per locale** in English and Simplified Chinese.
Five characters × Normal/Fast × two locales give twenty exercised combinations.
Independent source anchors and literal attack/hurt frames are checked together
with the complete effect composite. All opaque actor texels compare exactly;
death layer order, original text scrolling and unchanged canonical saves pass.
All thirty captured images were fully decoded and reviewed.

The final large-effect oracle samples the already alpha-composited background
for displacement. Requiring at least twenty actor pixels to remain uncovered
was an invalid assumption when StarryImpact legitimately covers the actor;
the final comparison retains the full composite instead of ignoring the effect.

This is actual-resource native rendering, not a desktop-GPU pixel-equivalence
claim. The CPU noise, antialiasing, particle and displacement adaptations above
remain explicit. The exported Architect-entry save is a controlled prior-room
result fixture and is not proof of a naturally completed three-act run.
See [`rooms-ui13-0.17.md`](rooms-ui13-0.17.md) and the aggregate
[`ui13-lcd-followup-0.17.md`](ui13-lcd-followup-0.17.md) for complete acceptance scope.
