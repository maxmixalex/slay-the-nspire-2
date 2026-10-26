# UI13：本轮 28 项反馈、保留差异与验收范围

参考版本固定为原版 **v0.111.0 / 41cef1ea**。本轮从 UI12 `9fcbd7f`
恢复开发，保留五角色、原版单人流程、已有存档和已经验证的内容基础。
这里记录真实实现和已经闭合的验证；最终程序、资源、Firebird 和交付的
确切版本由 [WORK_STATE.md](WORK_STATE.md) 与最终发行 receipt 统一标识。
原版 PCK、字体、生成图片和完整资源包保留在私有目录。
此前授权的完整 Ancient 动画存储例外，以及音频、多人、Timeline 排除范围
继续有效；LCD 像素、CPU shader／粒子和装饰随机数的具体映射分别在下文说明。

最终程序为 `792904cb5e7f5fcbbab7a5320ee0d54d247591c7`，资源转换为
`e606108221fd6f8202f61b3e5ae1372970de0e27`。两者的 `src/include/platform/CMake`
逐字节相同；后续测试 checkpoint 单独记录，不改变这个程序身份。最后复查
补入的 Random 动态背景、Fake Merchant 原场景层、主菜单模糊和标签独立
彩虹时钟均已进入该程序。完整 5,156 辅助目录、全部 16 语言转换和 native
读取门已经闭合；前端四门 × 英文／简中也已使用这些最终文件重新通过。
最终 Firebird 商人配对性能、自然首战及保存／继续／USB 读回、四个特殊场景
控制展示、测试后自然档恢复和最终四件套重新读回均已闭合。每个阶段保留独立
receipt，具体窗口、流程、精确文件绑定与限制见下文。

较早的 `d5f9a3c9dfc9bb55e4742239dbb7be4ee5d8c8d1` 程序和
`6e40566007bf616b2551c02a5e99c8960216cc5a` 资源仍是有效的历史候选证据，
其体积、图像、设备局面和 ARM 身份不会被重标成最终发行文件。

早期追加的 `b96bd0d05e3a4d00b5abda48fe21264f47cbc89e` 只修正测试，生产源码
与当时的 `d5f9a3c` 一致；该阶段全部 16 语言的实际 native gate 已在这个测试
checkpoint 闭合。
其后的 CardHUD 测试修正在 `78a2cef775ddb6943baf194f7bc9e0e8c568edf8` 闭合。
地图图例实际字形宽度修正在 `fca66343d0fee759b95b4b6acdf2d0ba9c7eea06`
通过新一轮双语 CardHUD；上述两个阶段的证据均保留其真实身份。

## 反馈逐项映射

### 主界面、百科、设置、顶部栏与地图

| # | 用户反馈 | 实际实现 | 对应证据 |
| --- | --- | --- | --- |
| 1 | 菜单间距过大，继续游戏与 Logo 重叠 | Logo、火焰和光晕整体上移 12px；行距 20→17px。有存档的五行菜单从 y121 开始，Logo 的完整边界结束于 y108。 | `include/title_scene.hpp`；`profiles_ui7_regression` 的新旧存档、悬停、按下和 Continue 全帧像素比较；真实中文截图人工检查。 |
| 2 | 确认／取消按钮过大 | 点击区域改为 80×24，原版面板／描边图为 88×32；保持原版悬停、按下曲线和 10px 标签，键帽独立位于下方。 | `include/popup_layout.hpp`、`src/frontend_popup_ui10.inc`；`button_presentation_regression`、`profiles_ui7_regression`。 |
| 3 | 新增移植制作名单是否覆盖全部语言 | `config/port-credits.json` 覆盖全部 16 个语言代码；转换器写入 8703 文本槽并仅补齐缺少的原版字体字形。泰文新组合字形不覆盖已有 PUA 字形。 | `tools/frontend_ui13_assets.py`；最终 `e606108` 全 16 语言实际 native gate 38,686 项检查、53,072 条描述通过，包含 8/10/12px 字形；真实 Credits 进入／返回测试。 |
| 4 | 百科下属页面的右箭头与滚动条重叠 | 过往记录的左右翻页箭头移到顶角，位于滚动条上方；右箭头点击区域不再被随后注册的滚动条截获。 | `src/ui_progress_ui.inc`；`frontend_ui12_regression` 在真实可滚动历史上点击 Next，验证页码改变及 canonical save 不变。 |
| 5 | 语言下拉仍是自绘控件 | 使用原版 `small_scrollbar_*` 三张纹理、源半透明轨道和滑块材质。原场景的列表背景本身是 ColorRect，保留其原色；居中语言名、独立滚动位置、拖动、滚轮、键盘可见性均接入。展开后屏蔽底层设置悬停。 | `src/settings_ui10.inc`、`src/pointer.cpp`、`include/settings_layout.hpp`；真实 UI13 ENG/ZHS 各 138 项 frontend 检查。 |
| 6 | 游玩时间图标消失；房间、楼层、Boss 间距不均 | 恢复已有原版时钟图；药水栏及 Room/Floor/Boss/Timer 由同一布局计算，统一 4px 间隔，四／五药水槽动态收紧。完整时间仍可通过悬停和键盘焦点查看。 | `include/hud_ui7_presentation.hpp`、`src/hud_ui6.inc`；`combat_ui_regression`、`potion_ui_regression`，包含多槽库存。 |
| 7 | 当前角色／进阶悬停缺少原版“+”等文本 | 使用原版 `PORTRAIT_DESCRIPTION` 和每项 ` +` 前缀；Custom/Daily 附带原版成就锁定说明，标准进阶 0 按原版不显示该 tooltip。 | `src/hud_ui6.inc`；16 份原版 `ascension.json` 和 `AscensionHelper` 源行为校核、combat UI 检查。 |
| 8 | 默认键位没有充分利用计算器实体键 | 两套默认键位使用 Tab、Doc、Home/On、Del、Var、Menu、Shift、Ctrl 和 Catalog；纯键盘模式中心键选择／出牌，Enter 确认当前多选或对话框，保留 Select／Confirm 的原版区别；Shift 松开时切换窥视。仅逐套迁移完全未修改的旧默认，自定义键位逐字节保留。 | `include/input_bindings.hpp`、`src/ui_preferences.inc`、`src/input_bindings.inc`；`input_bindings_regression`、`lcd_input_ui12_regression`。 |
| 9 | 地图图例仍溢出，希望加长、居中和留白 | 图例纸张为 129×170，每一项将图标＋完整文字作为一组居中，四周保留 padding。按实际字形墨迹边界计算词组宽度，修复仅用 pen advance 时误省略英文 Unknown、Merchant、Rest Site 的尾部。 | `src/world_map_ui.inc`；`world_map_ui_regression`、原版 16 语言字体测量；最终 CardHUD ENG/ZHS 为 712,932／790,294 项，完整词组像素与 padding 均检查。 |

