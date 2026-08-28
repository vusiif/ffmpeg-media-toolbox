#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTimer>

struct FFmpegProgress {
    qint64 frame = 0;
    qint64 outTimeUs = 0;
    double fps = 0;
    double speed = 0;
    qint64 totalSize = 0;
    double percent = 0;
};

class FFmpegProcess : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ isRunning NOTIFY stateChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY finished)

public:
    enum class State {
        Idle,
        Running,
        Finished,
        Failed,
        Cancelled
    };
    Q_ENUM(State)

    explicit FFmpegProcess(QObject *parent = nullptr);
    ~FFmpegProcess() override;

    bool start(const QString &program, const QStringList &arguments, double totalDuration = 0);
    void cancel();
    void kill();

    bool isRunning() const;
    double progress() const;
    State state() const;
    QString lastError() const;
    QString stderrOutput() const;
    int exitCode() const;

    FFmpegProgress currentProgress() const;

signals:
    void stateChanged();
    void progressChanged();
    void finished(bool success);
    void logMessage(const QString &message);

private slots:
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onErrorOccurred(QProcess::ProcessError error);

private:
    void parseProgressLine(const QString &line);
    void setState(State state);

    QProcess *m_process = nullptr;
    QTimer *m_killTimer = nullptr;

    State m_state = State::Idle;
    double m_progress = 0;
    double m_totalDuration = 0;
    QString m_stderrOutput;
    FFmpegProgress m_currentProgress;
    QString m_progressBuffer;
};
