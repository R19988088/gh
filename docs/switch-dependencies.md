# Guardian Heroes Switch 接入依赖

## 本次采用的来源

| 来源 | 固定版本 | 使用范围 |
| --- | --- | --- |
| [masseffect-nx](https://github.com/nebadasSwifty/masseffect-nx) 内的 SDK | `a85b0b419ec3e332165ae1617c980bd399856a44` | Horizon guest-memory core、NVK libelf 不支持路径的明确失败实现；保留 SDK BSD 声明 |
| [prototype-switch](https://github.com/fungalspores/prototype-switch) | `292de4b382dbe335011d8b4f56ec4212df5ef74c` | 工具链/静态 NVK/NRO 构建方式和补丁行为对照，未移入 Prototype 游戏代码、renderer 或地址补丁 |
| [mesa-switch](https://github.com/danfromtico/mesa-switch) | `d4a00ea0ab3f59afb967cc5d779e4263d237bd77`（供体 Mesa 文档），配参考项目 `patches/mesa-switch.patch` | 独立 NVK probe 的后续构建输入，本次未构建/安装 |
| devkitPro / devkitA64 / libnx | 项目内官方 `devkitpro/devkita64:latest` ARM64 payload：devkitA64 r29.2、libnx 4.12.0-1、switch-tools 1.13.1-1、deko3d 0.5.0 | 工具链、Horizon headers、libnx 与 nacptool/elf2nro；详情见 `toolchains/switch-local/README.md` |

前两项完整 commit 来自下载的 Git archive 元数据。参考源码只用于阅读/移入必要文件，不成为构建时 `/tmp` 路径依赖。

移入代码的许可文件保存在 `guardian_heroes/switch/licenses/`。Horizon SDK 的 BSD 声明与 Prototype 仓库根目录 GPL 不混淆；本批未引入 Prototype 的 read-only-as-RW 修补或私有 native renderer。

## 当前构建边界

Switch preset 在顶层进入独立 probe 构建分支，不构建桌面 SDL/动态 Vulkan loader，也没有可运行的完整 `guardian_heroes` Switch 目标。

- `guardian_heroes_switch_bootstrap_nro`：libnx console、启动模式、进程内存信息、SD 阶段日志。
- `guardian_heroes_switch_guest_abi_probe`：编译项目真实生成 PCH、guest ABI 布局与代表性大端/原子/SIMD/FPCR 宏，不链接或执行整个游戏。
- `guardian_heroes_switch_memory_probe_nro`：guest arena、真实别名与 host-memory adapter 的独立验证入口。
- `guardian_heroes_switch_vulkan_probe_nro`：静态 NVK 的 VI surface、交换链和颜色清屏；仅配置 NVK SDK 时加入。

探针是目标代码和验证入口，尚无本批 NRO 构建或真机运行证据。完整 runtime 的 Clang 限制保留，避免未经编译验证就扩大 GCC 支持声明。

内存核心要求 loader 传入真实进程 handle；缺失时明确拒绝，不自动添加 IPC 权限回退。当前 host-memory 后端支持 RW/NONE；RO、可执行映射、文件映射明确拒绝。alias 首次访问需显式映射，完整游戏尚需接入异常分派/写监控，并处理现有 Heap 零长度 release 调用和物理内存预提交策略。

## 构建入口

项目已在 `toolchains/switch-local/` 独立安装 CMake、devkitA64、libnx 和 Switch 工具。构建脚本自动加载其局部环境，不需要修改系统 PATH；默认只构建 bootstrap：

```sh
bash guardian_heroes/tools/build_switch.sh
bash guardian_heroes/tools/build_switch.sh guardian_heroes_switch_guest_abi_probe
bash guardian_heroes/tools/build_switch.sh guardian_heroes_switch_memory_probe_nro
```

NVK 使用 Horizon 专用静态库，不使用 macOS/Linux 桌面 Vulkan SDK：

```sh
export REXGLUE_SWITCH_NVK_SDK=/absolute/path/to/horizon/portlibs/switch
bash guardian_heroes/tools/build_switch.sh guardian_heroes_switch_vulkan_probe_nro
```

本批 probe 只把 `vkGetInstanceProcAddr` / `vkDestroyInstance` 作为静态入口，其余函数从实例/设备 proc table 取得。

NVK SDK 根需包含 `include/vulkan/vulkan.h`、`lib/libvulkan.a`；静态依赖 `expat`、`zstd`、`z` 可位于同根或 devkitPro Switch portlibs。所选 Mesa 实现须提供 `VK_NN_vi_surface`。Rust/NVK 的重复定义和额外依赖如在真实链接中出现，应按该版本的 archive 内容处理，不全局忽略链接错误。

保留 `out/build/switch-aarch64-release/*.elf` 和 `*.map` 用于定位，NRO 由 `nacptool` 和 `elf2nro` 打包。bootstrap 和 memory/ABI 不要求 NVK；同一个 build 目录若显式配置了损坏的 NVK SDK，先清除该环境变量再运行脚本。

## 目标机操作

通过 Application 模式 hbmenu 启动独立 NRO。NVK 和 guest-memory probe 不接受 Album/applet 模式作为完整环境，失败时应阅读日志而非修改游戏逻辑。

SD 日志位于 `sdmc:/switch/guardian-heroes/logs/`。Vulkan 探针通过周期性颜色变化辨认持续呈现，不显示游戏。无退出按键绑定；只在 Horizon 请求结束时退出。运行游戏后保持应用运行的项目约定同样适用。

验证前按项目约定确认目的、范围和耗时。下一道门槛是先取得 bootstrap/ABI 构建结果，再讨论完整游戏 runtime 移植。

## 本地验证记录（2026-10-22）

- `bash -n guardian_heroes/tools/build_switch.sh` 通过。
- preset JSON 解析通过，`/opt/homebrew/bin/cmake --list-presets=all -S guardian_heroes` 能识别 Switch configure/build preset。
- 当前命令环境的 PATH 未包含 CMake/Ninja；Homebrew CMake 可通过上述绝对路径调用。
- 默认 `/opt/devkitpro` 下缺少 devkitA64 编译器、libnx 和 NRO 打包工具。本轮止于依赖门槛，未运行目标配置、交叉编译或真机验证。

随后已将所需工具安装到项目内 `toolchains/switch/`，并接入构建脚本。上面的依赖缺失记录描述安装前状态；安装后的工具启动和交叉构建尚未验证。

### 安装后交叉构建

当前工程内工具链核验结果（2026-10-25）：`aarch64-none-elf-g++`、`libnx/switch.specs`、`libnx/lib/libnx.a`、`elf2nro`、`nacptool` 和 deko3d 均已存在。官方镜像中的这些可执行文件是 Linux ARM64 ELF，当前 macOS 主机只能完成路径和文件完整性核验；实际交叉编译需在 ARM64 Linux/container 中运行。

2026-10-22，使用项目内 devkitA64 r29.1 / GCC 15.2.0 与 libnx 4.12.0 完成 CMake 配置、bootstrap 和 memory probe 的编译、链接及 NACP/NRO 打包。两个产物的 NRO0 头与声明长度检查通过，发布包位于 `dist/switch-probes-2026-10-22.zip`。

本轮未构建 ABI probe 或 Vulkan probe，未安装 Mesa/NVK，未进行真机运行。两个已打包探针均不依赖 Mesa/NVK，不代表完整游戏运行通过。