### 战斗和手牌

| # | 用户反馈 | 实际实现 | 对应证据 |
| --- | --- | --- | --- |
| 10 | 玩家回合 banner 缺字 | 回合计数改用完整的 10px 字符带；旧 7px 稀疏场景字符带不包含部分中文计数字形。计数继续居中位于标题下方。 | `src/turn_banner_ui.inc`；真实双语 `combat_ui_regression` 检查字形存在和实际输出。 |
| 11 | 辉星数量应紧邻能量 | 按原版 Stars 相对 Energy 的 −36、+62 源坐标偏移，放回能量左下方紧邻的位置。 | `include/combat_layout.hpp`、`src/combat_ui.inc`；原版 `star_counter.tscn`、实际 Regent 画面与布局断言。 |
| 12 | 格挡与血量描边被移除 | 两种数字恢复四向 1px 细描边；一次解析字形后复用各描边 pass。 | `src/health_bar_ui.inc`；82,236 项独立 health-bar 渲染检查和 combat gate。 |
| 13 | 可用牌蓝光太粗，顶部异常凸出 | 修正 `KEEP_ASPECT_CENTERED`：759×759 方形 SDF 在 759×951 控件中垂直居中，源 inset 为 96。手牌及选择预览共享这个修正，保留原版波纹时钟和颜色。 | `include/hand_card_highlight.hpp`、相关手牌／选牌渲染；真实原版 SDF 像素比较与 `combat_ui_regression`。 |
| 14 | 2/4/6/8 仍导航，应直接选相应手牌 | 导航使用触摸板四边的实体方向键；1–9、0 分别选择第 1–10 张有效手牌；去掉未绑定数字 5 的确认 fallback。重复按住不会误触新出现的控件。 | `src/input_bindings.inc`；两套物理输入、重复按键归属和保存键位迁移的 LCD 回归。 |
| 15 | 充能球位置与总容量关系不对 | 重新用原版最大容量的 −150°→−25° 弧线、225→300 半径和 IntentPos/OrbPosition 校核。补齐缺少的对象生命周期：出列球淡出 250ms，余球 Sine.InOut 重排 450ms，新空位从原点进入；同种球重新充能也有独立出生过程。 | `combat_orb_presentation.hpp`、`combat_orb_ui.inc`、只读 `CoreOrbNotice`；真实容量 1/2/3/5/7/10、连续 Evoke/Capacity 画面、save/RNG 不变检查。 |
| 16 | 消耗牌堆纹理不对，且不应在结束回合上方 | 从原图直接重采样三种牌堆到 24×24。原消耗牌堆是紫色图案＋中心粉色计数，没有独立红色 badge；抽／弃牌堆保留各自原版红色计数底图。消耗堆移到结束回合下方、弃牌堆上方，飞牌目的地共用新坐标。 | `tools/combat_ui13_assets.py`、`combat_controls`；逐项检查原版 pile 场景／材质和实际帧，保持全部牌堆可点击。 |
| 17 | 玩家气泡偏左，尾部没有接上角色 | 思考气泡最后一团烟雾连接到玩家 body 右侧，保留源纹理和显示时长。 | `src/combat_ui.inc` 的 thought-bubble 路径；combat UI 实际资源检查。 |

