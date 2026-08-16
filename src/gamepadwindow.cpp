// gamepadwindow.cpp — 手柄键位显示窗口实现（实验性功能）
// GamepadWindow：手柄图在逻辑坐标 260×150 内绘制，随宠物缩放整体缩放，按下按键粉色高亮；
// GamepadKeyWindow：独立文字气泡（粉色圆角边框），贴手柄图左侧同行、底部对齐，列出按下键名。
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

// 手柄状态 → 按键名文本（与手柄图共用同一状态源，气泡独立显示）
QString gamepadStateText(const GamepadState &state)
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
    return parts.isEmpty() ? QString::fromUtf8("\u624B\u67C4\u5DF2\u8FDE\u63A5") : parts.join(QStringLiteral(" \u00B7 "));
}

// ── GamepadWindow：手柄图 ───────────────────────────────────────────

GamepadWindow::GamepadWindow(DesktopPet *pet, float scale, bool stayOnTop)
    : QWidget(nullptr, Qt::FramelessWindowHint | (stayOnTop ? Qt::WindowStaysOnTopHint : Qt::WindowType(0)) | Qt::Tool), m_pet(pet), m_scale(scale)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
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
        positionNearPet(); // 隐藏期间宠物可能移动/缩放，显示前重新贴靠
        show();
        raise();
        update();
        m_hideTimer->start();
        // 触发全员重排：文字气泡/音乐窗等立即避开本窗，避免最长 800ms 的重叠空窗
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
    setFixedSize(padW + 16, padH + 16);
    positionNearPet();
}

