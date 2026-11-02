#pragma once

#include <QString>
#include <QStringList>

namespace Compositor {

enum class LayoutMode {
    PictureInPicture = 0,
    SideBySide,
    TopBottom
};

enum class PipPosition {
    BottomRight = 0,
    BottomLeft,
    TopRight,
    TopLeft,
    Center,
    CustomCoordinate
};

enum class AudioMixPolicy {
    MixDual = 0,
    MainOnly,
    PipOnly,
    Ducking,
    MuteAll
};

struct CompositorConfig {
    LayoutMode layout = LayoutMode::PictureInPicture;
    PipPosition position = PipPosition::BottomRight;

    // PiP scaling & sizing
    double scalePercent = 0.25;  // 0.10 to 0.80 (25% default)
    int customWidth = 0;         // If > 0, overrides scalePercent
    int customHeight = 0;        // If > 0, overrides scalePercent

    // Position coordinates
    int margin = 16;             // Margin in pixels from edge
    int customX = 0;             // Used when position == CustomCoordinate
    int customY = 0;             // Used when position == CustomCoordinate

    // Border styling
    bool drawBorder = false;
    int borderWidth = 2;         // 1 to 10 px
    QString borderColor = QStringLiteral("white"); // e.g. "white", "black", "yellow", "#00FF00"

    // Time window (enable interval)
    bool hasTimeRange = false;
    double startTimeSec = 0.0;
    double durationSec = 0.0;    // 0 = through to end

    // Split-screen (SideBySide / TopBottom) sizing
    int splitWidth = 1920;       // Output canvas width
    int splitHeight = 1080;      // Output canvas height
    double splitRatio = 0.5;     // 0.5 = 50:50, 0.7 = 70:30, etc.

    // Audio mixing
    AudioMixPolicy audioPolicy = AudioMixPolicy::MixDual;
    double mainVolume = 1.0;     // 0.0 to 2.0 (1.0 = 100%)
    double pipVolume = 0.8;      // 0.0 to 2.0 (0.8 = 80%)

    // Encoding settings
    QString videoCodec = QStringLiteral("libx264");
    QString audioCodec = QStringLiteral("aac");
    int crf = 23;
    QString preset = QStringLiteral("medium");
    bool shortest = false;

    bool isModified() const;
    void reset();
};

class MultiTrackCompositor
{
public:
    // Builds the video -filter_complex expression
    static QString buildVideoFilter(const CompositorConfig &config);

    // Builds the audio -filter_complex expression
    static QString buildAudioFilter(const CompositorConfig &config);

    // Builds the full -filter_complex expression (combining video and audio if needed)
    static QString buildFilterComplex(const CompositorConfig &config);

    // Builds full ffmpeg command line arguments
    static QStringList buildCommand(
        const QString &mainInput,
        const QString &pipInput,
        const QString &outputPath,
        const CompositorConfig &config
    );

    // Helper: Position string expression for overlay filter
    static QString overlayPositionExpression(PipPosition pos, int margin, int customX = 0, int customY = 0);
};

} // namespace Compositor
