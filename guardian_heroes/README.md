# Guardian Heroes

Xbox 360 XBLA 版本的静态重编译工程。完整 app 默认使用项目维护的 ReXGlue/Xenia 派生运行时，Runtime、内核、内存、音频、图形和第三方依赖均由项目源码构建。macOS ARM64 Release 已构建并交付未压缩 app，标题听音和 continuation F5/F9 已获用户确认正常。

当前交付优先推进 macOS、Windows 和 Android，共用 Vulkan 路线；Windows x64 Vulkan Release 已在 Windows 上编译并完成 12 秒启动检查，完整游戏流程仍待验收，Android 尚待构建及设备验收。进度、证据和完成标准见 [原生运行时解耦进度](../docs/native-runtime-decoupling-progress-and-plan.md)。

## 文件

- `assets/`：游戏资源，包含运行所需的 `default.xex`；源码包不附带此目录。
- `guardian_heroes_manifest.toml`：主程序与资源根配置。
- `generated/default/`：已提交的静态重编译代码，登记 15,102 个入口，含 237 个导入。
- `src/`：项目宿主、服务、运行时与图形后端实现。
- `runtime/`：项目平台实现、兼容头、构建文件和第三方依赖源码。
- `tests/`、`tools/`、`templates/`：聚焦验证、离线生成工具和模板。
- `localization/`：汉化文本、字体配置及原工程中的资源产物；源码包只保留 JSON 文本与配置。
- `out/native-app/guardian_heroes_continuation_app`：带 title continuation 的可选执行路径；默认 `guardian_heroes` 使用原编译函数路径。
- `../dist/2026-10-17-macos-arm64/Guardian Heroes.app`：已交付的独立 ARM64 app，内置资源与 Vulkan／MoltenVK 库；源码包不附带构建产物。

游戏 Title ID 为 `0x58410B2B`，映像基址为 `0x82000000`，入口 `0x825D0F40` 对应 `xstart`。

## 生成与构建

在 `guardian_heroes` 目录操作。需要 CMake 3.25+、Ninja 和 C++23 工具链：Windows／Linux 使用 LLVM Clang 18+；macOS 使用 Xcode 16+ 的 AppleClang。将 CMake、Ninja 和编译器加入 `PATH`，Windows 还需在配置了目标架构的 Visual Studio 开发者终端中运行。

`GUARDIAN_HEROES_NATIVE_ONLY=ON` 为默认设置，构建完整 app，使用 `runtime/` 内的运行时和第三方依赖以及 `generated/default/` 中的生成代码，无需相邻 SDK 或游戏资源即可编译。运行与重新生成代码时需要另行提供 `assets/`。

Presets 中保留的 `REXSDK_DIR` 和 SDK CPM 缓存设置用于 `GUARDIAN_HEROES_NATIVE_ONLY=OFF` 的旧构建兼容路径。原生构建不使用这些 SDK 源码路径。

| 平台 | x86_64 Release preset | ARM64 Release preset | 图形后端 |
| --- | --- | --- | --- |
| Windows | `win-amd64-release` | `win-arm64-release` | 默认 D3D12，后续优先显式使用 Vulkan |
| Linux | `linux-amd64-release` | `linux-arm64-release` | Vulkan |
| macOS | `mac-amd64-release` | `mac-arm64-release` | Vulkan／MoltenVK |

Windows／Linux 的 ARM64 presets 必须配合 ARM64 原生编译器与依赖，不提供跨架构工具链。macOS presets 指定单一目标架构，部署版本设置为 13.3。各平台均保留同名的 `debug` 和 `relwithdebinfo` 配置。

macOS ARM64 构建：

```sh
cmake --preset mac-arm64-release
cmake --build --preset mac-arm64-release --target guardian_heroes --parallel
```

需要 F5/F9 的 title continuation 版本时，将构建目标改为 `guardian_heroes_continuation_app`。

Windows x86_64 Vulkan 构建（PowerShell）：

Visual Studio 2022 14.44 工具链的标准库要求 LLVM Clang 19+；搭配该版本时请勿使用 Clang 18。如果系统有 Microsoft Store 的 Python 别名，请通过 `-DPYTHON_EXECUTABLE=实际的/python.exe` 指定已安装的 Python，供 glslang 生成构建信息。

