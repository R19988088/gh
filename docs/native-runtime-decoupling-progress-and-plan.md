# Guardian Heroes 原生运行时解耦进度与下一阶段计划

更新时间：2026-10-17

范围：阶段 3～8。阶段 9 暂不纳入本计划。

## 当前结论

完整 macOS ARM64 app 已由项目源码独立构建。`GUARDIAN_HEROES_NATIVE_ONLY` 默认开启，直接编译项目的 Runtime、KernelState、XThread、内核导入、文件系统、平台 UI、输入、音频与 Xenos 图形后端；运行时和 GPU 以项目静态库进入 app，原生路径直接调用项目设备工厂。独立验证 app 在没有 ReX runtime/GPU 插件文件的环境中已实际显示标题和菜单；可选 continuation app 已出现持续动画的战斗场景，运行日志记录到非零 PCM。

这套运行时由保留 BSD 版权的 ReXGlue/Xenia 实现迁入项目维护，结合项目已有内存、内核、音频和命令服务。`rex` 命名空间、PPC 导入/映射和部分 CMake target 别名继续保持兼容；算法继承来源没有改变。项目现在持有并编译完整宿主实现，后续可以在同一源码树内继续替换和维护。

当前完成了完整 app 的独立构建、标题/菜单操作与 continuation 战斗动画观察。Runtime `GRS1` 已接入 guest 线程、持久等待队列、内存、双时钟和设备的协调保存恢复；真实 title 曾保存并恢复 515,508,145 字节现场，用户确认该次 F5/F9 正常。Release 的恢复 endpoint callback 已修复被断言移除的 credit 归还；最新真实 F9 恢复 357,310,404 字节后持续提交和消费非零 PCM。续执行器对连续 PC 的查验改为范围与对齐判断，保留稀疏 PC 的二分查找和全部暂停边界；标题 BGM 平均 callback 从约 10.7 ms 降至 2.705 ms，实测静音补帧占比从约 55.3% 降至 0.1241%。用户确认修复版“声音和 f9 正常”，当前标题听音与往返恢复验收通过。阶段 3～8 的整体完成条件还包括实际战斗操作、游戏存档和长时间设备行为验收，`full_runtime_replaced` 仍为 `false`。

## Mac、Windows 与 Android 交付路线

现有解耦程度已足够作为三个平台的共用代码底座。后续工作优先推进平台构建、适配和游戏验收，不把继续重写继承的运行时算法作为交付前置条件。推进顺序为 macOS 游戏验收、Windows 构建与验收、Android 平台接入；Switch 1 Atmosphere 保留为后续独立移植目标。

| 平台 | 当前基础 | 下一步 |
| --- | --- | --- |
| macOS ARM64 | 完整 app 已构建；Vulkan 经 MoltenVK 接到 Metal；标题听音与 F5/F9 已获用户确认 | 验收实际战斗、游戏存档读回、切场景和长时间运行 |
| Windows | 已有 Win32 平台、窗口 surface、D3D12 与 Vulkan 实现；当前构建默认开启 D3D12、关闭 Vulkan | 显式开启 Vulkan、关闭 D3D12，在 Windows 原生构建并验收游戏与 F5/F9 |
| Android | 可复用生成代码、GuestContext、内核服务、XMA/PCM 与 Vulkan 后端代码；已有部分 Android 条件分支 | 补齐 Android 应用入口/APK、前后台生命周期、资源和存档路径、输入、音频及设备适配；清理误入的 Linux X11/Wayland 等桌面构建依赖 |

三平台后续优先共用 Vulkan 渲染路径，macOS 通过 MoltenVK 使用 Metal。Windows 原生 Vulkan 的 surface 与 presenter 代码已存在，构建时可使用：

```text
-DREXGLUE_USE_VULKAN=ON
-DREXGLUE_USE_D3D12=OFF
```

应用工厂当前按 `any` 选择后端；同时编入 D3D12 和 Vulkan 时先选 D3D12。因此 Windows 的 Vulkan 构建需要显式关闭 D3D12。当前 Vulkan 快照实现可作为 Windows F5/F9 的适配基础；D3D12 仍明确拒绝快照。上述路线不改变当前代码默认值，Windows 与 Android 尚无本轮构建和目标设备运行验收。

## macOS app 交付

