# Guardian Heroes 第一代 Switch 移植实施计划

日期：2026-10-22  
目标平台：Nintendo Switch 1 / Atmosphère，开放的 devkitPro、devkitA64、libnx 工具链。  
目标产物：先交付可在 Application 模式运行的 NRO，再交付启动该 NRO 的 HOME 菜单 NSP forwarder。

## 1. 技术决策

保留 Guardian Heroes 自己的生成代码、guest ABI、项目内 runtime、原生替换和 Xenos/Vulkan 实现，把 Prototype 所用 Horizon fork 的平台能力移入现有边界。先证明 libnx 启动与 NVK 呈现，再接 guest 内存、游戏初始化、标题、输入、音频、战斗和存档。

不直接用外部 SDK 整体覆盖当前 runtime。当前项目已经从早期的外部 ReXGlue 接入，发展为 `guardian_heroes/runtime` 加 `src/gh_*` 的项目内运行时；整体替换会丢失已经迁入的内存、音频、GPU、文件和控制器行为。Horizon fork 是平台实现的供体和对照基线，不能当作无差异升级包。

新增文章的主要价值是 SDL3/libnx 接线、静态链接、运行模式、资源路径和安装包调试。文章的 EGL/GLES2 渲染方案不代替本项目的 Xenos → Vulkan → NVK 路径。第一版优先复用 Horizon fork 的窗口、输入、音频后端，不同时维护 SDL3 Switch 视频和直接 libnx 视频两套入口。

实施顺序：

```text
依赖版本与许可边界
  → devkitA64 / GCC 与现有代码兼容性
  → libnx 日志 NRO
  → NVK 独立清屏
  → guest 内存 / 线程 / 导入 / 资源
  → Guardian Heroes 标题画面
  → 输入与音频
  → 战斗、存档、性能与待机恢复
  → NRO 发布包
  → NSP forwarder
```

首个完整交付以单人离线游戏为门槛；本地多人单独验收，不把在线 Xbox 服务移植混入首版。降分辨率、改变帧率上限、减少特效、改按键语义或改变战斗时序，均先说明影响并取得确认。

## 2. 当前工程的实际接入点

本节依据本次读取的源码；旧 `docs/switch-porting.md` 中外部 SDK 主导的路径不能直接作为现状。

| 接入点 | 当前实现 | Switch 工作 |
| --- | --- | --- |
| runtime 选择 | `guardian_heroes/CMakeLists.txt` 默认 `GUARDIAN_HEROES_NATIVE_ONLY=ON`，包含 `GuardianHeroesStandaloneBootstrap.cmake` | 保留默认架构，移入 Horizon 平台实现 |
| 生成代码 | `generated/default/sources.cmake`，`guardian_heroes_recomp` 对象库 | 优先编译现有生成代码；只有 ABI/生成器兼容性需要时才重新生成 |
| codegen | `GUARDIAN_HEROES_CODEGEN_EXECUTABLE` 是现有离线主机工具入口 | 主机执行生成器，Switch 工具链只编译生成 C++ |
| 编译器限制 | `runtime/CMakeLists.txt` 明确拒绝非 Clang，且全局使用 `-ffp-model=strict` | devkitA64 是 GCC，必须先解决真实编译兼容性，不能只删拒绝条件 |
| 应用入口 | `cmake/GuardianHeroesApp.cmake` 固定加入 `windowed_app_main_sdl.cpp` | 按 Switch 条件选择 startup，保留 `GuardianHeroesApp` 生命周期 |
| 窗口/UI | `runtime/src/ui/CMakeLists.txt` 包含 SDL window/context，非 Win/Mac 落入 GNU/Linux surface | 增加明确 Switch 源集合，阻止误选 Linux surface 和桌面依赖 |
| 内存 arena | `src/gh_guest_arena.cpp` 的 Switch 分支明确返回 `Guest arena mapping is not implemented for libnx` | 实现实际映射；这是进入 guest 前的硬阻塞 |
| host memory | CMake 对非 Windows 仍选 `src/gh_host_memory_posix.cpp` | 新增 Horizon 实现并保留现有接口 |
| 音频 | `guardian_heroes_audio_sdl` 链接 SDL3；factory 只有 SDL/Nop；XMA 使用 libavcodec/libavutil | 接实际 Switch 输出，保留 XMA/混音/guest 回调 |
| 输入 | runtime 默认编译 SDL driver；项目有控制器分配 adapter | 接 libnx driver，沿用 slot 分配及 guest XInput 行为 |
| GPU | `cmake/GuardianHeroesGpu.cmake` 已把大量 GPU 实现迁入 `src/gh_gpu_*` | 在项目 GPU 上接 Horizon presenter 与静态 NVK |
| 资源根目录 | `GuardianHeroesApp::SetupEnvironment()` 使用 exe/assets、用户目录和现有 cvar | Switch 明确使用 SD 路径，读资源与写数据分离 |
| continuation | 另有 `GuardianHeroesTitleContinuation.cmake` 及运行时入口 | 第一条可运行基线使用现有 `guardian_heroes`；不同时切换 guest 执行模型 |

