// keydisplaywindow.h — 键盘键位显示窗口
// 粉色圆角边框 + 深色底，显示当前按下的键，2 秒无输入自动隐藏，随宠物缩放/贴靠。
#ifndef KEYDISPLAYWINDOW_H
#define KEYDISPLAYWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QFont>

class DesktopPet;

class KeyDisplayWindow : public QWidget
{
    Q_OBJECT
    public:
    KeyDisplayWindow(DesktopPet *pet, float scale, bool stayOnTop);

    void showKey(const QString &text);
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
    QString m_text; // 完整文本（标签显示省略后版本，重新测量时用）
};

#endif // KEYDISPLAYWINDOW_H
