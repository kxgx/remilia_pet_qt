#pragma once

#include <QString>

// Forward declaration — miniaudio types defined in audioplayer.cpp
typedef struct ma_engine ma_engine;
typedef struct ma_sound ma_sound;

/// Lightweight audio player wrapping miniaudio, API-compatible with QSoundEffect.
/// Uses a shared engine (reference-counted) — direct WASAPI/PulseAudio/ALSA/CoreAudio,
/// no GStreamer / FFmpeg / Qt Multimedia dependency.
class AudioPlayer {
public:
    AudioPlayer();
    ~AudioPlayer();

    void setSource(const QString &path);
    void setVolume(float volume);   // 0.0 — 1.0
    void play();
    void stop();

private:
    static ma_engine *s_engine;
    static int        s_engineRef;
    static void initEngine();
    static void shutdownEngine();

    ma_sound *m_sound = nullptr;
    bool      m_loaded = false;
    float     m_volume  = 1.0f;
};
