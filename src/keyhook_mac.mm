// keyhook_mac.mm — macOS 全局键盘监听（CGEventTap）
// 需在「系统设置 → 隐私与安全 → 辅助功能（输入监控）」中授权本应用；
// 未授权时 CGEventTapCreate 返回空，功能无显示（不弹错）。
#include "keyhook.h"
#import <ApplicationServices/ApplicationServices.h>

static CFMachPortRef s_tap = nullptr;
static CFRunLoopSourceRef s_source = nullptr;
static GlobalKeyCallback s_cb = nullptr;

static CGEventRef s_keyEventCallback(CGEventTapProxy, CGEventType type, CGEventRef event, void *)
{
    if (type == kCGEventKeyDown && s_cb) {
        UniChar buf[8] = {0};
        UniCharCount len = 0;
        CGEventKeyboardGetUnicodeString(event, 8, &len, buf);
        if (len > 0) {
            QString t = QString::fromUtf16(reinterpret_cast<const char16_t *>(buf), (qsizetype)len);
            s_cb(len == 1 ? t.toUpper() : t);
        }
    }
    return event;
}

bool startGlobalKeyListener(GlobalKeyCallback cb)
{
    if (s_tap) return true;
    CGEventMask mask = CGEventMaskBit(kCGEventKeyDown);
    s_tap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap,
                             kCGEventTapOptionListenOnly, mask, s_keyEventCallback, nullptr);
    if (!s_tap) return false; // 未授予辅助功能权限
    s_source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, s_tap, 0);
    CFRunLoopAddSource(CFRunLoopGetMain(), s_source, kCFRunLoopCommonModes);
    CGEventTapEnable(s_tap, true);
    s_cb = cb;
    return true;
}

void stopGlobalKeyListener()
{
    if (s_tap) {
        CGEventTapEnable(s_tap, false);
        if (s_source) {
            CFRunLoopRemoveSource(CFRunLoopGetMain(), s_source, kCFRunLoopCommonModes);
            CFRelease(s_source);
            s_source = nullptr;
        }
        CFRelease(s_tap);
        s_tap = nullptr;
    }
    s_cb = nullptr;
}
