# Guardian Heroes 第一代 Switch 移植进度

更新时间：2026-10-23（RomFS 资源打包和读取已接线；NVK cache 参数修正；Switch FFmpeg SDK 约定为项目内独立工具链目录，完整链接仍待验证）  
目标平台：Nintendo Switch 1 / Atmosphère Application 模式  
当前交付阶段：工具链与平台探针已完成；完整游戏 NRO 尚未完成

## 当前结论

项目已经具备独立的 devkitA64/libnx 工具链目录、Switch CMake 工具链文件、NRO 打包入口、guest arena 映射实现和两个可以打包的目标探针。启动探针和内存探针已有 Application 模式的 SD 日志；另行打出的 `dist/switch-probes-2026-10-22.zip` 只包含两个 NRO 和说明文件，包自身的 README 仍标注为“尚未真机验证”。日志证明对应探针曾在目标机运行，不证明重新打包的归档已再次部署。

当前还没有可以称为“Guardian Heroes 游戏版”的 NRO。现有 NRO 只验证 libnx 启动、日志、Application 模式和 Horizon guest-memory 别名映射；日志明确表明它们不执行游戏 runtime。完整游戏目标被 CMake 的依赖检查主动挡住，避免把探针误报成游戏版本。

本轮已完成多项接线：Switch 入口、`WindowSwitch`/`ViNWindowSurface`、`WindowedAppContext` Horizon adapter 和 Vulkan `VK_NN_vi_surface` 表面路径已接入；资源路径在 Switch 上默认落到 SD 的独立游戏资源和用户数据目录，并支持 NRO RomFS 回退；`Runtime::SetupVfs()` 会保留 `sdmc:/` 挂载前缀；`GuardianHeroesApp::ConfigureRuntime()` 在 Switch 下选择 Npad 输入 factory，桌面继续使用默认输入 factory。runtime 的真实 Switch target 已进入交叉编译，XMA C++ 已通过配置探针。完整游戏 NRO 的剩余硬门槛是：

1. 完成 runtime target 的 Switch 编译，并在真机验证窗口、presenter、主循环和清理路径。
2. NVK 构建脚本已正确关闭 Mesa shader-cache；现有 SDK archive 仍需重建，以消除 fossilize/disk-cache 对 `flockfile`/`funlockfile` 的引用，再完成 Vulkan probe。
3. 为 XMA 音频准备并构建真正的 Switch 目标 FFmpeg `libavcodec`/`libavutil` archive；Switch factory 已接通。
4. 在真机确认 RomFS 的 `default.xex` 读取、SD 开发资源优先级及用户数据写入；NRO RomFS 打包接线已完成。
5. 在真实 Switch 上依次验证 guest 初始化、标题画面、输入、音频、战斗和存档。

## 已完成并有文件证据的部分

### 项目内工具链

所有 Switch 工具和中间依赖都放在项目目录下，没有要求修改系统安装：

```text
toolchains/switch/devkitpro/
toolchains/switch/host/
toolchains/switch/nvk/
toolchains/switch/sources/
toolchains/switch/env.sh
```

当前使用的关键版本和来源：

| 组件 | 当前状态 | 位置或版本 |
| --- | --- | --- |
| devkitA64 | 已安装并用于交叉编译 | 项目内 `toolchains/switch/devkitpro/devkitA64` |
| libnx | 已安装并用于探针 | 项目内 `toolchains/switch/devkitpro/libnx`，4.12.0-1 |
| switch-tools | 已安装 | 项目内 `toolchains/switch/devkitpro/tools/bin`，1.13.1-1 |
| GCC | 已用于 AArch64 目标编译 | devkitA64 r29.1 / GCC 15.2.0 |
| Prototype Switch 参考 | 已固定 | `292de4b` |
| Mesa/NVK 源码 | 已检出并固定 | `d4a00ea0ab3f59afb967cc5d779e4263d237bd77` |
| Mesa Switch 补丁 | 已应用 | `toolchains/switch/sources/prototype-switch/patches/mesa-switch.patch` |
| Horizon ReXGlue 参考许可 | 已保留 | `guardian_heroes/switch/licenses/rexglue-horizon-BSD-3-Clause.txt` |

当前工具链配置位于 [switch-devkitA64.cmake](/Users/ddd/Documents/xbla-守护英雄/guardian_heroes/cmake/switch-devkitA64.cmake)。它设置 `REXGLUE_PLATFORM_SWITCH`、devkitA64 编译器、libnx linker specs、AArch64 编译参数和项目内查找根目录。

