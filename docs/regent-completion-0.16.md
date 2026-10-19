# 储君完整单人卡池：0.16 实现与审计记录

## 当前状态

本次实现覆盖固定原版 **v0.111.0 / 41cef1ea** 的储君单人池 86 个模型、4 个额外生成/状态模型，并复用既有 Sovereign Blade。86 个角色模型均有直接可执行分支；基础与升级、3 张可升级仆从、不可升级 Debris、已有君王之剑合计编写 **181 种有效形态的数值回归**。

**本文记录的是源码完成状态。尚未执行编译或回归；必须由主代理先推送并读回相同源码检查点，再在冻结目录使用 `tools/run_logged.py` 验证。** 0.15 的通过记录不代表储君已验证。没有将登记数量或资源数量当作效果完成证据。

## 来源与边界

原程序集 SHA-256 为 `0861bfa1df347538d932f22d580e75420f08082792eb914e53b4882764acdbe9`。池方法 `0x0600481d` 按固定顺序声明 91 个模型；以下 5 个模型的 `get_MultiplayerConstraint` 返回多人限定，因此不进入单人池：

| 模型 | 限定方法 |
| --- | --- |
| Constellation | `0x06003d1a` |
| HammerTime | `0x06004033` |
| Largesse | `0x0600413f` |
| Plot | `0x06004309` |
| Tutor | `0x060046ab` |

余下 86 张按原池顺序映射到 **314–399**，旧模型 0–227 保持原编号。Meteor Shower 与 The Sealed Throne 的稀有度为 Ancient：保留在角色完整目录中，普通奖励与战斗随机生成排除它们。多人、音频和 Timeline 继续排除。

公开仓库只保存独立运行的程序、数值事实与方法标识。原始程序集、PCK、解码 IL、原版描述、图片、字体和导出的游戏资源包均不随本次源码提交。

## 起始角色

储君初始生命为 **75**，金币 **99**；牌组为 4 张 Strike Regent、4 张 Defend Regent、Falling Star 与 Venerate。Divine Right 在进入每场战斗时提供 **3 颗星星**，计入该回合正向星星获得历史，不计为生成卡牌。

对应方法：生命 `0x06003abc`、金币 `0x06003abd`、牌组 `0x06003ac2`、起始遗物 `0x06003ac3`、遗物数值 `0x06001d86`。

## 星星、支付与重放

星星继续使用共享 `ExpansionState::stars`，锻造伤害继续使用 `ExpansionState::blade_damage`。储君没有第二套星星或君王之剑计数。

手动打出先快照能量和星星支付额，再完整结算能量消耗及 Orbit，随后结算星星消耗及 Child of the Stars，最后进入 BeforeCardPlayed 与卡牌效果。`SpendResources` 为 `0x06009b0c`，两个支付回调为 `0x06009b0a`、`0x06009b0e`。同一实体的重放只支付一次；每次重放仍独立触发前置、卡牌、普通后置和 Late 回调。

自动打出的 EnergySpent 与 StarsSpent 为 0；其 EnergyValue 和 StarValue 保留相应解析值。X 星费读取当时全部星星，X 能量读取当时全部能量；两者都绕过普通费用修改，因此 Void Form 不将 X 改为 0。X 的打出值在物理打出开始时捕获，自动打出不扣除资源。相关方法为 `0x060018da`、`0x06005220`、`0x0600b229`。

Black Hole 每次正向获得星星触发一次无加成群伤；支付星星则在该物理打出的最后一次重放后触发一次，均不按星星数量重复触发。Child of the Stars 按实际消耗的星星数量提供无加成格挡，不加入敏捷和卡牌格挡加成。The Sealed Throne 每次 BeforeCardPlayed 获得星星；Radiate 的次数在前置事件完成后读取，包含本次前置获得的星星。

Void Form 安装能力后请求结束当前回合。原版 BeforeApplied / BeforePowerAmountChanged 会禁用当前回合的免费额度；下一玩家回合才清零计数。额度按非自动的完整物理打出累计，重放不会多消耗额度。相关方法 `0x060029f9`、`0x060029fc`、`0x060029fd`。

