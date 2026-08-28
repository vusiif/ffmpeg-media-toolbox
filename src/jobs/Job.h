#pragma once

#include <QObject>
#include <QUuid>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QtQmlIntegration/qqmlintegration.h>

enum class JobType {
    Conversion,
    ImageOperation,
    VideoOperation
};

enum class JobStatus {
    Pending,
    Preparing,
    Running,
    Completed,
    Failed,
    Cancelled
};

struct JobLog {
    QDateTime timestamp;
    QString message;
};

class Job : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QUuid id READ id CONSTANT)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(JobType type READ type NOTIFY statusChanged)
    Q_PROPERTY(JobStatus status READ status NOTIFY statusChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY statusChanged)
    Q_PROPERTY(QString commandProgram READ commandProgram NOTIFY commandChanged)
    Q_PROPERTY(QStringList commandArguments READ commandArguments NOTIFY commandChanged)
    Q_PROPERTY(double totalDuration READ totalDuration WRITE setTotalDuration NOTIFY commandChanged)

public:
    explicit Job(QObject *parent = nullptr);

    QUuid id() const;
    QString name() const;
    JobType type() const;
    JobStatus status() const;
    double progress() const;
    QString errorMessage() const;

    QString commandProgram() const;
    QStringList commandArguments() const;
    double totalDuration() const;

    void setName(const QString &name);
    void setType(JobType type);
    void setStatus(JobStatus status);
    void setProgress(double progress);
    void setErrorMessage(const QString &message);
    void setTotalDuration(double duration);

    Q_INVOKABLE void setCommand(const QString &program, const QStringList &arguments);
    Q_INVOKABLE QString commandDisplayString() const;

    QStringList inputFiles() const;
    QString outputFile() const;
    QList<JobLog> logs() const;
    QDateTime startTime() const;
    QDateTime endTime() const;

    void setInputFiles(const QStringList &files);
    void setOutputFile(const QString &file);
    void addLog(const QString &message);

signals:
    void nameChanged();
    void statusChanged();
    void progressChanged();
    void commandChanged();

private:
    QUuid m_id;
    QString m_name;
    JobType m_type = JobType::Conversion;
    JobStatus m_status = JobStatus::Pending;
    double m_progress = 0;
    QString m_errorMessage;

    QStringList m_inputFiles;
    QString m_outputFile;
    QString m_commandProgram;
    QStringList m_commandArguments;
    double m_totalDuration = 0;
    QList<JobLog> m_logs;
    QDateTime m_startTime;
    QDateTime m_endTime;
};
