# Shared energy and star costs — 0.16

This source audit uses the pinned original v0.111.0 / `41cef1ea` assembly. The method identifiers below identify original methods; the assembly and private disassembly remain outside the repository. The new implementation and regression cases described here are authored and awaiting the next pushed, frozen validation checkpoint.

## Ordered local changes and global powers

`LocalCostModifier.Modify` (`06005283`, Add branch `0044–004c`) adds the raw amount without clamping. `CardEnergyCost.GetWithModifiers` (`0600521d`) evaluates the complete ordered local modifier list, calls the global cost hook, and applies `Math.Max(0, value)` at the end (`00c3–00c5`). A later Set masks earlier additions; additions after that Set remain effective. The shared implementation retains each module's accumulated additions and the accumulated values captured by the relevant temporary or combat Set.

`Hook.ModifyEnergyCostInCombat` (`06004cc7`, `000c–001a`) skips both global passes if the local subtotal is negative. Borrowed Time (`0600244c`) adds its amount in the ordinary global pass. Corruption, Free Attack, Free Skill, Free Power, Veilpiercer and Void Form apply their fixed energy override in the late global pass. Consequently Rocket Punch's `2 − 3 + 1` costs zero; an ordinary global addition does not revive a negative local subtotal; and a late free power still makes a nonnegative subtotal free after Transfigure or Borrowed Time. Energy X returns before modifiers.

The selection preview uses the same global pass after its prospective local Set. With Borrowed Time active, a selection that sets the local energy cost to zero can correctly preview one energy. Touch of Insanity's selection predicate also considers a positive local cost concealed by a global free power, following `CostsEnergyOrStars` (`060018db`) and its predicate (`0600a186`). Fixed star-only cards are eligible; an X cost alone is not.

## Independent duration of energy and stars

Bullet Time's original OnPlay state machine (`0600a82d`, `00cf`) calls `CardModel.SetToFreeThisTurn` (`060018c5`). That helper calls `EnergyCost.SetThisTurnOrUntilPlayed` (`06005223`, expiration value 6), and sets star cost with `SetStarCostThisTurn` (`060018c8`). `TemporaryCardCost.ThisTurn` (`06005298`) sets both `ClearsWhenTurnEnds` and `ClearsWhenCardIsPlayed`. Both fixed resource reductions therefore expire when the physical card finishes playing or when the turn ends. Replays share that physical play's captured payment and do not pay again.

This corrects the historical 0.15 assumption that Bullet Time persisted through repeated physical plays throughout the turn. The v13 migration translates its legacy flags into the shared temporary Set and a star flag while retaining the old writer's known addition boundary. No separate per-character Bullet Time origin arrays are required.

Flatten deliberately has a different lifetime: `Flatten.ReduceCost` (`06003f67`) calls `EnergyCost.SetThisTurn` (`06005224`, expiration value 2). Its energy Set persists after a physical play until the turn ends. A later temporary Set can mask it and reveal it again when that temporary Set expires; a later combat Set supersedes it. Flatten does not change star costs.

`TemporaryCardCost.ThisCombat` (`06005299`) clears on neither play nor turn end. The new star flags are copied with combat instances and Nightmare snapshots, cleared on transformation and temporary expiration, and appended to the v14 serializer. The two per-instance flags and two snapshot fields add 4,224 serialized bytes without changing the historical v13 prefix.

## Audited callers in implemented content

| Scope of reduction | Callers |
| --- | --- |
| Fixed energy and stars, until physical play or turn end | Bullet Time; Infernal Blade; White Noise; Discovery; Splash; Attack, Skill, Power and Colorless Potion; Liquid Memories; Orobic Acid |
| Fixed energy and stars for this combat | Touch of Insanity (`0600a187`, `00dc`) |
| Energy only | Adaptive Strike generation (`0600a78c`, `00ec`); Momentum Strike (`0600ab20`, `00d8`); Modded's addition (`0600ab1c`, `0196`); Rocket Punch's addition (`06004402`, `0046`); Snecko Oil; Corruption and the Free Attack / Skill / Power and Veilpiercer powers |
| Global free fixed energy and stars | Void Form, with its per-turn physical-play limit |

Star X returns before temporary, combat or global star modifiers. The UI retains the X marker, and resource payment captures the actual stars used by the original play.

## Regression scope

`regent_completion_regression.cpp` adds ordered Transfigure / Bullet Time scenarios in both directions, Rocket Punch's negative additions and expiration, Borrowed Time's negative-subtotal guard, Touch of Insanity's star-only and globally concealed candidates, persistent combat star reductions, Liquid Memories' two-resource expiration, energy-only reductions, both X resources and turn-end expiration. Existing Necrobinder scenarios cover cross-character additions and the late free powers; Silent and Necrobinder lifetime fixtures now use the original Bullet Time / Flatten distinction.

`five_character_ui_regression.cpp` adds a real potion selection flow with Borrowed Time and Reflect, checking an energy preview of one and a star preview of zero, read-only rendering, and equality with the committed combat costs. These are pending execution claims, not passing results.
