#include <QtTest>

#include <QSignalSpy>

#include "ffmpeg/FFmpegLocator.h"
#include "jobs/Job.h"
#include "jobs/JobQueue.h"

class JobQueueTest : public QObject
{
    Q_OBJECT

private slots:
    void runsQueuedJobsWithinConcurrencyLimit();
    void cancelWhileRunningLeavesJobCancelledAndFreesSlot();
    void shortJobCompletesAndSignalsAllFinished();
    void hardwareAccelerationFailureFallsBackToSoftware();
    void increasingMaxConcurrentImmediatelyStartsPendingJobs();
};

namespace {
Job *makeTestsrcJob(JobQueue &queue, const QString &ffmpeg, double seconds)
{
    auto *job = new Job(&queue);
    job->setCommand(ffmpeg,
                    {QStringLiteral("-f"), QStringLiteral("lavfi"),
                     QStringLiteral("-i"),
                     QStringLiteral("testsrc=duration=%1:size=128x128:rate=10").arg(seconds),
                     QStringLiteral("-f"), QStringLiteral("null"), QStringLiteral("-")});
    job->setTotalDuration(seconds);
    return job;
}
} // namespace

void JobQueueTest::runsQueuedJobsWithinConcurrencyLimit()
{
    FFmpegLocator locator;
    locator.waitForDetection();
    if (!locator.isValid()) {
        QSKIP("ffmpeg/ffprobe not available");
    }

    JobQueue queue(&locator, nullptr);
    queue.setMaxConcurrent(1);

    Job *first = makeTestsrcJob(queue, locator.ffmpegPath(), 60);
    Job *second = makeTestsrcJob(queue, locator.ffmpegPath(), 60);

    queue.addJob(first);
    QCOMPARE(first->status(), JobStatus::Running);

    queue.addJob(second);
    QCOMPARE(second->status(), JobStatus::Pending);
    QVERIFY(queue.hasRunningJobs());

    // Cancelling the running job must free its slot once its process is gone.
    queue.cancelJob(first->id().toString());
    QCOMPARE(first->status(), JobStatus::Cancelled);
    QTRY_VERIFY_WITH_TIMEOUT(second->status() == JobStatus::Running, 20000);
    QCOMPARE(first->status(), JobStatus::Cancelled);

    queue.cancelJob(second->id().toString());
    QTRY_VERIFY_WITH_TIMEOUT(!queue.hasRunningJobs(), 20000);
}

void JobQueueTest::cancelWhileRunningLeavesJobCancelledAndFreesSlot()
{
    FFmpegLocator locator;
    locator.waitForDetection();
    if (!locator.isValid()) {
        QSKIP("ffmpeg/ffprobe not available");
    }

    JobQueue queue(&locator, nullptr);
    queue.setMaxConcurrent(1);

    Job *job = makeTestsrcJob(queue, locator.ffmpegPath(), 60);
    queue.addJob(job);
    QCOMPARE(job->status(), JobStatus::Running);

    queue.cancelJob(job->id().toString());
    QCOMPARE(job->status(), JobStatus::Cancelled);

    // Cancellation survives terminate/kill/finished and must not be
    // overwritten into Failed/Completed by the late exit.
    QTest::qWait(150);
    QCOMPARE(job->status(), JobStatus::Cancelled);
    QTRY_VERIFY_WITH_TIMEOUT(!queue.hasRunningJobs(), 20000);
    QTest::qWait(300);
    QCOMPARE(job->status(), JobStatus::Cancelled);

    // Retry after cancel must be able to run a fresh process. With a free
    // slot it starts right away, so accept either Pending-then-Running.
    queue.retryJob(job->id().toString());
    QTRY_VERIFY_WITH_TIMEOUT(job->status() == JobStatus::Running, 20000);

    queue.cancelJob(job->id().toString());
    QTRY_VERIFY_WITH_TIMEOUT(!queue.hasRunningJobs(), 20000);
    QCOMPARE(job->status(), JobStatus::Cancelled);
}

