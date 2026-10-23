# UI template audit — 2026-10-23

## Scope and current verification status

The inventory reads every one of the **935 UI binding slots** in both private
NSP2/17 locale packs and their NSP2/18 replacements. Each pack has **200 dynamic
bindings, 735 static bindings and 35 distinct named fields**. All dynamic bindings
have a known display route. The Chinese UI string table is byte-identical between
17 and 18; the English UI string table is also byte-identical between 17 and 18.
The new graphics profile does not implicitly repair template binding.

`tools/audit_ui_templates.py` validates the public binding/index catalog, reads
the complete string directory, classifies every dynamic entry, and optionally
runs `ui_text_regression` through the real display routes. Its JSON output records
slot IDs, public keys, field names, byte counts and hashes only. It never exports
original localized prose, assets or disassembly. The tool's directory checks do
not replace the runtime's complete integrity and structural validation.

The first diagnostic used runtime checkpoint
`573cb816bbf6b57c8d677239060414c9fc124f72`, its frozen native library and a separately
compiled diagnostic driver. All four actual packs passed 935 binding lookups and
200 dynamic binding routes, with 230 model/UI invocations and 34,614 assertions
per pack. No braces remained after the correct display route. This was a
diagnostic, **not the formal gate for the newly added test source**.

Checking for remaining braces also exposed five models with incorrect numeric
values despite successful substitution. Their source fixes and exact regression
cases were subsequently saved and formally verified at checkpoint
`6d2b78ac0984b8d814b9761c8d44c8dc408614e7`. The standalone CTest and all four
actual-pack audits passed, each with 34,708 assertions. The fifth source stage
adds the separate Rolling Boulder case described below; that addition is not
part of the stage4 execution. The [validation metadata](validation-ui-templates-0.17.json)
records the execution identities and closed journal receipts.

## Resource identities

| Profile | Bytes | SHA-256 |
| --- | ---: | --- |
| zhs17 | 81,124,916 | `470a8e2a79338a3bd92d7cf9904b5aa1818f5b6a1ce23b849499e951cbab8f43` |
| eng17 | 78,349,238 | `7cdf55f9a909a6070b968aa8fbaafaebdad12f46e241a5db8451b4aef934ca7b` |
| zhs18 | 91,439,646 | `74279728406d3362993ef82e92c5ee9196bb3d4a84d3ede7bfc9bb081914997b` |
| eng18 | 88,663,968 | `716860f9a494bb0128148e9958a6e7ac45df02087269834ce27d4d866fbda697` |

The UI-table digest includes each slot number, its UTF-8 length and its exact
text bytes in slot order:

- Chinese: `04eed5d95d80cc116a91249fbded928d9f35775976623d961af7c424b94a10ad`.
- English: `696ce507fe511265f05345937d65228d61232bbc77b06dad63df6d4dbbc53ab8`.

## Complete field inventory

Counts are the number of binding slots containing a field, not the number of
occurrences inside a slot. A binding can contain several fields. Fields inside
nested `plural`, `choose`, `show` and `cond` expressions are included.

| Field | Chinese bindings | English bindings |
| --- | ---: | ---: |
| `Amount` | 31 | 88 |
| `ApplierName.StringValue` | 2 | 2 |
| `BaseCards` | 1 | 1 |
| `Block` | 5 | 5 |
| `Card` | 1 | 1 |
| `CardPlay` | 1 | 1 |
| `Cards` | 8 | 8 |
| `CardsLeft` | 1 | 1 |
| `Damage` | 4 | 4 |
| `DamageDecrease` | 2 | 2 |
| `DamageIncrease` | 1 | 1 |
| `Decrement` | 1 | 1 |
| `DexterityPower` | 2 | 2 |
| `Energy` | 7 | 7 |
| `Gold` | 1 | 1 |
| `Heal` | 4 | 4 |
| `Hotkey` | 2 | 2 |
| `Lightning` | 1 | 1 |
| `MaxHp` | 3 | 3 |
| `OnPlayer` | 4 | 4 |
| `OwnerName` | 6 | 6 |
| `SelfDamage` | 2 | 2 |
| `Stars` | 1 | 1 |
| `StrengthApplied` | 1 | 1 |
| `StrengthPower` | 3 | 3 |
| `Summon` | 1 | 1 |
| `ThornsPower` | 1 | 1 |
| `Turns` | 1 | 1 |
| `VigorPower` | 1 | 1 |
| `VulnerablePower` | 1 | 1 |
| `actNumber` | 1 | 1 |
| `ascension` | 1 | 1 |
| `energyPrefix` | 6 | 6 |
| `n` | 132 | 100 |
| `singleStarIcon` | 4 | 4 |

`HotKey` is an additional accepted spelling covered by the regression; the
pinned packs use `Hotkey`. The spelling change does not invent another key bind.

## Which display route owns each field

