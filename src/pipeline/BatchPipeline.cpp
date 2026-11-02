#include "BatchPipeline.h"

#include "conversion/RemuxPolicy.h"
#include "ffmpeg/FFmpegCapabilities.h"
#include "ffmpeg/FFmpegCommandBuilder.h"

#include <QFileInfo>
#include <QJsonArray>

namespace {
QString sanitizeCodec(const QString &codec)
{
    QString c = codec.trimmed();
    if (c == QLatin1String("H.264") || c == QLatin1String("AVC")) return QStringLiteral("libx264");
    if (c == QLatin1String("H.265 / HEVC") || c == QLatin1String("HEVC") || c == QLatin1String("H.265")) return QStringLiteral("libx265");
    if (c == QLatin1String("VP9")) return QStringLiteral("libvpx-vp9");
    if (c == QLatin1String("AV1")) return QStringLiteral("libsvtav1");
    if (c == QLatin1String("AAC")) return QStringLiteral("aac");
    if (c == QLatin1String("MP3")) return QStringLiteral("libmp3lame");
    if (c == QLatin1String("Opus")) return QStringLiteral("libopus");
    if (c == QLatin1String("FLAC")) return QStringLiteral("flac");
    return c;
}

QString watermarkOverlayCoordinates(const QString &position, int margin)
{
    const int m = qMax(0, margin);
    const QString pos = position.toLower();
    if (pos == QLatin1String("top-left")) {
        return QStringLiteral("%1:%1").arg(m);
    } else if (pos == QLatin1String("bottom-left")) {
        return QStringLiteral("%1:H-h-%1").arg(m);
    } else if (pos == QLatin1String("bottom-right")) {
        return QStringLiteral("W-w-%1:H-h-%1").arg(m);
    } else if (pos == QLatin1String("center")) {
        return QStringLiteral("(W-w)/2:(H-h)/2");
    }
    // Default: top-right
    return QStringLiteral("W-w-%1:%1").arg(m);
}
} // namespace

BatchPipeline::BatchPipeline(const QString &name, const PipelineStep &step)
    : m_name(name)
    , m_step(step)
{
}

QString BatchPipeline::name() const { return m_name; }
void BatchPipeline::setName(const QString &name) { m_name = name; }

PipelineStep BatchPipeline::settings() const { return m_step; }
void BatchPipeline::setSettings(const PipelineStep &settings) { m_step = settings; }

QString BatchPipeline::buildVideoFilter() const
{
    QStringList filters;

    // 1. Crop
    if (m_step.enableCrop && m_step.cropW > 0 && m_step.cropH > 0) {
        filters << QStringLiteral("crop=%1:%2:%3:%4")
                       .arg(m_step.cropW)
                       .arg(m_step.cropH)
                       .arg(qMax(0, m_step.cropX))
                       .arg(qMax(0, m_step.cropY));
    }

    // 2. Scale / Resize
    if (m_step.enableScale && (m_step.scaleWidth != 0 || m_step.scaleHeight != 0)) {
        int w = m_step.scaleWidth;
        int h = m_step.scaleHeight;
        if (w > 0 && h > 0) {
            filters << QStringLiteral("scale=%1:%2").arg(w).arg(h);
        } else if (w > 0) {
            filters << QStringLiteral("scale=%1:-2").arg(w);
        } else if (h > 0) {
            filters << QStringLiteral("scale=-2:%1").arg(h);
        }
    }

    // 3. Frame rate / FPS
    if (m_step.enableFps && m_step.fps > 0) {
        filters << QStringLiteral("fps=%1").arg(m_step.fps);
    }

    return filters.join(QLatin1Char(','));
}

QString BatchPipeline::buildAudioFilter() const
{
    if (m_step.mute || !m_step.enableAudio) {
        return QString();
    }

    QStringList filters;

    // Volume adjustment
    if (!qFuzzyCompare(m_step.volumeFactor, 1.0) && m_step.volumeFactor > 0.0) {
        filters << QStringLiteral("volume=%1").arg(QString::number(m_step.volumeFactor, 'f', 2));
    }

    // Loudness normalization (EBU R128)
    if (m_step.loudnorm) {
        filters << QStringLiteral("loudnorm=I=-16:TP=-1.5:LRA=11");
    }

    return filters.join(QLatin1Char(','));
}

