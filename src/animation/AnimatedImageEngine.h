#pragma once

#include <QString>
#include <QStringList>

namespace Animation {

enum class OutputFormat {
    GIF = 0,
    WebP,
    APNG
};

enum class DitherMode {
    Bayer = 0,
    FloydSteinberg,
    Sierra2_4a,
    None
};

enum class PaletteStatsMode {
    Diff = 0,
    Full
};

enum class PlaybackDirection {
    Normal = 0,
    Reverse,
    PingPong
};

struct AnimatedImageConfig {
    OutputFormat format = OutputFormat::GIF;

    // Time window
    double startSec = 0.0;
    double durationSec = 3.0;  // Default 3 seconds clip

    // Frame rate & Speed
    int fps = 15;              // 5 to 50 fps (15 default)
    double speedMultiplier = 1.0; // 0.5x to 3.0x
    PlaybackDirection direction = PlaybackDirection::Normal;

    // Geometry
    int width = 480;           // Default 480px width (-1 for original)
    int height = -1;           // -1 for auto aspect ratio

    // Color & Palette (GIF / 8-bit paletted)
    int maxColors = 256;       // 32 to 256
    PaletteStatsMode statsMode = PaletteStatsMode::Diff;
    DitherMode dither = DitherMode::Bayer;
    int bayerScale = 3;        // 1 to 5
    bool diffModeRectangle = true;

    // WebP specific
    int webpQuality = 75;      // 0 to 100
    bool webpLossless = false;

    // Loop count
    int loopCount = 0;         // 0 = infinite, -1 = play once, >0 = count

    // Size estimate
    qint64 estimateFileSizeBytes() const;
    int totalFrames() const;

    bool isModified() const;
    void reset();
};

class AnimatedImageEngine
{
public:
    static QString buildFilterGraph(const AnimatedImageConfig &config);

    static QStringList buildCommand(
        const QString &inputPath,
        const QString &outputPath,
        const AnimatedImageConfig &config
    );

    static QString ditherModeString(DitherMode mode, int bayerScale);
    static QString formatTime(double seconds);
};

} // namespace Animation
