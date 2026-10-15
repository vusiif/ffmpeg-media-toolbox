#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

class UntruncProcess : public QObject
{
    Q_OBJECT

public:
    enum class State {
        Idle,
        Running,
        Finished,
        Failed,
        Cancelled
    };
    Q_ENUM(State)

    explicit UntruncProcess(QObject *parent = nullptr);
    ~UntruncProcess() override;

    bool start(const QString &program, const QString &referenceFile, const QString &corruptedFile);
    void cancel();

    bool isRunning() const;
    State state() const;
    QString fixedFilePath() const;
    QString lastError() const;

    static QString predictFixedPath(const QString &referenceFile, const QString &corruptedFile);

signals:
    void stateChanged(UntruncProcess::State state);
    void logMessage(const QString &line);
    void progressChanged(double fraction);
    void finished(bool success, const QString &fixedFilePath);

private slots:
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onErrorOccurred(QProcess::ProcessError error);

private:
    void setState(State state);
    void processOutputBuffer(QString &buffer);

    QProcess *m_process = nullptr;
    State m_state = State::Idle;
    QString m_referenceFile;
    QString m_corruptedFile;
    QString m_fixedFilePath;
    QString m_stdoutBuffer;
    QString m_stderrBuffer;
    QString m_lastError;

    friend class tst_UntruncProcess;
};
