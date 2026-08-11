@echo off
setlocal enabledelayedexpansion
chcp 65001 >nul

set LOG=C:\OEM\install.log
echo %date% %time% === RemiliaPet auto config start === > %LOG%

:: ==========================================
:: 0. Setup Proxy (192.168.2.54:9081)
:: ==========================================
echo [0/7] Setting up proxy...
echo %date% %time% [0/7] Proxy: 192.168.2.54:9081 >> %LOG%
set HTTP_PROXY=http://192.168.2.54:9081
set HTTPS_PROXY=http://192.168.2.54:9081
set http_proxy=http://192.168.2.54:9081
set https_proxy=http://192.168.2.54:9081
set NO_PROXY=localhost,127.0.0.1
set no_proxy=localhost,127.0.0.1
:: git proxy (will take effect after git installed in step 3)
git config --global http.proxy http://192.168.2.54:9081 2>nul
git config --global https.proxy http://192.168.2.54:9081 2>nul
echo %date% %time% [OK] Proxy env vars set >> %LOG%

:: ==========================================
:: 1. Install Chocolatey
:: ==========================================
echo [1/7] Installing Chocolatey...
echo %date% %time% [1/7] Chocolatey >> %LOG%
powershell -NoProfile -ExecutionPolicy Bypass -Command "Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))" >> %LOG% 2>&1
if errorlevel 1 (
  echo %date% %time% [FAIL] Chocolatey >> %LOG%
  goto :done
)
echo %date% %time% [OK] Chocolatey >> %LOG%

:: Chocolatey proxy
choco config set proxy http://192.168.2.54:9081 >> %LOG% 2>&1

:: Refresh PATH after Chocolatey install
call refreshenv 2>nul
set "PATH=%ALLUSERSPROFILE%\chocolatey\bin;%PATH%"

:: ==========================================
:: 2. Install VS2022 Build Tools
:: ==========================================
echo [2/7] Installing VS2022 Build Tools (~15-30 min)...
echo %date% %time% [2/7] VS2022 >> %LOG%
choco install visualstudio2022buildtools -y --package-parameters "--add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 --add Microsoft.VisualStudio.Component.Windows11SDK.22621 --includeRecommended --passive --wait" >> %LOG% 2>&1
if errorlevel 1 (
  echo %date% %time% [FAIL] VS2022 >> %LOG%
  goto :done
)
echo %date% %time% [OK] VS2022 >> %LOG%

:: ==========================================
:: 3. Install Git
:: ==========================================
echo [3/7] Installing Git...
echo %date% %time% [3/7] Git >> %LOG%
choco install git -y >> %LOG% 2>&1
if errorlevel 1 (
  echo %date% %time% [FAIL] Git >> %LOG%
  goto :done
)
echo %date% %time% [OK] Git >> %LOG%
set "PATH=C:\Program Files\Git\bin;C:\Program Files\Git\usr\bin;%PATH%"

:: Git proxy
git config --global http.proxy http://192.168.2.54:9081 >> %LOG% 2>&1
git config --global https.proxy http://192.168.2.54:9081 >> %LOG% 2>&1
echo %date% %time% [OK] Git proxy configured >> %LOG%

:: ==========================================
:: 4. Install vcpkg
:: ==========================================
echo [4/7] Installing vcpkg...
echo %date% %time% [4/7] vcpkg clone >> %LOG%
cd /d C:\
if exist C:\vcpkg rmdir /s /q C:\vcpkg

:: Retry clone up to 3 times with --depth 1
set RETRY=0
:retry_clone
set /a RETRY+=1
echo %date% %time% vcpkg clone attempt !RETRY!/3 >> %LOG%
git clone --depth 1 https://github.com/microsoft/vcpkg.git >> %LOG% 2>&1
if not errorlevel 1 goto :clone_ok
if !RETRY! LSS 3 (
  echo %date% %time% clone failed, retrying in 10s... >> %LOG%
  if exist C:\vcpkg rmdir /s /q C:\vcpkg
  timeout /t 10 /nobreak >nul
  goto :retry_clone
)
echo %date% %time% [FAIL] vcpkg clone after 3 attempts >> %LOG%
goto :done

:clone_ok
cd C:\vcpkg
call bootstrap-vcpkg.bat >> %LOG% 2>&1
vcpkg integrate install >> %LOG% 2>&1
echo %date% %time% [OK] vcpkg >> %LOG%

