#include "inappfiledialog.h"

#include <QPointer>
#include <QHeaderView>
#include <QDirIterator>
#include <QItemSelectionModel>

const QColor PINK(255, 141, 161);

// Tree view that paints a translucent pink band over the drop target row while dragging.
class DropHighlightTree : public QTreeView {
public:
    using QTreeView::QTreeView;

    void setDropTarget(const QModelIndex &idx)
    {
        if (m_dropTarget == idx) return;
        QModelIndex old = m_dropTarget;
        m_dropTarget = idx;
        if (old.isValid()) viewport()->update(visualRect(old));
        if (m_dropTarget.isValid()) viewport()->update(visualRect(m_dropTarget));
    }

    void clearDropTarget() { setDropTarget(QModelIndex()); }

protected:
    void drawRow(QPainter *painter, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override
    {
        if (m_dropTarget.isValid() && idx == m_dropTarget) {
            painter->save();
            painter->fillRect(opt.rect, QColor(255, 141, 161, 70));
            painter->restore();
        }
        QTreeView::drawRow(painter, opt, idx);
    }

private:
    QModelIndex m_dropTarget;
};

InAppFileDialog::InAppFileDialog(const QString &dirPath, QWidget *parent)
    : QDialog(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
    , m_homeDir(dirPath)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(QString::fromUtf8("\u6587\u4ef6\u6d4f\u89c8")); // 文件浏览
    setupUi(dirPath);
}

void InAppFileDialog::setupUi(const QString &dirPath)
{
    resize(680, 480);
    setAcceptDrops(true);

    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(2, 2, 2, 2);

    QWidget *inner = new QWidget();
    inner->setStyleSheet("background:#1a1a1a;border-radius:8px;");
    QVBoxLayout *lay = new QVBoxLayout(inner);
    lay->setContentsMargins(8, 8, 8, 8);
    lay->setSpacing(4);

    // --- Title bar ---
    QHBoxLayout *titleBar = new QHBoxLayout();
    QLabel *titleLabel = new QLabel(QString::fromUtf8("\u6587\u4ef6\u6d4f\u89c8")); // 文件浏览
    titleLabel->setStyleSheet("color:#FF8DA1;font-weight:bold;font-size:13px;");

    QPushButton *closeBtn = new QPushButton(QString::fromUtf8("\u2715"));
    closeBtn->setFixedSize(20, 20);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet("QPushButton{background:transparent;color:#FF8DA1;border:1px solid #FF8DA1;border-radius:10px;font-weight:bold;font-size:10px;}QPushButton:hover{background:#FF8DA1;color:#111;}");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
    titleBar->addWidget(titleLabel);
    titleBar->addStretch();
    titleBar->addWidget(closeBtn);

    // --- Toolbar ---
    auto makeToolBtn = [](const QString &text) {
        QPushButton *b = new QPushButton(text);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet("QPushButton{background:#333;color:#ddd;border:none;border-radius:3px;font-size:11px;padding:2px 6px;}QPushButton:hover{background:#555;}QPushButton:disabled{color:#666;background:#2a2a2a;}");
        return b;
    };

    QHBoxLayout *toolbar = new QHBoxLayout();
    toolbar->setSpacing(4);

    m_backBtn = makeToolBtn(QString::fromUtf8("\u2190 \u540e\u9000")); // ← 后退
    m_forwardBtn = makeToolBtn(QString::fromUtf8("\u2192 \u524d\u8fdb")); // → 前进
    QPushButton *homeBtn = makeToolBtn(QString::fromUtf8("\u2302 \u4e3b\u9875")); // ⌂ 主页
    QPushButton *upBtn = makeToolBtn(QString::fromUtf8("\u2190 \u5411\u4e0a")); // ← 向上
    QPushButton *refreshBtn = makeToolBtn(QString::fromUtf8("\u21bb \u5237\u65b0")); // ↻ 刷新
    QPushButton *newFolderBtn = makeToolBtn(QString::fromUtf8("\u65b0\u5efa\u6587\u4ef6\u5939")); // 新建文件夹
    QPushButton *newFileBtn = makeToolBtn(QString::fromUtf8("\u65b0\u5efa\u6587\u4ef6")); // 新建文件

    toolbar->addWidget(m_backBtn);
    toolbar->addWidget(m_forwardBtn);
    toolbar->addWidget(homeBtn);
    toolbar->addSpacing(8);
    toolbar->addWidget(upBtn);
    toolbar->addWidget(refreshBtn);
    toolbar->addSpacing(8);
    toolbar->addWidget(newFolderBtn);
    toolbar->addWidget(newFileBtn);
    toolbar->addStretch();

    connect(m_backBtn, &QPushButton::clicked, this, &InAppFileDialog::navigateBack);
    connect(m_forwardBtn, &QPushButton::clicked, this, &InAppFileDialog::navigateForward);
    connect(homeBtn, &QPushButton::clicked, this, &InAppFileDialog::navigateHome);
    connect(upBtn, &QPushButton::clicked, this, &InAppFileDialog::navigateUp);
    connect(refreshBtn, &QPushButton::clicked, this, &InAppFileDialog::refreshView);
    connect(newFolderBtn, &QPushButton::clicked, this, &InAppFileDialog::newFolder);
    connect(newFileBtn, &QPushButton::clicked, this, &InAppFileDialog::newFile);

    // --- Path bar + filter ---
    QHBoxLayout *pathBar = new QHBoxLayout();
    pathBar->setSpacing(2);
    m_pathLabel = new QLabel(dirPath);
    m_pathLabel->setStyleSheet("color:#888;font-size:10px;padding:2px 4px;border:1px solid transparent;border-radius:2px;");
    m_pathLabel->setCursor(Qt::IBeamCursor);
    m_pathLabel->setWordWrap(false);
    m_pathLabel->setTextInteractionFlags(Qt::NoTextInteraction);
    m_pathLabel->installEventFilter(this); // click path label to edit

    m_pathEdit = new QLineEdit();
    m_pathEdit->setStyleSheet("background:#111;color:#ddd;border:1px solid #FF8DA1;border-radius:2px;font-size:10px;padding:1px 4px;");
    m_pathEdit->hide();
    connect(m_pathEdit, &QLineEdit::returnPressed, this, &InAppFileDialog::applyPathEdit);

    m_filterEdit = new QLineEdit();
    m_filterEdit->setFixedWidth(140);
    m_filterEdit->setPlaceholderText(QString::fromUtf8("\u641c\u7d22")); // 搜索
    m_filterEdit->setStyleSheet("background:#111;color:#888;border:1px solid #444;border-radius:2px;font-size:10px;padding:1px 4px;");
    connect(m_filterEdit, &QLineEdit::textChanged, this, &InAppFileDialog::onFilterChanged);

    pathBar->addWidget(m_pathLabel);
    pathBar->addWidget(m_pathEdit, 1);
    pathBar->addWidget(m_filterEdit);

    // --- File tree ---
    m_model = new QFileSystemModel(this);
    m_model->setRootPath(dirPath);
    m_model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);

