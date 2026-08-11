#include "petwindow.h"

#include <QApplication>
#include <QMenu>
#include <QAction>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QCloseEvent>
#include <QScreen>
#include <QImageReader>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QPushButton>
#include <QMessageBox>
#include <QPointer>
#include <QRandomGenerator>
#include <QPixmap>
#include <QStyle>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QLineEdit>
#include <QIntValidator>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QFontDatabase>
#include <QRectF>
#include <QSettings>
#include <QWidgetAction>
#include <QActionGroup>
#include <QWindow>
#include <QFileDialog>
#include <QDesktopServices>
#include <QFileInfo>
#include <QFileInfo>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#ifdef Q_OS_MAC
#include <objc/objc.h>
#include <objc/message.h>
#include <CoreGraphics/CoreGraphics.h>
#endif
#include <QListWidget>
#include <QRegularExpression>

static const QColor PINK(255, 141, 161);

// ========== DesktopPet resource override ==========

QString DesktopPet::resolveResourcePath(const QString &qrcPath) const
{
    QString key;
    if (qrcPath.startsWith("qrc:"))
        key = qrcPath.mid(4);
    else if (qrcPath.startsWith(":/"))
        key = qrcPath.mid(2);
    else
        return qrcPath;

    int dotIdx = key.lastIndexOf('.');
    if (dotIdx > 0)
        key = key.left(dotIdx);
    if (key.startsWith('/'))
        key = key.mid(1);

    if (m_resourceOverrides.contains(key))
        return m_resourceOverrides[key];
    return qrcPath;
}

void DesktopPet::loadOverrides()
{
    QStringList cand;
    cand << QApplication::applicationDirPath() + "/resources/"
         << QApplication::applicationDirPath() + QString::fromUtf8("/\u8D44\u6E90/");
    m_resourceDir.clear();
    for (const QString &d : cand) { if (QDir(d).exists()) { m_resourceDir = d; break; } }
    if (m_resourceDir.isEmpty()) { m_resourceDir = cand.first(); return; }
    struct { QByteArray sd; QByteArray ex; } m[] = {
        {"gif",".gif"},{"audio",".wav"},{"cards",".png"},{"drawing",".png"}
    };
    for (auto &x : m) {
        QDir dir(m_resourceDir + x.sd);
        if (!dir.exists()) { dir.mkpath("."); continue; }
        for (const QString &f : dir.entryList(QDir::Files)) {
            QFileInfo fi(dir.absoluteFilePath(f));
            if (fi.suffix().toLower() != x.ex.mid(1)) continue;
            m_resourceOverrides[x.sd + "/" + fi.completeBaseName()] = fi.absoluteFilePath();
        }
    }
}

void DesktopPet::refreshOverrides()
{
    m_resourceOverrides.clear();
    loadOverrides();
    setState(m_state);
}

// ========== DrawEffectWindow ==========
class DrawEffectWindow : public QWidget {
public:
    DrawEffectWindow(DesktopPet *pet, const QString &cardsDir, float scale)
        : QWidget(nullptr, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
        , m_pet(pet), m_cardsDir(cardsDir), m_scale(scale)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_DeleteOnClose);
        m_label = new QLabel(this);
        m_label->setAlignment(Qt::AlignCenter);
        m_opacity = new QGraphicsOpacityEffect(m_label);
        m_opacity->setOpacity(1.0);
        m_label->setGraphicsEffect(m_opacity);

        m_movie = new QMovie(m_pet->resolveResourcePath(":/gif/draw.gif"), QByteArray(), this);
        m_movie->jumpToFrame(0);
        QSize native = m_movie->frameRect().size();
        if (native.isValid() && native.width() > 0) {
            int sw = qMax(200, (int)(native.width() * scale));
            int maxW = m_pet->width() - 12;
            if (sw > maxW) sw = maxW;
            int sh = qMax(150, (int)(sw * native.height() / native.width()));
            int pad = 6;
            setFixedSize(sw + pad * 2, sh + pad * 2);
            m_label->setGeometry(pad, pad, sw, sh);
            m_movie->setScaledSize(QSize(sw, sh));
        }
        m_label->setMovie(m_movie);
        m_totalFrames = m_movie->frameCount();
        connect(m_movie, &QMovie::frameChanged, this, &DrawEffectWindow::onFrameChanged);
    }

    void startShow() {
        positionNearPet();
        show();
        m_movie->start();
    }

    void updateScaleAndPosition(float scale) {
        m_scale = scale;
        QSize native = m_movie->frameRect().size();
        if (native.isValid() && native.width() > 0) {
            int sw = qMax(200, (int)(native.width() * scale));
            int maxW = m_pet->width() - 12;
            if (sw > maxW) sw = maxW;
            int sh = qMax(150, (int)(sw * native.height() / native.width()));
            int pad = 6;
            setFixedSize(sw + pad * 2, sh + pad * 2);
            m_label->setGeometry(pad, pad, sw, sh);
            if (m_movie && m_movie->state() == QMovie::Running)
                m_movie->setScaledSize(QSize(sw, sh));
            else if (m_cardRevealed)
                renderCardContent();
        }
        positionNearPet();
    }

private slots:
    void onFrameChanged(int frame) {
        if (m_closed) return;
        if (m_totalFrames > 0 && frame == m_totalFrames - 1) {
            m_movie->stop();
            revealCard();
        }
    }

private:
    void revealCard() {
        if (m_closed) return;
        m_pet->setState(DesktopPet::Result);
        m_pet->playSound("result.mp3", false);
        m_label->setMovie(nullptr);
        int num = QRandomGenerator::global()->bounded(1, 56);
        m_revealedCardPath = m_pet->resolveResourcePath(m_cardsDir + QString("card_%1.png").arg(num));
        m_cardRevealed = true;
        renderCardContent();
        QPropertyAnimation *anim = new QPropertyAnimation(m_opacity, "opacity", this);
        anim->setDuration(500); anim->setStartValue(0.0); anim->setEndValue(1.0);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
        QTimer::singleShot(5000, this, &DrawEffectWindow::finishDraw);
    }

    void renderCardContent() {
        if (!m_cardRevealed || m_revealedCardPath.isEmpty()) return;
        QPixmap cardPix(m_revealedCardPath);
        if (cardPix.isNull()) return;
        int lw = m_label->width(), lh = m_label->height();
        int iw = (int)(lw * 0.5), ih = (int)(lh * 0.5);
        QPixmap scaled = cardPix.scaled(QSize(qMax(5,iw), qMax(5,ih)), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QPixmap canvas(lw, lh);
        canvas.fill(Qt::transparent);
        QPainter p(&canvas);
        p.setRenderHint(QPainter::Antialiasing);
        QFont font; font.setPixelSize(qMax(6, (int)(lh*0.08))); font.setBold(true); font.setStyleStrategy(QFont::PreferAntialias);
        p.setFont(font);
        p.setPen(QColor(255,255,255));
        int textH = p.fontMetrics().height();
        int spacing = (int)(lh * 0.03);
        int totalH = textH + spacing + scaled.height();
        int startY = (lh - totalH) / 2;
        QRect textR(0, startY, lw, textH);
        p.drawText(textR, Qt::AlignCenter, QString::fromUtf8("调频结果"));
        int cardY = startY + textH + spacing;
        p.drawPixmap((lw - scaled.width())/2, cardY, scaled);
        p.end();
        m_label->setPixmap(canvas);
    }

    void finishDraw() {
        if (m_pet->m_effectWindow != this) return;
        if (m_closed) return;
        m_pet->m_isDrawingCard = false;
        m_pet->m_idleCounter = 0;
        m_pet->m_effectWindow = nullptr;
        m_pet->setState(DesktopPet::Idle);
        close();
    }

    void closeEvent(QCloseEvent *) override {
        m_closed = true;
        if (m_pet) m_pet->m_effectWindow = nullptr;
    }

    void positionNearPet() {
        if (m_pet) {
            QRect pr = m_pet->geometry();
            move(pr.x() + pr.width() + 10, pr.y() + (pr.height() - height())/2);
        }
    }

    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QRectF r(2.0, 2.0, width()-4.0, height()-4.0);
        p.setBrush(QBrush(QColor(0,0,0,255)));
        p.setPen(QPen(PINK, 4));
        p.drawRoundedRect(r, 10, 10);
    }

    DesktopPet *m_pet;
    QString m_cardsDir;
    float m_scale;
    QString m_revealedCardPath;
    bool m_cardRevealed = false;
    QLabel *m_label = nullptr;
    QMovie *m_movie = nullptr;
    QGraphicsOpacityEffect *m_opacity = nullptr;
    int m_totalFrames = 0;
    bool m_closed = false;
};