### 房间、奖励、选牌和变形

| # | 用户反馈 | 实际实现 | 对应证据 |
| --- | --- | --- | --- |
| 18 | 搜刮标题不在缎带上，条目文字不垂直居中 | 标题落入缎带实际可见带；普通奖励行和长文 marquee 使用同一垂直居中基线。 | `src/world_reward_ui.inc`；真实 ENG/ZHS reward gate 各 4,076 项，已查看源素材截图。 |
| 19 | 选牌后的前进按钮没有悬停动画 | 指针可见时使用实际 pointer entry/exit，解除残留键盘 selected 状态，驱动既有原版 focus/unfocus 曲线。 | `src/native_controls.inc`；`world_reward_ui_regression`、原版按钮曲线检查。 |
| 20 | 事件文字越框、按钮横向拉伸、相关牌偏左、tooltip 叠在一起 | 186px 选项列使用原版按钮和 outline 的 nine-patch，15px 横向／6px 纵向 padding；长文在边界内滚动。左侧 104px 区域显示 69×96 相关牌，关键词在独立下方区域分页，木雕圆环纳入回归。 | `src/world_events_ui.inc`、`event_hover_ui.inc`、`keyword_tooltip_ui.inc`；最终实际 event ENG/ZHS 5,850／5,827 项及 keyword gate。 |
| 21 | 附魔 tooltip 挡卡牌／滚动条 | 附魔选牌使用两列，滚动条 x187，说明栏固定 x212..316。悬停卡牌接管同一说明栏，并只合并一次待附魔说明；关联牌与分页内容位于确认键上方。 | `card_grid_ui.inc`、`world_selection_ui.inc`、`keyword_tooltip_ui.inc`；实际 selection、deck-selection 和 keyword 截图／断言。 |
| 22 | 附魔描述应使用原版特殊标记 | 使用原版 enchant tab、图标、数量和 disabled 状态；仅 `HasExtraCardText` 明确要求的模型追加 `.extraCardText`。小卡和手牌缓存键包括附魔种类、数量和禁用状态。 | `src/card_face_ui.inc`、`src/world_text.inc`；实际 card keyword/marker 检查、变形前后独立卡牌事实检查。 |
| 23 | 商人持续低帧率，手不应缩小 | 商人手恢复原版稀疏 320×240 采样的原尺寸与指尖锚点。库存地毯复用精确覆盖信息，完全遮挡的旧层不重复加载／绘制；普通场景 5/6 MiB、商人 10/1 MiB 的图片／组缓存共享同一 11 MiB arena，以保留完整原版动画循环。 | `src/merchant_ui.inc`、`src/content.cpp`、`src/content_tiles.inc`；native 2,376 个全帧精确比较／41,569 项检查；最终 Firebird 进入商人 120s 后的 60s 窗口约 30.00FPS，UI12 为 9.83FPS，源读取由 20,364,684B 降至 12,700B。设备窗口仍有 1 次读取，不写成 0 IO。 |
| 24 | 选牌确认时仍触发下层悬停 | 模态确认期间卡牌网格不注册 focus/hit；HUD 绘制临时屏蔽 pointer、键盘焦点及新增 hit，确认预览自身仍可交互。另修复重叠选牌首次点击时重绘改变 z 序、选中邻卡的 Toasty 真实问题。 | `src/card_grid_ui.inc`、`src/world_selection_ui.inc`、`src/app.cpp`、`src/card_choice_ui.inc`；selection/deck-selection 保存不变 gate；`d5f9a3c` 最终压缩包的 `world_ui` 双语各 31,796 项通过。 |
| 25 | 变化误用了移除卡牌动画 | 由真实 `CardTransformed` observer 捕获旧／新模型，使用独立队列、源 mask/glow/shine/speck/flare/ring 和 250/750/1125/1225/1390/2190ms cue；批量变形有交错和分组，旧／新附魔及费用事实各自保留。 | `card_transform_presentation.hpp`、`card_transform_ui.inc`、`tools/rooms_ui13_assets.py`；真实变形、批量选择、展示不修改 canonical card facts 的回归。 |
| 26 | 休息点按钮边框过粗 | 使用原版 outline 相对 button 的尺寸比例，取消对已缩放按钮再叠加旧的大边框放大。 | `src/world_rooms_ui.inc`；真实 `world_room_ui_regression` 双语各 14,689 项及截图。 |
| 27 | 历史房间显示 `{ModelTitle:: {}\|}` | 公共有界 formatter 支持 SmartFormat 的空 formatter 条件语义，正确处理嵌套 current-value 和未选分支，未知变量仍保留可诊断性。 | `include/localized_template.hpp`；1,688 项 localized-template 检查、真实历史页检查。 |