未压缩 app 位于 `dist/2026-10-17-macos-arm64/Guardian Heroes.app`。使用 Release `guardian_heroes_continuation_app`，保留 F5 保存现场和 F9 恢复；游戏资源、Vulkan loader 与 MoltenVK 实际装入包内，启动脚本按包内相对位置定位。

本次交付构建退出码为 0，包内 416 个游戏资源文件的相对路径和大小与资源源目录一致，无符号链接；ARM64 架构、启动脚本语法、动态依赖路径及本地 ad-hoc 签名检查通过。包大小为 1,115,805,995 字节。本次打包未启动游戏，游戏验收范围沿用下方已有记录。

## 阶段进度

| 阶段 | 当前实现 | 后续工作 |
| --- | --- | --- |
| 3. 独立 ABI | 生成函数、hook 和运行时统一使用项目 `GuestContext`；项目运行时的 `PPCContext` 是同一类型别名 | 保留兼容名称与 ABI 断言，继续验证导入、异常和续执行边界 |
| 4. GuestMemory | 项目持有大端访问、RAM/MMIO、九段 arena、物理别名、页堆、保护、watch、OS 页操作与内存保存恢复；异常登记和函数表适配也在项目源码中 | 跨平台运行与目标硬件验证；libnx arena 尚未实现 |
| 5. 注册与分派 | 项目表注册 15,102 个唯一入口、237 个导入；项目 FunctionDispatcher、函数表、模块绑定和无效目标 trap 接入完整 app | 与 continuation 的导入/嵌套 callback 边界整合 |
| 6A. 基础服务 | 项目持有时钟、异常、日志、文件系统、临界区、控制器分配，以及继承的平台 UI/输入实现 | 实际输入、title 重启和长时间生命周期验收 |
| 6B. 内核线程服务 | 项目持有 Runtime/KernelState/XThread、七类等待、HostThread、guest APC/TLS、HandleTable、ObjectTable 和 guest 对象编排；`KRC1`/`TSC1` 已接入 | 继续覆盖已执行后外部挂起线程与未覆盖的导入/SEH 边界 |
| 6C. 音频 | 项目持有 SDL 设备、PCM/DSP、FFmpeg/XMA、context/ring/worker/client credit、guest callback、guest 分配、设备工厂及 `XA2D` 完整软件状态快照 | 战斗音效与长时播放验收 |
| 7A. Xenos 数学 | 格式转换与 gamma 项目实现，保留差分证据 | 随真实渲染继续验证 |
| 7B. 图形 | 项目持有 PM4、draw/shader、IR/SPIR-V/DXBC、纹理/资源缓存、Vulkan/D3D12 提交、同步和平台 presenter；原生路径直接构建设备 | macOS 已见标题/菜单与 continuation 战斗动画；实际战斗操作、多场景及其他平台仍需验收 |
| 8. 完整 native-only | 完整 app 独立构建成功；构建图无 SDK 源码/include，运行不加载 ReX runtime/GPU 插件；第三方依赖由项目树构建 | 完成实际战斗、长时间音频与游戏存档验收 |

## 当前源码与构建边界

### 项目运行时与宿主生命周期

`src/gh_system_*` 持有继承的 Runtime、KernelState、XThread、模块、分派、guest 对象、ObjectTable、XAM content/profile 等实现。`runtime/src` 持有项目平台 core、文件系统、UI、输入、内核导入和公共图形格式实现，`runtime/include` 保存对应头与项目实现转发。原生构建不再编译外部 SDK 的实现文件。

`GuardianHeroesApp` 持有配置/日志、设备工厂、窗口/overlay/input、XEX 初始化、achievement listener、shader storage、主线程启动/等待和清理。`cmake/GuardianHeroesStandaloneBootstrap.cmake`、`GuardianHeroesApp.cmake` 与 `runtime/CMakeLists.txt` 直接组装完整 app；默认使用已提交的生成代码，codegen 是可选的离线工具。保留的 `GUARDIAN_HEROES_NATIVE_ONLY=OFF` 路径用于旧 SDK 构建兼容。