// ========== DrawingEffectWindow ==========
class DrawingEffectWindow : public QWidget {
public:
    DrawingEffectWindow(DesktopPet *pet, const QString &drawingDir, float scale)
        : QWidget(nullptr, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
        , m_pet(pet), m_drawingDir(drawingDir), m_scale(scale)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_DeleteOnClose);
        m_label = new QLabel(this);
        m_label->setAlignment(Qt::AlignCenter);
        m_opacity = new QGraphicsOpacityEffect(m_label);
        m_label->setGraphicsEffect(m_opacity);
    }

    void startShow() {
        int num = QRandomGenerator::global()->bounded(1, 16);
        m_drawPath = m_pet->resolveResourcePath(m_drawingDir + QString("drawing_%1.png").arg(num));
        QPixmap pix(m_drawPath);
        if (pix.isNull()) { close(); return; }
        QSize native = pix.size();
        if (native.width() > 0) {
            int sw = qMax(200, (int)(native.width() * 1.4 * m_scale));
            int maxW = m_pet->width() - 12;
            if (sw > maxW) sw = maxW;
            int sh = qMax(150, (int)(sw * native.height() / native.width()));
            int pad = 6;
            setFixedSize(sw + pad * 2, sh + pad * 2);
            m_label->setGeometry(pad, pad, sw, sh);
            m_label->setPixmap(pix.scaled(sw, sh, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        positionNearPet();
        show();
        QPropertyAnimation *anim = new QPropertyAnimation(m_opacity, "opacity", this);
        anim->setDuration(500); anim->setStartValue(0.0); anim->setEndValue(1.0);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
        QTimer::singleShot(3500, this, &DrawingEffectWindow::startFadeOut);
    }

    void updateScaleAndPosition(float scale) {
        m_scale = scale;
        if (!m_drawPath.isEmpty()) {
            QPixmap pix(m_drawPath);
            if (!pix.isNull()) {
                QSize native = pix.size();
                if (native.width() > 0) {
                    int sw = qMax(200, (int)(native.width() * 1.4 * m_scale));
                    int maxW = m_pet->width() - 12;
                    if (sw > maxW) sw = maxW;
                    int sh = qMax(150, (int)(sw * native.height() / native.width()));
                    int pad = 6;
                    setFixedSize(sw + pad * 2, sh + pad * 2);
                    m_label->setGeometry(pad, pad, sw, sh);
                    m_label->setPixmap(pix.scaled(sw, sh, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                }
            }
        }
        positionNearPet();
    }

private slots:
    void startFadeOut() {
        if (m_closed) return;
        QPropertyAnimation *anim = new QPropertyAnimation(m_opacity, "opacity", this);
        anim->setDuration(500); anim->setStartValue(1.0); anim->setEndValue(0.0);
        connect(anim, &QPropertyAnimation::finished, this, [this]() {
            if (m_closed) return;
            m_pet->m_isDrawingCard = false;
            m_pet->m_idleCounter = 0;
            m_pet->m_drawingWindow = nullptr;
            m_pet->setState(DesktopPet::Idle);
            close();
        });
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void closeEvent(QCloseEvent *) override {
        m_closed = true;
        if (m_pet) m_pet->m_drawingWindow = nullptr;
    }

private:
    void positionNearPet() {
        if (m_pet) {
            QRect pr = m_pet->geometry();
            move(pr.x() + pr.width() + 10, pr.y() + (pr.height() - height())/2);
        }
    }

    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QRectF r(2.0, 2.0, width()-4.0, height()-4.0);
        p.setBrush(QBrush(QColor(255,255,255,255)));
        p.setPen(QPen(PINK, 4));
        p.drawRoundedRect(r, 10, 10);
    }

    DesktopPet *m_pet;
    QString m_drawingDir;
    QString m_drawPath;
    float m_scale;
    QLabel *m_label = nullptr;
    QGraphicsOpacityEffect *m_opacity = nullptr;
    bool m_closed = false;
};

// ========== TimerWindow ==========
class TimerWindow : public QWidget {
public:
    TimerWindow(DesktopPet *pet, float scale)
        : QWidget(nullptr, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
        , m_pet(pet), m_scale(scale)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        m_countdownTimer = new QTimer(this);
        connect(m_countdownTimer, &QTimer::timeout, this, &TimerWindow::updateCountdown);
        initUi();
        updateScaleAndPosition(scale);
    }

    void updateScaleAndPosition(float scale) {
        m_scale = scale;
        int sw = qMax(250, (int)(350 * scale));
        int sh = qMax(140, (int)(190 * scale));
        setFixedSize(sw, sh);
        int pad = 12;
        int mlr = qMax(8, (int)(12*scale)), mtb = qMax(6, (int)(9*scale));
        m_mainLayout->setContentsMargins(pad+mlr, pad+mtb, pad+mlr, pad+mtb);
        m_mainLayout->setSpacing(qMax(4, (int)(6*scale)));
        int fs = qMax(12, (int)(18*scale));
        m_titleLabel->setStyleSheet(QString("color: #FF8DA1; font-weight: bold; font-size: %1px;").arg(fs));
        m_minUnit->setStyleSheet(QString("color: #FF8DA1; font-weight: bold; font-size: %1px;").arg(fs));
        m_secUnit->setStyleSheet(QString("color: #FF8DA1; font-weight: bold; font-size: %1px;").arg(fs));
        QString inputStyle = QString("QLineEdit{background:#222;color:#FF8DA1;border:1px solid #FF8DA1;border-radius:2px;padding:%1px;font-size:%2px;font-weight:bold;}").arg(qMax(2,(int)(3*scale))).arg(fs);
        m_minInput->setStyleSheet(inputStyle);
        m_secInput->setStyleSheet(inputStyle);
        m_confirmBtn->setStyleSheet(QString("QPushButton{background:#FF8DA1;color:#111;border-radius:2px;font-weight:bold;font-size:%1px;padding:%2px;}QPushButton:hover{background:#FFA5B5;}").arg(fs).arg(qMax(4,(int)(6*scale))));
        int bs = qMax(18, (int)(24*scale));
        m_closeBtn->setFixedSize(bs, bs);
        m_closeBtn->setStyleSheet(QString("QPushButton{background:transparent;color:#FF8DA1;border:1px solid #FF8DA1;border-radius:%1px;font-weight:bold;font-size:%2px;padding:0;}QPushButton:hover{background:#FF8DA1;color:#111;}").arg(bs/2).arg(qMax(10,(int)(13*scale))));
        positionNearPet();
    }

    void stopAndCleanup() {
        m_isRunning = false;
        m_isFinishedReminding = false;
        m_countdownTimer->stop();
        close();
        m_pet->m_idleCounter = 0;
        m_pet->setState(DesktopPet::Idle);
    }

private:
    void initUi() {
        m_mainLayout = new QVBoxLayout(this);
        QHBoxLayout *topBar = new QHBoxLayout();
        topBar->setContentsMargins(0,0,0,0);
        m_titleLabel = new QLabel(QString::fromUtf8("\u8BBE\u5B9A\u5012\u8BA1\u65F6"), this);
        m_titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_closeBtn = new QPushButton(QString::fromUtf8("\u2715"), this);
        m_closeBtn->setCursor(Qt::PointingHandCursor);
        connect(m_closeBtn, &QPushButton::clicked, this, &TimerWindow::closeTimer);
        topBar->addWidget(m_titleLabel);
        topBar->addStretch();
        topBar->addWidget(m_closeBtn);

        m_inputWidget = new QWidget(this);
        QHBoxLayout *inputLay = new QHBoxLayout(m_inputWidget);
        inputLay->setContentsMargins(0,0,0,0);
        m_minInput = new QLineEdit(this);
        m_minInput->setPlaceholderText("0");
        m_minInput->setValidator(new QIntValidator(0, 999, this));
        m_minInput->setAlignment(Qt::AlignCenter);
        m_minUnit = new QLabel(QString::fromUtf8("\u5206"), this);
        m_secInput = new QLineEdit(this);
        m_secInput->setPlaceholderText("0");
        m_secInput->setValidator(new QIntValidator(0, 59, this));
        m_secInput->setAlignment(Qt::AlignCenter);
        m_secUnit = new QLabel(QString::fromUtf8("\u79D2"), this);
        m_confirmBtn = new QPushButton(QString::fromUtf8("\u786E\u8BA4"), this);
        m_confirmBtn->setCursor(Qt::PointingHandCursor);
        connect(m_confirmBtn, &QPushButton::clicked, this, &TimerWindow::startCountdown);
        inputLay->addWidget(m_minInput, 2);
        inputLay->addWidget(m_minUnit);
        inputLay->addWidget(m_secInput, 2);
        inputLay->addWidget(m_secUnit);
        inputLay->addWidget(m_confirmBtn, 2);

        m_statusLabel = new QLabel("", this);
        m_statusLabel->setAlignment(Qt::AlignCenter);
        m_statusLabel->setWordWrap(true);
        m_statusLabel->hide();

        m_mainLayout->addLayout(topBar);
        m_mainLayout->addWidget(m_inputWidget);
        m_mainLayout->addWidget(m_statusLabel);
    }

    void startCountdown() {
        int mins = m_minInput->text().trimmed().toInt();
        int secs = m_secInput->text().trimmed().toInt();
        int total = mins * 60 + secs;
        if (total <= 0) return;
        m_remainingSeconds = total;
        m_isRunning = true;
        m_pet->setState(DesktopPet::Idle);
        m_titleLabel->setText(QString::fromUtf8("\u5012\u8BA1\u65F6\u4E2D"));
        m_inputWidget->hide();
        m_statusLabel->show();
        updateCountdownDisplay();
        m_countdownTimer->start(1000);
    }

    void updateCountdown() {
        m_remainingSeconds--;
        if (m_remainingSeconds <= 0) {
            m_countdownTimer->stop();
            m_isRunning = false;
            m_isFinishedReminding = true;
            m_pet->setState(DesktopPet::Drag);
            m_pet->playSound("alarm.mp3");
            m_titleLabel->setText(QString::fromUtf8("\u63D0\u9192"));
            int fs = qMax(7, (int)(14 * m_scale));
            m_statusLabel->setStyleSheet(QString("color:#FF8DA1;font-weight:bold;font-size:%1px;").arg(fs));
            m_statusLabel->setText(QString::fromUtf8("\u65F6\u95F4\u5230\u4E86\u54E6\uFF0C\u7EC3\u4E60"));
        } else {
            updateCountdownDisplay();
        }
    }

    void updateCountdownDisplay() {
        int mins = m_remainingSeconds / 60;
        int secs = m_remainingSeconds % 60;
        int fs = qMax(8, (int)(22 * m_scale));
        m_statusLabel->setStyleSheet(QString("color:#FF8DA1;font-weight:bold;font-size:%1px;").arg(fs));
        m_statusLabel->setText(QString("%1:%2").arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0')));
    }

    void closeTimer() {
        m_isRunning = false;
        m_isFinishedReminding = false;
        m_countdownTimer->stop();
        close();
        m_pet->m_timerWindow = nullptr;
        m_pet->m_idleCounter = 0;
        m_pet->setState(DesktopPet::Idle);
    }

    void positionNearPet() {
        if (m_pet) {
            QRect pr = m_pet->geometry();
            move(pr.x() + pr.width() + 10, pr.y() + (pr.height() - height())/2);
        }
    }

    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QRectF r(2.0, 2.0, width()-4.0, height()-4.0);
        p.setBrush(QBrush(QColor(0,0,0,255)));
        p.setPen(QPen(PINK, 4));
        p.drawRoundedRect(r, 10, 10);
    }

    DesktopPet *m_pet;
    float m_scale;
    int m_remainingSeconds = 0;
    bool m_isRunning = false;
    bool m_isFinishedReminding = false;
    QTimer *m_countdownTimer = nullptr;
    QVBoxLayout *m_mainLayout = nullptr;
    QLabel *m_titleLabel = nullptr;
    QPushButton *m_closeBtn = nullptr;
    QWidget *m_inputWidget = nullptr;
    QLineEdit *m_minInput = nullptr;
    QLabel *m_minUnit = nullptr;
    QLineEdit *m_secInput = nullptr;
    QLabel *m_secUnit = nullptr;
    QPushButton *m_confirmBtn = nullptr;
    QLabel *m_statusLabel = nullptr;
};

// ========== VolumeSliderWindow ==========
class VolumeSliderWindow : public QWidget {
public:
    VolumeSliderWindow(DesktopPet *pet, float scale)
        : QWidget(nullptr, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
        , m_pet(pet), m_scale(scale)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_DeleteOnClose);
        initUi();
        updateScaleAndPosition(scale);
    }

    void updateScaleAndPosition(float scale) {
        m_scale = scale;
        int sw = qMax(50, (int)(70 * scale));
        int sh = qMax(120, (int)(180 * scale));
        setFixedSize(sw, sh);
        int pad = 6;
        int mlr = qMax(4, (int)(6*scale)), mtb = qMax(4, (int)(6*scale));
        m_mainLayout->setContentsMargins(pad+mlr, pad+mtb, pad+mlr, pad+mtb);
        m_mainLayout->setSpacing(qMax(4, (int)(6*scale)));
        int fs = qMax(8, (int)(11*scale));
        m_volLabel->setStyleSheet(QString("color:#FF8DA1;font-weight:bold;font-size:%1px;").arg(fs));
        int bs = qMax(12, (int)(16*scale));
        m_closeBtn->setFixedSize(bs, bs);
        m_closeBtn->setStyleSheet(QString("QPushButton{background:transparent;color:#FF8DA1;border:1px solid #FF8DA1;border-radius:%1px;font-weight:bold;font-size:%2px;padding:0;}QPushButton:hover{background:#FF8DA1;color:#111;}").arg(bs/2).arg(qMax(7,(int)(9*scale))));
        int swd = qMax(8, (int)(12*scale)), hd = qMax(16, (int)(24*scale));
        m_slider->setStyleSheet(QString("QSlider::groove:vertical{background:#222;border:1px solid #FF8DA1;width:%1px;border-radius:%2px;}QSlider::add-page:vertical{background:#FF8DA1;border-radius:%2px;}QSlider::handle:vertical{background:#FF8DA1;border:1px solid #fff;height:%3px;margin-left:-%4px;margin-right:-%4px;border-radius:%5px;}QSlider::handle:vertical:hover{background:#FFA5B5;}").arg(swd).arg(swd/2).arg(hd).arg(hd/4).arg(hd/2));
        positionNearPet();
    }

private:
    void initUi() {
        m_mainLayout = new QVBoxLayout(this);
        QHBoxLayout *topBar = new QHBoxLayout();
        topBar->setContentsMargins(0,0,0,0);
        topBar->addStretch();
        m_closeBtn = new QPushButton(QString::fromUtf8("\u2715"), this);
        m_closeBtn->setCursor(Qt::PointingHandCursor);
        connect(m_closeBtn, &QPushButton::clicked, this, &VolumeSliderWindow::closeVolumeWindow);
        topBar->addWidget(m_closeBtn);
        m_slider = new QSlider(Qt::Vertical, this);
        m_slider->setRange(0, 100);
        m_slider->setValue(m_pet->globalVolume());
        m_slider->setCursor(Qt::PointingHandCursor);
        connect(m_slider, &QSlider::valueChanged, this, &VolumeSliderWindow::onVolumeChanged);
        m_volLabel = new QLabel(QString("%1%").arg(m_pet->globalVolume()), this);
        m_volLabel->setAlignment(Qt::AlignCenter);
        m_mainLayout->addLayout(topBar);
        m_mainLayout->addWidget(m_slider, 1, Qt::AlignCenter);
        m_mainLayout->addWidget(m_volLabel);
    }

    void onVolumeChanged(int value) {
        m_volLabel->setText(QString("%1%").arg(value));
        m_pet->setGlobalVolume(value);
    }

    void closeVolumeWindow() {
        m_pet->m_volumeWindow = nullptr;
        close();
    }

    void positionNearPet() {
        if (m_pet) {
            QRect pr = m_pet->geometry();
            move(pr.x() + pr.width() + 10, pr.y() + (pr.height() - height())/2);
        }
    }

    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QRectF r(2.0, 2.0, width()-4.0, height()-4.0);
        p.setBrush(QBrush(QColor(0,0,0,255)));
        p.setPen(QPen(PINK, 4));
        p.drawRoundedRect(r, 10, 10);
    }

    DesktopPet *m_pet;
    float m_scale;
    QVBoxLayout *m_mainLayout = nullptr;
    QPushButton *m_closeBtn = nullptr;
    QSlider *m_slider = nullptr;
    QLabel *m_volLabel = nullptr;
};

// ========== AuthorWindow ==========
class AuthorWindow : public QWidget {
public:
    AuthorWindow(DesktopPet *pet, float scale)
        : QWidget(nullptr, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
        , m_pet(pet), m_scale(scale)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_DeleteOnClose);
        m_mainLayout = new QVBoxLayout(this);
        QHBoxLayout *topBar = new QHBoxLayout();
        topBar->setContentsMargins(0,0,0,0);
        topBar->addStretch();
        m_closeBtn = new QPushButton(QString::fromUtf8("\u2715"), this);
        m_closeBtn->setCursor(Qt::PointingHandCursor);
        connect(m_closeBtn, &QPushButton::clicked, this, &AuthorWindow::closeAuthorWindow);
        topBar->addWidget(m_closeBtn);
        m_imgLabel = new QLabel(this);
        m_imgLabel->setAlignment(Qt::AlignCenter);
        m_textLabel = new QLabel(QString::fromUtf8("\u684C\u5BA0\u8BBE\u8BA1\u4F5C\u8005\uFF1Ab\u7AD9\u8BC9\u8BF4\u65B0\u8BED\uFF0C\u6B64\u684C\u5BA0\u4E3A\u514D\u8D39\u684C\u5BA0\n\u5982\u679C\u559C\u6B22\u8FD9\u4E2A\u684C\u5BA0\u7684\u8BDD\u8BB0\u5F97\u7ED9up\u70B9\u70B9\u5173\u6CE8\u54E6\n\u7531b\u7AD9\u661F\u5149-k\u4F7F\u7528AI\u5DE5\u5177\u79FB\u690D\u5E76\u4F18\u5316"), this);
        m_textLabel->setAlignment(Qt::AlignCenter);
        m_textLabel->setWordWrap(true);
        m_mainLayout->addLayout(topBar);
        m_mainLayout->addWidget(m_imgLabel, 1);
        m_mainLayout->addWidget(m_textLabel);

        QString authorPath = m_pet->resolveResourcePath(":/drawing/author.png");
        m_authorPix = QPixmap(authorPath);
        updateScaleAndPosition(scale);
    }

    void updateScaleAndPosition(float scale) {
        m_scale = scale;
        int sw = qMax(200, (int)(280 * scale));
        int sh = qMax(150, (int)(200 * scale));
        setFixedSize(sw, sh);
        int pad = 6;
        int mlr = qMax(8, (int)(12*scale)), mtb = qMax(6, (int)(9*scale));
        m_mainLayout->setContentsMargins(pad+mlr, pad+mtb, pad+mlr, pad+mtb);
        m_mainLayout->setSpacing(qMax(4, (int)(6*scale)));
        int bs = qMax(12, (int)(16*scale));
        m_closeBtn->setFixedSize(bs, bs);
        m_closeBtn->setStyleSheet(QString("QPushButton{background:transparent;color:#FF8DA1;border:1px solid #FF8DA1;border-radius:%1px;font-weight:bold;font-size:%2px;padding:0;}QPushButton:hover{background:#FF8DA1;color:#fff;}").arg(bs/2).arg(qMax(7,(int)(9*scale))));
        if (!m_authorPix.isNull()) {
            int iw = qMax(50, (int)(sw * 0.75));
            int ih = qMax(50, (int)(sh * 0.45));
            m_imgLabel->setPixmap(m_authorPix.scaled(iw, ih, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        int fs = qMax(8, (int)(11*scale));
        m_textLabel->setStyleSheet(QString("color:#FF8DA1;font-weight:bold;font-size:%1px;").arg(fs));
        positionNearPet();
    }

private:
    void closeAuthorWindow() {
        m_pet->m_authorWindow = nullptr;
        close();
    }

    void positionNearPet() {
        if (m_pet) {
            QRect pr = m_pet->geometry();
            move(pr.x() + pr.width() + 10, pr.y() + (pr.height() - height())/2);
        }
    }

    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QRectF r(2.0, 2.0, width()-4.0, height()-4.0);
        p.setBrush(QBrush(QColor(255,255,255,255)));
        p.setPen(QPen(PINK, 4));
        p.drawRoundedRect(r, 10, 10);
    }

    DesktopPet *m_pet;
    float m_scale;
    QVBoxLayout *m_mainLayout = nullptr;
    QPushButton *m_closeBtn = nullptr;
    QLabel *m_imgLabel = nullptr;
    QLabel *m_textLabel = nullptr;
    QPixmap m_authorPix;
};

// ====== ResourceWindow ======
// External file replacement window.
// Each resource row has: replace | open folder | delete
// "全部替换" picks a directory and copies all matching files.
class ResourceWindow : public QWidget {
public:
    ResourceWindow(DesktopPet *pet, float scale)
        : QWidget(nullptr, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
        , m_pet(pet), m_scale(scale)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_DeleteOnClose);
        initUi();
        updateScaleAndPosition(scale);
    }

    void updateScaleAndPosition(float scale) {
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

private:
    struct ResourceDef { QString key; QString name; QString category; QString subDir; QString ext; };

    void copyFileToOverride(const QString &src, const QString &subDir, const QString &baseName, const QString &ext) {
        QDir d(m_pet->m_resourceDir + subDir);
        if (!d.exists()) d.mkpath(".");
        QString dst = d.absoluteFilePath(baseName + ext);
        if (QFile::exists(dst)) QFile::remove(dst);
        QFile::copy(src, dst);
    }

    void refreshRow(QLabel *sl, QPushButton *db, const QString &key) {
        bool s = m_pet->m_resourceOverrides.contains(key);
        sl->setText(s ? QString::fromUtf8("\u5df2\u66ff\u6362") : QString::fromUtf8("\u9ed8\u8ba4"));
        sl->setStyleSheet(s ? "color:#5f5;" : "color:#888;");
        db->setEnabled(s);
    }

    void initUi() {
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
            QPointer<QLabel> slPtr = sl;
            connect(rb, &QPushButton::clicked, this, [this, k, sd, bn, ext, slPtr]() {
                QString filter = QString("*%1").arg(ext);
                QString fp = QFileDialog::getOpenFileName(this, QString::fromUtf8("\u9009\u62e9\u66ff\u6362\u6587\u4ef6"), QString(), filter);
                if (fp.isEmpty()) return;
                copyFileToOverride(fp, sd, bn, ext);
                m_pet->refreshOverrides();
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
            connect(ob, &QPushButton::clicked, this, [this, sd]() { QString p = m_pet->m_resourceDir + sd; QDir().mkpath(p); QDesktopServices::openUrl(QUrl::fromLocalFile(p)); });

            // Delete button
            QPushButton *db = new QPushButton(QString::fromUtf8("\u5220\u9664"));
            db->setCursor(Qt::PointingHandCursor); db->setEnabled(over); db->setFixedWidth(qMax(28,(int)(38*m_scale)));
            connect(db, &QPushButton::clicked, this, [this, k, sl, db]() {
                QString fp = m_pet->m_resourceOverrides.value(k);
                if (!fp.isEmpty() && QFile::exists(fp)) QFile::remove(fp);
                m_pet->refreshOverrides();
                refreshRow(sl, db, k);
            });
            m_allBtns.append(rb); m_allBtns.append(ob); m_allBtns.append(db);
            lay->addWidget(nl); lay->addWidget(sl); lay->addWidget(rb); lay->addWidget(ob); lay->addWidget(db);
            QListWidgetItem *it = new QListWidgetItem();
            it->setSizeHint(row->sizeHint()); m_resList->addItem(it); m_resList->setItemWidget(it, row);
        }
        m_mainLayout->addLayout(topBar); m_mainLayout->addWidget(hint); m_mainLayout->addWidget(m_resList, 1);
    }

    void onReplaceAll() {
        QString dir = QFileDialog::getExistingDirectory(this, QString::fromUtf8("\u9009\u62e9\u5305\u542b\u66ff\u6362\u6587\u4ef6\u7684\u76ee\u5f55"));
        if (dir.isEmpty()) return;
        QDir d(dir);
        int count = 0;
        for (int i = 0; i < m_resList->count(); i++) {
            QListWidgetItem *it = m_resList->item(i);
            QWidget *row = m_resList->itemWidget(it);
            if (!row || !row->layout()) continue;
            QHBoxLayout *lay = qobject_cast<QHBoxLayout*>(row->layout());
            if (!lay || lay->count() < 3) continue;
            QLabel *nl = qobject_cast<QLabel*>(lay->itemAt(0)->widget());
            if (!nl) continue;
            QString name = nl->text();
            int dash = name.indexOf(" - ");
            if (dash < 0) continue;
            QString baseName = name.mid(dash + 3);
            QStringList exts = {".gif",".wav",".png"};
            for (const QString &ext : exts) {
                QFileInfo fi(d.filePath(baseName + ext));
                if (fi.exists() && fi.isFile()) {
                    QLabel *sl = qobject_cast<QLabel*>(lay->itemAt(1)->widget());
                    QPushButton *db = nullptr;
                    for (int j = 0; j < lay->count(); j++) {
                        auto *w = lay->itemAt(j)->widget();
                        if (auto *b = qobject_cast<QPushButton*>(w)) {
                            if (b->text() == QString::fromUtf8("\u5220\u9664")) { db = b; break; }
                        }
                    }
                    QString sd = (ext == ".gif") ? "gif" : (ext == ".wav") ? "audio" : "";
                    if (sd.isEmpty()) {
                        if (baseName.startsWith("card_")) sd = "cards";
                        else if (baseName.startsWith("drawing_") || baseName == "author") sd = "drawing";
                        else continue;
                    }
                    copyFileToOverride(fi.absoluteFilePath(), sd, baseName, ext);
                    if (sl && db) {
                        QString key = sd + "/" + baseName;
                        m_pet->refreshOverrides();
                        refreshRow(sl, db, key);
                    }
                    count++;
                    break;
                }
            }
        }
        QMessageBox::information(this, QString::fromUtf8("\u5168\u90e8\u66ff\u6362"), QString::fromUtf8("\u5df2\u66ff\u6362 %1 \u4e2a\u6587\u4ef6").arg(count));
    }

    void onOpenAll() {
        QString dir = m_pet->m_resourceDir;
        if (dir.endsWith(QChar('/'))) dir.chop(1);
        QDir d(dir);
        if (!d.exists()) d.mkpath(".");
        // Ensure all resource subdirectories exist (same logic as per-row open buttons)
        QStringList subDirs = {"gif", "audio", "cards", "drawing"};
        for (const QString &sd : subDirs)
            d.mkpath(sd);
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
    }

    void closeResourceWindow() { m_pet->m_resourceWindow = nullptr; close(); }
    void positionNearPet() {
        if (m_pet) { QRect pr = m_pet->geometry(); move(pr.x()+pr.width()+10, pr.y()+(pr.height()-height())/2); }
    }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        QRectF r(2.0,2.0,width()-4.0,height()-4.0);
        p.setBrush(QBrush(QColor(0,0,0,255))); p.setPen(QPen(PINK,4)); p.drawRoundedRect(r,10,10);
    }
    DesktopPet *m_pet; float m_scale;
    QVBoxLayout *m_mainLayout = nullptr; QLabel *m_titleLabel = nullptr;
    QPushButton *m_closeBtn = nullptr; QPushButton *m_replaceAllBtn = nullptr; QPushButton *m_openAllBtn = nullptr;
    QListWidget *m_resList = nullptr;
    QList<QPushButton*> m_allBtns;
};

// ========== DesktopPet ==========

DesktopPet::DesktopPet(QWidget *parent) : QLabel(parent) {
#ifdef Q_OS_MAC
    // Qt::Tool on macOS causes transparent frameless windows to disappear when
    // mouse transparency is enabled. Qt::Dialog avoids this issue.
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Dialog);
#else
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
#endif
    setAttribute(Qt::WA_TranslucentBackground);

    m_audioDir = QApplication::applicationDirPath() + "/../audio/";
    m_cardsDir = ":/cards/";
    m_drawingDir = ":/drawing/";
    m_resourceDir = QApplication::applicationDirPath() + "/resources/";

    m_fontFamily = QSettings().value("fontFamily").toString(); m_fontSize = QSettings().value("fontSize", -1).toInt(); m_fontBold = QSettings().value("fontBold", true).toBool();
    m_systemDefaultFont = QApplication::font();
    applyFontPreference();

    preloadNativeSizes();
    setMinimumSize(140, 100);

    preloadSounds();

    m_idleTimer = new QTimer(this);
    connect(m_idleTimer, &QTimer::timeout, this, &DesktopPet::checkIdle);
    m_idleTimer->start(1000);

    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setInterval(30000);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &DesktopPet::saveSettings);
    m_autoSaveTimer->start();

    setState(Idle);

    connect(qApp, &QApplication::aboutToQuit, this, &DesktopPet::saveSettings);

    setupTrayIcon();

    playSound("start.mp3");
    show();

    loadSettings();
    loadOverrides();
 }

DesktopPet::~DesktopPet() {
    if (m_movie) {
        disconnect(m_movie, &QMovie::frameChanged, this, &DesktopPet::manualPaintFrame);
        m_movie->stop();
    }
}

void DesktopPet::preloadNativeSizes() {
    QStringList files = {
        resolveResourcePath(":/gif/idle.gif"),
        resolveResourcePath(":/gif/click.gif"),
        resolveResourcePath(":/gif/drag.gif"),
        resolveResourcePath(":/gif/sleep.gif"),
        resolveResourcePath(":/gif/draw.gif"),
        resolveResourcePath(":/gif/result.gif")
    };
    State stateEnums[] = {Idle, Click, Drag, Sleep, Result, Result};
    int maxW = 0, maxH = 0, maxArea = 0;
    for (int i = 0; i < 6; i++) {
        QMovie temp(files[i]);
        temp.jumpToFrame(0);
        QSize sz = temp.frameRect().size();
        if (sz.isValid() && sz.width() > 0) {
            m_nativeSizes[stateEnums[i]] = sz;
            int area = sz.width() * sz.height();
            if (area > maxArea) { maxArea = area; maxW = sz.width(); maxH = sz.height(); }
        }
    }
    m_maxNativeSize = QSize(maxW, maxH);
    if (maxW <= 0) m_maxNativeSize = QSize(300, 300);
}

void DesktopPet::setState(State state) {
    if (m_state == state && m_movie && m_movie->state() == QMovie::Running) return;

    QString gifPath;
    switch (state) {
    case Idle:   gifPath = resolveResourcePath(":/gif/idle.gif"); break;
    case Click:  gifPath = resolveResourcePath(":/gif/click.gif"); break;
    case Drag:   gifPath = resolveResourcePath(":/gif/drag.gif"); break;
    case Sleep:  gifPath = resolveResourcePath(":/gif/sleep.gif"); break;
    case Result: gifPath = resolveResourcePath(":/gif/result.gif"); break;
    }

    if (m_movie) {
        disconnect(m_movie, &QMovie::frameChanged, this, &DesktopPet::manualPaintFrame);
        m_movie->stop();
        delete m_movie;
        m_movie = nullptr;
    }

    QSize orig = m_nativeSizes.value(state, m_maxNativeSize);
    int cw = qMax(20, (int)(orig.width() * m_scale));
    int ch = qMax(20, (int)(orig.height() * m_scale));
    if (width() != cw || height() != ch) {
        int oldRight = x() + width();
        cw = qMax(140, cw); ch = qMax(100, ch);
        setFixedSize(cw, ch);
        move(oldRight - cw, y());
    }
    if (orig.isValid() && orig.width() > 0) {
        m_currentTargetSize = QSize(qMax(10, (int)(orig.width() * m_scale)),
                                     qMax(10, (int)(orig.height() * m_scale)));
    }

    m_movie = new QMovie(gifPath, QByteArray(), this);
    m_state = state;
    connect(m_movie, &QMovie::frameChanged, this, &DesktopPet::manualPaintFrame);
    m_movie->start();
}

void DesktopPet::manualPaintFrame(int) {
    if (!m_movie || m_currentTargetSize.isEmpty()) return;
    int cw = width(), ch = height();
    if (cw <= 0 || ch <= 0) return;
    QPixmap curr = m_movie->currentPixmap();
    if (curr.isNull()) return;
    QPixmap scaled = curr.scaled(m_currentTargetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPixmap canvas(cw, ch);
    canvas.fill(Qt::transparent);
    QPainter p(&canvas);
    p.setRenderHint(QPainter::Antialiasing);
    p.drawPixmap(cw - scaled.width(), ch - scaled.height(), scaled);
    p.end();
    setPixmap(canvas);
}

void DesktopPet::applyScale() {
    if (!m_maxNativeSize.isValid()) return;
    int oldRight = x() + width();
    int cw = qMax(20, (int)(m_maxNativeSize.width() * m_scale));
    int ch = qMax(20, (int)(m_maxNativeSize.height() * m_scale));
    cw = qMax(140, cw); ch = qMax(100, ch);
    setFixedSize(cw, ch);
    move(oldRight - cw, y());
    QSize orig = m_nativeSizes.value(m_state);
    if (orig.isValid() && orig.width() > 0) {
        m_currentTargetSize = QSize(qMax(10, (int)(orig.width() * m_scale)),
                                     qMax(10, (int)(orig.height() * m_scale)));
    }
    if (m_movie) manualPaintFrame(m_movie->currentFrameNumber());
    updateSideWindowPositions();
}
void DesktopPet::applyScaleGeometry() {
    if (!m_maxNativeSize.isValid()) return;
    int cx = x() + width() / 2;
    int cy = y() + height() / 2;
    QSize orig = m_nativeSizes.value(m_state, m_maxNativeSize);
    int cw = qMax(60, (int)(orig.width() * m_scale));
    int ch = qMax(60, (int)(cw * orig.height() / orig.width()));
    cw = qMax(140, cw); ch = qMax(100, ch);
    setFixedSize(cw, ch);
    move(cx - cw / 2, cy - ch / 2);
    if (orig.isValid() && orig.width() > 0) {
        m_currentTargetSize = QSize(cw, ch);
    }
}

void DesktopPet::applyScaleRender() {
    if (m_movie) manualPaintFrame(m_movie->currentFrameNumber());
    updateSideWindowPositions();
}

void DesktopPet::resetScale() {
    playSound("reset.mp3");
    m_scale = 1.0f;
    applyScale();
    saveSettings();
}

void DesktopPet::updateSideWindowPositions() {
    if (auto *w = dynamic_cast<DrawEffectWindow*>(m_effectWindow.data()))
        if (w->isVisible()) w->updateScaleAndPosition(m_scale);
    if (auto *w = dynamic_cast<TimerWindow*>(m_timerWindow.data()))
        if (w->isVisible()) w->updateScaleAndPosition(m_scale);
    if (auto *w = dynamic_cast<DrawingEffectWindow*>(m_drawingWindow.data()))
        if (w->isVisible()) w->updateScaleAndPosition(m_scale);
    if (auto *w = dynamic_cast<VolumeSliderWindow*>(m_volumeWindow.data()))
        if (w->isVisible()) w->updateScaleAndPosition(m_scale);
    if (auto *w = dynamic_cast<AuthorWindow*>(m_authorWindow.data()))
        if (w->isVisible()) w->updateScaleAndPosition(m_scale);
    if (auto *w = dynamic_cast<ResourceWindow*>(m_resourceWindow.data()))
        if (w->isVisible()) w->updateScaleAndPosition(m_scale);
}

// ---------- Idle ----------

void DesktopPet::checkIdle() {
    bool anyWindowOpen = m_isDrawingCard
        || (m_timerWindow && m_timerWindow->isVisible())
        || (m_volumeWindow && m_volumeWindow->isVisible())
        || (m_authorWindow && m_authorWindow->isVisible())
        || (m_resourceWindow && m_resourceWindow->isVisible())
        || (m_effectWindow && m_effectWindow->isVisible())
        || (m_drawingWindow && m_drawingWindow->isVisible());
    if (!m_dragging && !anyWindowOpen) {
        m_idleCounter++;
        if (m_idleCounter >= 4) setState(Sleep);
    }
}

// ---------- Mouse Events ----------

void DesktopPet::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_idleCounter = 0;
        m_dragStart = event->globalPosition().toPoint();
        if (!m_isDrawingCard) {
            bool timerActive = m_timerWindow && m_timerWindow->isVisible();
            if (!timerActive) setState(Drag);
        }
    }
}

