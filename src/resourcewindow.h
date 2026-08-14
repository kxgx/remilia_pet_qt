// resourcewindow.h — 资源替换窗口 ResourceWindow
// 管理所有内置资源的覆盖替换：单个替换/全部替换/打开目录/删除，状态显示「已替换/默认」。
#ifndef RESOURCEWINDOW_H
#define RESOURCEWINDOW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QList>

class DesktopPet;

class ResourceWindow : public QWidget
{
    Q_OBJECT
    public:
    ResourceWindow(DesktopPet *pet, float scale);

    void updateScaleAndPosition(float scale);
    // Refresh all row status labels/enabled states from current override state
    void refreshAllRows();

    protected:
    void paintEvent(QPaintEvent *) override;

    private:
    struct ResourceDef
    {
        QString key;
        QString name;
        QString category;
        QString subDir;
        QString ext;
    };

    void initUi();
    void copyFileToOverride(const QString &src, const QString &subDir, const QString &baseName, const QString &ext);
    void refreshRow(QLabel *sl, QPushButton *db, const QString &key);
    void onReplaceAll();
    void onOpenAll();
    void closeResourceWindow();
    void positionNearPet();

    DesktopPet *m_pet;
    float m_scale;
    QVBoxLayout *m_mainLayout = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_hintLabel = nullptr;
    QPushButton *m_closeBtn = nullptr;
    QPushButton *m_replaceAllBtn = nullptr;
    QPushButton *m_openAllBtn = nullptr;
    QListWidget *m_resList = nullptr;
    QList<QPushButton *> m_allBtns;
};

#endif // RESOURCEWINDOW_H
