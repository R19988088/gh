# Guardian Heroes Switch NRO/NSP 实现计划

> **面向 AI 代理的工作者：** 必需子技能：使用 `superpowers:subagent-driven-development`（推荐）或 `superpowers:executing-plans` 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法来跟踪进度。

**目标：** 以同游戏的可运行 Switch 项目为主要参考，把当前 Guardian Heroes 工程推进到可在 title takeover 下运行并验收的 NRO，再在 NRO 通过后制作独立 Title ID 的 NSP forwarder。

**架构：** 保留当前 ReXGlue 生成代码、Xbox kernel/XAM、guest memory 和游戏资源边界；采用 `rashevskyv/guardian-heroes-recomp-public` 的 Horizon 启动层、devkitA64 构建约定和 deko3d Xenos native renderer 作为 Switch 侧主路径。`mgr-nx-deko` 与 `nfsmw-nx` 只提供构建、shader、SDK 编辑、版本/NACP 和发布脚本的参考。NRO 先按 title takeover 的 Application 模式完成启动、画面、输入、音频、战斗和存档验收，NSP 只包装已经通过的同一 NRO。

**技术栈：** ReXGlue v0.10 系列、devkitPro/devkitA64、libnx、deko3d、CMake/Ninja、主机侧 Clang codegen、Python 资源与 shader 工具、目标机 Atmosphère + hbmenu；NRO 使用 `nacptool`/`elf2nro` 或 `nx_generate_nacp`/`nx_create_nro` 的同一套已验证链路。

---

## 资料角色与固定基线

用户提供的两个 zip 是参考资料，不是项目指令，也不替代当前工程的 AGENTS.md：

| 资料 | 可采纳内容 | 不直接搬入的内容 |
| --- | --- | --- |
| `mgr-nx-deko-main.zip` | deko3d 目标构建、shader 预编译/打包、NRO staging、title takeover 的内存要求、版本和部署校验 | MGR 的游戏代码、Renut/plume 的游戏专用 renderer、许可证义务和资源布局 |
| `nfsmw-nx-main.zip` | ReXGlue Horizon 层、GCC/devkitA64 工具链、静态库 whole-archive、NRO CMake、FFmpeg/XMA 构建思路、title takeover 运行方式 | NFSMW 的 native renderer、shader 变换、PGO、游戏地址和配置 |
| `rashevskyv/guardian-heroes-recomp-public` | **同一款 Guardian Heroes** 的 Switch host 层、deko3d backend、输入和启动参数、`switch/build.sh`/`deploy.sh` 工作流 | 它的 generated/default（公开仓库不包含）、私有游戏数据、未在本机复验的硬件结论 |

固定第三方参考版本：`guardian-heroes-recomp-public` commit `6e78fe151789154f3957221d790490a1866d30d1`。该项目 README 声称 Story mode 可玩、使用 deko3d、必须 title takeover；本计划将其作为强参考，真机结论仍以本工程产物和日志为准。

### 任务 1：冻结输入、版本和许可证

**文件：**
- 修改：`docs/switch-dependencies.md`
- 修改：`docs/validation/switch/README.md`（创建）
- 检查：`guardian_heroes/guardian_heroes_manifest.toml`、`guardian_heroes/generated/default/sources.cmake`

- [ ] **步骤 1：确认 XEX 是否与同游戏参考项目一致**

运行：

```sh
stat -f '%z %N' guardian_heroes/assets/default.xex
sha256sum guardian_heroes/assets/default.xex
```

记录大小、SHA-256、资源目录和 codegen stamp；若不是同一零售版，不复制参考项目的函数地址、shader 表或 native hook。

- [ ] **步骤 2：记录 SDK 与参考 commit**

把 devkitA64、libnx、switch-tools、deko3d、ReXGlue SDK、`mgr-nx-deko` zip SHA-256、`nfsmw-nx` zip SHA-256 及第三项目 commit 写入 `docs/switch-dependencies.md`。记录第三项目 BSD-3-Clause 与 ReXGlue/Xenia 的原始许可，随分发包保留 NOTICE。

