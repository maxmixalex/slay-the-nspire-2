# Original enemy visual inputs and offline sampling

The visual fact directory is `config/enemy-visuals-0.17.json`. It covers all 109
runtime enemy IDs, including the Legacy Nibbit compatibility alias and the
Architect display model. It records the pinned game's original paths, clip names,
state transitions, skin selectors, scene transforms and constraint parameters.
It contains no image, mesh vertex, original scene source, skeleton binary, game
assembly or runtime binary. The pinned reference remains v0.111.0 / 41cef1ea;
the assembly SHA-256 is
`0861bfa1df347538d932f22d580e75420f08082792eb914e53b4882764acdbe9`.

## Complete private world-pack conversion

`tools/build_world.py` appends the world resource profile to the completed
NSP2/13 pack. It requires that pack's exact recorded SHA-256 and the original
release's PCK/DLL hashes. The source and cache contain distinct evidence: a
source-ready converter is not a completed resource conversion or a visual
similarity result.

```sh
python tools/world_ui_profile.py
python tools/build_world.py --plan
python tools/build_world.py /private/game /private/nSpire2-0.16.nsp.tns \
  --output /private/world/nSpire2-0.17.nsp.tns \
  --font /private/ReferenceCJK.otf \
  --runtime /private/OurNotes.Spine.Runtime.dll \
  --dotnet /private/dotnet \
  --fastnoise-header /private/FastNoiseLite.h \
  --cache /private/world-cache
```

Run a full conversion only from the verified frozen source checkpoint, through
`tools/run_logged.py` as required by `AGENTS.md`. All output, intermediate assets,
adapter assemblies and cache files are rejected if their resolved paths fall
inside the public source repository. No original game executable is run.

The generated public profile currently contains 558 cards, 6,100 strings,
1,170 auxiliary entries and 1,647 animation entries. The new enemy directory
keeps all 732 actually used native clip names as 1,600 skin/phase/move variants,
with 33,210 frames. Three original map-boss loops add 540 frames, and the
three treasure opening/shine queues add 99 frames. With the
unchanged historical images and new portraits and auxiliary images, the plan
has 37,379 images. Even at the 32,768-glyph directory bound, its directory
occupies 1,211,152 bytes of the 2 MiB limit. Actual payload
size remains unknown until all frames are encoded; the 256 MiB file limit is
checked during construction and on the final canonical decode/re-encode.

The converter preserves every historical image payload, card record, string,
auxiliary reference, clip record and glyph payload. Original localization retains
SmartFormat branches and live variable names, while literal `\\n` becomes a real
line break. Glyph coverage includes all text at five sizes and visible characters
from the complete runtime source. Enemy images use complete original periods,
8 fps for loops and 12 fps for actions, with 60 Hz physics integration.
Vantom's charge composites keep the full independent tail-extension period,
including Ink Blot's 0.15-second delay and the body's intervening idle loop.

Cache receipts bind each complete job to input hashes, its actual rendering
code/data dependencies and settings, and verify the payload hash before reuse.
Partial files lack completion receipts. Textures, scene renders, AtlasTexture
resources, enemy bounds, enemy frames and map-boss frames have separate dependency
scopes. An enemy scope includes its complete numeric model row. Core repairs or
additional map/UI catalog entries do not invalidate unchanged enemy geometry;
the final manifest and source-change guard still record the complete source
fingerprint. Bounds jobs and completed raster jobs resume independently.

Migration from schema-1 caches is restricted to the exact converter recorded in
checkpoint `5f5f652f8e38e0a5ca3d9036fe6a1fd7d67228e7`. It verifies the old producer
hash, unchanged renderer function and dependency hashes, settings, original
receipt identity, payload length and SHA-256. A new receipt records its original
identity and key. Old files remain unchanged; a mismatched job is rendered again.

`config/world-map-ui-0.17.json` appends original map backgrounds, outlines,
ancient/boss icons and atlas regions. Its three boss animations retain the native
clip duration, default skin, authored scale/anchor and RGB shader mask channels.
The UI applies the original act and visited colors at presentation time. The
converter checks every frame against the declared canvas and rejects overflow
instead of silently clipping or changing scale. Integer frame intervals are
stored separately from actual duration so loop playback can use the true period.

