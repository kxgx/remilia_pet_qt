// inappfiledialog.h — 内嵌文件浏览器 InAppFileDialog（纯 Qt，零外部依赖）
// 替代系统文件管理器：浏览/导入/删除/重命名/新建/搜索/多选批量操作；
// 文件变更后发 filesChanged 信号，桌宠据此即时重载资源（openDirInFM 支持回调）。
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
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
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
#include <QStringList>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPen>
#include <QMouseEvent>
#include <functional>

extern const QColor PINK;

class DropHighlightTree;

class InAppFileDialog : public QDialog {
    Q_OBJECT
public:
    explicit InAppFileDialog(const QString &dirPath, QWidget *parent = nullptr);

signals:
    // Emitted after files were imported/deleted/renamed/created in this dialog
    void filesChanged();

protected:
    void paintEvent(QPaintEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *) override;
    void dragMoveEvent(QDragMoveEvent *) override;
    void dragLeaveEvent(QDragLeaveEvent *) override;
    void dropEvent(QDropEvent *) override;

private slots:
    void onDoubleClicked(const QModelIndex &idx);
    void onCustomContextMenu(const QPoint &pos);
    void navigateUp();
    void navigateBack();
    void navigateForward();
    void navigateHome();
    void deleteSelected();
    void renameSelected();
    void newFolder();
    void newFile();
    void refreshView();
    void startPathEdit();
    void applyPathEdit();
    void onFilterChanged(const QString &text);
    void onSelectionChanged();

private:
    void setupUi(const QString &dirPath);
    QString currentPath() const;
    QStringList selectedPaths() const;
    QModelIndex dropTargetIndex(const QPoint &viewPos) const;
    void navigateTo(const QString &path);
    void updatePathDisplay();
    void updateNavButtons();
    void updateStatus();
    void createItem(bool folder);
    bool copyPath(const QString &src, const QString &dst);

    QFileSystemModel *m_model = nullptr;
    DropHighlightTree *m_tree = nullptr;
    QLabel *m_pathLabel = nullptr;
    QLineEdit *m_pathEdit = nullptr;
    QLineEdit *m_filterEdit = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_backBtn = nullptr;
    QPushButton *m_forwardBtn = nullptr;
    QString m_homeDir;
    QStringList m_backStack;
    QStringList m_forwardStack;
};

void openDirInFM(const QString &dirPath, std::function<void()> onFilesChanged = nullptr);

#endif // INAPPFILEDIALOG_H