    m_tree = new DropHighlightTree;
    m_tree->setModel(m_model);
    m_tree->setRootIndex(m_model->index(dirPath));
    m_tree->setHeaderHidden(false);
    m_tree->setSortingEnabled(true);
    m_tree->sortByColumn(0, Qt::AscendingOrder);
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tree->setDragDropMode(QAbstractItemView::NoDragDrop);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tree->setStyleSheet("QTreeView{background:#111;color:#ddd;border:1px solid #444;border-radius:4px;font-size:11px;}QTreeView::item:hover{background:#333;}QTreeView::item:selected{background:#FF8DA1;color:#111;}QHeaderView::section{background:#222;color:#FF8DA1;border:none;border-bottom:1px solid #444;padding:2px 6px;font-size:10px;}QScrollBar:vertical{width:5px;background:transparent;}QScrollBar::handle:vertical{background:#FF8DA1;border-radius:2px;min-height:15px;}QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}QScrollBar:horizontal{height:5px;background:transparent;}QScrollBar::handle:horizontal{background:#FF8DA1;border-radius:2px;}");

    connect(m_tree, &QTreeView::doubleClicked, this, &InAppFileDialog::onDoubleClicked);
    connect(m_tree, &QTreeView::customContextMenuRequested, this, &InAppFileDialog::onCustomContextMenu);
    connect(m_tree->selectionModel(), &QItemSelectionModel::selectionChanged, this, &InAppFileDialog::onSelectionChanged);
    connect(m_model, &QFileSystemModel::rowsInserted, this, [this]() { updateStatus(); });
    connect(m_model, &QFileSystemModel::rowsRemoved, this, [this]() { updateStatus(); });

