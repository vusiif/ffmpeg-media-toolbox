#include <QPointer>
#include <QtTest>

#include "jobs/Job.h"
#include "jobs/JobListModel.h"

class JobListModelTest : public QObject
{
    Q_OBJECT

private slots:
    void addAndFind();
    void clearCompletedKeepsActiveJobs();
    void removeJobByIdRemovesEntry();
    void removedJobsAreDeletedLater();
    void addJobsBatch();
};

void JobListModelTest::addAndFind()
{
    JobListModel model;
    auto *job = new Job();
    job->setName(QStringLiteral("clip"));

    model.addJob(job);
    QCOMPARE(model.count(), 1);
    QCOMPARE(model.getJob(0), job);
    QCOMPARE(model.findJobById(job->id().toString()), job);
    QVERIFY(model.findJobById(QStringLiteral("not-a-job")) == nullptr);

    model.removeJob(0);
    QCOMPARE(model.count(), 0);
}

void JobListModelTest::clearCompletedKeepsActiveJobs()
{
    JobListModel model;

    auto *completed = new Job();
    completed->setStatus(JobStatus::Completed);
    auto *failed = new Job();
    failed->setStatus(JobStatus::Failed);
    auto *running = new Job();
    running->setStatus(JobStatus::Running);
    auto *pending = new Job();

    model.addJob(completed);
    model.addJob(failed);
    model.addJob(running);
    model.addJob(pending);
    QCOMPARE(model.count(), 4);

    model.clearCompleted();
    QCOMPARE(model.count(), 2);
    QVERIFY(model.findJobById(completed->id().toString()) == nullptr);
    QVERIFY(model.findJobById(failed->id().toString()) == nullptr);
    QVERIFY(model.findJobById(running->id().toString()) == running);
    QVERIFY(model.findJobById(pending->id().toString()) == pending);

    model.removeJob(0);
    model.removeJob(0);
    QCOMPARE(model.count(), 0);
}

void JobListModelTest::removeJobByIdRemovesEntry()
{
    JobListModel model;
    auto *job = new Job();
    model.addJob(job);
    QCOMPARE(model.count(), 1);

    model.removeJobById(job->id().toString());
    QCOMPARE(model.count(), 0);

    // Removing again must not crash or corrupt the model.
    model.removeJobById(job->id().toString());
    QCOMPARE(model.count(), 0);
}

void JobListModelTest::removedJobsAreDeletedLater()
{
    JobListModel model;
    auto *job = new Job();
    job->setStatus(JobStatus::Completed);
    model.addJob(job);

    const QPointer<Job> guard = job;
    model.clearCompleted();

    QVERIFY(guard);
    QVERIFY(model.findJobById(job->id().toString()) == nullptr);
    QTest::qWait(1);
    QVERIFY(guard.isNull());
}

void JobListModelTest::addJobsBatch()
{
    JobListModel model;

    // 1. Empty list
    model.addJobs({});
    QCOMPARE(model.count(), 0);

    // 2. Batch with valid and null jobs
    auto *j1 = new Job();
    j1->setName(QStringLiteral("job1"));
    auto *j2 = new Job();
    j2->setName(QStringLiteral("job2"));
    auto *j3 = new Job();
    j3->setName(QStringLiteral("job3"));

    model.addJobs({j1, nullptr, j2, j3});
    QCOMPARE(model.count(), 3);
    QCOMPARE(model.getJob(0), j1);
    QCOMPARE(model.getJob(1), j2);
    QCOMPARE(model.getJob(2), j3);

    // Clean up
    model.removeJob(0);
    model.removeJob(0);
    model.removeJob(0);
    QCOMPARE(model.count(), 0);
}

QTEST_GUILESS_MAIN(JobListModelTest)
#include "tst_JobListModel.moc"
