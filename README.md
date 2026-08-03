# 蕾米埃尔桌宠 (Remilia Pet)

基于 Qt6 的桌面宠物程序，支持 GIF 动画播放、抽卡、随机画画、闹钟计时等功能。

- **原作者**：[b站诉说新语](https://www.bilibili.com/video/BV1M13h6AEHr)
- **移植优化**：[b站星光-k](https://space.bilibili.com/32819169)（使用 AI 工具）

## GitHub Actions 自动构建

本仓库的 CI 工作流自动编译 **动态链接版本**，覆盖以下平台：

| 平台 | 架构 | Runner | Qt 来源 |
|------|------|--------|---------|
| Windows | x64 | windows-latest | aqtinstall |
| Windows | ARM64 | windows-11-arm | aqtinstall |
| macOS | x86_64 (Intel) | macos-13 | Homebrew |
| macOS | ARM64 (Apple Silicon) | macos-latest | Homebrew |
| Linux | x86_64 | ubuntu-latest | apt |
| Linux | ARM64 | ubuntu-24.04-arm | apt |

每次推送到 `main`/`master` 分支或发起 Pull Request 时自动触发构建，产物可在 Actions 页面下载。

## 静态编译版本

由于 vcpkg 静态编译 Qt 的首次编译时间过长（约 1-2 小时），且编译产物体积超过 19 GB，超出 GitHub Actions 缓存上限（10 GB），**静态链接版本仅支持本地编译**。

当前可本地编译的静态版本：

| 平台 | 架构 | 状态 |
|------|------|------|
| Windows | x64 | ✅ 可编译（`cmake -B build-static -DVCPKG_TARGET_TRIPLET=x64-windows-static -DCMAKE_TOOLCHAIN_FILE=...`） |
| Windows | ARM64 | ❌ vcpkg 编译 `qtdeclarative` 等依赖存在问题，暂不支持 |

静态版本编译出的单个 EXE 约 57 MB，无需任何 DLL 即可独立运行。

### 本地静态编译命令

```powershell
# 需要先安装 vcpkg
cmake -B build-static -S . -G "Visual Studio 17 2022" `
  -DVCPKG_TARGET_TRIPLET=x64-windows-static `
  -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake"

cmake --build build-static --config Release --parallel
```

## 运行环境

- **动态版本**（从 Actions 下载）：需要安装 [Visual C++ 可再发行组件](https://aka.ms/vs/17/release/vc_redist.x64.exe)
- **静态版本**（本地编译）：无需额外依赖，直接运行
