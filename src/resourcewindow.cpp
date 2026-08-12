#include "resourcewindow.h"
#include "petwindow.h"
#include "inappfiledialog.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QPointer>
#include <QRect>
#include <algorithm>

// ====== ResourceWindow ======
// External file replacement window.
// Each resource row has: replace | open folder | delete
// "全部替换" picks a directory and copies all matching files.

ResourceWindow::ResourceWindow(DesktopPet *pet, float scale)
    : QWidget(nullptr, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
    , m_pet(pet), m_scale(scale)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    initUi();
    updateScaleAndPosition(scale);
}

void ResourceWindow::updateScaleAndPosition(float scale)
{
    m_scale = scale;
    int sw = qMax(360, (int)(520 * scale));
    int sh = qMax(280, (int)(380 * scale));
    setFixedSize(sw, sh);
    int pad = 6;
    int mlr = qMax(8, (int)(12*scale)), mtb = qMax(6, (int)(9*scale));
    m_mainLayout->setContentsMargins(pad+mlr, pad+mtb, pad+mlr, pad+mtb);
    m_mainLayout->setSpacing(qMax(2, (int)(4*scale)));
    int fs = qMax(8, (int)(11*scale));
    m_titleLabel->setStyleSheet(QString("color:#FF8DA1;font-weight:bold;font-size:%1px;").arg(fs));
    int bs = qMax(12, (int)(16*scale));
    m_closeBtn->setFixedSize(bs, bs);
    m_closeBtn->setStyleSheet(QString("QPushButton{background:transparent;color:#FF8DA1;border:1px solid #FF8DA1;border-radius:%1px;font-weight:bold;font-size:%2px;padding:0;}QPushButton:hover{background:#FF8DA1;color:#111;}").arg(bs/2).arg(qMax(7,(int)(9*scale))));
    int listItemFs = qMax(7, (int)(9*scale));
    m_resList->setStyleSheet(QString("QListWidget{background:#1a1a1a;color:#ddd;border:1px solid #FF8DA1;border-radius:4px;font-size:%1px;}QListWidget::item{padding:2px;}QListWidget::item:selected{background:#FF8DA1;color:#111;}QScrollBar:vertical{width:5px;background:transparent;}QScrollBar::handle:vertical{background:#FF8DA1;border-radius:2px;min-height:15px;}QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}QScrollBar:horizontal{height:5px;background:transparent;}QScrollBar::handle:horizontal{background:#FF8DA1;border-radius:2px;}").arg(listItemFs));
    int btnFs = qMax(6, (int)(8*scale));
    int btnPad = qMax(1, (int)(2*scale));
    QString btnStyle = QString("QPushButton{background:#FF8DA1;color:#111;border:none;border-radius:2px;font-weight:bold;font-size:%1px;padding:%2px %3px;}QPushButton:hover{background:#FFA5B5;}QPushButton:disabled{background:#666;color:#999;}").arg(btnFs).arg(btnPad).arg(qMax(2,(int)(4*scale)));
    for (auto *btn : m_allBtns) btn->setStyleSheet(btnStyle);
    m_replaceAllBtn->setStyleSheet(btnStyle);
    m_openAllBtn->setStyleSheet(btnStyle);
    positionNearPet();
}

void ResourceWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QRectF r(2.0, 2.0, width()-4.0, height()-4.0);
    p.setBrush(QBrush(QColor(0,0,0,255)));
    p.setPen(QPen(PINK, 4));
    p.drawRoundedRect(r, 10, 10);
}

// --- Private helpers ---

void ResourceWindow::copyFileToOverride(const QString &src, const QString &subDir, const QString &baseName, const QString &ext)
{
    QDir d(m_pet->m_resourceDir + subDir);
    if (!d.exists()) d.mkpath(".");
    QString dst = d.absoluteFilePath(baseName + ext);
    if (QFile::exists(dst)) QFile::remove(dst);
    QFile::copy(src, dst);
}

