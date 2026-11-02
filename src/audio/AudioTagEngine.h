#pragma once

#include <QString>
#include <QStringList>
#include <QVariantMap>

struct MediaFile;

namespace Audio {

enum class AudioFormat {
    MP3 = 0,
    FLAC,
    AAC,
    WAV,
    OPUS,
    OGG,
    M4A
};

enum class AudioQualityPreset {
    HighMp3_320k = 0,
    BalancedMp3_192k,
    LosslessFlac,
    LosslessWav,
    BroadcastAac_256k,
    VoiceOpus_64k,
    Custom
};

struct AudioMetadata {
    QString title;
    QString artist;
    QString album;
    QString albumArtist;
    QString composer;
    QString genre;
    QString year;
    QString track;
    QString disc;
    QString comment;

    // Optional embedded album cover image path (JPG / PNG)
    QString coverImagePath;

    bool hasCoverImage() const { return !coverImagePath.isEmpty(); }
    bool isEmpty() const;
    void clear();

    static AudioMetadata parseTags(const QVariantMap &tags);
    static AudioMetadata guessFromFileName(const QString &fileName);
};

struct AudioExtractOptions {
    AudioFormat format = AudioFormat::MP3;
    int bitrateKbps = 320;
    int sampleRate = -1;  // -1 = auto
    int channels = -1;    // -1 = auto
    AudioQualityPreset preset = AudioQualityPreset::HighMp3_320k;

    AudioMetadata metadata;

    double volumeGain = 1.0;
    bool normalizeLoudness = false;

    void applyPreset(AudioQualityPreset p);
};

class AudioTagEngine
{
public:
    static QString formatToString(AudioFormat fmt);
    static AudioFormat formatFromString(const QString &str);
    static QString codecForFormat(AudioFormat fmt);

    // Generates metadata injection arguments (-metadata key=value ...)
    static QStringList buildMetadataArgs(const AudioMetadata &meta, AudioFormat fmt);

    // Generates complete extraction / transcoding command
    static QStringList buildExtractCommand(
        const QString &inputPath,
        const QString &outputPath,
        const AudioExtractOptions &options
    );

    // Generates command to extract embedded cover art image to disk
    static QStringList buildCoverExtractCommand(
        const QString &mediaPath,
        const QString &outputImagePath
    );
};

} // namespace Audio