QStringList BatchPipeline::buildCommand(const QString &inputPath, const QString &outputPath,
                                       const FFmpegCapabilities *caps) const
{
    QStringList args;
    args << QStringLiteral("-hide_banner") << QStringLiteral("-y");

    // Fast keyframe seek before input
    if (m_step.enableTrim && m_step.startSec > 0.0) {
        args << QStringLiteral("-ss") << FFmpegCommandBuilder::formatTime(m_step.startSec);
    }

    // First input
    args << QStringLiteral("-i") << inputPath;

    // Second input (watermark) if enabled
    const bool hasWatermark = m_step.enableWatermark && !m_step.watermarkPath.isEmpty();
    if (hasWatermark) {
        args << QStringLiteral("-i") << m_step.watermarkPath;
    }

    // End time limit
    if (m_step.enableTrim && m_step.endSec > m_step.startSec) {
        const double duration = m_step.endSec - m_step.startSec;
        args << QStringLiteral("-t") << FFmpegCommandBuilder::formatTime(duration);
    }

    // Determine target container extension
    QString outExt = QFileInfo(outputPath).suffix().toLower();
    if (outExt.isEmpty()) {
        outExt = m_step.outputFormat.toLower();
    }

    const bool isAudioOnly = RemuxPolicy::isAudioContainer(outExt);

    if (isAudioOnly) {
        // Pure audio pipeline
        args << QStringLiteral("-vn");
        args << QStringLiteral("-map") << QStringLiteral("0:a?");

        const QString afilter = buildAudioFilter();
        if (!afilter.isEmpty()) {
            args << QStringLiteral("-af") << afilter;
        }

        const QString ac = sanitizeCodec(m_step.audioCodec);
        if (!ac.isEmpty() && ac != QLatin1String("copy")) {
            args << QStringLiteral("-c:a") << ac;
            if (!m_step.audioBitrate.isEmpty()) {
                args << QStringLiteral("-b:a") << m_step.audioBitrate;
            }
        } else if (ac == QLatin1String("copy")) {
            args << QStringLiteral("-c:a") << QStringLiteral("copy");
        }
    } else {
        // Video pipeline
        const QString vfilter = buildVideoFilter();

        if (hasWatermark) {
            // Complex filter for watermark overlay
            QStringList filterComplexParts;
            QString currentBase = QStringLiteral("[0:v]");
            if (!vfilter.isEmpty()) {
                filterComplexParts << QStringLiteral("[0:v]%1[vbase]").arg(vfilter);
                currentBase = QStringLiteral("[vbase]");
            }

            const double opacity = qBound(0.1, m_step.watermarkOpacity / 100.0, 1.0);
            filterComplexParts << QStringLiteral("[1:v]format=rgba,colorchannelmixer=aa=%1[wm]")
                                      .arg(QString::number(opacity, 'f', 2));

            const QString coords = watermarkOverlayCoordinates(m_step.watermarkPosition, m_step.watermarkMargin);
            filterComplexParts << QStringLiteral("%1[wm]overlay=%2[vout]").arg(currentBase, coords);

            args << QStringLiteral("-filter_complex") << filterComplexParts.join(QLatin1String("; "));
            args << QStringLiteral("-map") << QStringLiteral("[vout]");
        } else {
            if (!vfilter.isEmpty()) {
                args << QStringLiteral("-vf") << vfilter;
            }
            args << QStringLiteral("-map") << QStringLiteral("0:v?");
        }

        // Video codec & quality
        QString vc = sanitizeCodec(m_step.videoCodec);
        if (m_step.hwAccel && caps && caps->hasHwAcceleration()) {
            const QString hwEnc = caps->bestHwEncoder(m_step.videoCodec);
            if (!hwEnc.isEmpty()) {
                vc = hwEnc;
            }
        }

        if (!vc.isEmpty() && vc != QLatin1String("copy")) {
            args << QStringLiteral("-c:v") << vc;

            if (m_step.videoBitrateKbps > 0) {
                args << QStringLiteral("-b:v") << QStringLiteral("%1k").arg(m_step.videoBitrateKbps);
            } else if (m_step.crf >= 0) {
                args << QStringLiteral("-crf") << QString::number(m_step.crf);
            }

            if (!m_step.speedPreset.isEmpty()) {
                args << QStringLiteral("-preset") << m_step.speedPreset;
            }
        } else if (vc == QLatin1String("copy")) {
            args << QStringLiteral("-c:v") << QStringLiteral("copy");
        }

        // Audio stream handling
        if (m_step.mute) {
            args << QStringLiteral("-an");
        } else {
            if (m_step.preserveAllStreams) {
                args << QStringLiteral("-map") << QStringLiteral("0:a?");
            } else {
                args << QStringLiteral("-map") << QStringLiteral("0:a:0?");
            }

            const QString afilter = buildAudioFilter();
            if (!afilter.isEmpty()) {
                args << QStringLiteral("-af") << afilter;
            }

            const QString ac = sanitizeCodec(m_step.audioCodec);
            if (!ac.isEmpty() && ac != QLatin1String("copy")) {
                args << QStringLiteral("-c:a") << ac;
                if (!m_step.audioBitrate.isEmpty()) {
                    args << QStringLiteral("-b:a") << m_step.audioBitrate;
                }
            } else if (ac == QLatin1String("copy")) {
                args << QStringLiteral("-c:a") << QStringLiteral("copy");
            }
        }

        // Preserve subtitles if container supports and all streams requested
        if (m_step.preserveAllStreams && RemuxPolicy::containerSupportsSubtitleCodec(outExt, QStringLiteral("subrip"))) {
            args << QStringLiteral("-map") << QStringLiteral("0:s?");
            args << QStringLiteral("-c:s") << QStringLiteral("copy");
        }
    }

    // Target output
    args << outputPath;
    return args;
}

