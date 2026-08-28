#include "MediaProbeService.h"
#include "ffmpeg/FFmpegLocator.h"

#include <QProcess>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

MediaProbeService::MediaProbeService(const FFmpegLocator *locator, QObject *parent)
    : QObject(parent)
    , m_locator(locator)
{
}

MediaFile MediaProbeService::probe(const QString &filePath) const
{
    if (!m_locator || !m_locator->isValid()) {
        return {};
    }

    QFileInfo fi(filePath);
    if (!fi.exists()) {
        return {};
    }

    QProcess proc;
    proc.start(m_locator->ffprobePath(), {
        QStringLiteral("-v"), QStringLiteral("error"),
        QStringLiteral("-of"), QStringLiteral("json"),
        QStringLiteral("-show_format"),
        QStringLiteral("-show_streams"),
        QStringLiteral("-show_chapters"),
        filePath
    });

    if (!proc.waitForFinished(30000)) {
        proc.kill();
        return {};
    }

    if (proc.exitCode() != 0) {
        return {};
    }

    QString json = QString::fromUtf8(proc.readAllStandardOutput());
    return parseProbeJson(json, filePath);
}

MediaFile MediaProbeService::parseProbeJson(const QString &json, const QString &filePath) const
{
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isObject()) {
        return {};
    }

    QJsonObject root = doc.object();
    MediaFile media;
    media.path = filePath;

    QJsonObject formatObj = root.value(QStringLiteral("format")).toObject();
    media.formatName = formatObj.value(QStringLiteral("format_name")).toString();
    media.fileSize = formatObj.value(QStringLiteral("size")).toString().toLongLong();
    media.duration = formatObj.value(QStringLiteral("duration")).toString().toDouble();
    media.bitrate = formatObj.value(QStringLiteral("bit_rate")).toString().toLongLong();

    QJsonObject formatTags = formatObj.value(QStringLiteral("tags")).toObject();
    for (auto it = formatTags.begin(); it != formatTags.end(); ++it) {
        media.formatMetadata[it.key()] = it.value().toVariant();
    }

    QJsonArray streams = root.value(QStringLiteral("streams")).toArray();
    for (const QJsonValue &val : streams) {
        QJsonObject stream = val.toObject();
        QString codecType = stream.value(QStringLiteral("codec_type")).toString();

        if (codecType == QStringLiteral("video")) {
            VideoStream vs;
            vs.index = stream.value(QStringLiteral("index")).toInt();
            vs.codec = stream.value(QStringLiteral("codec_name")).toString();
            vs.width = stream.value(QStringLiteral("width")).toInt();
            vs.height = stream.value(QStringLiteral("height")).toInt();
            vs.pixelFormat = stream.value(QStringLiteral("pix_fmt")).toString();
            vs.bitrate = stream.value(QStringLiteral("bit_rate")).toString().toLongLong();

            QString rFrameRate = stream.value(QStringLiteral("r_frame_rate")).toString();
            auto parts = rFrameRate.split(QLatin1Char('/'));
            if (parts.size() == 2) {
                double num = parts[0].toDouble();
                double den = parts[1].toDouble();
                if (den > 0) {
                    vs.frameRate = num / den;
                }
            }

            QJsonObject tags = stream.value(QStringLiteral("tags")).toObject();
            for (auto it = tags.begin(); it != tags.end(); ++it) {
                vs.metadata[it.key()] = it.value().toVariant();
            }

            media.videoStreams.append(vs);
        }
        else if (codecType == QStringLiteral("audio")) {
            AudioStream as;
            as.index = stream.value(QStringLiteral("index")).toInt();
            as.codec = stream.value(QStringLiteral("codec_name")).toString();
            as.sampleRate = stream.value(QStringLiteral("sample_rate")).toString().toInt();
            as.channels = stream.value(QStringLiteral("channels")).toInt();
            as.channelLayout = stream.value(QStringLiteral("channel_layout")).toString();
            as.bitrate = stream.value(QStringLiteral("bit_rate")).toString().toLongLong();

            QJsonObject tags = stream.value(QStringLiteral("tags")).toObject();
            for (auto it = tags.begin(); it != tags.end(); ++it) {
                as.metadata[it.key()] = it.value().toVariant();
            }

            media.audioStreams.append(as);
        }
        else if (codecType == QStringLiteral("subtitle")) {
            SubtitleStream ss;
            ss.index = stream.value(QStringLiteral("index")).toInt();
            ss.codec = stream.value(QStringLiteral("codec_name")).toString();

            QJsonObject tags = stream.value(QStringLiteral("tags")).toObject();
            ss.language = tags.value(QStringLiteral("language")).toString();

            for (auto it = tags.begin(); it != tags.end(); ++it) {
                ss.metadata[it.key()] = it.value().toVariant();
            }

            media.subtitleStreams.append(ss);
        }
    }

    media.type = detectMediaType(media.formatName, media.videoStreams, media.audioStreams);

    return media;
}

MediaType MediaProbeService::detectMediaType(const QString &formatName,
                                              const QList<VideoStream> &videos,
                                              const QList<AudioStream> &audios) const
{
    if (!videos.isEmpty()) {
        return MediaType::Video;
    }
    if (!audios.isEmpty()) {
        return MediaType::Audio;
    }

    QString lower = formatName.toLower();
    if (lower.contains(QStringLiteral("image")) || lower.contains(QStringLiteral("png")) ||
        lower.contains(QStringLiteral("jpeg")) || lower.contains(QStringLiteral("webp")) ||
        lower.contains(QStringLiteral("bmp")) || lower.contains(QStringLiteral("tiff"))) {
        return MediaType::Image;
    }

    return MediaType::Unknown;
}
