#include <QtTest>

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "batch/BatchExportMatrix.h"
#include "batch/FolderWatcherService.h"
#include "ffmpeg/FFmpegLocator.h"
#include "jobs/Job.h"
#include "jobs/JobQueue.h"

class BatchProcessingTest : public QObject
{
    Q_OBJECT

private slots:
    void matrixDefaultTargets();
    void matrixSubpathAndOutputPathResolution();
    void matrixRecursiveDirectoryScan();
    void queuePauseAndResume();
    void queueCancelAllAndRetryAllFailed();
    void queueJobReordering();
    void queuePriorityScheduling();
    void folderWatcherStabilityDetection();
};

void BatchProcessingTest::matrixDefaultTargets()
{
    const QList<BatchExportTarget> targets = BatchExportMatrix::defaultTargets();
    QCOMPARE(targets.size(), 4);

    QCOMPARE(targets[0].format, QStringLiteral("mp4"));
    QCOMPARE(targets[0].vcodec, QStringLiteral("libx264"));
    QCOMPARE(targets[0].suffix, QStringLiteral("_1080p"));
    QVERIFY(targets[0].enabled);

    QCOMPARE(targets[1].format, QStringLiteral("mp4"));
    QCOMPARE(targets[1].suffix, QStringLiteral("_720p"));

    QCOMPARE(targets[2].format, QStringLiteral("mp4"));
    QCOMPARE(targets[2].vcodec, QStringLiteral("libx265"));

    QCOMPARE(targets[3].format, QStringLiteral("mp3"));
    QCOMPARE(targets[3].suffix, QStringLiteral("_audio"));

    // Serialization
    const QJsonObject json = targets[0].toJson();
    const BatchExportTarget restored = BatchExportTarget::fromJson(json);
    QCOMPARE(restored.name, targets[0].name);
    QCOMPARE(restored.format, targets[0].format);
    QCOMPARE(restored.quality, targets[0].quality);
}

void BatchProcessingTest::matrixSubpathAndOutputPathResolution()
{
    QCOMPARE(BatchExportMatrix::sanitizeSubPath(QStringLiteral("/Season1/Disc2/")), QStringLiteral("Season1/Disc2"));
    QCOMPARE(BatchExportMatrix::sanitizeSubPath(QStringLiteral("\\Season1\\Episode1\\")), QStringLiteral("Season1/Episode1"));

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString out = BatchExportMatrix::resolveOutputPath(tempDir.path(),
                                                           QStringLiteral("Season 1/Extras"),
                                                           QStringLiteral("Interview"),
                                                           QStringLiteral("mp4"),
                                                           QStringLiteral("_1080p"));

    QVERIFY(out.endsWith(QStringLiteral("Season 1/Extras/Interview_1080p.mp4")) ||
            out.endsWith(QStringLiteral("Season 1\\Extras\\Interview_1080p.mp4")));

    // Verify parent directory was created automatically
    QVERIFY(QDir(tempDir.path() + QStringLiteral("/Season 1/Extras")).exists());
}

void BatchProcessingTest::matrixRecursiveDirectoryScan()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString root = tempDir.path();
    QDir().mkpath(root + QStringLiteral("/Season1"));
    QDir().mkpath(root + QStringLiteral("/Season2/Bonus"));

    // Create dummy files
    QFile f1(root + QStringLiteral("/root_clip.mp4"));
    QVERIFY(f1.open(QIODevice::WriteOnly));
    f1.write("dummy");
    f1.close();

    QFile f2(root + QStringLiteral("/Season1/ep01.mkv"));
    QVERIFY(f2.open(QIODevice::WriteOnly));
    f2.write("dummy");
    f2.close();

    QFile f3(root + QStringLiteral("/Season2/Bonus/extra.mp3"));
    QVERIFY(f3.open(QIODevice::WriteOnly));
    f3.write("dummy");
    f3.close();

    // Ignore non-media
    QFile f4(root + QStringLiteral("/Season1/notes.txt"));
    QVERIFY(f4.open(QIODevice::WriteOnly));
    f4.write("notes");
    f4.close();

    const auto scanned = BatchExportMatrix::scanDirectory(root, true);
    QCOMPARE(scanned.size(), 3);

    bool foundRoot = false;
    bool foundSeason1 = false;
    bool foundSeason2Bonus = false;

    for (const auto &item : scanned) {
        if (item.fileName == QStringLiteral("root_clip.mp4")) {
            foundRoot = true;
            QVERIFY(item.relativeSubDir.isEmpty());
        } else if (item.fileName == QStringLiteral("ep01.mkv")) {
            foundSeason1 = true;
            QCOMPARE(item.relativeSubDir, QStringLiteral("Season1"));
        } else if (item.fileName == QStringLiteral("extra.mp3")) {
            foundSeason2Bonus = true;
            QCOMPARE(item.relativeSubDir, QStringLiteral("Season2/Bonus"));
        }
    }

    QVERIFY(foundRoot);
    QVERIFY(foundSeason1);
    QVERIFY(foundSeason2Bonus);
}