QJsonObject BatchPipeline::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("name")] = m_name;

    QJsonObject step;
    step[QStringLiteral("enableTrim")] = m_step.enableTrim;
    step[QStringLiteral("startSec")] = m_step.startSec;
    step[QStringLiteral("endSec")] = m_step.endSec;

    step[QStringLiteral("enableCrop")] = m_step.enableCrop;
    step[QStringLiteral("cropX")] = m_step.cropX;
    step[QStringLiteral("cropY")] = m_step.cropY;
    step[QStringLiteral("cropW")] = m_step.cropW;
    step[QStringLiteral("cropH")] = m_step.cropH;

    step[QStringLiteral("enableScale")] = m_step.enableScale;
    step[QStringLiteral("scaleWidth")] = m_step.scaleWidth;
    step[QStringLiteral("scaleHeight")] = m_step.scaleHeight;

    step[QStringLiteral("enableFps")] = m_step.enableFps;
    step[QStringLiteral("fps")] = m_step.fps;

    step[QStringLiteral("enableWatermark")] = m_step.enableWatermark;
    step[QStringLiteral("watermarkPath")] = m_step.watermarkPath;
    step[QStringLiteral("watermarkPosition")] = m_step.watermarkPosition;
    step[QStringLiteral("watermarkOpacity")] = m_step.watermarkOpacity;
    step[QStringLiteral("watermarkMargin")] = m_step.watermarkMargin;

    step[QStringLiteral("enableAudio")] = m_step.enableAudio;
    step[QStringLiteral("volumeFactor")] = m_step.volumeFactor;
    step[QStringLiteral("loudnorm")] = m_step.loudnorm;
    step[QStringLiteral("mute")] = m_step.mute;

    step[QStringLiteral("outputFormat")] = m_step.outputFormat;
    step[QStringLiteral("videoCodec")] = m_step.videoCodec;
    step[QStringLiteral("audioCodec")] = m_step.audioCodec;
    step[QStringLiteral("crf")] = m_step.crf;
    step[QStringLiteral("videoBitrateKbps")] = m_step.videoBitrateKbps;
    step[QStringLiteral("audioBitrate")] = m_step.audioBitrate;
    step[QStringLiteral("speedPreset")] = m_step.speedPreset;
    step[QStringLiteral("hwAccel")] = m_step.hwAccel;
    step[QStringLiteral("preserveAllStreams")] = m_step.preserveAllStreams;

    obj[QStringLiteral("step")] = step;
    return obj;
}