void DesktopPet::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        QPoint delta = event->globalPosition().toPoint() - m_dragStart;
        m_dragStart = event->globalPosition().toPoint();
        move(x() + delta.x(), y() + delta.y());
        updateSideWindowPositions();
    }
}

void DesktopPet::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;
        m_idleCounter = 0;
        saveSettings();
        if (!m_isDrawingCard) {
            bool timerActive = m_timerWindow && m_timerWindow->isVisible();
            if (!timerActive && m_state == Drag) {
                setState(Click);
                QTimer::singleShot(2000, this, [this]() {
                    if (!m_isDrawingCard) {
                        bool ta = m_timerWindow && m_timerWindow->isVisible();
                        if (!ta) setState(Idle);
                    }
                });
            }
        }
    }
}

void DesktopPet::wheelEvent(QWheelEvent *event) {
    float delta = event->angleDelta().y() / 1200.0f;
    m_scale = qBound(m_minScale, m_scale + delta, m_maxScale);
    event->accept();
    applyScaleGeometry();
    if (!m_scaleTimer) {
        m_scaleTimer = new QTimer(this);
        m_scaleTimer->setSingleShot(true);
        m_scaleTimer->setInterval(0);
        connect(m_scaleTimer, &QTimer::timeout, this, &DesktopPet::applyScaleRender);
    }
    m_scaleTimer->start();
    saveSettings();
}

