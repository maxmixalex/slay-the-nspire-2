# 安装与更新

[English](installation.md) | **简体中文** · [README](../README.zh-CN.md)

## 准备

- 已安装 [Ndless](https://github.com/ndless-nspire/Ndless) 的彩屏 TI-Nspire。
- 自己拥有的《杀戮尖塔 2》**v0.111.0 / 41cef1ea** 游戏文件。
- 一台用于转换和传输文件的电脑，并为转换过程留出临时空间。

模拟器测试使用 Firebird 中的 TI-Nspire CX II CAS、OS 6.4.0.74 和 Ndless r2022。
其他机型和系统版本仍需分别测试。

## 下载程序

打开成功的 GitHub Actions 构建，在 **Artifacts** 中下载：

| 程序 | 构建入口 | 文件名 |
| --- | --- | --- |
| 计算器启动器 | [Native tests and portable builds](https://github.com/maxmixalex/slay-the-nspire-2/actions/workflows/ci.yml) | `nspire2-launcher-<commit>` |
| Windows 转换器 | [Desktop converters](https://github.com/maxmixalex/slay-the-nspire-2/actions/workflows/converters.yml) | `nspire2-converter-windows-<commit>` |

解压后使用 `nSpire2.tns` 和 `nspire2-convert.exe`，并保留附带的许可证。
启动器需要配合转换器生成的数据。Linux 构建或构建附件过期时，可参照
[源码构建说明](../DEVELOPMENT.md)。

## 转换游戏

原版目录需要包含 `SlayTheSpire2.pck`、`release_info.json`，以及直接位于该目录
或 `data_sts2_*` 子目录中的 `sts2.dll`。将转换器放在这里，双击运行，或执行：

```powershell
.\nspire2-convert.exe
```

默认输出为 `nSpire2-converted/data/`，包含完整动画、英语和简体中文。
可按需使用以下选项：

| 参数 | 用途 |
| --- | --- |
| `--no-animations` | 精简角色动画，输出到 `nSpire2-converted-no-animations/` |
| `--languages all` | 生成全部语言 |
| `--languages eng,zhs,jpn` | 仅生成指定语言，须保留 `eng` |
| `--game "路径"` | 指定原版游戏目录 |
| `--output "路径"` | 指定输出目录 |
| `--work "路径"` | 在其他磁盘上使用专用工作目录 |
| `--workers 1` | 降低转换的并发内存占用 |

两种动画版本使用同一个启动器。需要同时生成时，请依次运行，使用不同输出目录。
再次执行相同命令可复用已完成的步骤。缓存和 Linux 用法见[转换器指南](converter.md)。

## 传入计算器

将 `nSpire2.tns` 和整个 `data/` 文件夹放在计算器的同一目录下。
`data/` 中的文件如下：

| 文件 | 内容 |
| --- | --- |
| `nSpire2.dat.tns` | 资源索引 |
| `nSpire2.interface.dat.tns`、`nSpire2.cards.dat.tns` | 界面和卡牌 |
| `nSpire2.characters.dat.tns`、`nSpire2.enemies.dat.tns`、`nSpire2.bosses.dat.tns` | 角色与敌人 |
| `nSpire2.rooms.dat.tns`、`nSpire2.effects.dat.tns`、`nSpire2.shared.dat.tns` | 房间和公共素材 |
| `nSpire2.eng.dat.tns`、`nSpire2.zhs.dat.tns` | 英语和简体中文 |
| `nSpire2.<语言代码>.dat.tns` | 所选的额外语言 |

保留全部八个素材分卷和英语文件。包括额外语言在内，所有文件都必须来自同一次转换。
打开 `nSpire2.tns` 后，在设置中选择语言。

## 语言

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

新增语言时，请与其他数据一起生成。即使版本号和动画设置相同，不同转换的资源地址
也可能不同。移植新增的部分标签仅有中英双语，其他语言下会显示英文。

## 存档与更新

游戏在启动器旁创建 `state/`，保存三个档案、对局、设置、历史和备份。

1. 退出游戏，将整个 `state/` 备份到电脑。
2. 保留计算器中的 `state/`，一起替换启动器和完整 `data/`。
3. 额外语言也要换成这次转换的配套文件。
4. 启动游戏，选择原档案并继续对局。

回退时，请恢复更新前备份的 `state/` 及其配套的旧启动器和数据。
旧启动器可能无法读取新版写入的存档。

## 导入较长的 ASCII 种子

种子超过计算器输入框长度时，将完整文本保存为 `seed.txt`，然后执行：

```powershell
.\nspire2-convert.exe --encode-seed-file .\seed.txt
```

在自定义模式中打开种子输入框，用上下键选择**导入种子码**，
输入生成的 `SEED64-` 代码。请使用导入选项，不要填入普通种子文本框。
编码种子不需要转换游戏数据。

## 常见问题

| 问题 | 处理方法 |
| --- | --- |
| 无法启动 | 检查当前系统上的 Ndless 是否可正常运行。 |
| 提示资源缺失或无效 | 从同一次转换中重新复制整个 `data/`。 |
| 找不到某种语言 | 复制配套的语言文件，再在设置中选择。 |
| 传输时空间不足 | 备份 `state/`，移除旧数据后传入完整的新数据。 |
| 转换中断 | 使用相同命令和工作目录重新运行。 |

报告问题时，请提供构建版本、机型、OS/Ndless 版本、语言和复现步骤。
请勿在公开 issue 中附上游戏数据或固件。
