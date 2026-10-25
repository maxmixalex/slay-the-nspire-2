# Combat card selection source map — 2026-10-24

Reference: **v0.111.0 / 41cef1ea**. This maps the existing port selectors to
the original command calls and UI contracts. It is a source audit, not a claim
that every corresponding presentation change has already passed validation.
The private reference tree contains the named original C# files; no original
source, scene, font or artwork is reproduced here.

## The command and the screen have different confirmation rules

`CardSelectorPrefs(prompt, count)` delegates to the min/max constructor with
both limits equal. The constructor sets `RequireManualConfirmation` when
the minimum is nonnegative and differs from the maximum. It also adds the
source values `Amount`, `MinCount` and `MaxCount` to the localized prompt.
An explicit caller initializer can override the flag. These facts come from
`CardSelection/CardSelectorPrefs.cs`, its two constructors.

That flag is **not a universal click-to-commit rule**:

| Original route | Before a screen is opened | Selection behavior on the screen |
| --- | --- | --- |
| `CardSelectCmd.FromHand` | Zero eligible cards returns empty. With manual confirmation false, eligible count at or below the requested minimum returns all eligible cards without a modal. | `NPlayerHand.SelectCardInSimpleMode` moves cards into the selected container and updates the independent confirm button. It does not commit the choice on reaching the maximum. |
| `FromHandForDiscard` | Delegates to `FromHand`, after supplying the Sly gold-glow predicate. | The same retained-hand screen and independent confirm button. |
| `FromHandForUpgrade` | Zero/one upgradeable card returns directly. | Uses `NPlayerHand.Mode.UpgradeSelect`, an upgrade comparison preview, and independent confirmation. |
| `FromCombatPile` | Zero eligible cards returns empty. A nonmanual selection with eligible count at or below the minimum returns all eligible cards. | `NCombatPileCardSelectScreen.OnCardClicked` checks completion immediately only when manual confirmation is false. The completion count is the smaller of requested maximum and displayed eligible count. Otherwise its confirm button submits. |
| `FromSimpleGrid` | The analogous empty/all-eligible shortcut. | `NSimpleCardSelectScreen` automatically completes nonmanual fixed-count selections; manual selections use its confirm button. |
| `FromChooseACardScreen` | A separate command taking `canSkip`, not `CardSelectorPrefs`. It accepts at most three offer cards. | `NChooseACardSelectionScreen` uses `gameplay_ui.CHOOSE_CARD_HEADER`, commits a chosen offer, and only exposes Skip when `canSkip` is true. |

Original command implementations are in `Commands/CardSelectCmd.cs`:
`FromCombatPile` at 3353/3361, `FromHand` at 3668, `FromHandForDiscard` at
3726, `FromHandForUpgrade` at 3744, `FromSimpleGrid` at 3295 and
`FromChooseACardScreen` at 3169 in the recovered reference.

In the recovered `NPlayerHand.cs`, the selection methods are at 1167/1180,
the confirm handler at 1257 and confirm enablement at 1270. Its private
`CheckIfSelectionComplete` method is registered for Godot reflection but has
no ordinary caller or connection in `player_hand.tscn`. `SelectCards` does not
read `RequireManualConfirmation`. Consequently, applying the grid's automatic
commit rule to all fixed-count hand choices would change the original UI.

At the hand selection maximum, selecting another card first deselects the
**last** selected card. Upgrade mode similarly returns the previous preview
card before selecting a replacement. The hand prompt uses the supplied
localized prompt and its source dynamic variables. Nonmatching hand cards
are hidden in selection mode and reappear while peeking.

## Reading the tables

`Manual` below is the **original preference flag**, not a promise that the
hand UI commits automatically. Hand rows use the independent confirmation
rule above whenever a screen is actually opened. `C(model)` denotes that
card model's `SelectionScreenPrompt`; `P(model)`, `R(model)` and `W(model)`
denote the potion, relic and power model's supplied prompt. For cards, relics
and powers, the base model constructs the corresponding localization-table
key `<MODEL_ID>.selectionScreenPrompt`; it also supplies model dynamic values.
Generic prompts below are in `card_selection` unless explicitly qualified.

