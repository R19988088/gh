# Guardian Heroes 第一代 Switch 移植路线

当前实施以 [2026-10-22 详细计划](superpowers/plans/2026-10-22-guardian-heroes-switch.md) 和 [Switch 接入依赖及构建入口](switch-dependencies.md) 为准。本文保留早期架构分析，其中外部 SDK 主导的路径不代表当前项目内 runtime 的构建方式。

本文记录把当前 Xbox 360 XBLA 版 Guardian Heroes 移植到第一代 Nintendo Switch 的技术边界和验收顺序。目标是复用已经生成的游戏逻辑，把 ReXGlue 运行时和平台后端接到 Switch 的构建、窗口、图形、输入、音频和文件系统上。

## 当前代码形态

ReXGlue 的静态重编译器把 Xbox 360 PowerPC 指令提前生成成 C++。生成器的定位是“PowerPC 到可移植 C++”，因此 ARM64 主机可以编译这些源文件；运行时不会再解释或 JIT 编译 PowerPC 指令。

生成结果仍然保留 Xbox 360 的执行语义。`PPCContext`、guest memory、大端访问、间接调用、导入表、线程和异常路径都由 ReXGlue runtime 支持。生成的 C++ 不是恢复后的高层游戏源码，也不是可以脱离 runtime 的普通 C++ 游戏实现。移植到 Switch 不要求先删除这些 PowerPC 语义；第一阶段应保留它们，先完成平台适配。

当前工程的入口文件和生成目录如下：

- Manifest：`/Users/ddd/Documents/xbla-守护英雄/guardian_heroes/guardian_heroes_manifest.toml`
- 生成目录：`/Users/ddd/Documents/xbla-守护英雄/guardian_heroes/generated/default`
- 生成注册和初始化：`guardian_heroes_register.cpp`、`guardian_heroes_init.cpp`
- 生成函数声明与运行时宏：`guardian_heroes_funcs.h`、`guardian_heroes_pch.h`
- 宿主入口：`/Users/ddd/Documents/xbla-守护英雄/guardian_heroes/src/main.cpp`
- 宿主目标配置：`/Users/ddd/Documents/xbla-守护英雄/guardian_heroes/CMakeLists.txt`

生成目录的文件数量会随 SDK 和 manifest 变化；不要在移植代码中按固定文件数量写逻辑。构建系统通过 `generated/rexglue.cmake` 和 `rexglue_setup_target(guardian_heroes GPU_PLUGINS xenos)` 纳入这些产物。

## 重新生成与现有验证

在当前仓库中，使用相邻的 SDK 构建工具重新生成：

```sh
cd /Users/ddd/Documents/xbla-守护英雄/guardian_heroes
../rexglue-sdk-main/out/mac-arm64/Release/rexglue codegen guardian_heroes_manifest.toml
```

也可以让 CMake 在输入变化后触发生成，或单独生成目标：

```sh
cd /Users/ddd/Documents/xbla-守护英雄/guardian_heroes
cmake --preset mac-arm64-release
cmake --build --preset mac-arm64-release --target guardian_heroes_codegen
cmake --build --preset mac-arm64-release --target guardian_heroes --parallel
```

当前 checkout 已有 macOS ARM64 产物：

- `guardian_heroes/out/build/mac-arm64-release/guardian_heroes`
- `guardian_heroes/out/build/mac-arm64-release/librexruntime.dylib`
- `guardian_heroes/out/build/mac-arm64-release/librexgpu-xenos.dylib`
- `guardian_heroes/out/build/mac-arm64-release/libTracyClient.dylib`

`logs/native-1179-validation.json` 记录了当前版本的 Release 构建退出码为 0、链接检查通过，以及 macOS ARM64 启动后观察到标题画面。该记录的范围是启动和标题渲染；它不证明 Switch 构建、整场战斗性能、完整输入、音效或存档已经验收。

当前 CMake presets 只有 Windows、Linux 和 macOS；没有 Switch preset，也没有在仓库中声明 Nintendo SDK、工具链文件或 Switch 专用编译参数。因此文档不预设 SDK 参数。拿到合法的 Switch SDK 和对应工具链后，应新增独立 preset 或外部 toolchain 配置，并保持现有桌面 preset 可复现。

## Switch 移植边界

### 1. 构建与启动入口

保留 `guardian_heroes` 的 C++ 目标、manifest 和生成目录，把 Switch 工具链接入同一目标。平台启动代码需要提供应用生命周期、主线程入口、资源根目录和日志出口。平台相关选择应集中在 ReXGlue 的 platform 层和宿主入口，不能在生成的 `recomp.*.cpp` 中加入 Switch 分支。

第一步的完成标准是：生成源、ReXGlue runtime、游戏宿主和图形插件可以在 Switch 工具链下编译、链接并启动到应用入口。只编译成功而没有进入 guest 主循环，不算完成。

### 2. ReXGlue core 与 guest ABI

需要检查并实现 `rexglue-sdk-main/src/core` 中的以下能力：

- guest memory 的分配、映射、保护和地址转换；
- 大端读写、原子操作、互斥量、条件变量、线程和 fiber；
- 时钟、定时器、动态库、异常和日志；
- 生成代码使用的 `PPCContext`、函数注册和间接分派。

这些接口决定生成 C++ 是否能保持原始调用约定。先让 `PPCContext` 和 guest memory 在 Switch 上稳定工作，再考虑消除任何兼容层。删除它们会把任务从平台移植变成重写整个重编译结果。

### 3. Xbox kernel 与 XAM 兼容层

生成代码通过 `__imp__` 入口调用 Xbox kernel/XAM。对应实现位于 `rexglue-sdk-main/src/kernel`，包括 CRT、`xboxkrnl`、XAM、线程、文件和同步接口。移植时要按本游戏实际导入表逐项确认：