特别注意：`gh_guest_arena.cpp` 的 arena 大小为 `0x120000000`，即 4.5 GiB **虚拟地址范围**，包含别名视图；不能据此分配 4.5 GiB 实体内存，也不能把它简化为互不共享的多个 buffer。

## 3. 参考项目核对结果

### 3.1 Prototype Switch：可以借鉴到什么程度

已阅读仓库 README、`prototype/CMakeLists.txt`、工具链、构建脚本、Mesa 脚本和 forwarder 脚本。

- README 指定 SDK 基线为 `nebadasSwifty/masseffect-nx` 的 `a85b0b4`，叠加 `patches/rexglue-sdk-masseffect-nx.patch`。
- Mesa 基线为 `danfromtico/mesa-switch` 的 `d4a00ea0ab3`，叠加 `patches/mesa-switch.patch`；并非安装桌面 Vulkan SDK 即可。
- 工具链使用 `CMAKE_SYSTEM_NAME=Generic`、`REXGLUE_PLATFORM_SWITCH=ON`、devkitA64 GCC、libnx specs、静态链接和专用 TLS 参数。
- NRO 入口确实是 `rexglue_switch_add_nro(...)`；runtime startup 有 `rexglue_switch_startup` target。
- `prototype/build.sh` 把主机 codegen 和目标交叉编译分开，但还执行 Prototype 专用的生成代码修改脚本，这些脚本不能搬进 Guardian Heroes。
- README 明确说明采用专用 native renderer。其可玩状态没有证明 Guardian Heroes 的通用 Xenos/Vulkan shader、EDRAM、resolve 能直接跑通。
- README 的帧率数据来自特定 OLED/Mariko 超频配置，不用于本项目默认频率性能承诺。
- forwarder 依赖 `Forwarder-Mod` 的 `9853bd9` 和同目录 `nsp_container.py`，不是复制一个 Python 文件就能完成打包。

这些版本号来自参考项目声明，实施时解析为完整 commit 并记录依赖来源。外部 `master` 是可变引用，不把本次读到的内容称作已冻结构建基线。

### 3.2 新增文章：采纳与修正

文章《把一个开源 PC 老游戏搬进 Switch》及其链接的 `neomody77/sdl3-switch` README 已阅读。

| 文章经验或说法 | 本项目采用方式 |
| --- | --- |
| SDL3 驱动源码存在但 bootstrap/配置宏没接入，会得到空对象文件 | 平台接入同时检查源码选择、宏、链接 map 和实际后端初始化 |
| 静态库循环依赖需链接组 | 仅对实际循环依赖使用 `LINK_GROUP:RESCAN`；不随意全局 whole-archive |
| AUDOUT 48 kHz、立体声 S16，libnx pad 输入 | 作为端点实现参考，先复用 Horizon fork 已有后端 |
| SDL3 视频通过 EGL/GLES2 | 该 fork 的视频能力不等于 Vulkan surface 支持，不能直接接当前 presenter |
| “NRO 就是 applet，约 448 MB” | 纠正：NRO 是格式，内存权限取决于启动上下文；通过应用接管启动 hbmenu 可运行 Application 模式 NRO |
| “必须 NSP 才能拿完整内存” | 不采用。先做 Application 模式 NRO；full-memory 不是 NSP 后缀自动赋予的能力 |
| “forwarder 内存取决于 NRO” | 应检查 loader 的 NPDM、启动上下文和实际可用堆；不是 NRO 文件自行决定 |
| “romfs 子目录逐文件打开不可靠，必须 ZIP” | 这是该文章的个案推论，不作为 libnx 通用限制；先检查挂载、路径和 VFS，不为此重打包 `.mrg/.mrgc` |
| “0x05 区段避开正版，0x01 不可用” | 不作为通用分配规则。参考 Prototype 本身用了 `01...`；任何前缀都不保证全局无碰撞 |
| NACP 偏移是 Title ID 的唯一来源 | 这取决于打包工具实现；统一输入并核对 NACP 相关字段、NPDM、NCA/CNMT，而非手改固定偏移 |
| 每行日志 fsync | 只用于极早期崩溃定位；正常游戏按阶段/错误落盘，避免把 SD 同步写入变成卡顿来源 |
| 全局 `-fsigned-char` | 先核对本项目依赖 char 符号性的代码；只有需要保持既有语义才加入，不照搬别的游戏修复 |
| NPDM 使用 36-bit、CPU 核 0–2 | 和 Prototype 的 39-bit、0–3 设置不同。按 guest 地址范围及运行时权限选取，不能拼接两套配置 |