## 锻造、君王之剑与生成

Forge 增加所有非消耗区君王之剑的战斗伤害；若一个都没有，则生成基础伤害 10 的新剑，再加入锻造值。消耗区旧剑保持原值。Summon Forth 先将所有非手牌君王之剑召回，随后锻造，因此刚从消耗区召回的剑也得到强化。`ForgeCmd` 的核心方法为 `0x0600b323`。

Seeking Edge 先安装群体攻击能力，再执行锻造。Sword Sage 为每次君王之剑打出追加重放，覆盖已存在和之后生成的剑；Parry 在每次剑的全部攻击完成后提供一次卡牌格挡。Conqueror 对标记敌人的君王之剑伤害固定乘 2，叠层延长回合数，不使倍率继续翻倍。Conqueror 在目标敌方回合结束时减 1；Reflect 在玩家回合开始时减 1。持续时间方法分别为 `0x06009e94` 和 `0x06009fe8`。

| 额外模型 | 能量 | 基础 → 升级 | 其他规则 |
| --- | ---: | --- | --- |
| Minion Strike | 0 | 伤害 6 → 9 | 抽 1；消耗；Strike、Minion 标签 |
| Minion Dive Bomb | 0 | 伤害 13 → 16 | 消耗；Minion 标签 |
| Minion Sacrifice | 0 | 格挡 7 → 10 | 消耗；Minion 标签 |
| Debris | 1 | 无效果 | 可以打出；消耗；最高升级级别 0 |
| Sovereign Blade，既有 ID 113 | 2 → 1 | 基础伤害 10 + 战斗锻造 | 保留；升级只降低能量费用 |

生成通过共享 `notify_card_generated(s,id)` 广播，供所有角色能力共同监听。Begone、Charge 与 Guards 的转化同样计入生成历史，并清除原实例的旧修饰。Arsenal 提供力量，Pillar of Creation 提供无加成格挡，按能力获取顺序执行；Supermassive 使用本场战斗创建数量。Heirloom Hammer 的即时克隆复制完整实例修饰，包括战斗伤害、费用覆盖、保留、狡诈与重放。

## 事件、历史和选择

- Orbit 与 Monologue 为独立能力实例。各 Orbit 的四点能量余数跨回合保留；每个 Monologue 在卡牌打出前捕获自己的数值，在对应后置位置应用临时力量，回合结束各自移除已经给出的力量。相应实例不能合并成一个简单总数。
- Pale Blue Dot 查询所有已完成的卡牌；原版名为 AttacksPlayedThisTurn 的 getter 实际没有攻击类型过滤。阈值为 5，每回合只预约一次下回合抽牌。方法 `0x0600275c`、`0x0600275e`。
- Beat Into Shape 使用目标本回合已收到的其他有加成攻击次数；完全被格挡和零点攻击也计入。当前攻击不额外计入本次锻造奖励。方法 `0x0600a7db`、`0x0600a7dc`。
- Monarch's Gaze 对每次有加成攻击命中触发，即使没有实际扣除生命；Reflect 依据实际吸收的攻击伤害反射无加成伤害。
- Kingly Kick 的抽牌降低属于战斗加法费用修饰，并保留与显式 Set 费用的先后次序；Kingly Punch 的已积累伤害与未来每次抽牌增长分开保存，升级不会丢失已经积累的伤害。
- Decisions, Decisions 对选中的技能执行 3 次独立自动打出，每次都有自己的物理组与重放；不会简单改为追加两次重放。子牌在消耗区仍可继续被下一次自动打出。
- Make It So 在第三张技能的 AfterCardPlayedLate 回手，普通后置监听器先完成。Summon Forth 和 Make It So 的回手接受普通 pile、暂停父牌与预约自动打出的 Limbo 实例；共享打出结算须尊重已经变化的区域，避免重复归位。
- Foregone Conclusion 在正常手牌抽取前选择抽牌堆实例；空抽牌堆先洗入弃牌，允许 Stratagem 等洗牌选择中断。能力在选牌结束后移除。
- Tyranny 修改一次正常回合抽牌数量，随后强制选择消耗对应张数；消耗逐张经过共享回调。Bombardment 在早期自动打出阶段检查消耗区；I Am Invincible 在回合结束的自动打出阶段检查抽牌堆顶。