`N` means the source dynamic count before the port clamps its eligible pool.
Do not replace a model's prompt dynamic variables with a generic selected
count or assume two callers sharing an `XChoice` have the same prompt.

## Exact prompt keys

All **37 keys** below were checked against both original `eng` and `zhs`
localization tables in the pinned PCK. The model property suffix is
**`.selectionScreenPrompt`**, not `.selectionPrompt`. No original localized
text is copied into this public source audit.

| Route | Localization table | Exact keys in that table |
| --- | --- | --- |
| Hand | `cards` | `PURITY.selectionScreenPrompt`, `THINKING_AHEAD.selectionScreenPrompt`, `GLIMMER.selectionScreenPrompt`, `PHOTON_CUT.selectionScreenPrompt`, `HAND_TRICK.selectionScreenPrompt`, `NIGHTMARE.selectionScreenPrompt`, `GUARDS.selectionScreenPrompt`, `DECISIONS_DECISIONS.selectionScreenPrompt`, `HEIRLOOM_HAMMER.selectionScreenPrompt`, `SCULPTING_STRIKE.selectionScreenPrompt`, `SNAP.selectionScreenPrompt`, `TRANSFIGURE.selectionScreenPrompt`, `DUAL_WIELD.selectionScreenPrompt` |
| Hand | `potions` | `GAMBLERS_BREW.selectionScreenPrompt`, `ASHWATER.selectionScreenPrompt`, `TOUCH_OF_INSANITY.selectionScreenPrompt` |
| Hand | `relics` | `GAMBLING_CHIP.selectionScreenPrompt` |
| Pile | `cards` | `HEADBUTT.selectionScreenPrompt`, `SECRET_TECHNIQUE.selectionScreenPrompt`, `SECRET_WEAPON.selectionScreenPrompt`, `SEEKER_STRIKE.selectionScreenPrompt`, `HOLOGRAM.selectionScreenPrompt`, `COSMIC_INDIFFERENCE.selectionScreenPrompt`, `DREDGE.selectionScreenPrompt`, `GRAVEBLAST.selectionScreenPrompt`, `NEOWS_FURY.selectionScreenPrompt`, `WISH.selectionScreenPrompt` |
| Pile | `powers` | `STRATAGEM_POWER.selectionScreenPrompt`, `FOREGONE_CONCLUSION_POWER.selectionScreenPrompt` |
| Pile | `potions` | `DROPLET_OF_PRECOGNITION.selectionScreenPrompt`, `LIQUID_MEMORIES.selectionScreenPrompt` |
| Simple grid | `relics` | `CHOICES_PARADOX.selectionScreenPrompt` |
| Shared hand/pile actions | `card_selection` | `TO_EXHAUST`, `TO_DISCARD`, `TO_TRANSFORM` |
| Hand upgrade | `gameplay_ui` | `CHOOSE_CARD_UPGRADE_HEADER` |
| Generated offer | `gameplay_ui` | `CHOOSE_CARD_HEADER` |

`PURITY` uses its model's `Cards:diff()` variable. `DREDGE`, `NEOWS_FURY`,
the two power prompts and the generic action prompts use `Amount`, including
the original plural-format expressions. These values must be supplied before
rich-text layout; the source color/format tags also remain part of the prompt.
The complete recovered `PotionModel.cs:375` confirms the potion-table prompt
property, matching the card/relic/power table construction.

## Ordinary combat phases

| Port phase | Original callers | Route / eligible cards | Source min…max | Manual | Source prompt |
| --- | --- | --- | --- | --- | --- |
| `ExhaustChoice` | True Grit upgraded; Burning Pact; Brand; Scavenge | `FromHand`, all remaining hand cards | 1…1 | false | `TO_EXHAUST` |
| `DiscardChoice` | Survivor; Acrobatics; Dagger Throw; Prepared; Hidden Daggers; Tools of the Trade power | `FromHandForDiscard`, remaining hand | 1…1, or the caller's exact `Cards`/power amount | false | `TO_DISCARD` |
| `ExhaustManyChoice` | Purity | `FromHand` | 0…`Cards` (base 3, upgraded value from the model) | true | `C(Purity)` |
| `ReturnChoice` | Headbutt | `FromCombatPile(Discard)` | 1…1 | false | `C(Headbutt)` |

