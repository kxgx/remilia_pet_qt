# 蕾米埃尔桌宠 (Remilia Pet)

基于 Qt6 的桌面宠物程序，支持 GIF 动画播放、抽卡、随机画画、闹钟计时等功能。

- **原作者**：[b站诉说新语](https://www.bilibili.com/video/BV1M13h6AEHr)
- **移植优化**：[b站星光-k](https://www.bilibili.com/video/BV1T5uF6KEas)（使用 AI 工具）

## GitHub Actions 自动构建

本仓库的 CI 工作流自动编译 **动态链接版本**，覆盖以下平台：

| 平台 | 架构 | Runner | Qt 来源 |
|------|------|--------|---------|
| Windows | x64 | windows-2022 | aqtinstall |
| macOS | x64 (Intel) | macos-26-intel | Homebrew |
| macOS | ARM64 (Apple Silicon) | macos-latest | Homebrew |
| Linux | x86_64 | ubuntu-latest | apt |
| Linux | ARM64 | ubuntu-24.04-arm | apt |

> **已知限制**：
> - **Windows ARM64**：aqtinstall 目前不提供 ARM64 架构的 Qt 预编译包，静态编译也因 vcpkg 的 `qtdeclarative` 依赖问题受阻

### 发布正式版

推送 `v` 开头的标签（如 `v1.0.0`）自动触发全平台构建并创建 GitHub Release。也可在 Actions 页面手动触发。

> 正式发布时手动上传 Windows x64 静态编译版本（便携版 + 安装器）到 Release。

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

## macOS 安装说明

DMG 安装包使用 ad-hoc 自签名，未经过 Apple 公证。

### 首次打开时的报错

macOS Gatekeeper 会拦截未签名应用，提示：

> **"蕾米埃尔桌宠_Qt6" 已损坏，无法打开。 你应该将它移到废纸篓。**
>
> 或
>
> **无法验证开发者。"蕾米埃尔桌宠_Qt6" 来自身份不明的开发者。**

### 解决方法

**方法一：右键打开（推荐）**
1. 在 Finder 中找到 `蕾米埃尔桌宠_Qt6.app`
2. **右键点击** → 选择 **"打开"**
3. 在弹出的对话框中点击 **"打开"** 确认
4. 之后双击即可正常启动

**方法二：终端解除隔离**
```bash
# 拖入 .app 后执行
xattr -cr /Applications/蕾米埃尔桌宠_Qt6.app
```

### 为什么会这样？

| 项 | 说明 |
|------|------|
| **签名方式** | ad-hoc 自签名（无证书） |
| **公证状态** | 未公证 |
| **影响** | Gatekeeper 拦截，需手动允许 |
| **正式分发** | 需 Apple Developer Program（$99/年）+ Developer ID 证书 + 公证 |