### 敌人清晰度

| # | 用户反馈 | 实际实现 | 对应证据 |
| --- | --- | --- | --- |
| 28 | 部分 Boss 尤其墨影幻灵分辨率过低 | 直接从原版骨骼、贴图、skin、轨道和物理过程重采样；每个 clip 独立 canvas/pivot/units，极端动作不再压低所有 idle 的像素密度。覆盖全部旧 `sprite_units>10000` 的七个模型，另补 Architect 的正常头部轨道与注册。 | `tools/rebake_enemy_ui13.py`、`Canvas::actor`、bestiary/Architect 注册路径；286 clips／5,340 frames 完整转换和逐项保留检查；实际最终像素解码比较。 |

细节分别见 [前端实现](ui13-front-feedback-0.17.md)、
[战斗与实体键](ui13-combat-feedback-0.17.md)、
[房间与卡牌修改](rooms-ui13-0.17.md) 和
[独立敌人画布](ui13-enemy-registration-0.17.md)。

## 此前明确保留的表现差异

### Doom 的完整表现族

`combat_doom_presentation.hpp`／`combat_doom_ui.inc` 补齐独立 Doom 表现：
源 Hurt 的 100ms 冻结（Inklet 40ms）、750ms 保持＋750ms Expo.In 下沉、
真正的 YIQ 灰度矩阵、前后景洞口／光晕／地面／液滴／八根尖刺、双重噪声
扭曲火焰和覆盖层。不同 Doom 的 shader 使用同一全局时间；目标例外、Test
Subject 复活次数、Waterfall Giant 条件和玩家不冻结的正常死亡均分别处理。
掉落粒子的源 ±350 重力方向和 Kaiser 共用精灵的单体裁剪也纳入最终修正。

目标清理继承仍在衰减的 trauma，同时清除该目标的 punch/rumble；视觉状态
不写入 canonical State 或存档。八张新增原图位于辅助目录 3331..3338。
`d5f9a3c` 与最终压缩资源的英文／日文 screen gate 各 2,122,313 项通过；
EndOfDays、连续 Evoke 与容量变化的实际逐帧 capture 也已闭合，canonical
save 与不带表现 observer 的对照一致。这是原生实际资源检查，不替代设备计时。
有界粒子密度、CPU 噪声取样和 LCD 像素取整仍是平台适配；不承诺桌面 GPU
bloom、无限粒子数量或浮点像素逐点等同。

### Architect 阅读、书写轨迹与真实攻击特效

在已有双角色、对话、伤害、退场流程上，补齐 `_tracks/head_normal`、阅读／
停止阅读、Silent Normal/Fast 的受击打断，以及 `trail_start` 到 `trail_end`
的书写轨迹。199 张阅读帧和 80 张普通／叠加轨迹帧保留源时序与层级。

13 种角色攻击和 FireBurst 共 824 个辅助资源行，使用各自原图、Spine clip、
flipbook、曲线、LUT、子发射器与生命周期。HeavyBlunt／StarryImpact 的位移
数据保留无损精度，通过 20 KiB 的 32 行窗口读取未污染的帧缓冲；入射 Flying
Slash 使用独立横向 pivot；终场黑幕与玩家死亡、FireBurst、书写轨迹的覆盖
顺序分别保留。打击列表和权重使用展示专用的事件随机流副本，重复绘制不推进
canonical RNG。完整双角色＋效果合成 oracle 用于检查大面积 StarryImpact，
取代旧的“必须留下 20 个未被特效遮挡像素”假设。

该功能的生产验证源 `d5f9a3c`、测试源 `b96bd0d` 和配套压缩资源的 Architect gate 已闭合：
英文／简中各 **84,365 checks**，覆盖五角色 × Normal/Fast × 两语言的 20 种
组合。源 anchor、字面攻击／受击帧、完整特效合成中的所有不透明角色像素均
精确比较；死亡覆盖顺序、原文滚动和 canonical save 不变也通过。30 张 capture
均可完整读取。该 fixture 从先前房间结果进入 Architect，不把它记成自然三幕通关。

源特效的固定装饰随机数、60Hz CPU 积分／30Hz 存储、整数屏幕扭曲和有界
turbulence 是明确的平台映射。详见 [Architect VFX](architect-vfx-ui13-0.17.md)。
音频、多人和 Timeline 继续保留原有排除范围。

### Random 选角背景的水波与全部装饰粒子