### NRO 构建入口

[GuardianHeroesSwitch.cmake](/Users/ddd/Documents/xbla-守护英雄/guardian_heroes/cmake/GuardianHeroesSwitch.cmake) 已提供统一的 `gh_switch_add_nro(...)` 包装：

1. 用 devkitA64 编译 ELF；
2. 用 `nacptool` 生成 NACP；
3. 用 `elf2nro` 生成 NRO；
4. 保留 ELF、map、NACP，便于后续定位目标机链接和崩溃问题。

完整游戏入口由 `GH_SWITCH_BUILD_GAME` 控制，默认关闭。开启后，[GuardianHeroesSwitchGame.cmake](/Users/ddd/Documents/xbla-守护英雄/guardian_heroes/cmake/GuardianHeroesSwitchGame.cmake) 会检查 runtime、Xenos、NVK、XMA、资源和平台 target 是否真实存在；缺少任一项时直接配置失败。

### 已打包的启动探针

构建目录：

```text
guardian_heroes/out/build/switch-aarch64-release/
```

已有产物：

| 产物 | SHA-256 | 已验证范围 |
| --- | --- | --- |
| `guardian_heroes_switch_bootstrap.nro` | `3fbe8afae837b6a7aff38e28674a6144fb8010c11df0e2f559d2d3530adea6a4` | libnx console、SD 日志、Application 模式、进程内存信息、事件循环 |
| `guardian_heroes_switch_memory_probe.nro` | `10c116e79dd8abd4f69b680f426819f4e85e6554d897c0401cfd53f485bbf6a4` | reserve、物理 backing、alias view、4 KiB 偏移、保护、decommit、release、实例清理 |

对应日志文件：

```text
/Users/ddd/Downloads/bootstrap.log
/Users/ddd/Downloads/memory-probe.log
```

启动日志确认 `Applet type=0; mode=Application`，并报告约 3189 MiB 的进程内存预算。内存探针的所有映射与别名测试为 PASS。两份日志的结尾都明确写出 probe only；它们没有覆盖游戏 runtime、异常处理、图形呈现、输入、音频、战斗或存档。

### 2026-10-22 主机侧构建证据

本轮使用项目内 `devkitA64 r29.1`、`libnx 4.12.0-1`、`elf2nro` 和 `nacptool` 完成两个 standalone probe 的 ARM64 交叉构建。构建缓存明确记录 `REXGLUE_PLATFORM_SWITCH=ON`、`GH_SWITCH_BUILD_GAME=OFF`；输出目录保留对应的 `.elf`、`.map`、`.nacp` 和 `.nro`，因此这次构建证明的是目标侧静态链接和 NRO 打包，不是游戏目标构建。`dist/switch-probes-2026-10-22.zip` 只包含两个 probe NRO 和 `README.txt`，没有游戏资源、NVK NRO 或 guest ABI 可执行 NRO。

`memory_probe` 覆盖 reservation、共享 backing、alias、保护、decommit/release 和清理生命周期；`gh_host_memory_switch.cpp` 对可执行/只读保护及文件映射仍返回 `ENOTSUP`，所以该探针不能替代异常分派、代码映射或完整 runtime 验收。

### Horizon guest arena

`gh_guest_arena`、`gh_host_memory_switch.cpp` 和 memory probe 已经形成可独立验证的 Switch 内存基础。当前已证明：

- 可以保留 4.5 GiB 的虚拟 guest arena；
- 新提交的 backing 初始为零；
- 同一 backing 可以通过多个 guest alias view 访问；
- 4 KiB 的 E alias 位移保持一致；
- 一个 alias 的保护或解除映射不会破坏其他 alias；
- decommit/release 会删除 view 访问但保留预期的共享 backing 生命周期。

这些结果只证明平台内存机制可行，不能代替 runtime 启动或游戏可玩性验收。

## 当前代码结构

### 游戏生成代码和项目 runtime

Guardian Heroes 自己的生成代码仍位于：

```text
guardian_heroes/generated/default/
```

`generated/default/sources.cmake` 是当前 guest 源集合。生成代码继续依赖 `PPCContext`、guest memory、大端访问、间接调用、导入表、线程和异常路径；Switch 接入必须保留这些语义，不能用启动探针或一个独立 demo 代替。

项目已经把大量桌面 runtime 代码收进：

```text
guardian_heroes/runtime/
guardian_heroes/src/gh_*.cpp
```

