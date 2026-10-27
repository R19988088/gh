# Guardian Heroes XBLA 解析与静态重编译计划

## 结论

推荐继续使用本地 ReXGlue SDK，先将 XBLA 内容包解出为普通目录，再把其中的 `default.xex` 交给现有重编译流程。无需为这个游戏另写 STFS 解析器，也无需更换为 XenonRecomp。

“这个项目不能直接读取这个格式”需要细分：ReXGlue 的重编译入口不能直接接收该内容包，但 SDK 内部已经具备 STFS 读取能力。本次已补齐 `extract` 命令行接入。XenonRecomp 当前源码同样不能直接读取该包，换项目不能消除这一步。

两者主要完成 PowerPC 指令到 C++ 的静态重编译，不会自动恢复原始类名、变量名和原工程。若目标是理解游戏逻辑，后续仍需结合反汇编／反编译工具进行函数识别和人工整理；若目标是原生运行，则还需要运行时和游戏适配。

## 一、实际文件与源码依据

分析日期：2026-10-14。

本地 SDK 实际以 `rexglue-sdk-main.zip` 保存，ZIP 注释标识源码版本为 `c94f5ebdcb3c9d1a460ca48e04f9758448f8d518`。以下 ReXGlue 结论针对这个快照。

对游戏文件读取包头、首个目录块和主程序起始字节，得到：

| 项目 | 结果 |
| --- | --- |
| 文件 | `Guardian Heroes/000D0000/6E2D21B55852DF2B0144A2974CF7487359180C2758` |
| 文件大小 | 967,532,544 字节，约 922.71 MiB |
| 包魔数 | `LIVE` |
| 内容类型 | `0x000D0000`，XBLA 内容 |
| 存储卷类型 | `0`，STFS |
| 显示名称 | `Guardian Heroes` |
| Title ID | `0x58410B2B` |
| 文件表 | 7 个块，起始逻辑块为 0 |
| 主程序 | 根目录 `default.xex`，3,178,496 字节，约 3.03 MiB |
| 主程序魔数 | `XEX2` |
| 已见资源 | `media` 目录、`ArcadeInfo.xml`、图片、`.mrg`／`.mrgc`、`font.xprc`、`open.wmv`、`shader.mrg` |

因此，这不是“没有 XEX 的游戏”，而是“XEX 和资源被装在 STFS 包内”。把整个文件改名为 `.xex` 无效；必须按文件表及块链读取内部文件。

### ReXGlue 的能力与缺口

| 源码位置（相对 SDK 根目录） | 已确认行为 |
| --- | --- |
| `src/filesystem/devices/stfs_container_device.cpp` | 校验内容包魔数、读取 STFS 目录、处理块地址和文件块链 |
| `include/rex/filesystem/devices/stfs_xbox.h` | 定义 `CON `／`LIVE`／`PIRS` 包头、卷描述和目录结构 |
| `src/filesystem/devices/stfs_container_file.cpp` | 通过 `ReadSync` 按文件块列表读取数据 |
| `src/filesystem/CMakeLists.txt` | 上述容器实现已编入 `rexfilesystem` |
| `src/filesystem/vfs_dump.cpp` | 保留导出文件的旧实现，但未列入构建，末尾工具入口被注释 |
| `src/rexglue/main.cpp` | 原快照注册 `init`、`codegen` 和重编译测试命令；本次增加 `extract` 注册 |
| `src/codegen/project_recompiler.cpp` | 把入口当作宿主 XEX 文件，建立游戏目录，再调用 `LoadXexImage` |
| `src/system/runtime.cpp` | `SetupVfs` 将宿主目录挂载为 `game:`／`d:` |
| `src/system/user_module.cpp` | 模块魔数分支只接收 XEX1／XEX2／ELF，不接受 `LIVE` |

这说明应当在工具入口层补接入，保留现有 XEX 加载器和代码生成器。

### Xenia 怎样读取“非 XEX”

参考 Xenia Canary 源码版本 `40cee11ab14cb9ad4286232dabfe3c69fc32b806`：

