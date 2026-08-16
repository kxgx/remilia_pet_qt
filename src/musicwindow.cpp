// musicwindow.cpp — 音乐信息显示窗口实现（实验性功能：SMTC 当前播放内容）
// 全量信息：专辑封面 + 标题 + 歌手 + 专辑 + 播放状态 + 时间 + 进度条；
// 逻辑布局 360×132（scale=1），随宠物缩放整体缩放；贴靠宠物左侧。
#include "musicwindow.h"
#include "petwindow.h"
#include "inappfiledialog.h" // PINK

#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QPixmap>
#include <QScreen>
#include <QGuiApplication>
#include <QPointer>

namespace
{
constexpr int kWinW = 360;
constexpr int kWinH = 132;

QString fmtTime(double sec)
{
    if (sec < 0.0 || sec > 359999.0)
        sec = 0.0;
    const int total = static_cast<int>(sec);
    const int h = total / 3600;
    const int m = (total % 3600) / 60;
    const int s = total % 60;
    if (h > 0)
        return QStringLiteral("%1:%2:%3").arg(h).arg(m, 2, 10, QLatin1Char('0')).arg(s, 2, 10, QLatin1Char('0'));
    return QStringLiteral("%1:%2").arg(m).arg(s, 2, 10, QLatin1Char('0'));
}
} // namespace

MusicWindow::MusicWindow(DesktopPet *pet, float scale, bool stayOnTop)
    : QWidget(nullptr, Qt::FramelessWindowHint | (stayOnTop ? Qt::WindowStaysOnTopHint : Qt::WindowType(0)) | Qt::Tool), m_pet(pet), m_scale(scale)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    m_cover = new QLabel(this);
    m_cover->setAlignment(Qt::AlignCenter);
    m_title = new QLabel(this);
    m_artist = new QLabel(this);
    m_status = new QLabel(this);
    m_time = new QLabel(this);
    m_barBg = new QLabel(this);
    m_barFill = new QLabel(this);
    m_barFill->setParent(m_barBg);
    // 标题跑马灯定时器：60ms 步进，短标题不启动
    m_scrollTimer = new QTimer(this);
    m_scrollTimer->setInterval(60);
    connect(m_scrollTimer, &QTimer::timeout, this, [this]()
            { scrollTick(); });
    updateScaleAndPosition(scale);
}

void MusicWindow::updateInfo(const MediaInfoSnapshot &snap)
{
    m_snap = snap;
    rebuildLayout();
    positionNearPet(); // 隐藏→显示期间宠物可能移动过，显示前重新贴靠
}

void MusicWindow::updateScaleAndPosition(float scale)
{
    m_scale = scale;
    const int w = qMax(200, static_cast<int>(kWinW * scale));
    const int h = qMax(80, static_cast<int>(kWinH * scale));
    const int cover = qMax(48, static_cast<int>(88 * scale));
    setFixedSize(w, h);
    m_cover->setGeometry(10, (h - cover) / 2, cover, cover);
    const int x0 = cover + 22;
    const int colW = w - x0 - 12;
    const int titleFs = qMax(13, static_cast<int>(15 * scale));
    const int artistFs = qMax(11, static_cast<int>(12 * scale));
    const int statusFs = qMax(11, static_cast<int>(12 * scale));
    m_title->setGeometry(x0, 8, colW, titleFs + 8);
    m_title->setStyleSheet(QString("color:#FF8DA1;font-weight:bold;font-size:%1px;background:transparent;").arg(titleFs));
    m_artist->setGeometry(x0, 8 + titleFs + 8, colW, artistFs + 6);
    m_artist->setStyleSheet(QString("color:#E8E8E8;font-size:%1px;background:transparent;").arg(artistFs));
    m_status->setGeometry(x0, h - 34, colW / 2, statusFs + 6);
    m_status->setStyleSheet(QString("color:#FF8DA1;font-weight:bold;font-size:%1px;background:transparent;").arg(statusFs));
    m_time->setGeometry(x0 + colW / 2, h - 34, colW / 2, statusFs + 6);
    m_time->setStyleSheet(QString("color:#9A9A9A;font-size:%1px;background:transparent;").arg(statusFs));
    m_time->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_barBg->setGeometry(x0, h - 18, colW, 8);
    m_barBg->setStyleSheet("background:#3A3A3A;border-radius:4px;");
    m_coverCacheKey = -1; // 尺寸变化：强制重缩放封面
    rebuildLayout();
    positionNearPet();
}

