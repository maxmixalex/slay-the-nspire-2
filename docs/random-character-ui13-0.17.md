# Random character background: source water and particles

UI12 retained the Random selection background as one static projected gradient.
UI13 now implements the water material and all three decorative emitters from
`scenes/screens/char_select/char_select_bg_random_character.tscn`. The old image
remains only a compatibility fallback for earlier private resource packs.

## Source correspondence

The source `TextureRect` uses a full 64×64 cubic seven-stop GradientTexture2D.
Its untrimmed texture is retained before projection, so water sampling near the
visible boundaries does not clamp to an already cropped 320×240 screenshot.
The complete anchored Control/pivot/AnimatedBg transform is applied: the source
2634×1236 rectangle has LCD scale `0.24158095238`, origin
`(-196.573542857, -30.922933333)`. Sampling uses destination pixel centres.

`shaders/vfx/vfx_water_reflection.gdshader` samples repeatable noise at
`UV - TIME*speed`, then shifts both source coordinates by
`(noise - 1)*strength/2` wherever mask alpha exceeds 0.1. Random uses source
speed 0.023 and strength 0.1. The source 2048×960 underdocks mask is entirely
opaque; the converter checks every alpha texel before omitting its redundant
storage. Shader TIME follows the presentation clock, including the original
3600-second rollover, independently of the selected panel's entrance clock.

The converter evaluates Godot 4.5.1 FastNoiseLite OpenSimplex2, seed0, frequency
0.0259, FBm with five octaves, lacunarity2 and gain0.5. The 10×5 noise texture
uses the source seamless quadrant shuffle, a 0.093 blend skirt, normalization
and integer grayscale blend. Its R8 samples are transported in alpha, with
white RGB, so RGB565 storage does not reduce the noise channel to five bits.
The shared sampler reads this channel explicitly. The separate Fake Merchant
material uses the same code with its own 50×50 noise and source parameters.

The three source particle systems are retained in their original order:

| Emitter | Source count and lifetime | Motion and appearance |
| --- | --- | --- |
| `light_small2` | 1, 3s | Source `light.png`, scale20 times its three-point scale curve, 3.1032rad node rotation, original gold modulation and gray ramp, additive blend. The original script only chooses an integer 0–5s preprocessing time. |
| `ash2` | 20, 15s, 15s preprocessing | Source `dot.png`, 800×500 rectangle emission, gravity(20,−10), 0–0.03 orbit velocity, 0–1 radial acceleration, 0–353.6° rotation, scale1, original green alpha0.137255. |
| `ash3` | 20, 15s, 15s preprocessing | Same source motion and emission, scale0.1–0.3, original green alpha0.431373. |

Both ash emitters retain the source alpha ramp from zero to full at lifetime
fraction0.178337, without an added fade-out. All 41 particles are represented.
Particle creation and warmup use a presentation-only seed, and do not advance
the run's random generators or modify its serialized state.

## Bounded LCD implementation

The water shader is evaluated at the visible LCD pixels with fixed-point
coordinates and bilinear filtering. Repeat sampling wraps before the pixel
loop's index operations, avoiding integer remainder calls for normal texture
coordinates. Noise filtering computes only its scalar channel. No full-frame
animation is stored. The four new images occupy auxiliary3339–3342. The full
original light/dot images retain all256×256 source texels. Their flat texel
sequence is stored in a320×205 physical image, with64 transparent trailing
padding texels, to respect Content's unchanged320×240 decoder bounds. A logical
256×256 sampler view preserves the original UVs without cropping or resampling.
Dense and indexed sampling use the logical flat index directly; only sparse
storage requires mapping that index to a physical row.

The forty moving ash particles use less than2KiB of presentation state. Their
force, clockwise orbit and velocity integration follow the source update
order. Q10 positions and steps of at most34ms implement the source's30Hz
preprocessing with bounded integer work; live updates use elapsed presentation
time. Large gaps can reconstruct the current bounded lifetime instead of
retaining an unbounded timeline. LCD RGB565 filtering, integer integration and
the independent cosmetic seed are explicit adaptations; the implementation
does not claim per-pixel equality with a desktop GPU particle simulation.

## Validation contract

`random_character_ui13_regression` includes an independent floating-point
water-material oracle, ordinary and sparse textures, mask threshold, repeated
noise and TIME rollover; it also checks the source force/orbit update order,
particle warmup, scale ranges and the anchored matrix. With the real private
pack it checks all four image dimensions, the complete water frame against
the independent source equation, animated frame changes, deterministic
same-time drawing, selection-screen output and canonical save-byte stability.
It can write real native frame captures for visual review. Synthetic images
exercise only sampler behavior and are not evidence of original artwork.

The source-free Python cases independently check literal seamless edge/corner
values and lossless R8 transport. At immutable checkpoint
`2b58ee31eb5ef6674ce95f02ad2eb9694a98a041`, all three Python cases and the
4,534-check independent native sampler/particle gate have closed passed journals
with `source_changed=false`. The matching137-test registered native suite also
passes. The private Random binary was independently linked from a hash-verified
copy of the matching core before its shared build directory was reused.

The new actual-source-texture gate and captures await the final matching
resource directory and runtime profile. Its result is recorded separately
from these synthetic checks and from device measurements; a conversion probe
or syntax check is not counted as completed actual-resource acceptance.

Primary engine references: [Godot4.5.1 Noise](https://github.com/godotengine/godot/blob/4.5.1-stable/modules/noise/noise.h),
[Noise normalization](https://github.com/godotengine/godot/blob/4.5.1-stable/modules/noise/noise.cpp),
[FastNoiseLite](https://github.com/godotengine/godot/blob/4.5.1-stable/thirdparty/misc/FastNoiseLite.h),
and [CPUParticles2D](https://github.com/godotengine/godot/blob/4.5.1-stable/scene/2d/cpu_particles_2d.cpp).
