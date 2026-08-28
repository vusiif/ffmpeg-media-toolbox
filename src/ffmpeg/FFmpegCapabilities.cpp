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

QSet<QString> FFmpegCapabilities::parseListOutput(const QString &output) const
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

        if (!inList || trimmed.isEmpty()) {
            continue;
        }

        auto parts = trimmed.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            QString flags = parts[0];
            QString name = parts[1];

            if (name.contains(QLatin1Char('='))) {
                continue;
            }

            result.insert(name.toLower());
        }
    }

    if (result.isEmpty()) {
        for (const QString &line : lines) {
            QString trimmed = line.trimmed();
            if (!trimmed.isEmpty() && !trimmed.startsWith(QLatin1String("--"))) {
                result.insert(trimmed.toLower());
            }
        }
    }

    return result;
}
