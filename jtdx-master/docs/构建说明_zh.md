# 构建 JTDX-F V3.3.0

## 源码基线和工具链

用户提供的 `jtdx-master.zip` 保留不变；其中 `Versions.cmake` 为 JTDX 2.2.159。本分支使用 C、C++11 和 Fortran，CMake 构建。

本次验证环境：Linux x86_64，GCC/G++/GFortran 16.2.1；Windows 交叉编译为 x86_64 MinGW-w64 GCC/GFortran 16.2.0、UCRT，配合 MSYS2 UCRT64 的 Qt 5.15.19、FFTW 3、Boost 和 Hamlib 4.7.2。完整包版本及 SHA-256 见发行源码附带的依赖清单。

Windows 运行库使用同一种 UCRT 体系，避免混用 MSVCRT 与 UCRT 的 C/C++ 运行库。未使用 `-march=native`，不要求构建机器相同的 CPU。优化器升级本身不保证解码速度提高；应使用相同音频、设置和硬件进行实测比较。

## 当前工作区构建

源码包保留 `jtdx-master/`、`scripts/` 两个同级目录。当前工作区下载的依赖在 `.deps/`，不更改系统编译器。

Linux：

```bash
source scripts/env.sh
cmake -S jtdx-master -B build -G Ninja \
  -DCMAKE_Fortran_COMPILER="$PWD/scripts/gfortran-local" \
  -DCMAKE_BUILD_TYPE=Release \
  -DWSJT_GENERATE_DOCS=OFF -DWSJT_SKIP_MANPAGES=ON \
  -DCMAKE_INSTALL_PREFIX="$PWD/install"
cmake --build build -j6
cmake --install build
ctest --test-dir build --output-on-failure
```

运行本机版本：`scripts/run-linux.sh`。这是针对当前 Linux 环境构建的程序，不是跨发行版的通用 Linux 安装包。

Windows：

```bash
scripts/build-windows.sh
```

脚本使用 `scripts/windows-toolchain.cmake`，输出在 `build-windows-ucrt/`，安装树在 `install-windows/`。`scripts/package_windows.py` 递归检查 PE 导入表、复制所需 DLL 和 Qt 插件，任何未解析的非系统 DLL 会使打包失败。`runtime-manifest.json` 记录所含 PE 文件的 SHA-256 和导入表。

生成安装程序：`scripts/make-windows-installer.sh`，采用 NSIS，默认输出 `release/JTDX-F-V3.3.0-Windows-x64-Setup.exe`。Wine 只在 Linux 构建安装器和兼容性测试时使用，Windows 用户不需要 Wine。所有 Wine 操作放在工作区的独立前缀和 Xvfb 显示中。

## 在其他环境准备依赖

常规原生构建需要 Qt 5 的 Core、Gui、Widgets、Network、Multimedia、SerialPort、WebSockets、LinguistTools；测试还需要 Test。Windows 另外需要 ActiveQt。还需要 GCC/GFortran、FFTW 的单精度及线程库、Boost 头文件、Hamlib、libusb、CMake 和 Ninja。可关闭 `BUILD_TESTING` 以跳过测试构建。

下载脚本 `scripts/download_windows_ucrt_deps.py` 优先读取源码包中的 `dependencies/manifest-ucrt64.json` 并验证 SHA-256；没有该清单时才读取已转换为 JSON 的 MSYS2 数据库 `.deps/msys2/db-ucrt64.json`。清单记录本次实际使用的版本。它不是保证永久可用的二进制镜像；包仓库更新后应使用相匹配的完整依赖集合。

libusb 1.0.30 和 Hamlib 4.7.2 使用官方源代码交叉构建，关键命令如下（从各自独立构建目录执行，`FM_PREFIX` 指向 UCRT64 前缀）：

```bash
../libusb-1.0.30/configure --host=x86_64-w64-mingw32 \
  --prefix="$FM_PREFIX" --disable-static
make -j6
make install

PKG_CONFIG_LIBDIR="$FM_PREFIX/lib/pkgconfig" \
../hamlib-4.7.2/configure --host=x86_64-w64-mingw32 \
  --prefix="$FM_PREFIX" --disable-static --without-cxx-binding \
  --without-readline --disable-winradio --without-lua-binding \
  --without-python-binding --without-tcl-binding --without-perl-binding
make -j6
make install
```

原生 Windows 可在 MSYS2 UCRT64 环境构建同一源码，并指定这些依赖的安装前缀。不要混用 `mingw64` 和 `ucrt64` 包。

`windows/omnirig` 提供生成的 ActiveQt 包装文件；`JTDX_OMNIRIG_GENERATED_DIR` 默认为该目录。它不包含 OmniRig 服务本身。若要使用本机注册的类型库重新生成，可将此 CMake 变量设为空；重新生成时需留意 Qt 5 `dumpcpp` 对失败的 COM 文档查询缺少检查的问题。

## 验证

`fm_regression` 使用真实主界面逻辑、临时配置和虚拟电台，不进行真实发射；包括监管边界、连续/单次通联、下一站时隙、MCP 控制、WAV 损坏与中文路径、HRD 分段/错误报文、解码进程故障恢复、人工发射授权、呼号管理、季度导入失败与重试，以及真实 FT8/SuperFox 模拟音频解码。

`fm_mcp_http` 使用标准库测试 HTTP、权限和 stdio 桥接；`tests/test_mcp_sdk.py` 可使用官方 `mcp` Python 包进一步验证。`fm_codec_memory` 检查 WSPR 堆栈译码器边界，另可用 AddressSanitizer 和 UndefinedBehaviorSanitizer 编译 `tests/CodecMemory.c` 运行。

`fm_decoder_buffers` 在启用 Fortran bounds 检查的情况下，以多线程验证解码缓冲区容量边界。SuperFox 接收代码及编码表来源见 `lib/superfox/PROVENANCE.txt`；模拟 WAV 是公开测试素材，用户日志不是测试包的一部分。可选的私有参考日志仅通过 `JTDXF_PRIVATE_LOG=/path/to/log.adi` 环境变量传入，测试在临时目录进行，原件保持不变。

Windows 回归在 Wine 11.20 中执行；真实 Windows 电台、声卡、驱动和长时间通联仍需目标硬件验收。这一限制不应被描述为所有 Windows 环境均已验证。


## V3.2.0 校时辅助程序

`jtdx-f-clock`（Windows 为 `.exe`）与主程序一起安装，仅负责 NTP 查询和设置系统 UTC 时钟。Windows 链接系统 Shell/权限 API；Linux 运行时需要 polkit 的 `pkexec`。主界面不需要整体以管理员身份运行。

`fm_ntp_tests` 验证协议、服务器列表、UTC/时区和授权生命周期。它只启动编译时启用假时钟的 `fm_ntp_helper`，不会设置宿主系统时间。两个测试程序均不安装到发行目录；生产辅助程序没有假时钟运行开关。

只读网络诊断：`jtdx-f-clock --query ntp1.aliyun.com`。这条命令仅打印 UTC、时间偏差及往返延迟，不申请权限、不校时。

安装脚本保存为 UTF-8 BOM，并在 `makensis` 命令中指定 `-INPUTCHARSET UTF8`，确保降级确认等中文字符串不会按系统 ANSI 代码页误读。`Unicode True` 仅控制输出程序，不能替代输入脚本编码声明。参考 [NSIS 命令行说明](https://nsis.sourceforge.io/Docs/Chapter3.html)。
