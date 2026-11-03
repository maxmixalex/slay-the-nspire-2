# 安装与更新

[English](installation.md) | **简体中文** · [返回 README](../README.zh-CN.md)

本指南对应 UI25 计算器安装，需要原版 **v0.111.0 / 41cef1ea** 游戏文件和正常工作的 Ndless。
已测试的启动程序、资源与 Firebird 环境见[发行记录](ui25-release-validation.md)。
UI25 是交付批次，游戏内的版本文字可能仍显示 `0.1.0-ui24`。

## 获取程序

在 GitHub Actions 中打开所需源码版本的成功运行记录，从 **Artifacts** 下载程序：

| 所需程序 | 工作流 | 构建产物 / 内部文件 |
| --- | --- | --- |
| 计算器启动程序 | [Native tests and portable builds](https://github.com/maxmixalex/slay-the-nspire-2/actions/workflows/ci.yml) | `nspire2-launcher-<commit>` / `nSpire2.tns` |
| Windows 转换器 | [Windows original-install converter](https://github.com/maxmixalex/slay-the-nspire-2/actions/workflows/converter-windows.yml) | `nspire2-converter-windows-<commit>` / `nspire2-convert.exe` |

启动程序产物包含程序和许可文件，不带 `data/`。转换器在电脑上读取你自己的原版安装，
生成 `data/`。请保留程序附带的依赖许可说明。Linux 转换器或已失效的工作流产物，
可按[源码构建说明](../DEVELOPMENT.md)自行构建。

公开仓库不提供完整游戏数据下载。源码 ZIP、转换器和只有启动程序的产物，都不是
可直接运行的完整计算器游戏安装。

## 转换资源

原版目录需要包含 `SlayTheSpire2.pck`、`release_info.json`，以及位于根目录或
`data_sts2_*` 子目录内的 `sts2.dll`。转换器读取这些文件，不会启动原版游戏。

把 `nspire2-convert.exe` 放进原版目录。Windows 双击即可默认生成中英双语完整动画版；
也可在 PowerShell 中按所需输出选择**其中一条**命令：

```powershell
.\nspire2-convert.exe
.\nspire2-convert.exe --no-animations
.\nspire2-convert.exe --languages all --output .\nSpire2-converted-all-languages
.\nspire2-convert.exe --no-animations --languages all --output .\nSpire2-converted-no-animations-all-languages
```

Linux 独立版先执行 `chmod +x ./nspire2-convert`，再用 `./nspire2-convert` 加相同参数。
下载文件如果带版本后缀，请相应替换命令中的文件名。

| 参数 | 默认输出或用途 |
| --- | --- |
| 不带参数 | 原版目录下的 `nSpire2-converted` |
| `--no-animations` | 原版目录下的 `nSpire2-converted-no-animations` |
| `--languages all` | 在所选输出中生成全部十六种语言 |
| `--languages eng,zhs,jpn` | 仅生成指定语言，必须包含英语 `eng` |
| `--game "PATH"` / `--output "PATH"` | 指定原版 / 输出目录 |
| `--work "PATH"` / `--workers 1` | 指定专用工作目录 / 减少并发进程 |

转换可能需要较长时间和较多临时空间。再次运行同一命令可复用已核实完成的阶段。
需要两个版本时请依次运行，并使用不同的输出目录。[转换器指南](converter-ui23.md#progress-and-resuming)
说明了缓存复用、进度记录和清理方式。最终输出包括 `data/` 与 `converter-receipt.json`；
启动程序需要单独获取。

## 传入计算器

在计算器文档存储中选择一个游戏目录。将启动器放在该目录根部，保留完整的 `data/`
文件夹结构；不要把数据文件全部平铺到根目录。

| 相对路径 | 内容 |
| --- | --- |
| `nSpire2.tns` | Ndless 启动程序 |
| `data/nSpire2.dat.tns` | 八个图像分包的索引 |
| `data/nSpire2.interface.dat.tns` | 界面图像 |
| `data/nSpire2.cards.dat.tns` | 卡牌图像 |
| `data/nSpire2.characters.dat.tns` | 角色图像与动作 |
| `data/nSpire2.enemies.dat.tns` | 敌人图像与动作 |
| `data/nSpire2.bosses.dat.tns` | 首领图像与动作 |
| `data/nSpire2.rooms.dat.tns` | 房间与场景效果 |
| `data/nSpire2.effects.dat.tns` | 公共特效 |
| `data/nSpire2.shared.dat.tns` | 其余公共图像 |
| `data/nSpire2.eng.dat.tns` | 英语文字与字形，建议保留作回退 |
| `data/nSpire2.zhs.dat.tns` | 简体中文文字与字形 |
| `data/nSpire2.<locale>.dat.tns` | 同一次转换生成的可选语言 |
| `state/` | 游戏创建的档案、存档、设置与备份目录 |

**任一动画模式、任一语言都需要全部八个图像分包。**索引、分包与语言文件必须配套。
打开 `nSpire2.tns` 即可开始，在设置中切换已安装的语言。启动时先使用已保存且可用的语言，
否则依次尝试支持的系统语言、英语和其他已安装且可用的语言。

已记录的 UI25 中英完整动画包解压后为 **69,839,495 字节**，精简包为 **29,811,189 字节**，
包含说明与许可证。存档和额外语言还需要更多空间；OS 在接收替换文件时可能暂时保留旧文件，
因此升级过程也可能需要额外空闲空间。

## 可选语言

请将所需语言与整套数据一起转换。即使版本号和动画模式相同，两次独立转换的资源地址
也不一定相同；不要把另一批转换的语言文件混入现有公共数据。

| 代码 | 语言 | 代码 | 语言 |
| --- | --- | --- | --- |
| `eng` | 英语 | `zhs` | 简体中文 |
| `zht` | 繁体中文 | `deu` | 德语 |
| `esp` | 拉丁美洲西班牙语 | `fra` | 法语 |
| `ind` | 印度尼西亚语 | `ita` | 意大利语 |
| `jpn` | 日语 | `kor` | 韩语 |
| `pol` | 波兰语 | `ptb` | 巴西葡萄牙语 |
| `rus` | 俄语 | `spa` | 西班牙西班牙语 |
| `tha` | 泰语 | `tur` | 土耳其语 |

如果你已有本轮三份配套的 UI25 安装归档，只选一个主包。第三份语言包只为这两套
确切的主包数据提供补充，不能单独运行：

| 主包文件名后缀 | 配套语言目录 |
| --- | --- |
| `full-animated-bilingual.zip` | `animated/data/` |
| `no-character-animation-bilingual.zip` | `no-animations/data/` |

从对应目录中仅取需要的 `nSpire2.<locale>.dat.tns`，复制到游戏的 `data/`。
两份主包都已包含英语和简体中文。完整文件名前缀是 `Slay-the-Nspire-2-0.1.0-ui25-`。
这里记录的是已验证安装包的命名，不是公开游戏数据下载链接。

原版文字使用所选语言，原文缺失时按原版回退到英语。本移植新增的历史/归档错误提示
与时间线状态标签目前提供英语、简体中文，其他语言显示这些标签时使用英语。

## 更新并保留存档

1. 正常退出游戏，将**整个 `state/`**备份到电脑。
2. 保留计算器上已有的 `state/`，不要用压缩包内的空目录替换它。
3. 删除旧 `data/`，再传入配套的新启动器和完整新 `data/`。
4. 已安装的可选语言也换成新批次对应文件，完成后再启动。
5. 打开游戏，选定原有档案，使用 Continue / 继续游戏。

三个档案共用安装文件，但进度和对局相互独立。第一档案在 `state/` 中使用
`nSpire2.sav.tns`、`nSpire2.progress.tns` 与 `nSpire2.history.tns`；第二、第三档案
在 `.tns` 前加入 `.p2`、`.p3`，例如 `nSpire2.sav.p2.tns`。保留整个目录还能保留设置、
备份与恢复所需的临时文件。

UI25 可读取 v3–v17 对局存档和 NSHA/更早进度，写入 v18/NSHB。旧启动器可能无法读取
新写入的文件；需要回滚时，把升级前的整个 `state/` 备份与匹配的旧启动器、旧数据一起恢复。
早期未公开版本使用的目录布局不会自动迁移。

## 时间线与档案

用标题界面左上角的档案控制切换三个档案。进入 Timeline / 时间线，Enter 查看所选节点，
对已获取节点再次按 Enter 即可揭示。新档案遵循解锁链；旧档案通过明确标注的兼容访问保留
原有内容，不会编造获取日期或揭示历史。计算器采用按时间排列的列表与滚动详情，
原版抢先体验阶段尚未开放的占位内容仍不可用。

## 长 ASCII 种子

计算器直接输入字段容量为 64 个 UTF-8 字节。更长的 ASCII 种子可完整保存到 `seed.txt`，
然后执行：

```powershell
.\nspire2-convert.exe --encode-seed-file .\seed.txt
```

Linux 使用 `./nspire2-convert --encode-seed-file seed.txt`。也可以直接用
`--encode-seed "complete ASCII seed"`，将引号内容替换为自己的 ASCII 种子。编码无需转换游戏数据。

在自定义模式打开种子字段，使用上/下选择 **Import seed code / 导入种子码**，
再输入输出的 `SEED64-` 代码，不要把它当普通种子文字输入。
[种子参考](seed-ascii-ui24.md)说明了完整身份处理方式，非 ASCII 编码输入会明确报错。

## 常见问题

| 情况 | 检查方法 |
| --- | --- |
| Ndless 无法启动程序 | 先确认 Ndless 能在当前 OS 正常运行，再核对已验证的型号和 OS 范围。 |
| 提示资源缺失或无效 | 重新传输完整 `data/`，检查索引、全部八个分包和英语文件是否来自同一次转换。 |
| 某种语言不可选或加载失败 | 使用同一转换、同一动画模式的语言文件，并在设置中选择。 |
| 传输提示空间不足 | 备份 `state/`、退出游戏，删除旧 `data/`；完整传入新数据后再启动。 |
| 转换中断 | 使用同样的命令与工作目录重试；失败或未完成的阶段不会被当成完成。 |

提交问题时请提供程序源码身份或 `BUILD-INFO`、计算器/OS/Ndless 版本、动画模式、语言
及复现步骤。不要在公开 Issue 附上原版游戏文件、固件、资源包或私人存档。
哪些路径实际通过验证，以[发行记录](ui25-release-validation.md)为准。