    // --- Status bar ---
    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet("color:#888;font-size:10px;padding:1px 2px;");

    lay->addLayout(titleBar);
    lay->addLayout(toolbar);
    lay->addLayout(pathBar);
    lay->addWidget(m_tree, 1);
    lay->addWidget(m_statusLabel);
    outerLayout->addWidget(inner);

    updateNavButtons();
    updateStatus();
}

// --- Event overrides ---

void InAppFileDialog::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QRectF r(1.0, 1.0, width() - 2.0, height() - 2.0);
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(PINK, 2));
    p.drawRoundedRect(r, 10, 10);
}

void InAppFileDialog::keyPressEvent(QKeyEvent *event)
{
    // Don't steal keys while typing in the path/filter line edits.
    if (qobject_cast<QLineEdit *>(focusWidget())) {
        QDialog::keyPressEvent(event);
        return;
    }
    if (event->key() == Qt::Key_Delete) {
        deleteSelected();
        return;
    }
    if (event->key() == Qt::Key_Backspace) {
        navigateUp();
        return;
    }
    if (event->key() == Qt::Key_F2) {
        renameSelected();
        return;
    }
    if (event->key() == Qt::Key_F5) {
        refreshView();
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QModelIndex idx = m_tree->currentIndex();
        if (idx.isValid()) onDoubleClicked(idx);
        return;
    }
    if (event->modifiers() & Qt::AltModifier) {
        switch (event->key()) {
        case Qt::Key_Left: navigateBack(); return;
        case Qt::Key_Right: navigateForward(); return;
        case Qt::Key_Up: navigateUp(); return;
        default: break;
        }
    }
    if (event->key() == Qt::Key_Escape) {
        close();
        return;
    }
    QDialog::keyPressEvent(event);
}

bool InAppFileDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_pathLabel && event->type() == QEvent::MouseButtonPress) {
        startPathEdit();
        return true;
    }
    return QDialog::eventFilter(obj, event);
}

void InAppFileDialog::dragEnterEvent(QDragEnterEvent *event)
{
    if (!event->mimeData()->hasUrls()) return;
    event->acceptProposedAction();
    m_statusLabel->setText(QString::fromUtf8("\u91ca\u653e\u4ee5\u5bfc\u5165\u5230 %1").arg(currentPath()));
}

void InAppFileDialog::dragMoveEvent(QDragMoveEvent *event)
{
    if (!event->mimeData()->hasUrls()) {
        event->ignore();
        return;
    }
    QPoint viewPos = m_tree->viewport()->mapFrom(this, event->position().toPoint());
    QModelIndex target = dropTargetIndex(viewPos);
    m_tree->setDropTarget(target);
    m_statusLabel->setText(QString::fromUtf8("\u91ca\u653e\u4ee5\u5bfc\u5165\u5230 %1").arg(m_model->filePath(target)));
    event->acceptProposedAction();
}

void InAppFileDialog::dragLeaveEvent(QDragLeaveEvent *)
{
    m_tree->clearDropTarget();
    updateStatus();
}

void InAppFileDialog::dropEvent(QDropEvent *event)
{
    QPoint viewPos = m_tree->viewport()->mapFrom(this, event->position().toPoint());
    QModelIndex target = dropTargetIndex(viewPos);
    QString targetDir = m_model->filePath(target);
    m_tree->clearDropTarget();

    const QList<QUrl> urls = event->mimeData()->urls();
    int count = 0;
    for (const QUrl &url : urls) {
        if (!url.isLocalFile()) continue;
        QString src = url.toLocalFile();
        QFileInfo fi(src);
        QString dst = QDir(targetDir).absoluteFilePath(fi.fileName());
        // Dropped onto itself (e.g. a dir onto its own parent row): skip.
        if (fi.canonicalFilePath() == QFileInfo(dst).canonicalFilePath()) continue;
        if (QFile::exists(dst)) {
            QMessageBox::StandardButton btn = QMessageBox::question(this,
                QString::fromUtf8("\u786e\u8ba4\u66ff\u6362"), // 确认替换
                QString::fromUtf8("\u76ee\u6807\u6587\u4ef6\u5df2\u5b58\u5728:\n%1\n\n\u662f\u5426\u66ff\u6362?") // 目标文件已存在:\n%1\n\n是否替换?
                    .arg(fi.fileName()));
            if (btn != QMessageBox::Yes) continue;
            QDir(dst).removeRecursively();
            QFile::remove(dst);
        }
        bool ok = fi.isDir() ? copyPath(src, dst) : QFile::copy(src, dst);
        if (ok) count++;
    }
    if (count > 0) {
        refreshView();
        emit filesChanged();
        QMessageBox::information(this,
            QString::fromUtf8("\u5bfc\u5165\u6210\u529f"), // 导入成功
            QString::fromUtf8("\u5df2\u5bfc\u5165 %1 \u4e2a\u6587\u4ef6") // 已导入 %1 个文件
                .arg(count));
    }
    updateStatus();
    event->acceptProposedAction();
}

