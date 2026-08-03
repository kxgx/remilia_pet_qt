#include <QtPlugin>

// ── Static Qt plugin imports ──────────────────────────────────────
// Required when linking against static Qt builds.
// Each platform needs its own platform integration plugin.
// Image format plugins are needed on all platforms for QRC resources.

#if defined(QT_STATIC)

// ── Platform integration ───────────────────────────────────────────
#if defined(Q_OS_WIN)
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#elif defined(Q_OS_MACOS)
Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin)
#elif defined(Q_OS_LINUX)
Q_IMPORT_PLUGIN(QXcbIntegrationPlugin)
#endif

// ── Multimedia backend ─────────────────────────────────────────────
Q_IMPORT_PLUGIN(QFFmpegMediaPlugin)

// ── Image format plugins (needed for QRC-embedded images) ──────────
Q_IMPORT_PLUGIN(QGifPlugin)
Q_IMPORT_PLUGIN(QICOPlugin)
Q_IMPORT_PLUGIN(QJpegPlugin)

#endif
