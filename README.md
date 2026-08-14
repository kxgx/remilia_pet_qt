# 蕾米埃尔桌宠 (Remilia Pet)

## v1.0.9 功能摘要

本版围绕「资源管理」「即时生效」「键位显示」「工程质量」四大主题大幅完善：

### 资源管理
- 内嵌文件浏览器完善：导航历史（后退/前进/主页）、表头排序、状态栏、新建文件/文件夹、搜索过滤、多选批量删除、目录拖入、悬停高亮
- 资源动态增加：放入 `card_56.png`、`drawing_16.png`（连续编号）即可被抽到，资源替换窗口自动列出新行
- 资源替换窗口文字随缩放自适应

### 改完即生效
- 文件管理器（含外部文件管理器）增删改文件后，程序 300ms 内自动重载 GIF/音频/尺寸，无需重启
- 宠物换尺寸时所有打开的窗口自动贴靠；抽卡窗口与宠物等比联动

### 键位显示
- 新增键盘键位显示：宠物上方粉色圆框显示当前按键，2 秒自动隐藏；右键/托盘双开关，独立置顶模式
- macOS 需辅助功能（输入监控）授权；Linux 基于 X11（XWayland）

### 界面与体验
- 缩放公式统一：窗口与内容始终一致；缩放下限降至最小可见（100×72）
- 位置按屏幕比例保存，分辨率变化实时跟随；托盘「重置位置」
- 计时器结束文字随缩放自适应且最小可读

### 工程质量（deskflow 级 CI）
- 合并单一流水线：52 项约束检查 + DeepSec 漏洞审查 + clang-format 门禁，任一不过即终止编译
- PR/push master 全平台编译：Win x64/ARM64、macOS ×2、Linux ×2、15 发行版容器矩阵、FreeBSD
- Werror 编译、编译摘要（job summary）、发布附 sha256 校验和与变更列表

基于 Qt6 的桌面宠物程序，支持 GIF 动画播放、抽卡、随机画画、闹钟计时等功能。

