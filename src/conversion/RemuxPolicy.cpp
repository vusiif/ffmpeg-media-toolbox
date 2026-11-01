#include "RemuxPolicy.h"

#include "media/MediaFile.h"

#include <QFileInfo>
#include <QSet>

namespace {

const QSet<QString> &videoCodecsFor(const QString &format)
{
    static const QSet<QString> empty;
    static const QSet<QString> mp4 = {
        QStringLiteral("h264"), QStringLiteral("hevc"), QStringLiteral("mpeg4"),
        QStringLiteral("av1"), QStringLiteral("vp9")};
    static const QSet<QString> mkv = {
        QStringLiteral("h264"), QStringLiteral("hevc"), QStringLiteral("mpeg4"),
        QStringLiteral("mpeg1video"), QStringLiteral("mpeg2video"),
        QStringLiteral("vp8"), QStringLiteral("vp9"), QStringLiteral("av1"),
        QStringLiteral("theora")};
    static const QSet<QString> webm = {
        QStringLiteral("vp8"), QStringLiteral("vp9"), QStringLiteral("av1")};
    static const QSet<QString> avi = {
        QStringLiteral("mpeg4"), QStringLiteral("mjpeg")};

    const QString f = format.toLower();
    if (f == QLatin1String("mp4") || f == QLatin1String("mov")) return mp4;
    if (f == QLatin1String("mkv")) return mkv;
    if (f == QLatin1String("webm")) return webm;
    if (f == QLatin1String("avi")) return avi;
    return empty;
}

const QSet<QString> &audioCodecsFor(const QString &format)
{
    static const QSet<QString> empty;
    static const QSet<QString> mp4 = {
        QStringLiteral("aac"), QStringLiteral("mp3"), QStringLiteral("ac3"),
        QStringLiteral("eac3"), QStringLiteral("alac"), QStringLiteral("opus"),
        QStringLiteral("flac")};
    static const QSet<QString> mkv = {
        QStringLiteral("aac"), QStringLiteral("mp3"), QStringLiteral("ac3"),
        QStringLiteral("eac3"), QStringLiteral("dts"), QStringLiteral("truehd"),
        QStringLiteral("flac"), QStringLiteral("opus"), QStringLiteral("vorbis"),
        QStringLiteral("alac"), QStringLiteral("pcm_s16le"), QStringLiteral("pcm_s24le"),
        QStringLiteral("pcm_s32le")};
    static const QSet<QString> webm = {
        QStringLiteral("opus"), QStringLiteral("vorbis")};
    static const QSet<QString> avi = {
        QStringLiteral("mp3"), QStringLiteral("ac3"), QStringLiteral("pcm_s16le")};
    static const QSet<QString> mp3 = {QStringLiteral("mp3")};
    static const QSet<QString> aac = {QStringLiteral("aac")};
    static const QSet<QString> flac = {QStringLiteral("flac")};
    static const QSet<QString> wav = {
        QStringLiteral("pcm_s16le"), QStringLiteral("pcm_s24le"),
        QStringLiteral("pcm_s32le"), QStringLiteral("pcm_f32le")};

    const QString f = format.toLower();
    if (f == QLatin1String("mp4") || f == QLatin1String("mov")) return mp4;
    if (f == QLatin1String("mkv")) return mkv;
    if (f == QLatin1String("webm")) return webm;
    if (f == QLatin1String("avi")) return avi;
    if (f == QLatin1String("mp3")) return mp3;
    if (f == QLatin1String("aac")) return aac;
    if (f == QLatin1String("flac")) return flac;
    if (f == QLatin1String("wav")) return wav;
    return empty;
}

const QSet<QString> &subtitleCodecsFor(const QString &format)
{
    static const QSet<QString> empty;
    static const QSet<QString> mp4 = {
        QStringLiteral("mov_text")};
    static const QSet<QString> mkv = {
        QStringLiteral("subrip"), QStringLiteral("srt"), QStringLiteral("ass"),
        QStringLiteral("ssa"), QStringLiteral("mov_text"), QStringLiteral("webvtt"),
        QStringLiteral("hdmv_pgs_subtitle"), QStringLiteral("dvd_subtitle"),
        QStringLiteral("text"), QStringLiteral("xsub")};
    static const QSet<QString> webm = {
        QStringLiteral("webvtt")};

    const QString f = format.toLower();
    if (f == QLatin1String("mp4") || f == QLatin1String("mov")) return mp4;
    if (f == QLatin1String("mkv")) return mkv;
    if (f == QLatin1String("webm")) return webm;
    return empty;
}

} // namespace

namespace RemuxPolicy {

bool isAudioContainer(const QString &format)
{
    const QString f = format.toLower();
    return f == QLatin1String("mp3") || f == QLatin1String("aac") ||
           f == QLatin1String("flac") || f == QLatin1String("wav") ||
           f == QLatin1String("m4a") || f == QLatin1String("ogg") ||
           f == QLatin1String("opus") || f == QLatin1String("wma");
}

bool containerSupportsVideoCodec(const QString &format, const QString &codec)
{
    if (codec.isEmpty()) {
        return false;
    }
    return videoCodecsFor(format).contains(codec.toLower());
}

bool containerSupportsAudioCodec(const QString &format, const QString &codec)
{
    if (codec.isEmpty()) {
        return false;
    }
    return audioCodecsFor(format).contains(codec.toLower());
}

bool containerSupportsSubtitleCodec(const QString &format, const QString &codec)
{
    if (codec.isEmpty()) {
        return false;
    }
    return subtitleCodecsFor(format).contains(codec.toLower());
}

bool canCopyAllStreams(const MediaFile &input, const QString &outputFormat)
{
    if (!input.isValid() || (!input.hasVideo() && !input.hasAudio())) {
        return false;
    }

    for (const auto &video : input.videoStreams) {
        if (!containerSupportsVideoCodec(outputFormat, video.codec)) {
            return false;
        }
    }
    for (const auto &audio : input.audioStreams) {
        if (!containerSupportsAudioCodec(outputFormat, audio.codec)) {
            return false;
        }
    }
    for (const auto &sub : input.subtitleStreams) {
        if (!containerSupportsSubtitleCodec(outputFormat, sub.codec)) {
            return false;
        }
    }
    return true;
}

bool canCopyByExtension(const QString &inputPath, const QString &outputFormat)
{
    const QString suffix = QFileInfo(inputPath).suffix().toLower();
    if (suffix.isEmpty() || outputFormat.isEmpty()) {
        return false;
    }
    return suffix == outputFormat.toLower();
}

std::optional<QString> subtitleCodecForContainer(const QString &format)
{
    const QString f = format.toLower();
    if (f == QLatin1String("mp4") || f == QLatin1String("mov")) {
        return QStringLiteral("mov_text");
    }
    if (f == QLatin1String("mkv")) {
        return QString(); // Matroska accepts subrip/ass as-is.
    }
    if (f == QLatin1String("webm")) {
        return QStringLiteral("webvtt");
    }
    return std::nullopt;
}

} // namespace RemuxPolicy
