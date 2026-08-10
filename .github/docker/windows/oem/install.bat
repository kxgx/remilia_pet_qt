@echo off
setlocal enabledelayedexpansion

set LOG=C:\OEM\install.log
echo %date% %time% === RemiliaPet 自动配置开始 === > %LOG%

:: ==========================================
:: 1. 安装 Chocolatey
:: ==========================================
echo [1/7] 安装 Chocolatey...
echo %date% %time% [1/7] 安装 Chocolatey >> %LOG%
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "Set-ExecutionPolicy Bypass -Scope Process -Force; ^
   [System.Net.ServicePointManager]::SecurityProtocol = 3072; ^
   iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))" >> %LOG% 2>&1
if %ERRORLEVEL% NEQ 0 (
  echo %date% %time% [FAIL] Chocolatey 安装失败 >> %LOG%
  goto :done
)
echo %date% %time% [OK] Chocolatey >> %LOG%

:: 刷新 PATH（Chocolatey 安装后需要）
call refreshenv 2>nul
set "PATH=%ALLUSERSPROFILE%\chocolatey\bin;%PATH%"

:: ==========================================
:: 2. 安装 Visual Studio 2022 Build Tools
:: ==========================================
echo [2/7] 安装 VS2022 Build Tools（需要 15-30 分钟）...
echo %date% %time% [2/7] VS2022 Build Tools >> %LOG%
choco install visualstudio2022buildtools -y --package-parameters ^
  "--add Microsoft.VisualStudio.Workload.VCTools ^
   --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 ^
   --add Microsoft.VisualStudio.Component.Windows11SDK.22621 ^
   --includeRecommended --passive --wait" >> %LOG% 2>&1
if %ERRORLEVEL% NEQ 0 (
  echo %date% %time% [FAIL] VS2022 Build Tools >> %LOG%
  goto :done
)
echo %date% %time% [OK] VS2022 Build Tools >> %LOG%

:: ==========================================
:: 3. 安装 Git
:: ==========================================
echo [3/7] 安装 Git...
echo %date% %time% [3/7] Git >> %LOG%
choco install git -y >> %LOG% 2>&1
if %ERRORLEVEL% NEQ 0 (
  echo %date% %time% [FAIL] Git >> %LOG%
  goto :done
)
echo %date% %time% [OK] Git >> %LOG%
set "PATH=C:\Program Files\Git\bin;C:\Program Files\Git\usr\bin;%PATH%"

:: ==========================================
:: 4. 安装 vcpkg
:: ==========================================
echo [4/7] 安装 vcpkg...
echo %date% %time% [4/7] vcpkg clone >> %LOG%
cd /d C:\
if exist C:\vcpkg rmdir /s /q C:\vcpkg
git clone https://github.com/microsoft/vcpkg.git >> %LOG% 2>&1
if %ERRORLEVEL% NEQ 0 (
  echo %date% %time% [FAIL] vcpkg clone >> %LOG%
  goto :done
)
cd C:\vcpkg
call bootstrap-vcpkg.bat >> %LOG% 2>&1
vcpkg integrate install >> %LOG% 2>&1
echo %date% %time% [OK] vcpkg >> %LOG%

:: ==========================================
:: 5. 预装 Qt6 静态依赖
:: ==========================================
echo [5/7] 预装 Qt6 静态依赖（需要 30-60 分钟）...
echo %date% %time% [5/7] Qt6 static >> %LOG%
vcpkg install qt6[core,multimedia,gui,widgets]:x64-windows-static-md >> %LOG% 2>&1
if %ERRORLEVEL% NEQ 0 (
  echo %date% %time% [FAIL] Qt6 vcpkg >> %LOG%
  goto :done
)
echo %date% %time% [OK] Qt6 static >> %LOG%

:: ==========================================
:: 6. 安装 GitHub Actions Runner
:: ==========================================
echo [6/7] 安装 GitHub Actions Runner...
echo %date% %time% [6/7] Runner download >> %LOG%
if exist C:\actions-runner rmdir /s /q C:\actions-runner
mkdir C:\actions-runner
cd C:\actions-runner
powershell -Command ^
  "Invoke-WebRequest -Uri 'https://github.com/actions/runner/releases/latest/download/actions-runner-win-x64.zip' -OutFile runner.zip" >> %LOG% 2>&1
powershell -Command "Expand-Archive runner.zip -DestinationPath ." >> %LOG% 2>&1
del runner.zip
echo %date% %time% [OK] Runner files >> %LOG%

:: ==========================================
:: 7. 自动注册 GitHub Runner
:: ==========================================
echo [7/7] 注册 GitHub Runner...
echo %date% %time% [7/7] Runner register >> %LOG%

set "REGISTERED=0"

if exist C:\OEM\github-pat.txt (
  echo 发现 GitHub PAT，正在获取 Runner 注册令牌...
  echo %date% %time% 读取 github-pat.txt >> %LOG%

  :: 调用 GitHub API 获取 runner 注册令牌
  powershell -Command ^
    "$pat = (Get-Content C:\OEM\github-pat.txt -Raw).Trim(); ^
     $body = @{} | ConvertTo-Json; ^
     $resp = Invoke-RestMethod -Uri 'https://api.github.com/repos/kxgx/remilia_pet_qt/actions/runners/registration-token' ^
       -Method Post ^
       -Headers @{Authorization=\"Bearer $pat\"; Accept=\"application/vnd.github+json\"} ^
       -Body $body ^
       -ContentType 'application/json'; ^
     $resp.token | Out-File -Encoding ASCII C:\OEM\runner-token.txt" >> %LOG% 2>&1

  if exist C:\OEM\runner-token.txt (
    set /p RUNNER_TOKEN=<C:\OEM\runner-token.txt
    echo %date% %time% Token acquired >> %LOG%

    call config.cmd --url https://github.com/kxgx/remilia_pet_qt ^
      --token !RUNNER_TOKEN! ^
      --name nas-win-x64 ^
      --labels Windows,X64,nas ^
      --unattended ^
      --runasservice >> %LOG% 2>&1

    if !ERRORLEVEL! EQU 0 (
      set "REGISTERED=1"
      echo %date% %time% [OK] Runner 注册成功，服务已启动 >> %LOG%
    ) else (
      echo %date% %time% [FAIL] Runner 注册失败 >> %LOG%
    )
  ) else (
    echo %date% %time% [FAIL] 未获取到注册令牌 >> %LOG%
  )
) else (
  echo %date% %time% 未找到 github-pat.txt，跳过自动注册 >> %LOG%
)

:done
echo %date% %time% === 配置结束，registered=!REGISTERED! === >> %LOG%

if "!REGISTERED!"=="0" (
  echo ============================================
  echo  安装完成，但 Runner 未自动注册。
  echo  需手动注册：
  echo    C:\actions-runner\config.cmd
  echo      --url https://github.com/kxgx/remilia_pet_qt
  echo      --token YOUR_RUNNER_TOKEN
  echo      --name nas-win-x64
  echo      --labels Windows,X64,nas
  echo      --runasservice
  echo ============================================
  echo  获取令牌：GitHub 仓库 Settings ^> Actions ^> Runners ^> New self-hosted runner
  echo  日志文件：C:\OEM\install.log
)

exit /b 0
