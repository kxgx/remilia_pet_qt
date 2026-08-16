// gamepadwindow.cpp — 手柄键位显示窗口实现（实验性功能）
// 手柄图在逻辑坐标 260×150 内绘制，随宠物缩放整体缩放；按下按键粉色高亮。
#include "gamepadwindow.h"
#include "petwindow.h"
#include "inappfiledialog.h" // PINK

#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QScreen>
#include <QGuiApplication>
#include <QStringList>
#include <cstring>

namespace
{
// 手柄图逻辑尺寸（scale=1 时）
constexpr int kPadW = 260;
constexpr int kPadH = 150;

// 单个元素绘制：按下 → 粉色填充 + 深色文字；未按下 → 深灰填充 + 灰边 + 浅灰文字
void drawButton(QPainter &p, const QRectF &r, bool pressed, const QString &text, double fontPx)
{
    if (pressed)
    {
        p.setBrush(QBrush(PINK));
        p.setPen(QPen(QColor(255, 255, 255), 2));
    }
    else
    {
        p.setBrush(QBrush(QColor(58, 58, 58)));
        p.setPen(QPen(QColor(90, 90, 90), 2));
    }
    p.drawRoundedRect(r, r.height() / 3.0, r.height() / 3.0);
    if (!text.isEmpty())
    {
        QFont f = p.font();
        f.setPixelSize(qMax(6, static_cast<int>(fontPx)));
        f.setBold(true);
        p.setFont(f);
        p.setPen(pressed ? QColor(26, 26, 26) : QColor(154, 154, 154));
        p.drawText(r, Qt::AlignCenter, text);
    }
}

void drawCircle(QPainter &p, const QPointF &c, double radius, bool pressed, const QString &text, double fontPx)
{
    p.setBrush(QBrush(pressed ? QColor(PINK) : QColor(58, 58, 58)));
    p.setPen(QPen(pressed ? QColor(255, 255, 255) : QColor(90, 90, 90), 2));
    p.drawEllipse(c, radius, radius);
    if (!text.isEmpty())
    {
        QFont f = p.font();
        f.setPixelSize(qMax(6, static_cast<int>(fontPx)));
        f.setBold(true);
        p.setFont(f);
        p.setPen(pressed ? QColor(26, 26, 26) : QColor(154, 154, 154));
        p.drawText(QRectF(c.x() - radius, c.y() - radius, radius * 2, radius * 2), Qt::AlignCenter, text);
    }
}
} // namespace

GamepadWindow::GamepadWindow(DesktopPet *pet, float scale, bool stayOnTop)
    : QWidget(nullptr, Qt::FramelessWindowHint | (stayOnTop ? Qt::WindowStaysOnTopHint : Qt::WindowType(0)) | Qt::Tool), m_pet(pet), m_scale(scale)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    m_label = new QLabel(this);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    m_hideTimer->setInterval(2000); // 2 秒无输入自动隐藏
    connect(m_hideTimer, &QTimer::timeout, this, &QWidget::hide);
    updateScaleAndPosition(scale);
}

void GamepadWindow::showState(const GamepadState &state)
{
    if (!state.connected)
    {
        m_state = state;
        hide(); // 手柄断开：立即隐藏
        return;
    }
    bool anyPressed = false;
    for (int i = 0; i < GP_COUNT; ++i)
        anyPressed = anyPressed || state.buttons[i];
    const bool changed = memcmp(state.buttons, m_state.buttons, sizeof(state.buttons)) != 0 || state.controllerIndex != m_state.controllerIndex;
    m_state = state;
    if (changed)
    {
        buildKeyText(state);
        positionNearPet(); // 隐藏期间宠物可能移动/缩放，显示前重新贴靠
        show();
        raise();
        update();
        m_hideTimer->start();
        // 触发全员重排：音乐窗等立即避开本窗，避免最长 800ms 的重叠空窗
        m_pet->updateSideWindowPositions();
    }
    else if (anyPressed)
    {
        m_hideTimer->start(); // 按住不放视为持续输入，窗口保持显示
    }
}