但顶层 Switch 分支目前在 [CMakeLists.txt](/Users/ddd/Documents/xbla-守护英雄/guardian_heroes/CMakeLists.txt) 进入 `GuardianHeroesSwitch.cmake` 后提前返回，因此桌面分支中的 `rexruntime`、`rexui`、`rexaudio`、`rexgpu-xenos` 等 target 不会自动创建。`GuardianHeroesSwitchGame.cmake` 目前按真实 target 检查，故完整目标仍然停在配置阶段。

### 当前 Switch 入口

[guardian_heroes_switch_main.cpp](/Users/ddd/Documents/xbla-守护英雄/guardian_heroes/switch/guardian_heroes_switch_main.cpp) 已按 libnx 应用入口的方向准备，负责：

- 初始化 cvar 和早期日志；
- 创建项目 app context；
- 查找注册的 `GuardianHeroesApp` factory；
- 运行 `OnInitialize()` 和主消息循环；
- 通过 libnx 的退出路径结束应用。

入口现在使用项目内新增的 `SwitchWindowedAppContext` 与 `windowed_app_context_switch.h`。adapter 实现 `hidInitialize`、`appletLockExit`、`appletMainLoop`、待处理 UI 回调和退出清理；它只解决应用生命周期接线，不代表窗口或图形呈现已经完成。

### 输入

[gh_switch_input_driver.cpp](/Users/ddd/Documents/xbla-守护英雄/guardian_heroes/src/gh_switch_input_driver.cpp) 和对应头文件已经实现 libnx Npad 到项目 XInput 形状状态的转换，包含：

- Npad 初始化和连接枚举；
- 按键、摇杆、采样序号和 keystroke；
- 控制器能力查询；
- 振动句柄接口。

源文件已独立通过 devkitA64 交叉编译，当前还没有接到 `GuardianHeroesApp::ConfigureRuntime()` 的 Switch factory，也没有进入完整游戏 NRO 链接。

### 音频

[gh_switch_audio.cpp](/Users/ddd/Documents/xbla-守护英雄/guardian_heroes/src/gh_switch_audio.cpp) 和对应头文件已经提供 libnx `audout` 输出方向的实现：

- 48 kHz、六声道 guest callback 输入；
- 大端 float 解包；
- 折叠为 Switch `audout` 可接受的立体声 PCM16；
- open/submit/pause/resume/close；
- 设备错误回调。

源文件已独立通过 devkitA64 交叉编译，`GuardianHeroesApp::ConfigureRuntime()` 也已选择 Switch audio factory。`config.h` 已显式选择 AArch64 XMA 配置，XMA C++ 的三个主要翻译单元交叉编译通过；但 FFmpeg C archive 尚未按 Switch/newlib 真实配置构建，不能据此认定音频完整可用。

### 图形和 NVK

项目已经有 Xenos/Vulkan 代码和独立 Vulkan probe 入口，但 Switch 游戏链接要求 Horizon NVK 静态库，不能直接使用 macOS/Linux 的 Vulkan loader、MoltenVK 或桌面 `libvulkan`。

NVK 专用 Mesa/NVK Switch 交叉构建已完成，静态库和头文件保存在 `toolchains/switch/nvk/sdk/`：

```text
toolchains/switch/nvk/sdk/include/vulkan/vulkan.h
toolchains/switch/nvk/sdk/lib/libvulkan.a
toolchains/switch/nvk/sdk/lib/libnvk.a
```

源码固定在 Mesa `d4a00ea0...`，并应用 Prototype 参考项目的 Switch 补丁。Vulkan probe 已通过 CMake 配置并编译，但最终 ELF 链接失败：

```text
GNU ld 曾报告 fossilize 对 `flockfile`/`funlockfile` 未定义。NVK 构建脚本现已将 `-Dshader-cache=false` 放在有效的 Meson 命令中，但当前 SDK archive 尚未用修复后的脚本重建；旧 probe 链接仍失败，因此 NVK NRO 尚未生成，也未在 Switch 设备验证。
```

因此当前还没有能成功链接进 NRO 的 Switch Vulkan 库，也没有完成 NVK 清屏、交换链、同步或 Xenos 标题画面验证。

### 资源和文件系统

桌面 `GuardianHeroesApp::SetupEnvironment()` 默认从可执行文件旁边寻找：

```text
assets/default.xex
```

当前 `assets` 资源约 946 MiB。NRO 不能依赖桌面 executable folder 语义；游戏目标现把资源目录打入 RomFS，并保留 SD 开发路径：

