#include "JobQueue.h"
#include "ffmpeg/FFmpegLocator.h"
#include "ffmpeg/FFmpegCommandBuilder.h"
#include "ffmpeg/FFmpegProcess.h"
#include "media/MediaProbeService.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPointer>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>

#include <algorithm>

QString JobQueue::defaultStoragePath()
{
    const QString appName = QCoreApplication::applicationName();
    if (appName.contains(QLatin1String("test"), Qt::CaseInsensitive) ||
        appName.contains(QLatin1String("tst_"), Qt::CaseInsensitive)) {
        return QString();
    }
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty()) {
        return QString();
    }
    QDir().mkpath(base);
    return QDir(base).filePath(QStringLiteral("jobs.json"));
}

JobQueue::JobQueue(const FFmpegLocator *locator, const MediaProbeService *probeService, QObject *parent)
    : JobQueue(locator, probeService, parent, defaultStoragePath())
{
}

JobQueue::JobQueue(const FFmpegLocator *locator, const MediaProbeService *probeService,
                   QObject *parent, const QString &storagePath)
    : QObject(parent)
    , m_locator(locator)
    , m_probeService(probeService)
    , m_commandBuilder(new FFmpegCommandBuilder(this))
    , m_model(new JobListModel(this))
    , m_storagePath(storagePath)
{
    connect(m_model, &QAbstractItemModel::rowsRemoved, this, [this]() {
        savePersistence();
    });

    if (!m_storagePath.isEmpty()) {
        loadPersistence();
    }
}

JobQueue::~JobQueue()
{
    // Gracefully cancel active FFmpeg processes upon queue destruction
    for (auto it = m_processes.begin(); it != m_processes.end(); ++it) {
        if (FFmpegProcess *proc = it.value()) {
            proc->disconnect(this);
            proc->cancel();
        }
    }
    m_processes.clear();
    savePersistence();
}

JobListModel* JobQueue::model() const { return m_model; }
int JobQueue::maxConcurrent() const { return m_maxConcurrent; }
bool JobQueue::hasRunningJobs() const { return runningJobCount() > 0; }

QString JobQueue::storagePath() const { return m_storagePath; }
void JobQueue::setStoragePath(const QString &path) { m_storagePath = path; }

bool JobQueue::loadPersistence()
{
    if (m_storagePath.isEmpty() || !QFile::exists(m_storagePath)) {
        return false;
    }
    QFile file(m_storagePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QByteArray data = file.readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        return false;
    }

    const QJsonArray array = doc.array();
    QList<Job*> loadedJobs;
    for (const QJsonValue &val : array) {
        if (!val.isObject()) continue;
        Job *job = Job::fromJson(val.toObject(), this);
        if (job) {
            connect(job, &Job::statusChanged, this, [this]() {
                savePersistence();
            });
            loadedJobs.append(job);
        }
    }
    m_model->addJobs(loadedJobs);
    return true;
}