还需处理一个直接冲突：文章的 SDL3 fork 通过 `CMAKE_SYSTEM_NAME= NintendoSwitch` 识别平台，Prototype 工具链设置的是 `Generic`。主线沿用 Horizon 的 `REXGLUE_PLATFORM_SWITCH`，统一项目内判断；如果引入 SDL3 后端，则显式桥接它的平台识别，不能同时依赖两个不一致的条件。

## 4. 构建与文件组织

以下路径除注明现有外均为计划新增，不代表当前已有可运行实现。相对路径均从工作区根目录计算。

| 文件或目录 | 工作内容 |
| --- | --- |
| `guardian_heroes/cmake/switch-devkitA64.cmake` | 基于参考工具链接 devkitA64、libnx、静态库搜索根和主机工具隔离 |
| `guardian_heroes/cmake/GuardianHeroesSwitch.cmake` | Switch target、静态后端绑定、NRO 元数据；优先复用供体 NRO 模块 |
| `guardian_heroes/CMakeLists.txt`、`CMakePresets.json` | 新增独立 Switch preset/build 目录，选择平台源码 |
| `guardian_heroes/cmake/GuardianHeroesApp.cmake` | startup 选择；隔离桌面 rpath、动态库复制 |
| `guardian_heroes/runtime/CMakeLists.txt`、`runtime/src/*/CMakeLists.txt` | GCC 适配、依赖裁剪、平台源选择；保留桌面设置 |
| `guardian_heroes/runtime/src/ui/runtime_switch.cpp` | 移入供体 startup，实现日志、服务和 applet 生命周期 |
| `guardian_heroes/runtime/share/` 下的 Switch 入口 | 按供体实际入口文件名移入并由 app target 选择；保持现有 app 工厂协议 |
| `guardian_heroes/src/gh_host_memory_switch.cpp` | 按现有 host-memory 接口实现 Horizon 操作 |
| `guardian_heroes/src/gh_guest_arena.cpp` | 完成当前已有的 Switch 分支及别名视图 |
| `guardian_heroes/runtime/src/input/switch/` | 移入供体 pad driver，接现有输入系统 |
| `guardian_heroes/src/gh_audio_switch.cpp`、`gh_audio_factory.h` | Switch 输出端点/factory；若供体可直接复用则不创建重复端点 |
| `guardian_heroes/src/guardian_heroes_app.cpp` | 平台 factory、路径、运行模式、生命周期接入 |
| `guardian_heroes/runtime/src/ui/vulkan/`、`src/gh_gpu_vulkan_*` | NVK 装载、窗口呈现、能力分支和实际游戏兼容修复 |
| `guardian_heroes/tools/build_switch.sh` | 串行执行已明确的主机生成与目标构建，缺依赖立即失败 |
| `guardian_heroes/switch/metadata.cmake` | 名称、版本、图标、独立 Title ID、NRO 路径的打包输入 |
| `guardian_heroes/switch/forwarder/` | 只在 NRO 验收后加入 loader 与打包脚本 |
| `docs/validation/switch/` | 每阶段环境、命令、日志、截图、指标及结论 |
| `docs/switch-dependencies.md` | 版本、来源、局部移植文件、许可和构建宿主记录 |

构建目录使用 `guardian_heroes/out/build/switch-aarch64-release`。保持现有 `generated/default` 为唯一游戏生成源；不把 macOS 对象、PCH、FFmpeg、Vulkan loader 或其他桌面库带入交叉链接。

优先在固定 Linux 构建环境复现参考链，macOS 保留编辑和已验证主机 codegen 能力。Mesa 脚本包含 Alpine/PRoot 路径、Rust wrapper 和 Linux 工具假设，不能把脚本原样当成 macOS 安装步骤。

## 5. 分阶段任务

### P0：冻结输入和平台依赖（预计 1–2 人日）

依赖：无。产物：依赖记录和可执行的第一阶段构建选择。