void GamepadWindow::positionNearPet()
{
    if (!m_pet)
        return;
    const QRect pr = m_pet->geometry();
    QScreen *sc = QGuiApplication::screenAt(pr.center());
    const QRect avail = sc ? sc->availableGeometry() : QRect();
    int keyW = 0;
    if (QWidget *kw = m_pet->m_keyWindow.data())
    {
        if (kw->isVisible())
            keyW = kw->width();
    }

    auto inAvail = [&avail](const QRect &r)
    {
        if (!avail.isValid())
            return true;
        return r.left() >= avail.left() && r.top() >= avail.top() && r.right() <= avail.left() + avail.width() - 1 &&
               r.bottom() <= avail.top() + avail.height() - 1;
    };

    // 槽位依次避让：键盘气泡左侧同行 → 键盘气泡右侧同行 → 宠物下方居中（
    // 杜绝独立钳位造成的互相压叠）
    QRect cand;
    if (keyW > 0)
    {
        cand = QRect(pr.x() + pr.width() / 2 - keyW / 2 - 8 - width(), pr.y() - height() - 10, width(), height());
        if (!inAvail(cand))
            cand = QRect(pr.x() + pr.width() / 2 + keyW / 2 + 8, pr.y() - height() - 10, width(), height());
    }
    else
    {
        cand = QRect(pr.x() + (pr.width() - width()) / 2, pr.y() - height() - 10, width(), height());
    }
    if (!inAvail(cand))
        cand = QRect(pr.x() + (pr.width() - width()) / 2, pr.y() + pr.height() + 10, width(), height());
    if (!inAvail(cand) && avail.isValid())
    {
        cand.moveTo(qBound(avail.left(), cand.x(), avail.left() + avail.width() - width()),
                    qBound(avail.top(), cand.y(), avail.top() + avail.height() - height()));
    }
    move(cand.topLeft());
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

// ── GamepadKeyWindow：独立文字气泡 ─────────────────────────────────

GamepadKeyWindow::GamepadKeyWindow(DesktopPet *pet, float scale, bool stayOnTop)
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

void GamepadKeyWindow::showState(const GamepadState &state)
{
    if (!state.connected)
    {
        if (m_state.connected)
        {
            // 在线→离线切换：显示"手柄未连接"2 秒后自动隐藏
            m_state = state;
            showDisconnectedNotice();
        }
        else
        {
            m_state = state;
            hide();
        }
        return;
    }
    bool anyPressed = false;
    for (int i = 0; i < GP_COUNT; ++i)
        anyPressed = anyPressed || state.buttons[i];
    const bool changed = memcmp(state.buttons, m_state.buttons, sizeof(state.buttons)) != 0 || state.controllerIndex != m_state.controllerIndex;
    m_state = state;
    if (changed)
    {
        m_text = gamepadStateText(state);
        relayoutLabel();   // 气泡宽度随内容自适应
        positionNearPet(); // 隐藏期间宠物可能移动/缩放，显示前重新贴靠
        show();
        raise();
        m_hideTimer->start();
        m_pet->updateSideWindowPositions(); // 音乐窗等立即避开本气泡
    }
    else if (anyPressed)
    {
        m_hideTimer->start(); // 按住不放视为持续输入，气泡保持显示
    }
}

void GamepadKeyWindow::showDisconnectedNotice()
{
    m_text = QString::fromUtf8("\u672A\u8FDE\u63A5"); // 未连接
    relayoutLabel();
    positionNearPet();
    show();
    raise();
    m_hideTimer->start();
}

void GamepadKeyWindow::updateScaleAndPosition(float scale)
{
    m_scale = scale;
    const int fs = qMax(16, static_cast<int>(36 * scale));
    m_label->setStyleSheet(QString("color:#FF8DA1;font-weight:bold;font-size:%1px;background:transparent;").arg(fs));
    relayoutLabel();
    positionNearPet();
}

QFont GamepadKeyWindow::labelFont() const
{
    // QSS 设置的 font-size 不反映在 widget->font()，这里构造与标签 QSS 一致的字体做测量
    QFont f = m_label->font();
    f.setPixelSize(qMax(16, static_cast<int>(36 * m_scale)));
    f.setBold(true);
    return f;
}

// 气泡宽度随文本自然宽度自适应：下限 80（x scale），上限与手柄图同宽（260 x scale），超长省略
void GamepadKeyWindow::relayoutLabel()
{
    const QFontMetrics fm(labelFont());
    const int minW = qMax(60, static_cast<int>(80 * m_scale));
    const int maxW = qMax(minW, static_cast<int>(kPadW * m_scale));
    const int labelW = qBound(minW, fm.horizontalAdvance(m_text) + 20, maxW);
    const int labelH = qMax(44, static_cast<int>(60 * m_scale));
    m_label->setFixedSize(labelW, labelH);
    setFixedSize(labelW + 16, labelH + 16);
    m_label->move(8, 8);
    m_label->setText(fm.elidedText(m_text, Qt::ElideRight, qMax(12, labelW - 4)));
}

void GamepadKeyWindow::positionNearPet()
{
    if (!m_pet)
        return;
    const QRect pr = m_pet->geometry();
    QScreen *sc = QGuiApplication::screenAt(pr.center());
    const QRect avail = sc ? sc->availableGeometry() : QRect();
    QRect padRect;
    if (QWidget *pw = m_pet->m_gamepadWindow.data())
    {
        if (pw->isVisible())
            padRect = pw->geometry();
    }
    QRect keyRect;
    if (QWidget *kw = m_pet->m_keyWindow.data())
    {
        if (kw->isVisible())
            keyRect = kw->geometry();
    }

    // 候选位置：不压宠物、不压手柄图、不与键盘气泡重叠（外扩 8px 视为重叠，避免贴死）
    auto fits = [&](const QRect &r)
    {
        if (r.intersects(pr))
            return false;
        if (padRect.isValid() && r.intersects(padRect))
            return false;
        if (keyRect.isValid() && r.intersects(keyRect.adjusted(-8, -8, 8, 8)))
            return false;
        if (avail.isValid())
            return r.left() >= avail.left() && r.top() >= avail.top() && r.right() <= avail.left() + avail.width() - 1 &&
                   r.bottom() <= avail.top() + avail.height() - 1;
        return true;
    };

    QRect cand;
    if (padRect.isValid())
    {
        // 槽位依次避让：手柄图左侧（底部对齐）→ 右侧 → 上方 → 下方
        cand = QRect(padRect.x() - width() - 8, padRect.y() + padRect.height() - height(), width(), height());
        if (fits(cand))
        {
            move(cand.topLeft());
            return;
        }
        cand = QRect(padRect.x() + padRect.width() + 8, padRect.y() + padRect.height() - height(), width(), height());
        if (fits(cand))
        {
            move(cand.topLeft());
            return;
        }
        cand = QRect(padRect.x(), padRect.y() - height() - 8, width(), height());
        if (fits(cand))
        {
            move(cand.topLeft());
            return;
        }
        cand = QRect(padRect.x(), padRect.y() + padRect.height() + 8, width(), height());
        if (fits(cand))
        {
            move(cand.topLeft());
            return;
        }
    }
    else
    {
        // 手柄图未显示：居中于宠物上方 → 下方
        cand = QRect(pr.x() + (pr.width() - width()) / 2, pr.y() - height() - 10, width(), height());
        if (fits(cand))
        {
            move(cand.topLeft());
            return;
        }
        cand = QRect(pr.x() + (pr.width() - width()) / 2, pr.y() + pr.height() + 10, width(), height());
        if (fits(cand))
        {
            move(cand.topLeft());
            return;
        }
    }
    // 兜底钳位
    if (avail.isValid())
    {
        move(qBound(avail.left(), cand.x(), avail.left() + avail.width() - width()),
             qBound(avail.top(), cand.y(), avail.top() + avail.height() - height()));
    }
    else
    {
        move(cand.topLeft());
    }
}

void GamepadKeyWindow::paintEvent(QPaintEvent *)
{
    // 与键盘气泡一致的粉色圆角边框 + 深色底
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QRectF r(2.0, 2.0, width() - 4.0, height() - 4.0);
    p.setBrush(QBrush(QColor(26, 26, 26, 255)));
    p.setPen(QPen(PINK, 4));
    p.drawRoundedRect(r, 10, 10);
}
