# Neow cloud and grain particle implementation

The converter reads the pinned original Neow scene and appends one 4,608-byte
parameter view at image 59,731, auxiliary 7,226. This extends the complete catalog
to 7,227 auxiliary entries and 59,732 images. Its three cloud emitters reuse the
four unchanged 200×200 cloud alpha masks already present for Nonu. The ten CPU
stars use the original 32×32 grain alpha, without reducing its dimensions.

The cloud implementation preserves all fifteen particles, the ten-second lives,
one-hundred-second preprocessing clock, source gravity, animated scale, original
angle curves and additive color gradient. In particular, the source angle curve
multiplies the initial fifteen-degree angle; this gives several complete turns.
The CPU stars retain their five-second lives, original emission rectangle,
initial velocity, gravity, three continuous gradient keys and nearly-unit node
scale. Their source additive material is explicitly checked by the converter.

These force-only paths use the closed form of the 30 Hz semi-implicit source
integrator and interpolate adjacent states. Godot uses 30 Hz to preprocess a
particle node whose `fixed_fps` is zero. Its subsequent variable frame history
and random history are not recorded in the original assets; the calculator uses
deterministic birth seeds and the same integration rate. This is a portable
representation of the authored emitters, not recovery of a particular GPU run.

Rendering follows the original scene order: three bone lights, the three cloud
nodes, ten grain stars, four static lights, vignette, and the last three lights.
The separate post-processing implementation runs after those layers. In the
reduced profile the existing full-room cache records the complete first pose.
No runtime particle allocation or persistent cache capacity is added. Borrowed
parameter views are consumed before a shared cloud-image request can evict them.

The focused tests cover independent force integration, continuous gradient
interpolation, lifecycle clock overflow, typed catalog boundaries and malformed
data regions. Actual full-scene Canvas, ARM allocation and supplied-firmware
performance remain separate final integration gates; these numeric tests do not
establish a calculator frame rate.
