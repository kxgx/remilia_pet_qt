// musicwindow.h — 音乐信息显示窗口（实验性功能）
// 无边框深色圆角窗：专辑封面 + 标题/歌手/专辑 + 播放状态 + 进度条（SMTC 当前播放内容），
// 独立窗口，贴靠宠物左侧，随宠物缩放；无媒体会话时由 DesktopPet 隐藏。
#ifndef MUSICWINDOW_H
#define MUSICWINDOW_H

#include <QWidget>
#include <QLabel>
#include "mediainfo.h"

class DesktopPet;

class MusicWindow : public QWidget
{
    Q_OBJECT
    public:
    MusicWindow(DesktopPet *pet, float scale, bool stayOnTop);

    void updateInfo(const MediaInfoSnapshot &snap);
    void updateScaleAndPosition(float scale);

    protected:
    void paintEvent(QPaintEvent *) override;

    private:
    void positionNearPet();
    void rebuildLayout();
    QString statusText() const;

    DesktopPet *m_pet;
    float m_scale;
    MediaInfoSnapshot m_snap;
    qint64 m_coverCacheKey = 0;
    QLabel *m_cover = nullptr;
    QLabel *m_title = nullptr;
    QLabel *m_artist = nullptr;
    QLabel *m_status = nullptr;
    QLabel *m_time = nullptr;
    QLabel *m_barBg = nullptr;
    QLabel *m_barFill = nullptr;
};

#endif // MUSICWINDOW_H