```text
外部文件
  → GetFileSignature：按文件内容识别格式
  → LIVE / CON / PIRS 分支
  → MountPath：挂载内容包
  → STFS 目录与块链解析
  → LaunchStfsContainer / FindLaunchModule
  → game:\default.xex
  → 常规 XEX 加载与执行
```

Xenia 没有把整个 LIVE 文件当作机器码；它先提供包内虚拟文件系统，再读取其中的 XEX 和资源。无扩展名不妨碍这个过程。

当前 Canary 的容器实现已拆分到 `xcontent_container_device` 和 `xcontent_devices/stfs_container_device`。ReXGlue 保留了较早形态的同源实现，应参考处理流程，避免整套替换成当前 Canary 的目录结构。

Canary 源码还提供独立的 `xenia-vfs-dump` 工具，可作为解包对照。它不是给 `xenia_canary.exe` 随意追加参数就能启用的同名功能。

### XenonRecomp 是否更合适

核对版本：`ddd128bcca99fe8bfbb99bea583c972351fa6ace`。

`Recompiler::LoadConfig` 读取配置中的文件后，进入 `Image::ParseImage`；后者只识别 ELF 和 `XEX2`，没有 `LIVE`／STFS 分支。项目 README 也明确说明不提供运行时。

| 选择 | 本包直接输入 | 解出的 XEX | 后续原生运行 |
| --- | --- | --- | --- |
| 本地 ReXGlue | 已通过新增 `extract` 命令完整解出本包 | 本游戏加载、代码生成和 ARM64 构建已通过 | 已有运行时基础，仍需游戏适配 |
| XenonRecomp | 不支持 | 已有解析和转换流程，需对本游戏实际验证 | 需要另行提供运行时 |
| Xenia Canary | 有内容包加载路径 | 有 XEX 执行路径 | 模拟器执行，可作行为参考 |

主线选 ReXGlue；只有遇到具体函数分析或指令转换问题时，再用 XenonRecomp 对同一份 XEX 做局部对照。

## 二、实施步骤

### 第一步：准备固定版本的 SDK

- [x] 使用上述 ReXGlue 源码版本建立可构建工作目录，补齐对应版本的 Git 子模块。本地 ZIP 中如 `thirdparty/fmt` 是空目录，不能把源码 ZIP 当作完整依赖包。
- [x] 根据实际主机选择 SDK 已有构建预设；源码包含 `mac-arm64` 和 `mac-amd64`。已使用 `mac-arm64` 构建 CLI 并确认工具可启动。
- [x] 保留原始 LIVE 包，建立独立的 `guardian_heroes` 工程和 `assets` 输出目录。

交付结果：可用的 `rexglue` CLI 与固定源码版本。无需为格式解析安装或运行本地 Windows 版 Xenia。

### 第二步：复用容器实现，补最小解包命令

已增加命令：`rexglue extract <package> <output-directory>`，并在本地 CLI 构建中使用。

- [x] 接入最小 `extract` 命令并完成 CLI 构建。
- [x] 完整解出游戏包；414 个文件、8 个目录、961,292,510 字节与包内清单逐项一致。

改动范围限于：

| 文件（相对 SDK 根目录） | 修改内容 |
| --- | --- |
| `src/rexglue/commands/extract_command.h`，新增 | 按现有命令惯例声明注册函数 |
| `src/rexglue/commands/extract_command.cpp`，新增 | 创建 STFS 设备、遍历目录、导出文件和返回错误 |
| `src/rexglue/main.cpp` | 注册 `extract` |
| `src/rexglue/CMakeLists.txt` | 加入命令源文件 |

处理顺序：

1. 通过 `StfsContainerDevice` 初始化原始包；失败时明确返回包读取失败。
2. 从设备根目录遍历所有目录项，保留原有层级、文件名和大小写。
3. 通过既有 `Entry::Open`／`File::ReadSync` 分块复制文件，使用固定大小缓冲区，例如 1 MiB。
4. 同时判断读取状态和实际读取字节数，再判断宿主写入状态。短读或短写立即失败并报告具体文件。
5. 关闭文件并释放对象；输出目录必须为空或尚不存在，避免覆盖已有资源或混入旧文件。
6. 输出文件数和总字节数；失败的导出不得报告成功。

