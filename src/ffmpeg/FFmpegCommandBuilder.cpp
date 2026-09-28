#include "FFmpegCommandBuilder.h"

FFmpegCommandBuilder::FFmpegCommandBuilder(QObject *parent)
    : QObject(parent)
{
}

QStringList FFmpegCommandBuilder::buildConversion(
    const QString &inputPath,
    const QString &outputPath,
    const QString &videoCodec,
    const QString &audioCodec,
    int crf,
    bool copyStreams,
    bool stripMetadata
) const
{
    QStringList args;

    if (inputPath.isEmpty() || outputPath.isEmpty()) {
        return args;
    }

    args << QStringLiteral("-y");
    args << QStringLiteral("-i") << inputPath;

    if (copyStreams) {
        args << QStringLiteral("-c") << QStringLiteral("copy");
    } else {
        if (!videoCodec.isEmpty()) {
            args << QStringLiteral("-c:v") << videoCodec;
        }
        if (!audioCodec.isEmpty()) {
            args << QStringLiteral("-c:a") << audioCodec;
        }
        if (crf >= 0) {
            args << QStringLiteral("-crf") << QString::number(crf);
        }
    }

    if (stripMetadata) {
        args << QStringLiteral("-map_metadata") << QStringLiteral("-1");
    }

    args << outputPath;

    return args;
}

QStringList FFmpegCommandBuilder::buildImageOperation(
    const QString &inputPath,
    const QString &outputPath,
    int cropX, int cropY, int cropW, int cropH,
    int resizeW, int resizeH,
    int rotate,
    bool flipH, bool flipV,
    const QString &outputFormat,
    int quality
) const
{
    QStringList args;

    if (inputPath.isEmpty() || outputPath.isEmpty()) {
        return args;
    }

    args << QStringLiteral("-y");
    args << QStringLiteral("-i") << inputPath;

    QStringList filters;

    if (cropW > 0 && cropH > 0) {
        filters << QStringLiteral("crop=%1:%2:%3:%4")
                      .arg(cropW)
                      .arg(cropH)
                      .arg(cropX)
                      .arg(cropY);
    }

    if (resizeW > 0 && resizeH > 0) {
        filters << QStringLiteral("scale=%1:%2")
                      .arg(resizeW)
                      .arg(resizeH);
    }

    if (rotate == 90) {
        filters << QStringLiteral("transpose=1");
    } else if (rotate == 180) {
        filters << QStringLiteral("transpose=1,transpose=1");
    } else if (rotate == 270) {
        filters << QStringLiteral("transpose=2");
    }

    if (flipH) {
        filters << QStringLiteral("hflip");
    }
    if (flipV) {
        filters << QStringLiteral("vflip");
    }

    if (!filters.isEmpty()) {
        args << QStringLiteral("-vf") << filters.join(QLatin1Char(','));
    }

    if (!outputFormat.isEmpty()) {
        args << QStringLiteral("-f") << outputFormat;
    }

    if (quality >= 0) {
        args << QStringLiteral("-q:v") << QString::number(quality);
    }

    args << outputPath;

    return args;
}

QStringList FFmpegCommandBuilder::buildTrim(
    const QString &inputPath,
    const QString &outputPath,
    double startSec, double endSec,
    const QString &outputFormat,
    const QString &videoCodec,
    const QString &audioCodec,
    int crf,
    bool copyStreams
) const
{
    QStringList args;
    if (inputPath.isEmpty() || outputPath.isEmpty()) return args;

    args << QStringLiteral("-y");
    if (startSec > 0) {
        args << QStringLiteral("-ss") << formatTime(startSec);
    }
    args << QStringLiteral("-i") << inputPath;
    if (endSec > 0) {
        args << QStringLiteral("-to") << formatTime(endSec);
    }

    if (copyStreams) {
        args << QStringLiteral("-c") << QStringLiteral("copy");
    } else {
        if (!videoCodec.isEmpty()) args << QStringLiteral("-c:v") << videoCodec;
        if (!audioCodec.isEmpty()) args << QStringLiteral("-c:a") << audioCodec;
        if (crf >= 0) args << QStringLiteral("-crf") << QString::number(crf);
    }
    if (!outputFormat.isEmpty()) {
        args << QStringLiteral("-f") << outputFormat;
    }
    args << outputPath;
    return args;
}