UI12 的静态 320×240 渐变仅保留为旧资源兼容路径。新实现保留未裁剪的
64×64 原始 cubic gradient，在真实父容器和 pivot 变换后，用原版 repeat
noise、全局 shader TIME、speed 0.023 和 strength 0.1 执行水波采样。
10×5 seamless FastNoiseLite 的原 R8 样本存入 alpha，避免 RGB565 丢失噪声
精度；全透明度为 255 的原 underdocks mask 逐 texel 核对后省去冗余存储。

源 light/dot 两张 256×256 图片保留全部 texel，线性存入 320×205 物理图像，
用逻辑 256×256 视图采样；仅补 64 个透明尾部像素，不裁剪、不重采样，遵守
原有 320×240 解码边界。普通／索引图使用 flat index，只有 sparse 行需要映射。
1＋20＋20 三个发射器均接入。小灯保留
3 秒生命周期、缩放曲线、灰度渐变、加法混合和脚本的 0–5 秒整数预热；
两组 ash 保留 15 秒预热、矩形发射范围、重力、顺时针 orbit、radial force、
原透明度和入场 ramp。40 个移动粒子的展示状态小于 2KiB，不使用游戏 RNG。
水波逐像素整数双线性采样，粒子采用有界 Q10 积分；不把它说成桌面 GPU
浮点粒子逐点等价。见 [Random 背景](random-character-ui13-0.17.md)。

最终 `792904cb` 程序与 `e606108` 资源的实际 Random gate 双语各 **317,696**
项通过；source-free sampler／particle／逻辑视图为 **5,147** 项。完整水波
oracle、动态帧、选角画面和 canonical save 均检查。另在最终 retained format7
资源上逐一核对两张原图的全部 65,536 texel，memory／range 两条生产读取路径
共 **786,733** 项检查通过。新增四张图的压缩 payload 合计 72,654B。
入口动画首帧和停稳的选角画面分别保存，没有把短探针计为最终实际资源验收。

### Fake Merchant 自定义房间的原场景层

补入原背景、商人、刀具的独立 Spine 层及其完整动画循环；房内玩家使用
`Character.CreateVisuals()` 对应普通 atlas 的 relaxed_loop，保留源位置和
比例。原粒子、小灯和 glitter 材质独立接入；glitter 复用同一水波采样器，
使用自己的 50×50 seamless noise、原图／alpha mask 和源速度／强度。
Regent 的 Normal／Add／Normal 源绘制顺序保留在同一 160×196 图集中，眼部
8×6 小区域分三层绘制，其余身体按 Normal 合成。静态与动态像素精确互斥，
每个区域即时读取并绘制，不跨后续解码保留可能失效的 `Picture`。战斗开始后
隐藏商人；事件结束仍使用该自定义房间，获得真地毯后使用完整无地毯源动画。

最终实际 Fake 主门双语各 **69,789,393** 项通过，涵盖 1,800 个动态帧、10 个
静态图、五角色、打开／购买／关闭／地图／finished／战后有无地毯。旧 5,011
目录兼容门另通过 59,710,237 项。八种场景共 14,544 次 `App.render` 的第二／
第三窗口均为 0 源读取，最大工作集 553 个图片条目／1,730 页，约 7.086MB；
使用同一 11MiB arena 的 10/1 分配，没有增加缓存预算。这是 native 工作量，
不等于设备 FPS。完整事件总门也以同一展示时间比较欢迎页背景，最终 ENG／ZHS
5,850／5,827 项均通过，包含真实购买价格、八个 Ancient、历史及普通事件。
详细身份见 [Fake Merchant](fake-merchant-ui13-0.17.md)。
普通 Shop 的持续性能仍是另一个实测场景，不沿用旧静态 Fake 截图。

### 主菜单模糊与标签独立彩虹时钟

主菜单子页面恢复 `BlurBackstop` 的真实 screen-texture filter：进入 250ms，
退出 150ms；LOD 0→3，black mix 0→0.7，中断时从当前数值继续。CPU 版使用
RGB565 mip、双线性及 mip 层之间插值，额外工作区为 50,400 字节有界栈空间，
没有新增完整图像缓存或安装素材。主菜单的设置、档案、百科、模式等页面共用
该滤镜；游戏内覆盖层仍按各自源场景处理。见 [主菜单模糊](main-menu-blur-ui13-0.17.md)。

彩虹文字从共享绝对时间改为标签／ItemFX 独立时间原点，使用少于 768 字节的
32 槽有界表；重复绘制不重复推进，同内容的不同标签独立，稳定 owner 保持
移动、滚动与入场期间的相位。沿用原版参数、局部字形横坐标、Normal/Fast
行为及透明度，状态不进入存档。见 [原版彩虹文本](rich-rainbow-0.17.md)。
最终双语 `world_frontend` 使用真实图片和字体通过 69,870／69,897 项，包含
Settings 右缘 5,100 个像素的独立滤镜比较、控件 mask、入场／退场和中断，
canonical save 不变。最终 CardHUD 同时覆盖七个实际文字 painter 的独立
rainbow 时钟。设备上普通 Ndless 的模糊工作栈和实际交互另行记录。

