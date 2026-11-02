#pragma once

#include <QColor>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

struct VideoQualityResult
{
    bool success = false;
    QString errorMessage;

    double vmaf = -1.0;          // 0.0 - 100.0 (Netflix VMAF score)
    double ssim = -1.0;          // 0.0 - 1.0 (Structural Similarity Index)
    double psnr = -1.0;          // dB (Peak Signal-to-Noise Ratio)

    QString vmafRating() const;
    QColor ratingColor() const;
    QString summaryText() const;
};

struct VideoQualityOptions
{
    bool calculateVmaf = true;
    bool calculateSsim = true;
    bool calculatePsnr = true;

    double sampleStart = 0.0;
    double sampleDuration = 0.0; // 0.0 = full video, >0 = sample slice
    int threads = 0;             // 0 = auto
    int subsample = 1;           // 1 = all frames, 2 = every 2nd frame
};

class FFmpegLocator;

class VideoQualityEvaluator : public QObject
{
    Q_OBJECT

public:
    explicit VideoQualityEvaluator(const FFmpegLocator *locator, QObject *parent = nullptr);
    ~VideoQualityEvaluator() override;

    bool startEvaluation(const QString &referencePath,
                         const QString &distortedPath,
                         const VideoQualityOptions &options = VideoQualityOptions(),
                         double totalDuration = 0.0,
                         int refWidth = 0,
                         int refHeight = 0);
    void cancel();

    bool isRunning() const;
    double progress() const;
    VideoQualityResult result() const;

    static QStringList buildCommand(const QString &referencePath,
                                   const QString &distortedPath,
                                   const VideoQualityOptions &options,
                                   int refWidth = 0,
                                   int refHeight = 0);

    static bool parseVmaf(const QString &line, double &score);
    static bool parseSsim(const QString &line, double &score);
    static bool parsePsnr(const QString &line, double &score);
    static QString generateReport(const QString &referencePath,
                                 const QString &distortedPath,
                                 const VideoQualityResult &res,
                                 const VideoQualityOptions &opts);

signals:
    void progressChanged(double fraction);
    void finished(const VideoQualityResult &result);
    void logMessage(const QString &line);

private slots:
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    const FFmpegLocator *m_locator;
    QProcess *m_process = nullptr;

    bool m_running = false;
    double m_progress = 0.0;
    double m_totalDuration = 0.0;
    double m_currentDecodeTime = 0.0;
    VideoQualityOptions m_options;
    VideoQualityResult m_result;

    QString m_stdoutBuffer;
    QString m_stderrBuffer;
};
