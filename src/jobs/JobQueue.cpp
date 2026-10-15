#include "JobQueue.h"
#include "ffmpeg/FFmpegLocator.h"
#include "ffmpeg/FFmpegCommandBuilder.h"
#include "ffmpeg/FFmpegProcess.h"
#include "media/MediaProbeService.h"

#include <QPointer>

#include <algorithm>

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
    if (m_processes.size() >= m_maxConcurrent) {
        return;
    }

    for (int i = 0; i < m_model->count(); ++i) {
        Job *job = m_model->getJob(i);
        if (job && job->status() == JobStatus::Pending) {
            // A cancelled job's process may still be winding down; its slot
            // stays occupied until that process reports finished.
            if (m_processes.contains(job->id())) {
                continue;
            }
            startJob(job);
            if (m_processes.size() >= m_maxConcurrent) {
                break;
            }
        }
    }
}

void JobQueue::cancelJob(const QString &jobId)
{
    Job *job = m_model->findJobById(jobId);
    if (!job) return;

    const JobStatus status = job->status();
    if (status != JobStatus::Running && status != JobStatus::Pending) {
        return;
    }

    if (status == JobStatus::Running) {
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
    job->setFallbackAttempted(false);
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

    // Retry-after-cancel can race the previous process's wind-down. Detach and
    // dispose of any stale process so it can never touch this job again.
    if (FFmpegProcess *stale = m_processes.take(job->id())) {
        stale->disconnect(this);
        stale->kill();
        stale->deleteLater();
    }

    FFmpegProcess *process = new FFmpegProcess(this);

    // The job can be cleared while its process is still winding down
    // (cancel + clear), so the handlers must not assume it is alive.
    const QPointer<Job> guard = job;
    connect(process, &FFmpegProcess::progressChanged, this, [guard, process]() {
        if (guard) guard->setProgress(process->progress());
    });

    connect(process, &FFmpegProcess::logMessage, this, [guard](const QString &msg) {
        if (guard) guard->addLog(msg);
    });

    connect(process, &FFmpegProcess::finished, this, &JobQueue::onJobFinished);

    m_processes[job->id()] = process;

    process->start(program, arguments, job->totalDuration());
}

void JobQueue::onJobFinished(bool success)
{
    FFmpegProcess *process = qobject_cast<FFmpegProcess*>(sender());
    if (!process) return;

    auto it = std::find_if(m_processes.begin(), m_processes.end(),
                           [process](FFmpegProcess *p) { return p == process; });
    if (it == m_processes.end()) {
        // Duplicate or stale emission (e.g. a superseded winding-down
        // process): accounting must not run twice for one process.
        return;
    }
    const QUuid jobId = it.key();

    Job *job = m_model->findJobById(jobId.toString());
    if (job) {
        const JobStatus status = job->status();
        if (status == JobStatus::Cancelled) {
            // Cancellation stays Cancelled through terminate/kill/finished.
            job->addLog(QStringLiteral("Cancelled"));
        } else if (status == JobStatus::Pending) {
            // Retried while the old process wound down; leave it Pending so
            // startNext() can launch a fresh process for it.
        } else if (success) {
            job->setStatus(JobStatus::Completed);
            job->setProgress(1.0);
            job->addLog(QStringLiteral("Completed successfully"));
        } else {
            if (job->autoSoftwareFallback() && !job->fallbackAttempted()) {
                QStringList fallbackArgs;
                if (FFmpegCommandBuilder::canFallbackToSoftware(job->commandArguments(), &fallbackArgs)) {
                    job->setFallbackAttempted(true);
                    job->setCommand(job->commandProgram(), fallbackArgs);
                    job->setStatus(JobStatus::Pending);
                    job->setProgress(0);
                    job->addLog(QStringLiteral("Hardware acceleration failed. Automatically falling back to software encoding (%1)...")
                                    .arg(job->commandDisplayString()));
                    m_processes.remove(jobId);
                    process->deleteLater();
                    emit runningCountChanged();
                    startNext();
                    return;
                }
            }

            job->setStatus(JobStatus::Failed);
            job->setErrorMessage(process->stderrOutput());
            job->addLog(QStringLiteral("Failed: %1").arg(process->stderrOutput()));
        }
    }

    m_processes.remove(jobId);
    process->deleteLater();

    emit runningCountChanged();

    bool anyActive = m_processes.isEmpty();
    for (int i = 0; i < m_model->count(); ++i) {
        const JobStatus status = m_model->getJob(i)->status();
        if (status == JobStatus::Running || status == JobStatus::Pending) {
            anyActive = false;
            break;
        }
    }
    if (anyActive) {
        emit allJobsFinished();
    }

    startNext();
}

int JobQueue::runningJobCount() const
{
    // Live processes are the real concurrency unit: a cancelled job whose
    // process is still terminating must keep occupying its slot.
    return m_processes.size();
}
