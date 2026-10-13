#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QVector>

struct CorruptedSegment
{
    double start = 0.0;    // start timestamp in seconds
    double end = 0.0;      // end timestamp in seconds
    QString errorMsg;      // summary description of error
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
    void cancel();

    bool isRunning() const;
    double progress() const;
    QVector<CorruptedSegment> segments() const;
    double totalDuration() const;

    // Helper to merge a list of error points into grouped segments
    static QVector<CorruptedSegment> mergeErrorTimestamps(
        const QVector<QPair<double, QString>> &rawErrors,
        double mergeThreshold = 0.5,
        double defaultFrameDuration = 0.08
    );

    // Extracts timestamp from an FFmpeg error log line, or returns -1.0 if none found
    static double extractTimestampFromLine(const QString &line);

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

    QString m_stdoutBuffer;
    QString m_stderrBuffer;

    QVector<CorruptedSegment> m_segments;
    CorruptedSegment m_activeSegment;
    bool m_hasActiveSegment = false;
};
