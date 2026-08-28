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
        {ErrorMessageRole, "jobError"}
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

void JobListModel::addJob(Job *job)
{
    beginInsertRows(QModelIndex(), m_jobs.size(), m_jobs.size());
    m_jobs.append(job);
    connectJobSignals(job);
    endInsertRows();
    emit countChanged();
}

void JobListModel::removeJob(int index)
{
    if (index < 0 || index >= m_jobs.size()) {
        return;
    }

    beginRemoveRows(QModelIndex(), index, index);
    m_jobs.removeAt(index);
    endRemoveRows();
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
            beginRemoveRows(QModelIndex(), i, i);
            m_jobs.removeAt(i);
            endRemoveRows();
        }
    }
    emit countChanged();
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
}