| Route | Dynamic bindings per pack | Runtime entry |
| --- | ---: | --- |
| Character/power/resource status | 151 | `ui_status_description` → per-model `localized_text`, or the legacy `{n}` fallback |
| Relic descriptions | 32 | `App::relic_description` with the actual relic model |
| Card selection headings | 13 | `App::ui_text` with the actual selection limit |
| Optional pile/deck shortcut | 2 | `App::ui_text` with `None` or the actual `Tab` action |
| Ascension label | 1 | `App::ui_text` with selected or saved run ascension |
| Act label | 1 | `App::ui_text` with the saved act index |

The 151 status bindings consist of 149 original `powers` references, the Stars
resource description and the port's Orb description. Their old UI-bound strings
contain many `{n}` and `{Amount}` fields. The rendered status uses the original
model key first; it binds the selected actor's actual saved amount, snapshots,
applier and independent instance. Only the Orb fallback substitutes legacy
`{n}` directly. Passing these descriptions through the final label resolver with
a guessed zero would hide a binding error and lose the owning model's context.

The 17 final UI entry bindings are exhaustively identified here:

| Original key / port label | Binding slots | Resolved context |
| --- | --- | --- |
| `card_selection.TO_REMOVE` | 7840, 8034 | Actual removal maximum, including two-card original events |
| `card_selection.TO_UPGRADE` | 8018, 8024, 8037 | Actual choice maximum |
| `card_selection.TO_TRANSFORM` | 8019, 8027, 8038 | Actual choice maximum |
| `card_selection.TO_DISCARD` | 8036 | Actual choice maximum |
| `card_selection.TO_EXHAUST` | 8040, 8041, 8042 | Actual mandatory/optional choice maximum |
| `card_selection.TO_ENCHANT` | 8047 | Actual event/relic choice maximum |
| `static_hover_tips.EXHAUST_PILE.title` | 7769 | `None`, selecting the original branch without a shortcut hint |
| `static_hover_tips.DECK.title` | 7794 | `Tab`, the actual inspect/deck action |
| `gameplay_ui.ASCENSION_LEVEL` | 8007 | Selected front-end level or saved run level |
| Port act label | 7848 | Saved act index plus one |

`singleStarIcon` occurs in model prose as well as being supported by the final
label boundary. The regression exercises the actual resource/model route and
both language-specific final-label values.

## Numeric defects found beyond brace scanning

These were reproduced against the frozen 573 runtime. The fixes bind live
saved fields and keep independent power instances separate. No gameplay rule,
RNG stream or save format changes are required.

| Model / field | Reproduced old display value | Required live value / new regression |
| --- | --- | --- |
| `CRIMSON_MANTLE_POWER.SelfDamage` | Catalog `0` after one application | Saved self-damage; two applications give `2` while block amount is `14` |
| `INFERNO_POWER.SelfDamage` | Catalog `0` after one application | Saved self-damage; two applications give `2` while damage amount is `14` |
| `PANACHE_POWER.CardsLeft` | Catalog `5` at progress counter `2` | `5 - counter`; two independent instances show 4 cards / 10 damage and 1 card / 14 damage |
| `MONOLOGUE_POWER.StrengthApplied` | Catalog `0` after an actual strength grant of `5` | Per-instance saved strength; two instances show `5` and `2`, each granting 1 Strength per subsequent card |
| `THE_BOMB_POWER.Amount`, `.Damage` | Aggregate bomb count and catalog damage `40` | Per-instance remaining turns and damage; 2 turns / 50 damage and 1 turn / 40 damage |

The icon's compact aggregate count remains an icon presentation choice; it is
not passed as a single model's timer, damage or independent progress. Panache,
Monologue and Bomb descriptions emit one original localized description per
active slot, matching the existing Nightmare snapshot handling.

A sixth instance issue was identified after that checkpoint was frozen:
`ROLLING_BOULDER_POWER` combined two instances' damage while retaining the
single-instance +5 growth. The next source stage displays each instance
separately; its test expects 7 damage / +5 and 12 damage / +5, rather than one
19 damage / +5 description. This follow-up requires its own saved-checkpoint
gate and is not included in the five-model verification above.

The regression also calls the canonical/detached model path with `model=-1`
and an inactive slot. Those calls retain the canonical defaults and never pick
an arbitrary first active instance. Enemy actor calls do not read player-only
self-damage state. Every live fixture passes state validation, and saved bytes
before and after all display calls must be exactly equal, including RNG state.

### Original reference checks

The pinned original DLL was read locally to verify the meaning of these fields.
Only method identities and conclusions are recorded here; no original IL is
published.

DLL SHA-256: `0861bfa1df347538d932f22d580e75420f08082792eb914e53b4882764acdbe9`.

