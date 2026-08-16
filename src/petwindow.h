// petwindow.h — 桌面宠物主窗口 DesktopPet
// 核心：GIF 状态机（idle/click/drag/sleep/result）、托盘菜单、设置保存、
// 抽卡/计时/画画/音量/作者声明/资源替换等功能入口，以及资源覆盖目录的加载、文件监听与即时重载。
// 各功能侧窗类（DrawEffectWindow/DrawingEffectWindow/TimerWindow 等）定义在对应 cpp 中。
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
class KeyDisplayWindow;
class GamepadWindow;
class GamepadKeyWindow;
class MusicWindow;
class QFileSystemWatcher;

class DesktopPet : public QLabel
{
    Q_OBJECT

    friend class DrawEffectWindow;
    friend class DrawingEffectWindow;
    friend class TimerWindow;
    friend class VolumeSliderWindow;
    friend class AuthorWindow;
    friend class ResourceWindow;
    friend class KeyDisplayWindow;
    friend class GamepadWindow;
    friend class GamepadKeyWindow;
    friend class MusicWindow;

    public:
    enum State
    {
        Idle,
        Click,
        Drag,
        Sleep,
        Result
    };

    explicit DesktopPet(QWidget *parent = nullptr);
    ~DesktopPet() override;

    // Called by side windows
    void onDrawEffectFinished();
    void setGlobalVolume(int vol);

    float scaleFactor() const
    {
        return m_scale;
    }
    int globalVolume() const
    {
        return m_volume;
    }
    void updateSideWindowPositions();

    // Re-apply resource overrides after files changed in the in-app file manager
    void applyResourceChanges();
    // Debounced entry: file-manager signals and external watcher events both route here,
    // so bursts of changes coalesce into a single applyResourceChanges().
    void scheduleResourceReload();
    // 编号资源数量（1..N 连续）：内置 QRC + 覆盖目录都算，新增编号文件即可被抽到
    int numberedResourceCount(const QString &dir, const QString &base, const QString &ext) const;
    // 键位显示：全局键盘钩子回调入口（由 petwindow.cpp 的自由函数 desktopPetHandleGlobalKey 调用）
    void onGlobalKey(const QString &text);

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
    void onScreenGeometryChanged();
    void pollGamepadState();
    void pollMediaInfo();

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
    float m_minScale = 0.3f; // 最小可见缩放
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
    QMap<QString, AudioPlayer *> m_sounds;
    void preloadSounds();
    void reloadSounds();
    QString resolveSoundSource(const QString &fileName) const;

    // Idle timer
    QTimer *m_idleTimer = nullptr;
    QTimer *m_scaleTimer = nullptr;
    QTimer *m_scaleSaveTimer = nullptr; // 滚轮缩放防抖保存（500ms，避免每事件 sync QSettings）
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

    // 键位显示
    bool m_keyDisplayEnabled = false;
    bool m_keyDisplayOnTop = true; // 独立置顶开关：仅全局置顶关闭时生效，开启时键位窗口单独保持置顶
    QPointer<QWidget> m_keyWindow;
    QAction *m_trayKeyAction = nullptr;
    QAction *m_trayKeyTopAction = nullptr;
    QTimer *m_linuxKeyPollTimer = nullptr;
    void toggleKeyDisplay();
    void toggleKeyDisplayOnTop();
    void applyKeyDisplay();
    void applyKeyWindowTop();
    void startGlobalKeyHook();

    // 实验性功能（仅 Windows）：手柄键位显示 + 音乐信息显示，合入"⌨ 键位显示"开关
    QPointer<QWidget> m_gamepadWindow;    // 手柄图
    QPointer<QWidget> m_gamepadKeyWindow; // 手柄按键文字气泡（独立窗口）
    QPointer<QWidget> m_musicWindow;
    QTimer *m_gamepadPollTimer = nullptr; // 手柄轮询 30ms（XInput/DI 均轻量）
    QTimer *m_musicPollTimer = nullptr;   // SMTC 音乐信息轮询 800ms

    // 相对位置：按屏幕可用区域的比例保存/恢复，分辨率变化时实时重算保持相对位置不变
    double m_rightRatio = -1.0;
    double m_yRatio = -1.0;
    QString m_screenName;
    QTimer *m_screenMoveTimer = nullptr;
    void startScreenTracking();
    void applyRelativePosition();

    // Watches the resource directories so external changes apply immediately too
    QFileSystemWatcher *m_resourceWatcher = nullptr;
    QTimer *m_resourceWatchTimer = nullptr;
    void startResourceWatcher();
    void syncResourceWatcher();
};