- 已有实现直接复用；
- 依赖桌面 API 的实现改到 Switch 平台服务；
- 当前 stub 只有在游戏路径不需要其输出时才能保留。

不能以“能链接”代替导入行为验证。菜单、控制器、音频、用户数据和存档路径都必须经过实际 guest 调用。

### 4. 图形后端

当前目标通过 `GPU_PLUGINS xenos` 使用 `rexglue-sdk-main/src/graphics` 的 Xenos 命令处理和资源模型，再由桌面 Vulkan 或 D3D12 后端提交。Switch 需要一个符合其图形 API 和资源限制的后端，至少覆盖：

- Xenos 寄存器、纹理格式、采样器、渲染目标和 resolve；
- shader 翻译或预生成、管线缓存和同步；
- guest 内存到 GPU 资源的上传、读回和生命周期；
- presenter、交换链、窗口尺寸和帧同步。

现有 `xenos` 插件可以作为行为参考，不能假设 Vulkan/MoltenVK 的实现可直接带到 Switch。先验证清屏、静态纹理、精灵、文字和一帧完整提交，再进入战斗场景。

### 5. 输入

桌面入口目前使用 SDK 的 SDL 输入路径，Switch 端应提供独立的控制器适配层，并将按键、摇杆、连接状态和帧间重复行为映射到 guest 的输入导出。验收必须包括标题菜单确认、角色移动、攻击、防御、暂停和多人控制器分配；启动日志中出现输入初始化不能代替这些结果。

### 6. 音频

ReXGlue 的音频代码位于 `rexglue-sdk-main/src/audio`，Xbox 侧导出位于 `src/kernel/xboxkrnl/xboxkrnl_audio*.cpp`。Switch 后端需要接住 XMA 解码、混音、声道配置、缓冲提交和暂停恢复。端点创建成功只能说明设备初始化，必须在菜单和战斗中分别确认音乐、音效、音量和长时间播放无断音。

### 7. 文件系统、用户数据与存档

`rexglue-sdk-main/src/filesystem` 和 `src/kernel/crt/file.cpp` 提供 guest 文件访问。Switch 端需要把只读游戏资源、可写用户数据和存档位置分开，并保持 guest 路径、大小写和异步行为的一致性。首个可交付版本至少要能读取 `assets`、创建用户数据目录、写入配置并在退出后重新读取；完整的游戏存档还要单独验证。

## 并行工作分组

可以并行推进以下互相独立的工作流，集成时按“核心 ABI → 图形/输入/音频 → 游戏验收”的顺序合并：

| 工作流 | 主要目录 | 交付物 |
| --- | --- | --- |
| 构建与 runtime | `rexglue-sdk-main/src/core`、`src/rexglue`、`guardian_heroes/CMakeLists.txt` | Switch 工具链构建、入口、guest memory、线程和日志 |
| 图形 | `rexglue-sdk-main/src/graphics`、`src/ui`、现有 `xenos` 插件 | 一帧提交、资源上传、管线和呈现 |
| 输入与音频 | `rexglue-sdk-main/src/input`、`src/audio`、kernel 导出 | 控制器事件、XMA 解码和音频输出 |
| 文件与兼容层 | `rexglue-sdk-main/src/filesystem`、`src/kernel/crt`、`src/kernel/xboxkrnl`、`src/kernel/xam` | 资源、配置、用户数据和存档路径 |
| 游戏验收 | `guardian_heroes/assets`、生成代码和宿主入口 | 启动、菜单、战斗、多人、存档的可重复记录 |

各工作流只修改自己的平台适配目录和接口边界；生成的 `guardian_heroes_recomp.*.cpp`、函数注册表和 manifest 由代码生成工作流统一更新，避免并行分支互相覆盖生成产物。

## 分阶段验收

### 阶段 A：生成代码可编译

- 固定 `guardian_heroes_manifest.toml`、SDK 版本和生成目录。
- Switch 工具链能编译生成 C++、`PPCContext`、guest memory 和注册表。
- 入口能加载 `assets/default.xex` 对应的生成模块，并进入 guest 初始化。

### 阶段 B：运行时与画面

- 完成线程、同步、内存、文件和最小 kernel 导入。
- 图形后端完成清屏、静态资源、文字、交换链和帧同步。
- 画面从标题进入菜单，记录 GPU 错误、未注册函数和异常。

### 阶段 C：可操作游戏

- 验收 Start/A、菜单导航、角色移动、攻击、防御和暂停。
- 验收音乐和至少一组战斗音效，观察长时间运行和切场景。
- 验收单人战斗，再验收多人控制器分配和断开重连。

### 阶段 D：数据与发布包

- 验收配置、用户数据和游戏存档的写入、重启读取及损坏恢复路径。
- 在目标硬件上测量首帧时间、稳定帧率、内存峰值、场景切换和待机恢复。
- 形成可重复的发布包构建、资源清单和运行日志。

只有阶段 D 完成后，才有依据判断是否需要删除部分 guest 兼容层或重写热点函数。即使后续做性能优化，生成 C++ 和 ReXGlue runtime 仍应保留为可回退基线。

## 明确不属于第一阶段的工作

- 把 14,000 级别的生成函数人工改写成高层 C++；
- 删除 `PPCContext`、guest memory、间接分派和导入注册；
- 把 Xenos 命令直接替换为与原语义不等价的渲染路径；
- 在没有目标工具链和硬件证据时声称“已支持 Switch”。

这些工作会改变兼容边界，应在 Switch 版本可以启动、输入、渲染、发声、战斗和存档之后，以实测瓶颈为依据逐模块处理。