## 性能、存储与被拒绝的候选

高清源重采样完整执行于 `7bfc391`，耗时 1,132.19s。Vantom 正常 idle 的实际
墨迹由 21×25 提高到 91×102；Ceremonial Beast 为 26×29→96×108，Waterfall
Giant 为 28×28→96×97，Mecha Knight 为 37×28→126×95。所有帧数和时钟保留。

随后完整执行了透明 padding＋pivot 对齐的无损 tile 候选。其 5,340 帧的世界
坐标像素均相等，但每种语言包实际增大 725,442B，部分解码组工作集也增大。
该候选已拒绝，最终选用未 padding 的高清画布。

安全目录清理在 `6e40566` 闭合：删除 3,006 个死 descriptor 和 38 个完全无引用
index group，每种逻辑语言包实际节省 **259,740B**。1,155 个 tile group 与
556 个 live index group 的压缩流逐字节保留；48,294 个 tiled image、2,969 个
retained image、全部字体和 8,704 个字符串以及所有尺寸／pivot／时序不变。
两种清理后基底为 69,915,084B（ENG）和 74,402,887B（ZHS）；它们共享图像，
不是需要同时安装的两个整包。目录清理反映到默认分包安装后节省 307,420B。
较早 1,138 个新原图的组压缩节省 1,205,506B；最终 2,955 个追加图片则使用
127 个压缩组及 1,013 个精确别名，相对逐图布局节省 **9,262,910B**。这两个
数值是不同阶段的口径，不能相加。最终四件套如下，均是实际文件字节数，
未把 ZIP 压缩后的下载体积当成安装体积：

| 文件 | 字节数 |
| --- | ---: |
| `nSpire2.tns` | 1,653,686 |
| `nSpire2.common.tns` | 70,659,306 |
| `nSpire2.eng.tns` | 1,140,675 |
| `nSpire2.zhs.tns` | 5,115,779 |
| 默认双语安装合计 | **78,569,446** |

这相当于 **78.57MB／74.93MiB**，不含存档和可选语言，仍超过原先的 65–70MB
目标。较早 `d5f9a3c` 候选为 76,297,785B，最终补齐的场景和程序净增
2,271,661B；没有继续沿用旧 76.30MB 数值。高清重采样本身增加约 6.258MB，
保留完整动画和真正改善的原图像素，没有通过删帧、修改时序或再次降低精度
来强凑体积。ARM 的分配为 **20,703,800B**，低于原有 24MiB 分配上限；普通
Ndless 的运行、工作栈与持续帧率仍以最终设备记录为准。详细资源身份、兼容
端点和完整源读取门见 [最终资源](ui13-retained-resources-0.17.md)。

真实 Content 文件读取路径检查了九个代表 idle，共 276 帧、连续三遍。
第二／第三遍的源读取 callbacks 和 bytes 都为 0。Vantom 占 324KiB 图片页，
Ceremonial Beast／Waterfall Giant／Frog Knight 各 512KiB，Mecha Knight
524KiB，Architect 344KiB。每个独立重采样 clip 均能装入普通 5MiB 图片池，
最大稀疏帧为 38,108B。这些是实际解码／缓存测量；Firebird 负责包含其他场景
资源和完整渲染的热帧时序。

初轮商人检查在 `b96bd0d`、实际 UI13 ENG 资源上通过 41,569 项断言，包含
2,376 个完整画面逐像素相等、五角色的完整动画循环、打开／关闭／中断、时钟
边界以及 controls／canonical state 一致。关闭库存的三个连续 360 帧窗口中，
第二窗口仍读取 93,904B，第三窗口已经 0 callbacks／0 bytes／0 seeks；后者的
native mean 为 0.536ms，仅用于说明这条原生执行路径的工作量，不换算成 ARM
或实机 FPS。

最终 Firebird 配对性能流程已闭合。两个窗口来自匹配的 snapshot／flash，在
关闭库存的商人房间先运行 120 个模拟秒，再比较额外 60 个模拟秒；通过正常
退出到 OS 后读取累计 ROOM 统计的 B−A，不对 min/max 相减。计时频率为
32,768Hz，程序为最终 `792904cb`，安装资源与最终四件套逐 SHA 一致。

| 额外 60s 热窗口 | UI12 | 最终 UI13 |
| --- | ---: | ---: |
| 平均帧间隔 | 101.7497ms | 33.3353ms |
| 推算帧率 | 9.8280FPS | 29.9982FPS |
| 源读取次数 | 1,205 | 1 |
| 请求源字节数 | 20,364,684B | 12,700B |
| seek 次数 | 1,051 | 1 |

