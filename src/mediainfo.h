// mediainfo.h — Windows 系统媒体控制（SMTC）当前播放信息接口（实验性功能：音乐信息显示）
// Windows: 后台线程轮询 GlobalSystemMediaTransportControlsSessionManager（C++/WinRT，SDK 自带头文件，零新依赖）
// 其他平台: mediainfo_stub.cpp 空实现——保证 macOS/Linux 构建链接通过
#ifndef MEDIAINFO_H
#define MEDIAINFO_H

#include <QImage>
#include <QString>

struct MediaInfoSnapshot
{
    bool hasSession = false; // 是否有活动的媒体会话
    bool isPlaying = false;  // 播放/暂停状态
    QString title;
    QString artist;
    QString album;
    double positionSec = 0.0; // 当前进度（秒）
    double durationSec = 0.0; // 总时长（秒），0 表示未知/直播流
    QImage cover;             // 专辑封面（无封面时为空）
};

// 启动后台轮询线程（约 800ms 一次，结果写入线程安全快照）。失败返回 false。
bool startMediaInfo();
void stopMediaInfo();
// 取最新快照（线程安全；无会话时 hasSession=false）
MediaInfoSnapshot mediaInfoSnapshot();

#endif // MEDIAINFO_H
