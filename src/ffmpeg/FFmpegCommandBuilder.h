#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QtQmlIntegration/qqmlintegration.h>

class FFmpegCommandBuilder : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit FFmpegCommandBuilder(QObject *parent = nullptr);

    Q_INVOKABLE QStringList buildConversion(
        const QString &inputPath,
        const QString &outputPath,
        const QString &outputFormat = {},
        const QString &videoCodec = {},
        const QString &audioCodec = {},
        int crf = -1,
        bool copyStreams = false
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

    static QString escapeFilterValue(const QString &value);
};