// --- Slots ---

void InAppFileDialog::onDoubleClicked(const QModelIndex &idx)
{
    QString fp = m_model->filePath(idx);
    if (fp.isEmpty()) return;
    QFileInfo fi(fp);
    if (fi.isDir()) {
        navigateTo(fp);
    } else {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fp));
    }
}

void InAppFileDialog::onCustomContextMenu(const QPoint &pos)
{
    QModelIndex idx = m_tree->indexAt(pos);
    QMenu menu(this);
    menu.setStyleSheet("QMenu{background:#1a1a1a;color:#ddd;border:1px solid #FF8DA1;padding:2px;}QMenu::item{padding:4px 12px;}QMenu::item:selected{background:#FF8DA1;color:#111;}");

    QAction *newFolderAct = menu.addAction(QString::fromUtf8("\u65b0\u5efa\u6587\u4ef6\u5939")); // 新建文件夹
    QAction *newFileAct = menu.addAction(QString::fromUtf8("\u65b0\u5efa\u6587\u4ef6")); // 新建文件
    menu.addSeparator();
    QAction *openAct = nullptr;
    QAction *deleteAct = nullptr;
    QAction *renameAct = nullptr;
    if (idx.isValid()) {
        openAct = menu.addAction(QString::fromUtf8("\u6253\u5f00")); // 打开
        menu.addSeparator();
        deleteAct = menu.addAction(QString::fromUtf8("\u5220\u9664")); // 删除
        renameAct = menu.addAction(QString::fromUtf8("\u91cd\u547d\u540d")); // 重命名
    }
    menu.addSeparator();
    QAction *refreshAct = menu.addAction(QString::fromUtf8("\u5237\u65b0")); // 刷新

    QAction *chosen = menu.exec(m_tree->viewport()->mapToGlobal(pos));
    if (!chosen) return;
    if (chosen == newFolderAct) {
        newFolder();
    } else if (chosen == newFileAct) {
        newFile();
    } else if (chosen == openAct) {
        if (idx.isValid()) onDoubleClicked(idx);
    } else if (chosen == deleteAct) {
        deleteSelected();
    } else if (chosen == renameAct) {
        renameSelected();
    } else if (chosen == refreshAct) {
        refreshView();
    }
}

void InAppFileDialog::navigateTo(const QString &path)
{
    QFileInfo fi(path);
    if (!fi.isDir()) return;
    QString abs = fi.absoluteFilePath();
    QString cur = currentPath();
    if (!cur.isEmpty() && QFileInfo(cur).canonicalFilePath() == fi.canonicalFilePath()) return;
    m_backStack.append(cur);
    m_forwardStack.clear();
    m_model->setRootPath(abs);
    m_tree->setRootIndex(m_model->index(abs));
    updatePathDisplay();
    updateNavButtons();
    updateStatus();
}

void InAppFileDialog::navigateUp()
{
    QModelIndex rootIdx = m_tree->rootIndex();
    QModelIndex parentIdx = rootIdx.parent();
    QString parent;
    if (parentIdx.isValid()) {
        parent = m_model->filePath(parentIdx);
    } else {
        QFileInfo fi(m_model->filePath(rootIdx));
        parent = fi.path();
        if (parent == fi.absoluteFilePath()) return; // already at filesystem root
    }
    navigateTo(parent);
}

void InAppFileDialog::navigateBack()
{
    if (m_backStack.isEmpty()) return;
    m_forwardStack.append(currentPath());
    QString path = m_backStack.takeLast();
    m_model->setRootPath(path);
    m_tree->setRootIndex(m_model->index(path));
    updatePathDisplay();
    updateNavButtons();
    updateStatus();
}

