# 亡灵契约师完整单人卡池：0.16 实现与审计

## 范围与验证状态

本模块实现固定原版 **v0.111.0 / 41cef1ea** 的亡灵契约师单人池 **86 个模型**，保留既有 Soul，并添加 Sentry Mode 生成的 Sweeping Gaze。86 张角色卡以及 Sweeping Gaze 的基础和升级形态均有独立可执行卡效，共编写 **174 种形态**的数值矩阵，以及 Osty、灾厄、灵魂、虚无、费用历史、选择、自动打出与永久成长的交互回归。

**当前为源码完成、待共享接入与已推送检查点验证的状态。尚未执行本轮编译、测试或模拟器。** 完成数量以实际模型分支和数值回归代码为依据，不以资源条目或登记表代替效果实现。主工作记录必须在推送并读回相同源码后，分别记录构建、回归、WASM 和设备验证结果。

原程序集 SHA-256：`0861bfa1df347538d932f22d580e75420f08082792eb914e53b4882764acdbe9`。公开源码只包含独立实现、数值事实、方法标识和审计结果；原程序集、PCK、IL、原版文本及图片不进入仓库。

池方法 `0x0600480c` 按原顺序声明 91 个模型。其中 Cacophony、Glimpse Beyond、Legion of Bone、Soulbound、Underworld 的多人限制明确排除单人模式，因此不计入本模块。剩余 86 个模型映射到 **400–485**，枚举采用 `Necro` 前缀；0–227 的旧模型编号保持不变。Forbidden Grimoire 与 Protector 为 Ancient 卡，只在完整目录和练习中提供；普通奖励、随机生成排除这两张。Eidolon 与 Transfigure 另外禁止随机生成。

## 起始角色与共享状态

起始生命 **66**，牌组为 **4 Strike Necrobinder、4 Defend Necrobinder、Bodyguard、Unleash**，起始遗物 **Bound Phylactery**。生命、牌组和遗物方法分别为 `0x06003a8e`、`0x06003a94`、`0x06003a95`。遗物在战斗开始召唤 1 点 Osty；后续回合在能量重置后再次召唤 1，首回合不重复。遗物方法为 `0x06009ba8`、`0x06009baa`、`0x06009bac`。

Osty 生命继续使用 `ExpansionState::osty_hp` / `osty_max_hp`；敌人灾厄继续使用 `ExpansionState::doom`；Soul 保留既有模型。召唤、灾厄与灵魂药水由共享钩子进入同一机制，不建立第二套宠物或灾厄计数。新增的玩家自身灾厄位于 `NecroState::player_doom`。

## Osty 的生命、攻击与伤害顺序

Osty 存活时，召唤数值增加最大生命，并回复相同的实际增量；死亡后再次召唤将最大生命与当前生命重置为本次召唤数值，不继承死亡前的最大生命。Spur 先召唤再治疗。Protector 读取最大生命，Unleash 读取当前生命，Sacrifice 读取最大生命的三倍后杀死 Osty，再提供卡牌格挡。召唤核心为 `0x0600b333`，增加最大生命并治疗的命令为 `0x0600b305`。

敌方有加成攻击先消耗玩家格挡，再由存活的 Osty 吸收未格挡伤害；超过 Osty 剩余生命的部分继续伤害玩家。Buffer 在 Osty 吸收后处理玩家剩余伤害。非攻击的生命损失不转移给 Osty。Die For You 的目标转移为 `0x06002534`，实际伤害命令随后将宠物 OverkillDamage 传回原目标的第二阶段生命损失处理。

Necro Mastery 对 Osty **实际失去的生命**造成全体直接生命损失，忽略格挡和攻击加成。Sacrifice、Bone Shards 杀死 Osty 的实际生命损失同样触发；过量伤害不会放大它。依据 `0x06009f7b`。每次真实的敌人或宠物死亡使已存在的 Melancholy 费用减 1；复活后的下一次死亡可以再次触发，新生成的 Melancholy 不追溯旧死亡。

13 个 Osty 攻击模型使用独立伤害来源，包括 Sweeping Gaze。它们不加入玩家力量，不消耗玩家 Vigor；Calcify 提供 Osty 的加成。以卡牌所有者判断的 Pen Nib、Gigantification、Double Damage 和 Lethality 仍然生效。`AttackCommand::FromOsty` 为 `0x06005654`；力量与 Vigor 的拥有者限制分别为 `0x06002902`、`0x060029e4`。共享伤害队列使用 `EffectKind::Damage` 的 `aux=2` 标识宠物攻击，最终统一舍入。

High Five 在没有 Osty 时禁止手动打出；其他宠物攻击可被打出，但 OnPlay 所有命令都不执行，因此 Fetch 不抽牌、Snap 不开启选择。Rattle 的次数是 1 加本回合此前完成的 **Osty 攻击命令数**，一次多段攻击只使后续 Rattle 增加一次。Sic 'Em 则对每段命中召唤，包括完全被格挡的命中。相关方法为 `0x06004091`、`0x0600abe3`、`0x0600a020`。

