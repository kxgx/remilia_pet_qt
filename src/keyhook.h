// keyhook.h — 全局键盘监听接口（键位显示用）
// Windows: WH_KEYBOARD_LL 钩子（回调在 Qt 主线程消息泵内直接执行）
// macOS:   CGEventTap（需「辅助功能」授权，未授权时安装失败、功能无显示）
// Linux:   XQueryKeymap 轮询（由 Qt 定时器驱动，Wayland 纯会话不支持，XWayland 正常）
#ifndef KEYHOOK_H
#define KEYHOOK_H

#include <QString>

using GlobalKeyCallback = void (*)(const QString &keyText);

bool startGlobalKeyListener(GlobalKeyCallback cb);
void stopGlobalKeyListener();

// Linux 专用：每 40ms 轮询一次，返回本次新按下的键名，无新键返回空串
QString pollLinuxGlobalKey();

#endif // KEYHOOK_H
