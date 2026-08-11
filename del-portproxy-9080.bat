@echo off
chcp 65001 >nul

echo === Deleting firewall rule "Proxy 9080" ===
netsh advfirewall firewall delete rule name="Proxy 9080"

echo.
echo === Deleting portproxy 9080 ===
netsh interface portproxy delete v4tov4 listenport=9080 listenaddress=0.0.0.0

echo.
echo === Verify ===
echo --- Firewall ---
netsh advfirewall firewall show rule name="Proxy 9080" 2>nul || echo (rule deleted)
echo --- PortProxy ---
netsh interface portproxy show all

echo.
echo Done.
pause
