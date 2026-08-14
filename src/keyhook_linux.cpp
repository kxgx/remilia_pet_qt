// keyhook_linux.cpp — Linux 全局键盘轮询（XQueryKeymap）
// 每 40ms 由 Qt 定时器调用 pollLinuxGlobalKey()：比较前后键盘位图找出新按下的键。
// Wayland 纯会话无 X display（XOpenDisplay 失败），功能无显示；XWayland 正常。
#include "keyhook.h"
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>

#include <cstring>

static Display *s_dpy = nullptr;
static unsigned char s_prev[32] = {0};

static bool shiftPressed()
{
    if (!s_dpy) return false;
    char keys[32];
    XQueryKeymap(s_dpy, keys);
    KeyCode lc = XKeysymToKeycode(s_dpy, XK_Shift_L);
    KeyCode rc = XKeysymToKeycode(s_dpy, XK_Shift_R);
    bool l = lc && (keys[lc / 8] & (1 << (lc % 8)));
    bool r = rc && (keys[rc / 8] & (1 << (rc % 8)));
    return l || r;
}

static QString x11KeyText(KeySym sym)
{
    switch (sym) {
    case XK_space:     return QString::fromUtf8("\u7A7A\u683C"); // 空格
    case XK_Return:    return QString::fromUtf8("\u56DE\u8F66"); // 回车
    case XK_BackSpace: return QString::fromUtf8("\u9000\u683C"); // 退格
    case XK_Delete:    return QString::fromUtf8("\u5220\u9664"); // 删除
    case XK_Tab:       return QStringLiteral("Tab");
    case XK_Escape:    return QStringLiteral("Esc");
    case XK_Caps_Lock: return QStringLiteral("Caps");
    case XK_Up:        return QString::fromUtf8("\u2191");
    case XK_Down:      return QString::fromUtf8("\u2193");
    case XK_Left:      return QString::fromUtf8("\u2190");
    case XK_Right:     return QString::fromUtf8("\u2192");
    case XK_Shift_L: case XK_Shift_R:   return QStringLiteral("Shift");
    case XK_Control_L: case XK_Control_R: return QStringLiteral("Ctrl");
    case XK_Alt_L: case XK_Alt_R:       return QStringLiteral("Alt");
    case XK_Super_L: case XK_Super_R:   return QStringLiteral("Win");
    case XK_Page_Up:   return QStringLiteral("PgUp");
    case XK_Page_Down: return QStringLiteral("PgDn");
    case XK_Home:      return QStringLiteral("Home");
    case XK_End:       return QStringLiteral("End");
    case XK_Insert:    return QStringLiteral("Ins");
    default: break;
    }
    if (sym >= XK_F1 && sym <= XK_F12)
        return QStringLiteral("F%1").arg(sym - XK_F1 + 1);
    if (sym >= 0x20 && sym <= 0x7e)
        return QString(QChar((ushort)sym)); // 可见 ASCII，Shift 状态已通过 level 1 体现
    return QString();
}

bool startGlobalKeyListener(GlobalKeyCallback)
{
    if (s_dpy) return true;
    s_dpy = XOpenDisplay(nullptr);
    if (!s_dpy) return false; // 无 X 环境
    std::memset(s_prev, 0, sizeof(s_prev));
    return true;
}

void stopGlobalKeyListener()
{
    if (s_dpy) {
        XCloseDisplay(s_dpy);
        s_dpy = nullptr;
    }
    std::memset(s_prev, 0, sizeof(s_prev));
}

QString pollLinuxGlobalKey()
{
    if (!s_dpy) return QString();
    char keys[32];
    XQueryKeymap(s_dpy, keys);
    bool shift = shiftPressed();
    for (int kc = 8; kc < 256; kc++) {
        bool down = (keys[kc / 8] >> (kc % 8)) & 1;
        bool was = (s_prev[kc / 8] >> (kc % 8)) & 1;
        if (down && !was) {
            KeySym sym = XkbKeycodeToKeysym(s_dpy, kc, 0, shift ? 1 : 0);
            std::memcpy(s_prev, keys, 32);
            if (sym == NoSymbol) continue;
            return x11KeyText(sym);
        }
    }
    std::memcpy(s_prev, keys, 32);
    return QString();
}