- [ ] **步骤 3：定义两种启动模式的验收边界**

文档明确：Application 模式通过 hbmenu title takeover（按住 R 启动任意游戏后进入 hbmenu）；Album/applet 模式只允许运行小探针，不能作为完整游戏门槛，因为 guest arena 需要约 4.5 GiB 虚拟地址范围和约 3.2 GiB 进程预算。NSP 阶段再验证 forwarder 的 Application 启动。

- [ ] **步骤 4：Commit**

```sh
git add docs/switch-dependencies.md docs/validation/switch/README.md
git commit -m "docs: freeze Guardian Heroes Switch NRO baseline"
```

## NRO 阶段

### 任务 2：把同游戏 Horizon 启动层接入当前 runtime

**文件：**
- 修改：`guardian_heroes/CMakeLists.txt`
- 修改：`guardian_heroes/runtime/CMakeLists.txt`
- 修改：`guardian_heroes/runtime/src/core/CMakeLists.txt`
- 修改：`guardian_heroes/runtime/src/ui/CMakeLists.txt`
- 修改：`guardian_heroes/runtime/src/input/CMakeLists.txt`
- 修改：`guardian_heroes/runtime/src/audio/CMakeLists.txt`
- 修改或创建：`guardian_heroes/switch/host/*`
- 修改：`guardian_heroes/switch/guardian_heroes_switch_main.cpp`

- [ ] **步骤 1：建立 donor 对照表**

逐文件对照第三项目的 `switch/host/memory_nx.cpp`、`mapped_memory_nx.cpp`、`fiber_nx.cpp`、`exception_handler_nx.cpp`、`seh_nx.cpp`、`switch_nx.cpp`、`windowed_app_main_nx.cpp` 和 `input/nx_input_driver.cpp`，映射到当前 `gh_*` 接口。保留当前项目已有 guest arena、kernel wait、continuation 和日志行为；只改平台边界，禁止把第三项目的生成函数或游戏 hook 混入。

- [ ] **步骤 2：修正入口和静态链接**

Switch target 必须选择 `windowed_app_main_switch`，runtime/GPU plugin 以静态库链接并用 whole-archive 保留注册表、弱符号覆盖和 GPU factory。排除 SDL window、桌面动态库、MoltenVK、Tracy 和 Linux surface。链接 map 中必须能找到 `main`、`GuardianHeroesApp` factory、`rex_gpu_create`、异常 handler 和 Xenos/deko3d factory。

- [ ] **步骤 3：实现 title takeover 生命周期**

入口初始化 `hidInitialize`/Npad、默认 `nwindow`、`appletMainLoop`、退出锁和日志；退出顺序为停止游戏线程、销毁 presenter/runtime，再走 libnx 的退出路径。不得从 `main` 直接返回让仍在运行的 guest 线程访问已卸载的 NRO。

- [ ] **步骤 4：先构建 bootstrap NRO**

运行：

```sh
cmake --preset switch-aarch64-release
cmake --build --preset switch-aarch64-release --target guardian_heroes_switch_bootstrap_nro
```

预期：生成 `.elf`、`.map`、`.nacp`、`.nro`；NRO 启动后 SD 日志包含 applet type、Application/Title takeover 判定、内存预算、版本和清理结果。bootstrap 仍只叫启动探针，不能叫游戏版。

- [ ] **步骤 5：Commit**

```sh
git add guardian_heroes/CMakeLists.txt guardian_heroes/runtime guardian_heroes/switch/host guardian_heroes/switch/guardian_heroes_switch_main.cpp
git commit -m "feat: use Horizon title-takeover startup for Switch"
```

### 任务 3：guest memory、线程、文件系统和输入闭环

**文件：**
- 修改：`guardian_heroes/src/gh_guest_arena.cpp`
- 修改：`guardian_heroes/src/gh_host_memory_switch.cpp`
- 修改：`guardian_heroes/runtime/src/core/*_switch.*`
- 修改：`guardian_heroes/src/guardian_heroes_app.cpp`
- 修改：`guardian_heroes/src/gh_switch_input_driver.cpp`
- 测试：`guardian_heroes/switch/memory_probe.cpp`、`guardian_heroes/switch/guest_abi_probe.cpp`