新增 8 种实例选择：手牌转仆从、抽牌堆转仆从、任意手牌转防御仆从、Decisions 技能、Heirloom 无色复制、弃牌置顶、Foregone 抽牌堆与 Tyranny 消耗。选择使用同一个可保存续执行队列。无效实例、重复实例、数量不合规和明确队列容量不足均在修改状态之前拒绝。

## 接线与保存契约

`src/regent_forward.inc` 列出共享核心需要的函数。`regent_before_card` 必须先于当前卡牌本体；`regent_begin_play` 的支付钩子必须排在它之前。每次卡牌 Finish 增加 Regent 历史后，统一按获取顺序调普通监听器，再调 `regent_after_play_late`；物理组结束后清除当前 frame 捕获，随后还原父 frame。

UI 的费用预览和可打出判定都须调用 `card_star_cost`；0 表示无星星支付，-1 表示 X。动态伤害、格挡和描述使用传入实例的共享预览接口。Regent 的 22 个能力数值和顺序、16 个独立 Orbit / Monologue、13 层物理组字段、512 张实例修饰均进入 v14 后缀。该后缀为 **2,415 个四字节字段，9,660 字节**；Nightmare 等共享快照的追加修饰由共享保存层单独处理。

严格校验包括能力顺序与数值一致、Orbit 余数小于 4、实例修饰只存在于相应卡、非活动 frame 没有待结算捕获、选牌源区域正确，以及操作参数、目标和生成 token 合法。

## 回归文件与待执行验证

`tests/regent_completion_regression.cpp` 为 86 张角色卡的每个基础与升级形态分别断言实际结果，并覆盖新 token 与旧剑。回归除费用、关键词和稀有度外，还直接检查伤害、格挡、资源变化、锻造值、生成数量、牌堆迁移、每个持久能力的触发与持续时间。

专门用例包括支付不足原子拒绝、重放只支付一次、自动 X、Void Form 与 X、独立 Orbit 跨回合余数、Kingly 实例成长、锻造对消耗区的排除和召回、Arsenal / Pillar / 转化、完全被格挡的攻击、Artifact 对先 Weak 后 Vulnerable 的消费、Conqueror / Reflect 双层持续、Foregone 洗牌暂停与三次自动技能的嵌套选择。

所有实例选择都写有保存/加载后继续一致性与非法选择不改变状态的断言。代码尚未运行，必须在 GitHub 已确认的冻结源码上执行；最终构建、WASM 一致性、界面验证、模拟器与设备状态需由主工作记录分别给出证据。不能从本文件推断这些项目已经通过。

## 全卡数值目录

下表每行由固定审计记录生成；“基础/升级”明确区分原始和升级值。能量或星星为 X 时需要动态捕获；变量名称与方法标识用于复核，不是原版描述文本。

