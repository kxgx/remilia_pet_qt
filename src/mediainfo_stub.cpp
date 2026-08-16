// mediainfo_stub.cpp — 媒体信息轮询空实现（非 Windows 平台）
// 保证 macOS/Linux/FreeBSD 构建链接通过；音乐信息显示为 Windows 实验性功能（SMTC 专属）。
#include "mediainfo.h"

bool startMediaInfo()
{
    return false;
}

void stopMediaInfo()
{
}

MediaInfoSnapshot mediaInfoSnapshot()
{
    return MediaInfoSnapshot();
}