旧 `vfs_dump.cpp` 只作为遍历示例：它会跳过部分打开失败，未完整处理读写结果，并按 512 MiB 向上扩张临时缓冲区。不直接启用整个旧文件，也不改动代码生成器。

当前 `StfsContainerFile::ReadSync` 在底层 `fread` 短读后仍可能返回成功，所以导出层必须核对 `bytes_read`。沿用 STFS 自身块链元数据，不另外增加内容哈希或缓存机制。

交付结果：完整 `assets/default.xex` 和资源目录。只抽取主程序可以启动静态分析，但要运行游戏必须保留其他资源。

### 第三步：进入现有重编译流程

- [x] 核对解出的主程序字节数为 3,178,496，文件头为 `XEX2`。
- [x] 枚举完整解出目录中的 XEX／DLL，已确认仅有根目录 `default.xex`。
- [x] 使用现有 `init` 建立项目，指定 `assets` 为 `game_root`，主程序为 `assets/default.xex`。
- [x] 使用现有 `codegen` 生成 C++，完成 XEX 加载、入口识别、导入映射和函数生成。

未发现需要独立生成的动态模块，当前工程无需添加 `init module`／manifest 模块配置。

以当前工作目录为起点，解包完成后的现有命令示例：

```sh
rexglue init \
  --project-name guardian_heroes \
  --project-root './guardian_heroes' \
  --game-root './guardian_heroes/assets' \
  --xex-path './guardian_heroes/assets/default.xex'

rexglue codegen './guardian_heroes/guardian_heroes_manifest.toml'
```

若 XEX 解密／解压失败，排查 XEX 加载阶段；若函数发现或间接跳转解析失败，处理具体地址和配置。不能把这些后续问题继续归因于外层 LIVE 容器，也不使用强制忽略错误作为通过标准。

本包 `0x826C6784` 的间接跳转没有局部索引上界，自动扫描会读入相邻表。在 `guardian_heroes/guardian_heroes_manifest.toml` 添加精确的 `[[entrypoint.switch_tables]]` 配置，使用 `r11` 原始索引，并保留 `0x823A2628` 到下一张已引用表 `0x823A2638` 之间的 16 字节对应目标，包括第 4 项 `0x826C6890`。这表示已确认的存储范围，不推断游戏状态的逻辑取值范围。原始字节、相邻表引用和寄存器语义记录见 `logs/switch-table-analysis.md`。配置后重新 codegen 已通过。

交付结果：可追溯到该游戏版本的生成代码、manifest 和具体阻塞项。到这里才算完成“取出游戏主程序并启动静态重编译”的第一阶段目标。

### 第四步：按最终用途推进

已完成生成工程的 macOS ARM64 Release 编译、链接和既有动态库暂存步骤。为接通现有 SDK 源码构建，修正了公开头文件的 ImGui 包含目录、USB 摄像头导出源文件列表，以及 macOS Vulkan ICD 路径。产物和复现命令见 `guardian_heroes/README.md`。

若用途是代码研究：围绕入口、导入调用、游戏循环、资源加载和战斗逻辑整理函数；生成 C++ 保留其作为指令转换结果的定位。

若用途是原生运行：先构建生成工程，然后按“进入入口 → 标题画面 → 菜单和输入 → 一场实际战斗 → 声音／动画 → 存档读写”逐步验证。针对真实触发的运行时缺口处理，资源格式和着色器问题在对应阶段分析。

包内已经看见 WMV、着色器和多种资源归档，因此 CPU 代码生成成功不能单独证明视频、画面或声音正常。Xenia 可作为同版本游戏的运行行为参考。

## 三、若坚持直接将 LIVE 包交给 codegen

这是另一条可行但范围更大的路线，需要协调：

