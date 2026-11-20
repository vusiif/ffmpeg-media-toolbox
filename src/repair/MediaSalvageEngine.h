#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

class FFmpegLocator;

class MediaSalvageEngine : public QObject
{
    Q_OBJECT

public:
    enum class SalvageMode {
        FastRemux,     // -c copy with -err_detect ignore_err and +genpts+discardcorrupt
        ForceTranscode // re-encode surviving packets to libx264/aac with error resilience
    };
    Q_ENUM(SalvageMode)

    enum class State {
        Idle,
        Running,
        Success,
        Failed,
        Cancelled
    };
    Q_ENUM(State)

    explicit MediaSalvageEngine(const FFmpegLocator *locator, QObject *parent = nullptr);
    ~MediaSalvageEngine() override;

    bool start(const QString &inputPath,
               const QString &outputPath,
               SalvageMode mode = SalvageMode::FastRemux,
               double totalDuration = 0.0);
    void cancel();

    bool isRunning() const;
    State state() const;
    SalvageMode mode() const;
    QString inputPath() const;
    QString outputPath() const;
    QString lastError() const;
    double progress() const;

    // Helper to build arguments
    static QStringList buildArguments(const QString &inputPath,
                                      const QString &outputPath,
                                      SalvageMode mode);

    // Predict a default output path
    static QString suggestOutputPath(const QString &inputPath, SalvageMode mode);

signals:
    void stateChanged(MediaSalvageEngine::State newState);
    void progressChanged(double fraction);
    void logMessage(const QString &line);
    void statusMessage(const QString &status);
    void finished(bool success, const QString &outputPath, const QString &errorMsg);

private slots:
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    void setState(State newState);
    void parseProgressLine(const QString &line);

    const FFmpegLocator *m_locator = nullptr;
    QProcess *m_process = nullptr;
    State m_state = State::Idle;
    SalvageMode m_mode = SalvageMode::FastRemux;
    QString m_inputPath;
    QString m_outputPath;
    QString m_lastError;
    double m_totalDuration = 0.0;
    double m_progress = 0.0;
    QString m_stdoutBuffer;
    QString m_stderrBuffer;

    friend class tst_MediaSalvageEngine;
};