- 开发验证阶段可使用 `sdmc:/switch/guardian-heroes/assets/`；
- 发布包可把只读资源放入 RomFS，并把用户数据/缓存放入 SD 的可写目录；
- runtime 的 `game_data_root`、`user_data_root`、`cache_root` 和存档路径必须保持区分。

游戏 NRO 构建入口现将 `assets` 作为 RomFS 目录打包，启动时挂载当前进程 RomFS；资源查找优先使用 SD 开发目录，未找到 `default.xex` 时回退至 `romfs:/`。用户目录仍单独位于 SD。该路径尚未在真机验证。

Switch 默认目录为：

```text
sdmc:/switch/guardian-heroes/assets/  # 开发时可覆盖 RomFS 资源
romfs:/                              # 打包后的 assets 内容根
sdmc:/switch/guardian-heroes/user/   # 存档、缓存和用户数据
```

`game_data_root`、`user_data_root` 和配置路径仍可由 cvar 覆盖；Switch VFS 初始化不会再把 `sdmc:/` 当作相对 POSIX 路径拼接当前工作目录。资源目录仍需在真机上部署并由完整 runtime 实际读取。

## 并行工作流状态

| 工作流 | 当前状态 | 下一出口 |
| --- | --- | --- |
| 工具链/NRO | 已完成探针级别 | 固定可复现构建命令，保留完整游戏 ELF/map |
| guest arena/host memory | 探针通过 | 接入 runtime 的分配、保护和别名 API |
| app entry/window context | Horizon Window/VI surface/context 已接入；入口可交叉编译 | 完成整套 runtime 编译和真机生命周期验证 |
| 输入 | Npad driver 已接入 `ConfigureRuntime()` 的 Switch factory；应用尚未完成完整链接 | 完整链接后做标题菜单、战斗和控制器断开/重连验证 |
| 音频 | audout 端点与 factory 已接入；XMA C++ 翻译单元交叉编译通过 | 构建 Switch FFmpeg archives，再验证设备输出和长时播放 |
| NVK | 构建脚本已修正为真正关闭 shader cache；现存 archive 未重建，旧 probe 链接仍缺 `flockfile`/`funlockfile` | 重建 Mesa archive，重试 Vulkan probe 链接及清屏/交换链验证 |
| Xenos Vulkan | 桌面代码存在 | 对照 NVK feature，接通 presenter、resolve、EDRAM、同步 |
| 资源/VFS | SD 开发目录优先、RomFS 根回退、NRO RomFS 打包及独立 SD 用户目录已接线 | 真机验证资源读取、用户数据和存档写回 |
| 游戏 app target | runtime target 可进入构建图；XMA C++ 编译探针通过 | 用已配置的 NVK/FFmpeg SDK 重跑全量构建，修复后续实际编译/链接错误 |
| NSP forwarder | 尚未开始 | NRO 真机可玩性通过后再分配独立 Title ID 并打包 |

## 构建和验证入口

### 当前可复现的探针构建

下面命令只构建当前已经可用的 bootstrap/memory probe：

```sh
cd /Users/ddd/Documents/xbla-守护英雄
source toolchains/switch/env.sh
cmake -S guardian_heroes \
  -B guardian_heroes/out/build/switch-aarch64-release \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=guardian_heroes/cmake/switch-devkitA64.cmake \
  -DREXGLUE_PLATFORM_SWITCH=ON \
  -DGH_SWITCH_BUILD_GAME=OFF
cmake --build guardian_heroes/out/build/switch-aarch64-release \
  --target guardian_heroes_switch_bootstrap_nro \
           guardian_heroes_switch_memory_probe_nro
```

输出目录中的 `.elf` 和 `.map` 应保留用于定位问题。把这两个探针复制到 Switch 并通过 Application 模式启动，可以复现已有日志；它们不执行 Guardian Heroes。

### 完整游戏构建准入条件

完整构建只有在下列条件都满足后才应开启：

```sh
test -f "$REXGLUE_SWITCH_NVK_SDK/include/vulkan/vulkan.h"
test -f "$REXGLUE_SWITCH_NVK_SDK/lib/libvulkan.a"
test -f "$REXGLUE_SWITCH_FFMPEG/lib/libavcodec.a"
test -f "$REXGLUE_SWITCH_FFMPEG/lib/libavutil.a"
```

随后需要让 `GuardianHeroesSwitchGame.cmake` 中列出的 target 全部由真实源码创建，再执行：