## 灾厄与减益联动

通常的灾厄在目标所属一方回合结束时检查：灾厄至少等于当前生命便杀死目标。End of Days 在附加全体灾厄后立即检查；Neurosurge 对玩家施加的灾厄在玩家一方回合结束检查。灾厄检查先于对应的回合结束减益移除。

Blight Strike 与 Reaper Form 使用 `DamageResult::TotalDamage`，即吸收的格挡伤害加实际生命伤害，不含过量伤害。Reaper Form 包含玩家与其 Osty 的有加成攻击。Blight Strike 自己再附加一次实际伤害对应的灾厄，顺序在攻击回调之后。依据 `0x06005202`、`0x0600a7f8`、`0x06009fd5`。

Shroud 对由己方施加的灾厄提供无加成格挡，包含对玩家自身的灾厄；Sleight of Flesh 对自己施加到敌人的非临时减益造成可被格挡的无加成伤害。Artifact 成功阻止的减益不触发这两者。Debilitate 将虚弱与易伤的偏离 100% 的部分加倍：75% 变成 50%，150% 变成 200%；堆叠只延长持续回合。相关方法为 `0x0600a01e`、`0x0600a02e`、`0x0600250f`、`0x06002510`。

Misery 先快照来源的减益数值和取得顺序，再逐个向其他敌人施加。覆盖 Weak、Vulnerable、Poison、负 Strength、Beetle、Demise、Doom、Strangle、Debilitate、Hang、Oblivion、Sic 'Em；每项独立经过 Artifact 与二次效果。旧存档没有顺序时使用固定后备顺序。临时力量恢复包装本身不被复制。Oblivion 在每次 CardPlay 前捕获旧层数，避免自己施加的新层数立即触发；每次重放独立捕获，玩家回合结束清除。

## 卡牌实例、费用和回合钩子

Banshee's Cry 每完成一次虚无牌打出，所有现有实例费用减 2；新生成的非克隆实例按已有虚无历史初始化。Melancholy 按实际死亡降低现有实例费用。Transfigure 对非 X 费用牌增加本场战斗费用 1，并添加一次重放；X 费用牌仍添加重放，保留 X 费用。费用加法与显式设置费用的先后边界分别保存，不能把所有费用修改压成一个最终值。

Flatten 在 Osty 完成攻击后把所有实例本回合费用设为 0，之后打出 Flatten 自己仍保留本回合的 0 费；后来发生的 Transfigure 加法可以再抬高费用，下一次 Osty 攻击的 Set 又覆盖之前的加法。Fetch 的首次抽牌标记属于物理实例，每回合重置，重放不重复抽；克隆获得新的首次机会。

Right Hand Hand 在 Late 阶段检查该物理打出的资源快照 **EnergyValue**，而不是实际扣除的 EnergySpent。自动打出仍保留当时解析费用，实际扣除为 0；原版 AutoPlay 为 `0x0600b229`，回手条件为 `0x0600ac0b`。回手按真实弃牌堆顺序处理，满手时保留其余实例。Danse Macabre 在每次 CardPlay 前查询当时解析费用，提供无加成格挡；Spirit of Ash 在此时检查虚无；Veilpiercer 在其取得顺序所对应的位置消耗一层。

Demesne 与 Friendship 修改下一次正常能量重置；Delay、Invoke 的能量与召唤只在下一回合提供。Call of the Void、Sentry Mode 在正常手牌抽取之前生成；Countdown、Neurosurge 在抽牌之后触发。Pagestorm 只对实际抽到的虚无牌追加抽牌，保留 No Draw、满手、洗牌和递归选择的共享规则。

The Scythe 的伤害增长绑定永久牌组中的原始实例：基础伤害 13，每次打出永久加 5，升级后每次加 7。战斗内临时克隆复制已有伤害，但不会写回原永久卡；移除、替换、复制永久牌组条目时，统一 deck hooks 同时维护 Scythe 与 Genetic Algorithm 的索引。永久增长不是一个全局伤害总数。

Forbidden Grimoire 每层在战斗胜利后产生一次可选永久移牌奖励。`necro_grimoire_choice(State&, index)` 在 Reward 阶段执行：合法索引移除一张，`-1` 放弃一次；每次成功消耗一个 pending。Eternal 不能被永久移除，当前有限牌组引擎至少保留一张牌。

## 选择、转换与 Eidolon

Cleanse 从抽牌堆消耗一张；Dredge 从弃牌堆选择最多 3 张，按剩余手牌容量截断；Sculpting Strike 与 Snap 只选择尚未有对应关键词的手牌；Seance 从抽牌堆选择；Transfigure 选择手牌。非法实例、重复选择或数量不符在任何修改之前拒绝。每次合法暂停都可以保存、加载后继续。

