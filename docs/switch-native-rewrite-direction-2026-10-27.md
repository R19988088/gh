# Guardian Heroes Switch 自有版本重写路线

日期：2026-10-27  
目标：基于当前 Guardian Heroes 的 XEX、manifest 和生成代码，结合昨天参考工程已经验证过的 Switch 引导方式，重新构建一个由本项目自己维护的 Switch 版本。

## 结论

当前路线需要调整。现有工程把平台启动、guest runtime、NVK/Vulkan、Xenos 兼容、音频和完整游戏 NRO 同时推进，已经可以生成探针和尝试构建完整 NRO，但没有形成一条可在真机逐段验收的游戏路径。昨天参考工程运行时出现丢帧和缺少音乐，说明“能启动”不能作为 renderer、音频或完整运行时的正确性证据。

新的主线采用：

```text
固定同一份 default.xex
  → 重新生成本项目自己的 guest C++
  → 复用参考工程已验证的 Horizon/Application 引导
  → 以 deko3d 建立 Switch 图形主路径
  → 接入本项目自己的 Xenos draw/resolve 语义
  → 接入 libnx Npad 输入
  → 接入 XMAFRAMES 解码和 AUDOUT 输出
  → 逐段验证标题、菜单、战斗、存档和性能
```

这不是把参考工程改名成 Guardian Heroes，也不是把生成代码人工改写成另一套游戏源码。参考工程只提供平台层和构建方法；游戏代码、XEX、资源、manifest、guest ABI 和验证结果全部以本项目为准。

## 为什么调整

### 当前工程的实际边界

当前入口位于：

- `guardian_heroes/switch/guardian_heroes_switch_main.cpp`
- `guardian_heroes/src/guardian_heroes_app.cpp`
- `guardian_heroes/cmake/GuardianHeroesSwitch.cmake`
- `guardian_heroes/cmake/GuardianHeroesSwitchGame.cmake`

当前代码包含 Switch 输入和音频适配，并有通过 GitHub Actions 构建的完整游戏 NRO target。游戏图形主路径是 deko3d，Mesa/NVK 只用于独立诊断 probe；完整 NRO 仍依赖真实 runtime targets 和 Switch FFmpeg/XMA archive。Action 的构建与 NACP 检查不替代 NRO Gate 的真机验收。

现有 `gh_switch_audio.cpp` 已经尝试使用 `audout`，但这只证明存在一个输出适配器，不证明以下链路已经通过：

```text
XMA 数据 → XMAFRAMES 解码 → guest mixer → 6 声道/立体声转换
→ AUDOUT buffer 所有权 → BGM 和战斗音效连续播放
```

同样，NVK 清屏或交换链探针不证明 Xenos shader、EDRAM resolve、纹理格式、present 等待和游戏帧时间已经正确。

### 昨天参考工程能提供什么

可以吸收：

- Application/title takeover 的启动条件；
- libnx 服务初始化和退出顺序；
- guest arena、线程、异常和 Horizon 生命周期的实现方法；
- devkitA64、NRO staging、NACP 和部署脚本约定；
- deko3d renderer 的资源生命周期、shader 构建和帧提交结构；
- Switch Npad 和 AUDOUT/Audren 的平台接线方式。

不能直接复制：

- 参考工程的 generated/default、函数地址和 manifest；
- 参考工程的 native renderer、shader 表和游戏专用 hook；
- 参考工程的 XEX、媒体目录和存档格式；
- 参考工程的性能数字或“可玩”结论；
- 只在参考工程上成立的内存、超频和帧率参数。

昨天出现的丢帧和缺音乐正是需要在自有版本中重新建立测量和故障边界的原因。不能把参考工程的运行结果作为本项目的基线。

## 目标架构

### 保留的游戏基础

以下部分继续作为自有版本的唯一游戏基础：

- `guardian_heroes/assets/default.xex`；
- `guardian_heroes/guardian_heroes_manifest.toml`；
- `guardian_heroes/generated/default/`；
- `guardian_heroes/src/` 中的项目原生逻辑、资源路径、Xenos 语义和音频解码代码；
- `guardian_heroes/runtime/` 中已经迁入项目的 guest ABI、kernel、memory、filesystem 和线程实现；
- 当前本地化、存档和资源目录布局。

生成代码仍由主机侧 ReXGlue codegen 产生，Switch 工具链只负责编译 ARM64 C++。不在 `generated/default/recomp.*.cpp` 中添加平台分支；平台差异放在 runtime、Switch adapter、graphics backend、audio backend 和 app lifecycle。

### Switch 平台层

Switch 平台层分成五个边界：

