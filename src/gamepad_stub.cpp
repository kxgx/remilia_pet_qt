// gamepad_stub.cpp — 手柄输入轮询空实现（非 Windows 平台）
// 保证 macOS/Linux/FreeBSD 构建链接通过；手柄键位显示为 Windows 实验性功能。
#include "gamepad.h"

bool initGamepad()
{
    return false;
}

void shutdownGamepad()
{
}

bool pollGamepad(GamepadState &out)
{
    out = GamepadState();
    return false;
}