| ID / 模型 | 能量 | 星星 | 数值变量（基础 → 升级） | 关键词（基础 → 升级） | OnPlay / OnUpgrade |
| --- | --- | --- | --- | --- | --- |
| 314 / Alignment | 0 | 2 | Energy 2 → 3 | — | `0x06003b20` / `0x06003b21` |
| 315 / Arsenal | 1 | 0 | ArsenalPower 1 | — → Innate | `0x06003b45` / `0x06003b46` |
| 316 / AstralPulse | 0 | 3 | Damage 6 → 8 | — | `0x06003b5b` / `0x06003b5c` |
| 317 / BeatIntoShape | 1 | 0 | Damage 5 → 7; CalculationBase 5 → 7; CalculationExtra 5 → 7 | — | `0x06003ba3` / `0x06003ba4` |
| 318 / Begone | 1 | 0 | 见事件/选择规则 | — | `0x06003bac` / `—` |
| 319 / BigBang | 0 | 0 | Cards 1; Energy 1; Stars 1; Forge 5 | Exhaust → Exhaust, Innate | `0x06003bbc` / `0x06003bbd` |
| 320 / BlackHole | 1 | 0 | BlackHolePower 3 → 4 | — | `0x06003bc0` / `0x06003bc1` |
| 321 / Bombardment | 3 | 0 | Damage 18 → 24 | Exhaust | `0x06003c07` / `0x06003c09` |
| 322 / Bulwark | 2 | 0 | Block 12 → 15; Forge 10 → 13 | — | `0x06003c56` / `0x06003c57` |
| 323 / BundleOfJoy | 1 | 0 | Cards 3 → 4 | Exhaust | `0x06003c5b` / `0x06003c5c` |
| 324 / CelestialMight | 2 | 0 | Damage 6; Repeat 3 → 4 | — | `0x06003ca6` / `0x06003ca7` |
| 325 / Charge | 1 | 0 | Cards 2 | — | `0x06003cb0` / `—` |
| 326 / ChildOfTheStars | 1 | 0 | BlockForStars 2 → 3 | — | `0x06003cba` / `0x06003cbb` |
| 327 / CloakOfStars | 0 | 1 | Block 7 → 10 | — | `0x06003ce4` / `0x06003ce5` |
| 328 / CollisionCourse | 0 | 0 | Damage 10 → 14 | — | `0x06003cf1` / `0x06003cf2` |
| 329 / Comet | 0 | 5 | Damage 33 → 44; VulnerablePower 3; WeakPower 3 | — | `0x06003cfd` / `0x06003cfe` |
| 330 / Conqueror | 1 | 0 | Forge 3 → 5 | — | `0x06003d17` / `0x06003d18` |
| 331 / Convergence | 1 | 0 | Energy 1; Stars 1 → 2 | — | `0x06003d29` / `0x06003d2a` |
| 332 / CosmicIndifference | 1 | 0 | Block 6 → 9 | — | `0x06003d48` / `0x06003d49` |
| 333 / CrashLanding | 1 | 0 | Damage 21 → 26 | — | `0x06003d52` / `0x06003d53` |
| 334 / CrescentSpear | 1 | 1 | CalculationBase 8; ExtraDamage 2 → 3 | — | `0x06003d5b` / `0x06003d5c` |
| 335 / CrushUnder | 1 | 0 | Damage 8 → 9; StrengthLoss 1 → 2 | — | `0x06003d6a` / `0x06003d6b` |
| 336 / DecisionsDecisions | 0 | 6 | Cards 3 → 5; Repeat 3 | Exhaust | `0x06003dc6` / `0x06003dc7` |
| 337 / DefendRegent | 1 | 0 | Block 5 → 8 | — | `0x06003dde` / `0x06003ddf` |
| 338 / Devastate | 1 | 4 | Damage 35 → 45 | — | `0x06003e1b` / `0x06003e1c` |
| 339 / DyingStar | 1 | 3 | Damage 9 → 11; StrengthLoss 9 → 11 | Ethereal | `0x06003e81` / `0x06003e82` |
| 340 / FallingStar | 0 | 2 | Damage 8 → 12; VulnerablePower 1; WeakPower 1 | — | `0x06003efa` / `0x06003efb` |
| 341 / ForegoneConclusion | 1 | 0 | Cards 2 → 3 | — | `0x06003f88` / `0x06003f89` |
| 342 / Furnace | 1 | 0 | Forge 5 → 7 | — | `0x06003fad` / `0x06003fae` |
| 343 / GammaBlast | 0 | 3 | Damage 13 → 18; VulnerablePower 2; WeakPower 2 | — | `0x06003fb8` / `0x06003fb9` |
| 344 / GatherLight | 1 | 0 | Block 8 → 11; Stars 1 | — | `0x06003fc2` / `0x06003fc3` |
| 345 / Genesis | 2 | 0 | StarsPerTurn 2 → 3 | — | `0x06003fc6` / `0x06003fc7` |
| 346 / Glimmer | 1 | 0 | Cards 3 → 4; PutBack 1 | — | `0x06003fe7` / `0x06003fe8` |
| 347 / Glitterstream | 2 | 0 | Block 11 → 13; BlockNextTurn 5 → 7 | — | `0x06003ff3` / `0x06003ff4` |
| 348 / Glow | 1 | 0 | Stars 1 → 2; Cards 1 | — | `0x06003ff7` / `0x06003ff8` |
| 349 / Guards | 2 | 0 | 见事件/选择规则 | Exhaust | `0x0600401b` / `—` |
| 350 / GuidingStar | 1 | 1 | Damage 12 → 13; Cards 2 → 3 | — | `0x0600401f` / `0x06004020` |
| 351 / HeavenlyDrill | X | 0 | Damage 8 → 10; Energy 4 | — | `0x0600405e` / `0x0600405f` |
| 352 / Hegemony | 2 | 0 | Damage 15 → 18; Energy 2 → 3 | — | `0x06004063` / `0x06004064` |
| 353 / HeirloomHammer | 2 | 0 | Damage 20 → 25; Repeat 1 | — | `0x06004067` / `0x06004068` |
| 354 / HiddenCache | 1 | 0 | Stars 1; StarNextTurnPower 3 → 4 | — | `0x06004083` / `0x06004084` |
| 355 / IAmInvincible | 1 | 0 | Block 10 → 13 | — | `0x060040b8` / `0x060040ba` |
| 356 / KinglyKick | 4 | 0 | Damage 27 → 35 | — | `0x0600411b` / `0x0600411c` |
| 357 / KinglyPunch | 1 | 0 | Damage 8 → 10; Increase 4 → 6 | — | `0x06004122` / `0x06004124` |
| 358 / KnockoutBlow | 3 | 0 | Damage 30 → 38; Stars 5 | — | `0x06004131` / `0x06004132` |
| 359 / KnowThyPlace | 0 | 0 | WeakPower 1; VulnerablePower 1 | Exhaust → — | `0x06004137` / `0x06004138` |
| 360 / LunarBlast | 0 | 0 | Damage 4 → 5; CalculationBase 0; CalculationExtra 1 | — | `0x06004175` / `0x06004176` |
| 361 / MakeItSo | 0 | 0 | Damage 6 → 9; Cards 3 | — | `0x06004196` / `0x06004198` |
| 362 / ManifestAuthority | 1 | 0 | Block 7 → 8 | — | `0x060041a9` / `0x060041aa` |
| 363 / MeteorShower | 0 | 2 | Damage 14 → 21; VulnerablePower 2; WeakPower 2 | — | `0x060041d3` / `0x060041d4` |
| 364 / MonarchsGaze | 2 → 1 | 0 | StrengthLoss 1 | — | `0x06004225` / `0x06004226` |
| 365 / Monologue | 0 | 0 | Power 1 | — → Retain | `0x0600422a` / `0x0600422b` |
| 366 / NeutronAegis | 1 | 5 | PlatingPower 8 → 11 | — | `0x06004253` / `0x06004254` |
| 367 / Orbit | 2 → 1 | 0 | Energy 1 | — | `0x06004297` / `0x06004298` |
| 368 / PaleBlueDot | 1 | 0 | Cards 1 → 2; CardPlay 5 | — | `0x060042bb` / `0x060042bc` |
| 369 / Parry | 1 | 0 | ParryPower 10 → 14 | — | `0x060042ca` / `0x060042cb` |
| 370 / ParticleWall | 0 | 2 | Block 9 → 12 | — | `0x060042d5` / `0x060042d7` |
| 371 / Patter | 1 | 0 | Block 8 → 10; VigorPower 2 → 3 | — | `0x060042dc` / `0x060042dd` |
| 372 / PhotonCut | 1 | 0 | Damage 10 → 13; Cards 1 → 2; PutBack 1 | — | `0x060042ef` / `0x060042f0` |
| 373 / PillarOfCreation | 1 | 0 | Block 2 → 3 | — | `0x060042fe` / `0x060042ff` |
| 374 / Prophesize | 2 | 0 | Cards 6 → 9 | — | `0x06004346` / `0x06004347` |
| 375 / Quasar | 0 | 2 | 见事件/选择规则 | — | `0x06004376` / `—` |
| 376 / Radiate | 0 | 0 | Damage 3 → 4; Stars 1; CalculationBase 0; CalculationExtra 1 | — | `0x06004379` / `0x0600437a` |
| 377 / RefineBlade | 1 | 0 | Forge 8 → 12; Energy 1 | — | `0x060043b9` / `0x060043ba` |
| 378 / Reflect | 1 | 3 | Block 15 → 20 | — | `0x060043bf` / `0x060043c0` |
| 379 / Resonance | 1 | 2 | StrengthPower 1 → 2 | — | `0x060043e3` / `0x060043e4` |
| 380 / RoyalGamble | 0 | 5 | Stars 9 | Exhaust → Exhaust, Retain | `0x0600440c` / `0x0600440d` |
| 381 / Royalties | 1 | 0 | Gold 30 → 40 | — | `0x06004411` / `0x06004412` |
| 382 / SeekingEdge | 1 | 0 | Forge 7 → 11 | — | `0x06004459` / `0x0600445a` |
| 383 / SevenStars | 2 → 1 | 7 | Damage 7; Repeat 7 | — | `0x0600446d` / `0x0600446e` |
| 384 / ShiningStrike | 1 | 0 | Damage 8 → 11; Stars 2 | — | `0x0600449a` / `0x0600449c` |
| 385 / SolarStrike | 1 | 0 | Damage 9 → 10; Stars 1 → 2 | — | `0x060044ff` / `0x06004500` |
| 386 / SpectrumShift | 2 → 1 | 0 | Cards 1 | — | `0x06004535` / `0x06004536` |
| 387 / SpoilsOfBattle | 1 | 0 | Forge 6 → 9; Cards 2 | — | `0x0600455f` / `0x06004560` |
| 388 / Stardust | 0 | X | Damage 5 → 7 | — | `0x06004583` / `0x06004584` |
| 389 / StrikeRegent | 1 | 0 | Damage 6 → 9 | — | `0x060045b5` / `0x060045b6` |
| 390 / SummonForth | 1 | 0 | Forge 8 → 11 | — | `0x060045c7` / `0x060045c8` |
| 391 / Supermassive | 1 | 0 | CalculationBase 5; ExtraDamage 3 → 4 | — | `0x060045d6` / `0x060045d7` |
| 392 / SwordSage | 2 → 1 | 0 | SwordSagePower 1 | — | `0x060045f6` / `0x060045f7` |
| 393 / Terraforming | 1 | 0 | VigorPower 7 → 10 | — | `0x06004622` / `0x06004623` |
| 394 / TheSealedThrone | 1 → 0 | 3 | 见事件/选择规则 | — | `0x06004651` / `0x06004652` |
| 395 / TheSmith | 1 | 4 | Forge 30 → 40 | — | `0x06004657` / `0x06004658` |
| 396 / Tyranny | 1 | 0 | 见事件/选择规则 | — → Innate | `0x060046b5` / `0x060046b6` |
| 397 / Venerate | 1 | 0 | Stars 2 → 3 | — | `0x060046f8` / `0x060046f9` |
| 398 / VoidForm | 3 | 0 | VoidFormPower 2 | Ethereal → — | `0x06004707` / `0x06004708` |
| 399 / WroughtInWar | 1 | 0 | Damage 7 → 9; Forge 7 → 9 | — | `0x06004755` / `0x06004756` |
