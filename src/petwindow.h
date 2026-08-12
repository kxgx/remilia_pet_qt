#pragma once

#include <QLabel>
#include <QMovie>
#include <QTimer>
#include "audioplayer.h"
#include <QSystemTrayIcon>
#include <QMap>
#include <QPoint>
#include <QSize>
#include <QPointer>

// Forward declarations
class DrawEffectWindow;
class DrawingEffectWindow;
class TimerWindow;
class VolumeSliderWindow;
class AuthorWindow;
class ResourceWindow;
class QFileSystemWatcher;

class DesktopPet : public QLabel {
    Q_OBJECT

    friend class DrawEffectWindow;
    friend class DrawingEffectWindow;
    friend class TimerWindow;
    friend class VolumeSliderWindow;
    friend class AuthorWindow;
    friend class ResourceWindow;

public:
    enum State { Idle, Click, Drag, Sleep, Result };

    explicit DesktopPet(QWidget *parent = nullptr);
    ~DesktopPet() override;

    // Called by side windows
    void onDrawEffectFinished();
    void setGlobalVolume(int vol);

    float scaleFactor() const { return m_scale; }
    int globalVolume() const { return m_volume; }
    void updateSideWindowPositions();

    // Re-apply resource overrides after files changed in the in-app file manager
    void applyResourceChanges();
    // Debounced entry: file-manager signals and external watcher events both route here,
    // so bursts of changes coalesce into a single applyResourceChanges().
    void scheduleResourceReload();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void checkIdle();
    void onResourceWatchChanged(const QString &path);

private:
    void setState(State state);
    void manualPaintFrame(int frame);
    void applyScale();
    void applyScaleGeometry();
    void applyScaleRender();
    void resetScale();
    void playSound(const QString &file, bool override = true);
    void closeOtherSideWindows();
    void preloadNativeSizes();

    // Feature starters
    void startDrawCard();
    void startTimerFeature();
    void startDrawingFeature();
    void startVolumeFeature();
    void startAuthorFeature();
    void startResourceFeature();
    void toggleStayOnTop();
    void saveSettings();
    void loadSettings();
    void loadOverrides();
    void refreshOverrides();
    void setupTrayIcon();

    // Drawing feature steps
    void drawingStep2Idle();
    void drawingStep3Result();
    void drawingStep4ShowWindow();

    // Resource override
    QString resolveResourcePath(const QString &qrcPath) const;

    // State
    State m_state = Idle;
    float m_scale = 1.0f;
    float m_minScale = 0.5f;
    float m_maxScale = 3.0f;
    QSize m_maxNativeSize;
    QMap<State, QSize> m_nativeSizes;
    QSize m_currentTargetSize;

    // GIF
    QMovie *m_movie = nullptr;

    // Interaction
    bool m_dragging = false;
    QPoint m_dragStart;
    int m_idleCounter = 0;
    bool m_isDrawingCard = false;

    // Audio — miniaudio directly uses WASAPI/PulseAudio/ALSA/CoreAudio
    int m_volume = 80;
    QMap<QString, AudioPlayer*> m_sounds;
    void preloadSounds();
    void reloadSounds();
    QString resolveSoundSource(const QString &fileName) const;

    // Idle timer
    QTimer *m_idleTimer = nullptr;
    QTimer *m_scaleTimer = nullptr;
    QTimer *m_autoSaveTimer = nullptr;

    // Tray
    QSystemTrayIcon *m_trayIcon = nullptr;
    QMenu *m_trayMenu = nullptr;

    // Stay on top
    bool m_stayOnTop = true;
    bool m_mouseTransparent = false;
    void toggleMouseTransparent();
    void applyMouseTransparent();
    void applyStayOnTop();
    QAction *m_trayMouseAction = nullptr;
    QString m_fontFamily;
    int m_fontSize = -1;
    bool m_fontBold = true;
    QFont m_systemDefaultFont;
    void applyFontPreference();
    QString menuStylesheet(int fs, int pv, int ph, int mv, int mh, int br, int ibr, int mp, int bw, int smv, int smh);

    // Side windows (QPointer auto-nulls when widget is deleted)
    QPointer<QWidget> m_effectWindow;
    QPointer<QWidget> m_timerWindow;
    QPointer<QWidget> m_drawingWindow;
    QPointer<QWidget> m_volumeWindow;
    QPointer<QWidget> m_authorWindow;
    QPointer<QWidget> m_resourceWindow;

    // Paths
    QString m_audioDir;
    QString m_cardsDir;
    QString m_drawingDir;

    // Resource overrides (directory-based): resourceKey -> absoluteFilePath
    QString m_resourceDir;
    QMap<QString, QString> m_resourceOverrides;

    // Watches the resource directories so external changes apply immediately too
    QFileSystemWatcher *m_resourceWatcher = nullptr;
    QTimer *m_resourceWatchTimer = nullptr;
    void startResourceWatcher();
    void syncResourceWatcher();


};
