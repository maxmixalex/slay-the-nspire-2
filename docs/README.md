# Documentation / 文档索引

Player entry points: [English README](../README.md) · [中文 README](../README.zh-CN.md).
This index keeps current setup and evidence separate from earlier checkpoints.
玩家安装请从双语指南开始；旧阶段文档保留原始身份，不代表当前版本的验证结果。

## For players / 玩家指南

| Document | What it answers |
| --- | --- |
| [Installation and updates](installation.md) / [安装与更新](installation.zh-CN.md) | 获取程序、转换、传输、可选语言、保留存档与回滚 |
| [Converter guide / 转换器指南](converter-ui23.md) | 独立转换器参数、续跑、缓存和完整源码构建方法 |
| [ASCII seeds / 种子参考](seed-ascii-ui24.md) | 普通种子与长 ASCII 种子码导入的准确行为 |
| [Gameplay capture / 演示采集说明](media/README.md) | README 中真实 Firebird 录制的场景、来源与播放时钟 |

## Current delivery / 当前交付

| Document | Scope |
| --- | --- |
| [UI25 release record](ui25-release-validation.md) | 当前三份配套安装包、来源、完整大小、独立校验、Firebird 流程和限制 |
| [UI25 numeric/hash record](ui25-release-validation.json) | 机器可读的源、产物哈希、执行结果与失败记录 |
| [UI25 player-feedback reconciliation](ui25-player-feedback-reconciliation.md) | 当前源实现、旧反馈边界，以及独立兼容档中已通过的真实触摸板 Custom 启动/退出检查；历史失败的原触发与根因仍未知 |
| [WORK_STATE](WORK_STATE.md) | 当前维护者入口；历次构建、决策和验证原文见[完整历史记录](WORK_STATE-through-ui25.md) |
| [Development](../DEVELOPMENT.md) | 当前目录结构、源码构建、回归测试与证据规则 |

UI25 is the delivery stage. The source `VERSION` and game label may still say
`0.1.0-ui24`; consult `BUILD-INFO` and the release's exact source identities.
Passing source CI, accepted converted resources and emulator execution are
separate claims. Physical-handheld performance has no current measurement.

## Feature and implementation references / 功能参考

These documents describe the implemented feature and the evidence at its stated
checkpoint; a newer package does not silently rerun an older test.

| Topic | Reference |
| --- | --- |
| Timeline, profiles and unlocks | [Timeline](ui24-timeline.md), [numeric evidence](ui24-timeline-validation.json) |
| Necrobinder/Osty animation and death | [UI24 repairs](ui24-necrobinder-osty.md), [UI25 action acceptance](ui25-release-validation.md) |
| Ancient scene performance | [UI24 detailed measurements](ui24-ancient-performance.md), [UI25 final windows](ui25-release-validation.md) |
| Continuous card materials | [Card materials](ui23-continuous-card-materials.md) |
| Paged run history | [History archive](ui23-history-archive.md) |
| Locale behavior and storage | [Locale labels](locale-labels-ui10-0.17.md), [calendar locales](calendar-locales-ui18-0.17.md), [converter](converter-ui23.md) |
| Resource format and architecture history | [Split content](split-content-format.md), [resource source](resource-source-0.17.md) |
| Original UI fidelity and remaining differences | [Fidelity contract](parity.md), [UI23 ledger](ui23-retained-differences.md), [current release boundaries](ui25-release-validation.md) |

## Historical records / 历史记录

| Stage | Entry point |
| --- | --- |
| UI24 accepted packages and recovery | [Release record](ui24-release-validation.md), [numeric record](ui24-release-validation.json) |
| UI23 storage and retained differences | [Storage evidence](ui23-color-storage-evidence.md), [differences](ui23-retained-differences.md) |
| UI22 source precision and storage | [Storage/performance](ui22-storage-performance.md), [source precision](ui22-source-precision.md) |
| UI17 feedback and device acceptance | [Feedback ledger](ui17-feedback-acceptance-0.17.md), [device validation](ui17-final-device-validation-0.17.md) |
| Early 0.17/UI3 | [Delivery](delivery-ui3-0.17.json), [Firebird](validation-ui3-firebird-0.17.json) |
| 0.16 five-character baseline | [Validation](validation-0.16.json), [five-character UI](five-character-ui-0.16.md) |
| 0.12 source recovery | [Historical development report](development.md) |

Older installation layouts, exclusions, size limits, performance numbers and
unfinished-work statements apply to their recorded stage. Use the current
installation guides and UI25 record for today's setup and accepted artifacts.
