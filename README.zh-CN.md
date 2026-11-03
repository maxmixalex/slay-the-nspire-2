# Slay the Nspire 2

[![CI](https://github.com/maxmixalex/slay-the-nspire-2/actions/workflows/ci.yml/badge.svg)](https://github.com/maxmixalex/slay-the-nspire-2/actions/workflows/ci.yml)
[![License: GPL-3.0-or-later](https://img.shields.io/badge/license-GPL--3.0--or--later-blue.svg)](COPYING)

[English](README.md) | **简体中文**

将 **Slay the Spire 2（杀戮尖塔 2）**带到运行 Ndless 的 TI-Nspire 计算器上。
这个非官方原生移植在 320×240 屏幕上提供单人战役、五名角色、计算器操作、
三个独立存档与时间线解锁流程。

当前 **UI25** 交付对应原版 **v0.111.0 / 41cef1ea**。你需要自行准备这个版本的
原版游戏安装；公开项目提供启动程序和转换工具，不包含原版游戏资源或计算器固件。

<p align="center">
  <img src="docs/media/gameplay.gif" alt="UI25 Firebird 连续录制：亡灵契约师挥刀、Osty 出击，随后 Offering 使双方死亡" width="320">
</p>

**UI25 · 亡灵契约师与 Osty**：Firebird 连续 LCD 实录。战斗由游戏内控制台预设，
按模拟器时间回放，不代表实体机帧率。[采集说明](docs/media/README.md)。

**[安装与更新](docs/installation.zh-CN.md)** ·
**[转换器指南](docs/converter-ui23.md)** ·
**[开发说明（英文）](DEVELOPMENT.md)** ·
**[发行验证记录](docs/ui25-release-validation.md)**

## 可以玩到什么

- 三幕单人路线，包含 Ironclad、Silent、Defect、Regent 和 Necrobinder 五名角色，
  以及各地区的战斗、事件、古神与地图历史。
- 57 个纪元的时间线，连接正常获取、揭示流程和持久化解锁状态。
- 默认英语和简体中文，可选其余十四种原版语言。
- 完整采样的角色动画，或保留必要姿态与原动作时长的精简版。
- 三个独立档案、对局存档、设置、图鉴与分页历史；触摸板和键盘均可操作并重新映射。

音频、多人游戏、云存档与在线排行榜尚未实现。剩余画面差异、首次加载停顿与
验证覆盖范围见[发行记录](docs/ui25-release-validation.md)。
本次普通 Firebird 检查在独立旧格式兼容档中，通过真实触摸板完成自定义对局的启动、
保存和退出，返回 OS 后观察 30 秒未自动重启。历史失败的原触发条件与原因仍未知；
具体前置和输入时序见[反馈记录](docs/ui25-player-feedback-reconciliation.md#touchpad-custom-run-check-and-historical-exit-issue)。

## 选择版本

两个版本共用 `nSpire2.tns`。启动程序从数据自动识别动画模式，无需换专用启动器或调整设置。

| 版本 | 画面表现 | 已验证的 UI25 安装大小 |
| --- | --- | ---: |
| **完整动画版** | 保留角色与敌人的采样动作动画 | **69,839,495 字节**，约 69.84 MB |
| **精简人物/怪物动作版** | 保留必要姿态与相同动作时长，部分场景特效保持静态 | **29,811,189 字节**，约 29.81 MB |

这是中英双语完整安装包的**解压后总大小**，包含说明和许可证，不含之后产生的存档
与额外语言；不是 ZIP 下载大小。精确文件身份见[安装包记录](docs/ui25-release-validation.md#three-matching-packages)。
UI25 是本轮交付与验证批次，游戏内可能仍显示 `0.1.0-ui24`；请以包内 `BUILD-INFO`
和记录中的启动程序身份确认版本。

## 快速开始

### 1. 获取程序

准备已正常安装 [Ndless](https://github.com/ndless-nspire/Ndless) 的彩屏 TI-Nspire、
`nSpire2.tns` 启动程序，以及与其配套的电脑端转换器。

| 程序 | 公开构建入口 |
| --- | --- |
| 计算器启动程序 | [Native tests and portable builds](https://github.com/maxmixalex/slay-the-nspire-2/actions/workflows/ci.yml) |
| Windows 转换器 | [Windows original-install converter](https://github.com/maxmixalex/slay-the-nspire-2/actions/workflows/converter-windows.yml) |
| 自行从源码构建 | [开发说明](DEVELOPMENT.md) |

打开所需源码版本的成功工作流记录。Windows 构建产物名为
`nspire2-converter-windows-<commit>`，其中包含 `nspire2-convert.exe` 与依赖许可说明。
启动程序产物的具体获取步骤见[安装指南](docs/installation.zh-CN.md#获取程序)。
公开下载不含原版资源，完整游戏数据需要从你自己的原版安装转换。

UI25 已记录的模拟器验证环境是 **Firebird、TI-Nspire CX II CAS、OS 6.4.0.74、
Ndless r2022**。这不等于实体机验证，也不表示所有 TI-Nspire 型号都已确认兼容。
[环境与结果](docs/ui25-release-validation.md)与编译结果分别记录。

### 2. 转换原版资源

把独立转换器放进原版游戏目录。工具内置需要的运行时，使用者无需另装 Python 或 .NET。
默认生成中英双语完整动画版；Windows 可直接双击 `nspire2-convert.exe`，也可执行：

```powershell
.\nspire2-convert.exe
```

Linux 下载后先赋予执行权限，再运行：

```bash
chmod +x ./nspire2-convert
./nspire2-convert
```

添加 `--no-animations` 可生成精简动作版；添加 `--languages all` 可生成全部十六种语言。
若下载文件带版本后缀，请在命令中使用实际文件名。转换器生成完整 `data/` 目录和转换记录，
**不会生成启动程序**。指定原版位置、语言选择与中断续跑方法见[转换器指南](docs/converter-ui23.md)。

### 3. 传输并启动

将 `nSpire2.tns` 和转换器生成的**整个 `data/` 文件夹**放进计算器上的同一个游戏目录，
打开 `nSpire2.tns`。游戏会创建保存档案和存档的 `state/`。全部八个图像分包都需要保留，
英语文件也应保留作回退；在设置中选择已安装的语言。

**更新前先退出游戏，并备份整个 `state/`。**保留这个目录，用配套的新版本整体替换启动器
和 `data/`，已安装的额外语言也一并更换。不要混用不同转换批次或动画模式的语言文件。
[安装指南](docs/installation.zh-CN.md)列出了完整目录、可选语言对应关系和回滚步骤。

## 操作

| 计算器按键 | 操作 |
| --- | --- |
| 触摸板边缘 / 中央 | 导航 / 点击 |
| `1`–`9`、`0` | 选择十个手牌槽位 |
| `Enter` | 瞄准或确认 |
| `Esc` | 返回或暂停 |
| `Menu` | 结束回合 |
| `Tab` | 地图 |
| `Doc` | 牌组 |
| `Flag` | 开发者控制台 |

两组输入映射均可在设置中修改。自定义/每日模式的原生确认/Menu 操作在松开时开始对局；
Enter 保留当前字段或角色选择的用途。标题菜单可进入时间线：Enter 查看节点，
已获取的节点再次按 Enter 即可揭示。

较长的 ASCII 种子可由电脑端转换器编码，在自定义模式种子字段中明确选择
**Import seed code / 导入种子码**后输入。完整步骤见[长种子导入](docs/installation.zh-CN.md#长-ascii-种子)。

## 开发与验证

[DEVELOPMENT.md](DEVELOPMENT.md)说明目录结构、构建与测试方式。
[文档索引](docs/README.md)区分当前发行、功能参考和历史记录；
[WORK_STATE.md](docs/WORK_STATE.md)用于维护者接续工作。

UI25 发行记录分别列出已测启动程序、转换产物、归档校验与完整/精简模式的 Firebird 流程。
本机回归测试、模拟器测量和实体计算器性能属于不同证据；不宣称统一帧率或与原版完全等价。

## 许可与致谢

项目源码采用 [GPL-3.0-or-later](COPYING)。再分发程序时请保留
[NOTICE.md](NOTICE.md)和适用的[第三方许可](licenses/)。Slay the Spire 2 及其资源
属于 Mega Crit 和各自权利人；本项目为非官方移植，未获 Mega Crit 背书，源码许可不授予
重新分发原版游戏内容的权利。

感谢 [Ndless](https://github.com/ndless-nspire/Ndless)、
[Firebird](https://github.com/nspire-emus/firebird)与
[nZATO](https://github.com/zyf722/nZATO)。nZATO 的离线内容转换与原生运行方式，
以及面向玩家的仓库呈现，为本项目提供了参考。