Every ordinary model receives one measured scale and pivot across all clips,
skins and conditional phases. Source-driven target controls use the explicitly
recorded original single-player/single-enemy placement before fitting the actor
to the calculator stage. Multi-enemy placement remains a calculator layout
adaptation to review visually. Kaiser uses its exact shared 320 × 218 scene
canvas, original camera placement and separate back/front planes; it is not
duplicated for the two arm targets.

The generated `world_enemy_assets.hpp`/`.inc` API exposes native states,
transitions, ordered move triggers, conditions, skin weights and clip durations.
Unknown facts fail condition evaluation instead of becoming zero. A missing
animator trigger retains the current state. The app may query a directory row
with `world_enemy_clip_directory` for the actual baked duration. Static scene
diagnostics retain dynamic-node, particle and shader limitations; neither these
metadata counts nor passing asset-free tests establishes 95% visual parity.

## Runtime dependency

The independent `tools/SpineSampleAdapter.cs` uses a separately supplied, licensed
Spine 4.2 C# runtime. Both the Python caller and the adapter require the reviewed
runtime SHA-256:

`0e05031a0377bea228e6419572f171496cb77d58bfdc9129c4607d54601ba3ec`

The runtime assembly name is `OurNotes.Spine.Runtime`; its private DLL is not
distributed with the repository. The adapter requires a local .NET SDK version
10 or newer and does not download packages. Set `NSPIRE_SPINE_RUNTIME` and,
optionally, `NSPIRE_DOTNET`, or pass the same paths as constructor arguments.
Build outputs, extracted skeleton/atlas inputs and generated geometry belong in
the caller's private work directory. Changing the runtime hash is a reviewed
dependency change, not a way to bypass a failed conversion.

The complete runtime supplies path, physics, IK and transform constraints, bone
inheritance, skin activation, weighted/deformed meshes, attachment timelines,
animation tracks, clipping and draw order. Source inspection alone is not a
passing image comparison. Initial read-only inventory successfully loaded all
102 distinct skeleton resources used by these models and their child/shared
layers. Their binary versions are 4.2.38 or 4.2.43. Among these resources, 79 have
IK constraints, 91 transform constraints, 25 path constraints, 20 physics
constraints and three have two-color slots. These counts describe resource
features, not independent fidelity tests.

## Python interface

```python
from pck import Pack
from world_enemy_assets import SpineSampler, bake_enemy, manifest

pack = Pack(private_pck)
row = next(r for r in manifest()['models'] if r['model'] == 'BowlbugEgg')
with SpineSampler(pack, runtime=private_runtime, dotnet=local_dotnet,
                  work_dir=private_work) as sampler:
    metadata = sampler.metadata(row)
    frames = sampler.sample(row, 'idle_loop', [0, 0.1], skins=['cocoon'])
    layers = sampler.draw(row, frames[0])
    images, info = bake_enemy(pack, row, 'idle_loop', sampler=sampler,
                              frames=16, height=88, max_width=160, cache={})
```

`metadata(resource)` returns skin names, animation durations/event cues/timeline
types, and setup constraint facts. `resource` can be a `.tres` resource path, a
`.skel` resource path, a complete visual-manifest row, or a known skeleton
basename. A `.tres` path is preferred for scene resources outside the enemy
directory. It resolves the actual referenced skeleton and atlas; textures are
looked up relative to that atlas, so duplicate page basenames do not collide.

`sample(resource, clip, times, *, skins=None, setup_tracks=(), tracks=(), track=0,
loop=False, physics_step=1/60, bone_drives=())` returns a list of raw frame dictionaries.
`setup_tracks` are installed before the primary clip. `tracks` are installed
after it. Each track command has a `track` index, `clip`, `loop` and optional
`queued`/`delay` fields. A setup track's optional `time` starts at that exact
native clip time; this preserves the other Kaiser arm's terminal death pose.
An empty clip selects an empty track. The sample times
must be ordered and finite, between zero and 120 seconds, with at most 256 samples
per request. Physics advances in bounded steps, independently of the output
frame rate.