- [ ] 记录当前 Guardian Heroes 源码状态、manifest、`default.xex` 对应版本及生成器版本；不覆盖现有生成代码。
- [ ] 按 `docs/switch-dependencies.md` 记录 Horizon SDK、Prototype 补丁、Mesa/NVK、devkitA64、libnx、FFmpeg、Rust、bindgen 和 Mesa 主机生成工具版本。
- [ ] 对照项目 runtime 与供体的 memory/thread/filesystem/startup/UI/audio/input 接口，列出要移入的文件和当前 `gh_*` 调用方。
- [ ] 核对 `PPCContext` 布局、函数指针、生成注册、import adapter 与异常模型；保持当前 native hook 和本地化 hook。
- [ ] 记录引用文件的许可证。Prototype 工具链及专用 renderer 带 GPL 标注；项目当前 runtime 保留 BSD 来源。复制代码前确定所选文件的分发义务，随发布保留适用声明。
- [ ] 记录 Switch 型号、系统/Atmosphère/hbloader 版本和用户已有运行入口；硬件未到位时允许主机阶段推进，但硬件门槛保持未通过。

出口：选定唯一供体版本和工具链路径；能说明每项平台改动应落在项目哪一层。不能用“替换整个 SDK”跳过接口差异。

### P1：GCC、静态链接与最小 NRO（预计 2–5 人日）

文件：工具链、runtime 顶层 CMake、app CMake、Switch startup、构建脚本。

- [ ] 先编译一个现有生成函数、`guest_abi_check.cpp` 和实际使用的 PPC SIMD/原子辅助代码，确认 devkitA64 的头文件、内建函数、浮点和 ABI 能力。
- [ ] 将 Clang 专属参数限定给 Clang；GNU 使用项目已采用的严格浮点/别名参数思路，保留 `-ffp-contract=off` 和不启用 fast-math 的语义。
- [ ] 只有受支持源文件确实适配后，才放开 runtime 的 Clang-only 条件。GCC 不兼容时定位具体内建/汇编/类型，不以删除检查宣称适配。
- [ ] 平台选择统一采用 Horizon fork 的 Switch 条件。排除 Linux surface、桌面 SDL video、MoltenVK、D3D12、动态加载和不适用的 Tracy 路径。
- [ ] 沿用现有 runtime/plugin 接口静态注册后端；链接 map 必须能定位 factory 和注册入口，防止 `--gc-sections` 丢掉无人直接引用的初始化代码。
- [ ] 分开主机工具查找与目标头文件/库查找；禁止交叉构建执行目标侧生成器。
- [ ] 新增 `guardian_heroes_switch_bootstrap` 最小目标：初始化 SD 日志、查询启动模式、显示版本文本并处理应用事件；此目标暂不链接整个 guest/GPU。
- [ ] 用供体 `rexglue_switch_add_nro` 模块或等价的现有供体入口生成 bootstrap NRO；不创建第二套自制 NRO 格式写入器。

出口：NRO 在真机运行到 startup，SD 上有模式、版本、可用内存、启动阶段日志，屏幕有可识别输出。这一步的交付名称必须是“启动探针”，不能称为游戏已启动。

### P2：独立 NVK 呈现（预计 2–5 人日）

依赖：P1。文件：runtime Vulkan instance/device/presenter、Switch CMake、Mesa 构建说明。

- [ ] 按参考 Mesa 版本和补丁构建主机 `mesa_clc`、`vtn_bindgen2` 等所需工具，再交叉构建 Horizon 库；Rust 编译链也固定版本。
- [ ] 校正参考脚本中的绝对路径与宿主假设，构建失败即停止；不保留吞掉构建错误的 `|| true`。
- [ ] 使用匹配 NVK 静态库的 headers 和符号装载方式，接入供体 NWindow/surface 与 presenter。
- [ ] 初始化实例、设备、队列和呈现；输出实际 Vulkan API、扩展、features、limits、内存类型。
- [ ] bootstrap 增加固定颜色清屏和帧号，完成提交/等待/呈现循环；此时仍不接 Xenos。
- [ ] 排查 Vulkan 静态符号重复、Rust runtime 重复和链接组。参考工具链的 `--allow-multiple-definition` 不掩盖项目自身重复定义；必须从链接 map 区分来源。

出口：Application 模式下颜色和帧号连续更新，切回 HOME 再返回仍可呈现。失败只处理驱动/启动/呈现，不同时调试游戏 shader。

### P3：guest 内存、线程与调度（预计 4–10 人日）

依赖：P1；P2 必须在接游戏画面前通过。文件：`gh_guest_arena.cpp`、新增 host-memory 后端、runtime core/system、项目 kernel adapter。