void DesktopPet::contextMenuEvent(QContextMenuEvent *) {
    QMenu menu(this);
    int fs = qMax(9, (int)(15 * m_scale));
    int pv = qMax(4, (int)(8 * m_scale));
    int ph = qMax(12, (int)(24 * m_scale));
    int mv = qMax(1, (int)(3 * m_scale));
    int mh = qMax(3, (int)(6 * m_scale));
    int br = qMax(5, (int)(10 * m_scale));
    int ibr = qMax(2, (int)(5 * m_scale));
    int mp = qMax(2, (int)(6 * m_scale));
    int bw = qMax(1, (int)(2 * m_scale));
    int smv = qMax(2, (int)(5 * m_scale));
    int smh = qMax(5, (int)(10 * m_scale));
    menu.setStyleSheet(menuStylesheet(fs,pv,ph,mv,mh,br,ibr,mp,bw,smv,smh));

    QAction *drawAction = menu.addAction(QString::fromUtf8("\u2728 \u5E78\u8FD4\u62BD\u5361"));
    QAction *timerAction = menu.addAction(QString::fromUtf8("\u23F0 \u95F9\u949F\u8BA1\u65F6"));
    QAction *drawingAction = menu.addAction(QString::fromUtf8("\U0001F3A8 \u968F\u673A\u753B\u753B"));
    QAction *volumeAction = menu.addAction(QString::fromUtf8("\U0001F50A \u97F3\u91CF\u8C03\u8282"));
    QAction *authorAction = menu.addAction(QString::fromUtf8("\u2117 \u5236\u4F5C\u58F0\u660E"));
    QAction *resourceAction = menu.addAction(QString::fromUtf8("\U0001F4C1 \u8D44\u6E90\u66FF\u6362"));

    if (m_isDrawingCard) {
        drawAction->setEnabled(false);
        drawingAction->setEnabled(false);
    }

    menu.addSeparator();
    QMenu *fontMenu = menu.addMenu("字体");
    QAction *fontDefault = fontMenu->addAction("系统默认");
    fontDefault->setCheckable(true);
    if (m_fontFamily.isEmpty()) fontDefault->setChecked(true);
    fontMenu->addSeparator();
    QWidgetAction *fontListAction = new QWidgetAction(fontMenu);
    QListWidget *fontList = new QListWidget();
    fontList->setMaximumHeight(qMax(150, (int)(350 * m_scale)));
    fontList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    fontList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    fontList->setStyleSheet(
        QString("QListWidget{background:#FF8DA1;border:none;outline:none;}"
                "QListWidget::item{color:#fff;font-weight:bold;font-size:%1px;"
                "padding:%2px %3px;margin:%4px %5px;border-radius:%6px;}"
                "QListWidget::item:hover{background:#FF6B8B;}"
                "QScrollBar:vertical{width:6px;background:transparent;}"
                "QScrollBar::handle:vertical{background:#fff;border-radius:3px;min-height:20px;}"
                "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}")
            .arg(fs).arg(pv).arg(ph).arg(mv).arg(mh).arg(ibr));
    QFontDatabase fontDb;
    QStringList commonCN = {"微软雅黑","宋体","黑体","楷体","仿宋","等线","新宋体","幼圆","隶书","华文楷体","华文宋体","华文仿宋","华文细黑","华文新魏","华文行楷","华文中宋","方正黑体","方正书宋","方正仿宋","方正楷体","方正隶书","方正姚体","方正舒体","思源黑体","思源宋体","Microsoft YaHei","SimSun","SimHei","KaiTi","FangSong","DengXian","NSimSun","YouYuan","LiSu","Noto Sans CJK SC","Noto Serif CJK SC"};
    QStringList cjkCommon, cjkRest, other;
    for (const QString &f : fontDb.families()) {
        if (commonCN.contains(f)) { cjkCommon << f; continue; }
        QList<QFontDatabase::WritingSystem> ws = fontDb.writingSystems(f);
        bool isCJK = false;
        for (auto w : ws) {
            if (w == QFontDatabase::SimplifiedChinese || w == QFontDatabase::TraditionalChinese ||
                w == QFontDatabase::Japanese || w == QFontDatabase::Korean)
            { isCJK = true; break; }
        }
        if (isCJK) cjkRest << f; else other << f;
    }
    auto addHeader = [&](const QString &title) {
        QListWidgetItem *h = new QListWidgetItem(title);
        h->setFlags(Qt::NoItemFlags);
        h->setForeground(QColor("#FFD0D8"));
        h->setTextAlignment(Qt::AlignCenter);
        fontList->addItem(h);
    };
    if (!cjkCommon.isEmpty()) { addHeader(QString::fromUtf8("\u2501\u2501 \u5E38\u7528\u4E2D\u6587\u5B57\u4F53 \u2501\u2501")); fontList->addItems(cjkCommon); }
    if (!cjkRest.isEmpty())   { addHeader(QString::fromUtf8("\u2501\u2501 \u5176\u4ED6CJK\u5B57\u4F53 \u2501\u2501")); fontList->addItems(cjkRest); }
    if (!other.isEmpty())     { addHeader(QString::fromUtf8("\u2501\u2501 \u5176\u4ED6\u5B57\u4F53 \u2501\u2501")); fontList->addItems(other); }
    if (!m_fontFamily.isEmpty()) {
        for (int i = 0; i < fontList->count(); i++) { QListWidgetItem *it = fontList->item(i); if ((it->flags() & Qt::ItemIsSelectable) && it->text() == m_fontFamily) { it->setSelected(true); break; } } //
        
    }
    connect(fontList, &QListWidget::itemClicked, fontMenu, [fontMenu, this](QListWidgetItem *item) {
        if (!(item->flags() & Qt::ItemIsSelectable)) return; m_fontFamily = item->text();
        QSettings().setValue("fontFamily", m_fontFamily);
        applyFontPreference();
        fontMenu->close();
    });
    fontListAction->setDefaultWidget(fontList);
    fontMenu->addAction(fontListAction);
    QMenu *sizeSubMenu = menu.addMenu(QString::fromUtf8("\u5B57\u4F53\u5927\u5C0F"));
    QWidgetAction *sizeSliderAction = new QWidgetAction(sizeSubMenu);
    QWidget *sizeWidget = new QWidget();
    QHBoxLayout *sizeLayout = new QHBoxLayout(sizeWidget);
    int sm = qMax(2,(int)(6*m_scale));
    sizeLayout->setContentsMargins(qMax(4,(int)(8*m_scale)),2,qMax(4,(int)(8*m_scale)),2);
    sizeLayout->setSpacing(sm);
    QSlider *sizeSlider = new QSlider(Qt::Horizontal); sizeSlider->setRange(8,36);
    int initSize = (m_fontSize>0) ? m_fontSize : fs; sizeSlider->setValue(initSize);
    sizeSlider->setStyleSheet(QString("QSlider::groove:horizontal{height:4px;background:#222;border:1px solid #FF8DA1;border-radius:2px;}QSlider::handle:horizontal{background:#FF8DA1;border:1px solid #fff;width:12px;margin:-4px 0;border-radius:6px;}"));
    QLabel *sizeValue = new QLabel(QString("%1px").arg(initSize)); sizeValue->setStyleSheet(QString("color:#fff;font-weight:bold;font-size:%1px;min-width:32px;").arg(fs));
    sizeLayout->addWidget(sizeSlider,1); sizeLayout->addWidget(sizeValue);
    connect(sizeSlider, &QSlider::valueChanged, sizeSubMenu, [&menu, sizeSubMenu, fs, pv, ph, mv, mh, br, ibr, mp, bw, smv, smh, sizeValue, sizeWidget, this](int val) {
        sizeValue->setText(QString("%1px").arg(val));
        sizeValue->setStyleSheet(QString("color:#fff;font-weight:bold;font-size:%1px;min-width:32px;").arg(val));
        m_fontSize = val;
        QString ss = menuStylesheet(val,pv,ph,mv,mh,br,ibr,mp,bw,smv,smh);
        menu.setStyleSheet(ss);
        sizeSubMenu->setStyleSheet(ss);
        applyFontPreference();
    });
    sizeSliderAction->setDefaultWidget(sizeWidget);
    sizeSubMenu->addAction(sizeSliderAction);
    QMenu *weightMenu = menu.addMenu(QString::fromUtf8("\u5B57\u4F53\u7C97\u7EC6"));
    QActionGroup *weightGroup = new QActionGroup(weightMenu); weightGroup->setExclusive(true);
    QAction *boldAct = weightMenu->addAction(QString::fromUtf8("\u7C97\u4F53")); boldAct->setCheckable(true); boldAct->setActionGroup(weightGroup);
    QAction *normalAct = weightMenu->addAction(QString::fromUtf8("\u6B63\u5E38")); normalAct->setCheckable(true); normalAct->setActionGroup(weightGroup);
    if (m_fontBold) boldAct->setChecked(true); else normalAct->setChecked(true);
    menu.addSeparator();
    QAction *topAction = menu.addAction(QString::fromUtf8("\u7F6E\u9876\u663E\u793A"));
    topAction->setCheckable(true);
    topAction->setChecked(m_stayOnTop);
    QAction *mouseAction = menu.addAction(QString::fromUtf8("\u9F20\u6807\u7A7F\u900F"));
    mouseAction->setCheckable(true);
    mouseAction->setChecked(m_mouseTransparent);
    QAction *resetAction = menu.addAction(QString::fromUtf8("重置大小 (100%)"));
    menu.addSeparator();
    QAction *hideAction = menu.addAction(QString::fromUtf8("隐藏桌宠"));
    QAction *quitAction = menu.addAction(QString::fromUtf8("退出桌宠"));

    QRect pr = geometry();
    QSize ms = menu.sizeHint();
    int mx = pr.x() + pr.width() + 10;
    int my = pr.y() + (pr.height() - ms.height()) / 2;
    QAction *chosen = menu.exec(QPoint(mx, my));

    if (!chosen) return;
    if (chosen == drawAction) startDrawCard();
    else if (chosen == timerAction) startTimerFeature();
    else if (chosen == drawingAction) startDrawingFeature();
    else if (chosen == volumeAction) startVolumeFeature();
    else if (chosen == authorAction) startAuthorFeature();
    else if (chosen == resourceAction) startResourceFeature();
    else if (chosen == topAction) toggleStayOnTop();
    else if (chosen == mouseAction) toggleMouseTransparent();
    else if (chosen == resetAction) resetScale();
    else if (chosen == fontDefault) {
        m_fontFamily.clear();
        QSettings().setValue("fontFamily", "");
        applyFontPreference();
    }
    else if (weightGroup->actions().contains(chosen)) {
        m_fontBold = (chosen == boldAct);
        QSettings().setValue("fontBold", m_fontBold);
        applyFontPreference();
    }
    else if (chosen == hideAction) hide();
    else if (chosen == quitAction) qApp->exit(0);
    QSettings().setValue("fontSize", m_fontSize);
}

