#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

class FFmpegCapabilities;

struct PipelineStep {
    // 1. Time slicing / Trim
    bool enableTrim = false;
    double startSec = 0.0;
    double endSec = 0.0; // 0.0 means until end of stream

    // 2. Crop
    bool enableCrop = false;
    int cropX = 0;
    int cropY = 0;
    int cropW = 0;
    int cropH = 0;

    // 3. Resize / Scale
    bool enableScale = false;
    int scaleWidth = 0;   // e.g. 1920 (or -2 for auto aspect ratio)
    int scaleHeight = 0;  // e.g. 1080 (or -2)

    // 4. Frame rate
    bool enableFps = false;
    int fps = 0;          // e.g. 24, 30, 60

    // 5. Watermark image overlay
    bool enableWatermark = false;
    QString watermarkPath;
    QString watermarkPosition = QStringLiteral("top-right"); // top-left, top-right, bottom-left, bottom-right, center
    int watermarkOpacity = 80; // 10% - 100%
    int watermarkMargin = 10;

    // 6. Audio processing
    bool enableAudio = false;
    double volumeFactor = 1.0;
    bool loudnorm = false;
    bool mute = false;

    // 7. Output encoding & container
    QString outputFormat = QStringLiteral("mp4");
    QString videoCodec = QStringLiteral("H.264");
    QString audioCodec = QStringLiteral("AAC");
    int crf = 23;
    int videoBitrateKbps = -1;
    QString audioBitrate = QStringLiteral("192k");
    QString speedPreset;
    bool hwAccel = false;
    bool preserveAllStreams = true;
};

class BatchPipeline
{
public:
    BatchPipeline() = default;
    explicit BatchPipeline(const QString &name, const PipelineStep &step = PipelineStep());

    QString name() const;
    void setName(const QString &name);

    PipelineStep settings() const;
    void setSettings(const PipelineStep &settings);

    // Generates linear video filter string (crop, scale, fps)
    QString buildVideoFilter() const;

    // Generates audio filter string (volume, loudnorm)
    QString buildAudioFilter() const;

    // Builds the complete FFmpeg argument list for an input-output pair
    QStringList buildCommand(const QString &inputPath, const QString &outputPath,
                             const FFmpegCapabilities *caps = nullptr) const;

    // Serialization
    QJsonObject toJson() const;
    static BatchPipeline fromJson(const QJsonObject &obj);

    QVariantMap toVariantMap() const;
    static BatchPipeline fromVariantMap(const QVariantMap &map);

private:
    QString m_name;
    PipelineStep m_step;
};