```powershell
cmake --preset win-amd64-release -DREXGLUE_USE_VULKAN=ON -DREXGLUE_USE_D3D12=OFF
cmake --build --preset win-amd64-release --target guardian_heroes --parallel
```

应用工厂按 `any` 选择后端，同时编入两种后端时优先 D3D12，因此 Vulkan 构建需显式关闭 D3D12。2026-10-17 的 Windows 增量包记录了使用 Clang 19.1.7 的 Release 构建及 12 秒启动检查：从其他工作目录启动时，无需指定资源和 GPU 参数即可找到 EXE 旁的 `assets/default.xex` 并初始化 Vulkan。对应源码修正已合入本工程；完整游戏、音画效果和 continuation 目标的 F5/F9 尚待 Windows 验收。Android 的应用入口、APK、生命周期和设备接入仍需补齐。

Linux x86_64 构建：

```sh
cmake --preset linux-amd64-release
cmake --build --preset linux-amd64-release --target guardian_heroes --parallel
```

若 LLVM 以版本化名称安装，可在配置命令增加 `-DCMAKE_C_COMPILER=clang-20 -DCMAKE_CXX_COMPILER=clang++-20`。需要降低构建内存占用时，在 `--parallel` 后指定并行任务数。

原生构建默认直接编译已提交的生成代码。需要重新生成时，先提供游戏资源，并在配置时指定离线工具：

```sh
cmake --preset mac-arm64-release -DGUARDIAN_HEROES_CODEGEN_EXECUTABLE=/path/to/rexglue
cmake --build --preset mac-arm64-release --target guardian_heroes_codegen
```

指定工具后，`guardian_heroes_codegen` 会执行 `rexglue codegen guardian_heroes_manifest.toml`。未指定工具时，该目标不重新生成文件。

## 静态重编译与运行时边界

生成代码保留 PowerPC 执行语义，使用项目 `GuestContext`、GuestMemory、大端访问、导入和间接分派。项目运行时的 `PPCContext` 是同一上下文的兼容别名。

1,803 个 native 替换编入 `guardian_heroes_native` 静态库。全部 15,102 个入口同时登记到项目地址表；未登记的间接调用目标抛出 `InvalidFunctionTrap`。完整 app 的线程、文件、输入、音频和图形实现已迁入项目维护，原生路径不加载 ReX runtime/GPU 插件。

`generated/rexglue.cmake` 和旧 SDK 路径用于兼容构建。历史 Switch 路线见 `../docs/switch-porting.md`；当前源码所有权和交付范围以解耦进度文档为准。

## 原生计算替换

`src/native_logic.cpp` 通过 SDK 的强符号 hook 接入普通 C++ 替换，保留原始
`__imp__` 入口和自动生成代码，不使用平台专用指令：

- `sub_82498748`：坐标比例计算，保留逐步单精度舍入、寄存器和栈临时数据。
  非有限数值沿用原实现，保留 NaN 数据。
- `sub_828885C0`：内存填充，使用 `std::memset` 合并存储循环。
- `sub_82887388`：小块内存复制，使用 `std::memcpy` 合并复制循环。
- `sub_82886F00`：大块复制中，目标 128 字节对齐、源 8 字节对齐且长度至少
  128 字节的分支使用 `std::memcpy`，其余分支沿用原实现。

内存替换保留原函数的寄存器和栈状态。重叠复制、物理地址别名、MMIO 和跨越
内存区域的操作沿用原实现；图片、音乐和战斗规则不变。

另有 20 个叶函数按用途分组接入，保留完整 PPC 寄存器状态和内存结果；
可能重叠的访问保留原有读写顺序：

| 源文件 | 函数 | 用途 |
| --- | --- | --- |
| `src/native_state.cpp` | `824D0BE8`、`82435950`、`8243A9D8`、`82496508`、`82425D18` | 队列与状态判断、计数、槽位有效性、条目选择 |
| `src/native_bits.cpp` | `8278E858`、`8267A938`、`82803DB8`、`828ABA20`、`82434A00` | 索引查表、二分查找、槽位读取、整数区间与位查询 |
| `src/native_arrays.cpp` | `824AB4E0`、`824A8AD8`、`828192B0`、`8268D158`、`824A6CE0` | 范围清空、记录定位、固定字段复制、标志映射与索引读取 |
| `src/native_bytes.cpp` | `8246B550`、`8241BAF0`、`824A7998`、`82498068`、`82888BD8` | 字节计数、五字段表复制、有界调整、空闲槽位扫描与字符串复制 |