| Method | Metadata token | Verified behavior |
| --- | --- | --- |
| `CrimsonMantlePower.IncrementSelfDamage` | `0x060024de` | Increments the instance's `SelfDamage` by one |
| `InfernoPower.IncrementSelfDamage` | `0x06002682` | Increments the instance's `SelfDamage` by one |
| `MonologuePower.get_CanonicalVars` | `0x060026f6` | Starts with Strength 1 and StrengthApplied 0 |
| `MonologuePower.BeforeCardPlayed` | `0x060026f9` | Captures that instance's Strength for the played card |
| `MonologuePower.AfterCardPlayed` async body | `0x06009f77` | Adds granted Strength to that instance's StrengthApplied |
| `PanachePower.get_CanonicalVars` | `0x06002763` | Starts CardsLeft at 5 |
| `PanachePower.AfterCardPlayed` async body | `0x06009fa5` | Decrements CardsLeft on eligible subsequent plays and resets at the threshold |
| `TheBombPower.SetDamage` | `0x06002991` | Stores the particular bomb's damage |
| `TheBombPower.BeforeSideTurnEnd` async body | `0x0600a0a8` | Decrements Amount while greater than 1; explodes using that instance's Damage |
| `RollingBoulderPower.get_InstanceType` / `.get_CanonicalVars` | `0x06002810` / `0x06002811` | Independent instances, each with growth Damage 5 |
| `RollingBoulderPower.AfterPlayerTurnStart` async body | `0x06009ff8` | Increases that instance's Amount by its Damage growth |

## Regression boundaries

`tests/ui_text_regression.cpp` is registered with CTest. Its dependency-free
mode exercises all final-label variables, missing-field preservation, buffer
boundaries and live numeric resolver values. With an actual pack, it additionally
checks original plural branches, actual glyph/color pixels and full localized
power descriptions. With the catalog generated by the audit script, it checks
all 935 lookups and every dynamic display route.

Selection cases enter through production mechanics: buying removal at a shop,
choosing Luminous Choir's two-card removal, and playing an already-owned Purity
from a seeded practice deck. Optional exhaust is genuinely 0..3; highlighted
selection count cannot change the requested amount. The English singular branch
correctly says "a card" instead of being required to contain the digit 1.

The rendered `[blue]{Amount}[/blue]` label is compared pixel-for-pixel with the
known digit rendered in blue through single-line, paragraph and fitted text.
Width measurements must agree with the visible digit. This catches both template
substitution and literal-markup rendering at the final drawing boundary.

Long unknown event prose exceeds 1,600 bytes and must be returned intact without
being copied into the 1,024-byte label buffer. A long known template that exceeds
the destination capacity also returns the complete original string. Exact-fit
and one-byte-too-small cases check `LocalizedTemplate::truncated()` directly.
Mixed known and unknown fields preserve the unknown field visibly. An unrelated
model `{n}` or owner field is never fabricated at the UI boundary.

## Residual inventory and formal receipts

The four-pack formal audit has **zero unclassified dynamic slots, zero
unsupported final-UI field names and zero remaining braces after the owning
display route**. Every one of its five exact numeric model fixes passed in both
languages and both resource versions. The complete per-slot report contains
metadata only: 471,492 bytes, SHA-256
`7ef69e1fa69129e86b44390127a394144608d94a554a8dc8e6786baa52f4bc26`.

| Formal stage4 gate | Result | Journal |
| --- | --- | --- |
| Clang Release configuration | Passed | `template-configure-2854a2457cec.json` |
| Native regression build | Passed | `template-build-4f4d9c170cb6.json` |
| Standalone CTest | Passed | `template-standalone-fee4d9e4a018.json` |
| All four original packs + complete catalog | Passed; 34,708 checks per pack | `template-four-packs-02f3d1e3a596.json` |

Every journal was read back with `status=passed`, `returncode=0` and
`source_changed=false`. Their source digest is
`c21a11c21b1f623ceca8443f5130a68978fd3c10668dbe0cc9be70fb395630cf`.
The public validation JSON includes the individual journal/log hashes and actual
runner/source-input identities.

The remaining prepared numeric follow-up is Rolling Boulder, whose two-instance
description needs its stage5 gate. This inventory is limited to the 935 reviewed
UI bindings and the model routes that they reach. It is not a claim that every
original localization key in every event/card/potion has been numerically
exhaustively tested.

The complete audit is reproducible after building `ui_text_regression`:

```sh
python3 tools/audit_ui_templates.py \
  --pack zhs17=/private/zhs17.nsp \
  --pack eng17=/private/eng17.nsp \
  --pack zhs18=/private/zhs18.nsp \
  --pack eng18=/private/eng18.nsp \
  --runner build/template-verify/ui_text_regression \
  --output build/template-verify/ui-template-audit.json
```

Use `tools/run_logged.py` with the actual verified checkpoint for subsequent
formal builds and runs. A later source or test change does not inherit an older
execution result.