- [ ] 对照现有 `kViews` 和 `gh_system_xmemory.cpp`，写清 guest virtual、physical alias、MMIO、保护页与共享 GPU 页的关系。
- [ ] 移入供体 Horizon 映射方案，实现 reserve/commit/protect/free，以及同一底层页通过不同 guest 地址访问的一致性。
- [ ] 核对 arena 基地址约束、4.5 GiB 虚拟跨度与所选地址空间。Prototype forwarder 的 39-bit 仅是参考；不得照搬文章 36-bit 后默认映射一定可用。
- [ ] 将已有 arena 测试中关键别名读写、边界和释放重建用例适配为目标探针；物理页实际提交量另行记录。
- [ ] 接线程、TLS、时钟、event/semaphore、等待和 guest 回调。`-mtp=soft`、TLS model、`--no-relax` 与 libnx/编译器版本一起验证。
- [ ] 明确 guest 原子与多线程内存顺序；保留现有 volatile/原子语义，不照搬 Prototype `prepare_no_volatile.py`。
- [ ] 记录 guest 主线程、渲染、音频及 worker 的栈预算和实际线程数；不直接套用每线程 8 MiB 和 2 GiB heap 的 Prototype 配置。
- [ ] 接通生成注册、间接调用及 native hook；异常、未注册函数、映射失败应留下具体 guest PC/地址，不将失败变成成功返回。

出口：目标机通过关键 alias/TLS/同步路径，guest 初始化能执行到可定位的阶段，实际内存预算允许继续接入 GPU。

### P4：资源、导入和 guest 启动（预计 2–4 人日）

依赖：P3。文件：`GuardianHeroesApp::SetupEnvironment()`、`gh_filesystem*`、runtime kernel/filesystem。

- [ ] 使用固定开发目录，不依赖 hbmenu 或 forwarder 的当前工作目录：

```text
sdmc:/switch/guardian-heroes/
  guardian_heroes.nro
  guardian_heroes.toml
  assets/default.xex
  assets/…原有目录和 .mrg/.mrgc 文件…
  user/                 # 用户数据、游戏存档
  cache/                # shader/pipeline 等可重建缓存
  logs/                 # 启动阶段、错误和性能记录
```

- [ ] 复用 `game_data_root`、`user_data_root`、`cache_root` 等现有配置；不另造一套 Switch VFS。
- [ ] 读取 `default.xex`、元数据、一个 `.mrg/.mrgc`、本地化内容，保留原始目录和文件编码。
- [ ] 按游戏实际触发顺序处理文件/用户/XAM 导入；文件读写、seek、length、异步完成和错误码保持一致。
- [ ] 特别复核写后长度与截断后的长度，Prototype 曾在此造成存档失败；只修本项目实际存在的问题。
- [ ] 把启动路径分为 runtime 构造、模块装载、guest entry、资源打开、首批 GPU 命令五个日志节点。

出口：能区分缺资源、guest 执行失败和 GPU 失败；资源内容正确进入游戏消费者。

### P5：Xenos/Vulkan 到标题画面（预计 5–15 人日，最大不确定项）

依赖：P2、P4。文件：`src/gh_gpu_vulkan_*`、shader 翻译器、runtime presenter。

- [ ] 从实际 Vulkan device/pipeline/render-target 代码列出必需能力，与 P2 记录的 NVK features 对照；只启用已支持能力。
- [ ] 检查本游戏用到的 shader 指令、descriptor/资源绑定、格式、采样、深度/模板、blend、EDRAM 和 resolve 路径。
- [ ] 按 clear → 静态纹理 → 字体/UI → 标题完整帧逐步接入，失败时保留该帧的 guest PC、GPU packet、shader 和资源信息。
- [ ] 验证 guest 写入至纹理/顶点上传的可见性，以及 GPU 完成至 guest fence/中断的反馈链。
- [ ] `WAIT_REG_MEM`、present 等待、guest fence 不得通过直接返回或假完成绕过；定位实际提交序号和完成值。
- [ ] 检查 render target resolve、透明边缘、字体、调色板/颜色、背景与角色层次，对照同内容桌面画面。
- [ ] 使用现有 GPU snapshot 支持缩小 CPU 侧问题；NVK 驱动与真机呈现仍由目标机验证。

出口：标题文字和背景正确、持续刷新、无 GPU hang；保留截图和对应日志。若必需能力在所选 NVK 不可用，先报告具体能力及替代成本，再决定升级驱动或改渲染方案。

### P6：输入与音频闭环（预计 3–6 人日）

依赖：P5；后端实现可在 P3/P4 后先准备。

**输入：**

- [ ] libnx pad → runtime driver → `gh_controller_assignment_adapter` → guest 查询完整贯通。
- [ ] 明确 Nintendo ABXY 与 Xbox ABXY 的物理位置对应，沿用游戏当前确认/返回语义，避免两次交换。
- [ ] 验收标题确认、菜单方向、返回、移动、跳跃、攻击、防御/换线、暂停和恢复；以当前游戏实际映射为准记录表格。
- [ ] 检查摇杆范围/死区、按下/保持/松开、断开后清除旧状态、重连后的玩家槽位。
- [ ] 单人先通过，再测双手柄分配和第二玩家加入；更多玩家按游戏模式实际支持数单独记录。