QString DesktopPet::menuStylesheet(int fs, int pv, int ph, int mv, int mh, int br, int ibr, int mp, int bw, int smv, int smh) {
    QString ff = m_fontFamily.isEmpty() ? QString() : QString("font-family:'%1';").arg(m_fontFamily);
    return QString("QMenu{background:#FF8DA1;border:%1px solid #fff;border-radius:%2px;padding:%3px 0;}"
        "QMenu::item{background:transparent;color:#fff;%4font-size:%5px;font-weight:%13;padding:%6px %7px;margin:%8px %9px;border-radius:%10px;}"
        "QMenu::item:selected{background:#FF6B8B;}"
        "QMenu::item:disabled{color:#FFC0CB;}"
        "QMenu::separator{height:1px;background:#fff;margin:%11px %12px;}")
        .arg(bw).arg(br).arg(mp).arg(ff).arg(m_fontSize > 0 ? m_fontSize : fs).arg(pv).arg(ph).arg(mv).arg(mh).arg(ibr).arg(smv).arg(smh).arg(m_fontBold ? "bold" : "normal");
}

void DesktopPet::applyFontPreference() {
    QFont f = m_fontFamily.isEmpty() ? m_systemDefaultFont : QFont(m_fontFamily);
    if (m_fontSize > 0) f.setPixelSize(m_fontSize);
    f.setBold(m_fontBold);
    f.setStyleStrategy(QFont::PreferAntialias);
    qApp->setFont(f);
}