void InAppFileDialog::navigateForward()
{
    if (m_forwardStack.isEmpty()) return;
    m_backStack.append(currentPath());
    QString path = m_forwardStack.takeLast();
    m_model->setRootPath(path);
    m_tree->setRootIndex(m_model->index(path));
    updatePathDisplay();
    updateNavButtons();
    updateStatus();
}

void InAppFileDialog::navigateHome()
{
    navigateTo(m_homeDir);
}

void InAppFileDialog::deleteSelected()
{
    QStringList paths = selectedPaths();
    if (paths.isEmpty()) return;

    // Never delete the directory currently shown as root (view would break).
    QString curCanonical = QFileInfo(currentPath()).canonicalFilePath();
    QStringList toDelete;
    for (const QString &p : paths) {
        QFileInfo fi(p);
        if (!curCanonical.isEmpty() && fi.canonicalFilePath() == curCanonical) {
            QMessageBox::warning(this,
                QString::fromUtf8("\u5220\u9664\u5931\u8d25"), // 删除失败
                QString::fromUtf8("\u65e0\u6cd5\u5220\u9664\u5f53\u524d\u76ee\u5f55: %1") // 无法删除当前目录: %1
                    .arg(fi.fileName()));
            continue;
        }
        toDelete << p;
    }
    if (toDelete.isEmpty()) return;

    QStringList names;
    for (const QString &p : toDelete) names << QFileInfo(p).fileName();
    QString detail = names.join(QStringLiteral(", "));
    if (names.size() > 5)
        detail = names.mid(0, 5).join(QStringLiteral(", ")) + QString::fromUtf8(" \u7b49 %1 \u9879").arg(toDelete.size()); // 等 %1 项

    QMessageBox::StandardButton btn = QMessageBox::question(this,
        QString::fromUtf8("\u786e\u8ba4\u5220\u9664"), // 确认删除
        toDelete.size() == 1
            ? QString::fromUtf8("\u786e\u5b9a\u8981\u5220\u9664\n%1\n\u5417?").arg(detail) // 确定要删除\n%1\n吗?
            : QString::fromUtf8("\u786e\u5b9a\u8981\u5220\u9664\u4ee5\u4e0b %1 \u9879\u5417?\n\n%2") // 确定要删除以下 %1 项吗?\n\n%2
                .arg(toDelete.size()).arg(detail));
    if (btn != QMessageBox::Yes) return;

    int failed = 0;
    for (const QString &p : toDelete) {
        QFileInfo fi(p);
        bool ok = fi.isDir() ? QDir(p).removeRecursively() : QFile::remove(p);
        if (!ok) failed++;
    }
    if (failed > 0) {
        if (toDelete.size() == 1)
            QMessageBox::warning(this,
                QString::fromUtf8("\u5220\u9664\u5931\u8d25"), // 删除失败
                QString::fromUtf8("\u65e0\u6cd5\u5220\u9664: %1").arg(QFileInfo(toDelete.first()).fileName())); // 无法删除: %1
        else
            QMessageBox::warning(this,
                QString::fromUtf8("\u5220\u9664\u5931\u8d25"), // 删除失败
                QString::fromUtf8("\u65e0\u6cd5\u5220\u9664 %1 \u9879").arg(failed)); // 无法删除 %1 项
    } else if (toDelete.size() > 1) {
        QMessageBox::information(this,
            QString::fromUtf8("\u5df2\u5220\u9664"), // 已删除
            QString::fromUtf8("\u5df2\u5220\u9664 %1 \u9879").arg(toDelete.size())); // 已删除 %1 项
    }
    refreshView();
    emit filesChanged();
}

