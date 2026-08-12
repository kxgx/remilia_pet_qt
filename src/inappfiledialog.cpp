#include "inappfiledialog.h"

#include <QPointer>

const QColor PINK(255, 141, 161);

InAppFileDialog::InAppFileDialog(const QString &dirPath, QWidget *parent)
    : QDialog(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(QString::fromUtf8("\u6587\u4ef6\u6d4f\u89c8")); // 文件浏览
    setupUi(dirPath);
}

void InAppFileDialog::setupUi(const QString &dirPath)
{
    resize(580, 420);
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
    QHBoxLayout *toolbar = new QHBoxLayout();
    toolbar->setSpacing(4);

    QPushButton *upBtn = new QPushButton(QString::fromUtf8("\u2190 \u5411\u4e0a")); // ← 向上
    upBtn->setCursor(Qt::PointingHandCursor);
    upBtn->setStyleSheet("QPushButton{background:#333;color:#ddd;border:none;border-radius:3px;font-size:11px;padding:2px 6px;}QPushButton:hover{background:#555;}");
    connect(upBtn, &QPushButton::clicked, this, &InAppFileDialog::navigateUp);

    QPushButton *refreshBtn = new QPushButton(QString::fromUtf8("\u21bb \u5237\u65b0")); // ↻ 刷新
    refreshBtn->setCursor(Qt::PointingHandCursor);
    refreshBtn->setStyleSheet(upBtn->styleSheet());
    connect(refreshBtn, &QPushButton::clicked, this, &InAppFileDialog::refreshView);

    toolbar->addWidget(upBtn);
    toolbar->addWidget(refreshBtn);

    // --- Path bar ---
    QHBoxLayout *pathBar = new QHBoxLayout();
    pathBar->setSpacing(2);
    m_pathLabel = new QLabel(dirPath);
    m_pathLabel->setStyleSheet("color:#888;font-size:10px;padding:2px 4px;border:1px solid transparent;border-radius:2px;");
    m_pathLabel->setCursor(Qt::IBeamCursor);
    m_pathLabel->setWordWrap(false);
    m_pathLabel->setTextInteractionFlags(Qt::NoTextInteraction);
    m_pathLabel->installEventFilter(this);
    // Click path label to edit
    connect(m_pathLabel, &QLabel::linkActivated, this, [this](const QString &) { startPathEdit(); });

    m_pathEdit = new QLineEdit();
    m_pathEdit->setStyleSheet("background:#111;color:#ddd;border:1px solid #FF8DA1;border-radius:2px;font-size:10px;padding:1px 4px;");
    m_pathEdit->hide();
    connect(m_pathEdit, &QLineEdit::returnPressed, this, &InAppFileDialog::applyPathEdit);

    pathBar->addWidget(m_pathLabel);
    pathBar->addWidget(m_pathEdit, 1);
    toolbar->addLayout(pathBar);

    // --- File tree ---
    m_model = new QFileSystemModel(this);
    m_model->setRootPath(dirPath);
    m_model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);

    m_tree = new QTreeView();
    m_tree->setModel(m_model);
    m_tree->setRootIndex(m_model->index(dirPath));
    m_tree->hideColumn(1);
    m_tree->hideColumn(2);
    m_tree->hideColumn(3);
    m_tree->setHeaderHidden(true);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->setDragDropMode(QAbstractItemView::NoDragDrop);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tree->setStyleSheet("QTreeView{background:#111;color:#ddd;border:1px solid #444;border-radius:4px;font-size:11px;}QTreeView::item:hover{background:#333;}QTreeView::item:selected{background:#FF8DA1;color:#111;}QScrollBar:vertical{width:5px;background:transparent;}QScrollBar::handle:vertical{background:#FF8DA1;border-radius:2px;min-height:15px;}QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}");

    connect(m_tree, &QTreeView::doubleClicked, this, &InAppFileDialog::onDoubleClicked);
    connect(m_tree, &QTreeView::customContextMenuRequested, this, &InAppFileDialog::onCustomContextMenu);

    lay->addLayout(titleBar);
    lay->addLayout(toolbar);
    lay->addWidget(m_tree, 1);
    outerLayout->addWidget(inner);
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
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        deleteSelected();
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
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void InAppFileDialog::dropEvent(QDropEvent *event)
{
    QModelIndex idx = m_tree->indexAt(event->position().toPoint() - m_tree->viewport()->pos());
    QString targetDir;
    if (idx.isValid()) {
        QFileInfo fi(m_model->filePath(idx));
        targetDir = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
    } else {
        targetDir = m_model->rootPath();
    }

    const QList<QUrl> urls = event->mimeData()->urls();
    int count = 0;
    for (const QUrl &url : urls) {
        if (!url.isLocalFile()) continue;
        QString src = url.toLocalFile();
        QFileInfo fi(src);
        QString dst = QDir(targetDir).absoluteFilePath(fi.fileName());
        // Ask if overwrite
        if (QFile::exists(dst)) {
            QMessageBox::StandardButton btn = QMessageBox::question(this,
                QString::fromUtf8("\u786e\u8ba4\u66ff\u6362"), // 确认替换
                QString::fromUtf8("\u76ee\u6807\u6587\u4ef6\u5df2\u5b58\u5728:\n%1\n\n\u662f\u5426\u66ff\u6362?") // 目标文件已存在:\n%1\n\n是否替换?
                    .arg(fi.fileName()));
            if (btn != QMessageBox::Yes) continue;
            QFile::remove(dst);
        }
        if (QFile::copy(src, dst)) count++;
    }
    if (count > 0)
        QMessageBox::information(this,
            QString::fromUtf8("\u5bfc\u5165\u6210\u529f"), // 导入成功
            QString::fromUtf8("\u5df2\u5bfc\u5165 %1 \u4e2a\u6587\u4ef6") // 已导入 %1 个文件
                .arg(count));
    event->acceptProposedAction();
}

