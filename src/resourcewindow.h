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

class ResourceWindow : public QWidget {
    Q_OBJECT
public:
    ResourceWindow(DesktopPet *pet, float scale);

    void updateScaleAndPosition(float scale);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    struct ResourceDef { QString key; QString name; QString category; QString subDir; QString ext; };

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
    QPushButton *m_closeBtn = nullptr;
    QPushButton *m_replaceAllBtn = nullptr;
    QPushButton *m_openAllBtn = nullptr;
    QListWidget *m_resList = nullptr;
    QList<QPushButton*> m_allBtns;
};

#endif // RESOURCEWINDOW_H