原生路径通过 `gh::graphics::CreateGraphicsSystem` 和 `gh::audio::CreateSdlAudioSystem` 创建设备。`rex::runtime`、`rexgpu-xenos` 等名称在这条路径上指向项目 target；实际产物是 `libguardian_heroes_runtime.a`、`libguardian_heroes_gpu.a` 及相关项目库。SDL、FFmpeg、Vulkan loader、MoltenVK、shader 工具等第三方代码位于 `runtime/thirdparty`。macOS Vulkan loader 和 MoltenVK 仍作为平台动态库 staging，并由图形路径加载。

### 线程、对象与内存

`gh_kernel.h/.cpp` 的 HostThread 持有原生线程创建、挂起/恢复、FIFO callback、退出与 Join。`gh_kernel_guest.*` 管理 guest APC 链表和 TLS，`gh_kernel_objects.*` 管理句柄槽及引用计数；`gh_system_xthread.*`、`gh_system_util_object_table.*` 与 KernelState 负责 guest ABI、名称映射和 title 生命周期。event、semaphore、mutant、timer、thread、file、notification 统一使用项目 WaitAny/WaitAll/SignalAndWait 状态。对象引用在全局锁外释放，保留 timer/APC 清理的锁顺序。

`gh_guest_arena.*`、`gh_heap.*`、`gh_memory_service.*` 与 `gh_host_memory_*` 持有实际 backing store、九段映射、A/C/E 别名、macOS ARM64 E 区补偿、页表/分配/保护、write watch、OS 页操作和内存保存恢复。`gh_mmio.*` 执行 range、fault 和宿主 load/store 解码；项目 `gh_system_mmio_handler.*` 与 `gh_system_xmemory.*` 保留异常上下文及函数表 ABI 接口。

### 镜像与游戏存档

`gh_image.*` 处理 XEX payload、session key、CBC、原格式 SHA1 block chain 和 LZX；`gh_image_metadata.*` 持有 XEX/PE 元数据，`gh_image_loader.*` 执行 import binding 和 delta patch。项目 XexModule、resolver 和内存服务执行 guest 分配、导入发布和映像映射。

游戏存档通过项目的 XAM ContentManager 挂载可写 HostPathDevice，再经内核 NtCreateFile/NtWriteFile/NtReadFile、XFile 和宿主文件 API 读写。路径和调用算法与迁移前一致。游戏存档与运行线程快照是两条独立链路。

### 音频与图形

`gh_audio_system.*` 实际执行 guest callback worker、client 注册/注销、callback 参数分配和原保存布局；`gh_audio_decoder.*` 连接 XMA MMIO 与 guest context 分配。`gh_audio_runtime.*` 持有 320 个 context、寄存器、worker 和 client credit，`gh_audio_context.*` 执行跨包 frame、loop/skip、192-sample carry 与输出 ring，`gh_audio_xma.*` 持有 FFmpeg codec 状态。关闭时先停止 guest audio worker，等待 guest 完成事件及 native host Join，再停止 XMA、释放 context，最后关闭 client 设备和参数。

`gh_audio_sdl.*` 与 `gh_audio_sdl_driver.*` 持有实际设备、callback、队列、帧缓冲池、静音与关闭。输入规格保持 48 kHz、六个平面大端声道、每帧 256 samples；原六声道/立体声矩阵、增益和大端转换继续沿用。

`gh_gpu_commands.*` 执行 PM4 通用与 draw/shader packet；`gh_gpu_shader*` 持有 shader IR、分析、反汇编及 SPIR-V/DXBC 翻译；其余 `gh_gpu_*` 持有完整 command processor、draw、纹理、shared memory、render target、pipeline cache 和 Vulkan/D3D12 backend。平台窗口、swapchain、设备和 presenter 实现在项目 `runtime/src/ui`。旧头通过 `runtime/include/rex/graphics` 转发到这些项目实现，原有格式和类接口继续保持。

项目树内的 MoltenVK `MVKImage.h/.mm` 以同一 recursive mutex 保护 drawable 获取/释放，并让返回的 drawable 保持 retain/autorelease，覆盖 command-buffer block 复制前的生命期；completion 在 image 释放前持 detachment lock 通知 swapchain。修复后的 continuation app 已显示持续动画的战斗场景。

## 运行线程 continuation

项目的指令预算执行器以 `GHC3` 保存完整 GuestContext、frame/PC、临时寄存器和 pending import，仍读取旧 `GHC2` frame。title program 包含 13,046 个 step、2,051 个 atomic-host、5 个 queued import 和 0 个 unsupported 入口。可选 `guardian_heroes_continuation_app` 安装这套 program；默认 app 继续使用原编译函数调用路径。