`draw(resource, frame, transform=None)` produces the existing rasterizer's
`(xy, uv, triangles, texture, tint, blend)` tuples. The texture is an RGBA float
array. It preserves each slot's blend mode, handles two-color tinting and
normalizes premultiplied atlas pages to straight alpha. A scene renderer can
compose these tuples directly, keeping additive/multiply/screen slots separate.
`frame_layers(pack, resource, clip=None, time=0, ..., sampler=...)` is the one-frame
convenience wrapper and also returns bone and slot matrices.

`bake_enemy(pack, model_row, clip, texture=None, *, frames=16, height=88,
max_width=160, cache=None, sampler=None, skin_variant=None, visual_state=None,
measure_only=False)`
returns `(PIL_RGBA_images, metadata)`. Metadata includes the original duration,
frame period, pivot, dimensions, chosen skins, shared scale and dependency hash.
`measure_only=True` returns all-frame geometry bounds and reference-pose bounds
without rasterizing. The full builder measures every skin, move and conditional
variant before selecting one common model layout. Its `visual_state['layout']`
contains the shared `scale`, `origin`, `size` and measured `bounds`, and applies
unchanged to every clip. This retains both scale and pivot across actions.
Passing only a cache retains a common scale but does not establish a common
pivot; use the measured layout for runtime animation. The skin variant
is a skin name or ordered list of component skin names. The visual state can
specify `setup_tracks`, `tracks`, a native move key in `move`, `base_clip`,
`track`, an explicit scale and per-child clip/start selections.

## Coordinate and frame protocol

The adapter sets Spine's global `Bone.yDown` exactly as the original Godot
module does. It solves constraints in that coordinate system, then converts
emitted vertices and full bone matrices to x-right/y-up coordinates. Setting
`yDown` before solving matters: reflecting only the final geometry can produce
a different constrained pose.

A full `matrix` contains `[a, b, worldX, c, d, worldY]`. A Godot scene uses y down:
multiply vertices by `F = diag(1, -1, 1)` before the scene's node matrix. The
separate `node_matrix` on both bones and slots is already in Godot y-down
coordinates, relative to the SpineSprite. It reproduces the original Follow
node's transform reconstructed from world rotation and axis lengths, including
its deliberate omission of Spine shear. Use it directly for a following Godot
node, then apply the child's local transform. The adapter captures these Follow
transforms at the original signal boundary, before the second world-transform
pass needed by Drive nodes. A serialized Follow position is a preview value.

The original sprite places every direct SpineSlotNode immediately after its
slot's mesh in current draw order. It overrides that node's serialized
`show_behind_parent` flag. Following nodes have their own visibility; a missing
slot attachment or an inactive bone does not automatically hide their children.
The raw frame retains slots even when no mesh is emitted for them; its explicit
`draw_order` lists all slot names in their current order. Mesh placeholders are
created when the skeleton resource is assigned, before normal serialized child
nodes are appended. Explicit scene child indices still affect that initial
order. During each sort, the sprite captures a mesh index once per slot and
moves every node for that slot to the same following index. The current 109
model scenes contain no duplicate SlotNodes on one slot.

Each raw frame has `time`, `draw`, `slots` and `bones`. Each draw item contains
the slot name/index, attachment and atlas region name, page filename, flat world
vertices, flat UV pairs, triangle indices, RGBA tint, optional dark RGB tint,
premultiplied-alpha flag and blend enum. Blend values are normal=0, additive=1,
multiply=2 and screen=3. UVs address the complete atlas page, including its
original packing rotation and trim; they do not address an unpacked sub-image.
The runtime clips meshes before emission and rejects non-finite geometry.

The adapter uses one JSON request and response per line over stdin/stdout. A
request has `schema_version: 1`, `op: metadata|samples`, local `skeleton_path`
and local `atlas_path`, plus the sample parameters above. Responses have
`ok: true, result: ...` or `ok: false, type: ..., error: ...`. The process remains
alive for subsequent requests and caches immutable skeleton data.

## Manifest interpretation

