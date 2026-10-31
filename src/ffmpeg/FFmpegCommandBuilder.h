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

    struct ConversionOptions
    {
        QString inputPath;
        QString outputPath;
        QString videoCodec;
        QString audioCodec;
        int crf = -1;
        bool copyStreams = false;
        bool stripMetadata = false;

        // Advanced Audio Settings
        int audioBitrateKbps = -1;   // e.g. 192 -> -b:a 192k
        int audioSampleRate = -1;    // e.g. 48000 -> -ar 48000
        int audioChannels = -1;      // e.g. 2 -> -ac 2
        bool audioNormalize = false; // -> -af loudnorm

        // Advanced Video & Codec Settings
        QString preset;              // -preset <preset>
        QString pixelFormat;         // -pix_fmt <format>
        QString videoFilter;         // -vf <filters>
        QString audioFilter;         // -af <filters>

        // Color Space & HDR
        QString colorSpace;          // -colorspace <cs>
        QString colorPrimaries;      // -color_primaries <cp>
        QString colorTrc;            // -color_trc <trc>
        QString hdrTonemap;          // e.g. "hable", "mobius", "reinhard" -> tonemap filter

        // Hardware Acceleration
        QString hwAccel;             // -hwaccel <auto|cuda|qsv|d3d11va|vaapi>

        // Target File Size Compression
        double targetSizeMB = -1.0;  // Target output size in Megabytes (e.g. 25.0, 50.0)
        double durationSeconds = 0.0;// Duration in seconds (required for target size calculation)
        int videoBitrateKbps = -1;   // Explicit video bitrate (or calculated from targetSizeMB)
    };

    struct CropArea
    {
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
        bool isValid() const { return w > 0 && h > 0; }
    };

    QStringList buildConversion(const ConversionOptions &options) const;

    // Detects whether hardware acceleration/encoders are present in command arguments,
    // and produces a sanitized software-encoding fallback argument list if requested.
    static bool canFallbackToSoftware(const QStringList &arguments, QStringList *fallbackArgs = nullptr);

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

    // Crop/scale/rotate/flip chain shared by image and video adjustments;
    // empty when nothing was requested.
    static QStringList adjustmentFilters(int cropX, int cropY, int cropW, int cropH,
                                         int resizeW, int resizeH, int rotate,
                                         bool flipH = false, bool flipV = false);

    // Re-encoded video adjustment (filters force a re-encode): runs the
    // shared adjustment chain with -crf. Returns empty when there is no
    // actual operation to perform.
    QStringList buildVideoAdjust(
        const QString &inputPath,
        const QString &outputPath,
        int cropX, int cropY, int cropW, int cropH,
        int resizeW, int resizeH,
        int rotate,
        int crf
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

    struct AudioProcessOptions
    {
        QString inputPath;
        QString outputPath;
        QString format;           // mp3, aac, flac, wav, opus, ogg, m4a
        QString audioCodec;       // libmp3lame, aac, flac, pcm_s16le, libopus...
        int bitrateKbps = -1;     // e.g. 128, 192, 256, 320
        int sampleRate = -1;      // e.g. 44100, 48000
        int channels = -1;        // e.g. 1, 2

        // Denoise & filters
        bool denoise = false;
        double denoiseNoiseReduction = 12.0; // dB (6.0 - 25.0)
        int highpassFreq = -1;               // Hz (e.g. 80)
        int lowpassFreq = -1;                // Hz (e.g. 12000)

        // Loudness & Volume & Silence
        bool loudnorm = false;               // EBU R128 (-16 LUFS)
        double volumeFactor = 1.0;           // Multiplier (e.g. 1.5)
        bool removeSilence = false;          // silenceremove

        // Tempo & Effects
        double tempo = 1.0;                  // 0.5 - 2.0
        bool reverse = false;                // areverse
    };

    QStringList buildAudioProcess(const AudioProcessOptions &options) const;

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

    // Stacks images into a grid with `cols` images per row (rows grow as
    // needed); spacing > 0 pads every input with the background color first,
    // padding > 0 adds an outer margin around the finished stack.
    QStringList buildGridJoin(
        const QStringList &inputPaths,
        const QString &outputPath,
        int cols,
        int spacing,
        const QString &backgroundColor,
        int padding = 0
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

    // Muxes an external subtitle file into a copy of the input: audio/video
    // streams are copied, the new subtitle stream takes `subtitleCodec`
    // (empty = plain copy for containers that accept it as-is).
    QStringList buildSubtitleMux(
        const QString &inputPath,
        const QString &subtitlePath,
        const QString &outputPath,
        const QString &subtitleCodec
    ) const;

    // Burns the subtitle into the picture (hard subtitle): video is
    // re-encoded through the subtitles filter, audio is copied. crf < 0
    // keeps the encoder default.
    QStringList buildSubtitleBurnin(
        const QString &inputPath,
        const QString &subtitlePath,
        const QString &outputPath,
        int crf
    ) const;

    QStringList buildWatermark(
        const QString &inputPath,
        const QString &watermarkPath,
        const QString &outputPath,
        const QString &position,
        int opacityPercent,
        const QString &outputFormat = {}
    ) const;

    // Overlays an inset video on top of the main video (picture-in-picture):
    // inset is scaled to pipWidth x pipHeight (<= 0 keeps aspect ratio, both
    // <= 0 defaults to iw/3:-1), placed at `position` with `margin` px padding.
    // Audio from the main video is preserved (-map 0:a?).
    QStringList buildPip(
        const QString &mainPath,
        const QString &pipPath,
        const QString &outputPath,
        int pipWidth = 0,
        int pipHeight = 0,
        const QString &position = QStringLiteral("bottom-right"),
        int margin = 10,
        int crf = -1
    ) const;

    static QString escapeFilterValue(const QString &value);
    // Quotes a filesystem path for use as a filter option value: forward
    // slashes, escaped colon, single-quoted (the form the subtitles filter
    // expects on Windows).
    static QString escapeFilterPath(const QString &path);
    static QString formatTime(double seconds);
    static QString normalizeMuxerFormat(const QString &format);

    // Auto Crop Detection
    QStringList buildCropdetect(const QString &inputPath, double seekSec = 10.0, int frames = 20) const;
    static CropArea parseCropdetectOutput(const QString &ffmpegOutput);
};
