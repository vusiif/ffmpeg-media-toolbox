#include "FFmpegCapabilities.h"
#include "FFmpegLocator.h"

#include <QProcess>

FFmpegCapabilities::FFmpegCapabilities(const FFmpegLocator *locator, QObject *parent)
    : QObject(parent)
    , m_locator(locator)
{
}

bool FFmpegCapabilities::isLoaded() const { return m_loaded; }

bool FFmpegCapabilities::scan()
{
    if (!m_locator || !m_locator->isValid()) {
        return false;
    }

    QString ffmpeg = m_locator->ffmpegPath();

    auto runQuery = [&](const QStringList &args) -> QString {
        QProcess proc;
        proc.start(ffmpeg, args);
        if (!proc.waitForFinished(10000)) {
            proc.kill();
            return {};
        }
        return QString::fromUtf8(proc.readAllStandardOutput());
    };

    m_formats = parseListOutput(runQuery({QStringLiteral("-formats")}));
    m_codecs = parseListOutput(runQuery({QStringLiteral("-codecs")}));
    m_encoders = parseListOutput(runQuery({QStringLiteral("-encoders")}));
    m_decoders = parseListOutput(runQuery({QStringLiteral("-decoders")}));
    m_filters = parseListOutput(runQuery({QStringLiteral("-filters")}));
    m_hwAccels = parseListOutput(runQuery({QStringLiteral("-hwaccels")}));

    m_loaded = true;
    emit capabilitiesChanged();
    return true;
}

bool FFmpegCapabilities::hasFormat(const QString &name) const { return m_formats.contains(name); }
bool FFmpegCapabilities::hasCodec(const QString &name) const { return m_codecs.contains(name); }
bool FFmpegCapabilities::hasEncoder(const QString &name) const { return m_encoders.contains(name); }
bool FFmpegCapabilities::hasDecoder(const QString &name) const { return m_decoders.contains(name); }
bool FFmpegCapabilities::hasFilter(const QString &name) const { return m_filters.contains(name); }
bool FFmpegCapabilities::hasHwAccel(const QString &name) const { return m_hwAccels.contains(name); }

QSet<QString> FFmpegCapabilities::formats() const { return m_formats; }
QSet<QString> FFmpegCapabilities::codecs() const { return m_codecs; }
QSet<QString> FFmpegCapabilities::encoders() const { return m_encoders; }
QSet<QString> FFmpegCapabilities::decoders() const { return m_decoders; }
QSet<QString> FFmpegCapabilities::filters() const { return m_filters; }
QSet<QString> FFmpegCapabilities::hwAccels() const { return m_hwAccels; }

int FFmpegCapabilities::formatCount() const { return m_formats.size(); }
int FFmpegCapabilities::encoderCount() const { return m_encoders.size(); }
int FFmpegCapabilities::decoderCount() const { return m_decoders.size(); }
int FFmpegCapabilities::filterCount() const { return m_filters.size(); }
int FFmpegCapabilities::hwAccelCount() const { return m_hwAccels.size(); }

QStringList FFmpegCapabilities::videoEncoderCandidates()
{
    return {
        QStringLiteral("libx264"),   QStringLiteral("libx265"),
        QStringLiteral("libvpx-vp9"), QStringLiteral("h264_nvenc"),
        QStringLiteral("hevc_nvenc"), QStringLiteral("h264_qsv"),
        QStringLiteral("hevc_qsv"),   QStringLiteral("h264_amf"),
        QStringLiteral("hevc_amf"),   QStringLiteral("h264_vaapi"),
        QStringLiteral("hevc_vaapi"), QStringLiteral("h264_videotoolbox"),
        QStringLiteral("hevc_videotoolbox"),
    };
}

QStringList FFmpegCapabilities::audioEncoderCandidates()
{
    return {
        QStringLiteral("aac"),         QStringLiteral("libmp3lame"),
        QStringLiteral("flac"),        QStringLiteral("libopus"),
        QStringLiteral("pcm_s16le"),
    };
}

QStringList FFmpegCapabilities::filterEncoders(const QStringList &candidates) const
{
    if (!m_loaded) {
        return candidates;
    }

    QStringList available;
    for (const QString &candidate : candidates) {
        if (m_encoders.contains(candidate)) {
            available.append(candidate);
        }
    }
    return available;
}

namespace {
// Listing rows start with one or two flag columns (e.g. "DE", "V....D",
// or "D d" where the lone "d" marks a capture device) before the name.
bool isFlagsColumnToken(const QString &token)
{
    if (token == QLatin1String("d")) {
        return true;
    }
    if (token.isEmpty() || token.size() > 6) {
        return false;
    }
    for (const QChar &c : token) {
        if (!c.isUpper() && c != QLatin1Char('.')) {
            return false;
        }
    }
    return true;
}
}

QSet<QString> FFmpegCapabilities::parseListOutput(const QString &output)
{
    QSet<QString> result;

    auto lines = output.split(QLatin1Char('\n'));
    bool inList = false;

    for (const QString &line : lines) {
        QString trimmed = line.trimmed();

        if (trimmed.startsWith(QLatin1String("---"))) {
            inList = true;
            continue;
        }

        if (trimmed.startsWith(QLatin1String("--"))) {
            continue;
        }

        if (!inList || trimmed.isEmpty()) {
            continue;
        }

        auto parts = trimmed.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        int nameIndex = 0;
        while (nameIndex < parts.size() && isFlagsColumnToken(parts[nameIndex])) {
            ++nameIndex;
        }
        if (nameIndex >= parts.size()) {
            continue;
        }
        QString name = parts[nameIndex];

        if (name.contains(QLatin1Char('='))) {
            continue;
        }

        result.insert(name.toLower());
    }

    if (result.isEmpty()) {
        for (const QString &line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed.isEmpty() || trimmed.startsWith(QLatin1String("--"))
                || trimmed.endsWith(QLatin1Char(':')) || trimmed.contains(QLatin1String(" = "))) {
                continue;
            }
            // Plain lists (e.g. `ffmpeg -hwaccels`) have single-token entries
            // followed by the names only; anything with spaces is not an entry.
            if (trimmed.contains(QLatin1Char(' '))) {
                continue;
            }
            result.insert(trimmed.toLower());
        }
    }

    return result;
}