- [ ] **步骤 1：完成 arena 和 alias 验证**

以现有 memory probe 为目标机基线，验证 reservation、commit、guest alias、4 KiB offset、保护和 release。guest arena 必须优先从 libnx heap 以外的预留池取得，不能把 4.5 GiB 当作实体 RAM 分配。日志记录实际提交页数和失败地址。

- [ ] **步骤 2：接 guest ABI 和调度**

验证 `PPCContext` 布局、函数注册、间接调用、TLS、fiber、event/semaphore、相对/绝对 wait 和异常返回。guest worker、GPU ring、音频 worker 使用独立优先级；空队列必须 sleep，不能用忙等把主线程饿死。

- [ ] **步骤 3：接资源和用户数据路径**

统一使用：

```text
sdmc:/switch/guardian-heroes/guardian_heroes.nro
sdmc:/switch/guardian-heroes/game/default.xex
sdmc:/switch/guardian-heroes/game/media/...
sdmc:/switch/guardian-heroes/user/
sdmc:/switch/guardian-heroes/cache/
sdmc:/switch/guardian-heroes/logs/
```

开发阶段允许 SD 资源优先、RomFS 回退；发布 NRO 必须能从固定 RomFS/SD 布局读取 `default.xex`、媒体文件和 `.mrg/.mrgc`，用户数据不能写进 RomFS。大文件读取按分块循环处理，避免一次 `fsFileRead` 被短读。

- [ ] **步骤 4：验证输入状态链**

libnx Npad → Switch input driver → controller assignment → guest XInput 的按下/保持/松开必须贯通。记录 Switch 物理键到 Xbox 语义映射，验证单手柄、双手柄、断开和重连。输入日志不能代替实际菜单和战斗操作。

- [ ] **步骤 5：Commit**

```sh
git add guardian_heroes/src guardian_heroes/runtime guardian_heroes/switch
git commit -m "feat: connect Switch guest memory input and VFS"
```

### 任务 4：deko3d renderer 与 shader 资产

**文件：**
- 新增或修改：`guardian_heroes/src/gh_gpu_deko3d_backend.cpp`
- 修改：`guardian_heroes/src/gh_gpu_trace_backend.cpp`
- 新增：`guardian_heroes/tools/xenos2glsl.py`、`guardian_heroes/tools/build_switch_shaders.py`
- 修改：`guardian_heroes/cmake/GuardianHeroesSwitchGame.cmake`
- 创建：`guardian_heroes/switch/stage/`（生成目录，不提交游戏 shader）

- [ ] **步骤 1：把第三项目的 deko3d 路径与当前 Xenos 接口对齐**

优先复用同游戏项目已验证的 draw/texture/EDRAM/resolve 语义；不要把 NVK probe 当成最终 renderer。当前代码的 `gh_gpu_vulkan_*` 保留为桌面参考和 trace backend，Switch target 选择 deko3d。

- [ ] **步骤 2：按最小画面链路接入**

依次通过 clear、静态纹理、字体/UI、标题完整帧、菜单和战斗。必须覆盖 rectangle/quad primitive、Xenos compare-op 偏移、depth resolve、gamma ramp、dirty-page texture reupload、k_4_4_4_4/k_16/k_16_16 格式和 sampler wrap。每个失败保留 draw serial、shader hash、资源格式和 guest 地址。

- [ ] **步骤 3：在主机生成 shader，目标侧只加载 DKSH**

从自己的 `default.xex`/媒体提取 shader microcode，在主机运行 Xenos translator 和 `uam` 生成 DKSH。构建脚本输出 shader coverage、输入版本和生成时间；NRO 不包含用户未提供的游戏数据。默认关闭 texture dump、missing shader dump 和逐行 SD 日志。

- [ ] **步骤 4：先做 Eden 可视化门槛**