最终窗口为 1,800 帧，平均 render 为 14.9121ms；超过 50ms 的间隔 1 次，超过
100ms／250ms 均为 0。相对 UI12 帧率为 **3.0523 倍**，请求源字节减少
**99.9376%**，原尺寸手和完整动画均保留。报告为 `cp17-merchant-final-delta.json`，
两个通过的 controller journal 为 `9c917389865e`／`318a24bd9498`。这是真实
Firebird 模拟器的普通 Ndless 路径，不能标成物理计算器 FPS 或所有房间帧率。
先前跨 Ndless 执行沿用 USB 连接的一次回读超时仍保留为失败；最终数据来自
独立新 controller 对已保存匹配快照的读取，没有把超时运行混入这些统计。

## 已闭合检查与最终发行衔接

最终程序 `792904cb` 的完整已注册 native suite 为 **137/137 passed、0 failures**，
42.68s，journal `cp17-registered-3b0638b5ead5` 的源码保持不变。最终转换器
`e606108` 的 Python 发现运行 271 项、附加运行 56 项，其中目录清理的 2 项
被重复执行，因此合计 **325 个不同测试**。较早 `792904cb` 的对应数值为
269＋56−2＝323，保留其历史身份；不把重复执行算成新的覆盖。测试修正各自
记录 checkpoint，不改最终程序身份。

最终前端四门在真实 ENG/ZHS 各闭合一次：Random 各 317,696，主菜单／设置／
模糊 69,870／69,897，focused frontend 各 138，profile／Continue／确认框门
均通过。所有实际输入的 SHA 与闭合的 16 语言最终资源 receipt 再次核对。
68 个捕获均完整解码，PNG 与原 PPM 像素相同；13 张关键图逐一目视，包含
双语停稳 Random、全 16 语下拉的首尾、Continue 间距、紧凑确认框和源模糊。
入口动画或压力流程中间帧单独标识，不作为停稳画面的替代。

最终 `resource_content` 实际源读取门在 `21c3cce` 测试源闭合：ENG 为
8,387,948 项、ZHS 为 8,549,992 项，包含全部 2,955 个追加图的 memory／range
读取和 18,225,705 个解码字节，以及完整字体、真实 kerning、字符串、clip 和
40 个 UI 帧。16 语言 `locale_editions_regression` 另通过 **38,686 checks、
53,072 descriptions、0 failures**，每种语言 3,317 条描述，未展开插值均为 0；
journal 为 `ui13-retained-final-all16-native-c8adc28428d3`。这些是真实应用读取
和完整解码检查，独立于 Python 文件目录验证。CardHUD 最终 synthetic／ENG／
ZHS 分别为 **385,902／712,932／790,294** 项，含完整图例文字和七个实际文本
painter 的标签时钟。

最终 USB 验证器已用同一 core 重新链接，六个受控 merchant／Architect 原始与
NSZ1 fixture 都通过生产 decode／validate／save／reload，并拒绝截断输入。
它的二进制及 canonical fixture 结果与较早版本逐字节相同。这只是格式兼容
和验证器检查，不把 fixture 当作自然游玩。

最终自然设备流程 `cp17-natural-final-4881221c7040` 已闭合 passed，源码未变。
在空白进度下通过普通菜单开启 Standard Ironclad，经 Neow 两张牌、第一场战斗
四回合获胜、领取 20 gold 与卡牌、查看地图、保存退出，再通过 Continue 恢复
相同清场局面，最后再次保存退出并正常回到 OS 完成 USB 读回。该流程实际展示
Settings、语言下拉、地图图例和奖励界面，并操作数字键选牌。

两个 USB 文件分别为 7,510B／559B。最终生产 core 重新链接的验证器完成
NSZ1 解压、局面校验、保存、再次读取与再次保存：两种 canonical 输出分别为
230,620B／18,371B，逐字节稳定，原文件不变。局面为 Ironclad、种子
`FD81C60B`、Act 1／floor 2／Monster 已清场、`Reward` phase／turn 4、HP80/80、
gold119、deck13、revision38；进度的种子、角色及保存 revision 与局面一致。
验证 journal 为 `usb-cp17-natural-final-production-e100ef0d38d0`，同样 closed
passed、源码未变。这里验证的是自然首战及保存续玩链路，不宣称自然三幕通关
或物理设备测试。

四个特殊场景在最终程序和资源下分别通过普通 OS 启动、Continue 后验证，全部
controller journal 均 closed passed、源码未变：

| 控制场景 | 实际设备观察 | Journal |
| --- | --- | --- |
| Vantom | 高清完整 idle 和第二回合姿态；数字 2 选择格挡牌并出牌、结束回合、承受 7−5 格挡伤害至 HP78、第二回合 banner 和抽牌。 | `cp17-vantom-display-fa768f5970f6` |
| Architect | 双角色与两句正文，终场攻击和伤害、左侧角色死亡与结束层、总结发现项，返回主菜单后已完成局面不再出现 Continue。 | `cp17-architect-display-71ae3a78b747` |
| Fake Merchant／Regent | 原角色与环境持续运动；Catalog 打开六件商品及价格，关闭后离开；返回已完成房间时 Catalog 不再打开库存或阅读器，仅保留 Proceed。 | `cp17-fake-regent-display-796f1603daec` |
| Fake Merchant／战后无地毯 | 商人和真正的 rug attachment 隐藏，原背景的绿色绘制保留；Catalog 无作用，Proceed 返回可操作地图。 | `cp17-fake-norug-display-1958b368f2a5` |