Verified call sites: `Cards/TrueGrit.cs:124`, `BurningPact.cs:81`,
`Brand.cs:172`, `Scavenge.cs:73`, `Survivor.cs:120`, `Acrobatics.cs:115`,
`DaggerThrow.cs:153`, `Prepared.cs:111`, `HiddenDaggers.cs:70`,
`Powers/ToolsOfTheTradePower.cs:74`, `Purity.cs:63`, `Headbutt.cs:125`.
Unupgraded True Grit chooses randomly and does not use this selector.

## Extended hand selections

| `XChoice` | Original caller(s) | Route / eligible cards | Source min…max | Manual | Source prompt |
| --- | --- | --- | --- | --- | --- |
| `Top` | Thinking Ahead; Glimmer; Photon Cut | `FromHand` | 1…1 / caller's `PutBack` count | false | `C(ThinkingAhead)`, `C(Glimmer)`, `C(PhotonCut)` |
| `DiscardRedraw` | Gambler's Brew | `FromHandForDiscard` | 0…999999999 | true | `P(GamblersBrew)` |
| `Exhaust` | Ashwater | `FromHand` | 0…999999999 | true | `P(Ashwater)` |
| `Free` | Touch of Insanity | `FromHand`; costs energy or stars with either local-only or full modifiers | 1…1 | false | `P(TouchOfInsanity)` |
| `Transform` | Entropy power | `FromHand` | power amount…same amount | false | `TO_TRANSFORM` |
| `IUpgrade` | Armaments, unupgraded | `FromHandForUpgrade`; upgradeable hand cards | 1…1 | false | `gameplay_ui.CHOOSE_CARD_UPGRADE_HEADER` |
| `SlySkill` | Hand Trick | `FromHand`; Skill and not already Sly this turn | 1…1 | false | `C(HandTrick)` |
| `NightmareCard` | Nightmare | `FromHand` | 1…1 | false | `C(Nightmare)` |
| `RTransformHand` | Begone | `FromHand` | 1…1 | false | `TO_TRANSFORM` |
| `RTransformGuards` | Guards | `FromHand` | 0…999999999 | true | `C(Guards)` |
| `RDecisions` | Decisions, Decisions | `FromHand`; Skill and lacks `Unplayable`; `PretendCardsCanBePlayed=true` | 1…1 | false | `C(DecisionsDecisions)` |
| `RHeirloom` | Heirloom Hammer | `FromHand`; visual card pool is Colorless | 1…1 | false | `C(HeirloomHammer)` |
| `RTyranny` | Tyranny power | `FromHand` | power amount…same amount | false | `TO_EXHAUST` |
| `NEthereal` | Sculpting Strike | `FromHand`; lacks local Ethereal keyword | 1…1 | false | `C(SculptingStrike)` |
| `NRetain` | Snap | `FromHand`; lacks Retain keyword | 1…1 | false | `C(Snap)` |
| `NTransfigure` | Transfigure | `FromHand` | 1…1 | false | `C(Transfigure)` |
| `EventDualWield` | Dual Wield | `FromHand`; Attack or Power | 1…1 | false | `C(DualWield)` |

Verified model calls: `ThinkingAhead.cs:115`, `Glimmer.cs:115`,
`PhotonCut.cs:119`, `Potions/GamblersBrew.cs:73`, `Ashwater.cs:72`,
`TouchOfInsanity.cs:54`, `Powers/EntropyPower.cs:77`, `Armaments.cs:86`,
`HandTrick.cs:85`, `Nightmare.cs:93`, `Begone.cs:65`, `Guards.cs:105`,
`DecisionsDecisions.cs:150–154`, `HeirloomHammer.cs:126`,
`Powers/TyrannyPower.cs:74`, `SculptingStrike.cs:90`, `Snap.cs:106`,
`Transfigure.cs:68`, `DualWield.cs:70`. Upgraded Armaments upgrades the
eligible hand directly and does not open this selection screen.

