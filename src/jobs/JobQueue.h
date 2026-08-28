#pragma once

#include <QObject>
#include <QList>
#include <QMap>
#include "Job.h"
#include "JobListModel.h"

class FFmpegLocator;
class FFmpegCommandBuilder;
class FFmpegProcess;
class MediaProbeService;

class JobQueue : public QObject
{
    Q_OBJECT
    Q_PROPERTY(JobListModel* model READ model CONSTANT)
    Q_PROPERTY(int maxConcurrent READ maxConcurrent WRITE setMaxConcurrent NOTIFY maxConcurrentChanged)
    Q_PROPERTY(bool hasRunningJobs READ hasRunningJobs NOTIFY runningCountChanged)

public:
    explicit JobQueue(const FFmpegLocator *locator, const MediaProbeService *probeService,
                      QObject *parent = nullptr);

    JobListModel* model() const;
    int maxConcurrent() const;
    void setMaxConcurrent(int max);
    bool hasRunningJobs() const;

    Q_INVOKABLE void addJob(Job *job);
    Q_INVOKABLE void startNext();
    Q_INVOKABLE void cancelJob(const QString &jobId);
    Q_INVOKABLE void retryJob(const QString &jobId);
    Q_INVOKABLE void clearCompleted();

signals:
    void maxConcurrentChanged();
    void runningCountChanged();
    void allJobsFinished();

private slots:
    void onJobFinished(bool success);

private:
    void startJob(Job *job);
    int runningJobCount() const;

    const FFmpegLocator *m_locator;
    const MediaProbeService *m_probeService;
    FFmpegCommandBuilder *m_commandBuilder;
    JobListModel *m_model;

    int m_maxConcurrent = 1;
    QMap<QUuid, FFmpegProcess*> m_processes;
};