QStringList FFmpegCommandBuilder::buildExtractAudio(
    const QString &inputPath,
    const QString &outputPath,
    const QString &audioFormat,
    const QString &audioCodec,
    int bitrateKbps
) const
{
    QStringList args;
    if (inputPath.isEmpty() || outputPath.isEmpty()) return args;

    args << QStringLiteral("-y");
    args << QStringLiteral("-i") << inputPath;
    args << QStringLiteral("-vn");
    if (!audioCodec.isEmpty()) {
        args << QStringLiteral("-c:a") << audioCodec;
    }
    if (bitrateKbps > 0) {
        args << QStringLiteral("-b:a") << QStringLiteral("%1k").arg(bitrateKbps);
    }
    if (!audioFormat.isEmpty()) {
        args << QStringLiteral("-f") << audioFormat;
    }
    args << outputPath;
    return args;
}

QStringList FFmpegCommandBuilder::buildMute(
    const QString &inputPath,
    const QString &outputPath,
    const QString &outputFormat,
    const QString &videoCodec,
    int crf,
    bool copyStreams
) const
{
    QStringList args;
    if (inputPath.isEmpty() || outputPath.isEmpty()) return args;

    args << QStringLiteral("-y");
    args << QStringLiteral("-i") << inputPath;
    args << QStringLiteral("-an");
    if (copyStreams) {
        args << QStringLiteral("-c:v") << QStringLiteral("copy");
    } else {
        if (!videoCodec.isEmpty()) args << QStringLiteral("-c:v") << videoCodec;
        if (crf >= 0) args << QStringLiteral("-crf") << QString::number(crf);
    }
    if (!outputFormat.isEmpty()) {
        args << QStringLiteral("-f") << outputFormat;
    }
    args << outputPath;
    return args;
}

QStringList FFmpegCommandBuilder::buildScreenshot(
    const QString &inputPath,
    const QString &outputPath,
    double timeSec,
    const QString &imageFormat
) const
{
    QStringList args;
    if (inputPath.isEmpty() || outputPath.isEmpty()) return args;

    args << QStringLiteral("-y");
    if (timeSec > 0) {
        args << QStringLiteral("-ss") << formatTime(timeSec);
    }
    args << QStringLiteral("-i") << inputPath;
    args << QStringLiteral("-frames:v") << QStringLiteral("1");
    if (!imageFormat.isEmpty()) {
        args << QStringLiteral("-f") << imageFormat;
    }
    args << outputPath;
    return args;
}

QStringList FFmpegCommandBuilder::buildGif(
    const QString &inputPath,
    const QString &outputPath,
    int fps, int width,
    double startSec, double endSec
) const
{
    QStringList args;
    if (inputPath.isEmpty() || outputPath.isEmpty()) return args;

    args << QStringLiteral("-y");
    if (startSec > 0) {
        args << QStringLiteral("-ss") << formatTime(startSec);
    }
    args << QStringLiteral("-i") << inputPath;
    if (endSec > 0) {
        args << QStringLiteral("-to") << formatTime(endSec);
    }

    QString filter = QStringLiteral("fps=%1").arg(fps > 0 ? fps : 12);
    if (width > 0) {
        filter += QStringLiteral(",scale=%1:-1:flags=lanczos").arg(width);
    }
    args << QStringLiteral("-vf") << filter;
    args << QStringLiteral("-loop") << QStringLiteral("0");
    args << outputPath;
    return args;
}

QStringList FFmpegCommandBuilder::buildVideoJoin(
    const QStringList &inputPaths,
    const QString &outputPath
) const
{
    QStringList args;
    if (inputPaths.size() < 2 || outputPath.isEmpty()) return args;

    args << QStringLiteral("-y");
    for (const QString &file : inputPaths) {
        args << QStringLiteral("-i") << file;
    }

    QString filter;
    for (int i = 0; i < inputPaths.size(); ++i) {
        filter += QStringLiteral("[%1:v][%1:a]").arg(i);
    }
    filter += QStringLiteral("concat=n=%1:v=1:a=1[outv][outa]").arg(inputPaths.size());

    args << QStringLiteral("-filter_complex") << filter;
    args << QStringLiteral("-map") << QStringLiteral("[outv]");
    args << QStringLiteral("-map") << QStringLiteral("[outa]");
    args << outputPath;
    return args;
}

