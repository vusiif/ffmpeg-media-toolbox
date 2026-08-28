#include "FFmpegCommandBuilder.h"

FFmpegCommandBuilder::FFmpegCommandBuilder(QObject *parent)
    : QObject(parent)
{
}

FFmpegCommand FFmpegCommandBuilder::buildConversion(const ConversionSettings &settings) const
{
    FFmpegCommand cmd;

    if (settings.inputPath.isEmpty() || settings.outputPath.isEmpty()) {
        return cmd;
    }

    cmd.arguments << QStringLiteral("-y");
    cmd.arguments << QStringLiteral("-i") << settings.inputPath;

    if (settings.copyStreams) {
        cmd.arguments << QStringLiteral("-c") << QStringLiteral("copy");
    } else {
        if (!settings.videoCodec.isEmpty()) {
            cmd.arguments << QStringLiteral("-c:v") << settings.videoCodec;
        }
        if (!settings.audioCodec.isEmpty()) {
            cmd.arguments << QStringLiteral("-c:a") << settings.audioCodec;
        }
        if (settings.crf >= 0) {
            cmd.arguments << QStringLiteral("-crf") << QString::number(settings.crf);
        }
        if (settings.videoBitrate > 0) {
            cmd.arguments << QStringLiteral("-b:v") << QStringLiteral("%1k").arg(settings.videoBitrate);
        }
        if (settings.audioBitrate > 0) {
            cmd.arguments << QStringLiteral("-b:a") << QStringLiteral("%1k").arg(settings.audioBitrate);
        }
        if (!settings.preset.isEmpty()) {
            cmd.arguments << QStringLiteral("-preset") << settings.preset;
        }
        if (!settings.pixelFormat.isEmpty()) {
            cmd.arguments << QStringLiteral("-pix_fmt") << settings.pixelFormat;
        }
        if (settings.width > 0 && settings.height > 0) {
            cmd.arguments << QStringLiteral("-vf")
                          << QStringLiteral("scale=%1:%2").arg(settings.width).arg(settings.height);
        }
        if (settings.fps > 0) {
            cmd.arguments << QStringLiteral("-r") << QString::number(settings.fps, 'f', 2);
        }
    }

    cmd.arguments << settings.extraArgs;
    cmd.arguments << settings.outputPath;

    return cmd;
}

FFmpegCommand FFmpegCommandBuilder::buildImageOperation(const ImageOperationSettings &settings) const
{
    FFmpegCommand cmd;

    if (settings.inputPath.isEmpty() || settings.outputPath.isEmpty()) {
        return cmd;
    }

    cmd.arguments << QStringLiteral("-y");
    cmd.arguments << QStringLiteral("-i") << settings.inputPath;

    QStringList filters;

    if (settings.cropW > 0 && settings.cropH > 0) {
        filters << QStringLiteral("crop=%1:%2:%3:%4")
                      .arg(settings.cropW)
                      .arg(settings.cropH)
                      .arg(settings.cropX)
                      .arg(settings.cropY);
    }

    if (settings.resizeW > 0 && settings.resizeH > 0) {
        filters << QStringLiteral("scale=%1:%2")
                      .arg(settings.resizeW)
                      .arg(settings.resizeH);
    }

    if (settings.rotate == 90) {
        filters << QStringLiteral("transpose=1");
    } else if (settings.rotate == 180) {
        filters << QStringLiteral("transpose=1,transpose=1");
    } else if (settings.rotate == 270) {
        filters << QStringLiteral("transpose=2");
    }

    if (settings.flipH) {
        filters << QStringLiteral("hflip");
    }
    if (settings.flipV) {
        filters << QStringLiteral("vflip");
    }

    if (!filters.isEmpty()) {
        cmd.arguments << QStringLiteral("-vf") << filters.join(QLatin1Char(','));
    }

    if (!settings.outputFormat.isEmpty()) {
        cmd.arguments << QStringLiteral("-f") << settings.outputFormat;
    }

    if (settings.quality >= 0) {
        cmd.arguments << QStringLiteral("-q:v") << QString::number(settings.quality);
    }

    cmd.arguments << settings.outputPath;

    return cmd;
}

QString FFmpegCommandBuilder::escapeFilterValue(const QString &value)
{
    QString result = value;
    result.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    result.replace(QLatin1Char('\''), QStringLiteral("\\'"));
    result.replace(QLatin1Char(';'), QStringLiteral("\\;"));
    return result;
}
