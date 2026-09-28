#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QtQmlIntegration/qqmlintegration.h>

class FFmpegCommandBuilder : public QObject
{
    Q_OBJECT

public:
    explicit FFmpegCommandBuilder(QObject *parent = nullptr);

    Q_INVOKABLE QStringList buildConversion(
        const QString &inputPath,
        const QString &outputPath,
        const QString &videoCodec = {},
        const QString &audioCodec = {},
        int crf = -1,
        bool copyStreams = false,
        bool stripMetadata = false
    ) const;

    Q_INVOKABLE QStringList buildImageOperation(
        const QString &inputPath,
        const QString &outputPath,
        int cropX = 0, int cropY = 0, int cropW = 0, int cropH = 0,
        int resizeW = 0, int resizeH = 0,
        int rotate = 0,
        bool flipH = false, bool flipV = false,
        const QString &outputFormat = {},
        int quality = -1
    ) const;

    QStringList buildTrim(
        const QString &inputPath,
        const QString &outputPath,
        double startSec, double endSec,
        const QString &outputFormat = {},
        const QString &videoCodec = {},
        const QString &audioCodec = {},
        int crf = -1,
        bool copyStreams = false
    ) const;

    QStringList buildExtractAudio(
        const QString &inputPath,
        const QString &outputPath,
        const QString &audioFormat = {},
        const QString &audioCodec = {},
        int bitrateKbps = -1
    ) const;

    QStringList buildMute(
        const QString &inputPath,
        const QString &outputPath,
        const QString &outputFormat = {},
        const QString &videoCodec = {},
        int crf = -1,
        bool copyStreams = false
    ) const;

    QStringList buildScreenshot(
        const QString &inputPath,
        const QString &outputPath,
        double timeSec,
        const QString &imageFormat = {}
    ) const;

    QStringList buildGif(
        const QString &inputPath,
        const QString &outputPath,
        int fps, int width,
        double startSec, double endSec
    ) const;

    QStringList buildVideoJoin(
        const QStringList &inputPaths,
        const QString &outputPath
    ) const;

    QStringList buildGridJoin(
        const QStringList &inputPaths,
        const QString &outputPath,
        int rows, int cols,
        int spacing,
        const QString &backgroundColor
    ) const;

    QStringList buildImageSequenceToVideo(
        const QString &inputPattern,
        const QString &outputPath,
        int fps
    ) const;

    QStringList buildVideoToImageSequence(
        const QString &inputPath,
        const QString &outputPattern,
        double fps
    ) const;

    QStringList buildWatermark(
        const QString &inputPath,
        const QString &watermarkPath,
        const QString &outputPath,
        const QString &position,
        int opacityPercent,
        const QString &outputFormat = {}
    ) const;

    static QString escapeFilterValue(const QString &value);
    static QString formatTime(double seconds);
};
