#include "JobListModel.h"

JobListModel::JobListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int JobListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_jobs.size();
}

QVariant JobListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_jobs.size()) {
        return {};
    }

    Job *job = m_jobs.at(index.row());

    switch (role) {
    case IdRole: return job->id().toString();
    case NameRole: return job->name();
    case TypeRole: return static_cast<int>(job->type());
    case StatusRole: return static_cast<int>(job->status());
    case ProgressRole: return job->progress();
    case ErrorMessageRole: return job->errorMessage();
    case PriorityRole: return static_cast<int>(job->priority());
    default: return {};
    }
}

QHash<int, QByteArray> JobListModel::roleNames() const
{
    return {
        {IdRole, "jobId"},
        {NameRole, "jobName"},
        {TypeRole, "jobType"},
        {StatusRole, "jobStatus"},
        {ProgressRole, "jobProgress"},
        {ErrorMessageRole, "jobError"},
        {PriorityRole, "jobPriority"}
    };
}

int JobListModel::count() const { return m_jobs.size(); }

int JobListModel::runningCount() const
{
    int count = 0;
    for (const Job *job : m_jobs) {
        if (job->status() == JobStatus::Running) {
            count++;
        }
    }
    return count;
}

int JobListModel::completedCount() const
{
    int count = 0;
    for (const Job *job : m_jobs) {
        if (job->status() == JobStatus::Completed) {
            count++;
        }
    }
    return count;
}

Job* JobListModel::getJob(int index) const
{
    if (index < 0 || index >= m_jobs.size()) {
        return nullptr;
    }
    return m_jobs.at(index);
}

Job* JobListModel::findJobById(const QString &id) const
{
    QUuid uuid(id);
    for (Job *job : m_jobs) {
        if (job->id() == uuid) {
            return job;
        }
    }
    return nullptr;
}

int JobListModel::indexOfJob(const QString &id) const
{
    QUuid uuid(id);
    for (int i = 0; i < m_jobs.size(); ++i) {
        if (m_jobs.at(i)->id() == uuid) {
            return i;
        }
    }
    return -1;
}

bool JobListModel::moveJobUp(int index)
{
    if (index <= 0 || index >= m_jobs.size()) {
        return false;
    }
    beginMoveRows(QModelIndex(), index, index, QModelIndex(), index - 1);
    m_jobs.swapItemsAt(index, index - 1);
    endMoveRows();
    return true;
}

bool JobListModel::moveJobDown(int index)
{
    if (index < 0 || index >= m_jobs.size() - 1) {
        return false;
    }
    beginMoveRows(QModelIndex(), index, index, QModelIndex(), index + 2);
    m_jobs.swapItemsAt(index, index + 1);
    endMoveRows();
    return true;
}

bool JobListModel::moveJobToTop(int index)
{
    if (index <= 0 || index >= m_jobs.size()) {
        return false;
    }
    beginMoveRows(QModelIndex(), index, index, QModelIndex(), 0);
    Job *job = m_jobs.takeAt(index);
    m_jobs.prepend(job);
    endMoveRows();
    return true;
}

bool JobListModel::moveJobToBottom(int index)
{
    if (index < 0 || index >= m_jobs.size() - 1) {
        return false;
    }
    beginMoveRows(QModelIndex(), index, index, QModelIndex(), m_jobs.size());
    Job *job = m_jobs.takeAt(index);
    m_jobs.append(job);
    endMoveRows();
    return true;
}

void JobListModel::addJob(Job *job)
{
    if (!job) return;
    addJobs({job});
}

void JobListModel::addJobs(const QList<Job*> &jobs)
{
    QList<Job*> validJobs;
    validJobs.reserve(jobs.size());
    for (Job *job : jobs) {
        if (job) {
            validJobs.append(job);
        }
    }
    if (validJobs.isEmpty()) {
        return;
    }

    const int startRow = m_jobs.size();
    const int endRow = startRow + validJobs.size() - 1;
    beginInsertRows(QModelIndex(), startRow, endRow);
    for (Job *job : validJobs) {
        m_jobs.append(job);
        connectJobSignals(job);
    }
    endInsertRows();
    emit countChanged();
}

void JobListModel::removeJob(int index)
{
    if (index < 0 || index >= m_jobs.size()) {
        return;
    }

    beginRemoveRows(QModelIndex(), index, index);
    Job *job = m_jobs.takeAt(index);
    endRemoveRows();
    // Deferred so queued signal handlers and the still-winding-down process
    // lambdas never touch a freed job.
    job->deleteLater();
    emit countChanged();
}

void JobListModel::removeJobById(const QString &id)
{
    QUuid uuid(id);
    for (int i = 0; i < m_jobs.size(); ++i) {
        if (m_jobs.at(i)->id() == uuid) {
            removeJob(i);
            return;
        }
    }
}

void JobListModel::clearCompleted()
{
    for (int i = m_jobs.size() - 1; i >= 0; --i) {
        JobStatus status = m_jobs.at(i)->status();
        if (status == JobStatus::Completed || status == JobStatus::Failed || status == JobStatus::Cancelled) {
            removeJob(i);
        }
    }
}

void JobListModel::connectJobSignals(Job *job)
{
    connect(job, &Job::statusChanged, this, [this, job]() {
        int idx = m_jobs.indexOf(job);
        if (idx >= 0) {
            QModelIndex modelIndex = index(idx);
            emit dataChanged(modelIndex, modelIndex, {StatusRole});
            emit countChanged();
        }
    });

    connect(job, &Job::progressChanged, this, [this, job]() {
        int idx = m_jobs.indexOf(job);
        if (idx >= 0) {
            QModelIndex modelIndex = index(idx);
            emit dataChanged(modelIndex, modelIndex, {ProgressRole});
        }
    });

    connect(job, &Job::nameChanged, this, [this, job]() {
        int idx = m_jobs.indexOf(job);
        if (idx >= 0) {
            QModelIndex modelIndex = index(idx);
            emit dataChanged(modelIndex, modelIndex, {NameRole});
        }
    });

    connect(job, &Job::priorityChanged, this, [this, job]() {
        int idx = m_jobs.indexOf(job);
        if (idx >= 0) {
            QModelIndex modelIndex = index(idx);
            emit dataChanged(modelIndex, modelIndex, {PriorityRole});
        }
    });
}
