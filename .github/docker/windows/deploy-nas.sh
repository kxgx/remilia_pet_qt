#!/bin/bash
# =============================================
# RemiliaPet Windows 编译 VM 部署脚本
# 在 NAS 上执行: bash deploy-nas.sh
# =============================================

set -e

echo "=== RemiliaPet Windows 编译 VM 部署 ==="

# 1. 创建目录结构
mkdir -p /vol6/1000/windows/oem
mkdir -p /vol6/1000/windows/data

echo "[OK] 目录已创建"

# 2. 放置 github-pat.txt（把 GitHub PAT 放在这里）
echo "⚠️  请确保 /vol6/1000/windows/oem/github-pat.txt 已填入你真实的 GitHub PAT"
echo "   获取地址: https://github.com/settings/tokens"
echo "   所需权限: repo + admin:org/manage_runners"

# 3. 停止并删除旧容器
echo "停止旧容器..."
docker stop windows-build 2>/dev/null || true
docker rm windows-build 2>/dev/null || true

echo "[OK] 旧容器已清理"

# 4. 启动新容器
echo "启动 Windows VM（512G 磁盘）..."
docker run -d \
  --name windows-build \
  --device=/dev/kvm \
  --device=/dev/net/tun \
  --cap-add NET_ADMIN \
  -e VERSION=11l \
  -e DISK_SIZE=512G \
  -e RAM_SIZE=16G \
  -e CPU_CORES=8 \
  -e LANGUAGE=Chinese \
  -p 8006:8006 \
  -p 3389:3389/tcp \
  -p 3389:3389/udp \
  -v /vol6/1000/windows/oem:/oem \
  -v /vol6/1000/windows/data:/storage \
  --stop-timeout 120 \
  dockurr/windows:latest

echo ""
echo "=== 部署完成 ==="
echo ""
echo "后续步骤："
echo "1. 监控安装日志: docker logs -f windows-build"
echo "2. Windows 安装完成后会自动执行 install.bat"
echo "3. 查看 OEM 执行日志: docker exec windows-build cat /storage/../oem/install.log"
echo "   或者通过 RDP 登录查看 C:\OEM\install.log"
echo "4. 确认 Runner 在线: https://github.com/kxgx/remilia_pet_qt/settings/actions/runners"
echo ""
echo "Web VNC:  http://192.168.2.2:8006"
echo "RDP:      rdp://192.168.2.2:3389"
echo "容器日志: docker logs -f windows-build"
