#define MINIAUDIO_IMPLEMENTATION
#define MA_DEBUG_OUTPUT
#include "miniaudio.h"
#include "audioplayer.h"
#include <QFileInfo>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QStandardPaths>

// ── File log helper (Windows GUI apps have no console) ─────────────
static void audioLog(const QString &msg) {
    static QFile s_logFile;
    static bool s_inited = false;
    if (!s_inited) {
        QString logPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/remilia_audio.log";
        s_logFile.setFileName(logPath);
        s_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
        s_inited = true;
    }
    if (s_logFile.isOpen()) {
        QTextStream ts(&s_logFile);
        ts << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << " " << msg << "\n";
        ts.flush();
    }
}

// ── Shared engine (reference counted) ──────────────────────────────
ma_engine *AudioPlayer::s_engine = nullptr;
int        AudioPlayer::s_engineRef = 0;

void AudioPlayer::initEngine() {
    if (s_engineRef++ == 0) {
        s_engine = new ma_engine;
        audioLog(QStringLiteral("ma_engine_init starting..."));
        ma_result r = ma_engine_init(nullptr, s_engine);
        if (r != MA_SUCCESS) {
            QString err = QStringLiteral("ma_engine_init FAILED: ") + ma_result_description(r);
            qWarning() << "AudioPlayer:" << err;
            audioLog(err);
        } else {
            audioLog(QStringLiteral("ma_engine_init SUCCESS"));
        }
    }
}

void AudioPlayer::shutdownEngine() {
    if (--s_engineRef == 0) {
        audioLog(QStringLiteral("ma_engine_uninit"));
        ma_engine_uninit(s_engine);
        delete s_engine;
        s_engine = nullptr;
    }
}

// ── AudioPlayer ────────────────────────────────────────────────────

AudioPlayer::AudioPlayer() {
    initEngine();
}

AudioPlayer::~AudioPlayer() {
    stop();
    if (m_sound) {
        ma_sound_uninit(m_sound);
        delete m_sound;
        m_sound = nullptr;
    }
    shutdownEngine();
}

void AudioPlayer::setSource(const QString &path) {
    stop();
    if (m_sound) {
        ma_sound_uninit(m_sound);
        delete m_sound;
        m_sound = nullptr;
    }
    m_loaded = false;

    if (path.isEmpty()) {
        audioLog(QStringLiteral("setSource: empty path, skipped"));
        return;
    }
    if (!QFileInfo::exists(path)) {
        audioLog(QStringLiteral("setSource: file not found: ") + path);
        return;
    }

    m_sound = new ma_sound;
    QByteArray utf8 = path.toUtf8();
    ma_result r = ma_sound_init_from_file(s_engine, utf8.constData(),
                                           0,
                                           nullptr, nullptr, m_sound);
    if (r != MA_SUCCESS) {
        QString err = QStringLiteral("load FAILED: ") + ma_result_description(r) + " path: " + path;
        qWarning() << "AudioPlayer:" << err;
        audioLog(err);
        delete m_sound;
        m_sound = nullptr;
        return;
    }
    audioLog(QStringLiteral("loaded: ") + path);
    ma_sound_set_volume(m_sound, m_volume);
    m_loaded = true;
}

void AudioPlayer::setVolume(float volume) {
    m_volume = volume;
    if (m_loaded && m_sound)
        ma_sound_set_volume(m_sound, m_volume);
}

void AudioPlayer::play() {
    if (!m_loaded || !m_sound) {
        audioLog(QStringLiteral("play: not loaded, skipped"));
        return;
    }
    audioLog(QStringLiteral("play"));
    ma_sound_seek_to_pcm_frame(m_sound, 0);
    ma_sound_start(m_sound);
}

void AudioPlayer::stop() {
    if (m_loaded && m_sound)
        ma_sound_stop(m_sound);
}
