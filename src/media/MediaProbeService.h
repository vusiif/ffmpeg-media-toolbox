#pragma once

#include <QObject>
#include <QString>
#include "MediaFile.h"

class FFmpegLocator;

class MediaProbeService : public QObject
{
    Q_OBJECT

public:
    explicit MediaProbeService(const FFmpegLocator *locator, QObject *parent = nullptr);

    MediaFile probe(const QString &filePath) const;

    // Pure parsing helpers, exposed for unit tests.
    static MediaFile parseProbeJson(const QString &json, const QString &filePath);
    static MediaType detectMediaType(const QString &formatName, const QList<VideoStream> &videos,
                                     const QList<AudioStream> &audios);

private:
    const FFmpegLocator *m_locator;
};