**音频：**

- [ ] 保留现有 XMA decode、混音、buffer credit 和 guest completion；只替换最终 SDL 输出端点。
- [ ] 交叉构建实际需要的 libavcodec/libavutil 和 XMA 组件，确认不会悄悄退化为 Nop 音频。
- [ ] 接供体音频后端；若使用 AUDOUT，明确 PCM 格式转换、48 kHz 采样、对齐、buffer 所有权和回收时机。
- [ ] 检查 BGM、战斗音效、连续播放、暂停/恢复和 HOME 返回；输出欠载次数、队列深度及回调状态。

出口：一次连续操作可以从标题进入可控制场景，音乐和动作音效均正常；设备初始化或按钮日志不能代替此门槛。

### P7：战斗、存档和性能（预计 4–10 人日）

依赖：P6。文件：仅修改实际暴露问题所在模块；不预设一批游戏地址补丁。

- [ ] 固定角色/模式/场景，覆盖移动、连续攻击、受击、多人同屏元素、暂停、场景转换和返回菜单。
- [ ] 对照桌面基线的速度、输入响应、画面和声音；先保持现有分辨率、逻辑帧率和内容规模。
- [ ] 分开记录 CPU guest 执行、GPU 提交/等待、pipeline 创建、文件读取和音频端点；不能用 native 函数数量替代性能测量。
- [ ] 在标称频率记录首帧、典型战斗帧时间 P50/P95/P99、长帧来源、内存峰值、场景转换峰值及 shader 冷/热缓存差异。
- [ ] 检查物理内存构成：可执行代码/只读数据、guest 已提交页、线程栈、GPU 缓冲/纹理、编译缓存、音频和解码临时区；避免把共享页重复计费。
- [ ] 首轮预算保留至少约 15% 实测可用内存余量作为规划目标，最终按最重场景峰值修订；不假定所有机器固定拥有文章所写 3.2 GB。
- [ ] 保存后读取同一槽位；经用户明确同意退出/重启后，再验证跨进程持久化。配置保存、即时存档和游戏原生存档分开记录。
- [ ] HOME 返回、休眠唤醒、控制器重连分别验证，避免仅重新初始化画面却遗漏音频与 guest 时钟。
- [ ] 首次可用后进行一次约 30 分钟连续游戏观察；只有发现新问题或修改影响该路径时重跑。

出口：单人实际可玩并可跨进程保存/恢复，性能有目标硬件数据。未达到目标时明确瓶颈，视觉/手感折衷另行确认后执行。

### P8：NRO 发布包（预计 1–2 人日）

依赖：P7。

- [ ] 固定构建命令与依赖版本，保留本地未剥离 ELF/link map 用于崩溃定位；发布 NRO 和必要配置/说明。
- [ ] README 写明 Application 模式启动、资源目录、支持的游戏版本、按键、存档位置和已验证机器。
- [ ] 测试从实际发布目录加载，不依赖开发者绝对路径、隐藏环境变量或未打包库。
- [ ] 复制到 SD 后核对文件字节数和目录；只处理确实发生的截断/漏文件，不新增通用指纹系统。
- [ ] 发布物包含适用的许可声明，不包含用户密钥和游戏资产；资源由使用者提供。

出口：一套完整的 NRO 目录包，可在同配置机器按说明复现安装和启动。

### P9：HOME 菜单 NSP forwarder（预计 1–3 人日）

依赖：P8。第一版采用直接转发到 NRO，不额外移植 Prototype 的设置启动器。

- [ ] 在 `switch/metadata.cmake` 中维护唯一 Switch Title ID 输入，格式为 16 位十六进制；选定值时核对本机已安装内容及已有自制软件，不把 Xbox `0x5841000B` 转填。
- [ ] 将统一值传给 forwarder NPDM 的 program/title ID 与允许范围、NACP 对应标识字段及 NCA/CNMT；名称、版本、图标也来自同一组打包输入。
- [ ] NRO 固定路径采用上文 `sdmc:/switch/guardian-heroes/guardian_heroes.nro`。目录使用名称即可，不必为了统一 ID 强制重命名用户数据目录。
- [ ] 采用 Prototype forwarder 的 loader/容器实现，核对准确版本及许可，保持 key 输入为本地参数。
- [ ] 按 P3 的地址需求设置地址空间，按实际服务和线程需求设置能力。不要未经验证继承 ForceDebug、核心 3、所有服务权限。
- [ ] 打包后检查 NPDM、NACP、CNMT 一致性和 `nextNroPath/nextArgv`，确认 baseline JPEG 图标实际可被系统显示。
- [ ] 真实从 HOME 图标启动，核对运行模式、内存、NVK 初始化、资源与存档根；与 hbmenu Application 模式 NRO 对照。
- [ ] 检查 NRO 缺失时错误说明、NRO 更新后 forwarder 继续启动、用户数据保持原位。

