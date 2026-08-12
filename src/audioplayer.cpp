#define MINIAUDIO_IMPLEMENTATION
#define MA_DEBUG_OUTPUT
#include "miniaudio.h"
#include "audioplayer.h"
#include <QFileInfo>
#include <QDebug>

// ── Shared engine (reference counted) ──────────────────────────────
ma_engine *AudioPlayer::s_engine = nullptr;
int        AudioPlayer::s_engineRef = 0;

void AudioPlayer::initEngine() {
    if (s_engineRef++ == 0) {
        s_engine = new ma_engine;
        ma_result r = ma_engine_init(nullptr, s_engine);
        if (r != MA_SUCCESS)
            qWarning() << "AudioPlayer: ma_engine_init failed:" << ma_result_description(r);
    }
}

void AudioPlayer::shutdownEngine() {
    if (--s_engineRef == 0) {
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

    if (path.isEmpty() || !QFileInfo::exists(path))
        return;

    m_sound = new ma_sound;
    QByteArray utf8 = path.toUtf8();
    ma_result r = ma_sound_init_from_file(s_engine, utf8.constData(),
                                           0,
                                           nullptr, nullptr, m_sound);
    if (r != MA_SUCCESS) {
        qWarning() << "AudioPlayer: load failed:" << ma_result_description(r) << "path:" << path;
        delete m_sound;
        m_sound = nullptr;
        return;
    }
    qDebug() << "AudioPlayer: loaded" << path;
    ma_sound_set_volume(m_sound, m_volume);
    m_loaded = true;
}

void AudioPlayer::setVolume(float volume) {
    m_volume = volume;
    if (m_loaded && m_sound)
        ma_sound_set_volume(m_sound, m_volume);
}

void AudioPlayer::play() {
    if (!m_loaded || !m_sound) return;
    ma_sound_seek_to_pcm_frame(m_sound, 0);
    ma_sound_start(m_sound);
}

void AudioPlayer::stop() {
    if (m_loaded && m_sound)
        ma_sound_stop(m_sound);
}