Seance 生成一张新的基础 Soul，清除旧卡的升级、费用、关键词、重放与实例成长，同时保留原抽牌堆位置。Eternal 的永久牌组限制不阻止战斗中的转换；`CardModel::IsTransformable` 为 `0x0600189a`，TransformTo 为 `0x0600b239`。

Eidolon 快照当时消耗区内所有虚无实例，再按消耗区顺序逐张自动打出。快照不会提前把全部牌移出消耗区，因此前面的 Soul Storm 仍能数到后面尚未打出的 Souls；若先前的牌结束战斗，后续候选仍留在消耗区。嵌套选择保存各层 CardPlay 的灾厄捕获、费用资源值与首次攻击历史，子牌结束后还原父层。

## 保存与回归接口

`src/necro_forward.inc` 定义完整接线面。每次物理打出只初始化一次资源快照；每次重放独立调用 Before、Body、普通 After 和 Late。结束物理组后先清空该 frame 的 Necro 捕获，再还原父层。普通玩家能力按取得顺序触发。

新增状态包括 22 个能力及取得顺序、12 种减益的取得顺序、512 张实例的费用/关键词/成长、13 层待执行快照、192 个永久牌组增长值与待领取移牌数。所有字段进入 v14 后缀，保留旧版前缀；Nightmare 的新角色实例修饰由共享后缀保存。验证明确拒绝越界实例、错误卡种的修饰、非活动层残留捕获与非法指令操作数。

`tests/necro_completion_regression.cpp` 使用独立字面量的费用、关键词、稀有度和效果断言。除 174 个形态外，覆盖宠物吸收与过量伤害、力量/Vigor 区分、伤害倍率、灾厄与 Artifact、无色 Soul 联动、费用修改先后、Fetch 重放、Flatten 跨物理组、Misery 复制、Eidolon 暂停/致死中断、The Scythe 跨战斗及 Grimoire 索引维护。保存测试要求重新编码字节完全一致。测试结果须在已推送冻结源码上执行后填写。

## 全卡数值目录

下表中的“基础 → 升级”只列数值和行为事实；关键词、完整方法标识与逐项变量见 `config/necro-cards-0.16.json`。

