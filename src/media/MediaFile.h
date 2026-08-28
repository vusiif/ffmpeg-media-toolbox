#pragma once

#include <QString>
#include <QVariantMap>
#include <QList>

enum class MediaType {
    Unknown,
    Video,
    Audio,
    Image,
    Subtitle
};

struct VideoStream {
    int index = -1;
    QString codec;
    int width = 0;
    int height = 0;
    double frameRate = 0;
    QString pixelFormat;
    qint64 bitrate = 0;
    QVariantMap metadata;
};

struct AudioStream {
    int index = -1;
    QString codec;
    int sampleRate = 0;
    int channels = 0;
    QString channelLayout;
    qint64 bitrate = 0;
    QVariantMap metadata;
};

struct SubtitleStream {
    int index = -1;
    QString codec;
    QString language;
    QVariantMap metadata;
};

struct MediaFile {
    QString path;
    MediaType type = MediaType::Unknown;
    QString formatName;
    qint64 fileSize = 0;
    double duration = 0;
    qint64 bitrate = 0;

    QList<VideoStream> videoStreams;
    QList<AudioStream> audioStreams;
    QList<SubtitleStream> subtitleStreams;

    QVariantMap formatMetadata;

    bool isValid() const { return !path.isEmpty(); }
    bool hasVideo() const { return !videoStreams.isEmpty(); }
    bool hasAudio() const { return !audioStreams.isEmpty(); }

    const VideoStream& primaryVideo() const {
        static const VideoStream empty;
        return videoStreams.isEmpty() ? empty : videoStreams.first();
    }

    const AudioStream& primaryAudio() const {
        static const AudioStream empty;
        return audioStreams.isEmpty() ? empty : audioStreams.first();
    }
};
