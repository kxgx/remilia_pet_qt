// gamepadwindow.h — 手柄键位显示窗口（实验性功能）
// 无边框深色圆角窗：QPainter 程序绘制手柄轮廓与按键，按下高亮（粉色），
// 下方文字气泡列出当前按下的键名；2 秒无输入自动隐藏；随宠物缩放/贴靠，与键盘气泡并排。
#ifndef GAMEPADWINDOW_H
#define GAMEPADWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include "gamepad.h"

class DesktopPet;

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
    void buildKeyText(const GamepadState &state);

    DesktopPet *m_pet;
    float m_scale;
    QLabel *m_label = nullptr;
    QTimer *m_hideTimer = nullptr;
    GamepadState m_state;
    QString m_keyText;
};

#endif // GAMEPADWINDOW_H