出口：独立 Guardian Heroes HOME 图标能启动同一 NRO，输入、音频、存档无入口差异。

### P10：自包含 Application NSP（独立可选后续）

只在明确需要“资源也进安装包”时启动，不属于首版 forwarder 交付门槛。

采用 ELF → NSO → ExeFS/main + NPDM，以及 RomFS、Control NCA、Meta NCA → NSP 的完整应用路径。单独处理 Application startup、GPU 服务/ARUID、RomFS 挂载和权限。原始 `.mrg/.mrgc` 仍按现有归档格式读取；用实际路径读取验证排查问题，不预先套 ZIP。

资源只读根改为 `romfs:/assets`，存档/配置/缓存保留在 SD 可写目录。若切换系统 save-data 服务，则单独规划数据迁移，不静默改变已有存档位置。

## 6. 未来构建入口约定

本节是实施后应提供的命令接口，不是当前已可执行的 Switch 构建命令。

```sh
cd /Users/ddd/Documents/xbla-守护英雄/guardian_heroes
export DEVKITPRO=/opt/devkitpro
# NVK_SDK 指向 P2 构建完成的 Horizon portlibs 根，含 include/ 与 lib/。
: "${NVK_SDK:?set the built Horizon NVK SDK path}"
cmake -S . -B out/build/switch-aarch64-release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=cmake/switch-devkitA64.cmake \
  -DREXGLUE_SWITCH_NVK_SDK="$NVK_SDK" \
  -DGUARDIAN_HEROES_NATIVE_ONLY=ON
cmake --build out/build/switch-aarch64-release \
  --target guardian_heroes --parallel 4
```

主机 codegen 独立执行，仅在需要重新生成时运行：

```sh
cd /Users/ddd/Documents/xbla-守护英雄/guardian_heroes
: "${GH_HOST_REXGLUE:?set the matching host rexglue executable}"
"$GH_HOST_REXGLUE" codegen guardian_heroes_manifest.toml
```

由 P0 确认匹配生成器支持的参数；生成内容有差异时保留差异审阅，不执行 Prototype 的地址补丁或去 volatile 脚本。构建并行度先使用 4，按实际内存占用调整；大规模 guest 与 Mesa 编译不同时启动。

## 7. 验证安排与停止条件

执行阶段开始前，每批检查先说明检测的失败、影响的下一步、范围与预计耗时，并按项目规则等待确认。必要构建和最小验证集中成一批，不重复运行没有新增信息的检查。

| 检查 | 要检测的具体失败 | 失败后改变什么 | 预计机器/操作耗时 |
| --- | --- | --- | --- |
| 代表性 guest/GCC 编译 | Clang 内建、PPC 类型布局、TLS/原子不兼容 | 修窄兼容点；暂停全量交叉编译 | 5–15 分钟 |
| 首次全量目标构建 | 桌面库污染、静态未解析符号、重定位问题 | 修源码选择/链接/工具链 | 30–120 分钟，按宿主修订 |
| bootstrap NRO | startup、模式、日志、服务初始化失败 | 只修平台入口 | 5–10 分钟真机 |
| NVK clear | surface/device/queue/present 失败 | 修驱动集成，不碰游戏逻辑 | 5–15 分钟真机 |
| arena/TLS/sync 探针 | alias 不一致、线程状态损坏、等待不唤醒 | 停止接 guest，修运行时语义 | 5–15 分钟真机 |
| 标题画面 | shader、EDRAM、resolve、fence 或资源错误 | 根据首个错误帧修 GPU/资源路径 | 10–20 分钟真机 |
| 标题至战斗 | slot/映射错误、假音频、guest 回调丢失 | 修对应输入/音频链 | 15–30 分钟真机 |
| 保存与重载 | 写后长度错误、路径错误、未持久化 | 修文件/存档行为 | 10–20 分钟，含用户批准重启 |
| 连续游戏/休眠 | 泄漏、迟发死锁、时钟/音频恢复错误 | 按测量结果限定修复范围 | 30–45 分钟 |
| forwarder 对照 | NPDM、ID、argv、权限或模式差异 | 只修打包/loader | 10–20 分钟 |

