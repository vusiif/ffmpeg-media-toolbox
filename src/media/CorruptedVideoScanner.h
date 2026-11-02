#pragma once

#include <QColor>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QVector>

enum class QCIssueType
{
    DecodeError, // Frame decode corruption
    Silence,     // Audio silence interval
    BlackFrame,  // Video black frame interval
    FreezeFrame  // Video frozen / static frame interval
};

struct CorruptedSegment
{
    QCIssueType type = QCIssueType::DecodeError;
    double start = 0.0;    // start timestamp in seconds
    double end = 0.0;      // end timestamp in seconds
    QString errorMsg;      // summary description of error

    QString typeName() const;
    QColor color() const;
};

struct QCOptions
{
    bool checkDecodeErrors = true;
    bool checkSilence = true;
    double silenceNoiseDb = -50.0;
    double silenceMinDuration = 1.0;

    bool checkBlackFrames = true;
    double blackPicTh = 0.98;
    double blackPixTh = 0.10;
    double blackMinDuration = 1.0;

    bool checkFreezeFrames = true;
    double freezeNoiseDb = -60.0;
    double freezeMinDuration = 2.0;

    bool hasVideo = true;
    bool hasAudio = true;
};

class FFmpegLocator;

class CorruptedVideoScanner : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)

public:
    explicit CorruptedVideoScanner(const FFmpegLocator *locator, QObject *parent = nullptr);
    ~CorruptedVideoScanner() override;

    bool startScan(const QString &filePath, double totalDuration = 0.0);
    bool startScan(const QString &filePath, double totalDuration, const QCOptions &options);
    void cancel();

    bool isRunning() const;
    double progress() const;
    QVector<CorruptedSegment> segments() const;
    double totalDuration() const;
    QCOptions options() const;

    // Helper to merge a list of error points into grouped segments
    static QVector<CorruptedSegment> mergeErrorTimestamps(
        const QVector<QPair<double, QString>> &rawErrors,
        double mergeThreshold = 0.5,
        double defaultFrameDuration = 0.08
    );

    // Extracts timestamp from an FFmpeg error log line, or returns -1.0 if none found
    static double extractTimestampFromLine(const QString &line);

    // QC Log Parsing helpers
    static bool parseSilenceDetect(const QString &line, double &start, double &end, double &duration, bool &isStart);
    static bool parseBlackDetect(const QString &line, double &start, double &end, double &duration);
    static bool parseFreezeDetect(const QString &line, double &start, double &end, double &duration, bool &isStart);

    // Generate formatted text report
    static QString generateReport(const QString &filePath, double totalDuration, const QVector<CorruptedSegment> &segments);

signals:
    void runningChanged(bool running);
    void progressChanged(double fraction);
    void segmentDiscovered(const CorruptedSegment &segment);
    void scanFinished(bool success, const QVector<CorruptedSegment> &segments);
    void logMessage(const QString &line);

private slots:
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    void recordError(double timestamp, const QString &errorMsg);
    void finalizeCurrentSegment();

    const FFmpegLocator *m_locator;
    QProcess *m_process = nullptr;

    bool m_running = false;
    double m_progress = 0.0;
    double m_totalDuration = 0.0;
    double m_currentDecodeTime = 0.0;
    QCOptions m_options;

    double m_activeSilenceStart = -1.0;
    double m_activeFreezeStart = -1.0;
    double m_activeFreezeDuration = 0.0;

    QString m_stdoutBuffer;
    QString m_stderrBuffer;

    QVector<CorruptedSegment> m_segments;
    CorruptedSegment m_activeSegment;
    bool m_hasActiveSegment = false;
};