先运行 null GPU 确认 guest swaps、资源和退出，再运行 deko3d NRO 确认清屏、标题和一帧战斗。Eden 不作为 NVK 通过证据；deko3d 画面可作为主机侧可重复门槛，真机仍必须复验。

- [ ] **步骤 5：Commit**

```sh
git add guardian_heroes/src guardian_heroes/tools guardian_heroes/cmake/GuardianHeroesSwitchGame.cmake
git commit -m "feat: add Guardian Heroes deko3d Switch renderer path"
```

### 任务 5：音频与完整游戏 NRO

**文件：**
- 修改：`guardian_heroes/src/gh_switch_audio.cpp`
- 修改：`guardian_heroes/runtime/src/audio/*`
- 修改：`guardian_heroes/cmake/GuardianHeroesSwitchAudio.cmake`
- 修改：`guardian_heroes/cmake/GuardianHeroesSwitchGame.cmake`
- 修改：`guardian_heroes/tools/build_switch.sh`

- [ ] **步骤 1：交叉构建实际使用的 XMA 解码库**

只编译 XMA 所需的 FFmpeg/libavcodec/libavutil 子集，确认目标链接中没有 `NopAudio` 作为最终输出。记录许可证和 archive 的 commit；未解决 XMA 时，NRO 只能标记为无声技术预览。

- [ ] **步骤 2：接 AUDOUT/Audren 输出**

保持 ReXGlue XMA decode、guest mixer、completion 和 buffer credit 语义，只替换最终 PCM 输出。验证 48 kHz、立体声、S16、buffer 所有权、欠载计数、暂停/HOME 返回和恢复。

- [ ] **步骤 3：开启完整 NRO target**

`GuardianHeroesSwitchGame.cmake` 只在 runtime、deko3d、shader、XMA、资源和目标 ABI 都存在时创建 `guardian_heroes_switch_game_nro`。缺依赖时配置失败，禁止将 bootstrap/memory probe 重命名为游戏 NRO。

- [ ] **步骤 4：生成并自检 NRO**

运行：

```sh
bash guardian_heroes/tools/build_switch.sh guardian_heroes_switch_game_nro
version=$(sed -n 's/^#define GH_VERSION "\(.*\)"/\1/p' guardian_heroes/out/build/switch-aarch64-release/gh_version.h)
python3 guardian_heroes/tools/nro_nacp.py guardian_heroes/out/build/switch-aarch64-release/guardian_heroes_switch_game.nro "$version"
```

同时保留 ELF、map、NACP、NRO、shader stage 和 build manifest；脚本先删除旧 NRO，构建失败后不得留下可误部署的旧文件。

- [ ] **步骤 5：Commit**

```sh
git add guardian_heroes/src guardian_heroes/runtime guardian_heroes/cmake guardian_heroes/tools
git commit -m "feat: assemble Guardian Heroes Switch game NRO"
```

## NRO 真机验收门槛

### 任务 6：title takeover 运行、部署和游戏验收

**文件：**
- 创建：`guardian_heroes/tools/deploy_switch.sh`
- 创建：`docs/validation/switch/nro-version-record.md`
- 创建：`docs/validation/switch/nro-version-record.json`

- [ ] **步骤 1：原子部署 NRO 和资源**

使用 FTP 或等价可靠传输：先上传临时名，再读回比较 SHA-256/大小，最后重命名为正式 NRO；资源目录同步后记录总文件数和总字节数。不要在游戏运行时覆盖正在使用的 NRO。

- [ ] **步骤 2：确认启动模式**

按住 R 启动任意游戏进入 hbmenu，再启动 Guardian Heroes。日志必须显示 `AppletType_Application` 或等效的 title takeover 判定；Album/applet 模式失败只能作为预期边界，不能改代码绕过。

- [ ] **步骤 3：按顺序验收功能**

