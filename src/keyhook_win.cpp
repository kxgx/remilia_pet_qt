// keyhook_win.cpp — Windows 全局键盘钩子（WH_KEYBOARD_LL）
// 钩子安装在 Qt 主线程：低层键盘消息经该线程消息泵分发，回调可直接触达 UI。
#include "keyhook.h"
#include <windows.h>

static HHOOK s_hook = nullptr;
static GlobalKeyCallback s_cb = nullptr;

static QString winKeyText(DWORD vk)
{
    switch (vk) {
    case VK_SPACE:   return QString::fromUtf8("\u7A7A\u683C"); // 空格
    case VK_RETURN:  return QString::fromUtf8("\u56DE\u8F66"); // 回车
    case VK_BACK:    return QString::fromUtf8("\u9000\u683C"); // 退格
    case VK_DELETE:  return QString::fromUtf8("\u5220\u9664"); // 删除
    case VK_TAB:     return QStringLiteral("Tab");
    case VK_ESCAPE:  return QStringLiteral("Esc");
    case VK_CAPITAL: return QStringLiteral("Caps");
    case VK_UP:      return QString::fromUtf8("\u2191");
    case VK_DOWN:    return QString::fromUtf8("\u2193");
    case VK_LEFT:    return QString::fromUtf8("\u2190");
    case VK_RIGHT:   return QString::fromUtf8("\u2192");
    case VK_LSHIFT: case VK_RSHIFT: return QStringLiteral("Shift");
    case VK_LCONTROL: case VK_RCONTROL: return QStringLiteral("Ctrl");
    case VK_LMENU: case VK_RMENU: return QStringLiteral("Alt");
    case VK_LWIN: case VK_RWIN: return QStringLiteral("Win");
    case VK_PRIOR:  return QStringLiteral("PgUp");
    case VK_NEXT:   return QStringLiteral("PgDn");
    case VK_HOME:   return QStringLiteral("Home");
    case VK_END:    return QStringLiteral("End");
    case VK_INSERT: return QStringLiteral("Ins");
    default: break;
    }
    if (vk >= VK_F1 && vk <= VK_F12)
        return QStringLiteral("F%1").arg(vk - VK_F1 + 1);

    // 普通字符键：GetKeyboardState + ToUnicode 按 Shift/Caps 状态取实际字符
    BYTE keyState[256] = {0};
    if (!GetKeyboardState(keyState)) return QString();
    WCHAR buf[8] = {0};
    UINT sc = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    int len = ToUnicode(vk, sc, keyState, buf, 8, 0);
    if (len <= 0) return QString();
    QString t = QString::fromWCharArray(buf, len);
    if (t.trimmed().isEmpty()) return QString();
    return len == 1 ? t.toUpper() : t;
}

static LRESULT CALLBACK s_lowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) && s_cb) {
        KBDLLHOOKSTRUCT *kb = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
        QString text = winKeyText(kb->vkCode);
        if (!text.isEmpty()) s_cb(text);
    }
    return CallNextHookEx(s_hook, nCode, wParam, lParam);
}

bool startGlobalKeyListener(GlobalKeyCallback cb)
{
    if (s_hook) return true;
    s_cb = cb;
    s_hook = SetWindowsHookExW(WH_KEYBOARD_LL, s_lowLevelKeyboardProc,
                               GetModuleHandleW(nullptr), 0);
    return s_hook != nullptr;
}

void stopGlobalKeyListener()
{
    if (s_hook) {
        UnhookWindowsHookEx(s_hook);
        s_hook = nullptr;
    }
    s_cb = nullptr;
}