- `init` 的路径验证和 manifest 表达：区分宿主容器路径与包内入口路径。
- `ProjectRecompiler::Run`：取消该分支对宿主目录内真实 XEX 文件的假设。
- `Runtime::SetupVfs`：允许 STFS 设备作为游戏根挂载，入口仍为 `game:\default.xex`。
- 模块发现和现有输入依赖记录：从容器解析包内路径，保持变更后能够重新生成。
- 成品程序的资源挂载：运行时也必须能够访问容器，不能只让工具阶段支持。

本次目标用一次解包即可达成，推荐先完成第二、三步，再决定是否确实需要“免解包直接输入”。

## 四、实施时的最小验证

| 验证 | 能发现的具体失败 | 失败后的处理 |
| --- | --- | --- |
| SDK CLI 构建 | 依赖缺失、命令注册或链接错误 | 补齐固定版本依赖，修正接入 |
| 本包完整解出 | 文件块链、目录层级、短读／短写、遗漏资源 | 定位具体目录项和块，不继续生成 |
| 解出主程序加载 | 错误提取、XEX 格式／解压问题 | 按导出层或 XEX 加载层分别修复 |
| 一次 codegen 与生成工程构建 | 函数发现、指令支持、生成代码或导入链接问题 | 处理具体地址、符号及编译错误 |
| 若要求可玩，再验证一场战斗及存档 | 图形、输入、音频、行为和持久化问题 | 只处理实际触发的运行时路径 |

完整解包时按目录表核对文件大小和总数，不增加全资源哈希。额外对照解包仅在出现差异时使用 `xenia-vfs-dump`。实施时先按用户要求说明所需测试的目的、范围和预计耗时，经确认后运行。

## 五、限制

已完成 SDK CLI 构建、完整解包、XEX 加载、代码生成及 macOS ARM64 Release 编译、链接：414 个文件、8 个目录、961,292,510 字节与包内目录清单的数量和大小逐项一致；生成 181 个文件，含 14,824 个函数及 237 个导入跳板。生成 C++ 未含未实现 opcode 调用。游戏启动与玩法尚未验证，GPU 插件和现有 SDK stub 等运行适配边界见 `guardian_heroes/README.md`。参考的是指定提交的公开 Xenia Canary 源码，尚未确认本地 `xenia_canary.exe` 对应的源码版本。

## 参考源码

- [ReXGlue：本地 ZIP 对应版本的 STFS 实现](https://github.com/rexglue/rexglue-sdk/blob/c94f5ebdcb3c9d1a460ca48e04f9758448f8d518/src/filesystem/devices/stfs_container_device.cpp)
- [ReXGlue：项目重编译入口](https://github.com/rexglue/rexglue-sdk/blob/c94f5ebdcb3c9d1a460ca48e04f9758448f8d518/src/codegen/project_recompiler.cpp)
- [ReXGlue：尚未接入的 vfs_dump](https://github.com/rexglue/rexglue-sdk/blob/c94f5ebdcb3c9d1a460ca48e04f9758448f8d518/src/filesystem/vfs_dump.cpp)
- [Xenia Canary：文件识别、挂载与启动](https://github.com/xenia-canary/xenia-canary/blob/40cee11ab14cb9ad4286232dabfe3c69fc32b806/src/xenia/emulator.cc)
- [Xenia Canary：STFS 读取器](https://github.com/xenia-canary/xenia-canary/blob/40cee11ab14cb9ad4286232dabfe3c69fc32b806/src/xenia/vfs/devices/xcontent_devices/stfs_container_device.cc)
- [Xenia Canary：独立导出工具](https://github.com/xenia-canary/xenia-canary/blob/40cee11ab14cb9ad4286232dabfe3c69fc32b806/src/xenia/vfs/vfs_dump.cc)
- [XenonRecomp：输入魔数解析](https://github.com/hedge-dev/XenonRecomp/blob/ddd128bcca99fe8bfbb99bea583c972351fa6ace/XenonUtils/image.cpp)
- [XenonRecomp：用途与运行时边界](https://github.com/hedge-dev/XenonRecomp/blob/ddd128bcca99fe8bfbb99bea583c972351fa6ace/README.md)