| Field | Meaning |
| --- | --- |
| `entries` | Six fixed resource roles: idle, attack, cast, hurt, die, revive |
| `states` / `transitions` | Original animator graph, clip chaining and branch predicates |
| `moves` | Native move node/key, originating method, self animation triggers and direct clips |
| `tracks` | Explicit secondary animation-track calls, with method and target scope |
| `skins` | Fixed, random composite or named-slot choices; declaration order and duplicate weights retained |
| `layers` | Child skeletons, bone/slot anchors, textures, particle nodes and driver paths |
| `scene_controls` | Scene bones configured for external driving, with audited `binding` rules |
| `constraints` | Setup constraint facts, separate from each clip's animated timeline types |
| `clips` | Actual skeleton animation catalog, with `used` marking declared animator/track usage |

`playback: hold_current` means that the original does not play a new clip for
that trigger. The stored fixed-slot clip is only a directory placeholder; the
presentation layer must not play it. Some original default animators name clips
that their skeleton does not contain. These are explicitly listed in
`missing_declared_clips` and retain the original visible animation behavior.

Several models need more than six role entries:

* Test Subject uses three phase-specific animation families and knockout/revive
  chains. Its front/back burn effects are real child skeletons and start at the
  original `start_burn_vfx` animation event, not at the beginning of the move.
* Bowlbug Egg combines the bowlbug `cocoon` skin with a separate Tough Egg
  skeleton using `egg1`, attached at the original slot and draw-order location.
* Fat Gremlin has normal and `_no_bag/` clip families selected when its animator
  is created. Tough Egg switches between egg and hatched animation families.
* Lagavulin's eye track, Queen/Soul Nexus writhe tracks and Vantom's charge track
  are independent of their primary animation. Preserve their explicit tracks.
* Crusher and Rocket are invisible creature markers for a single Kaiser Crab
  background skeleton. Body, left arm, right arm and reaction tracks share one
  scene. `bake_enemy` rejects an accidental independent creature rendering;
  shared scene conversion can explicitly pass `shared_background: true`.
* Two Tailed Rat requests `barnacle1`/`barnacle3`, whereas the pinned skeleton
  supplies `barnacles1`/`barnacles3`. The original nullable skin-composition call
  ignores the missing requests. The manifest retains this fact and its head
  choices; it does not silently correct the game's skin names.
* Decimillipede attacks also use a procedural scene tween. Its original Hit
  trigger returns to the idle animation. Both facts are recorded explicitly.

The skeletal baker composes original skins, child skeletons, scene transforms,
color modulation and runtime-solved animation. The manifest also inventories
Godot-driven bones, procedural motion, particles and material layers. Those
scene effects require the scene/presentation integration; a successful skeleton
sample does not establish their execution or a 95% UI fidelity result. Keep
source checkpoints, full conversion runs and visual comparisons separate in the
work-state ledger.

## External bone controls

`bone_drives(model_row, clip, times, visual_state=None, *, noise=None)` resolves
all 39 Drive nodes across 30 models. It returns the `bone_drives` input for
`SpineSampler.sample`. Each entry has `bone` and a six-component `matrix` in
Godot y-down coordinates relative to the original SpineSprite. Optional ordered
`keyframes` contain `time` and `matrix`; linear interpolation is the default and
`hold: true` selects step interpolation. `enabled: false` disables that entry.
The adapter rejects missing bones, non-finite transforms and unordered times.

`bake_enemy` builds these inputs automatically. The visual state accepts original
Godot coordinates in `creature_position`, `creature_transform` and `targets`,
plus `creature_local_position` for the four robot fall controls. A target is a
2D global position or a dictionary containing `position`. Target-dependent
controllers change only at their original move or animation-event boundary.
No player location is guessed when a target list is absent. The Tunneler and
Torch Head Amalgam retain their original explicit empty-target fallback.

`node_overrides` supplies known original node properties carried from a prior
pose. Explicit `bone_drives` replaces generated entries by bone name. This is
needed when a target controller remains at its last attack position or an
external node transform changes between clips. `cached_global_positions` and
`initial_creature_transform` preserve Ceremonial Beast's Ready-time world
positions when its current transform changes.