这些是明确标记的控制 fixture，不能替代自然 boss 进程或三幕通关。Architect
设备截图证明阶段衔接，完整逐攻击／FireBurst 像素合成由独立实际资源 native gate
覆盖；该控制进度的早期历史刻意不完整。

特殊场景展示后，独立 OS controller 恢复原自然局面的配对 snapshot／flash，
重新通过 USB 读取两份自然存档和四个安装文件。设备 journal
`cp17-release-restore-f7c4994628da` 与生产验证 journal
`usb-cp17-release-restore-production-dafcd2baea7d` 均 closed passed、源码未变。
恢复前后的 7,510B 局面与 559B 进度逐字节相同，两种 canonical 输出的改变字节数
均为 0，全部状态字段相同；没有把控制展示的存档混入自然进度。设备读回的程序、
common、ENG、ZHS 与最终发行源文件逐块相同，独立 SHA256 也全部吻合，合计
78,569,446B。最终安装及恢复绑定全部来自这次正常闭合的流程，不依赖早期失败
controller 中单独成功的传输步骤。

### 保留身份的早期检查

`d8a1025` 的配套 UI13 实际 ENG/ZHS 通过前端每语言 138 项检查，以及 profile、
主菜单、Continue 和百科按钮的独立全帧像素比较。这个阶段还发现并修复了
`Content.open` 能识别新资源，而 `App.init` 的旧 profile 特征链未接受新辅助
目录端点的真实集成故障。新增 `has_ui13_assets` 后上述检查重新闭合。

同阶段实际双语 selection 为 1,396,392／1,436,724 项，deck-selection 为
551,381／591,713，keyword 为 635,846／635,881，card marker/facts 各 486,263，
reward 各 4,076，event 为 5,841／5,818；`6e40566` 同生产源的 room gate 各
14,689 项。combat UI 各 154,880 项，ENG split secondary 390,445，英文／日文
screen＋Doom 2,122,313 项。检查次数包含像素及重复状态断言，不等于独立场景数。

`d5f9a3c` 配套 ARM 已构建成功且源码未变；其最终压缩资源的 deck-selection
ENG/ZHS 为 551,382／591,714 项，asset-independent 为 453 项，world UI 各
31,796 项（包含 Toasty），combat UI 各 154,880 项，secondary 为 390,445 项，
均已通过。最终 CardHUD 的独立 tooltip／energy／legend／Settings oracle 保留
逐像素、身份、点击及 canonical 断言，在 `78a2cef` 的 synthetic、实际 ENG、
实际 ZHS 三种运行分别通过 **385,861／712,879／790,241** 项。Architect 完整
合成 oracle 与实际双语检查同样已经闭合。

`b96bd0d` 的完整已注册 native suite 为 **135/135 passed、0 failures**，43.42s。
补入 Random、模糊与标签时钟后的 `2b58ee31` 全套已注册 native suite 为
**137/137 passed、0 failures**，43.63s；这组默认运行仍不替代新增真实素材门。
Python 检查共 **316 项**：`d8a1025` 在配套 conversion venv 中完整发现的 260 项，
加上 `6e40566` 的 56 项附加转换器／日志／动画／目录清理检查，均有闭合 passed
receipt。早先缺少 `lz4` 的错误解释器运行仍保留为失败记录，没有重新标为通过。
`78a2cef` 的全部 `tools/` 还通过配套解释器的 `compileall`，journal
`python-tools-compile-ade5b45fc767` 已闭合且 `source_changed=false`；这项工具
编译检查不重复计入上述 316 项测试。
这些早期检查不替代上文独立闭合的最终商人性能、自然首战及保存续玩、特殊
场景控制展示、自然档恢复和四个安装文件重新读回。旧 UI12 Firebird 结果
保留原有身份，不重标成 UI13。

该阶段压缩分包另由 `locale_editions_regression` 不加语言过滤，逐一完成 16 个
语言的 `App.init`、模型描述插值、实际使用字符的 8／10／12px 原图字形和三种
energy sprite 尺寸检查。`b96bd0d` 的结果为 **38,686 checks、53,072 descriptions、
0 failures**；每种语言 3,317 条模型描述，未展开插值均为 0。日志在测试完成后
从 `/tmp` 复制到私有发行证据目录，journal 为 `ui13-native-all16-locales-c31e3d29a45a`，
`source_changed=false`。这一检查使用真实应用读取路径，独立于 Python 包目录验证。