FunctionDispatcher 只为 guest thread 启动 Machine；音频等 host callback 使用原编译函数，并由所属设备的 Pause 排空后再进入快照边界。host callback 不注册 guest coordinator，其执行栈无需转成可恢复 frame。

`Program` 注册时逐项确认合法 PC 是否连续相差 4 字节；连续集合使用范围与相对对齐查验，包含锁区域合并空洞的稀疏集合继续二分查找。当前 PC、Next、Call 返回地址和保存恢复的合法性检查均保留，指令预算、逐指令 safepoint 与快照格式不变。

`KeDelayExecutionThread`、Ke/Nt 单对象与多对象等待接入持久 `QueuedWait`；同一适配器也支持 SignalAndWait。等待跨预算让出期间保持原 dispatcher 注册，保存 FIFO order、已消费的成功/abandoned/APC/timeout 结果、WaitAll 和 guest tick deadline，保留 Pulse、mutant 所有权及 exactly-once signal。匿名内部 handle 保证 guest 关闭原 handle 后，仍在等待的对象继续进入对象表快照。单独的 frame `Stop::Restore` 拒绝含 pending import 的现场，避免仅倒带 PC 却保留未来对象状态。

`Runtime::Save/Restore` 使用 `GRS1` 聚合 GPU、音频、`KRC1` kernel 和完整 guest memory。保存先暂停并排空音频 callback、timer 与 GPU 写入源，关闭 deferred dispatch 并等待正在执行的任务返回，再等待 guest safepoint，最后冻结双时钟。未执行的 deferred closure 不序列化；Machine 捕获后再次核对 dispatch queue/in-flight 为空，确认后才替换内存。恢复先退休旧 native hosts、放弃旧 guest storage 所有权，再替换内存并重建对象。新的 hosts 在关闭的 start gate 内恢复 mutant 所有权与已保存的等待注册；设备与时钟恢复后才开放 guest gate。破坏性恢复失败保持暂停并拒绝继续保存/恢复，关闭仍能释放相关资源。

主映像恢复直接匹配当前 `executable_module_`，辅助映像从已加载列表查找；校验同一 path/HMODULE 并复用原对象，避免重新加载映像覆盖恢复后的 guest 页。

`TSC1` 单独保存创建后从未 Execute 的线程、startup 参数/PPCContext、PCR/TLS/stack/scratch、priority 和 suspend count；恢复后新的 native host 继续挂在 creation gate，最后一次 Resume 才走原有预启动 APC 和 Execute。已执行的外部挂起线程若没有可保存 Machine，仍拒绝快照。`TMR1` 保存 timer reset/signaled/armed、剩余时间、周期和 APC routine/argument/owner，并把 owner 绑定到新的 XThread。

`FIL1` 保存同步或当前已完成宿主 I/O 的异步文件位置、完成事件消费状态，以及目录匹配规则和枚举游标；恢复通过原 VFS entry 重开文件，包括挂载根目录。关联 IO completion port 或 delete-on-close 的句柄拒绝快照。文件系统路径和外部磁盘内容不随内存现场倒带。

音频 `XA2D` 捕获 XMA 寄存器/context 分配、跨包/帧/carry/subframe/loop 与 FFmpeg 精确 decoder checkpoint、callback credit、endpoint 软件队列和 SDL resampler history/phase、map/gain/ratio/flush。恢复前释放旧 callback wrapper 的 guest heap 所有权，恢复失败不启动 worker/endpoint。已交给 CoreAudio/DAC 的样本无法倒放。

Vulkan 快照包含 command processor CPU/ring/register/shader/gamma/counter 状态、GPU-authoritative shared pages、延迟 readback history、未结束 occlusion query 的累积值，以及 HostRT 每个 MSAA sample 的原始 color/depth/stencil、EDRAM 和独立 ownership。host submission/frame ID 保持递增，缓存重建；swapchain 已显示图像在下一次 guest present 前保持原画面。D3D12 明确拒绝快照。

atomic-host 内部的嵌套 guest callback、其他阻塞导入和 SEH 宿主栈仍须完整返回后才能形成 safepoint。真实 title 的往返结果记录在下方验证记录中。

## 验证记录

### 完整 app 与本轮运行

