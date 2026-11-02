#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

enum class ColorPreset {
    Custom = 0,
    Default,
    CinematicTealOrange,
    WarmVintage,
    CoolNight,
    BwHighContrast,
    LowlightBoost,
    VividPop
};

struct ColorGradingOptions {
    ColorPreset preset = ColorPreset::Default;

    // 3D LUT (.cube / .3dl)
    QString lutFilePath;

    // Basic Adjustments (eq)
    double brightness = 0.0;    // -1.0 to 1.0 (default 0.0)
    double contrast = 1.0;      // 0.0 to 2.0 (default 1.0)
    double saturation = 1.0;    // 0.0 to 3.0 (default 1.0)
    double gamma = 1.0;         // 0.2 to 3.0 (default 1.0)

    // Color Temperature & Tint (colorbalance)
    double temperature = 0.0;   // -1.0 (cool/blue) to 1.0 (warm/amber), default 0.0
    double tint = 0.0;          // -1.0 (green) to 1.0 (magenta), default 0.0

    // Enhancement Filters
    bool enableDenoise = false;
    double denoiseStrength = 4.0; // 1.0 to 10.0 (maps to hqdn3d)

    bool enableSharpen = false;
    double sharpenAmount = 0.8;   // 0.1 to 2.5 (maps to unsharp)

    bool enableVignette = false;
    double vignetteStrength = 0.5; // 0.1 to 1.0

    bool enableDeinterlace = false; // yadif

    bool isModified() const;
    void reset();
    void applyPreset(ColorPreset p);
};

enum class LoudnessStandard {
    Custom = 0,
    YouTube,      // -14 LUFS
    ApplePodcast, // -16 LUFS
    EbuR128,      // -23 LUFS
    AtscA85       // -24 LUFS
};

struct AudioFxOptions {
    // Loudness Normalization (EBU R128 / loudnorm)
    bool enableLoudnorm = false;
    LoudnessStandard loudnessStandard = LoudnessStandard::YouTube;
    double targetLufs = -14.0; // -24 to -10

    // Background Denoise (afftdn)
    bool enableDenoise = false;
    double noiseReductionDb = 12.0; // 6 to 25 dB

    // Equalizer / Filters
    bool enableHighpass = false;
    int highpassFreq = 80; // 40 - 200 Hz

    bool enableLowpass = false;
    int lowpassFreq = 12000; // 8000 - 18000 Hz

    // Volume Adjustment
    double volumeDb = 0.0; // -24 to +24 dB

    bool isModified() const;
    void reset();
    void applyStandard(LoudnessStandard s);
};

class MediaFilterStudio : public QObject
{
    Q_OBJECT

public:
    explicit MediaFilterStudio(QObject *parent = nullptr);

    // Video filter compiler
    static QString compileVideoFilters(const ColorGradingOptions &opts);

    // Audio filter compiler
    static QString compileAudioFilters(const AudioFxOptions &opts);

    // Command builder for quick sample preview (e.g. 5s sample with filters applied)
    static QStringList buildSamplePreviewCommand(const QString &inputPath, const QString &outputPath,
                                                 const ColorGradingOptions &videoOpts,
                                                 const AudioFxOptions &audioOpts,
                                                 double startSec = 0.0, double durationSec = 5.0);

    // Command builder for full export with filters
    static QStringList buildFilterExportCommand(const QString &inputPath, const QString &outputPath,
                                                const ColorGradingOptions &videoOpts,
                                                const AudioFxOptions &audioOpts,
                                                const QString &videoCodec = QString(),
                                                const QString &audioCodec = QString(),
                                                int crf = 18);

    // Preset helper names
    static QString presetName(ColorPreset preset);
    static QString standardName(LoudnessStandard standard);
};
