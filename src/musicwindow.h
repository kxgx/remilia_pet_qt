// musicwindow.h — 音乐信息显示窗口（实验性功能）
// 无边框深色圆角窗：专辑封面 + 标题/歌手/专辑 + 播放状态 + 进度条（SMTC 当前播放内容），
// 独立窗口，贴靠宠物左侧，随宠物缩放；无媒体会话时由 DesktopPet 隐藏。
#ifndef MUSICWINDOW_H
#define MUSICWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
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
    QFont titleFont() const; // 与标题 QSS 一致的字体（QSS 不影响 widget->font()）
    QString scrollWindow(const QFontMetrics &fm, int offset) const;
    void scrollTick(); // 跑马灯步进（长标题滚动显示）

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

    // 标题跑马灯状态：标题或标签宽度变化才重置，滚动由定时器独立推进，
    // 不受 800ms 信息刷新打断
    QTimer *m_scrollTimer = nullptr;
    bool m_scrolling = false;
    QString m_scrollBase;   // 原始标题
    QString m_scrollFull;   // base + 空隙 + base（无缝循环）
    int m_scrollTotal = 0;  // 首段 base+空隙 的像素宽（循环点）
    int m_scrollOffset = 0; // 当前滚动像素偏移
    int m_scrollHold = 0;   // 起始/末尾停留计数（tick）
    int m_scrollFitWidth = -1;
};

#endif // MUSICWINDOW_H
