@echo off
setlocal enabledelayedexpansion

echo ============================================
echo  RemiliaPet Windows Build VM - 自动配置
echo ============================================

:: ==========================================
:: 1. 安装 Chocolatey
:: ==========================================
echo [1/6] 安装 Chocolatey...
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))"

:: ==========================================
:: 2. 安装 Visual Studio 2022 Build Tools
:: ==========================================
echo [2/6] 安装 VS2022 Build Tools...
choco install visualstudio2022buildtools -y --package-parameters ^
  "--add Microsoft.VisualStudio.Workload.VCTools ^
   --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 ^
   --add Microsoft.VisualStudio.Component.Windows11SDK.22621 ^
   --includeRecommended --passive --wait"

:: ==========================================
:: 3. 安装 Git
:: ==========================================
echo [3/6] 安装 Git...
choco install git -y

:: ==========================================
:: 4. 安装 vcpkg
:: ==========================================
echo [4/6] 安装 vcpkg...
cd /d E:\
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat
vcpkg integrate install

:: 预装 Qt6 静态库（可选，首次构建时 vcpkg 会自动装）
echo [4/6] 预装 Qt6 静态依赖（需要较长时间）...
vcpkg install qt6[core,multimedia,gui,widgets]:x64-windows-static-md

:: ==========================================
:: 5. 安装 GitHub Actions Runner
:: ==========================================
echo [5/6] 安装 GitHub Actions Runner...
mkdir C:\actions-runner
cd C:\actions-runner
powershell -Command "Invoke-WebRequest -Uri https://github.com/actions/runner/releases/latest/download/actions-runner-win-x64.zip -OutFile runner.zip"
powershell -Command "Expand-Archive runner.zip -DestinationPath ."
del runner.zip

:: ==========================================
:: 6. 配置自启动
:: ==========================================
echo [6/6] 配置 Runner 自启动...
schtasks /create /tn "GitHubRunner" /tr "C:\actions-runner\run.cmd" /sc onstart /ru SYSTEM /f

echo ============================================
echo  配置完成！请手动注册 Runner:
echo  C:\actions-runner\config.cmd --url https://github.com/kxgx/remilia_pet_qt --token YOUR_TOKEN --name nas-win-x64 --labels Windows,X64,nas
echo ============================================