:: ==========================================
:: 5. Install Qt6 static deps
:: ==========================================
echo [5/7] Installing Qt6 static deps (~30-60 min)...
echo %date% %time% [5/7] Qt6 static >> %LOG%

set RETRY=0
:retry_qt6
set /a RETRY+=1
echo %date% %time% Qt6 install attempt !RETRY!/3 >> %LOG%
vcpkg install qt6[core,multimedia,gui,widgets]:x64-windows-static-md >> %LOG% 2>&1
if not errorlevel 1 goto :qt6_ok
if !RETRY! LSS 3 (
  echo %date% %time% Qt6 failed, retrying in 10s... >> %LOG%
  timeout /t 10 /nobreak >nul
  goto :retry_qt6
)
echo %date% %time% [FAIL] Qt6 after 3 attempts >> %LOG%
goto :done

:qt6_ok
echo %date% %time% [OK] Qt6 >> %LOG%

:: ==========================================
:: 6. Install GitHub Actions Runner
:: ==========================================
echo [6/7] Installing GitHub Actions Runner...
echo %date% %time% [6/7] Runner download >> %LOG%
if exist C:\actions-runner rmdir /s /q C:\actions-runner
mkdir C:\actions-runner
cd C:\actions-runner
powershell -Command "Invoke-WebRequest -Uri 'https://github.com/actions/runner/releases/latest/download/actions-runner-win-x64.zip' -OutFile runner.zip" >> %LOG% 2>&1
powershell -Command "Expand-Archive runner.zip -DestinationPath ." >> %LOG% 2>&1
del runner.zip
echo %date% %time% [OK] Runner files >> %LOG%

:: ==========================================
:: 7. Auto-register GitHub Runner
:: ==========================================
echo [7/7] Registering GitHub Runner...
echo %date% %time% [7/7] Runner register >> %LOG%

set "REGISTERED=0"

if exist C:\OEM\github-pat.txt (
  echo Found github-pat.txt, fetching runner token...
  echo %date% %time% reading github-pat.txt >> %LOG%

  :: Write a temp PowerShell script and run it
  (
    echo $pat = (Get-Content C:\OEM\github-pat.txt -Raw).Trim()
    echo $resp = Invoke-RestMethod -Uri 'https://api.github.com/repos/kxgx/remilia_pet_qt/actions/runners/registration-token' -Method Post -Headers @{Authorization="Bearer $pat"; Accept="application/vnd.github+json"} -Body '{}' -ContentType 'application/json'
    echo $resp.token ^| Out-File -Encoding ASCII C:\OEM\runner-token.txt
  ) > C:\OEM\_get_token.ps1

  powershell -NoProfile -ExecutionPolicy Bypass -File C:\OEM\_get_token.ps1 >> %LOG% 2>&1
  del C:\OEM\_get_token.ps1

  if exist C:\OEM\runner-token.txt (
    set /p RUNNER_TOKEN=<C:\OEM\runner-token.txt
    echo %date% %time% Token acquired >> %LOG%

    call config.cmd --url https://github.com/kxgx/remilia_pet_qt --token !RUNNER_TOKEN! --name nas-win-x64 --labels Windows,X64,nas --unattended --runasservice >> %LOG% 2>&1

    if !ERRORLEVEL! EQU 0 (
      set "REGISTERED=1"
      echo %date% %time% [OK] Runner registered, service started >> %LOG%
    ) else (
      echo %date% %time% [FAIL] Runner registration >> %LOG%
    )
  ) else (
    echo %date% %time% [FAIL] Failed to get runner token >> %LOG%
  )
) else (
  echo %date% %time% No github-pat.txt, skip auto-register >> %LOG%
)

:done
echo %date% %time% === Config done, registered=!REGISTERED! === >> %LOG%

if "!REGISTERED!"=="0" (
  echo ============================================
  echo  Install done. Runner NOT auto-registered.
  echo  Manual registration:
  echo    C:\actions-runner\config.cmd
  echo      --url https://github.com/kxgx/remilia_pet_qt
  echo      --token YOUR_RUNNER_TOKEN
  echo      --name nas-win-x64
  echo      --labels Windows,X64,nas
  echo      --runasservice
  echo ============================================
  echo  Log: C:\OEM\install.log
)

exit /b 0