表中省略函数名的 `sub_` 前缀。整数运算保留原来的回绕和符号判断；状态提交、
有界调整等函数保留原规则。字符串复制保留前向重叠语义，零填充复用原生内存边界判断。

此前增加的 100 个普通 C++ 叶函数，使替换累计达到 124 个：

| 源文件 | 数量 | 用途 |
| --- | ---: | --- |
| `src/native_queries.cpp` | 25 | 资源地址、状态和位查询、字符串比较、定点限幅、余数计算 |
| `src/native_links.cpp` | 25 | 链表摘除、槽位和记录搜索、范围判断、句柄与偏移换算 |
| `src/native_flags.cpp` | 25 | 位标志、渲染状态字段、槽位分配、定点系数和格式宽度解码 |
| `src/native_records.cpp` | 25 | 记录初始化与重置、常量表写入、UTF-16 长度、步进字节复制 |

记录批量写入只覆盖原函数写过的字节，保留结构体空隙；物理地址和跨区域访问
沿用原实现。链表、槽位和重叠复制保留会影响结果的交错读写。所有函数保留
完整 PPC 上下文，包括临时寄存器高位、CR、XER 与 CTR，未改图片和音乐。
原函数中不终止的输入不纳入差分测试，也不改变其行为。

2026-10-15 再按相关访问模式增加 500 个叶函数，累计 624 个强符号替换：

| 源文件 | 数量 | 用途 |
| --- | ---: | --- |
| `src/native_accessors.cpp` | 200 | 对象字段、全局指针、指针链与子对象地址读取 |
| `src/native_writers.cpp` | 153 | 字段赋值、记录重置、字段复制与简单更新 |
| `src/native_predicates.cpp` | 103 | 位提取、字段相等判断与有符号字段查询 |
| `src/native_constants.cpp` | 44 | 固定地址查询 |

这些基础访问函数保留完整 PPC 上下文、访问宽度与影响结果的交错读写顺序；
连续零写仅在普通内存范围内合并。对应的 `tools/native_*_manifest.json` 记录
原函数位置与调用方证据，未找到直接调用方的条目不作为高频函数结论。

后续批次按可直接对照的函数族增加 1,179 个替换，累计 1,803 个：

| 源文件 | 数量 | 用途 |
| --- | ---: | --- |
| `src/native_addresses_ext.cpp` | 399 | 固定地址、子对象偏移与索引地址计算 |
| `src/native_queries_ext.cpp` | 226 | 可空相对表、字段、状态、位与条件查询 |
| `src/native_writers_ext.cpp` | 209 | 整数字段、记录、位域和简单更新 |
| `src/native_float_ext.cpp` | 345 | 简单浮点加减乘、乘加、读取与写入 |

有限的单精度加减乘直接使用 float 运算，并保留原 double 寄存器结果；乘加仍保留
double 乘加后单精度舍入的顺序。非有限输入、物理别名和非常规地址沿用原函数。
差分验证使用与生成代码 Release 一致的严格浮点计算选项。
本批原函数位置与调用证据见 `tools/native_*_ext_manifest.json`。

在 macOS／Linux 完成 Ninja 构建配置后，可以对照实际生成函数验证：

```sh
python3 tests/run_native_logic_test.py --build-dir out/build/mac-arm64-release
```

Linux 使用对应构建目录。计算测试覆盖边界值、随机值和四种舍入模式；内存测试覆盖
对齐、分块边界、重叠、栈缓冲区别名及地址区域。测试比较完整 PPC 上下文与临时内存，
并输出单函数计时；单函数计时不能作为整场战斗的帧率提升。

此前 124 个替换的 macOS ARM64 对照通过 429,900 个用例，其中上一批 100 个
函数的用例为 14,695 个；证据位于工作区 `logs/native-100-test.txt`。
新增 500 个函数另通过 43,548 个原函数对照用例，比较完整 PPC 上下文、栈和
相关内存区域；证据位于 `logs/native-500-test.txt`。这 500 个函数可单独运行：

