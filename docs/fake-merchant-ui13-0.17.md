# UI13 Fake Merchant source scene and acceptance

Reference: the owner's v0.111.0 / `41cef1ea` PCK, the original custom-room scene,
`NFakeMerchant`, `Character.CreateVisuals`, `NNecrobinderVfx`, and `MegaBone.Hide`.
Original art, assemblies, sampled geometry and converted packs remain private.

## Complete room layers

`src/fake_merchant_ui13.inc` replaces this event's static background with its
own authored scene. It does not substitute ordinary Shop animation or the
characters' Shop atlases. The room draws the background, masked glitter,
three candle lights, two square emitters, the active character's relaxed body,
two ambient lights, 300 star particles, the merchant and foreground cutter in
their source order. The focused merchant uses the source outline skin at the
same clock as its ordinary body. Opening and closing inventory keeps the room
and character clocks running.

All complete Spine periods are sampled at 24 Hz in a single ordered request
with the original physics step. The offline host adapter's bounded limit is
512 samples, allowing the 320-sample Regent loop without truncation or a second
physics warm-up. This limit does not allocate calculator runtime state.

| Layer | Samples per full period | Period |
| --- | ---: | ---: |
| Background and foreground cutter | 144 each | 6 s |
| Merchant ordinary and outline skin | 80 each | 3⅓ s |
| Ironclad relaxed body | 288 | 12 s |
| Silent relaxed body | 168 | 7 s |
| Defect relaxed body | 288 | 12 s |
| Regent relaxed body | 320 | 13⅓ s |
| Necrobinder body and attached head fire | 144 | 6 s |
| Background with source rug hidden | 144 | 6 s |

Necrobinder's attached `flameattach` shader follows the live source slot at
every body sample. The source ready-state disables its unrelated scythe and
hurt emitters; these are not invented during the relaxed loop.

The glitter retains its own 50×50 seamless noise, texture/alpha mask, speed
0.388 and strength 0.058. Lights and stars reuse the Random background's two
original textures through their logical 256×256 views. Their physical storage
is a lossless 320×205 linear arrangement that respects Content's decoder bounds.
Two 30-particle square emitters retain source lifetime, preprocessing, authored
curves, colors, gravity, angular ranges and additive blending. Their bounded
CPU curl field is a calculator adaptation; it is not a claim of identical
desktop GPU particle trajectories. All presentation randomness is separate
from saved gameplay RNG, and square-particle state stays below 4 KiB.

## Ordered eye emission and exact storage

The Regent eye crosses a partially transparent body edge. Flattening it into
ordinary alpha would change its blend with the room. The renderer instead
keeps the original Normal / Add / Normal order inside an 8×6 pixel eye region.
Its single 160×196 sparse atlas stores the body with that region cleared and
three small ordered bands. The emissive band keeps straight color and source
alpha, so rendering does not multiply its edge alpha twice. This remains one
decoded cache entry per frame. Every band fetch is drawn before another image
load can invalidate its storage.

Visible source attachments are examined per sample. The original minion sweat
adds a Normal slot in 62 of the 320 Regent samples; a fixed draw-list length
must not reject or drop it. A source frame without an additive eye would retain
its Normal body and empty emission bands instead of inventing a glow.

Each sequence has one exact invariant image and disjoint moving pixels. Their
union is checked against every original RGB565/alpha frame before packing.
Global exact aliases and retained sparse groups preserve all sampled frames.
The foreground's 144 samples legitimately produce only eleven distinct LCD
images; the actual-resource gate uses independently reconstructed exact source
counts, not an arbitrary minimum animation-density threshold.

Fake Merchant occupies auxiliary IDs 3343–5155: 1,800 dynamic samples, ten
invariant images and three source textures. The 5,011-image base endpoint remains
supported; the final 5,156 endpoint adds the complete 145-row no-rug sequence.
The latter increases the verified English logical pack by only 29,290 bytes
after exact aliasing and compression. Missing this optional suffix on the older
complete endpoint retains its old rug view without making Content unhealthy.

## Canonical room and input behavior

The existing six saved wares preserve the original StartedFight fact, so no
State field, RNG call or save-format change is needed. After the actual Foul
Potion encounter, the merchant disappears. Receiving the genuine Fake
Merchant's Rug selects the source background with its rug bone hidden. The
converter removes only that bone's attachment; it does not erase other pixels
baked into the background.

A completed event continues to display its custom room under the map and
offers its single canonical Proceed when the map closes. It cannot reopen
inventory or the ordinary event reader. Both keyboard Detail and direct
ReadEvent dispatch are covered; unfinished Detail still opens inventory.
Welcome speech retains its original independent timing while the scene moves
behind it. Pixel comparisons isolate the speech against the background at the
same presentation clock instead of incorrectly requiring the room to freeze.

## Actual-resource acceptance

Production runtime: `792904cb5e7f5fcbbab7a5320ee0d54d247591c7`.
Converted resources: `e606108221fd6f8202f61b3e5ae1372970de0e27`.
The resource-only checkpoint has byte-identical production files. The accepted
Fake gate uses test-only checkpoint `21c3cce61e5870f2018d2e3c231f2b5efa938c87`.

The final English and Chinese Fake gates each pass **69,789,393 checks**,
covering all 1,800 source samples, exact static/moving separation, five actors,
live source focus, repeated-render stability, real open/buy/close/map/finished
interactions and controlled post-combat variants with and without the relic.
The older 5,011 endpoint passes **59,710,237 checks** in English. Inline reading
focus and canonical pointer choice pass **55 checks per language**. The full
event/speech gate passes **5,850 English / 5,827 Chinese checks** at test-only
checkpoint `7d4bebdb5049231153ad6b4815d330fa0272fead`, including the exact same-clock
welcome comparison, the real one-gold price change, eight Ancients, revealed
dialogue/history and ordinary events. Earlier animation-density and cross-time
oracle failures remain separate failed receipts; neither required a runtime or
resource change.

The continuous range-reader run passes **22,100 checks** over **14,544 real
App.render frames**: five characters, Regent focus, and two post-fight variants,
each with three 606-frame windows at 33 ms. Every second and third window has
**zero resource reads, bytes and seeks**. The highest observed final occupancy
is 553 image entries and 1,730 4 KiB pages (7,086,080 bytes). This room uses the
existing 11 MiB arena's 10/1 MiB image/tile split; combat and combat-choice
underlays retain the ordinary 5/6 split. No additional full framebuffer is kept.

These are actual-resource correctness and cache measurements, not calculator
FPS. Host third-window means are 1.50–2.10 ms and are not a device performance
claim. Captures include the complete custom room, all five characters,
inventory prices, focus, completed rooms, merchant disappearance and the rug
variant. Post-fight outcomes are explicit controlled fixtures; they do not
establish a naturally played encounter or a three-act victory.