| Binding | Original behavior supplied by the helper |
| --- | --- |
| `scene_static` | The exact scene control and ancestor transforms |
| `target_global_x` | Selected target X plus the model-specific offset times Visuals scale X; current global Y retained |
| `target_local_delta_x` | Fuzzy Wurm Crawler's direct local X displacement from creature to target |
| `target_global_xy` | Myte or Snapping Jaxfruit's target point at its cast event, with the original vertical offset |
| `bot_fall` | Local Y = `(125 - creature_local_Y) / body_scale_Y`, with local X reset to zero |
| `clam_growth` | Initial scale 0.1; each grow event adds up to 0.2, capped at 1.5, after a 0.5-second delay and a 0.5-second Elastic Out tween |
| `shrink_scale` | Vantom's skeletal scaling control, 1 or 0.5, with its 0.75-second Sine In Out transition |
| `cached_global_position` | Ceremonial Beast restores the original cached world point at plowStart/plowEnd |
| `decimillipede_noise` | The original two-axis Perlin motion and optional whole-encounter attack shake |

Carry Sewer Clam's current scale through `coral_scale`; the default is its
Ready-time 0.1. Vantom accepts `shrink`, optional `shrink_from` and
`shrink_started`. These controls do not infer visual state from Strength or the
currently selected intent. Decimillipede accepts `driver_elapsed`, an optional
existing `strike_offset`, and `decimillipede_strike: true`. Its three attack move
keys also enable the strike. Every segment must receive the strike when any
segment attacks. The default attack bake lasts 1.25 seconds, while the original
idle clip and noise continue. `duration_seconds` explicitly controls a bake's
sampling span; output metadata keeps both the sampled and original clip durations.

### Decimillipede noise dependency

The PCK header identifies Godot 4.5.1. The original driver uses Godot's
FastNoiseLite 1.1.0 implementation with Perlin noise, seed 0, frequency 1, FBM,
five octaves, lacunarity 2, gain 0.5 and weighted strength 0. Its one-dimensional
noise call is the library's two-dimensional call at `(x, 0)`.

Supply the public Godot dependency locally with `NSPIRE_FASTNOISE_HEADER` or
`SpineSampler(..., fastnoise_header=...)`:

[Godot 4.5.1 FastNoiseLite header](https://github.com/godotengine/godot/blob/4.5.1-stable/thirdparty/misc/FastNoiseLite.h)

The reviewed file has git blob
`fb6dbcb92ad1120de9b1ac728bd5dffd38f9be9d` and SHA-256
`38b24b9b04aa5e9f1e63336d4acbbbb73b11f15b697bb08a25f5ec0c6c274901`.
It is MIT licensed upstream. The repository includes only an independent API
caller; the header and its license remain together in the private dependency
copy. A local C++17 compiler builds this small caller in the private work
directory with wrapping integer arithmetic and without floating-point
contraction. No package or source is downloaded automatically. Models without
these drivers do not require this dependency.

### Runtime animation predicates

`enemy_visual_fact(state, slot, key, value)` exposes the manifest's original
animation conditions. Boolean facts are 0 or 1; `Respawns`, `Heist` and the
Rocket arm phase retain numeric values. The live condition bits and the latest
actual death snapshot use existing saved enemy data cells. In particular,
selecting a reload, wake or charge intent does not claim that the move has
already executed. Damage-induced wake/stun, actual move start/end and death
update those conditions at their respective continuation boundaries.

`visual_state['facts']` can supply these saved values to the offline baker.
Manifest `phase_tracks` maps `IsShellAwake` to Lagavulin's closed/open eye loop
and `tail_charge` to Vantom's four stable tail-charge tracks. The eye state
persists after healing above half health. Original transition clips remain in
the catalog. Vantom's move-specific charge-up tracks apply to the charge-up
segment, after its attack or buff; the baker does not overlay the later charge
on the preceding attack. Death samples apply original `AfterDeath` track
changes, including Queen/Soul Nexus's empty overlay and Lagavulin's dead eyes.
Explicit `setup_tracks` and `tracks` remain available for full presentation
timelines; they take precedence over the derived stable setup.

The resolved Drive inputs, full runtime constraints and true child-skeleton
composition remove the earlier unsolved skeleton-control gaps. Particle
simulation, arbitrary material shaders, presentation timing and complete UI
comparisons remain separate responsibilities of the scene and presentation
pipeline. These interfaces do not imply a measured 95% fidelity result.