void InAppFileDialog::renameSelected()
{
    QModelIndex idx = m_tree->currentIndex();
    if (!idx.isValid()) return;
    QString oldPath = m_model->filePath(idx);
    QFileInfo fi(oldPath);
    if (fi.canonicalFilePath() == QFileInfo(currentPath()).canonicalFilePath()) {
        QMessageBox::warning(this,
            QString::fromUtf8("\u91cd\u547d\u540d\u5931\u8d25"), // 重命名失败
            QString::fromUtf8("\u65e0\u6cd5\u91cd\u547d\u540d\u5f53\u524d\u76ee\u5f55")); // 无法重命名当前目录
        return;
    }
    QString oldName = fi.fileName();

    bool ok;
    QString newName = QInputDialog::getText(this,
        QString::fromUtf8("\u91cd\u547d\u540d"),          // 重命名
        QString::fromUtf8("\u65b0\u6587\u4ef6\u540d:"),   // 新文件名:
        QLineEdit::Normal, oldName, &ok);

    if (!ok || newName.isEmpty() || newName == oldName) return;

    QString newPath = fi.absoluteDir().absoluteFilePath(newName);
    if (QFile::exists(newPath)) {
        QMessageBox::warning(this,
            QString::fromUtf8("\u91cd\u547d\u540d\u5931\u8d25"),
            QString::fromUtf8("\u76ee\u6807\u6587\u4ef6\u5df2\u5b58\u5728: %1").arg(newName));
        return;
    }

    if (QFile::rename(oldPath, newPath)) {
        refreshView();
        emit filesChanged();
    } else {
        QMessageBox::warning(this,
            QString::fromUtf8("\u91cd\u547d\u540d\u5931\u8d25"),
            QString::fromUtf8("\u65e0\u6cd5\u91cd\u547d\u540d: %1").arg(oldName));
    }
}

void InAppFileDialog::newFolder()
{
    createItem(true);
}

void InAppFileDialog::newFile()
{
    createItem(false);
}

void InAppFileDialog::createItem(bool folder)
{
    bool ok;
    QString name = QInputDialog::getText(this,
        folder ? QString::fromUtf8("\u65b0\u5efa\u6587\u4ef6\u5939") : QString::fromUtf8("\u65b0\u5efa\u6587\u4ef6"), // 新建文件夹 / 新建文件
        QString::fromUtf8("\u540d\u79f0:"), // 名称:
        QLineEdit::Normal, QString(), &ok);
    if (!ok) return;
    name = name.trimmed();
    if (name.isEmpty()) return;
    if (name.contains(QLatin1Char('/')) || name.contains(QLatin1Char('\\'))) {
        QMessageBox::warning(this,
            QString::fromUtf8("\u521b\u5efa\u5931\u8d25"), // 创建失败
            QString::fromUtf8("\u65e0\u6cd5\u521b\u5efa: %1").arg(name)); // 无法创建: %1
        return;
    }

    QString full = QDir(currentPath()).absoluteFilePath(name);
    if (QFile::exists(full)) {
        QMessageBox::warning(this,
            QString::fromUtf8("\u521b\u5efa\u5931\u8d25"), // 创建失败
            QString::fromUtf8("\u76ee\u6807\u6587\u4ef6\u5df2\u5b58\u5728: %1").arg(name)); // 目标文件已存在: %1
        return;
    }

    bool created = false;
    if (folder) {
        created = QDir().mkdir(full);
    } else {
        QFile f(full);
        created = f.open(QIODevice::WriteOnly);
        f.close();
    }
    if (!created) {
        QMessageBox::warning(this,
            QString::fromUtf8("\u521b\u5efa\u5931\u8d25"), // 创建失败
            QString::fromUtf8("\u65e0\u6cd5\u521b\u5efa: %1").arg(name)); // 无法创建: %1
        return;
    }

    // Clear an active filter so the new item is visible, then select it.
    if (!m_filterEdit->text().isEmpty()) m_filterEdit->clear();
    refreshView();
    QModelIndex idx = m_model->index(full);
    if (idx.isValid()) {
        m_tree->setCurrentIndex(idx);
        m_tree->scrollTo(idx);
    }
    updateStatus();
    emit filesChanged();
}