QStringList FFmpegCommandBuilder::buildGridJoin(
    const QStringList &inputPaths,
    const QString &outputPath,
    int cols,
    int spacing,
    const QString &backgroundColor,
    int padding
) const
{
    QStringList args;
    if (inputPaths.isEmpty() || outputPath.isEmpty()) return args;
    if (cols < 1) cols = 1;

    args << QStringLiteral("-y");
    for (const QString &file : inputPaths) {
        args << QStringLiteral("-i") << file;
    }

    QString filter;
    if (spacing > 0) {
        for (int i = 0; i < inputPaths.size(); ++i) {
            filter += QStringLiteral("[%1:v]pad=iw+%2:ih+%2:%2:%2:color=%3[p%1];")
                          .arg(i)
                          .arg(spacing)
                          .arg(backgroundColor.isEmpty() ? QStringLiteral("black") : backgroundColor);
        }
    }
    for (int i = 0; i < inputPaths.size(); ++i) {
        filter += (spacing > 0 ? QStringLiteral("[p%1]").arg(i)
                               : QStringLiteral("[%1:v]").arg(i));
    }
    filter += QStringLiteral("xstack=inputs=%1:layout=").arg(inputPaths.size());

    QStringList layout;
    for (int i = 0; i < inputPaths.size(); ++i) {
        const int r = i / cols;
        const int c = i % cols;
        const QString x = c == 0 ? QStringLiteral("0") : QStringLiteral("%1*w0").arg(c);
        const QString y = r == 0 ? QStringLiteral("0") : QStringLiteral("%1*h0").arg(r);
        layout << x + QLatin1Char('_') + y;
    }
    filter += layout.join(QLatin1Char('|'));
    filter += QStringLiteral("[outv]");

    QString mapTarget = QStringLiteral("[outv]");
    if (padding > 0) {
        filter += QStringLiteral(";[outv]pad=iw+%1:ih+%1:%2:%2:color=%3[outp]")
                      .arg(padding * 2)
                      .arg(padding)
                      .arg(backgroundColor);
        mapTarget = QStringLiteral("[outp]");
    }

    args << QStringLiteral("-filter_complex") << filter;
    args << QStringLiteral("-map") << mapTarget;
    args << outputPath;
    return args;
}

QStringList FFmpegCommandBuilder::buildImageSequenceToVideo(
    const QString &inputPattern,
    const QString &outputPath,
    int fps
) const
{
    QStringList args;
    if (inputPattern.isEmpty() || outputPath.isEmpty()) return args;

    args << QStringLiteral("-y");
    args << QStringLiteral("-framerate") << QString::number(fps > 0 ? fps : 25);
    args << QStringLiteral("-i") << inputPattern;
    args << QStringLiteral("-pix_fmt") << QStringLiteral("yuv420p");
    args << outputPath;
    return args;
}

QStringList FFmpegCommandBuilder::buildVideoToImageSequence(
    const QString &inputPath,
    const QString &outputPattern,
    double fps
) const
{
    QStringList args;
    if (inputPath.isEmpty() || outputPattern.isEmpty()) return args;

    args << QStringLiteral("-y");
    args << QStringLiteral("-i") << inputPath;
    if (fps > 0) {
        args << QStringLiteral("-vf") << QStringLiteral("fps=%1").arg(fps);
    }
    args << outputPattern;
    return args;
}

QStringList FFmpegCommandBuilder::buildSubtitleMux(
    const QString &inputPath,
    const QString &subtitlePath,
    const QString &outputPath,
    const QString &subtitleCodec
) const
{
    if (inputPath.isEmpty() || subtitlePath.isEmpty() || outputPath.isEmpty()) {
        return {};
    }

    QStringList args;
    args << QStringLiteral("-y");
    args << QStringLiteral("-i") << inputPath;
    args << QStringLiteral("-i") << subtitlePath;
    // Keep every stream of the source plus the new subtitle stream.
    args << QStringLiteral("-map") << QStringLiteral("0");
    args << QStringLiteral("-map") << QStringLiteral("1");
    args << QStringLiteral("-c") << QStringLiteral("copy");
    if (!subtitleCodec.isEmpty()) {
        args << QStringLiteral("-c:s") << subtitleCodec;
    }
    args << outputPath;
    return args;
}