已有测试优先复用 `gh_guest_arena_test.cpp`、`gh_kernel_test.cpp`、`gh_filesystem_imports_test.cpp`、`gh_controller_assignment_adapter_test.cpp`、`gh_audio_runtime_test.cpp` 和 GPU snapshot 支持。桌面通过只说明对应平台行为，不能抵扣真机门槛。

证据按阶段保存为 `docs/validation/switch/Pn-日期/`，包括依赖版本、命令、退出码、运行模式、硬件/频率、关键日志及对应画面。阶段记录用“未执行 / 通过 / 阻塞”表达；本计划各实施阶段初始均为未执行。

启动游戏后默认保持运行。不得在验证末尾自动关闭窗口、杀进程或发送退出键；涉及重新启动的验证先由用户明确授权或操作。

## 8. 排期和关键风险

单名熟悉本项目的工程师，工具链与硬件可用时，P0–P9 初步合计约 **25–62 人日**。这是范围估算，不是交付承诺；P3 内存映射、P5 通用 Xenos 在 NVK 上的兼容性最可能扩大范围。P1 通过后重新估计构建工作，P5 通过后再给首个可玩版本排期。

| 风险 | 可观察信号 | 决策 |
| --- | --- | --- |
| 当前项目和 Horizon fork ABI 已分叉 | 类型、导入、注册或 factory 不匹配 | 保留项目接口，移植平台实现，避免整库替换 |
| GCC 无法保持生成代码语义 | 代表函数编译失败或结果不一致 | 先修具体兼容点；若需换编译器，单独评估完整 libnx ABI 链 |
| Horizon 无法满足 arena/alias 需求 | reserve/map 失败或别名读写不一致 | 首先修供体映射与地址空间；不能伪造独立内存继续 |
| NVK 缺少本游戏必需能力 | feature 缺失、shader 失败、resolve 错画面 | 给出最小失败场景，再决定升级驱动或新后端研究 |
| 内存超限 | 场景转换峰值 OOM、缓存增长不回收 | 先定位实际占用/释放问题，再考虑有界缓存；画质折衷需确认 |
| 仅超频可用 | 标称频率长帧或音频欠载 | 单独报告，不拿 Prototype 超频数据承诺默认体验 |
| forwarder 与 NRO 表现不同 | Application 模式、权限、argv 或目录不同 | 对照启动日志/NPDM，不以主界面图标出现视作成功 |

暂不立项：从零编写 deko3d 渲染器、移植 Prototype native renderer、全面消除 PPCContext、把生成函数人工重写、PGO/超频方案、额外设置启动器。只有主线证据显示必要时再单独评估。

## 9. 限制与交付边界

本次交付是依据现有源码和两份外部参考整理的实施计划。未安装工具链、未交叉编译、未启动 Switch 验证，也未更改游戏功能。参考项目作者报告的可玩状态和性能不是 Guardian Heroes 的运行证据。

最早可交付物是 libnx 启动探针和 NVK 清屏探针；中间交付物是标题/可操作 NRO；完成 P7/P8 才称为本项目已验证的 Switch 单人版本。forwarder 和自包含 NSP 是两个不同成果，不能互相代称。

## 10. 参考资料

1. [Prototype Switch 仓库与 README](https://github.com/fungalspores/prototype-switch)
2. [devkitA64 工具链](https://github.com/fungalspores/prototype-switch/blob/master/prototype/cmake/switch-devkitA64.cmake)
3. [Prototype CMake 与 NRO 接入](https://github.com/fungalspores/prototype-switch/blob/master/prototype/CMakeLists.txt)
4. [主机 codegen / Switch 构建脚本](https://github.com/fungalspores/prototype-switch/blob/master/prototype/build.sh)
5. [Mesa/NVK 构建脚本](https://github.com/fungalspores/prototype-switch/blob/master/tools/proot/mesa_p2_build.sh)
6. [NSP forwarder 脚本](https://github.com/fungalspores/prototype-switch/blob/master/forwarder/build_forwarder.py)
7. [Horizon SDK 来源：masseffect-nx](https://github.com/nebadasSwifty/masseffect-nx)
8. [Mesa Horizon 来源：mesa-switch](https://github.com/danfromtico/mesa-switch)
9. [把一个开源 PC 老游戏搬进 Switch](https://hater.cc/2026/06/26/porting-an-open-source-pc-game-to-switch/)
10. [文章对应的 SDL3 Switch 后端](https://github.com/neomody77/sdl3-switch)

参考资料 7、8 的具体版本来自 Prototype README；平台移植前必须读取选定版本的源文件和补丁。文章提供实践案例，格式、权限和 API 细节以实际所用 libnx、loader 和打包工具为准。