void InAppFileDialog::refreshView()
{
    QString rootPath = m_model->rootPath();
    QModelIndex rootIdx = m_tree->rootIndex();
    if (rootIdx.isValid() && rootIdx != m_model->index(rootPath))
        rootPath = m_model->filePath(rootIdx);

    QStringList sel = selectedPaths();
    m_model->setRootPath("");
    m_model->setRootPath(rootPath);
    m_tree->setRootIndex(m_model->index(rootPath));

    QItemSelectionModel *sm = m_tree->selectionModel();
    sm->clearSelection();
    for (const QString &p : sel) {
        QModelIndex idx = m_model->index(p);
        if (idx.isValid()) sm->select(idx, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
    updatePathDisplay();
    updateStatus();
}

void InAppFileDialog::startPathEdit()
{
    m_pathLabel->hide();
    m_pathEdit->setText(currentPath());
    m_pathEdit->show();
    m_pathEdit->setFocus();
    m_pathEdit->selectAll();
}

void InAppFileDialog::applyPathEdit()
{
    QString newPath = m_pathEdit->text().trimmed();
    m_pathEdit->hide();
    m_pathLabel->show();
    QFileInfo fi(newPath);
    if (fi.exists() && fi.isDir()) {
        navigateTo(fi.absoluteFilePath());
    } else {
        QMessageBox::warning(this,
            QString::fromUtf8("\u65e0\u6548\u8def\u5f84"), // 无效路径
            QString::fromUtf8("\u8def\u5f84\u4e0d\u5b58\u5728\u6216\u4e0d\u662f\u76ee\u5f55: %1").arg(newPath)); // 路径不存在或不是目录: %1
        updatePathDisplay();
    }
}

void InAppFileDialog::onFilterChanged(const QString &text)
{
    QString t = text;
    const QString bad = QString("\\/:*?\"<>|");
    for (QChar c : bad) t.remove(c);
    if (t.isEmpty()) {
        m_model->setNameFilters(QStringList());
    } else {
        m_model->setNameFilters({QString("*%1*").arg(t)});
    }
    updateStatus();
}

void InAppFileDialog::onSelectionChanged()
{
    updateStatus();
}

// --- Helpers ---

QString InAppFileDialog::currentPath() const
{
    return m_model->filePath(m_tree->rootIndex());
}

QStringList InAppFileDialog::selectedPaths() const
{
    QStringList out;
    const QModelIndexList rows = m_tree->selectionModel()->selectedRows();
    for (const QModelIndex &idx : rows) {
        QString fp = m_model->filePath(idx);
        if (!fp.isEmpty()) out << fp;
    }
    return out;
}

QModelIndex InAppFileDialog::dropTargetIndex(const QPoint &viewPos) const
{
    QModelIndex idx = m_tree->indexAt(viewPos);
    if (idx.isValid()) {
        QFileInfo fi(m_model->filePath(idx));
        if (!fi.isDir()) idx = idx.parent();
        return idx;
    }
    return m_tree->rootIndex();
}

bool InAppFileDialog::copyPath(const QString &src, const QString &dst)
{
    QFileInfo srcFi(src);
    if (!srcFi.isDir()) return QFile::copy(src, dst);
    QString dstAbs = QFileInfo(dst).absoluteFilePath();
    if (dstAbs.startsWith(srcFi.absoluteFilePath() + QLatin1Char('/')))
        return false; // would copy into itself
    if (!QDir().mkpath(dst)) return false;
    QDirIterator it(src, QDir::AllEntries | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QFileInfo fi = it.fileInfo();
        QString rel = fi.absoluteFilePath().mid(srcFi.absoluteFilePath().length() + 1);
        QString tgt = QDir(dst).absoluteFilePath(rel);
        if (fi.isDir()) {
            if (!QDir().mkpath(tgt)) return false;
        } else {
            if (!QFile::copy(fi.absoluteFilePath(), tgt)) return false;
        }
    }
    return true;
}

void InAppFileDialog::updatePathDisplay()
{
    m_pathLabel->setText(currentPath());
}

void InAppFileDialog::updateNavButtons()
{
    m_backBtn->setEnabled(!m_backStack.isEmpty());
    m_forwardBtn->setEnabled(!m_forwardStack.isEmpty());
}

void InAppFileDialog::updateStatus()
{
    if (!m_statusLabel) return;
    int total = m_model->rowCount(m_tree->rootIndex());
    int sel = m_tree->selectionModel()->selectedRows().size();
    if (sel > 0)
        m_statusLabel->setText(QString::fromUtf8("\u5171 %1 \u9879\uff0c\u5df2\u9009 %2 \u9879").arg(total).arg(sel)); // 共 %1 项，已选 %2 项
    else
        m_statusLabel->setText(QString::fromUtf8("\u5171 %1 \u9879").arg(total)); // 共 %1 项
}

// --- Convenience function ---

void openDirInFM(const QString &dirPath, std::function<void()> onFilesChanged)
{
    QString path = dirPath;
    if (path.endsWith(QChar('/'))) path.chop(1);
    static QPointer<InAppFileDialog> s_dlg;
    if (s_dlg) s_dlg->close();
    s_dlg = new InAppFileDialog(path);
    if (onFilesChanged)
        QObject::connect(s_dlg, &InAppFileDialog::filesChanged, s_dlg, onFilesChanged);
    s_dlg->show();
}