void DesktopPet::preloadSounds() {
    QStringList files = {"start.mp3", "draw.mp3", "drawing.mp3", "result.mp3", "reset.mp3", "alarm.mp3", "clock.mp3"};
    for (const QString &f : files) {
        auto *effect = new QSoundEffect(this);
        effect->setSource(QUrl(resolveResourcePath("qrc:/audio/" + f)));
        effect->setVolume(m_volume / 100.0f);
        m_sounds[f] = effect;
    }
}

void DesktopPet::playSound(const QString &file, bool override) {
    if (!m_sounds.contains(file)) return;
    if (override) {
        for (auto *s : m_sounds) s->stop();
    }
    m_sounds[file]->play();
}

void DesktopPet::setGlobalVolume(int vol) {
    m_volume = vol;
    for (auto *s : m_sounds) s->setVolume(vol / 100.0f);
    saveSettings();
}

// ---------- Features ----------

void DesktopPet::startDrawCard() {
    if (m_isDrawingCard) return;
    closeOtherSideWindows();
    m_isDrawingCard = true;
    m_idleCounter = 0;
    setState(Idle);
    QApplication::processEvents();
    playSound("draw.mp3");
    m_effectWindow = new DrawEffectWindow(this, m_cardsDir, m_scale);
    static_cast<DrawEffectWindow*>(m_effectWindow.data())->startShow();
}