1. **启动与生命周期**：`guardian_heroes_switch_main.cpp`、`SwitchWindowedAppContext`、Application 模式检查、RomFS/SD 挂载、HOME/退出恢复。
2. **内存与线程**：guest arena、alias view、保护、TLS、fiber、wait、异常恢复和线程优先级。
3. **图形**：deko3d device/queue/swapchain、shader 资产、Xenos command translation、texture/EDRAM/resolve、present fence。
4. **输入**：libnx Npad → `SwitchInputDriver` → `InputSystem` → controller assignment → guest XInput。
5. **音频**：XMAFRAMES decoder → guest mixer → 立体声 PCM16 → AUDOUT/Audren，包含队列、暂停、HOME 恢复和欠载统计。

桌面 SDL、MoltenVK、桌面动态库和 macOS/Linux surface 不进入 Switch 游戏链接。桌面实现继续保留，用于逐帧画面和音频对照。

## 分阶段实施

### S0：冻结输入和基线

目的：避免把错误 XEX、错误资源或错误生成器版本带入重写版本。

工作：

- 记录 `default.xex` 大小、SHA-256、manifest 和 codegen 版本；
- 固定参考工程 commit、devkitA64/libnx/deko3d 版本及适用许可证；
- 保留当前探针作为平台诊断工具，但不再把探针名称或产物称为游戏版本；
- 建立单独的 Switch 自有版本构建目录，不覆盖桌面 build 目录；
- 定义 SD 目录：

```text
sdmc:/switch/guardian-heroes/
  guardian_heroes_switch_game.nro
  assets/default.xex
  assets/media/...
  user/
  cache/
  logs/
```

出口：同一份 XEX、资源、生成源和工具链可以在构建记录中复现。

### S1：自有 Application 引导和最小 NRO

目的：先证明“自己的 NRO 在正确启动模式下运行”，不同时调试游戏画面。

工作：

- 保留参考工程的 Application/title takeover 入口约定；
- 通过 `hidInitialize`、Npad、RomFS 和日志初始化建立明确生命周期；
- 记录 applet type、可用内存、资源根和版本；
- 退出前停止 guest、音频、GPU worker，再卸载 RomFS；
- 构建 NRO、ELF、map、NACP 和 manifest。

验收：

- 只能用 Application/title takeover 作为完整游戏环境；
- Album/applet 启动失败不通过修改代码绕过；
- HOME 返回后重新进入，进程不残留旧 worker；
- SD 日志能区分启动失败、资源失败和运行时失败。

### S2：guest memory、线程和资源读取

目的：让本项目自己的 guest 执行到游戏初始化，而不是只执行探针。

工作：

- 完成 `gh_guest_arena.cpp` 的 Switch reservation/commit/alias/protect/release；
- 保持 4.5 GiB 是虚拟地址跨度的含义，不将其当作实体 RAM 预算；
- 复核 guest 地址、物理页映射、共享页和 GPU 可见性；
- 接通线程、TLS、fiber、event/semaphore、相对/绝对 wait；
- 让资源读取按现有 `game_data_root`、`user_data_root` 和 `cache_root` 语义工作；
- 对 `default.xex`、至少一个 `.mrg`、`.mrgc`、本地化文件和用户数据做实际读取记录。

验收：

- guest entry、模块装载、资源打开和首批 GPU 命令各有日志节点；
- 映射失败包含 guest 地址、长度、保护和阶段；
- 资源错误不被伪装成空文件或成功返回；
- 目标机内存提交量、线程数和栈预算有记录。

### S3：deko3d 基础图形链路

目的：先建立 Switch 上稳定、可测量的 native graphics backend。

工作顺序：

1. 固定颜色清屏和连续 present；
2. 静态纹理和顶点 buffer；
3. 字体/UI；
4. 标题完整帧；
5. 菜单；
6. 单人战斗首帧；
7. 场景切换和 HUD。

每个阶段记录 draw serial、shader 标识、资源格式、GPU fence 和帧时间。deko3d 只负责 Switch 执行和资源生命周期；Xenos primitive、blend、depth、texture format、EDRAM resolve 和 gamma 语义仍由本项目实现。

帧同步要求：

- 不在每帧强制全局 queue idle；
- 使用有限数量的 frame context、command pool 和 fence；
- 明确 CPU 等待、GPU 等待、present 等待和资源上传等待的时间；
- 出现长帧时能区分 shader 编译、纹理上传、CPU guest 执行和 GPU 阻塞。

这一步直接针对昨天的丢帧问题。没有 frame-time trace，就不能以“看起来卡”决定改渲染器。

### S4：输入闭环

目的：确认真实手柄操作到达 Guardian Heroes 的 XInput 语义。

工作：

- 使用项目已有 `SwitchInputDriver`，不再依赖 SDL input；
- 固定 Nintendo ABXY 到 Xbox ABXY 的映射表；
- 验证按下、保持、松开、摇杆死区、断开、重连和第二手柄加入；
- 在标题、菜单、角色选择和战斗中验证确认/返回/攻击/防御/换线/暂停。

