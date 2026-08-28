#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QUuid>
#include "Job.h"

class JobListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int runningCount READ runningCount NOTIFY countChanged)
    Q_PROPERTY(int completedCount READ completedCount NOTIFY countChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        TypeRole,
        StatusRole,
        ProgressRole,
        ErrorMessageRole
    };

    explicit JobListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    int runningCount() const;
    int completedCount() const;

    Q_INVOKABLE Job* getJob(int index) const;
    Q_INVOKABLE Job* findJobById(const QString &id) const;

    void addJob(Job *job);
    void removeJob(int index);
    void removeJobById(const QString &id);
    void clearCompleted();

signals:
    void countChanged();

private:
    void connectJobSignals(Job *job);

    QList<Job*> m_jobs;
};
