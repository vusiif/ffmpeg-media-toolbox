#include "AnimatedImageEngine.h"

#include <QtGlobal>
#include <QFileInfo>
#include <QTime>

namespace Animation {

bool AnimatedImageConfig::isModified() const
{
    if (format != OutputFormat::GIF) return true;
    if (fps != 15) return true;
    if (qAbs(speedMultiplier - 1.0) > 0.01) return true;
    if (direction != PlaybackDirection::Normal) return true;
    if (width != 480) return true;
    if (height != -1) return true;
    if (maxColors != 256) return true;
    if (statsMode != PaletteStatsMode::Diff) return true;
    if (dither != DitherMode::Bayer) return true;
    if (bayerScale != 3) return true;
    if (loopCount != 0) return true;
    return false;
}

void AnimatedImageConfig::reset()
{
    *this = AnimatedImageConfig();
}

int AnimatedImageConfig::totalFrames() const
{
    const double dur = qMax(0.2, durationSec > 0 ? durationSec : 3.0);
    const double speed = qBound(0.1, speedMultiplier, 10.0);
    int frames = static_cast<int>(dur * fps / speed);
    if (direction == PlaybackDirection::PingPong) {
        frames *= 2;
    }
    return qMax(1, frames);
}

qint64 AnimatedImageConfig::estimateFileSizeBytes() const
{
    const int frames = totalFrames();
    const int estW = width > 0 ? width : 640;
    const int estH = height > 0 ? height : qMax(120, estW * 9 / 16);
    const double pixels = static_cast<double>(estW) * estH;

    double bytesPerFrame = 0.0;
    switch (format) {
    case OutputFormat::GIF: {
        const double colorFactor = maxColors / 256.0;
        bytesPerFrame = pixels * 0.20 * colorFactor;
        break;
    }
    case OutputFormat::WebP: {
        if (webpLossless) {
            bytesPerFrame = pixels * 0.35;
        } else {
            bytesPerFrame = pixels * 0.08 * (qBound(10, webpQuality, 100) / 75.0);
        }
        break;
    }
    case OutputFormat::APNG:
        bytesPerFrame = pixels * 0.40;
        break;
    }

    return qMax<qint64>(30000, static_cast<qint64>(frames * bytesPerFrame));
}

QString AnimatedImageEngine::formatTime(double seconds)
{
    if (seconds <= 0.0) return QStringLiteral("00:00:00.000");
    const int totalMs = static_cast<int>(seconds * 1000.0);
    const int ms = totalMs % 1000;
    const int totalSec = totalMs / 1000;
    const int s = totalSec % 60;
    const int totalMin = totalSec / 60;
    const int m = totalMin % 60;
    const int h = totalMin / 60;

    return QStringLiteral("%1:%2:%3.%4")
        .arg(h, 2, 10, QLatin1Char('0'))
        .arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0'))
        .arg(ms, 3, 10, QLatin1Char('0'));
}

QString AnimatedImageEngine::ditherModeString(DitherMode mode, int bayerScale)
{
    switch (mode) {
    case DitherMode::FloydSteinberg:
        return QStringLiteral("floyd_steinberg");
    case DitherMode::Sierra2_4a:
        return QStringLiteral("sierra2_4a");
    case DitherMode::None:
        return QStringLiteral("none");
    case DitherMode::Bayer:
    default:
        return QStringLiteral("bayer:bayer_scale=%1").arg(qBound(1, bayerScale, 5));
    }
}

QString AnimatedImageEngine::buildFilterGraph(const AnimatedImageConfig &config)
{
    QStringList baseFilters;

    // 1. Frame rate
    const int targetFps = qBound(1, config.fps, 60);
    baseFilters << QStringLiteral("fps=%1").arg(targetFps);

    // 2. Playback speed
    if (qAbs(config.speedMultiplier - 1.0) > 0.01) {
        const double factor = qBound(0.1, config.speedMultiplier, 10.0);
        baseFilters << QStringLiteral("setpts=PTS/%1").arg(factor, 0, 'f', 2);
    }

    // 3. Scaling
    if (config.width > 0 && config.height > 0) {
        baseFilters << QStringLiteral("scale=%1:%2:flags=lanczos").arg(config.width).arg(config.height);
    } else if (config.width > 0) {
        baseFilters << QStringLiteral("scale=%1:-1:flags=lanczos").arg(config.width);
    } else if (config.height > 0) {
        baseFilters << QStringLiteral("scale=-1:%1:flags=lanczos").arg(config.height);
    }

    // 4. Direction (Reverse / PingPong)
    if (config.direction == PlaybackDirection::Reverse) {
        baseFilters << QStringLiteral("reverse");
    }

    const QString baseChain = baseFilters.join(QStringLiteral(","));

    if (config.format == OutputFormat::GIF) {
        const QString stats = (config.statsMode == PaletteStatsMode::Full)
                                  ? QStringLiteral("full")
                                  : QStringLiteral("diff");
        const int colors = qBound(32, config.maxColors, 256);
        const QString dither = ditherModeString(config.dither, config.bayerScale);
        const QString diffRect = config.diffModeRectangle ? QStringLiteral(":diff_mode=rectangle") : QString();

        if (config.direction == PlaybackDirection::PingPong) {
            return QStringLiteral("[0:v]%1,split[fwd][bwd];"
                                  "[bwd]reverse[rev];"
                                  "[fwd][rev]concat=n=2:v=1:a=0,split[v0][v1];"
                                  "[v0]palettegen=max_colors=%2:stats_mode=%3:reserve_transparent=1[pal];"
                                  "[v1][pal]paletteuse=dither=%4%5")
                .arg(baseChain)
                .arg(colors)
                .arg(stats)
                .arg(dither)
                .arg(diffRect);
        }

        return QStringLiteral("[0:v]%1,split[v0][v1];"
                              "[v0]palettegen=max_colors=%2:stats_mode=%3:reserve_transparent=1[pal];"
                              "[v1][pal]paletteuse=dither=%4%5")
            .arg(baseChain)
            .arg(colors)
            .arg(stats)
            .arg(dither)
            .arg(diffRect);
    }

    // WebP or APNG
    if (config.direction == PlaybackDirection::PingPong) {
        return QStringLiteral("[0:v]%1,split[fwd][bwd];"
                              "[bwd]reverse[rev];"
                              "[fwd][rev]concat=n=2:v=1:a=0[outv]")
            .arg(baseChain);
    }

    return baseChain;
}

QStringList AnimatedImageEngine::buildCommand(
    const QString &inputPath,
    const QString &outputPath,
    const AnimatedImageConfig &config)
{
    if (inputPath.isEmpty() || outputPath.isEmpty()) {
        return {};
    }

    QStringList args;
    args << QStringLiteral("-y");

    if (config.startSec > 0.0) {
        args << QStringLiteral("-ss") << formatTime(config.startSec);
    }

    args << QStringLiteral("-i") << inputPath;

    if (config.durationSec > 0.0) {
        args << QStringLiteral("-t") << formatTime(config.durationSec);
    }

    const QString filter = buildFilterGraph(config);

    if (config.format == OutputFormat::GIF || config.direction == PlaybackDirection::PingPong) {
        args << QStringLiteral("-filter_complex") << filter;
    } else {
        args << QStringLiteral("-vf") << filter;
    }

    switch (config.format) {
    case OutputFormat::GIF:
        args << QStringLiteral("-loop") << QString::number(config.loopCount);
        break;

    case OutputFormat::WebP:
        args << QStringLiteral("-vcodec") << QStringLiteral("libwebp");
        if (config.webpLossless) {
            args << QStringLiteral("-lossless") << QStringLiteral("1");
        } else {
            args << QStringLiteral("-lossless") << QStringLiteral("0");
            args << QStringLiteral("-q:v") << QString::number(qBound(0, config.webpQuality, 100));
        }
        args << QStringLiteral("-loop") << QString::number(config.loopCount);
        args << QStringLiteral("-an");
        break;

    case OutputFormat::APNG:
        args << QStringLiteral("-vcodec") << QStringLiteral("apng");
        args << QStringLiteral("-plays") << QString::number(config.loopCount);
        args << QStringLiteral("-an");
        break;
    }

    args << outputPath;
    return args;
}

} // namespace Animation