- **原作者**：[b站诉说新语](https://www.bilibili.com/video/BV1M13h6AEHr)
- **移植优化**：[b站星光-k](https://www.bilibili.com/video/BV1T5uF6KEas)（使用 AI 工具）

## 资源文件替换

将自定义文件放入以下任一位置即可替换内置资源（按优先级查找）：
1. **用户数据目录** `resources/` — 推荐，各平台均保证可写
2. **程序根目录** `resources/` — 便携/静态版直接放旁边
3. **程序根目录** `资源/` — 中文目录名

右键菜单「📁 资源替换」可查看替换状态和管理文件。

### 目录结构

```
（以用户数据目录为例，Linux: ~/.local/share/RemiliaPet/）
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

右键菜单 -> 「📁 资源替换」打开管理侧窗：
- **替换** — 弹出文件选择框，选取外部文件直接替换对应资源
- **全部替换** — 选取一个目录，自动扫描同名文件批量替换
- **打开** — 弹出内嵌文件浏览器查看对应资源子目录
- **删除** — 删除替换文件，恢复为内置资源
- 状态显示：绿色「已替换」或灰色「默认」

### 内嵌文件浏览器

「打开」按钮不再依赖系统文件管理器，改为应用内嵌浏览器（InAppFileDialog）：
- **无外部依赖** — 纯 Qt (QFileSystemModel + QTreeView)，所有平台一致（无需 dbus/xdg-open/GLib）
- **导航** — 向上/后退/前进/主页按钮，浏览历史可回退
- **路径跳转** — 点击路径标签可直接输入目标路径（无效路径弹提示）
- **双击打开/进入** — 双击文件用系统默认应用打开，双击目录进入
- **排序** — 表头点击按名称/大小/类型/日期排序
- **状态栏** — 底部显示条目数与选中数
- **新建** — 工具栏/右键菜单新建文件或文件夹，创建后自动选中
- **搜索** — 顶部输入框按文件名实时过滤
- **拖拽导入** — 从外部拖拽文件/目录自动复制到当前目录（同名提示覆盖确认，悬停高亮目标行）
- **右键菜单** — 新建文件夹 / 新建文件 / 打开 / 删除 / 重命名 / 刷新
- **多选删除** — Ctrl/Shift 多选，批量删除带总数确认
- **即时生效** — 管理器内或外部文件管理器增删改文件后，程序立即重载对应 GIF/音频/尺寸（300ms 防抖），宠物换尺寸时所有打开窗口自动贴靠自适应，无需重启
- **快捷键** — F2 重命名、Delete 删除、F5 刷新、Enter 打开、Backspace/Alt+↑ 向上、Alt+←/→ 后退/前进、Esc 关闭
- **单实例** — 重复点击「打开」复用同一窗口，切换目录

### 新增卡片/画作资源

抽卡结果（`cards/card_N.png`）与随机画画（`drawing/drawing_N.png`）支持**动态增加**：
- 把 `card_56.png`、`card_57.png`…（或 `drawing_16.png`…）放进对应子目录，宠物马上能抽到——编号需**连续**（从内置数量 +1 开始）
- 资源替换窗口会自动列出新增编号的行，可继续替换/删除

### 键盘键位显示

右键菜单 / 托盘菜单 →「⌨ 键位显示」开关：
- 宠物上方弹出粉色圆框，显示当前按下的键（空格、回车、方向键等都会显示），2 秒无输入自动隐藏
- 右键菜单/托盘另有「⌨ 键位显示置顶」开关（独立于全局置顶）：全局置顶关闭时，开启它键位窗口仍单独保持置顶；全局置顶开启时所有窗口置顶，开关自然不冲突
- 状态自动保存，下次启动保持；随宠物缩放联动
- **macOS**：需在「系统设置 → 隐私与安全 → 辅助功能（输入监控）」中授权本应用，否则无显示
- **Linux**：基于 X11 轮询，XWayland 正常；纯 Wayland 会话无显示

### 查找优先级

三级备选查找：**用户数据目录** → **程序根目录** `resources/` → **程序根目录** `资源/`。找到第一个存在的即用，启动时自动创建缺失的子目录。

## GitHub Actions 自动构建

本仓库包含 3 个 CI 工作流：

| 工作流 | 触发条件 | 职责 |
|--------|---------|------|
| [build.yml](.github/workflows/build.yml) | push master / PR / tag `v*` | 52 项约束检查 + DeepSec 漏洞审查 + clang-format 门禁（不通过终止编译）→ 全平台编译矩阵 + GitHub Release（仅 tag） |
| [dev.yml](.github/workflows/dev.yml) | 手动触发 | Linux AppImage 开发测试（含 GStreamer） |
| [sync.yml](.github/workflows/sync.yml) | push master / 手动 | 多仓同步：GitHub → 极狐 GitLab |

> 原 `static.yml` 副 CI 已合并进 `build.yml`（`static`/`release-static` job）。

`build.yml` 自动编译覆盖以下平台：

| 平台 | 架构 | Runner | Qt 来源 | 产物大小 |
|------|------|--------|---------|----------|
| Windows | x64 | windows-2022 | aqtinstall | ZIP 约 41 MB |
| macOS | x64 (Intel) | macos-26-intel | Homebrew | DMG 约 57 MB |
| macOS | ARM64 (Apple Silicon) | macos-latest | Homebrew | DMG 约 54 MB |
| Linux | x86_64 | ubuntu-latest | apt | tar.gz 约 49 MB |
| Linux | ARM64 | ubuntu-24.04-arm | apt | tar.gz 约 49 MB |

> **已知限制**：
> - **Windows ARM64**：aqtinstall 暂无 ARM64 Qt 预编译包；vcpkg 静态编译因 `qtdeclarative` 依赖问题受阻
> - **Arch Linux**：使用便携版 `.tar.gz`，不打包 `.pkg.tar.zst`

### 发布正式版

推送 `v` 开头的标签（如 `v1.0.0`）自动触发全平台构建并创建 GitHub Release。也可在 Actions 页面手动触发。

> 静态版（便携版 + 安装器）由 CI 的 `static`/`release-static` job（NAS 自托管 runner）自动编译并发布，无需手动上传。

## 静态编译版本

> 由于 vcpkg 编译 Qt 静态库的中间产物体积超过 19 GB，超出 GitHub Actions 缓存上限（10 GB），静态链接版本无法使用 GitHub 托管 runner。CI 上通过自托管 runner（NAS Windows 虚拟机）完成静态构建（本地也可编译）。

### 编译环境要求

| 工具 | 版本/说明 |
|------|----------|
| Visual Studio | 2022（含 MSVC v14.44 工具集 + Windows SDK 10.0.26100） |
| CMake | 3.16+ |
| vcpkg | 最新版，安装 `qtbase[gui,widgets,jpeg,png,network]` |
| Inno Setup | 6.x（仅打包安装器时需要） |
| 磁盘空间 | 至少 30 GB（vcpkg 编译 Qt 约 19 GB + 构建约 2 GB） |

### 当前支持平台

| 平台 | 架构 | 状态 |
|------|------|------|
| Windows | x64 | ✅ 可编译（`x64-windows-static-md` triplet） |
| Windows | ARM64 | ❌ vcpkg 编译 `qtdeclarative` 等依赖存在问题 |

### 构建产物

| 产物 | 大小 | 说明 |
|------|------|------|
| 便携版 EXE | 约 39 MB | 单文件，无需 DLL，解压即用 |
| 便携版 ZIP | 约 55 MB | 压缩包，含 EXE |
| 安装器 Setup.exe | 约 27 MB | Inno Setup 打包，安装到 Program Files |

### 本地编译步骤

```powershell
# 1. 安装 vcpkg 并编译 Qt 静态库（首次约 1-2 小时）
vcpkg install qtbase[gui,widgets,jpeg,png,network] --triplet x64-windows-static-md

# 2. 配置 CMake
cmake -B build-static -S . -G "Visual Studio 17 2022" -T version=14.44 `
  -DVCPKG_TARGET_TRIPLET=x64-windows-static-md `
  -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake"

# 3. 编译
cmake --build build-static --config Release --parallel

# 4. （可选）打包安装器
& "C:\Program Files (x86)\Inno Setup 6\iscc.exe" /DBuildDir="build-static\Release" installer.iss
```

构建输出在 `build-static/Release/` 目录，Inno Setup 安装器输出在 `Output/`。

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

> **音频**：使用 miniaudio + WAV（嵌入 QRC），无需 GStreamer / FFmpeg / Qt Multimedia。
> **显示**：默认使用 XCB（X11），Wayland 桌面通过 XWayland 兼容层自动适配。
