#include "FFmpegCommandBuilder.h"

FFmpegCommandBuilder::FFmpegCommandBuilder(QObject *parent)
    : QObject(parent)
{
}

QStringList FFmpegCommandBuilder::buildConversion(
    const QString &inputPath,
    const QString &outputPath,
    const QString &outputFormat,
    const QString &videoCodec,
    const QString &audioCodec,
    int crf,
    bool copyStreams
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

QString FFmpegCommandBuilder::escapeFilterValue(const QString &value)
{
    QString result = value;
    result.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    result.replace(QLatin1Char('\''), QStringLiteral("\\'"));
    result.replace(QLatin1Char(';'), QStringLiteral("\\;"));
    return result;
}