void DesktopPet::startTimerFeature() {
    if (m_isDrawingCard) return;
    closeOtherSideWindows();
    setState(Click);
    playSound("clock.mp3", false);
    m_idleCounter = 0;
    m_timerWindow = new TimerWindow(this, m_scale);
    m_timerWindow->show();
}

void DesktopPet::startDrawingFeature() {
    if (m_isDrawingCard) return;
    closeOtherSideWindows();
    m_isDrawingCard = true;
    m_idleCounter = 0;
    playSound("drawing.mp3", false);
    setState(Sleep);
    QTimer::singleShot(1000, this, &DesktopPet::drawingStep2Idle);
}

void DesktopPet::drawingStep2Idle() {
    setState(Idle);
    QTimer::singleShot(2000, this, &DesktopPet::drawingStep3Result);
}

void DesktopPet::drawingStep3Result() {
    setState(Result);
    QTimer::singleShot(1000, this, &DesktopPet::drawingStep4ShowWindow);
}

void DesktopPet::drawingStep4ShowWindow() {
    m_drawingWindow = new DrawingEffectWindow(this, m_drawingDir, m_scale);
    static_cast<DrawingEffectWindow*>(m_drawingWindow.data())->startShow();
}

void DesktopPet::startVolumeFeature() {
    if (m_isDrawingCard) return;
    closeOtherSideWindows();
    m_idleCounter = 0;
    m_volumeWindow = new VolumeSliderWindow(this, m_scale);
    m_volumeWindow->show();
}