## Extended pile selections

| `XChoice` | Original caller(s) | Route / eligible cards | Source min…max | Manual | Source prompt |
| --- | --- | --- | --- | --- | --- |
| `Draw` | Secret Technique; Secret Weapon; Seeker Strike; Droplet of Precognition; Stratagem power | `FromCombatPile(Draw)`; respectively Skill, Attack, sampled option membership, all, all | 1…1; Stratagem uses its power amount | false | Respective `C`, `P` or `W` model prompt |
| `Discard` | Hologram; Liquid Memories | `FromCombatPile(Discard)` | 1…1 | false | `C(Hologram)` / `P(LiquidMemories)` |
| `RTransformDraw` | Charge | `FromCombatPile(Draw)` | `Cards`…same count (2) | false | `TO_TRANSFORM` |
| `RDiscardTop` | Cosmic Indifference | `FromCombatPile(Discard)` | 1…1 | false | `C(CosmicIndifference)` |
| `RForegone` | Foregone Conclusion power | `FromCombatPile(Draw)` | power amount…same amount | false | `W(ForegoneConclusionPower)` |
| `NCleanse` | Cleanse | `FromCombatPile(Draw)` | 1…1 | false | `TO_EXHAUST` |
| `NDredge` | Dredge; Graveblast | `FromCombatPile(Discard)` | Dredge: `min(Cards, free hand slots)` exactly; Graveblast: 1 exactly | false | `C(Dredge)` / `C(Graveblast)` |
| `NSeance` | Seance | `FromCombatPile(Draw)` | `Cards`…same count | false | `TO_TRANSFORM` |
| `EventNeowsFury` | Neow's Fury | `FromCombatPile(Discard)` | 0…`min(Cards, free hand slots)` | true | `C(NeowsFury)` |
| `EventWish` | Wish | `FromCombatPile(Draw)` | 1…1 | false | `C(Wish)` |

Verified model calls: `SecretTechnique.cs:64`, `SecretWeapon.cs:64`,
`SeekerStrike.cs:141`, `Potions/DropletOfPrecognition.cs:71`,
`Powers/StratagemPower.cs:74`, `Hologram.cs:120`,
`Potions/LiquidMemories.cs:76`, `Charge.cs:77`,
`CosmicIndifference.cs:141–143`, `Powers/ForegoneConclusionPower.cs:142`,
`Cleanse.cs:118`, `Dredge.cs:111–114`, `Graveblast.cs:125`,
`Seance.cs:77`, `NeowsFury.cs:119–122`, `Wish.cs:64`.
Seeker Strike still uses a real Draw-pile screen with an eligibility filter;
its sampled candidates do not turn it into a generated-card offer screen.

## Offers and combat relic selections

| Port selector | Original caller | Original route | Limit / skip | Prompt / commit |
| --- | --- | --- | --- | --- |
| `XChoice::Offer` | Discovery; Quasar; Splash; Attack, Colorless, Power and Skill Potions | `FromChooseACardScreen` | At most one; **`canSkip=true` for every current caller** | `gameplay_ui.CHOOSE_CARD_HEADER`; choose directly or Skip |
| `XChoice::EventAbundance` | Abundance | `FromChooseACardScreen` | One; default `canSkip=false` | Same original offer header; no Skip |
| `XChoice::EnemyKnowledge` | Knowledge Demon | `FromChooseACardScreen` | One of two; default `canSkip=false` | Same original offer header; no custom knowledge title |
| `WorldRelicChoice::Hand` | Gambling Chip | `FromHandForDiscard` | 0…999999999; manual flag true | `R(GamblingChip)`; retained hand and independent confirmation |
| `WorldRelicChoice::Hand` | Toasty Mittens | `FromHand` | 1…1; manual flag false | `TO_EXHAUST`; hand UI still independently confirmed if opened |
| `WorldRelicChoice::CombatOffer` | Toolbox | `FromChooseACardScreen` | One of three; **default `canSkip=false`** | Original offer header; no Skip |
| `WorldRelicChoice::CombatOffer` | Choices Paradox | **`FromSimpleGrid`**, not Choose-a-card | One of five; prefs 1…1, manual false | `relics.CHOICES_PARADOX.selectionScreenPrompt`; grid auto-completes on one selection |