void JobQueueTest::shortJobCompletesAndSignalsAllFinished()
{
    FFmpegLocator locator;
    locator.waitForDetection();
    if (!locator.isValid()) {
        QSKIP("ffmpeg/ffprobe not available");
    }

    JobQueue queue(&locator, nullptr);
    QSignalSpy allFinishedSpy(&queue, &JobQueue::allJobsFinished);
    QSignalSpy jobFinishedSpy(&queue, &JobQueue::jobFinished);

    Job *job = makeTestsrcJob(queue, locator.ffmpegPath(), 0.2);
    queue.addJob(job);

    QTRY_VERIFY_WITH_TIMEOUT(job->status() == JobStatus::Completed, 30000);
    QCOMPARE(job->progress(), 1.0);
    QTRY_COMPARE_WITH_TIMEOUT(allFinishedSpy.count(), 1, 30000);
    QTRY_COMPARE_WITH_TIMEOUT(jobFinishedSpy.count(), 1, 30000);
    QCOMPARE(jobFinishedSpy.first().at(1).toBool(), true);
    QTest::qWait(200);
    QCOMPARE(allFinishedSpy.count(), 1);
}

void JobQueueTest::hardwareAccelerationFailureFallsBackToSoftware()
{
    FFmpegLocator locator;
    locator.waitForDetection();
    if (!locator.isValid()) {
        QSKIP("ffmpeg/ffprobe not available");
    }

    JobQueue queue(&locator, nullptr);
    auto *job = new Job(&queue);
    job->setCommand(locator.ffmpegPath(),
                    {QStringLiteral("-y"),
                     QStringLiteral("-hwaccel"), QStringLiteral("non_existent_dummy_hwaccel"),
                     QStringLiteral("-f"), QStringLiteral("lavfi"),
                     QStringLiteral("-i"), QStringLiteral("testsrc=duration=0.2:size=128x128:rate=10"),
                     QStringLiteral("-c:v"), QStringLiteral("h264_nvenc"),
                     QStringLiteral("-f"), QStringLiteral("null"), QStringLiteral("-")});
    job->setTotalDuration(0.2);

    QVERIFY(job->autoSoftwareFallback());
    QVERIFY(!job->fallbackAttempted());

    queue.addJob(job);

    // Initial hardware invocation fails due to invalid hwaccel/encoder,
    // triggers automatic fallback to software libx264, and completes successfully.
    QTRY_VERIFY_WITH_TIMEOUT(job->status() == JobStatus::Completed, 30000);
    QVERIFY(job->fallbackAttempted());
    QCOMPARE(job->progress(), 1.0);
    QVERIFY(job->commandArguments().contains(QStringLiteral("libx264")));
    QVERIFY(!job->commandArguments().contains(QStringLiteral("-hwaccel")));
}

void JobQueueTest::increasingMaxConcurrentImmediatelyStartsPendingJobs()
{
    FFmpegLocator locator;
    locator.waitForDetection();
    if (!locator.isValid()) {
        QSKIP("ffmpeg/ffprobe not available");
    }

    JobQueue queue(&locator, nullptr);
    queue.setMaxConcurrent(1);

    Job *first = makeTestsrcJob(queue, locator.ffmpegPath(), 60);
    Job *second = makeTestsrcJob(queue, locator.ffmpegPath(), 60);

    queue.addJob(first);
    queue.addJob(second);

    QCOMPARE(first->status(), JobStatus::Running);
    QCOMPARE(second->status(), JobStatus::Pending);

    // Raising concurrency limit must immediately schedule the waiting job
    queue.setMaxConcurrent(2);
    QCOMPARE(second->status(), JobStatus::Running);

    queue.cancelJob(first->id().toString());
    queue.cancelJob(second->id().toString());
    QTRY_VERIFY_WITH_TIMEOUT(!queue.hasRunningJobs(), 20000);
}

QTEST_GUILESS_MAIN(JobQueueTest)
#include "tst_JobQueue.moc"