| 证据 | 已确认结果 |
| --- | --- |
| `/tmp/gh-native-app-build.log` | macOS ARM64 Release 完整 app 构建成功 |
| `/tmp/gh-native-app-final-build.log` | 包含 title continuation 桥的最终 app 与 kernel adapter target 构建成功 |
| `/tmp/gh-native-kernel-adapter.log` | 实际项目 runtime 的 14 个 kernel adapter case 通过 |
| [构建来源记录](validation/native-runtime-project-build-2026-10-17.json) | 编译源/include、静态库、构建图、动态依赖与 Vulkan staging 来源已核对；记录内 SDK 路径引用和项目外源/include 均为 0 |
| [菜单截图](validation/native-runtime-project-menu-2026-10-17.png) | 无 ReX runtime/GPU 插件文件的独立验证 app 已显示标题和菜单 |
| `/tmp/gh-native-app-runtime.log` | 11:43～11:46 的实际运行中记录到非零 PCM，包含连续 256 个非零左右声道 sample 的帧 |
| `/tmp/gh-native-app-runtime-open.log` | 当前独立 app 通过 LaunchServices 启动的运行日志 |
| [continuation 战斗截图](validation/native-runtime-continuation-battle-2026-10-17.png) | MoltenVK present 故障修复后，可选 continuation app 已进入并持续渲染战斗场景；实际战斗操作与完整可玩性尚未验收 |

来源记录中的 `otool -L` 结果未包含 ReX runtime/GPU 动态库，Vulkan loader/MoltenVK 的 staging 文件与项目构建产物一致。截图、运行日志与构建来源分别记录运行、输出和构建边界。

[实际 title 日志](validation/native-runtime-loaded-title-2026-10-17.log) 记录 15:48:29 的 F5 保存与 15:48:37 的 F9 恢复，现场均为 515,508,145 字节；恢复后重新打开双声道 MacBook Air 扬声器 endpoint，并继续输出非零 PCM。用户确认该次 F5/F9 正常、有声音，但声音断续。恢复 endpoint callback 在 Release 下必须在断言外归还 credit；该修复已由开启 `NDEBUG` 的集成 fixture 和[真实 F9 日志](validation/native-runtime-audio-restored-title-2026-10-17.log)验证：17:05:43 恢复 357,310,404 字节后，持续数千帧非零 PCM 与 endpoint 消费。最终修复版已获用户确认“声音和 f9 正常”。应用尊重系统默认输出设备，并在打开设备前查询物理声道数，双声道扬声器使用原有立体声混音路径。

### 音频修复验收

[实际 CPU profile](validation/native-runtime-audio-continuation-profile-2026-10-17.json) 使用匹配 UUID 的 Release 二进制符号化，确认续执行器的当前/下一 PC 二分查找是主要开销。连续 PC 查验优化后的[实际运行日志](validation/native-runtime-audio-dense-pc-title-2026-10-17.log)中，标题 BGM callback 平均为 2.705 ms，低于每帧 5.333 ms 的供给预算；启动后 +4～+34 秒统计有 5,633 个消费块、7 个静音补帧块，占比 0.1241%。此前对应供给开销约 10.7 ms，静音补帧约 55.3%。性能窗口覆盖启动后 +4～+34 秒，按日志统计，未对齐同一音频块边界。

用户确认修复版“声音和 f9 正常”。[完整验收记录](validation/native-runtime-acceptance-2026-10-17.json)将当前标题听音与 F5/F9 往返记为通过；战斗音效、游戏进度存档和长时间运行继续分别验收。用户已授权自行读取后销毁测试进程，本轮测试实例均已结束。

### 已有聚焦验证

下列记录保留此前实际完成的验证范围。独立内存、内核与镜像 fixtures 不链接 ReX runtime；shader 差分覆盖继承的格式/工具和翻译算法。