```sh
cmake -S guardian_heroes \
  -B guardian_heroes/out/build/switch-aarch64-release \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=guardian_heroes/cmake/switch-devkitA64.cmake \
  -DREXGLUE_PLATFORM_SWITCH=ON \
  -DGH_SWITCH_BUILD_GAME=ON \
  -DREXGLUE_SWITCH_NVK_SDK="$REXGLUE_SWITCH_NVK_SDK" \
  -DREXGLUE_SWITCH_FFMPEG="$REXGLUE_SWITCH_FFMPEG"
cmake --build guardian_heroes/out/build/switch-aarch64-release \
  --target guardian_heroes_switch_game_nro
```

这段命令目前是目标接口和验收模板，不代表当前环境已经满足依赖。不能通过把探针重命名为 `guardian_heroes_switch_game.nro` 来绕过准入条件。

## 下一阶段验收门槛

### Gate A：runtime 和窗口

- Switch 入口不再引用不存在的类型；
- app factory 注册、`OnInitialize()`、主消息循环和清理路径在目标机运行；
- 日志可写入 SD；
- `Window::Create` 返回实际 Horizon window；
- app 不因资源根目录或桌面 SDL 初始化失败。

### Gate B：NVK 清屏和一帧呈现

- 创建 Vulkan instance/device/queue；
- 创建 VI surface/swapchain；
- 固定颜色持续刷新；
- 提交、等待、呈现和退出事件没有 hang；
- 记录实际 NVK feature，不能只以 `libvulkan.a` 存在作为通过。

### Gate C：guest 初始化和标题画面

- guest arena、PPCContext、导入表和线程创建成功；
- `default.xex` 被实际读取；
- Xenos command processor 能完成至少清屏、静态纹理、精灵/文字和一帧完整提交；
- 标题画面截图与 SD 日志同时保存。

### Gate D：输入、音频和战斗

- 标题菜单确认/返回；
- 单人移动、攻击、防御、暂停；
- 至少两个控制器的分配和断开/重连；
- 菜单音乐、战斗音效、音量和长时间播放无断音；
- 运行一场完整战斗并记录 GPU/CPU/音频异常。

### Gate E：存档和发布 NRO

- 写入用户目录、退出后重启并读回；
- 明确 SD/RomFS 资源和可写数据目录；
- 生成独立 NRO 包、ELF/map、版本和安装说明；
- 只有在 Gate A–D 通过后，才进入 NSP forwarder、NACP/NPDM 和独立 Switch Title ID。

## 参考资料的使用范围

`prototype-switch` 提供了 Horizon runtime、NRO CMake、NVK/Mesa 和 forwarder 的工程参考，但它的生成代码、manifest、函数地址、native renderer、补丁和 Xbox Title ID 都不能直接用于 Guardian Heroes。

文章[《Porting an open-source PC game to Switch》](https://hater.cc/2026/06/26/porting-an-open-source-pc-game-to-switch/)及其 `sdl3-switch` 参考说明了 SDL3 在 Switch 上的窗口、输入、音频和构建接入方式。它对平台边界有参考价值，但不能代替 Guardian Heroes 的 ReXGlue runtime、Xenos/NVK 能力和 XMA 音频验证。文章使用的 `CMAKE_SYSTEM_NAME=NintendoSwitch` 与当前供体工具链的 `Generic` 设置不同，项目统一以 `REXGLUE_PLATFORM_SWITCH` 为主条件，避免两套平台判定互相漏接。

## 当前限制

- 没有完整游戏 NRO；
- 当前 NVK `libvulkan.a` 尚未用修复后的 shader-cache 设置重建，devkitA64 probe 链接仍失败；
- 没有已验证的 Switch 目标 FFmpeg archive；当前 C++ 配置选择复用了 Linux AArch64 宏集，完整 FFmpeg 目标构建尚未验证；
- app 入口、RomFS/SD 资源路径和 HID adapter 已接线，但实际窗口/presenter ABI 及设备生命周期尚未验证；
- 尚未证明 NVK 能覆盖 Guardian Heroes 的 shader、EDRAM、resolve、同步和呈现路径；
- 尚未完成 Switch 真机上的标题、战斗、输入、音频或存档验收；
- 尚未分配或打包 NSP forwarder Title ID；Xbox `0x5841000B` 仍只作为游戏内部 ID，不能直接用作 Switch Title ID。

因此当前准确状态是“Switch 平台基础和探针已验证，游戏 NRO 的 runtime/NVK/FFmpeg 链接与真机验收未完成；RomFS/SD 资源接线已进入待设备验证阶段”。