void ResourceWindow::refreshRow(QLabel *sl, QPushButton *db, const QString &key)
{
    bool s = m_pet->m_resourceOverrides.contains(key);
    sl->setText(s ? QString::fromUtf8("\u5df2\u66ff\u6362") : QString::fromUtf8("\u9ed8\u8ba4"));
    sl->setStyleSheet(s ? "color:#5f5;" : "color:#888;");
    db->setEnabled(s);
}

void ResourceWindow::refreshAllRows()
{
    for (int i = 0; i < m_resList->count(); i++) {
        QWidget *row = m_resList->itemWidget(m_resList->item(i));
        if (!row) continue;
        QString key = row->property("resourceKey").toString();
        if (key.isEmpty()) continue; // category header rows have no widget
        QLayout *rowLay = row->layout();
        if (!rowLay) continue;
        // initUi layout order: name(0), status label(1), replace(2), open(3), delete(4)
        auto *sl = qobject_cast<QLabel*>(rowLay->itemAt(1)->widget());
        auto *db = qobject_cast<QPushButton*>(rowLay->itemAt(4)->widget());
        if (sl && db) refreshRow(sl, db, key);
    }
}

void ResourceWindow::initUi()
{
    m_mainLayout = new QVBoxLayout(this);
    QHBoxLayout *topBar = new QHBoxLayout();
    topBar->setContentsMargins(0,0,0,0);
    m_titleLabel = new QLabel(QString::fromUtf8("\u8d44\u6e90\u66ff\u6362"), this);
    m_titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_replaceAllBtn = new QPushButton(QString::fromUtf8("\u5168\u90e8\u66ff\u6362"), this);
    m_replaceAllBtn->setCursor(Qt::PointingHandCursor);
    connect(m_replaceAllBtn, &QPushButton::clicked, this, &ResourceWindow::onReplaceAll);
    m_closeBtn = new QPushButton(QString::fromUtf8("\u2715"), this);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_closeBtn, &QPushButton::clicked, this, &ResourceWindow::closeResourceWindow);
    m_openAllBtn = new QPushButton(QString::fromUtf8("\u6253\u5f00\u5168\u90e8\u76ee\u5f55"), this);
    m_openAllBtn->setCursor(Qt::PointingHandCursor);
    connect(m_openAllBtn, &QPushButton::clicked, this, &ResourceWindow::onOpenAll);
    topBar->addWidget(m_titleLabel); topBar->addWidget(m_replaceAllBtn); topBar->addWidget(m_openAllBtn); topBar->addStretch(); topBar->addWidget(m_closeBtn);

    QLabel *hint = new QLabel(QString::fromUtf8("\u70b9\u201c\u66ff\u6362\u201d\u9009\u6587\u4ef6\uff0c\u6216\u201c\u5168\u90e8\u66ff\u6362\u201d\u6279\u91cf\u66ff\u6362"), this);
    hint->setStyleSheet("color:#888;font-size:10px;padding:2px 4px;"); hint->setWordWrap(true);

    m_resList = new QListWidget(this);
    m_resList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_resList->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QList<ResourceDef> defs;
    QStringList gifs = {"idle","click","drag","sleep","draw","result"};
    for (const QString &g : gifs)
        defs.append({"gif/"+g, "GIF - "+g, QString::fromUtf8("GIF\u52a8\u753b"), "gif", ".gif"});
    QStringList audios = {"start","draw","drawing","result","reset","alarm","clock"};
    QStringList an = {QString::fromUtf8("\u542f\u52a8"),QString::fromUtf8("\u62bd\u5361"),QString::fromUtf8("\u753b\u753b"),QString::fromUtf8("\u7ed3\u679c"),QString::fromUtf8("\u91cd\u7f6e"),QString::fromUtf8("\u95f9\u949f"),QString::fromUtf8("\u65f6\u949f")};
    for (int i=0;i<audios.size();i++)
        defs.append({"audio/"+audios[i],"WAV - "+an[i],QString::fromUtf8("\u97f3\u6548"),"audio",".wav"});
    for (int i=1;i<=55;i++)
        defs.append({"cards/card_"+QString::number(i),QString::fromUtf8("PNG - card_%1").arg(i),QString::fromUtf8("\u5361\u7247"),"cards",".png"});
    for (int i=1;i<=15;i++)
        defs.append({"drawing/drawing_"+QString::number(i),QString::fromUtf8("PNG - drawing_%1").arg(i),QString::fromUtf8("\u753b\u4f5c"),"drawing",".png"});
    defs.append({"drawing/author","PNG - author",QString::fromUtf8("\u5176\u4ed6"),"drawing",".png"});

    QString curCat;
    for (const auto &df : defs) {
        if (df.category != curCat) {
            curCat = df.category;
            QListWidgetItem *ci = new QListWidgetItem("\u2500\u2500 " + df.category + " \u2500\u2500");
            ci->setFlags(Qt::NoItemFlags); ci->setForeground(QColor("#FF8DA1")); ci->setTextAlignment(Qt::AlignCenter);
            m_resList->addItem(ci);
        }
        QWidget *row = new QWidget();
        QHBoxLayout *lay = new QHBoxLayout(row);
        lay->setContentsMargins(4,1,4,1); lay->setSpacing(3);
        QLabel *nl = new QLabel(df.name);
        nl->setStyleSheet("color:#fff;font-weight:bold;"); nl->setFixedWidth(qMax(80,(int)(150*m_scale)));
        bool over = m_pet->m_resourceOverrides.contains(df.key);
        QLabel *sl = new QLabel(over ? QString::fromUtf8("\u5df2\u66ff\u6362") : QString::fromUtf8("\u9ed8\u8ba4"));
        sl->setStyleSheet(over ? "color:#5f5;" : "color:#888;"); sl->setFixedWidth(qMax(35,(int)(48*m_scale))); sl->setAlignment(Qt::AlignCenter);

        // Replace button
        QPushButton *rb = new QPushButton(QString::fromUtf8("\u66ff\u6362"));
        rb->setCursor(Qt::PointingHandCursor); rb->setFixedWidth(qMax(30,(int)(40*m_scale)));
        QString k = df.key; QString sd = df.subDir; QString ext = df.ext; QString bn = df.key.section('/',1);
        // Store resource metadata on row widget for onReplaceAll() to use
        row->setProperty("resourceKey", df.key);
        row->setProperty("subDir", df.subDir);
        row->setProperty("ext", df.ext);
        QPointer<QLabel> slPtr = sl;
        connect(rb, &QPushButton::clicked, this, [this, k, sd, bn, ext, slPtr]() {
            QString filter = QString("*%1").arg(ext);
            QString fp = QFileDialog::getOpenFileName(this, QString::fromUtf8("\u9009\u62e9\u66ff\u6362\u6587\u4ef6"), QString(), filter);
            if (fp.isEmpty()) return;
            copyFileToOverride(fp, sd, bn, ext);
            m_pet->scheduleResourceReload();
            QPushButton *db = nullptr;
            QLayout *rowLay = qobject_cast<QPushButton*>(sender())->parentWidget()->layout();
            for (int i=0;i<rowLay->count();i++) {
                auto *w = rowLay->itemAt(i)->widget();
                if (auto *b = qobject_cast<QPushButton*>(w)) {
                    if (b->text() == QString::fromUtf8("\u5220\u9664")) { db = b; break; }
                }
            }
            if (slPtr && db) refreshRow(slPtr, db, k);
        });

        // Open folder button
        QPushButton *ob = new QPushButton(QString::fromUtf8("\u6253\u5f00"));
        ob->setCursor(Qt::PointingHandCursor); ob->setFixedWidth(qMax(28,(int)(38*m_scale)));
        connect(ob, &QPushButton::clicked, this, [this, sd]() { QString p = m_pet->m_resourceDir + sd; QDir().mkpath(p); openDirInFM(p, [this] { m_pet->scheduleResourceReload(); }); });

        // Delete button
        QPushButton *db = new QPushButton(QString::fromUtf8("\u5220\u9664"));
        db->setCursor(Qt::PointingHandCursor); db->setEnabled(over); db->setFixedWidth(qMax(28,(int)(38*m_scale)));
        connect(db, &QPushButton::clicked, this, [this, k, sl, db]() {
            QString fp = m_pet->m_resourceOverrides.value(k);
            if (!fp.isEmpty() && QFile::exists(fp)) QFile::remove(fp);
            m_pet->scheduleResourceReload();
            refreshRow(sl, db, k);
        });
        m_allBtns.append(rb); m_allBtns.append(ob); m_allBtns.append(db);
        lay->addWidget(nl); lay->addWidget(sl); lay->addWidget(rb); lay->addWidget(ob); lay->addWidget(db);
        QListWidgetItem *it = new QListWidgetItem();
        it->setSizeHint(row->sizeHint()); m_resList->addItem(it); m_resList->setItemWidget(it, row);
    }
    m_mainLayout->addLayout(topBar); m_mainLayout->addWidget(hint); m_mainLayout->addWidget(m_resList, 1);
}