| 验证入口 | 已有结果与覆盖 |
| --- | --- |
| `run_gh_host_thread_test.py` | 通过；原生线程创建、挂起/恢复、callback、退出与取消点终止 |
| `run_gh_posix_exception_test.py` | 通过；真实子进程保护页 fault 恢复、未处理 fault 的默认信号退出，以及原普通/SA_SIGINFO handler 转发 |
| `run_gh_kernel_test.py` | 通过；统一等待、Pulse、abandonment、timer capture、临界区取消、所有权重建，以及持久等待 FIFO/已消费结果/WaitAll/SignalAndWait/APC/deadline 和 mixed native/queued 顺序 |
| `run_gh_kernel_state_test.py` | 通过；guest APC、TLS、句柄槽与引用生命周期 |
| `run_gh_guest_arena_test.py` | 通过；九段实际映射/别名、4 KiB 补偿、释放重建、实例隔离；Switch 未实现分支语法检查通过 |
| `run_gh_memory_core_test.py` | 通过；独立页堆分配/子页保护、保存恢复字节、watch 失效、MMIO 直接访问及真实 ARM64 LDR post-index fault；16 KiB host 的 E alias 初始化与 4 KiB 补偿 |
| `run_gh_memory_service_test.py` | 通过；真实 OS arena/页操作、布局与 MMIO 安装顺序、零页保护、E alias/共享物理字节、watch 失效、完整 Save/Restore 与 host file aliases |
| `run_gh_image_metadata_test.py` | 通过；真实 XEX optional headers/security/page 表、9 个 PE section 与 487 条原始 import records 逐项对等；XEX1/patch Refresh/exports/截断拒绝 |
| `run_gh_image_test.py` | 通过；真实 default.xex 解码 10,813,440 字节，与保留的旧解密/解包流程逐字节相同且 PE=MZ；BASIC CBC 跨 gap、AES、长度边界通过 |
| `run_gh_image_loader_test.py` | 通过；重复 ordinal/变量 slot/import stub，overlap/zero/LZX delta，完整 header/AES key 旋转/CBC/SHA1 块链、映像 grow/shrink 和失败副作用；未使用真实 XEXP |
| `run_gh_audio_runtime_test.py` | 通过；真实 XMA 精确 decoder checkpoint、跨 input buffer/carry/subframe/loop 恢复，320-context pool/MMIO、endpoint/credit 和暂停/停止 |
| `run_gh_audio_test.py` | 通过；PCM/队列与六声道 impulse DSP |
| `run_gh_audio_output_test.py --system-output` | 通过；真实系统设备打开、消费/credit、关闭/重开、静音；四帧真实 BGM XMA 含非字节对齐 frame，逐 sample 对等，Reset 后重放对等 |
| `run_gh_graphics_test.py` / `gh_gpu_commands_test.cpp` | 通过；图形契约及 PM4 执行；新增 draw DMA/register/major/viz、后端失败继续处理、shader source/address 和截断 packet 覆盖 |
| `run_gh_gpu_shader_test.py --reference-source …` | 通过；8 个真实 title shader、两组 feature、interlock 与四种 depth mode；48 个 SPIR-V 和 32 个反汇编输出共 1,218,344 字节，与迁移前算法逐字节相同 |

[本轮快照验证记录](validation/native-runtime-snapshot-validation-2026-10-17.json) 汇总实际命令与证据：Runtime 聚合 fixture 使用真实 guest threads/kernel/memory 和设备 test doubles，覆盖短流拒绝、恢复失败保持暂停与 Shutdown；AudioSystem 集成 fixture 覆盖 `XA2D`/legacy `XAUD`、heap ownership 和 endpoint/credit；Apple M5/MoltenVK 的真实 Vulkan fixture 恢复 10,586,944 字节现场，核对 GPU-only 字节、raw color/depth/stencil 每个 sample、EDRAM/ownership、readback 和 active query。设备 fixture 与真实已载入 title 的 F5/F9 恢复分别验收。

`guardian_heroes_continuation_waits_test` 已通过真实五线程、完整 guest memory/kernel 的 pending import 保存恢复：关闭原 handle 后仍恢复等待对象、Pulse/WaitAll、SignalAndWait 仅 signal 一次、guest-time KeDelay 和单次 alertable APC；frame-only restore 被拒绝后原等待仍可继续保存和恢复。

`guardian_heroes_continuation_restore_test` 已通过真实新 hosts 的完整 kernel/memory 恢复，覆盖 timer deadline/period/APC 新 owner 与 owner 退出后的信号、mutant 递归/abandonment、`TSC1` 多次 Suspend 及预启动 APC/startup、scratch 和退休 guest storage，以及 deferred dispatch 暂停、入队后拒绝快照、单次恢复和暂停状态关闭；同时核对 host callback 走编译函数、参数/结果/栈保持正确，guest thread 继续进入 Machine 并恢复。