BatchPipeline BatchPipeline::fromJson(const QJsonObject &obj)
{
    BatchPipeline pipeline;
    pipeline.m_name = obj.value(QStringLiteral("name")).toString();

    const QJsonObject step = obj.value(QStringLiteral("step")).toObject();
    pipeline.m_step.enableTrim = step.value(QStringLiteral("enableTrim")).toBool();
    pipeline.m_step.startSec = step.value(QStringLiteral("startSec")).toDouble();
    pipeline.m_step.endSec = step.value(QStringLiteral("endSec")).toDouble();

    pipeline.m_step.enableCrop = step.value(QStringLiteral("enableCrop")).toBool();
    pipeline.m_step.cropX = step.value(QStringLiteral("cropX")).toInt();
    pipeline.m_step.cropY = step.value(QStringLiteral("cropY")).toInt();
    pipeline.m_step.cropW = step.value(QStringLiteral("cropW")).toInt();
    pipeline.m_step.cropH = step.value(QStringLiteral("cropH")).toInt();

    pipeline.m_step.enableScale = step.value(QStringLiteral("enableScale")).toBool();
    pipeline.m_step.scaleWidth = step.value(QStringLiteral("scaleWidth")).toInt();
    pipeline.m_step.scaleHeight = step.value(QStringLiteral("scaleHeight")).toInt();

    pipeline.m_step.enableFps = step.value(QStringLiteral("enableFps")).toBool();
    pipeline.m_step.fps = step.value(QStringLiteral("fps")).toInt();

    pipeline.m_step.enableWatermark = step.value(QStringLiteral("enableWatermark")).toBool();
    pipeline.m_step.watermarkPath = step.value(QStringLiteral("watermarkPath")).toString();
    pipeline.m_step.watermarkPosition = step.value(QStringLiteral("watermarkPosition")).toString(QStringLiteral("top-right"));
    pipeline.m_step.watermarkOpacity = step.value(QStringLiteral("watermarkOpacity")).toInt(80);
    pipeline.m_step.watermarkMargin = step.value(QStringLiteral("watermarkMargin")).toInt(10);

    pipeline.m_step.enableAudio = step.value(QStringLiteral("enableAudio")).toBool();
    pipeline.m_step.volumeFactor = step.value(QStringLiteral("volumeFactor")).toDouble(1.0);
    pipeline.m_step.loudnorm = step.value(QStringLiteral("loudnorm")).toBool();
    pipeline.m_step.mute = step.value(QStringLiteral("mute")).toBool();

    pipeline.m_step.outputFormat = step.value(QStringLiteral("outputFormat")).toString(QStringLiteral("mp4"));
    pipeline.m_step.videoCodec = step.value(QStringLiteral("videoCodec")).toString(QStringLiteral("H.264"));
    pipeline.m_step.audioCodec = step.value(QStringLiteral("audioCodec")).toString(QStringLiteral("AAC"));
    pipeline.m_step.crf = step.value(QStringLiteral("crf")).toInt(23);
    pipeline.m_step.videoBitrateKbps = step.value(QStringLiteral("videoBitrateKbps")).toInt(-1);
    pipeline.m_step.audioBitrate = step.value(QStringLiteral("audioBitrate")).toString(QStringLiteral("192k"));
    pipeline.m_step.speedPreset = step.value(QStringLiteral("speedPreset")).toString();
    pipeline.m_step.hwAccel = step.value(QStringLiteral("hwAccel")).toBool(false);
    pipeline.m_step.preserveAllStreams = step.value(QStringLiteral("preserveAllStreams")).toBool(true);

    return pipeline;
}

QVariantMap BatchPipeline::toVariantMap() const
{
    return toJson().toVariantMap();
}

BatchPipeline BatchPipeline::fromVariantMap(const QVariantMap &map)
{
    return fromJson(QJsonObject::fromVariantMap(map));
}