void BatchProcessingTest::queuePauseAndResume()
{
    JobQueue queue(nullptr, nullptr);
    queue.setMaxConcurrent(2);
    QVERIFY(!queue.isPaused());

    queue.setPaused(true);
    QVERIFY(queue.isPaused());

    auto *j1 = new Job(&queue);
    j1->setName(QStringLiteral("Job1"));
    j1->setCommand(QStringLiteral("ffmpeg"), {QStringLiteral("-version")});

    queue.addJob(j1);
    // Because queue is paused, job remains Pending even though maxConcurrent=2
    QCOMPARE(j1->status(), JobStatus::Pending);

    queue.setPaused(false);
    QVERIFY(!queue.isPaused());
}

void BatchProcessingTest::queueCancelAllAndRetryAllFailed()
{
    JobQueue queue(nullptr, nullptr);
    queue.setPaused(true); // Keep pending

    auto *j1 = new Job(&queue);
    j1->setName(QStringLiteral("J1"));
    j1->setCommand(QStringLiteral("ffmpeg"), {QStringLiteral("-version")});

    auto *j2 = new Job(&queue);
    j2->setName(QStringLiteral("J2"));
    j2->setCommand(QStringLiteral("ffmpeg"), {QStringLiteral("-version")});

    queue.addJob(j1);
    queue.addJob(j2);

    QCOMPARE(queue.pendingJobCount(), 2);

    queue.cancelAll();
    QCOMPARE(j1->status(), JobStatus::Cancelled);
    QCOMPARE(j2->status(), JobStatus::Cancelled);
    QCOMPARE(queue.pendingJobCount(), 0);

    queue.retryAllFailed();
    QCOMPARE(j1->status(), JobStatus::Pending);
    QCOMPARE(j2->status(), JobStatus::Pending);
    QCOMPARE(queue.pendingJobCount(), 2);
}

void BatchProcessingTest::queueJobReordering()
{
    JobQueue queue(nullptr, nullptr);
    queue.setPaused(true);

    auto *j1 = new Job(&queue);
    j1->setName(QStringLiteral("First"));
    j1->setCommand(QStringLiteral("ffmpeg"), {QStringLiteral("-version")});

    auto *j2 = new Job(&queue);
    j2->setName(QStringLiteral("Second"));
    j2->setCommand(QStringLiteral("ffmpeg"), {QStringLiteral("-version")});

    auto *j3 = new Job(&queue);
    j3->setName(QStringLiteral("Third"));
    j3->setCommand(QStringLiteral("ffmpeg"), {QStringLiteral("-version")});

    queue.addJob(j1);
    queue.addJob(j2);
    queue.addJob(j3);

    // Initial order: j1, j2, j3
    QCOMPARE(queue.model()->getJob(0)->name(), QStringLiteral("First"));
    QCOMPARE(queue.model()->getJob(1)->name(), QStringLiteral("Second"));
    QCOMPARE(queue.model()->getJob(2)->name(), QStringLiteral("Third"));

    // Move Second up
    QVERIFY(queue.moveJobUp(j2->id().toString()));
    QCOMPARE(queue.model()->getJob(0)->name(), QStringLiteral("Second"));
    QCOMPARE(queue.model()->getJob(1)->name(), QStringLiteral("First"));
    QCOMPARE(queue.model()->getJob(2)->name(), QStringLiteral("Third"));

    // Move Third to top
    QVERIFY(queue.moveJobToTop(j3->id().toString()));
    QCOMPARE(queue.model()->getJob(0)->name(), QStringLiteral("Third"));
    QCOMPARE(queue.model()->getJob(1)->name(), QStringLiteral("Second"));
    QCOMPARE(queue.model()->getJob(2)->name(), QStringLiteral("First"));

    // Move Third down
    QVERIFY(queue.moveJobDown(j3->id().toString()));
    QCOMPARE(queue.model()->getJob(0)->name(), QStringLiteral("Second"));
    QCOMPARE(queue.model()->getJob(1)->name(), QStringLiteral("Third"));
    QCOMPARE(queue.model()->getJob(2)->name(), QStringLiteral("First"));
}

