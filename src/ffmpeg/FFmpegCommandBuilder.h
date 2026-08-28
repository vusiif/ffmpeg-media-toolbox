#pragma once

#include <QObject>
#include <QString>
#include "FFmpegCommand.h"

struct ConversionSettings {
    QString inputPath;
    QString outputPath;
    QString outputFormat;
    QString videoCodec;
    QString audioCodec;
    int videoBitrate = 0;
    int audioBitrate = 0;
    int crf = -1;
    QString preset;
    QString pixelFormat;
    int width = 0;
    int height = 0;
    double fps = 0;
    bool copyStreams = false;
    QStringList extraArgs;
};

struct ImageOperationSettings {
    QString inputPath;
    QString outputPath;
    int cropX = 0;
    int cropY = 0;
    int cropW = 0;
    int cropH = 0;
    int resizeW = 0;
    int resizeH = 0;
    int rotate = 0;
    bool flipH = false;
    bool flipV = false;
    QString outputFormat;
    int quality = -1;
};

class FFmpegCommandBuilder : public QObject
{
    Q_OBJECT

public:
    explicit FFmpegCommandBuilder(QObject *parent = nullptr);

    FFmpegCommand buildConversion(const ConversionSettings &settings) const;
    FFmpegCommand buildImageOperation(const ImageOperationSettings &settings) const;

    static QString escapeFilterValue(const QString &value);
};