bool JobQueue::savePersistence() const
{
    if (m_storagePath.isEmpty()) {
        return false;
    }

    QFileInfo fi(m_storagePath);
    fi.dir().mkpath(QStringLiteral("."));

    QJsonArray array;
    for (int i = 0; i < m_model->count(); ++i) {
        if (Job *job = m_model->getJob(i)) {
            array.append(job->toJson());
        }
    }

    QJsonDocument doc(array);
    QSaveFile file(m_storagePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    return file.commit();
}

void JobQueue::setMaxConcurrent(int max)
{
    if (max < 1) max = 1;
    if (m_maxConcurrent == max) return;
    m_maxConcurrent = max;
    emit maxConcurrentChanged();
    startNext();
}

void JobQueue::addJob(Job *job)
{
    if (!job) return;
    addJobs({job});
}

void JobQueue::addJobs(const QList<Job*> &jobs)
{
    QList<Job*> validJobs;
    validJobs.reserve(jobs.size());
    for (Job *job : jobs) {
        if (job) {
            validJobs.append(job);
            connect(job, &Job::statusChanged, this, [this]() {
                savePersistence();
            });
        }
    }
    if (validJobs.isEmpty()) {
        return;
    }

    m_model->addJobs(validJobs);
    savePersistence();
    startNext();
}

bool JobQueue::isPaused() const { return m_isPaused; }
void JobQueue::setPaused(bool paused)
{
    if (m_isPaused == paused) return;
    m_isPaused = paused;
    emit queuePausedChanged(m_isPaused);
    if (!m_isPaused) {
        startNext();
    }
}
void JobQueue::pause() { setPaused(true); }
void JobQueue::resume() { setPaused(false); }

void JobQueue::cancelAll()
{
    for (int i = 0; i < m_model->count(); ++i) {
        if (Job *job = m_model->getJob(i)) {
            const JobStatus s = job->status();
            if (s == JobStatus::Running || s == JobStatus::Pending) {
                cancelJob(job->id().toString());
            }
        }
    }
}

void JobQueue::retryAllFailed()
{
    for (int i = 0; i < m_model->count(); ++i) {
        if (Job *job = m_model->getJob(i)) {
            const JobStatus s = job->status();
            if (s == JobStatus::Failed || s == JobStatus::Cancelled) {
                retryJob(job->id().toString());
            }
        }
    }
}

bool JobQueue::moveJobUp(const QString &jobId)
{
    const int idx = m_model->indexOfJob(jobId);
    if (idx > 0) {
        bool ok = m_model->moveJobUp(idx);
        if (ok) savePersistence();
        return ok;
    }
    return false;
}

bool JobQueue::moveJobDown(const QString &jobId)
{
    const int idx = m_model->indexOfJob(jobId);
    if (idx >= 0 && idx < m_model->count() - 1) {
        bool ok = m_model->moveJobDown(idx);
        if (ok) savePersistence();
        return ok;
    }
    return false;
}

bool JobQueue::moveJobToTop(const QString &jobId)
{
    const int idx = m_model->indexOfJob(jobId);
    if (idx > 0) {
        bool ok = m_model->moveJobToTop(idx);
        if (ok) savePersistence();
        return ok;
    }
    return false;
}

int JobQueue::pendingJobCount() const
{
    int count = 0;
    for (int i = 0; i < m_model->count(); ++i) {
        if (Job *job = m_model->getJob(i)) {
            if (job->status() == JobStatus::Pending) count++;
        }
    }
    return count;
}

int JobQueue::failedJobCount() const
{
    int count = 0;
    for (int i = 0; i < m_model->count(); ++i) {
        if (Job *job = m_model->getJob(i)) {
            if (job->status() == JobStatus::Failed) count++;
        }
    }
    return count;
}

JobQueue::PostQueueAction JobQueue::postQueueAction() const { return m_postAction; }
void JobQueue::setPostQueueAction(JobQueue::PostQueueAction action) { m_postAction = action; }

void JobQueue::startNext()
{
    if (m_isPaused) {
        return;
    }
    if (m_processes.size() >= m_maxConcurrent) {
        return;
    }

    while (m_processes.size() < m_maxConcurrent) {
        Job *bestJob = nullptr;
        int bestPriorityVal = -1;

        for (int i = 0; i < m_model->count(); ++i) {
            Job *job = m_model->getJob(i);
            if (!job || job->status() != JobStatus::Pending) {
                continue;
            }
            if (m_processes.contains(job->id())) {
                continue;
            }

            const int prio = static_cast<int>(job->priority());
            if (prio > bestPriorityVal) {
                bestPriorityVal = prio;
                bestJob = job;
            }
        }

        if (!bestJob) {
            break;
        }

        startJob(bestJob);
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
    savePersistence();
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
    savePersistence();
    startNext();
}

void JobQueue::clearCompleted()
{
    m_model->clearCompleted();
    savePersistence();
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
                    savePersistence();
                    startNext();
                    return;
                }
            }

            job->setStatus(JobStatus::Failed);
            job->setErrorMessage(process->stderrOutput());
            job->addLog(QStringLiteral("Failed: %1").arg(process->stderrOutput()));
        }
    }

    const QString jobName = job ? job->name() : QString();
    m_processes.remove(jobId);
    process->deleteLater();

    emit runningCountChanged();
    emit jobFinished(jobName, success);

    bool noActiveJobs = m_processes.isEmpty();
    for (int i = 0; i < m_model->count(); ++i) {
        Job *j = m_model->getJob(i);
        if (!j) continue;
        const JobStatus status = j->status();
        if (status == JobStatus::Running || status == JobStatus::Pending) {
            noActiveJobs = false;
            break;
        }
    }
    if (noActiveJobs) {
        emit allJobsFinished();
        if (m_postAction == PostQueueAction::OpenFolder) {
            QString outDir;
            for (int i = m_model->count() - 1; i >= 0; --i) {
                if (Job *j = m_model->getJob(i)) {
                    if (j->status() == JobStatus::Completed && !j->outputFile().isEmpty()) {
                        outDir = QFileInfo(j->outputFile()).absolutePath();
                        break;
                    }
                }
            }
            if (!outDir.isEmpty() && QDir(outDir).exists()) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(outDir));
            }
        } else if (m_postAction == PostQueueAction::SoundBeep) {
            QApplication::beep();
        }
    }

    savePersistence();
    startNext();
}

int JobQueue::runningJobCount() const
{
    // Live processes are the real concurrency unit: a cancelled job whose
    // process is still terminating must keep occupying its slot.
    return m_processes.size();
}