Verified calls: `Cards/Discovery.cs:71`, `Quasar.cs:71`, `Splash.cs:102`,
`Abundance.cs:83`, `Potions/AttackPotion.cs:72`, `ColorlessPotion.cs:73`,
`PowerPotion.cs:72`, `SkillPotion.cs:72`,
`Monsters/KnowledgeDemon.cs:124`, `Relics/GamblingChip.cs:74`,
`ToastyMittens.cs:81`, `Toolbox.cs:80`, `ChoicesParadox.cs:102`.

## Integration findings at the audit boundary

The audit found three direct routing/skip mismatches in the then-current port:
common `XChoice::Offer` was fixed to 1…1 even though its source callers allow
Skip; Toolbox was marked optional although the source does not allow Skip;
and Choices Paradox shared the generated-offer route although the original
uses the simple grid. These findings were sent to the combat implementation
owner before its source changes. Decisions, Decisions also needs the original
`!Unplayable` eligibility condition, in addition to Skill.

The source metadata above does not justify changing all hand choices into grid
choices or making their fixed-count selections commit immediately. It also
does not justify using a retained power/relic/card title in place of the
model's actual selection prompt. Hand selection, pile selection, an upgrade
preview and a generated offer remain distinct source behaviors.

For Decisions, Decisions, the port already exposes the relevant intrinsic
keyword query: `card_keywords(state, card, instance)` in `src/characters.cpp`.
Its bit `128` is Unplayable, and the helper includes the upgraded keyword set
and event keyword modifications. Combine absence of that bit with the actual
`card_type` being Skill. `can_play_card` and `card_has_nonresource_play_block`
also apply unrelated current-play restrictions; using either would exclude
cards that the original selector deliberately accepts. Testing a negative
energy cost would also confuse the independent X-cost and keyword concepts.

## Selection implementation in the pending UI checkpoint

The UI routes now follow the table above. `combat_selection_kind()` keeps
retained-hand selection, hand upgrade previews, generated offers, combat pile
selection and the Choices Paradox simple grid distinct. Hefty Tablet and Lead
Paperweight also use their original optional choose-a-card screen; ordinary
card rewards remain under the separate reward controller.

Generated offers use the original 0.8 card scale, 340-source-pixel spacing,
common banner, optional Skip and strictly greater than 350 ms click gate.
Inspecting or peeking does not accept an offer. A click commits through the
existing canonical selector; there is no extra generic confirmation page.
The source offer callers allow Skip, while Toolbox and Knowledge Demon do
not. Number shortcuts belong to `NPlayerHand.ActiveHolders` and no longer
select through an offer or pile overlay.

Hand selections retain the existing fan, hide filtered-out cards, and move
selected cards into the original central container. The container uses the
source three size bands, 500 ms sine movement and the selected holder's
150 ms cubic movement. A full selection replaces its last selected card.
Peek exposes the remaining hand, including previously filtered cards, while
keeping the selected container hidden. Hand upgrade previews use the scene's
0.9 scale and 50-source-pixel vertical offset: only the before card returns
to the hand; only the after card supplies hover tips. Hover enlarges the
before preview by 1.1 and releases it with the source 500 ms exponential
curve. Confirmation is independently enabled at a valid count and uses the
native right-side control's 350 ms entrance/exit.

Pile and simple-grid selectors use the source 80-pixel top inset, bottom
selection prompt, ordinary card grid and a single native Confirm when the
original preferences require it. Fixed-count pile choices commit when the
required eligible count is selected. The core's automatic all-eligible
shortcut is implemented at the command boundary, not in rendering. The
original prompt receives requested minimum/maximum values before filtering;
manual ranges such as Purity and Neow's Fury are not rewritten to the
remaining eligible count.

The original `run.tscn` shared overlay backstop is black at 0.85 opacity,
with a 500 ms cubic entrance. The retained-hand backstop is its separate
0.75-opacity, 200 ms linear control. The 0.752941 color in the selection
screen scenes belongs to the small bottom prompt, not the whole screen.

