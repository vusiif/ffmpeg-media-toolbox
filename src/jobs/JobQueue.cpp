#include "JobQueue.h"
#include "ffmpeg/FFmpegLocator.h"
#include "ffmpeg/FFmpegCommandBuilder.h"
#include "ffmpeg/FFmpegProcess.h"
#include "media/MediaProbeService.h"

JobQueue::JobQueue(const FFmpegLocator *locator, const MediaProbeService *probeService, QObject *parent)
    : QObject(parent)
    , m_locator(locator)
    , m_probeService(probeService)
    , m_commandBuilder(new FFmpegCommandBuilder(this))
    , m_model(new JobListModel(this))
{
}

JobListModel* JobQueue::model() const { return m_model; }
int JobQueue::maxConcurrent() const { return m_maxConcurrent; }
bool JobQueue::hasRunningJobs() const { return runningJobCount() > 0; }

void JobQueue::setMaxConcurrent(int max)
{
    if (max < 1) max = 1;
    if (m_maxConcurrent == max) return;
    m_maxConcurrent = max;
    emit maxConcurrentChanged();
}

void JobQueue::addJob(Job *job)
{
    m_model->addJob(job);
    startNext();
}

void JobQueue::startNext()
{
    if (runningJobCount() >= m_maxConcurrent) {
        return;
    }

    for (int i = 0; i < m_model->count(); ++i) {
        Job *job = m_model->getJob(i);
        if (job && job->status() == JobStatus::Pending) {
            startJob(job);
            if (runningJobCount() >= m_maxConcurrent) {
                break;
            }
        }
    }
}

void JobQueue::cancelJob(const QString &jobId)
{
    Job *job = m_model->findJobById(jobId);
    if (!job) return;

    if (job->status() == JobStatus::Running) {
        auto it = m_processes.find(job->id());
        if (it != m_processes.end()) {
            it.value()->cancel();
        }
    }

    job->setStatus(JobStatus::Cancelled);
    startNext();
}

void JobQueue::retryJob(const QString &jobId)
{
    Job *job = m_model->findJobById(jobId);
    if (!job) return;

    if (job->status() != JobStatus::Failed && job->status() != JobStatus::Cancelled) {
        return;
    }

    job->setStatus(JobStatus::Pending);
    job->setProgress(0);
    job->setErrorMessage({});
    startNext();
}

void JobQueue::clearCompleted()
{
    m_model->clearCompleted();
}

void JobQueue::startJob(Job *job)
{
    if (!m_locator || !m_locator->isValid()) {
        job->setStatus(JobStatus::Failed);
        job->setErrorMessage(QStringLiteral("FFmpeg not found"));
        return;
    }

    QString program = job->commandProgram();
    QStringList arguments = job->commandArguments();

    if (program.isEmpty()) {
        job->setStatus(JobStatus::Failed);
        job->setErrorMessage(QStringLiteral("Invalid command"));
        return;
    }

    job->setStatus(JobStatus::Running);
    job->setProgress(0);
    job->addLog(QStringLiteral("Starting: %1").arg(job->commandDisplayString()));

    FFmpegProcess *process = new FFmpegProcess(this);

    connect(process, &FFmpegProcess::progressChanged, this, [job, process]() {
        job->setProgress(process->progress());
    });

    connect(process, &FFmpegProcess::logMessage, this, [job](const QString &msg) {
        job->addLog(msg);
    });

    connect(process, &FFmpegProcess::finished, this, &JobQueue::onJobFinished);

    m_processes[job->id()] = process;

    process->start(program, arguments, job->totalDuration());
}

void JobQueue::onJobFinished(bool success)
{
    FFmpegProcess *process = qobject_cast<FFmpegProcess*>(sender());
    if (!process) return;

    QUuid jobId;
    for (auto it = m_processes.begin(); it != m_processes.end(); ++it) {
        if (it.value() == process) {
            jobId = it.key();
            break;
        }
    }

    Job *job = m_model->findJobById(jobId.toString());
    if (job) {
        if (success) {
            job->setStatus(JobStatus::Completed);
            job->setProgress(1.0);
            job->addLog(QStringLiteral("Completed successfully"));
        } else {
            job->setStatus(JobStatus::Failed);
            job->setErrorMessage(process->stderrOutput());
            job->addLog(QStringLiteral("Failed: %1").arg(process->stderrOutput()));
        }
    }

    m_processes.remove(jobId);
    process->deleteLater();

    emit runningCountChanged();

    bool anyRunning = false;
    for (int i = 0; i < m_model->count(); ++i) {
        if (m_model->getJob(i)->status() == JobStatus::Running) {
            anyRunning = true;
            break;
        }
    }
    if (!anyRunning) {
        bool anyPending = false;
        for (int i = 0; i < m_model->count(); ++i) {
            if (m_model->getJob(i)->status() == JobStatus::Pending) {
                anyPending = true;
                break;
            }
        }
        if (!anyPending) {
            emit allJobsFinished();
        }
    }

    startNext();
}

int JobQueue::runningJobCount() const
{
    int count = 0;
    for (int i = 0; i < m_model->count(); ++i) {
        if (m_model->getJob(i)->status() == JobStatus::Running) {
            count++;
        }
    }
    return count;
}