void ResourceWindow::onReplaceAll()
{
    QString dir = QFileDialog::getExistingDirectory(this, QString::fromUtf8("\u9009\u62e9\u5305\u542b\u66ff\u6362\u6587\u4ef6\u7684\u76ee\u5f55"));
    if (dir.isEmpty()) return;
    QDir d(dir);
    int count = 0;
    for (int i = 0; i < m_resList->count(); i++) {
        QListWidgetItem *it = m_resList->item(i);
        QWidget *row = m_resList->itemWidget(it);
        if (!row || !row->layout()) continue;
        // Use stored metadata instead of parsing label text (avoids Chinese name mismatch)
        QString key = row->property("resourceKey").toString();
        QString sd = row->property("subDir").toString();
        QString ext = row->property("ext").toString();
        if (key.isEmpty() || sd.isEmpty() || ext.isEmpty()) continue;
        QString baseName = key.section('/', 1);
        // Try all extensions for this resource
        QFileInfo fi(d.filePath(baseName + ext));
        if (!fi.exists() || !fi.isFile()) {
            // Also try with common alternate extensions
            QStringList altExts;
            if (ext == ".gif") altExts = {".png", ".webp"};
            else if (ext == ".wav") altExts = {".mp3", ".ogg"};
            else if (ext == ".png") altExts = {".jpg", ".jpeg", ".bmp"};
            for (const QString &ae : altExts) {
                fi = QFileInfo(d.filePath(baseName + ae));
                if (fi.exists() && fi.isFile()) break;
            }
        }
        if (fi.exists() && fi.isFile()) {
            QHBoxLayout *lay = qobject_cast<QHBoxLayout*>(row->layout());
            QLabel *sl = lay ? qobject_cast<QLabel*>(lay->itemAt(1)->widget()) : nullptr;
            QPushButton *db = nullptr;
            if (lay) {
                for (int j = 0; j < lay->count(); j++) {
                    auto *w = lay->itemAt(j)->widget();
                    if (auto *b = qobject_cast<QPushButton*>(w)) {
                        if (b->text() == QString::fromUtf8("\u5220\u9664")) { db = b; break; }
                    }
                }
            }
            copyFileToOverride(fi.absoluteFilePath(), sd, baseName, ext);
            if (sl && db) refreshRow(sl, db, key);
            count++;
            // Process events to show immediate visual feedback for each replaced file
            QApplication::processEvents();
        }
    }
    if (count > 0) m_pet->scheduleResourceReload(); // re-apply once after the whole batch
    QMessageBox::information(this, QString::fromUtf8("\u5168\u90e8\u66ff\u6362"), QString::fromUtf8("\u5df2\u66ff\u6362 %1 \u4e2a\u6587\u4ef6").arg(count));
}

void ResourceWindow::onOpenAll()
{
    QString dir = m_pet->m_resourceDir;
    QDir d(dir);
    if (!d.exists()) d.mkpath(".");
    QStringList subDirs = {"gif", "audio", "cards", "drawing"};
    for (const QString &sd : subDirs)
        d.mkpath(sd);
    openDirInFM(dir, [this] { m_pet->scheduleResourceReload(); });
}

void ResourceWindow::closeResourceWindow() { m_pet->m_resourceWindow = nullptr; close(); }

void ResourceWindow::positionNearPet()
{
    if (m_pet) { QRect pr = m_pet->geometry(); move(pr.x()+pr.width()+10, pr.y()+(pr.height()-height())/2); }
}