void MusicWindow::positionNearPet()
{
    if (!m_pet)
        return;
    const QRect pr = m_pet->geometry();
    QScreen *sc = QGuiApplication::screenAt(pr.center());
    const QRect avail = sc ? sc->availableGeometry() : QRect();

    // 已占位窗口（键盘气泡/手柄窗），避免互相遮挡
    auto visRect = [this](const QPointer<QWidget> &win) -> QRect
    {
        QWidget *w = win.data();
        return (w && w->isVisible()) ? w->geometry() : QRect();
    };
    const QRect keyRect = visRect(m_pet->m_keyWindow);
    const QRect padRect = visRect(m_pet->m_gamepadWindow);
    const QRect padKeyRect = visRect(m_pet->m_gamepadKeyWindow);

    // 候选位置必须：不压宠物、不压键盘气泡、不压手柄图/文字气泡、不出屏幕可用区
    auto fits = [&](const QRect &r)
    {
        if (r.intersects(pr))
            return false;
        if (keyRect.isValid() && r.intersects(keyRect))
            return false;
        if (padRect.isValid() && r.intersects(padRect))
            return false;
        if (padKeyRect.isValid() && r.intersects(padKeyRect))
            return false;
        if (avail.isValid())
            return r.left() >= avail.left() && r.top() >= avail.top() && r.right() <= avail.left() + avail.width() - 1 &&
                   r.bottom() <= avail.top() + avail.height() - 1;
        return true;
    };

    QRect cand;
    // 1) 宠物左侧垂直居中（首选）
    cand = QRect(pr.x() - width() - 12, pr.y() + (pr.height() - height()) / 2, width(), height());
    if (fits(cand))
    {
        move(cand.topLeft());
        return;
    }
    // 2) 宠物右侧垂直居中
    cand = QRect(pr.x() + pr.width() + 12, pr.y() + (pr.height() - height()) / 2, width(), height());
    if (fits(cand))
    {
        move(cand.topLeft());
        return;
    }
    // 3) 宠物上方：叠在键盘/手柄气泡行之上
    int topmost = pr.y() - 10;
    if (keyRect.isValid())
        topmost = qMin(topmost, keyRect.top() - 10);
    if (padRect.isValid())
        topmost = qMin(topmost, padRect.top() - 10);
    cand = QRect(pr.x() + (pr.width() - width()) / 2, topmost - height(), width(), height());
    if (fits(cand))
    {
        move(cand.topLeft());
        return;
    }
    // 4) 宠物下方兜底
    cand = QRect(pr.x() + (pr.width() - width()) / 2, pr.y() + pr.height() + 10, width(), height());
    if (fits(cand))
    {
        move(cand.topLeft());
        return;
    }
    // 5) 全部槽位不可用：屏幕可用区内钳位
    if (avail.isValid())
    {
        const int x = qBound(avail.left(), cand.x(), avail.left() + avail.width() - width());
        const int y = qBound(avail.top(), cand.y(), avail.top() + avail.height() - height());
        move(x, y);
    }
    else
    {
        move(cand.topLeft());
    }
}

QString MusicWindow::statusText() const
{
    if (!m_snap.hasSession)
        return QString();
    if (m_snap.isPlaying)
        return QString::fromUtf8("\u64AD\u653E\u4E2D"); // 播放中
    return QString::fromUtf8("\u5DF2\u6682\u505C");     // 已暂停（Stopped 亦显示为未播放）
}

QFont MusicWindow::titleFont() const
{
    // QSS 设置的 font-size 不反映在 widget->font()，这里构造与标题 QSS 一致的字体做测量
    QFont f = m_title->font();
    f.setPixelSize(qMax(13, static_cast<int>(15 * m_scale)));
    f.setBold(true);
    return f;
}

// 从 m_scrollFull 中截取偏移 offset 像素起、宽度不超过标签宽的一段可见文本
QString MusicWindow::scrollWindow(const QFontMetrics &fm, int offset) const
{
    int acc = 0;
    int start = 0;
    while (start < m_scrollFull.size() && acc < offset)
    {
        acc += fm.horizontalAdvance(m_scrollFull.at(start));
        ++start;
    }
    QString shown;
    int used = 0;
    for (int i = start; i < m_scrollFull.size(); ++i)
    {
        const QChar c = m_scrollFull.at(i);
        const int w = fm.horizontalAdvance(c);
        if (used + w > m_scrollFitWidth && used > 0)
            break;
        shown += c;
        used += w;
    }
    return shown;
}

