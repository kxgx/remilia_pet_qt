// gamepadwindow.h — 手柄键位显示窗口（实验性功能）
// GamepadWindow：无边框深色圆角窗，QPainter 程序绘制手柄轮廓与按键，按下高亮（粉色）；
// GamepadKeyWindow：独立粉色文字气泡，列出当前按下的键名（贴手柄图左侧同行、底部对齐）；
// 两窗均 2 秒无输入自动隐藏，随宠物缩放/贴靠，与键盘气泡同行排列。
#ifndef GAMEPADWINDOW_H
#define GAMEPADWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QFont>
#include "gamepad.h"

class DesktopPet;

// 手柄状态 → 按键名文本（无输入且已连接时返回"手柄已连接"）
QString gamepadStateText(const GamepadState &state);

class GamepadWindow : public QWidget
{
    Q_OBJECT
    public:
    GamepadWindow(DesktopPet *pet, float scale, bool stayOnTop);

    void showState(const GamepadState &state);
    void updateScaleAndPosition(float scale);

    protected:
    void paintEvent(QPaintEvent *) override;

    private:
    void positionNearPet();

    DesktopPet *m_pet;
    float m_scale;
    QTimer *m_hideTimer = nullptr;
    GamepadState m_state;
};

class GamepadKeyWindow : public QWidget
{
    Q_OBJECT
    public:
    GamepadKeyWindow(DesktopPet *pet, float scale, bool stayOnTop);

    void showState(const GamepadState &state);
    void showDisconnectedNotice(); // 显示"手柄未连接"2 秒后自动隐藏（断开/无手柄提示）
    void showConnectedNotice();    // 显示"手柄已连接"2 秒后自动隐藏（仅连接瞬间一次）
    void updateScaleAndPosition(float scale);

    protected:
    void paintEvent(QPaintEvent *) override;

    private:
    void positionNearPet();
    QFont labelFont() const; // 与标签 QSS 一致的字体（QSS 不影响 widget->font()）
    void relayoutLabel();    // 按文本自然宽度自适应窗口尺寸，超长省略

    DesktopPet *m_pet;
    float m_scale;
    QLabel *m_label = nullptr;
    QTimer *m_hideTimer = nullptr;
    GamepadState m_state;
    QString m_text; // 完整文本（标签显示省略后版本，重新测量时用）
};

#endif // GAMEPADWINDOW_H