验收以实际菜单和战斗结果为准。pad 初始化、按钮日志和 device enumeration 不能代替操作验收。

### S5：XMA 和 Switch 音频

目的：解决昨天缺少音乐的问题，并证明 BGM 与战斗音效共用的真实链路。

工作：

- 确认 Switch 目标 `libavcodec` 含 `AV_CODEC_ID_XMAFRAMES`；
- 在主机侧用已知 XMA 数据验证解码帧数、采样率、声道数和结束条件；
- 保留 guest mixer、completion、buffer credit 和暂停语义；
- 用 AUDOUT/Audren 输出 48 kHz、立体声、PCM16；
- 明确每个 output buffer 的所有权、提交、回收和欠载；
- 记录 BGM、菜单音效、战斗音效、场景切换、暂停和 HOME 恢复；
- 输出队列深度、提交间隔、回收间隔、欠载次数和静音帧数。

重点检查：

- BGM 文件是否按 guest 实际路径打开；
- XMA packet 是否完整拼接，是否错误丢弃首帧/循环帧；
- 6 声道到立体声转换是否产生有效非零 PCM；
- 生产者是否因队列满阻塞游戏线程；
- 音频 worker 是否因等待方式导致主线程或 GPU worker 饿死。

只有菜单音乐、至少三类战斗音效和连续播放通过，音频才标记为通过。初始化成功或 `audout` 打开成功不算音频完成。

### S6：完整游戏 NRO

目的：把已经分别通过的平台、图形、输入和音频边界组合成一个自有 NRO。

构建要求：

- 主机 codegen 和 Switch 交叉编译分离；
- 缺少 runtime、deko3d、shader、XMA 或资源依赖时配置失败；
- 构建失败不得留下可误部署的旧 NRO；
- 保存 ELF、map、NACP、NRO、shader manifest 和 build manifest；
- NRO 只包含本项目自己的 generated/default、资源和平台适配。

### S7：战斗、存档和性能

目的：确认不是“标题能动”的技术演示，而是可持续运行的游戏版本。

验收顺序：

1. 标题和菜单；
2. 角色选择；
3. 单人战斗；
4. 场景切换；
5. HUD、字体、透明和深度效果；
6. 存档写入、读取和重启 round-trip；
7. 连续运行 20 分钟；
8. 标称频率下帧时间和音频欠载统计；
9. HOME 返回后恢复。

超频、降分辨率、改变帧率上限、减少特效或改变战斗时序不作为默认修复。若只有超频能通过，记录为性能缺口，不能写成完成。

## 产物和证据

每个阶段至少保留：

- 构建 commit、XEX SHA-256、工具链版本；
- ELF/map/NACP/NRO 文件大小和 SHA-256；
- SD 日志和启动模式；
- 必要的截图或录像；
- frame-time CSV 或等价统计；
- 音频队列、欠载和非零 PCM 统计；
- 未通过项目对应的代码入口。

证据分为四类，不能互相替代：

| 证据 | 能证明 | 不能证明 |
| --- | --- | --- |
| 主机编译 | 源码和目标 ABI 能编译 | 真机画面、输入、音频和手感 |
| NRO/NACP 检查 | 文件格式和元数据正确 | 游戏进入标题或战斗 |
| 真机启动日志 | 入口、资源、线程阶段 | 画面正确、音乐完整 |
| 真机操作与 trace | 输入、画面、音频、帧时间 | 未覆盖场景的完整游戏支持 |

## 明确暂不做

- 不把 generated/default 人工改写成高层 C++ 游戏源码；
- 不复制参考工程的游戏专用 native renderer、地址补丁或 shader 表；
- 不同时维护 NVK 和 deko3d 两条完整游戏 renderer 主线；NVK 保留为诊断/对照路径；
- 不先制作 NSP forwarder；NRO 在 Application 模式下通过完整游戏门槛后再做；
- 不以探针、静态链接、资源数量或 CI 成功代替真机可玩性；
- 不通过降低画质、改帧率或修改战斗时序掩盖性能问题。

## 第一批实现顺序

确认后按以下最小批次开始：

1. 新增本路线的构建入口和 manifest 记录；
2. 把参考工程的 Horizon 启动/内存/线程边界逐项对照到当前 runtime；
3. 建立 deko3d 清屏、静态纹理和标题帧；
4. 接通自己的 Xenos draw/resolve；
5. 接通 Npad 输入并完成菜单操作；
6. 验证 XMAFRAMES 解码和 BGM；
7. 再组合完整游戏 NRO。

任何阶段若出现长帧或无声，先保留 trace 并定位到 CPU、GPU、资源或音频队列，再改对应边界。

