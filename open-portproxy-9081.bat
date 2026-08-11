@echo off
chcp 65001 >nul

echo Setting up port proxy: 0.0.0.0:9081 -> 127.0.0.1:9081

:: 先删旧规则（如果有）
netsh interface portproxy delete v4tov4 listenport=9081 listenaddress=0.0.0.0 2>nul

:: 添加转发规则
netsh interface portproxy add v4tov4 listenport=9081 listenaddress=0.0.0.0 connectport=9081 connectaddress=127.0.0.1

:: 验证
echo.
echo Current portproxy rules:
netsh interface portproxy show all

echo.
echo Now external can access 9081 -> forwarded to local SSRDOG
pause