QStringList FFmpegCommandBuilder::buildSubtitleBurnin(
    const QString &inputPath,
    const QString &subtitlePath,
    const QString &outputPath,
    int crf
) const
{
    if (inputPath.isEmpty() || subtitlePath.isEmpty() || outputPath.isEmpty()) {
        return {};
    }

    QStringList args;
    args << QStringLiteral("-y");
    args << QStringLiteral("-i") << inputPath;
    args << QStringLiteral("-i") << subtitlePath;
    args << QStringLiteral("-filter_complex")
         << QStringLiteral("[0:v][1:s]subtitles=filename=%1[v]")
                .arg(escapeFilterPath(subtitlePath));
    args << QStringLiteral("-map") << QStringLiteral("[v]");
    args << QStringLiteral("-map") << QStringLiteral("0:a?");
    args << QStringLiteral("-c:a") << QStringLiteral("copy");
    if (crf >= 0) {
        args << QStringLiteral("-crf") << QString::number(crf);
    }
    args << outputPath;
    return args;
}

QStringList FFmpegCommandBuilder::buildWatermark(
    const QString &inputPath,
    const QString &watermarkPath,
    const QString &outputPath,
    const QString &position,
    int opacityPercent,
    const QString &outputFormat
) const
{
    QStringList args;
    if (inputPath.isEmpty() || watermarkPath.isEmpty() || outputPath.isEmpty()) return args;

    const double opacity = qBound(0, opacityPercent, 100) / 100.0;

    args << QStringLiteral("-y");
    args << QStringLiteral("-i") << inputPath;
    args << QStringLiteral("-i") << watermarkPath;

    QString overlay;
    if (position == QLatin1String("top-left")) {
        overlay = QStringLiteral("10:10");
    } else if (position == QLatin1String("top-right")) {
        overlay = QStringLiteral("main_w-overlay_w-10:10");
    } else if (position == QLatin1String("bottom-left")) {
        overlay = QStringLiteral("10:main_h-overlay_h-10");
    } else if (position == QLatin1String("center")) {
        overlay = QStringLiteral("(main_w-overlay_w)/2:(main_h-overlay_h)/2");
    } else {
        overlay = QStringLiteral("main_w-overlay_w-10:main_h-overlay_h-10");
    }

    const QString filter = QStringLiteral("[1:v]format=rgba,colorchannelmixer=aa=%1[wm];[0:v][wm]overlay=%2")
                               .arg(opacity)
                               .arg(overlay);

    args << QStringLiteral("-filter_complex") << filter;
    args << QStringLiteral("-map") << QStringLiteral("0:a?");
    if (!outputFormat.isEmpty()) {
        args << QStringLiteral("-f") << outputFormat;
    }
    args << outputPath;
    return args;
}

QString FFmpegCommandBuilder::escapeFilterPath(const QString &path)
{
    QString result = path;
    // Forward slashes work in ffmpeg on Windows and dodge the backslash
    // escaping maze; the colon still has to be escaped for the option parser.
    result.replace(QLatin1Char('\\'), QLatin1Char('/'));
    result.replace(QLatin1Char(':'), QStringLiteral("\\:"));
    result.replace(QLatin1Char('\''), QStringLiteral("\\'"));
    return QStringLiteral("'") + result + QStringLiteral("'");
}

QString FFmpegCommandBuilder::formatTime(double seconds)
{
    const int total = static_cast<int>(seconds);
    const int h = total / 3600;
    const int m = (total % 3600) / 60;
    const int s = total % 60;
    return QStringLiteral("%1:%2:%3")
        .arg(h, 2, 10, QLatin1Char('0'))
        .arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0'));
}

QString FFmpegCommandBuilder::escapeFilterValue(const QString &value)
{
    QString result = value;
    result.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    result.replace(QLatin1Char('\''), QStringLiteral("\\'"));
    result.replace(QLatin1Char(';'), QStringLiteral("\\;"));
    return result;
}