```sh
python3 tests/run_native_logic_test.py \
  --suite native_constants_test --suite native_accessors_test \
  --suite native_writers_test --suite native_predicates_test
```

新增 1,179 个函数通过 150,158 个原函数对照用例，覆盖完整 PPC 上下文、
内存读写、浮点舍入与主机浮点状态；证据位于工作区 `logs/native-1180-test.txt`
和 `logs/native-1179-queries-test.txt`。本批可单独运行：

```sh
python3 tests/run_native_logic_test.py \
  --suite native_addresses_ext_test --suite native_queries_ext_test \
  --suite native_writers_ext_test --suite native_float_ext_test
```

其他平台构建与整场战斗帧率尚未测量，累计替换数量不代表实际性能增幅。

## 启动

运行时使用完整的 `assets` 目录，`xenos` 插件与宿主可执行文件位于同一构建目录。保持该目录和相邻运行时库完整，勿仅拷贝可执行文件。

macOS ARM64：

```sh
./out/build/mac-arm64-release/guardian_heroes --gpu_plugin=xenos --game_data_root=assets
```

Linux x86_64（需要可用的 Vulkan 驱动）：

```sh
./out/build/linux-amd64-release/guardian_heroes --gpu_plugin=xenos --game_data_root=assets
```

Windows x86_64（PowerShell）：

```powershell
.\out\build\win-amd64-release\guardian_heroes.exe --gpu_plugin=xenos --game_data_root=assets
```

其他架构使用对应 preset 的构建目录。在其他工作目录启动时，将 `--game_data_root` 改为 `assets` 的绝对路径。

无手柄时可在启动命令末尾添加 `--mnk_mode=true`，启用 SDK 现有键盘控制器模拟。默认关闭；窗口获得焦点后，`Return`／`X` 对应 Start，`Space`／`;` 对应 A，`WASD` 对应左摇杆。具体游戏操作仍需运行验证。

## 解包

解包命令已加入 SDK；输出目录须为空或尚不存在。在工作区根目录使用：

```sh
./rexglue-sdk-main/out/mac-arm64/Release/rexglue extract \
  './Guardian Heroes/000D0000/6E2D21B55852DF2B0144A2974CF7487359180C2758' \
  './guardian_heroes/assets'
```

## 配置与 SDK 接入

Manifest 补充 17 个经原始指令与调用路径确认的入口，包括 CRT 初始化函数和对象虚表 thunk，保持原始指令语义。运行阶段修复 Vulkan presenter 成功创建管线后未保存交换链格式、导致每帧重建管线的问题；Linux 动态库暂存沿用 SDK 现有机制。

Manifest 对 `0x826C6784` 的间接跳转使用现有 `switch_tables` 配置：以 `r11` 为索引，保留原字节表到下一张已引用表之间的 16 项映射。原始字节和相邻表引用见工作区 `logs/switch-table-analysis.md`。

SDK 增加 `extract` 命令，并修正源码构建的三处接入：公开头文件所需的 ImGui 包含目录、已有 USB 摄像头导出源文件的构建列表、macOS Vulkan ICD 文件的定位路径。

## 限制

生成代码是 PowerPC 指令的 C++ 转换结果，原始函数名、变量名与工程结构不会自动恢复。

本游戏的 237 个导入中有 24 个使用 SDK 现有 stub，包括部分输入控制、摄像头、网络和系统 UI；这些 stub 不设置返回码或输出参数，清单见工作区 `logs/import-summary.json`。摄像头状态接口仅报告未连接，`XamContentGetLicenseMask` 的默认许可掩码为零。

Windows／Linux 和 macOS x86_64 的命令用于跨平台构建准备，尚未在这些平台验证构建与运行。macOS ARM64 已确认插件加载、GPU 设备与交换链建立、guest 帧循环和音频端点初始化。累计 624 个替换的版本已观察到中文主菜单正常绘制。累计 1,803 个替换的版本已启动并显示标题画面；这次验证未验收菜单输入、实际音效、战斗与存档，也未测量整场战斗帧率。最新启动记录见 `logs/native-1179-validation.json`。

完整解包的数量、目录及文件大小核对结果见工作区 `logs/extraction-verification.json`；本阶段成功的 Release 构建日志见 `logs/build-guardian-heroes-runtime.log`，运行结论见 `logs/runtime-title-menu.md`。
