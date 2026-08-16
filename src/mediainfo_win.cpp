// mediainfo_win.cpp — Windows SMTC（系统媒体传输控制）当前播放信息轮询（实验性功能：音乐信息显示）
// 零新依赖：C++/WinRT 投影头文件由 Windows SDK 自带（cppwinrt/winrt/windows.media.control.h），
// 仅链接系统库 combase.lib。后台线程每 800ms 轮询当前会话，结果写入互斥锁保护的快照，
// UI 线程按需取快照——WinRT 异步接口在 C++17 下用阻塞 get() 驱动（无协程）。
#include "mediainfo.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <unknwn.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h> // IVectorView 的 consume 定义在此顶层头（仅含 impl 分片会 C3779）
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>

#include <QMutex>
#include <QMutexLocker>
#include <QByteArray>
#include <atomic>
#include <thread>

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Media::Control;
using namespace winrt::Windows::Storage::Streams;

namespace
{
std::thread s_thread;
std::atomic<bool> s_running{false};
QMutex s_mutex;
MediaInfoSnapshot s_snap;
QString s_coverKey; // 与 s_snap 同锁保护：封面仅随曲目切换重取

// 封面读取上限：异常大图（非图片流）直接放弃
constexpr uint64_t kMaxCoverBytes = 32ull * 1024ull * 1024ull;

QString hstr(const winrt::hstring &h)
{
    if (h.empty())
        return QString();
    return QString::fromWCharArray(h.c_str(), static_cast<qsizetype>(h.size()));
}

// 专辑封面：IRandomAccessStreamReference → 字节流 → QImage
QImage readCover(const IRandomAccessStreamReference &thumbRef)
{
    try
    {
        auto stream = thumbRef.OpenReadAsync().get();
        if (!stream)
            return QImage();
        const uint64_t size = stream.Size();
        if (size == 0 || size > kMaxCoverBytes)
            return QImage();
        DataReader reader{stream.GetInputStreamAt(0)};
        const uint32_t loaded = reader.LoadAsync(static_cast<uint32_t>(size)).get();
        if (loaded == 0)
            return QImage();
        QByteArray bytes(static_cast<qsizetype>(loaded), Qt::Uninitialized);
        auto *begin = reinterpret_cast<uint8_t *>(bytes.data());
        reader.ReadBytes(winrt::array_view<uint8_t>(begin, begin + bytes.size()));
        QImage img;
        if (img.loadFromData(bytes))
            return img;
    }
    catch (...)
    {
    }
    return QImage();
}

void pollOnce(GlobalSystemMediaTransportControlsSessionManager &manager, MediaInfoSnapshot &snap)
{
    try
    {
        GlobalSystemMediaTransportControlsSession session{nullptr};
        if (manager)
            session = manager.GetCurrentSession();
        if (!session)
        {
            // 部分播放器有会话但未注册为"当前"：回退取第一个会话
            auto sessions = manager.GetSessions();
            if (sessions.Size() > 0)
                session = sessions.GetAt(0);
        }
        if (!session)
            return; // 无媒体会话 → hasSession 保持 false

        snap.hasSession = true;
        auto info = session.GetPlaybackInfo();
        if (info)
            snap.isPlaying = info.PlaybackStatus() == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing;
        auto timeline = session.GetTimelineProperties();
        if (timeline)
        {
            snap.positionSec = static_cast<double>(timeline.Position().count()) / 10000000.0;
            snap.durationSec = static_cast<double>(timeline.EndTime().count()) / 10000000.0;
        }
        auto props = session.TryGetMediaPropertiesAsync().get();
        if (props)
        {
            snap.title = hstr(props.Title());
            snap.artist = hstr(props.Artist());
            snap.album = hstr(props.AlbumTitle());
            // 封面只随曲目切换重取，避免每轮都解码整张图
            const QString key = snap.title + QLatin1Char('\n') + snap.artist;
            bool mustFetch = false;
            {
                QMutexLocker locker(&s_mutex);
                snap.cover = s_snap.cover; // 默认沿用上一张封面
                if (key != s_coverKey)
                {
                    s_coverKey = key;
                    mustFetch = true;
                }
            }
            if (mustFetch)
                snap.cover = readCover(props.Thumbnail());
        }
    }
    catch (...)
    {
        // 会话在查询间隙消失等瞬时错误：本轮跳过，下轮重试
    }
}

void pollLoop()
{
    try
    {
        init_apartment(apartment_type::multi_threaded);
    }
    catch (...)
    {
        return; // COM 初始化失败（罕见）：静默退出，功能不可用
    }

    GlobalSystemMediaTransportControlsSessionManager manager{nullptr};
    try
    {
        manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
    }
    catch (...)
    {
        // 系统媒体控制不可用（精简系统/旧系统）：线程退出
        return;
    }

    while (s_running.load())
    {
        MediaInfoSnapshot snap;
        pollOnce(manager, snap);
        {
            QMutexLocker locker(&s_mutex);
            s_snap = snap;
        }
        // 分片睡眠以便 stopMediaInfo 快速退出
        for (int i = 0; i < 8 && s_running.load(); ++i)
            Sleep(100);
    }
}
} // namespace

bool startMediaInfo()
{
    if (s_thread.joinable())
        return true;
    s_running = true;
    try
    {
        s_thread = std::thread(pollLoop);
    }
    catch (...)
    {
        s_running = false;
        return false;
    }
    return true;
}

void stopMediaInfo()
{
    s_running = false;
    if (s_thread.joinable())
    {
        s_thread.join();
        s_thread = std::thread(); // 复位为不可 join 状态，允许再次 startMediaInfo
    }
}

MediaInfoSnapshot mediaInfoSnapshot()
{
    QMutexLocker locker(&s_mutex);
    return s_snap;
}
