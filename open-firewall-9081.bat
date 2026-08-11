@echo off
chcp 65001 >nul
echo Adding firewall rules for port 9081...

netsh advfirewall firewall add rule name="Proxy9081 TCP IN"  dir=in  action=allow protocol=TCP localport=9081
netsh advfirewall firewall add rule name="Proxy9081 TCP OUT" dir=out action=allow protocol=TCP localport=9081
netsh advfirewall firewall add rule name="Proxy9081 UDP IN"  dir=in  action=allow protocol=UDP localport=9081
netsh advfirewall firewall add rule name="Proxy9081 UDP OUT" dir=out action=allow protocol=UDP localport=9081

echo.
echo Done. Verifying...
netsh advfirewall firewall show rule name="Proxy9081 TCP IN" | findstr /i "启用"
echo.
pause