// --- Slots ---

void InAppFileDialog::onDoubleClicked(const QModelIndex &idx)
{
    QString fp = m_model->filePath(idx);
    if (fp.isEmpty()) return;
    QFileInfo fi(fp);
    if (fi.isDir()) {
        m_tree->setRootIndex(m_model->index(fp));
        updatePathDisplay();
    } else {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fp));
    }
}

void InAppFileDialog::onCustomContextMenu(const QPoint &pos)
{
    QModelIndex idx = m_tree->indexAt(pos);
    QMenu menu(this);
    menu.setStyleSheet("QMenu{background:#1a1a1a;color:#ddd;border:1px solid #FF8DA1;padding:2px;}QMenu::item{padding:4px 12px;}QMenu::item:selected{background:#FF8DA1;color:#111;}");

    QAction *openAct = menu.addAction(QString::fromUtf8("\u6253\u5f00")); // 打开
    menu.addSeparator();
    QAction *deleteAct = nullptr;
    QAction *renameAct = nullptr;
    if (idx.isValid()) {
        deleteAct = menu.addAction(QString::fromUtf8("\u5220\u9664")); // 删除
        renameAct = menu.addAction(QString::fromUtf8("\u91cd\u547d\u540d")); // 重命名
    }
    menu.addSeparator();
    QAction *refreshAct = menu.addAction(QString::fromUtf8("\u5237\u65b0")); // 刷新

    QAction *chosen = menu.exec(m_tree->viewport()->mapToGlobal(pos));
    if (!chosen) return;
    if (chosen == openAct) {
        if (idx.isValid()) onDoubleClicked(idx);
    } else if (chosen == deleteAct) {
        deleteSelected();
    } else if (chosen == renameAct) {
        renameSelected();
    } else if (chosen == refreshAct) {
        refreshView();
    }
}

void InAppFileDialog::navigateUp()
{
    QModelIndex rootIdx = m_tree->rootIndex();
    QModelIndex parentIdx = rootIdx.parent();
    if (parentIdx.isValid()) {
        m_tree->setRootIndex(parentIdx);
    } else {
        // Already at filesystem root
        QFileInfo fi(m_model->filePath(rootIdx));
        QString parent = fi.path();
        if (parent != fi.absoluteFilePath()) {
            m_tree->setRootIndex(m_model->index(parent));
        }
    }
    updatePathDisplay();
}

void InAppFileDialog::deleteSelected()
{
    QString fp = selectedFilePath();
    if (fp.isEmpty()) return;
    QFileInfo fi(fp);
    QMessageBox::StandardButton btn = QMessageBox::question(this,
        QString::fromUtf8("\u786e\u8ba4\u5220\u9664"), // 确认删除
        QString::fromUtf8("\u786e\u5b9a\u8981\u5220\u9664\n%1\n\u5417?").arg(fi.fileName())); // 确定要删除\n%1\n吗?
    if (btn != QMessageBox::Yes) return;
    bool ok = fi.isDir() ? QDir(fp).removeRecursively() : QFile::remove(fp);
    if (!ok) {
        QMessageBox::warning(this,
            QString::fromUtf8("\u5220\u9664\u5931\u8d25"), // 删除失败
            QString::fromUtf8("\u65e0\u6cd5\u5220\u9664: %1").arg(fi.fileName())); // 无法删除: %1
    }
}

void InAppFileDialog::renameSelected()
{
    QModelIndex idx = m_tree->currentIndex();
    if (!idx.isValid()) return;
    // Enter edit mode on the tree item
    m_tree->edit(idx);
}

void InAppFileDialog::refreshView()
{
    QString rootPath = m_model->rootPath();
    QModelIndex rootIdx = m_tree->rootIndex();
    if (rootIdx.isValid() && rootIdx != m_model->index(rootPath)) {
        // Currently in a subdirectory, preserve it
        rootPath = m_model->filePath(rootIdx);
    }
    m_model->setRootPath("");
    m_model->setRootPath(rootPath);
    m_tree->setRootIndex(m_model->index(rootPath));
    updatePathDisplay();
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
        m_model->setRootPath(newPath);
        m_tree->setRootIndex(m_model->index(newPath));
        updatePathDisplay();
    } else {
        m_pathLabel->setText(currentPath());
    }
}

// --- Helpers ---

QString InAppFileDialog::selectedFilePath() const
{
    QModelIndex idx = m_tree->currentIndex();
    return idx.isValid() ? m_model->filePath(idx) : QString();
}

QString InAppFileDialog::currentPath() const
{
    return m_model->filePath(m_tree->rootIndex());
}

void InAppFileDialog::updatePathDisplay()
{
    m_pathLabel->setText(currentPath());
}

// --- Convenience function ---

void openDirInFM(const QString &dirPath)
{
    QString path = dirPath;
    if (path.endsWith(QChar('/'))) path.chop(1);
    static QPointer<InAppFileDialog> s_dlg;
    if (s_dlg) s_dlg->close();
    s_dlg = new InAppFileDialog(path);
    s_dlg->show();
}

