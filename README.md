# JTDX-F

**JTDX-F V3.3.0** 是 Fluxmancer 基于 [JTDX](https://github.com/jtdx-project/jtdx) **2.2.159** 维护的独立分支，用于 FT8 / FT4 / JT65 / JT9 / T10 / WSPR 等业余无线电数字模式通联。

> 本项目是社区独立分支，**未获原 JTDX 作者认可或背书**。原版作者署名、版权声明和 GNU GPLv3 许可均完整保留。

JTDX-F 使用独立的应用名称和设置目录，可以和原版 JTDX 及其他分支同时安装，不会覆盖它们的设置和通联日志。

---

## 目录

- [相对原版新增了什么](#相对原版新增了什么)
- [相对原版修复了什么](#相对原版修复了什么)
- [行为与默认值的变化](#行为与默认值的变化)
- [如何查看与原版的差异](#如何查看与原版的差异)
- [仓库结构](#仓库结构)
- [构建](#构建)
- [测试](#测试)
- [文档索引](#文档索引)
- [已知限制](#已知限制)
- [版本号规则](#版本号规则)
- [许可与致谢](#许可与致谢)

---

## 相对原版新增了什么

下表是 JTDX-F 在 JTDX 2.2.159 之上**新增**的功能。列出的源文件都是原版中没有的新文件（另有约 90 个原版文件被修改，见[差异查看](#如何查看与原版的差异)）。

| 新增功能 | 说明 | 主要新文件 |
|---|---|---|
| **托管模式** | 主菜单新增“托管模式”：DXCC猎人 / QSO爱好者 / 关闭。按日志历史自动挑选新分区、新 DXCC、新网格、新呼号等目标，来电优先，含呼叫次数上限、失败忽略计时和每 15 分钟三轮 CQ。仍需人工启用发射。 | `ManagedMode.*`、`ManagedSettings.*` |
| **MCP 服务** | 本机 [Model Context Protocol](https://modelcontextprotocol.io/) 服务（Streamable HTTP，默认 `127.0.0.1:22380/mcp`），令牌认证，提供 21 个工具和 5 个资源，可读取状态/解码/日志，并在授权后控制模式、频率、目标和自动程序。默认关闭；不能代替人工启用发射。附带 stdio 桥接脚本。 | `McpServer.*`、`mainwindow_mcp.cpp`、`tools/jtdx_mcp_bridge.py` |
| **NTP 校时** | “设置 → 常规”中可选开启。内置 50 个按地区排列（亚洲在前）的 NTP 服务器，也可自定义；间隔 1 分钟～12 小时，默认 15 分钟。只调整系统 UTC 时钟、不改时区；发射/PTT 期间暂缓。通过独立的提权辅助程序 `jtdx-f-clock` 完成（Windows 用 UAC，Linux 用 polkit）。 | `NtpClient.*`、`NtpController.*`、`NtpSettings.*`、`NtpClockHelper.cpp`、`NtpInterval.hpp` |
| **多呼号资料管理** | “设置 → 常规 → 电台资料”可新增、修改、删除多个呼号，每个呼号单独保存网格、IARU 分区等设置，并使用独立的日志目录（如 `N0CALL_QRP/`）。发射会话期间锁定，避免资料与日志混用。 | `StationProfiles.*` |
| **按季度分文件的通联日志** | 通联按 UTC 日期分季度保存（如 `N0CALL2026S3.adi`），查重读取该呼号的全部历史。把 `.adi` / `.adif` / 旧 `wsjtx.log` 拖进主窗口即可在后台导入、去重，原件备份到 `old/`。 | `OperatorLog.*` |
| **后台处理提示窗** | 切换呼号、导入旧日志时显示“系统处理中，请稍候…”，处理在后台线程进行，发射中仍保留“停止发射”按钮。 | `ProcessingDialog.*` |
| **SuperFox 接收（SuperHound 模式）** | “DX 远征 → SuperFox 解码”可接收 SuperFox 信号，使用普通 FT8 回复；频率容差 5～200 Hz 可调。只接收，不发送 SuperFox 波形，也不做 OTP 在线身份认证（签名一律视为未验证）。 | `lib/superfox/`（来源见 `PROVENANCE.txt`） |
| **发射监管计时器** | 0～999 分钟（0 = 不限时），从人工启用发射开始按单调时钟累计，跨多个 QSO，不会被鼠标、解码或记日志重置。 | `TxWatchdog.hpp` |
| **连续 FT8 通联** | 关闭“单次通联”后，完成一次 QSO 会自动应答下一个符合筛选条件的呼叫，无候选才 CQ；完成消息不会被下一位目标打断。 | 修改 `mainwindow.cpp` |
| **解码进程故障恢复** | 解码子进程崩溃时不再让主程序退出：停止发射、保留界面、最多自动重启 3 次，错误写入 `decoder-errors.log`。 | 修改 `mainwindow.cpp` |
| **新品牌与安装器** | 新的猫巫师 Logo（程序图标、安装器、“关于”窗口）；NSIS Windows 安装器支持升级、降级确认（默认“否”）、卸载保留用户数据。 | `artwork/fluxmancer/`、`scripts/windows-installer.nsi` |
| **Windows UCRT 交叉编译工具链** | 在 Linux 上用 MinGW-w64 UCRT 交叉编译 Windows x64，依赖清单带 SHA-256，打包时递归检查 PE 导入表。 | `scripts/`、`dependencies/`、`windows/omnirig/` |
| **回归测试套件** | 原版无自动测试。新增真实主界面回归、MCP HTTP/stdio、NTP、托管模式、解码缓冲区、WSPR 译码器内存、随机种子等测试，可通过 `ctest` 运行。 | `tests/`、`lib/decode_buffers.f90`、`DebugFile.hpp` |
| **中文文档与翻译补充** | 新增 7 篇中文文档；为 MCP 页、监管状态、托管模式、关于信息和解码错误提示补充简体中文翻译。 | `docs/`、`translations/jtdx_zh_CN.ts` |

### 安全设计原则（贯穿所有新增功能）

- **只有人工点击“启用发射”才能开始发射会话。** 自动应答、UDP、MCP、设置恢复、定时器都不能开启已停止的发射；点击“停止”立即撤销授权并释放 PTT。
- 每次启动时发射停止、托管模式关闭、MCP 服务默认关闭、NTP 校时默认关闭。
- 不会在未完成交换时伪造“已完成通联”或日志记录。

---

## 相对原版修复了什么

以下是经源码审查确认并修复的原版缺陷（完整列表和验证方式见 [审查记录](jtdx-master/docs/审查记录_zh.md)）：

| 范围 | 原版问题 | 处理 |
|---|---|---|
| 解码交接 | 连续解码时锁文件存在竞争，界面可能一直停在“解码中” | 子进程先建立空闲锁再通知完成 |
| 启动随机种子 | `init_random_seed` 读取 EOF/错误不终止导致死循环，短读按整数而非字节移动可能越界 | Windows 改用 `rand_s`，POSIX 按字节累计并处理错误 |
| FT8 解码缓冲 | 200 条消息等数组无容量保护，统计写入存在并发竞争 | 有界写入 + OpenMP 同步 |
| WAV 读取 | 无效格式可除以零；补零偏移可能越界 | 统一有界 `QFile` 读取，支持中文路径 |
| HRD 网络接口 | 短回复按完整结构读取，magic 校验不完整 | 分段读取、长度和双 magic 校验、5 秒超时 |
| CAT 重新连接 | 重连时无条件开启监听，覆盖用户的 Stop，打断 WAV 解码 | 保留停止状态 |
| ADIF 解析 | 多行备注、备注内标签、小写呼号、仅有 FREQ 的旧记录处理不可靠 | 按字段长度解析，规范化呼号/频段 |
| 发射监管 | 计时被自动通联等路径反复重置，上限不能可靠生效 | 独立单调时钟会话 |
| 调试日志 | 文件打开失败仍向空 `FILE*` 写入，可能崩溃 | 可空的输出函数 |
| Reed-Solomon 初始化 | 非法位移、分配失败后解引用（三份实现） | 参数限制与分配检查 |
| WSPR | 堆栈译码器内存泄漏与越界风险；哈希文件无界扫描 | 有界数组与索引检查，ASan/UBSan 回归 |
| FT8 呼号数据库 | 文件缺失导致 Fortran 运行时终止 | 禁用辅助校验，保留常规解码 |
| 其他 | 迭代器失效（UDP 守护示例）、负数左移、通联历史字段未初始化、跨午夜计算、消息模型重复分配等 | 逐项修正 |

---

## 行为与默认值的变化

- 新配置默认启用：解码周期间隔行、**Tx/Rx 异频**、自动记录通联日志。
- 移除“73 后用 CW 发呼号”和“发射 CW 信息时间间隔”，旧配置中的这两项不再生效。
- “显示通知 → 曾经通联 → 隐藏不显示”默认关闭；开启后仍保留当前 QSO 和发给自己的消息。
- 自动程序菜单精简，移除强制先 CQ 的条件；合规的 73 / RR73 / RRR 也可作为新通联候选。
- 设置与数据目录独立于原版 JTDX。

---

## 如何查看与原版的差异

本仓库直接建立在上游 JTDX 2.2.159 的最后一个提交 `2a0e2be`（[jtdx-project/jtdx](https://github.com/jtdx-project/jtdx) `master`）之上，保留完整的上游历史。JTDX-F 的提交把源码移入 `jtdx-master/`（构建脚本依赖这个目录结构），并加入 `scripts/`、`dependencies/`。git 会把这一移动识别为重命名，因此可以直接对比：

```bash
git clone https://github.com/Northeast-Monsoon-Project/JTDX-F.git
cd JTDX-F
BASE=2a0e2be   # 上游 JTDX 2.2.159

git diff -M --stat $BASE HEAD                          # 改动概览
git diff -M $BASE HEAD -- mainwindow.cpp jtdx-master/mainwindow.cpp   # 单个文件
git diff -M --diff-filter=A --name-only $BASE HEAD     # 只看新增文件
```

相对原版：源码部分共 196 个文件变化（含新增文件），约 +16,700 / −3,200 行。

---

## 仓库结构

```
JTDX-F/
├── jtdx-master/            程序源码（C / C++11 / Fortran，CMake）
│   ├── docs/               中文文档（使用、构建、MCP、托管模式、NTP、审查记录、第三方组件）
│   ├── tests/              JTDX-F 新增的回归测试
│   ├── tools/              MCP stdio 桥接脚本
│   ├── lib/superfox/       SuperFox 接收代码及来源记录
│   ├── windows/omnirig/    预生成的 OmniRig ActiveQt 包装
│   └── artwork/fluxmancer/ JTDX-F Logo
├── scripts/                构建、打包、安装器脚本（Linux 原生 / Windows 交叉编译）
├── dependencies/           Windows UCRT64 依赖清单（含版本与 SHA-256）
├── README.txt              源码包原始说明
└── SOURCE_COMMIT.txt       源码包对应的内部工作区提交
```

---

## 构建

完整说明见 [构建说明](jtdx-master/docs/构建说明_zh.md)。

### 依赖

- CMake、Ninja、GCC / G++ / GFortran（验证环境为 16.x）
- Qt 5：Core、Gui、Widgets、Network、Multimedia、SerialPort、WebSockets、LinguistTools（测试需 Test，Windows 另需 ActiveQt）
- FFTW 3（单精度及线程库）、Boost 头文件、Hamlib（验证版本 4.7.2）、libusb

### Linux

```bash
source scripts/env.sh        # 使用工作区 .deps/ 中的依赖时；系统已装依赖可跳过
cmake -S jtdx-master -B build -G Ninja \
  -DCMAKE_Fortran_COMPILER="$PWD/scripts/gfortran-local" \
  -DCMAKE_BUILD_TYPE=Release \
  -DWSJT_GENERATE_DOCS=OFF -DWSJT_SKIP_MANPAGES=ON \
  -DCMAKE_INSTALL_PREFIX="$PWD/install"
cmake --build build -j6
cmake --install build
scripts/run-linux.sh         # 运行本机构建
```

若使用系统自带的 gfortran，可去掉 `-DCMAKE_Fortran_COMPILER` 一行。

### Windows x64（在 Linux 上交叉编译）

```bash
python3 scripts/download_windows_ucrt_deps.py   # 按 dependencies/manifest-ucrt64.json 下载并校验依赖
scripts/build-windows.sh                        # 输出 build-windows-ucrt/、install-windows/
scripts/make-windows-installer.sh               # 生成 release/JTDX-F-V3.3.0-Windows-x64-Setup.exe
```

libusb 与 Hamlib 需从官方源码交叉构建，命令见构建说明。不要混用 MSYS2 的 `mingw64` 与 `ucrt64` 包。也可在原生 Windows 的 MSYS2 UCRT64 环境中构建。

---

## 测试

```bash
ctest --test-dir build --output-on-failure
```

| 测试 | 内容 |
|---|---|
| `fm_regression` | 真实主界面 + 虚拟电台（不真实发射）：监管计时、连续/单次通联、时隙、MCP 控制、损坏 WAV 与中文路径、HRD 报文、解码进程故障恢复、人工发射授权、呼号管理、季度导入、FT8/SuperFox 模拟音频解码 |
| `fm_mcp_http` | MCP HTTP 认证、权限、请求边界、stdio 桥接 |
| `fm_ntp_tests` | NTP 协议、服务器列表、UTC/时区、授权生命周期（使用假时钟，不修改宿主系统时间） |
| `fm_decoder_buffers` | 启用 Fortran bounds 检查的多线程解码缓冲区边界 |
| `fm_codec_memory` | WSPR 堆栈译码器边界（可配合 ASan / UBSan） |

可选：`tests/test_mcp_sdk.py` 用官方 `mcp` Python SDK 联调；`JTDXF_PRIVATE_LOG=/path/to/log.adi` 可用私有日志在临时目录测试导入（原件不变）。

---

## 文档索引

| 文档 | 内容 |
|---|---|
| [使用说明](jtdx-master/docs/使用说明_zh.md) | 安装、呼号与日志、发射、NTP、SuperFox、连续通联 |
| [托管模式](jtdx-master/docs/托管模式_zh.md) | 目标条件、候选排序、定时 CQ、呼叫次数与忽略规则 |
| [MCP 指南](jtdx-master/docs/MCP_zh.md) | 连接方式、全部工具与参数、权限、错误处理 |
| [NTP 校时](jtdx-master/docs/NTP校时_zh.md) | 50 个服务器及来源、UTC 处理、权限说明 |
| [构建说明](jtdx-master/docs/构建说明_zh.md) | 工具链、Linux / Windows 构建、验证 |
| [审查记录](jtdx-master/docs/审查记录_zh.md) | 各版本修复的缺陷与验证方式 |
| [第三方组件](jtdx-master/docs/第三方组件_zh.md) | 许可与依赖组件来源 |

---

## 已知限制

- 开发验证未进行真实射频发射，未在实际电台、Windows 声卡和 CAT/PTT 硬件上测试；Windows 回归在 Wine 11.20 中执行，不能替代真实驱动测试。
- SuperFox 只接收，不发送 SuperFox 波形，不做远征身份 OTP 在线认证。
- OmniRig 用户仍需自行在 Windows 安装注册 OmniRig；使用 Hamlib 则不需要。
- Linux 构建产物针对构建所在环境，不是跨发行版的通用安装包。
- Windows 安装包未做商业代码签名。

遇到问题请提供：版本、操作系统、模式、声卡型号、CAT/PTT 类型、是否使用 OmniRig/HRD/TCI、异常时刻、复现步骤，以及当前呼号日志目录中的 `decoder-errors.log`。

---

## 版本号规则

小改动第三位加 1；大改动第二位加 1、第三位归零。

| 版本 | 主要内容 |
|---|---|
| V3.3.0 | 托管模式；“已通联隐藏”默认值修正；解码交接竞争、随机种子死循环修复 |
| V3.2.0 | NTP 校时；后台日志处理与提示窗；安装器中文编码修复；移除定时 CW |
| V3.1.1 | 修正默认仍锁定 Tx=Rx 的问题 |
| V3.1.0 | 多呼号管理、季度日志与拖放导入、SuperFox 接收、人工发射授权、FT8 解码容量保护、新品牌图标、安装器升级/降级 |
| V3.0.0 | 发射监管计时器、连续通联、解码进程故障恢复、大量缺陷修复、Windows UCRT 构建 |

---

## 许可与致谢

JTDX-F 以 **GNU GPLv3** 发布，见 [`jtdx-master/COPYING`](jtdx-master/COPYING)。

- JTDX 由 JTDX 项目团队开发，基于 Joe Taylor K1JT 等人的 WSJT-X；原有 `AUTHORS`、`THANKS` 与各文件版权声明均保留。
- SuperFox 接收代码来自 [CE3TSK JTDX_CONTEST](https://github.com/ce3tsk/jtdx_contest)，基于 WSJT-X 的 GPL 接收代码与 Nico Palermo 的 GPL polar codec；未包含上游带额外限制的 `qpc_n127k50q128.c`，详见 `jtdx-master/lib/superfox/PROVENANCE.txt`。
- Qt、Hamlib、libusb、FFTW、Boost、GCC/MinGW-w64 运行库等第三方组件的许可与来源见 [第三方组件](jtdx-master/docs/第三方组件_zh.md)。
- JTDX-F 的修改由 Fluxmancer 完成，Logo 由项目团队提供。