void BatchProcessingTest::queuePriorityScheduling()
{
    FFmpegLocator locator;
    locator.waitForDetection();
    if (!locator.isValid()) {
        QSKIP("ffmpeg not available");
    }

    JobQueue queue(&locator, nullptr);
    queue.setPaused(true); // Don't start yet
    queue.setMaxConcurrent(1);

    auto *jLow = new Job(&queue);
    jLow->setName(QStringLiteral("LowJob"));
    jLow->setPriority(JobPriority::Low);
    jLow->setCommand(locator.ffmpegPath(), {QStringLiteral("-version")});

    auto *jUrgent = new Job(&queue);
    jUrgent->setName(QStringLiteral("UrgentJob"));
    jUrgent->setPriority(JobPriority::Urgent);
    jUrgent->setCommand(locator.ffmpegPath(), {QStringLiteral("-version")});

    auto *jNormal = new Job(&queue);
    jNormal->setName(QStringLiteral("NormalJob"));
    jNormal->setPriority(JobPriority::Normal);
    jNormal->setCommand(locator.ffmpegPath(), {QStringLiteral("-version")});

    // Add in order: Low, Urgent, Normal
    queue.addJob(jLow);
    queue.addJob(jUrgent);
    queue.addJob(jNormal);

    // Unpause queue. Concurrency limit is 1. The Urgent job should be picked first!
    queue.setPaused(false);

    QCOMPARE(jUrgent->status(), JobStatus::Running);
    QCOMPARE(jLow->status(), JobStatus::Pending);
    QCOMPARE(jNormal->status(), JobStatus::Pending);

    queue.cancelAll();
}

void BatchProcessingTest::folderWatcherStabilityDetection()
{
    QTemporaryDir watchDir;
    QTemporaryDir outDir;
    QVERIFY(watchDir.isValid());
    QVERIFY(outDir.isValid());

    FolderWatcherService watcher;
    watcher.setCheckIntervalMs(500);

    QSignalSpy detectedSpy(&watcher, &FolderWatcherService::fileDetected);
    QSignalSpy readySpy(&watcher, &FolderWatcherService::fileReady);

    QVERIFY(watcher.startWatching(watchDir.path(), outDir.path()));
    QVERIFY(watcher.isWatching());

    // Create a new video file in the watch directory
    const QString filePath = watchDir.path() + QStringLiteral("/incoming_test.mp4");
    QFile f(filePath);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("0123456789ABCDEF");
    f.close();

    // Trigger immediate scan
    watcher.scanDirectoryNow();
    QVERIFY(detectedSpy.count() >= 1);

    // Wait for stability timer to trigger fileReady
    QVERIFY(readySpy.wait(3000));
    QCOMPARE(readySpy.count(), 1);
    QCOMPARE(readySpy.first().at(0).toString(), filePath);

    watcher.stopWatching();
    QVERIFY(!watcher.isWatching());
}

QTEST_MAIN(BatchProcessingTest)
#include "tst_BatchProcessing.moc"
