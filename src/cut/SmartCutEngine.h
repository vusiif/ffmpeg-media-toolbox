#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QPair>

enum class CutMode {
    KeyframeLossless = 0, // Keyframe stream copy (-c copy), instantaneous, 0 loss
    SmartCut = 1,         // Smart cut: decode & re-encode edge GOP frames only, stream copy middle GOPs
    AccurateReencode = 2  // Full accurate re-encode (-crf 18)
};

enum class ExportMode {
    MergeAll = 0,         // Concatenate all segments into one video file
    SeparateFiles = 1     // Export each segment as a standalone video file
};

struct CutSegment {
    double startSec = 0.0;
    double endSec = 0.0;
    QString label;

    double duration() const { return (endSec > startSec) ? (endSec - startSec) : 0.0; }
    bool isValid() const { return endSec > startSec; }
};

struct SmartChunk {
    enum class ChunkType {
        StreamCopy, // -c copy
        Reencode    // decode & re-encode
    };
    ChunkType type = ChunkType::StreamCopy;
    double startSec = 0.0;
    double endSec = 0.0;
    QString tempOutput;

    double duration() const { return (endSec > startSec) ? (endSec - startSec) : 0.0; }
};

struct SmartCutPlan {
    QList<SmartChunk> chunks;
    QString finalOutput;
    QString concatListFile;
    bool needsConcat = false;
    bool isValid = false;
};

class SmartCutEngine : public QObject
{
    Q_OBJECT

public:
    explicit SmartCutEngine(QObject *parent = nullptr);

    // Keyframe probe & analysis
    static QStringList buildKeyframeProbeCommand(const QString &ffprobePath, const QString &inputPath);
    static QVector<double> parseKeyframeOutput(const QString &output);

    // Keyframe navigation & snapping helpers
    static double findPrevKeyframe(double targetSec, const QVector<double> &keyframes);
    static double findNextKeyframe(double targetSec, const QVector<double> &keyframes);
    static double findNearestKeyframe(double targetSec, const QVector<double> &keyframes);
    static bool isKeyframeAligned(double targetSec, const QVector<double> &keyframes, double tolerance = 0.08);

    // Single segment cutting command builders
    static QStringList buildLosslessCut(const QString &inputPath, const QString &outputPath,
                                        double startSec, double endSec,
                                        const QString &outputFormat = QString());

    static QStringList buildAccurateReencode(const QString &inputPath, const QString &outputPath,
                                             double startSec, double endSec,
                                             const QString &videoCodec = QString(),
                                             int crf = 18,
                                             const QString &outputFormat = QString());

    // Smart Cut planning & chunk command builders
    static SmartCutPlan planSmartCut(const QString &inputPath, const QString &outputPath,
                                     double startSec, double endSec,
                                     const QVector<double> &keyframes,
                                     const QString &tempDir = QString());

    static QStringList buildChunkCommand(const QString &inputPath, const SmartChunk &chunk,
                                         const QString &videoCodec = QString(),
                                         int crf = 18);

    // Concat commands
    static QStringList buildConcatDemuxer(const QString &concatListPath, const QString &outputPath);
    static QString generateConcatFileList(const QStringList &filePaths);
    static QStringList buildFilterConcat(const QStringList &inputPaths, const QString &outputPath);

    // Time formatting helpers
    static QString formatTime(double seconds);
    static QString formatDuration(double seconds);
};
