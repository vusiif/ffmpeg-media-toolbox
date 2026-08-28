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

private:
    MediaFile parseProbeJson(const QString &json, const QString &filePath) const;
    MediaType detectMediaType(const QString &formatName, const QList<VideoStream> &videos,
                              const QList<AudioStream> &audios) const;

    const FFmpegLocator *m_locator;
};