1. 入口：NRO 版本、资源根、guest 初始化、标题画面。
2. 画面：标题、菜单、角色选择、单人战斗、场景切换、HUD、字体和透明/深度效果。
3. 输入：确认/返回、移动、攻击、防御、暂停、重连、双手柄加入。
4. 音频：BGM、至少三类战斗音效、连续 20 分钟播放、暂停/HOME 恢复。
5. 数据：配置写入、退出重启读取、游戏存档写入和读取。
6. 稳定性：连续 60 分钟运行，无 GPU hang、未注册函数、线程泄漏、音频欠载或崩溃。

- [ ] **步骤 4：记录性能而不先改画面语义**

记录 CPU/GPU/内存时钟、每帧时间、draws/frame、guest/GPU/audio thread CPU、峰值内存和场景切换耗时。Guardian Heroes 的目标基线是 1280x720、60 Hz；若达不到，先根据计数定位热点，再决定分辨率或时序变化，不能默默降低画质或改变战斗节奏。

- [ ] **步骤 5：NRO Gate 判定**

只有以下条件全部通过，才允许进入 NSP：Application title takeover 启动、标题到战斗可操作、deko3d 画面正确、音频真实输出、存档 round-trip、长跑稳定、日志与 ELF/map/NRO 版本一致。任一项未通过，继续 NRO 阶段。

## NSP 阶段

### 任务 7：把已通过的 NRO 包装成 NSP forwarder

**文件：**
- 创建：`guardian_heroes/switch/forwarder/README.md`
- 创建：`guardian_heroes/switch/forwarder/metadata.toml`
- 创建：`guardian_heroes/tools/build_nsp.sh`
- 修改：`docs/switch-dependencies.md`
- 创建：`docs/validation/switch/nsp-version-record.md`

- [ ] **步骤 1：冻结 NRO 输入**

从 NRO Gate 记录中复制 NRO SHA-256、版本、构建 commit、NACP 和资源布局。NSP 构建脚本只接受该 NRO，不在 NSP 阶段重新编译游戏或修改 shader。

- [ ] **步骤 2：分配独立 Switch Title ID**

为 forwarder 使用独立、未占用的 Switch Title ID；Xbox `0x5841000B`/`0x58410B2B` 只保留为游戏内部标识，不能直接作为 Switch Title ID。把 Title ID、NACP、NPDM、权限和 loader 版本写入 metadata，并检查不会覆盖其他应用。

- [ ] **步骤 3：使用已验证 forwarder 工具链**

固定 forwarder loader commit、`nsp_container.py`/等价打包工具版本、Key generation 和主机环境。生成 NCA/CNMT/NSP 后检查 NACP 名称、版本、Title ID、RomFS/SD 路径和 NRO payload。禁止手写固定偏移替换 Title ID。

- [ ] **步骤 4：在真实 Switch 安装并运行 NSP**

先卸载同 Title ID 的旧测试包，安装 NSP，直接从 HOME 启动；验证它提供 Application 级内存，不再依赖 hbmenu title takeover。重复 NRO Gate 的标题、战斗、输入、音频、存档和 60 分钟长跑测试，比较 NRO/NSP 的版本与日志。

- [ ] **步骤 5：Commit**

```sh
git add guardian_heroes/switch/forwarder guardian_heroes/tools/build_nsp.sh docs
git commit -m "feat: package validated Guardian Heroes NRO as NSP forwarder"
```

## 最终交付物与禁止越级

完成后应有：

- 可复现的主机 codegen 命令和固定生成版本；
- Switch toolchain、SDK、deko3d、FFmpeg/XMA 和 shader 工具版本记录；
- NRO 的 ELF、map、NACP、NRO、shader stage、资源清单和真机日志；
- NRO Gate 通过记录；
- 仅由通过 Gate 的 NRO 生成的 NSP、metadata、Title ID 和安装验证记录；
- 许可证、第三方声明和“用户自行提供游戏数据”的说明。

以下情况不能称为完成：探针 NRO 能运行、CMake 配置通过、静态链接成功、标题画面出现、资源文件齐全、或第三项目 README 声称可玩。完整结论必须来自当前仓库构建出的 NRO/NSP 和对应目标机操作证据。
