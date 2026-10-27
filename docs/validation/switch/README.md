# Switch 验证记录

此目录保存当前工程生成的探针、游戏 NRO 和后续 NSP 的版本化证据。每份记录必须包含：

- 主机 commit、ReXGlue codegen 版本、devkitA64/libnx、deko3d、NVK/FFmpeg 依赖版本；
- NRO/ELF/map/NACP 的文件大小、SHA-256 和显示版本；
- Application/title takeover 启动日志、资源目录清单和目标机系统信息；
- 画面、输入、音频、战斗、存档、长跑和性能结果；
- 未通过项目及下一步修复入口。

探针运行或主机侧 NRO 打包不等于游戏已通过。`nro-version-record.md` 只有在真实 Switch 上验证后才能标记 Gate 通过；NSP 记录必须引用已通过的 NRO SHA-256。
