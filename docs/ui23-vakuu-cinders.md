# Vakuu cinder source candidate

Vakuu's first visible particle node, `sparks big`, uses the existing source
cinder physics and sprite. This extension adds one 64-byte typed geometry
image and no texture, curve, particle-pool, or framebuffer capacity. Its
auxiliary/image endpoint is 7226/59731 after the Neow-post endpoint
7225/59730. The new parameter root explicitly references the existing Tez
sprite 59710 and parameter image 59711.

The original PCK check proves all 3,072 bytes of the Q20 orbit/scale/alpha
lookup grids equal the existing Tez source table, and the decoded 24×24 sprite
has the same source hash. The original node has 24 particles, a seven-second
life with 0.5 lifetime randomness, six seconds of preprocessing, fixed60Hz,
box emission20×20×1, initial speed100–200, orbit−1–3, gravity(100,−20), and the
authored hue/scale/alpha properties. Its signed source affine projection,
including skew, is retained. Local coordinates make inherited emitter motion
zero because this particular source node is stationary.

The existing scene-owned cinder state advances only its first24 records in
Vakuu mode. Its mode flag occupies existing metadata padding; the state
remains56,844 bytes. The other1,752 records are untouched. The existing scene
owner binds content/language, run seed, room entry, floor and event; changing
between Tez and Vakuu invalidates state. Reduced mode uses clockzero, then
the completed-scene cache replaces the union owner and holds the resulting
entry pose. No presentation code consumes game RNG.

The draw is inserted after Vakuu's opaque static plate and before both first
lights, all turbulent specks, the later light, visible CPU lights and final
stationary sparkles. The same cinder quad function is generalized over a
geometry adapter, preserving Tez's established path.

Focused validation passed616,971 lifecycle/guard/capacity checks, including
wide clocks, seeks, exact current-lifecycle reconstruction and all untouched
records, plus an actualApp syntax build. Source randomness is deterministic;
the original GPU random history is not recovered. Full production allocation,
real-source full/reduced Canvas captures, final typed guard integration and
combined physical publication remain acceptance gates.