void GamepadWindow::updateScaleAndPosition(float scale)
{
    m_scale = scale;
    const int padW = qMax(60, static_cast<int>(kPadW * scale));
    const int padH = qMax(40, static_cast<int>(kPadH * scale));
    const int labelH = qMax(24, static_cast<int>(26 * scale));
    const int fs = qMax(14, static_cast<int>(17 * scale));
    m_label->setStyleSheet(QString("color:#FF8DA1;font-weight:bold;font-size:%1px;background:transparent;").arg(fs));
    setFixedSize(padW + 16, padH + labelH + 24);
    m_label->setGeometry(8, padH + 8, padW, labelH);
    positionNearPet();
}

void GamepadWindow::positionNearPet()
{
    if (!m_pet)
        return;
    const QRect pr = m_pet->geometry();
    // 与键盘气泡同排：键盘气泡居中于宠物上方，手柄窗贴其左侧（间隔 8px）；
    // 键盘气泡未显示时手柄窗占键盘位居中（与键位显示一致，不偏移）
    int keyW = 0;
    if (QWidget *kw = m_pet->m_keyWindow.data())
    {
        if (kw->isVisible())
            keyW = kw->width();
    }
    int x;
    if (keyW > 0)
        x = pr.x() + pr.width() / 2 - keyW / 2 - 8 - width();
    else
        x = pr.x() + (pr.width() - width()) / 2;
    int y = pr.y() - height() - 10;
    QScreen *sc = QGuiApplication::screenAt(pr.center());
    if (sc)
    {
        const QRect avail = sc->availableGeometry();
        // 上方无空间时移到宠物下方（不遮挡宠物）
        if (y < avail.top())
            y = pr.y() + pr.height() + 10;
        x = qBound(avail.left(), x, avail.left() + avail.width() - width());
        y = qBound(avail.top(), y, avail.top() + avail.height() - height());
    }
    move(x, y);
}

void GamepadWindow::buildKeyText(const GamepadState &state)
{
    QStringList parts;
    if (state.buttons[GP_A])
        parts << QStringLiteral("A");
    if (state.buttons[GP_B])
        parts << QStringLiteral("B");
    if (state.buttons[GP_X])
        parts << QStringLiteral("X");
    if (state.buttons[GP_Y])
        parts << QStringLiteral("Y");
    if (state.buttons[GP_DPAD_UP])
        parts << QString::fromUtf8("\u2191");
    if (state.buttons[GP_DPAD_DOWN])
        parts << QString::fromUtf8("\u2193");
    if (state.buttons[GP_DPAD_LEFT])
        parts << QString::fromUtf8("\u2190");
    if (state.buttons[GP_DPAD_RIGHT])
        parts << QString::fromUtf8("\u2192");
    if (state.buttons[GP_LB])
        parts << QStringLiteral("LB");
    if (state.buttons[GP_RB])
        parts << QStringLiteral("RB");
    if (state.buttons[GP_LT])
        parts << QStringLiteral("LT");
    if (state.buttons[GP_RT])
        parts << QStringLiteral("RT");
    if (state.buttons[GP_L3])
        parts << QStringLiteral("L3");
    if (state.buttons[GP_R3])
        parts << QStringLiteral("R3");
    if (state.buttons[GP_START])
        parts << QStringLiteral("Start");
    if (state.buttons[GP_BACK])
        parts << QStringLiteral("Back");
    if (state.buttons[GP_LS_UP])
        parts << QString::fromUtf8("L\u2191");
    if (state.buttons[GP_LS_DOWN])
        parts << QString::fromUtf8("L\u2193");
    if (state.buttons[GP_LS_LEFT])
        parts << QString::fromUtf8("L\u2190");
    if (state.buttons[GP_LS_RIGHT])
        parts << QString::fromUtf8("L\u2192");
    if (state.buttons[GP_RS_UP])
        parts << QString::fromUtf8("R\u2191");
    if (state.buttons[GP_RS_DOWN])
        parts << QString::fromUtf8("R\u2193");
    if (state.buttons[GP_RS_LEFT])
        parts << QString::fromUtf8("R\u2190");
    if (state.buttons[GP_RS_RIGHT])
        parts << QString::fromUtf8("R\u2192");
    m_keyText = parts.isEmpty() ? QString::fromUtf8("\u624B\u67C4\u5DF2\u8FDE\u63A5") : parts.join(QStringLiteral(" \u00B7 "));
    m_label->setText(m_keyText);
}

void GamepadWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    // 窗口背景：与键位气泡一致的深色圆角 + 粉色描边
    QRectF bg(2.0, 2.0, width() - 4.0, height() - 4.0);
    p.setBrush(QBrush(QColor(26, 26, 26, 255)));
    p.setPen(QPen(PINK, 4));
    p.drawRoundedRect(bg, 10, 10);

    // 手柄图：逻辑坐标 260×150，起点 (8,8)，随 m_scale 缩放
    p.save();
    p.translate(8.0, 8.0);
    p.scale(m_scale, m_scale);
    const GamepadState &st = m_state;

    // 扳机 + 肩键（顶部）
    drawButton(p, QRectF(10, 6, 40, 16), st.buttons[GP_LT], QStringLiteral("LT"), 7);
    drawButton(p, QRectF(210, 6, 40, 16), st.buttons[GP_RT], QStringLiteral("RT"), 7);
    drawButton(p, QRectF(10, 22, 62, 20), st.buttons[GP_LB], QStringLiteral("LB"), 8);
    drawButton(p, QRectF(188, 22, 62, 20), st.buttons[GP_RB], QStringLiteral("RB"), 8);

    // 机身
    p.setBrush(QBrush(QColor(46, 46, 46)));
    p.setPen(QPen(PINK, 2));
    p.drawRoundedRect(QRectF(5, 42, 250, 103), 34, 34);

    // 方向键（十字）
    drawButton(p, QRectF(42, 62, 20, 16), st.buttons[GP_DPAD_UP], QString(), 0);
    drawButton(p, QRectF(42, 96, 20, 16), st.buttons[GP_DPAD_DOWN], QString(), 0);
    drawButton(p, QRectF(24, 80, 16, 20), st.buttons[GP_DPAD_LEFT], QString(), 0);
    drawButton(p, QRectF(64, 80, 16, 20), st.buttons[GP_DPAD_RIGHT], QString(), 0);

    // 摇杆：底座圆 + 方向偏移的内点（按摇杆方向移动，按下 L3/R3 内点变粉）
    auto drawStick = [&](const QPointF &c, int l, int r, int u, int d, bool click)
    {
        p.setBrush(QBrush(QColor(58, 58, 58)));
        p.setPen(QPen(QColor(90, 90, 90), 2));
        p.drawEllipse(c, 24, 24);
        double ox = 0.0;
        double oy = 0.0;
        if (st.buttons[l])
            ox -= 7.0;
        if (st.buttons[r])
            ox += 7.0;
        if (st.buttons[u])
            oy -= 7.0;
        if (st.buttons[d])
            oy += 7.0;
        p.setBrush(QBrush(click ? QColor(PINK) : QColor(154, 154, 154)));
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(c.x() + ox, c.y() + oy), 9, 9);
    };
    drawStick(QPointF(104, 92), GP_LS_LEFT, GP_LS_RIGHT, GP_LS_UP, GP_LS_DOWN, st.buttons[GP_L3]);
    drawStick(QPointF(156, 92), GP_RS_LEFT, GP_RS_RIGHT, GP_RS_UP, GP_RS_DOWN, st.buttons[GP_R3]);

    // Back / Start
    drawButton(p, QRectF(112, 48, 16, 8), st.buttons[GP_BACK], QString(), 0);
    drawButton(p, QRectF(140, 48, 16, 8), st.buttons[GP_START], QString(), 0);

    // ABXY 菱形排布
    drawCircle(p, QPointF(222, 104), 11, st.buttons[GP_A], QStringLiteral("A"), 9);
    drawCircle(p, QPointF(240, 86), 11, st.buttons[GP_B], QStringLiteral("B"), 9);
    drawCircle(p, QPointF(204, 86), 11, st.buttons[GP_X], QStringLiteral("X"), 9);
    drawCircle(p, QPointF(222, 68), 11, st.buttons[GP_Y], QStringLiteral("Y"), 9);

    p.restore();
}