| ID | 模型 | 能量 | 基础 → 升级变量 |
| ---: | --- | --- | --- |
| 400 | Afterlife | 1 | Summon 6 → 9 |
| 401 | BansheesCry | 9 → 7 | Damage 33；Energy 2 |
| 402 | BlightStrike | 1 | Damage 8 → 10 |
| 403 | Bodyguard | 1 | Summon 5 → 7 |
| 404 | BoneShards | 1 | OstyDamage 9 → 12；Block 9 → 12 |
| 405 | BorrowedTime | 1 | Energy 4 → 6；ExtraCost 1 |
| 406 | Bury | 4 | Damage 52 → 63 |
| 407 | Calcify | 1 | CalcifyPower 4 → 6 |
| 408 | CallOfTheVoid | 1 | Cards 1 |
| 409 | CaptureSpirit | 1 | Damage 3 → 4；Cards 3 → 4 |
| 410 | Cleanse | 1 | Summon 3 → 5 |
| 411 | Countdown | 1 | CountdownPower 6 → 9 |
| 412 | DanseMacabre | 1 | DanseMacabrePower 4 → 6；Energy 2 |
| 413 | DeathMarch | 1 | CalculationBase 8 → 9；ExtraDamage 4 → 6 |
| 414 | Deathbringer | 2 | DoomPower 21 → 26；WeakPower 1 |
| 415 | DeathsDoor | 1 | Block 6 → 7；Repeat 2 |
| 416 | Debilitate | 1 | Damage 10 → 12；DebilitatePower 2 → 3 |
| 417 | DefendNecrobinder | 1 | Block 5 → 8 |
| 418 | Defile | 1 | Damage 13 → 17 |
| 419 | Defy | 1 | Block 6 → 9；WeakPower 1 |
| 420 | Delay | 2 | Block 11 → 13；Energy 1 → 2 |
| 421 | Demesne | 3 → 2 | Energy 1；Cards 1 |
| 422 | DevourLife | 1 | DevourLifePower 1 → 2 |
| 423 | Dirge | X | Summon 3 → 4 |
| 424 | DrainPower | 1 | Damage 10 → 12；Cards 2 → 3 |
| 425 | Dredge | 1 | Cards 3 |
| 426 | Eidolon | 2 → 1 | 行为见对应显式卡效 |
| 427 | EndOfDays | 3 | DoomPower 29 → 37 |
| 428 | EnfeeblingTouch | 1 | StrengthLoss 8 → 11 |
| 429 | Eradicate | X | Damage 11 → 14 |
| 430 | Fear | 1 | Damage 7 → 8；VulnerablePower 1 → 2 |
| 431 | Fetch | 0 | OstyDamage 3 → 6；Cards 1 |
| 432 | Flatten | 2 | OstyDamage 12 → 16 |
| 433 | ForbiddenGrimoire | 2 → 1 | 行为见对应显式卡效 |
| 434 | Friendship | 1 | StrengthPower 2 → 1；Energy 1 |
| 435 | GraveWarden | 1 | Block 8 → 11；Cards 1 |
| 436 | Graveblast | 1 | Damage 4 → 6 |
| 437 | Hang | 1 | Damage 10 → 13 |
| 438 | Haunt | 1 | HpLoss 7 → 9 |
| 439 | HighFive | 2 | OstyDamage 11 → 13；VulnerablePower 2 → 3 |
| 440 | Invoke | 1 | Summon 2 → 3；Energy 2 → 3 |
| 441 | Lethality | 1 | LethalityPower 50 → 75 |
| 442 | Melancholy | 3 | Block 13 → 17；Energy 1 |
| 443 | Misery | 0 | Damage 7 → 9 |
| 444 | NecroMastery | 2 | Summon 5 → 8 |
| 445 | NegativePulse | 1 | Block 5 → 6；DoomPower 7 → 11 |
| 446 | Neurosurge | 0 | NeurosurgePower 3；Energy 3 → 4；Cards 2 |
| 447 | NoEscape | 1 | DoomThreshold 10；CalculationBase 10 → 15；CalculationExtra 5 |
| 448 | Oblivion | 0 | DoomPower 3 → 4 |
| 449 | Pagestorm | 1 → 0 | Cards 1 |
| 450 | Parse | 1 | Cards 3 → 4 |
| 451 | Poke | 0 | OstyDamage 6 → 9 |
| 452 | Protector | 1 → 0 | CalculationBase 10 → 15；ExtraDamage 1 |
| 453 | PullAggro | 2 | Summon 4 → 5；Block 7 → 9 |
| 454 | PullFromBelow | 1 | Damage 5 → 7；CalculationBase 0；CalculationExtra 1 |
| 455 | Putrefy | 1 | Power 2 → 3 |
| 456 | Rattle | 1 | OstyDamage 7 → 9；CalculationBase 0；CalculationExtra 1 |
| 457 | Reanimate | 3 | Summon 20 → 25 |
| 458 | Reap | 3 | Damage 27 → 33 |
| 459 | ReaperForm | 3 | 行为见对应显式卡效 |
| 460 | Reave | 1 | Damage 10 → 13；Cards 1 |
| 461 | RightHandHand | 0 | OstyDamage 4 → 6；Energy 2 |
| 462 | Sacrifice | 1 → 0 | CalculationBase 0；CalculationExtra 1 |
| 463 | Scourge | 1 | DoomPower 13 → 16；Cards 1 → 2 |
| 464 | SculptingStrike | 1 | Damage 9 → 12 |
| 465 | Seance | 1 → 0 | Cards 1 |
| 466 | SentryMode | 2 → 1 | SentryModePower 1 |
| 467 | Severance | 2 | Damage 13 → 18 |
| 468 | SharedFate | 0 | EnemyStrengthLoss 2 → 3；PlayerStrengthLoss 2 |
| 469 | Shroud | 1 | Block 3 → 4 |
| 470 | SicEm | 1 | OstyDamage 5 → 6；SicEmPower 3 → 4 |
| 471 | SleightOfFlesh | 2 | SleightOfFleshPower 9 → 13 |
| 472 | Snap | 1 | OstyDamage 7 → 10 |
| 473 | SoulStorm | 1 | CalculationBase 9；ExtraDamage 4 → 6 |
| 474 | Sow | 1 | Damage 8 → 11 |
| 475 | SpiritOfAsh | 1 | BlockOnExhaust 4 → 5 |
| 476 | Spur | 1 | Summon 3 → 5；Heal 5 → 7 |
| 477 | Squeeze | 3 | CalculationBase 25 → 30；ExtraDamage 5 → 6 |
| 478 | StrikeNecrobinder | 1 | Damage 6 → 9 |
| 479 | TheScythe | 2 | Damage 13；Increase 5 → 7 |
| 480 | TimesUp | 2 | CalculationBase 0；ExtraDamage 1 |
| 481 | Transfigure | 1 | Energy 1 |
| 482 | Undeath | 0 | Block 7 → 9 |
| 483 | Unleash | 1 | CalculationBase 6 → 9；ExtraDamage 1 |
| 484 | Veilpiercer | 1 | Damage 10 → 13 |
| 485 | Wisp | 0 | Energy 1 |
| 496 | SweepingGaze | 0 | OstyDamage 10 → 15；虚无、消耗 |