void DesktopPet::startAuthorFeature() {
    if (m_isDrawingCard) return;
    closeOtherSideWindows();
    m_idleCounter = 0;
    m_authorWindow = new AuthorWindow(this, m_scale);
    m_authorWindow->show();
}

void DesktopPet::startResourceFeature() {
    if (m_isDrawingCard) return;
    closeOtherSideWindows();
    m_idleCounter = 0;
    m_resourceWindow = new ResourceWindow(this, m_scale);
    m_resourceWindow->show();
}

void DesktopPet::onDrawEffectFinished() {
    setState(Result);
    playSound("result.mp3");
    QTimer::singleShot(1000, this, [this]() {
        m_isDrawingCard = false;
        m_idleCounter = 0;
        setState(Idle);
    });
}

void DesktopPet::closeOtherSideWindows() {
    if (m_effectWindow && m_effectWindow->isVisible()) { m_effectWindow->close(); m_effectWindow = nullptr; }
    if (m_timerWindow) {
        if (m_timerWindow->isVisible()) {
            auto *tw = static_cast<TimerWindow*>(m_timerWindow.data());
            tw->stopAndCleanup();
        }
        m_timerWindow = nullptr;
    }
    if (m_drawingWindow && m_drawingWindow->isVisible()) { m_drawingWindow->close(); m_drawingWindow = nullptr; }
    if (m_volumeWindow && m_volumeWindow->isVisible()) { m_volumeWindow->close(); m_volumeWindow = nullptr; }
    if (m_authorWindow && m_authorWindow->isVisible()) { m_authorWindow->close(); m_authorWindow = nullptr; }
    if (m_resourceWindow && m_resourceWindow->isVisible()) { m_resourceWindow->close(); m_resourceWindow = nullptr; }
}

void DesktopPet::closeEvent(QCloseEvent *event) {
    saveSettings();
    hide();
    event->ignore();
}

void DesktopPet::saveSettings() {
    QSettings s;
    s.setValue("window/x", x());
    s.setValue("window/y", y());
    s.setValue("window/scale", m_scale);
    s.setValue("globalVolume", m_volume);
    s.setValue("stayOnTop", m_stayOnTop);
    s.setValue("mouseTransparent", m_mouseTransparent);
    s.sync();
}

void DesktopPet::loadSettings() {
    QSettings s;
    // 1. Scale first — applyScale() may resize & shift position
    float savedScale = s.value("window/scale", -1.0f).toFloat();
    if (savedScale > 0.1f) {
        m_scale = qBound(m_minScale, savedScale, m_maxScale);
        applyScale();
    }
    // 2. Position after scale (avoids applyScale overwriting saved position)
    int sx = s.value("window/x", -1).toInt();
    int sy = s.value("window/y", -1).toInt();
    if (sx >= 0 || sy >= 0) {
        QScreen *sc = QApplication::primaryScreen();
        if (sc) {
            QRect avail = sc->availableGeometry();
            // Clamp to keep at least 50px visible, don't reject edge positions
            int maxX = qMax(0, avail.right() - 50);
            int maxY = qMax(0, avail.bottom() - 50);
            sx = qBound(0, sx, maxX);
            sy = qBound(0, sy, maxY);
            move(sx, sy);
        }
    }
    // 3. Volume
    m_volume = s.value("globalVolume", 80).toInt();
    for (auto *s : m_sounds) s->setVolume(m_volume / 100.0f);
    // 4. Stay on top — apply without toggling
    m_stayOnTop = s.value("stayOnTop", true).toBool();
    applyStayOnTop();
    // 5. Mouse transparent — apply without toggling (toggle would flip the value back!)
    m_mouseTransparent = s.value("mouseTransparent", false).toBool();
    applyMouseTransparent();
}

void DesktopPet::setupTrayIcon() {
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(":/icon.png"));
    m_trayIcon->setToolTip(QString::fromUtf8("蕾米埃尔桌宠"));

    m_trayMenu = new QMenu();
    m_trayMenu->setStyleSheet(
        "QMenu{background:#FF8DA1;border:2px solid #fff;border-radius:10px;padding:6px 0;}"
        "QMenu::item{background:transparent;color:#fff;font-size:15px;font-weight:bold;padding:8px 24px;margin:3px 6px;border-radius:5px;}"
        "QMenu::item:selected{background:#FF6B8B;}"
    );
    QAction *showAction = m_trayMenu->addAction(QString::fromUtf8("显示桌面宠物"));
    connect(showAction, &QAction::triggered, this, [this]() { show(); });
    QAction *hideAction = m_trayMenu->addAction(QString::fromUtf8("隐藏桌面宠物"));
    connect(hideAction, &QAction::triggered, this, &DesktopPet::hide);
    m_trayMenu->addSeparator();
    m_trayMouseAction = m_trayMenu->addAction(QString::fromUtf8("鼠标穿透"));
    m_trayMouseAction->setCheckable(true);
    connect(m_trayMouseAction, &QAction::triggered, this, &DesktopPet::toggleMouseTransparent);
    m_trayMenu->addSeparator();
    QAction *quitAction = m_trayMenu->addAction(QString::fromUtf8("退出程序"));
    connect(quitAction, &QAction::triggered, qApp, []() { qApp->exit(0); });
    m_trayIcon->setContextMenu(m_trayMenu);

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            if (isVisible()) hide(); else show();
        }
    });
    m_trayIcon->show();
}

void DesktopPet::toggleMouseTransparent() {
    m_mouseTransparent = !m_mouseTransparent;
    applyMouseTransparent();
    saveSettings();
}

void DesktopPet::applyMouseTransparent() {
    if (m_trayMouseAction) m_trayMouseAction->setChecked(m_mouseTransparent);

    // Mouse transparency: cross-platform Qt + platform-native enhancement
    setAttribute(Qt::WA_TransparentForMouseEvents, m_mouseTransparent);

#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    if (m_mouseTransparent)
        SetWindowLong(hwnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT | WS_EX_LAYERED);
    else
        SetWindowLong(hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);
    SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
#elif defined(Q_OS_MAC)
    {
        QWindow *win = windowHandle();
        if (win) {
            win->setFlag(Qt::WindowTransparentForInput, m_mouseTransparent);
            bool wasVisible = isVisible();
            if (wasVisible) {
                hide();
                show();
            }
        }
    }
#elif defined(Q_OS_LINUX)
    {
        QWindow *win = windowHandle();
        if (win) {
            win->setFlags(m_mouseTransparent ? (win->flags() | Qt::WindowTransparentForInput) : (win->flags() & ~Qt::WindowTransparentForInput));
        }
    }
#endif
    if (m_trayIcon)
        m_trayIcon->setToolTip(QString::fromUtf8("\u857E\u7C73\u57C3\u5C14\u684C\u5BA0"));
}

void DesktopPet::toggleStayOnTop() {
    m_stayOnTop = !m_stayOnTop;
    applyStayOnTop();
    saveSettings();
}

void DesktopPet::applyStayOnTop() {
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    SetWindowPos(hwnd, m_stayOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
                 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#elif defined(Q_OS_MAC)
    {
        void *nsview = reinterpret_cast<void *>(winId());
        if (nsview) {
            id window = ((id (*)(id, SEL))objc_msgSend)((id)nsview, sel_registerName("window"));
            if (window) {
                long level = m_stayOnTop ? (long)CGWindowLevelForKey(kCGOverlayWindowLevelKey) : (long)CGWindowLevelForKey(kCGNormalWindowLevelKey);
                ((void (*)(id, SEL, long))objc_msgSend)(window, sel_registerName("setLevel:"), level);
            }
        }
    }
#else
    Qt::WindowFlags flags = windowFlags();
    if (m_stayOnTop) flags |= Qt::WindowStaysOnTopHint;
    else flags &= ~Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);
    show();
#endif

    auto updateWindowFlag = [this](QWidget *w) {
        if (w) {
            Qt::WindowFlags wf = w->windowFlags();
            if (m_stayOnTop) wf |= Qt::WindowStaysOnTopHint;
            else wf &= ~Qt::WindowStaysOnTopHint;
            w->setWindowFlags(wf);
            if (w->isVisible()) w->show();
        }
    };
    updateWindowFlag(m_effectWindow);
    updateWindowFlag(m_timerWindow);
    updateWindowFlag(m_drawingWindow);
    updateWindowFlag(m_volumeWindow);
    updateWindowFlag(m_authorWindow);
    updateWindowFlag(m_resourceWindow);
}