Grid selection replaces the previous green border/checkmark with the
original card ripple texture and equation. The private converter samples the
original half-float SDF alpha and color at the projected texture centers;
the existing image cache holds the interleaved scalar/color words. Rendering
uses bounded fixed-point evaluation and additive color. The projection and
RGB565 quantization are explicit adaptations; this does not claim identical
Godot rasterization or reproduce unrelated GPU effects.

### Verification boundary

The added `combat_selection_ui9_regression` invokes real card, potion and
relic commands for Discovery, Abundance, Purity, Prepared, Armaments,
Headbutt, Neow's Fury, Liquid Memories, Hefty Tablet and Lead Paperweight.
It checks strict opening gates, native hit regions, hand replacement and
Peek behavior, independent confirmation, original preview ownership,
scrolling, optional Skip, and byte-identical canonical results against the
same core commands without UI observation. The shader check uses an
independent floating-point transcription of the original equation.
Existing tactics, expansion, world-combat, Silent and five-character UI
fixtures are being adapted to these source-specific click/confirm rules,
while retaining their save/load, immutable-inspection and continuation
assertions. Source syntax checks have passed. Formal frozen-source native
execution and the new English/Chinese production-asset screenshots remain
pending at this checkpoint; this section does not convert source coverage
or test counts into a percentage of original-game fidelity.

After executor recovery on 2026-10-25, the remote `1e504049` CI log was
recovered with its original failures intact. Short diagnostic drivers linked
the unchanged frozen `1e504049` core, headers and drawing source. The updated
combat selector driver passed 71,026 checks. Its geometry checks now first
allow the original 500 ms offer unfocus transition to finish, distinguish
the lifted keyboard-focused hand card from the ordinary fan, and render a
selection change before timing its holder movement. These fixture changes
do not relax the 350 ms offer click gate or the canonical byte comparisons.
The shared, expansion and five-character drivers also passed their focused
diagnostics with original native Back/Confirm controls and filtered deck
entries. This is diagnostic evidence; formal combined-checkpoint execution
and both production-resource visual reviews remain required.

## Fixed world-deck command companion

The same source shortcut applies to `CardSelectCmd.FromDeckForUpgrade`
(3441–3455), `FromDeckForTransformation` (3485–3499), and `FromDeckGeneric`
(3614–3637): a nonmanual request returns all eligible cards when their count
does not exceed the requested minimum. `FromDeckForRemoval` (3607–3610)
delegates to Generic with a stable Curse-first ordering. Enchantment's
overload (3550–3572) returns its supplied list when its count is at or below
the minimum, before the normal sorted screen path.

`world_relics.cpp` now applies this after a complete acquisition step, using
the existing low-level selection continuation. It does not call the public
selection entry recursively, and it does not resolve a pending selection on
load. Royal Stamp completes its shuffled offer list before this boundary.
The whitelist is explicit:

| Source command | Relic acquisition callers |
| --- | --- |
| Transform | Astrolabe, New Leaf |
| Remove | Biiig Hug, Empty Cage, Pael's Tooth, Precarious Shears, Precise Scissors, Preserved Fog |
| Duplicate through Generic | Dolly's Mirror |
| Enchant | Electric Shrymp, Pael's Growth, Punch Dagger, Royal Stamp, Tri-Boomerang |
| Upgrade | Pomander, Yummy Cookie |

Claws, Gnarled Hammer and Kifuda retain their optional manual selection.
Rest-site Smith/Cook and reward-list removal do not enter this acquisition
whitelist. The helper needs at most five local card indices and adds no
serialized field or persistent storage.

`world_selection_shortcut_regression` exercises all sixteen callers through
real relic acquisition, including Curse-first removal notices, permanent
Genetic Algorithm metadata through Dolly/Bing Bong, Royal Stamp's filtered
index, fewer-than-requested multi-card sets, optional zero selection, and
byte-exact old suspended saves. Its short matched-core diagnostic passed
326 checks; the existing world-relic suite passed 2,016 with the same new
translation unit. Formal execution is required again from the combined
final source checkpoint.
