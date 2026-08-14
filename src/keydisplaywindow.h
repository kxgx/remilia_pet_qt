// keydisplaywindow.h — 键盘键位显示窗口
// 粉色圆角边框 + 深色底，显示当前按下的键，2 秒无输入自动隐藏，随宠物缩放/贴靠。
#ifndef KEYDISPLAYWINDOW_H
#define KEYDISPLAYWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QTimer>

class DesktopPet;

class KeyDisplayWindow : public QWidget {
    Q_OBJECT
public:
    KeyDisplayWindow(DesktopPet *pet, float scale, bool stayOnTop);

    void showKey(const QString &text);
    void updateScaleAndPosition(float scale);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    void positionNearPet();

    DesktopPet *m_pet;
    float m_scale;
    QLabel *m_label = nullptr;
    QTimer *m_hideTimer = nullptr;
};

#endif // KEYDISPLAYWINDOW_H