`guardian_heroes_xfile_continuation_test` 已通过真实 VFS 文件与挂载根目录的 `FIL1` 往返：异步读写位置、完成事件消费前后、目录匹配与游标、明确拒绝的端口/delete-on-close，以及原有文件格式恢复。

`run_gh_audio_output_test.py` 本轮精确比较软件 stream 的 resampling/history 和 paused frame queue；当前 dummy backend 不可用，device callback 分支跳过。此前 system-output 记录保留原范围。

独立源码审阅已对照迁移前实现核对音频、PM4、image、arena、存档路径、对象和线程的调用顺序。迁入实现保留版权与原错误处理。

## 机器可读边界

运行 `python3 guardian_heroes/tools/audit_runtime_boundaries.py`，默认检查 `out/native-app/compile_commands.json`；可用 `--build-dir` 指向其他已配置的独立构建。快照保存于 `docs/native-runtime-boundaries.json`。

- `native_only_builds_complete_app=true`、`native_only_libraries_only=false`。
- `xenos_project_backend_present=true`，原生路径直接选择项目 GPU factory。
- `guest_context_unified_runtime_abi=true`；项目运行时与生成代码共享 GuestContext。
- `mixed_wait_fallback=false`、`legacy_wait_handle_dispatch=false`、`native_wait_mirrors=false`。
- 来源明确记录为项目维护的 ReXGlue/Xenia fork，并分别报告项目服务、继承 runtime、平台设备与第三方依赖。
- `full_runtime_replaced=false`；源码/构建图审计与游戏运行验收分别记录。

## 下一阶段与完成标准

1. 在已观察到战斗动画的 continuation app 中完成实际战斗操作、场景切换、控制器/键鼠输入、战斗音效和长时间运行验证，保留实际画面与运行记录。
2. 在明确的 `--user_data_root` 下触发游戏内保存，核对真实文件写入；通过游戏重新打开存档确认读取及进度恢复。单独创建目录、header 或文件不能代替游戏保存/读取验收。
3. 在更多场景与长时间运行中继续验证全设备往返；继续覆盖已执行后外部挂起线程与实际 title 生命周期。atomic-host/嵌套 callback/SEH 仍需返回原宿主栈边界。
4. 验证 title 重启与关闭时的对象、APC/timer、音频 worker 和 GPU 资源生命周期；保持 guest 地址布局、ABI、画面与声音行为。
5. 推进 Windows 原生 Vulkan 构建，验收启动、输入、渲染、声音、游戏存档和 F5/F9；保留当前 D3D12 实现。
6. 补齐 Android 的构建与应用生命周期，接入资源、可写数据、控制器/触控和音频，在实际手机上验证 Vulkan 特性、内存和性能。
7. Linux 与 Switch 按目标平台分别推进。Switch 1 Atmosphere 的独立 arena、线程、GPU、音频和目标硬件性能仍需实现及验证。

阶段 8 的最终验收对象是完整 app：使用项目运行时启动、输入、渲染、发声、战斗并读写游戏存档，不加载 ReX runtime/GPU 插件。运行线程快照恢复另需真实续执行证据。

## 限制

- 本轮已观察战斗动画，实际战斗操作、多人、游戏存档重新读取、长时间音频与 title 重启尚未完成验收；当前标题听音与 F5/F9 已获用户确认正常。
- 默认 app 未安装 title continuation；可选 continuation app 的 Release F9 credit 回归已修复并观察到恢复后持续 PCM；已执行后外部挂起线程以及未覆盖的 in-flight import/SEH 尚未验收。
- D3D12 快照未实现；Vulkan swapchain 已显示图像和已交付系统音频硬件的样本不序列化。快照采用同一构建的 ABI，不提供跨版本迁移。
- macOS pthread cancellation 依赖可达取消点；无取消点忙循环无法保证强制终止，Join 防止提前释放仍在使用的 guest 栈。
- Windows/Linux/Android 分支与 Switch 1 Atmosphere 尚无本轮目标硬件验收。
- XEX 解码使用临时 vector 后复制到 guest image，额外占用一份解码后镜像内存。
- 现存 TOML 路径配置在路径冻结后加载，目录覆盖可能被忽略；本轮保持既有行为，验证路径使用 CLI 显式指定。