void MusicWindow::scrollTick()
{
    if (!m_scrolling || m_scrollFull.isEmpty())
        return;
    if (m_scrollHold > 0)
    {
        --m_scrollHold; // 停留阶段：保持当前显示
        return;
    }
    m_scrollOffset += 4; // 4px/60ms ≈ 67px/s
    if (m_scrollOffset >= m_scrollTotal)
    {
        m_scrollOffset = 0;
        m_scrollHold = 12; // 无缝循环（第二段 base 衔接），开头再停留约 0.7s
    }
    m_title->setText(scrollWindow(QFontMetrics(titleFont()), m_scrollOffset));
}

void MusicWindow::rebuildLayout()
{
    // 封面（无封面时显示音符占位）；按 cacheKey 去重，避免每轮轮询重复缩放整张图
    const int side = m_cover->height();
    if (!m_snap.cover.isNull())
    {
        if (m_snap.cover.cacheKey() != m_coverCacheKey)
        {
            m_coverCacheKey = m_snap.cover.cacheKey();
            m_cover->setPixmap(QPixmap::fromImage(m_snap.cover).scaled(side, side, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        m_cover->setText(QString());
        m_cover->setStyleSheet("background:#2E2E2E;border:1px solid #5A5A5A;border-radius:8px;");
    }
    else
    {
        m_coverCacheKey = 0;
        m_cover->setPixmap(QPixmap());
        m_cover->setText(QStringLiteral("\u266A"));
        m_cover->setStyleSheet(QString("color:#9A9A9A;font-size:%1px;font-weight:bold;background:#2E2E2E;border:1px solid #5A5A5A;border-radius:8px;").arg(qMax(18, static_cast<int>(30 * m_scale))));
    }
    // 标题：放得下 → 静态显示；放不下 → 跑马灯滚动（标题/宽度变化时重置，
    // 否则保持 scrollTick 的推进节奏，不被 800ms 信息刷新打断）
    const QFontMetrics fm(titleFont());
    const int labelW = m_title->width();
    if (m_snap.title.isEmpty() || fm.horizontalAdvance(m_snap.title) <= labelW)
    {
        m_scrolling = false;
        m_scrollTimer->stop();
        m_scrollBase.clear();
        m_title->setText(m_snap.title);
    }
    else if (m_scrollBase != m_snap.title || m_scrollFitWidth != labelW)
    {
        m_scrollBase = m_snap.title;
        m_scrollFitWidth = labelW;
        m_scrollFull = m_scrollBase + QStringLiteral("       ") + m_scrollBase;
        m_scrollTotal = fm.horizontalAdvance(m_scrollBase) + fm.horizontalAdvance(QStringLiteral("       "));
        m_scrollOffset = 0;
        m_scrollHold = 15; // 开头停留约 0.9s
        m_scrolling = true;
        m_scrollTimer->start();
        m_title->setText(scrollWindow(fm, m_scrollOffset));
    }
    const QString artistLine = m_snap.artist + (m_snap.album.isEmpty() ? QString() : QStringLiteral(" \u2014 ") + m_snap.album);
    QFont af = m_artist->font();
    af.setPixelSize(qMax(11, static_cast<int>(12 * m_scale)));
    const QFontMetrics fam(af);
    m_artist->setText(fam.elidedText(artistLine, Qt::ElideRight, m_artist->width()));
    // 状态 + 时间 + 进度条
    m_status->setText(statusText());
    m_time->setText(fmtTime(m_snap.positionSec) + QStringLiteral(" / ") + fmtTime(m_snap.durationSec));
    const int bw = m_barBg->width();
    int fw = 0;
    if (m_snap.durationSec > 0.5 && bw > 0)
    {
        const double frac = qBound(0.0, m_snap.positionSec / m_snap.durationSec, 1.0);
        fw = qMax(6, static_cast<int>(bw * frac));
    }
    m_barFill->setGeometry(0, 0, fw, m_barBg->height());
    m_barFill->setStyleSheet(fw > 0 ? "background:#FF8DA1;border-radius:4px;" : "background:transparent;");
}

void MusicWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QRectF bg(2.0, 2.0, width() - 4.0, height() - 4.0);
    p.setBrush(QBrush(QColor(26, 26, 26, 255)));
    p.setPen(QPen(PINK, 4));
    p.drawRoundedRect(bg, 10, 10);
}
