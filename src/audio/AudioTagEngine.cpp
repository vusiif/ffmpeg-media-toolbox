#include "AudioTagEngine.h"

#include <QFileInfo>
#include <QRegularExpression>

namespace Audio {

bool AudioMetadata::isEmpty() const
{
    return title.isEmpty() && artist.isEmpty() && album.isEmpty() &&
           albumArtist.isEmpty() && composer.isEmpty() && genre.isEmpty() &&
           year.isEmpty() && track.isEmpty() && disc.isEmpty() &&
           comment.isEmpty() && coverImagePath.isEmpty();
}

void AudioMetadata::clear()
{
    *this = AudioMetadata();
}

AudioMetadata AudioMetadata::parseTags(const QVariantMap &tags)
{
    AudioMetadata meta;
    for (auto it = tags.begin(); it != tags.end(); ++it) {
        const QString key = it.key().toLower();
        const QString val = it.value().toString().trimmed();
        if (val.isEmpty()) continue;

        if (key == QLatin1String("title")) {
            meta.title = val;
        } else if (key == QLatin1String("artist")) {
            meta.artist = val;
        } else if (key == QLatin1String("album")) {
            meta.album = val;
        } else if (key == QLatin1String("album_artist") || key == QLatin1String("albumartist")) {
            meta.albumArtist = val;
        } else if (key == QLatin1String("composer")) {
            meta.composer = val;
        } else if (key == QLatin1String("genre")) {
            meta.genre = val;
        } else if (key == QLatin1String("date") || key == QLatin1String("year")) {
            meta.year = val;
        } else if (key == QLatin1String("track")) {
            meta.track = val;
        } else if (key == QLatin1String("disc")) {
            meta.disc = val;
        } else if (key == QLatin1String("comment") || key == QLatin1String("description")) {
            meta.comment = val;
        }
    }
    return meta;
}

AudioMetadata AudioMetadata::guessFromFileName(const QString &fileName)
{
    AudioMetadata meta;
    QString base = QFileInfo(fileName).completeBaseName().trimmed();

    // Remove leading track numbers e.g. "01. " or "01 - " or "1- "
    static const QRegularExpression trackPrefixRx(QStringLiteral(R"(^\d{1,3}[\s._-]+)"));
    const auto match = trackPrefixRx.match(base);
    if (match.hasMatch()) {
        base = base.mid(match.capturedLength()).trimmed();
    }

    // Check if filename has "Artist - Title" pattern
    const int dashIdx = base.indexOf(QStringLiteral(" - "));
    if (dashIdx > 0) {
        meta.artist = base.left(dashIdx).trimmed();
        meta.title = base.mid(dashIdx + 3).trimmed();
    } else {
        meta.title = base;
    }

    return meta;
}

void AudioExtractOptions::applyPreset(AudioQualityPreset p)
{
    preset = p;
    switch (p) {
    case AudioQualityPreset::HighMp3_320k:
        format = AudioFormat::MP3;
        bitrateKbps = 320;
        sampleRate = -1;
        channels = -1;
        break;
    case AudioQualityPreset::BalancedMp3_192k:
        format = AudioFormat::MP3;
        bitrateKbps = 192;
        sampleRate = -1;
        channels = -1;
        break;
    case AudioQualityPreset::LosslessFlac:
        format = AudioFormat::FLAC;
        bitrateKbps = 0;
        sampleRate = -1;
        channels = -1;
        break;
    case AudioQualityPreset::LosslessWav:
        format = AudioFormat::WAV;
        bitrateKbps = 0;
        sampleRate = -1;
        channels = -1;
        break;
    case AudioQualityPreset::BroadcastAac_256k:
        format = AudioFormat::M4A;
        bitrateKbps = 256;
        sampleRate = 48000;
        channels = 2;
        break;
    case AudioQualityPreset::VoiceOpus_64k:
        format = AudioFormat::OPUS;
        bitrateKbps = 64;
        sampleRate = 48000;
        channels = 1;
        break;
    case AudioQualityPreset::Custom:
        break;
    }
}

QString AudioTagEngine::formatToString(AudioFormat fmt)
{
    switch (fmt) {
    case AudioFormat::FLAC: return QStringLiteral("flac");
    case AudioFormat::AAC:  return QStringLiteral("aac");
    case AudioFormat::WAV:  return QStringLiteral("wav");
    case AudioFormat::OPUS: return QStringLiteral("opus");
    case AudioFormat::OGG:  return QStringLiteral("ogg");
    case AudioFormat::M4A:  return QStringLiteral("m4a");
    case AudioFormat::MP3:
    default:
        return QStringLiteral("mp3");
    }
}

AudioFormat AudioTagEngine::formatFromString(const QString &str)
{
    const QString s = str.toLower().trimmed();
    if (s == QLatin1String("flac")) return AudioFormat::FLAC;
    if (s == QLatin1String("aac"))  return AudioFormat::AAC;
    if (s == QLatin1String("wav"))  return AudioFormat::WAV;
    if (s == QLatin1String("opus")) return AudioFormat::OPUS;
    if (s == QLatin1String("ogg"))  return AudioFormat::OGG;
    if (s == QLatin1String("m4a"))  return AudioFormat::M4A;
    return AudioFormat::MP3;
}

QString AudioTagEngine::codecForFormat(AudioFormat fmt)
{
    switch (fmt) {
    case AudioFormat::FLAC: return QStringLiteral("flac");
    case AudioFormat::AAC:  return QStringLiteral("aac");
    case AudioFormat::WAV:  return QStringLiteral("pcm_s16le");
    case AudioFormat::OPUS: return QStringLiteral("libopus");
    case AudioFormat::OGG:  return QStringLiteral("libvorbis");
    case AudioFormat::M4A:  return QStringLiteral("aac");
    case AudioFormat::MP3:
    default:
        return QStringLiteral("libmp3lame");
    }
}

QStringList AudioTagEngine::buildMetadataArgs(const AudioMetadata &meta, AudioFormat fmt)
{
    QStringList args;

    auto addTag = [&args](const QString &tag, const QString &val) {
        if (!val.isEmpty()) {
            args << QStringLiteral("-metadata") << QStringLiteral("%1=%2").arg(tag, val);
        }
    };

    addTag(QStringLiteral("title"), meta.title);
    addTag(QStringLiteral("artist"), meta.artist);
    addTag(QStringLiteral("album"), meta.album);
    addTag(QStringLiteral("album_artist"), meta.albumArtist);
    addTag(QStringLiteral("composer"), meta.composer);
    addTag(QStringLiteral("genre"), meta.genre);
    addTag(QStringLiteral("date"), meta.year);
    addTag(QStringLiteral("track"), meta.track);
    addTag(QStringLiteral("disc"), meta.disc);
    addTag(QStringLiteral("comment"), meta.comment);

    if (meta.hasCoverImage()) {
        if (fmt == AudioFormat::MP3) {
            args << QStringLiteral("-id3v2_version") << QStringLiteral("3");
            args << QStringLiteral("-metadata:s:v") << QStringLiteral("title=Album cover");
            args << QStringLiteral("-metadata:s:v") << QStringLiteral("comment=Cover (front)");
        } else {
            args << QStringLiteral("-disposition:v:0") << QStringLiteral("attached_pic");
        }
    }

    return args;
}

QStringList AudioTagEngine::buildExtractCommand(
    const QString &inputPath,
    const QString &outputPath,
    const AudioExtractOptions &options)
{
    if (inputPath.isEmpty() || outputPath.isEmpty()) {
        return {};
    }

    QStringList args;
    args << QStringLiteral("-y");
    args << QStringLiteral("-i") << inputPath;

    const bool hasCover = options.metadata.hasCoverImage();
    if (hasCover) {
        args << QStringLiteral("-i") << options.metadata.coverImagePath;
        args << QStringLiteral("-map") << QStringLiteral("0:a:0?");
        args << QStringLiteral("-map") << QStringLiteral("1:0");
    } else {
        args << QStringLiteral("-map") << QStringLiteral("0:a:0?");
    }

    // Audio filter chain
    QStringList filters;
    if (options.normalizeLoudness) {
        filters << QStringLiteral("loudnorm=I=-16:TP=-1.5:LRA=11");
    }
    if (qAbs(options.volumeGain - 1.0) > 0.01) {
        filters << QStringLiteral("volume=%1").arg(options.volumeGain, 0, 'f', 2);
    }
    if (!filters.isEmpty()) {
        args << QStringLiteral("-af") << filters.join(QStringLiteral(","));
    }

    // Audio encoding parameters
    const QString codec = codecForFormat(options.format);
    args << QStringLiteral("-c:a") << codec;

    if (options.bitrateKbps > 0 && options.format != AudioFormat::FLAC && options.format != AudioFormat::WAV) {
        args << QStringLiteral("-b:a") << QStringLiteral("%1k").arg(options.bitrateKbps);
    }
    if (options.sampleRate > 0) {
        args << QStringLiteral("-ar") << QString::number(options.sampleRate);
    }
    if (options.channels > 0) {
        args << QStringLiteral("-ac") << QString::number(options.channels);
    }

    if (hasCover) {
        args << QStringLiteral("-c:v") << QStringLiteral("copy");
    }

    // Metadata injection
    args.append(buildMetadataArgs(options.metadata, options.format));

    args << outputPath;
    return args;
}

QStringList AudioTagEngine::buildCoverExtractCommand(
    const QString &mediaPath,
    const QString &outputImagePath)
{
    if (mediaPath.isEmpty() || outputImagePath.isEmpty()) {
        return {};
    }

    QStringList args;
    args << QStringLiteral("-y");
    args << QStringLiteral("-i") << mediaPath;
    args << QStringLiteral("-an");
    args << QStringLiteral("-vcodec") << QStringLiteral("copy");
    args << outputImagePath;
    return args;
}

} // namespace Audio
