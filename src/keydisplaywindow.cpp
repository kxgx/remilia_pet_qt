// keydisplaywindow.cpp — 键盘键位显示窗口实现
#include "keydisplaywindow.h"
#include "petwindow.h"
#include "inappfiledialog.h" // PINK

#include <QPainter>
#include <QPen>
#include <QBrush>

KeyDisplayWindow::KeyDisplayWindow(DesktopPet *pet, float scale, bool stayOnTop)
    : QWidget(nullptr, Qt::FramelessWindowHint | (stayOnTop ? Qt::WindowStaysOnTopHint : Qt::WindowType(0)) | Qt::Tool), m_pet(pet), m_scale(scale)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    m_label = new QLabel(this);
    m_label->setAlignment(Qt::AlignCenter);
    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    m_hideTimer->setInterval(2000); // 2 秒无输入自动隐藏
    connect(m_hideTimer, &QTimer::timeout, this, &QWidget::hide);
    updateScaleAndPosition(scale);
}

void KeyDisplayWindow::showKey(const QString &text)
{
    m_text = text;
    updateScaleAndPosition(m_scale);
    show();
    raise();
    m_hideTimer->start();
    // 键位气泡出现后通知其他侧窗（手柄/音乐）立即重排，避免互相遮挡
    m_pet->updateSideWindowPositions();
}

void KeyDisplayWindow::updateScaleAndPosition(float scale)
{
    m_scale = scale;
    const int fs = qMax(16, static_cast<int>(36 * scale));
    m_label->setStyleSheet(QString("color:#FF8DA1;font-weight:bold;font-size:%1px;background:transparent;").arg(fs));
    relayoutLabel();
    positionNearPet();
}

QFont KeyDisplayWindow::labelFont() const
{
    // QSS 设置的 font-size 不反映在 widget->font()，这里构造与标签 QSS 一致的字体做测量
    QFont f = m_label->font();
    f.setPixelSize(qMax(16, static_cast<int>(36 * m_scale)));
    f.setBold(true);
    return f;
}

// 气泡宽度随文本自然宽度自适应：下限 80（x scale），上限 260（x scale），超长省略
void KeyDisplayWindow::relayoutLabel()
{
    const QFontMetrics fm(labelFont());
    const int minW = qMax(60, static_cast<int>(80 * m_scale));
    const int maxW = qMax(minW, static_cast<int>(260 * m_scale));
    const int labelW = qBound(minW, fm.horizontalAdvance(m_text) + 20, maxW);
    const int labelH = qMax(44, static_cast<int>(60 * m_scale));
    m_label->setFixedSize(labelW, labelH);
    setFixedSize(labelW + 16, labelH + 16);
    m_label->move(8, 8);
    m_label->setText(fm.elidedText(m_text, Qt::ElideRight, qMax(12, labelW - 4)));
}

void KeyDisplayWindow::positionNearPet()
{
    if (!m_pet)
        return;
    QRect pr = m_pet->geometry();
    move(pr.x() + (pr.width() - width()) / 2, pr.y() - height() - 10);
}

void KeyDisplayWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QRectF r(2.0, 2.0, width() - 4.0, height() - 4.0);
    p.setBrush(QBrush(QColor(26, 26, 26, 255)));
    p.setPen(QPen(PINK, 4));
    p.drawRoundedRect(r, 10, 10);
}
