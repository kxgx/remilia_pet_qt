#ifndef INAPPFILEDIALOG_H
#define INAPPFILEDIALOG_H

#include <QDialog>
#include <QFileSystemModel>
#include <QTreeView>
#include <QColor>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QKeyEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMenu>
#include <QMimeData>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QDesktopServices>
#include <QInputDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPen>
#include <QMouseEvent>

extern const QColor PINK;

class InAppFileDialog : public QDialog {
    Q_OBJECT
public:
    explicit InAppFileDialog(const QString &dirPath, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onDoubleClicked(const QModelIndex &idx);
    void onCustomContextMenu(const QPoint &pos);
    void navigateUp();
    void deleteSelected();
    void renameSelected();
    void refreshView();
    void startPathEdit();
    void applyPathEdit();

private:
    void setupUi(const QString &dirPath);
    void dragEnterEvent(QDragEnterEvent *) override;
    void dropEvent(QDropEvent *) override;
    QString selectedFilePath() const;
    QString currentPath() const;
    void updatePathDisplay();

    QFileSystemModel *m_model = nullptr;
    QTreeView *m_tree = nullptr;
    QLabel *m_pathLabel = nullptr;
    QLineEdit *m_pathEdit = nullptr;
};

void openDirInFM(const QString &dirPath);

#endif // INAPPFILEDIALOG_H
