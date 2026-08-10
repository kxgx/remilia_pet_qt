# 蕾米埃尔桌宠 (Remilia Pet)

基于 Qt6 的桌面宠物程序，支持 GIF 动画播放、抽卡、随机画画、闹钟计时等功能。

- **原作者**：[b站诉说新语](https://www.bilibili.com/video/BV1M13h6AEHr)
- **移植优化**：[b站星光-k](https://www.bilibili.com/video/BV1T5uF6KEas)（使用 AI 工具）

## 资源文件替换

将自定义文件放入程序目录下的 `resources/` 子文件夹即可替换内置资源。右键菜单「📁 资源替换」可查看替换状态和管理文件。

### 目录结构

```
程序目录/
  蕾米埃尔桌宠_Qt6.exe
  resources/          ← 替换资源根目录（也支持中文名"资源"）
    gif/              ← GIF 动画替换
      idle.gif
      click.gif
      ...
    audio/            ← 音效替换 (.wav)
      start.wav
      ...
    cards/            ← 卡片替换
      card_1.png
      ...
    drawing/          ← 画作替换
      drawing_1.png
      author.png
```

### 替换规则

| 资源类别 | 数量 | 后缀 | 命名要求 |
|---------|------|------|---------|
| GIF 动画 | 6 | .gif | 文件名完全匹配 (idle/click/drag/sleep/draw/result) |
| 音效 | 7 | .wav | 文件名完全匹配 (start/draw/drawing/result/reset/alarm/clock) |
| 卡片 | 55 | .png | card_N (N=1~55) |
| 画作 | 15 | .png | drawing_N (N=1~15) |
| 作者图 | 1 | .png | author |

### 资源管理窗口

右键菜单 →「📁 资源替换」打开管理侧窗：
- **打开目录** — 在文件管理器中打开对应资源目录，方便放置文件
- **删除** — 删除替换文件，恢复为内置资源
- 状态显示：绿色「已替换」或灰色「默认」

### 中文目录支持

程序优先查找 `resources/`，若不存在则查找 `资源/`（中文目录名）。启动时自动创建缺失的子目录。

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
> - **Arch Linux**：不再打包 `.pkg.tar.zst`，使用便携版 `.tar.gz` 解压即用（见下方安装说明）

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
  -DCMAKE_TOOLCHAIN_FILE="E:/vcpkg/scripts/buildsystems/vcpkg.cmake"

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

> ⚠️ **macOS Sequoia (15.x) 及更新版本**：Apple 已禁用右键→打开绕过 Gatekeeper 的方式，请使用以下方法。

**方法一：系统设置放行（推荐，适用于 macOS Sequoia+）**
1. 双击 `.app` → 弹出警告 → 点击 **"完成"**
2. 打开 **系统设置 → 隐私与安全性**
3. 滚动到底部"安全性"区域，找到："已阻止「蕾米埃尔桌宠_Qt6」以保护 Mac"
4. 点击旁边的 **"仍要打开"** 按钮 → 输入密码确认
5. 之后双击即可正常启动

**方法二：终端解除隔离（所有版本通用）**
```
# 拖入 .app 后执行
xattr -d com.apple.quarantine /Applications/蕾米埃尔桌宠_Qt6.app
# 然后双击打开
```

**方法三：右键打开（仅限 macOS Sonoma 14.x 及更早版本）**
1. 在 Finder 中找到 `.app`
2. **右键点击** → 选择 **"打开"**
3. 在弹出的对话框中点击 **"打开"** 确认
4. 之后双击即可正常启动

## Linux 安装说明

提供三种原生包 + 便携版，覆盖所有主流发行版：

| 发行版 | 包格式 | 安装命令 |
|--------|--------|----------|
| Debian / Ubuntu | `.deb` | `sudo dpkg -i RemiliaPet_Qt6-linux-{x64,arm64}.deb` |
| Fedora / RHEL | `.rpm` | `sudo rpm -i RemiliaPet_Qt6-linux-{x64,arm64}.rpm` |
| Arch / Manjaro | `.tar.gz` 便携版 | 见下方说明 |
| 通用便携 | `.tar.gz` | `tar -xzf RemiliaPet_Qt6-linux-{x64,arm64}.tar.gz && ./AppRun` |

安装后：
- 命令行输入 `remilia-pet` 即可启动
- 系统应用菜单中会出现"蕾米埃尔桌宠"图标
- 开机自启已默认启用，取消：`sudo rm /etc/xdg/autostart/remilia-pet.desktop`

### 安装方式

#### Debian / Ubuntu（.deb）

```bash
# 安装
sudo dpkg -i RemiliaPet_Qt6-linux-x64.deb
# 如有缺失依赖，自动修复
sudo apt-get install -f
```

安装后立即启用开机自启，取消：`sudo rm /etc/xdg/autostart/remilia-pet.desktop`
卸载：`sudo dpkg --purge remilia-pet`

#### Fedora / RHEL（.rpm）

```bash
# 安装（推荐 dnf，自动解析依赖）
sudo dnf install RemiliaPet_Qt6-linux-x64.rpm
# 或用 rpm 原生命令
sudo rpm -i RemiliaPet_Qt6-linux-x64.rpm
```

卸载：`sudo dnf remove remilia-pet`

#### Arch / Manjaro

推荐使用**便携版（tar.gz）**，零依赖、解压即用：

```
# 下载并解压
wget https://github.com/kxgx/remilia_pet_qt/releases/latest/download/RemiliaPet_Qt6-linux-x64.tar.gz
tar -xzf RemiliaPet_Qt6-linux-x64.tar.gz
cd 解压目录

# 运行
./AppRun
```

> 系统已自带 Qt6 运行时库，便携版直接复用无需额外安装依赖。

#### 便携版（tar.gz）

```
tar -xzf RemiliaPet_Qt6-linux-x64.tar.gz
cd 解压目录 && ./AppRun
```

无需安装，适用任何 Linux 发行版。需要系统已安装 Qt6 运行时库。

### 系统要求

- **Debian/Ubuntu** ≥ 20.04（自动安装依赖）
- **Fedora** ≥ 36
- **Arch / Manjaro**：使用便携版 `.tar.gz` 解压即用（见上方说明）
- 其他发行版可用 `tar.gz` 便携版直接运行

> **音频**：使用 QSoundEffect + WAV（嵌入 QRC），无需 GStreamer 或额外解码器。
> **显示**：默认使用 XCB（X11），Wayland 桌面通过 XWayland 兼容层自动适配。
