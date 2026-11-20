#include <QtTest>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QProcess>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <memory>

#include "cut/CutExportBuilder.h"
#include "ffmpeg/FFmpegLocator.h"
#include "jobs/JobQueue.h"
#include "media/MediaProbeService.h"

class CutExportTest : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void smartCutAcrossGops();
    void mergeRangesAndConcurrentExports();
    void failureStopsFollowingSteps();
    void cancellationRetryAndRecovery();
    void outputCollisionAndInvalidWorkspace();
    void conservativeSmartPlans();
    void bFramesAndWebmUseAccurateEncoding();
    void immediateRetryAndQueueTeardown();
    void cancelDuringFinalEncodeCanRetry();
private:
    QByteArray run(const QString &program, const QStringList &args, int *exit = nullptr);
    void verifyOutput(const QString &path, double duration, int frames);
    void verifyFrames(const QString &path, const QVector<QPair<double, double>> &times);
    std::unique_ptr<QTemporaryDir> m_dir;
    std::unique_ptr<FFmpegLocator> m_locator;
    MediaFile m_media;
    QVector<double> m_keyframes;
};

QByteArray CutExportTest::run(const QString &program, const QStringList &args, int *exit)
{
    QProcess process;
    process.start(program, args);
    if (!process.waitForFinished(30000)) {
        process.kill();
        process.waitForFinished();
        QTest::qFail("Fixture process timed out", __FILE__, __LINE__);
        if (exit) *exit = -1;
        return {};
    }
    if (exit) *exit = process.exitCode();
    if (process.exitCode() != 0 && !exit) {
        QTest::qFail(process.readAllStandardError().constData(), __FILE__, __LINE__);
    }
    return process.readAllStandardOutput();
}

void CutExportTest::initTestCase()
{
    if (QStandardPaths::findExecutable(QStringLiteral("ffmpeg")).isEmpty() ||
        QStandardPaths::findExecutable(QStringLiteral("ffprobe")).isEmpty()) {
        QSKIP("Real cut export tests require ffmpeg and ffprobe");
    }
    const QString root = QDir::current().filePath(QStringLiteral(".sandbox-test"));
    QVERIFY(QDir().mkpath(root));
    m_dir = std::make_unique<QTemporaryDir>(root + QStringLiteral("/cut-export-XXXXXX"));
    QVERIFY(m_dir->isValid());
    m_locator = std::make_unique<FFmpegLocator>();
    QVERIFY(m_locator->waitForDetection());
    const QString input = m_dir->filePath(QStringLiteral("源视频 'quoted clip.mp4"));
    run(m_locator->ffmpegPath(), {"-v", "error", "-f", "lavfi", "-i", "testsrc2=size=128x96:rate=25:duration=5",
        "-f", "lavfi", "-i", "sine=frequency=440:sample_rate=48000:duration=5",
        "-c:v", "libx264", "-preset", "veryfast", "-pix_fmt", "yuv420p", "-g", "25", "-keyint_min", "25",
        "-sc_threshold", "0", "-bf", "0", "-c:a", "aac", input});
    MediaProbeService probe(m_locator.get());
    m_media = probe.probe(input);
    QVERIFY(m_media.hasVideo());
    QVERIFY(m_media.hasAudio());
    QCOMPARE(m_media.primaryVideo().hasBFrames, 0);
    QVERIFY(m_media.primaryVideo().constantFrameRate);
    const QStringList keyframeCommand = SmartCutEngine::buildKeyframeProbeCommand(m_locator->ffprobePath(), input);
    m_keyframes = SmartCutEngine::parseKeyframeOutput(run(keyframeCommand.first(), keyframeCommand.mid(1)));
    QCOMPARE(m_keyframes, QVector<double>({0, 1, 2, 3, 4}));
}

void CutExportTest::verifyOutput(const QString &path, double duration, int frames)
{
    QVERIFY(QFileInfo::exists(path));
    MediaProbeService probe(m_locator.get());
    const MediaFile result = probe.probe(path);
    QVERIFY(qAbs(result.duration - duration) < 0.10);
    QVERIFY(result.hasVideo());
    QVERIFY(result.hasAudio());
    const QByteArray json = run(m_locator->ffprobePath(), {"-v", "error", "-count_frames", "-select_streams", "v:0",
        "-show_entries", "stream=nb_read_frames", "-of", "json", path});
    const auto streams = QJsonDocument::fromJson(json).object().value("streams").toArray();
    QVERIFY(!streams.isEmpty());
    QCOMPARE(streams.first().toObject().value("nb_read_frames").toString().toInt(), frames);
    QProcess decode;
    decode.start(m_locator->ffmpegPath(), {"-v", "error", "-i", path, "-f", "null", "-"});
    QVERIFY(decode.waitForFinished(30000));
    QCOMPARE(decode.exitCode(), 0);
    QVERIFY2(decode.readAllStandardError().isEmpty(), "Spliced output must decode without errors");
}

void CutExportTest::smartCutAcrossGops()
{
    JobQueue queue(m_locator.get(), nullptr, nullptr, QString());
    const QString output = m_dir->filePath("smart.mp4");
    QString error;
    Job *job = CutExportBuilder::createJob(m_locator->ffmpegPath(), m_media, output,
        {{0.32, 3.68, {}}}, CutMode::SmartCut, m_keyframes, &error);
    QVERIFY2(job, qPrintable(error));
    QVERIFY(job->commandSteps().size() > 1);
    QSignalSpy finished(&queue, &JobQueue::jobFinished);
    queue.addJob(job);
    const QString workspace = job->executionWorkspace();
    QVERIFY(QDir(workspace).exists());
    QTRY_VERIFY_WITH_TIMEOUT(job->status() == JobStatus::Completed || job->status() == JobStatus::Failed, 30000);
    QVERIFY2(job->status() == JobStatus::Completed, qPrintable(job->errorMessage()));
    QCOMPARE(finished.size(), 1);
    QVERIFY(!QDir(workspace).exists());
    verifyOutput(output, 3.36, 84);
    verifyFrames(output, {{0, 0.32}, {0.64, 0.96}, {0.68, 1.0},
                          {2.64, 2.96}, {2.68, 3.0}, {3.32, 3.64}});
}

void CutExportTest::verifyFrames(const QString &path, const QVector<QPair<double, double>> &times)
{
    const auto frame = [&](const QString &file, double time) {
        return run(m_locator->ffmpegPath(), {"-v", "error", "-ss", QString::number(time, 'f', 6),
            "-i", file, "-frames:v", "1", "-an", "-pix_fmt", "gray", "-f", "rawvideo", "pipe:1"});
    };
    for (const auto &[outputTime, sourceTime] : times) {
        const QByteArray actual = frame(path, outputTime);
        const QByteArray reference = frame(m_media.path, sourceTime);
        QCOMPARE(actual.size(), reference.size());
        QVERIFY(!actual.isEmpty());
        double squaredError = 0;
        for (qsizetype i = 0; i < actual.size(); ++i) {
            const int delta = static_cast<unsigned char>(actual[i]) - static_cast<unsigned char>(reference[i]);
            squaredError += delta * delta;
        }
        const double mse = squaredError / actual.size();
        QVERIFY2(mse < 10, qPrintable(QStringLiteral("Frame at %1 does not match source %2 (MSE %3)")
                                          .arg(outputTime).arg(sourceTime).arg(mse)));
    }
}

void CutExportTest::mergeRangesAndConcurrentExports()
{
    const QString sentinel = m_dir->filePath("源视频 'quoted clip_seg_01.mp4");
    QFile old(sentinel);
    QVERIFY(old.open(QIODevice::WriteOnly));
    old.write("keep original"); old.close();
    JobQueue queue(m_locator.get(), nullptr, nullptr, QString());
    queue.setMaxConcurrent(2);
    const QVector<CutSegment> ranges{{0.4, 1.2, {}}, {2.2, 3.4, {}}};
    Job *first = CutExportBuilder::createJob(m_locator->ffmpegPath(), m_media,
        m_dir->filePath("merged1.mp4"), ranges, CutMode::AccurateReencode, {});
    Job *second = CutExportBuilder::createJob(m_locator->ffmpegPath(), m_media,
        m_dir->filePath("merged2.mp4"), ranges, CutMode::SmartCut, {0, 1, 2, 3, 4});
    QVERIFY(first); QVERIFY(second);
    queue.addJobs({first, second});
    QVERIFY(first->executionWorkspace() != second->executionWorkspace());
    QTRY_VERIFY_WITH_TIMEOUT(!queue.hasRunningJobs(), 30000);
    QVERIFY2(first->status() == JobStatus::Completed, qPrintable(first->errorMessage()));
    QVERIFY2(second->status() == JobStatus::Completed, qPrintable(second->errorMessage()));
    verifyOutput(first->outputFile(), 2.0, 50);
    verifyOutput(second->outputFile(), 2.0, 50);
    verifyFrames(first->outputFile(), {{0, 0.4}, {0.76, 1.16}, {0.8, 2.2}, {1.96, 3.36}});
    QVERIFY(old.open(QIODevice::ReadOnly));
    QCOMPARE(old.readAll(), QByteArray("keep original"));
}

void CutExportTest::failureStopsFollowingSteps()
{
    JobQueue queue(m_locator.get(), nullptr, nullptr, QString());
    auto *job = new Job();
    const QString output = m_dir->filePath("must-not-exist.wav");
    job->setCommandSequence(m_locator->ffmpegPath(), {
        {{"-f", "lavfi", "-i", "no_such_filter", "-f", "null", "-"}, 1},
        {{"-f", "lavfi", "-i", "sine=duration=0.1", output}, 0.1}});
    QSignalSpy done(&queue, &JobQueue::jobFinished);
    queue.addJob(job);
    QTRY_VERIFY_WITH_TIMEOUT(job->status() == JobStatus::Failed, 10000);
    QCOMPARE(done.size(), 1);
    QVERIFY(!QFileInfo::exists(output));
    QVERIFY(!QDir(job->executionWorkspace()).exists());
}

void CutExportTest::cancellationRetryAndRecovery()
{
    const QString storage = m_dir->filePath("sequence-jobs.json");
    const QString output = m_dir->filePath("retry.wav");
    QString firstWorkspace;
    QString id;
    {
        JobQueue queue(m_locator.get(), nullptr, nullptr, storage);
        auto *job = new Job();
        const QString intermediate = Job::workspaceToken() + "/audio.wav";
        const QString staged = Job::workspaceToken() + "/published.wav";
        job->setCommandSequence(m_locator->ffmpegPath(), {
            {{"-n", "-re", "-f", "lavfi", "-i", "sine=duration=1", intermediate}, 1},
            {{"-n", "-i", intermediate, "-c", "copy", staged}, 1}}, {{"marker.txt", "workspace recipe"}});
        job->setOutputFile(output);
        job->setStagedOutputFile("published.wav");
        queue.addJob(job);
        id = job->id().toString();
        firstWorkspace = job->executionWorkspace();
        queue.cancelJob(id);
        QTRY_VERIFY_WITH_TIMEOUT(!queue.hasRunningJobs(), 10000);
        QCOMPARE(job->status(), JobStatus::Cancelled);
        QVERIFY(!QFileInfo::exists(output));
        QVERIFY(!QDir(firstWorkspace).exists());
    }
    JobQueue restored(m_locator.get(), nullptr, nullptr, storage);
    QCOMPARE(restored.model()->count(), 1);
    Job *job = restored.model()->getJob(0);
    QCOMPARE(job->commandSteps().size(), 2);
    QCOMPARE(job->stagedOutputFile(), QStringLiteral("published.wav"));
    QCOMPARE(job->workspaceFiles().value("marker.txt"), QStringLiteral("workspace recipe"));
    restored.retryJob(id);
    QVERIFY(job->executionWorkspace() != firstWorkspace);
    QTRY_VERIFY_WITH_TIMEOUT(!restored.hasRunningJobs(), 15000);
    QVERIFY2(job->status() == JobStatus::Completed, qPrintable(job->errorMessage()));
    QVERIFY(QFileInfo::exists(output));
    QVERIFY(!QDir(job->executionWorkspace()).exists());
}

void CutExportTest::outputCollisionAndInvalidWorkspace()
{
    const QString output = m_dir->filePath("collision.mp4");
    std::unique_ptr<Job> planned(CutExportBuilder::createJob(m_locator->ffmpegPath(), m_media, output,
        {{0, 1, {}}}, CutMode::AccurateReencode, {}));
    QVERIFY(planned);
    QFile sentinel(output);
    QVERIFY(sentinel.open(QIODevice::WriteOnly)); sentinel.write("existing file"); sentinel.close();
    JobQueue queue(m_locator.get(), nullptr, nullptr, QString());
    Job *job = planned.release();
    queue.addJob(job);
    QTRY_VERIFY_WITH_TIMEOUT(!queue.hasRunningJobs(), 10000);
    QCOMPARE(job->status(), JobStatus::Failed);
    QVERIFY(sentinel.open(QIODevice::ReadOnly));
    QCOMPARE(sentinel.readAll(), QByteArray("existing file"));
    QVERIFY(!CutExportBuilder::createJob(m_locator->ffmpegPath(), m_media, output,
        {{0, 1, {}}}, CutMode::AccurateReencode, {}));

    auto *invalid = new Job();
    invalid->setCommandSequence(m_locator->ffmpegPath(), {{{"-version"}, 0}}, {{"../escape.txt", "forbidden"}});
    queue.addJob(invalid);
    QCOMPARE(invalid->status(), JobStatus::Failed);
    QVERIFY(!invalid->errorMessage().isEmpty());
}

void CutExportTest::conservativeSmartPlans()
{
    QCOMPARE(SmartCutEngine::parseKeyframeOutput("0.0,H.26[45] User Data\n1.0\nN/A\n-2.0\ninf\n"),
             QVector<double>({0, 1}));
    auto plan = SmartCutEngine::planSmartCut("in.mp4", "out.mp4", 1.02, 3.02, {0, 1, 2, 3, 4});
    QVERIFY(plan.needsConcat);
    QCOMPARE(plan.chunks.first().type, SmartChunk::ChunkType::Reencode);
    plan = SmartCutEngine::planSmartCut("in.mp4", "out.mp4", 1.0, 3.0, {});
    QCOMPARE(plan.chunks.first().type, SmartChunk::ChunkType::Reencode);
    MediaFile bFrames = m_media;
    bFrames.videoStreams[0].hasBFrames = 2;
    std::unique_ptr<Job> job(CutExportBuilder::createJob(m_locator->ffmpegPath(), bFrames,
        m_dir->filePath("bframes.mp4"), {{0.32, 3.68, {}}}, CutMode::SmartCut, {0, 1, 2, 3, 4}));
    QVERIFY(job);
    QCOMPARE(job->commandSteps().size(), 1);
    QVERIFY(!job->logs().isEmpty());
}

void CutExportTest::bFramesAndWebmUseAccurateEncoding()
{
    const QString input = m_dir->filePath("bframes-input.mp4");
    run(m_locator->ffmpegPath(), {"-v", "error", "-i", m_media.path,
        "-c:v", "libx264", "-bf", "2", "-g", "25", "-c:a", "copy", input});
    MediaProbeService probe(m_locator.get());
    const auto media = probe.probe(input);
    QVERIFY(media.primaryVideo().hasBFrames > 0);
    JobQueue queue(m_locator.get(), nullptr, nullptr, QString());
    for (const QString &extension : {QStringLiteral("mp4"), QStringLiteral("webm")}) {
        Job *job = CutExportBuilder::createJob(m_locator->ffmpegPath(), media,
            m_dir->filePath("accurate-fallback." + extension), {{0.32, 3.68, {}}},
            CutMode::SmartCut, {0, 1, 2, 3, 4});
        QVERIFY(job);
        QCOMPARE(job->commandSteps().size(), 1);
        QVERIFY(!job->name().isEmpty());
        queue.addJob(job);
        QTRY_VERIFY_WITH_TIMEOUT(!queue.hasRunningJobs(), 30000);
        QVERIFY2(job->status() == JobStatus::Completed, qPrintable(job->errorMessage()));
        verifyOutput(job->outputFile(), 3.36, 84);
    }
}

void CutExportTest::immediateRetryAndQueueTeardown()
{
    const auto make = [&](const QString &output) {
        auto *job = new Job();
        const QString part = Job::workspaceToken() + "/tone.wav";
        job->setCommandSequence(m_locator->ffmpegPath(), {
            {{"-n", "-re", "-f", "lavfi", "-i", "sine=duration=1", part}, 1},
            {{"-n", "-i", part, "-c", "copy", output}, 1}});
        return job;
    };
    QString workspace;
    const QString immediate = m_dir->filePath("immediate-retry.wav");
    {
        JobQueue queue(m_locator.get(), nullptr, nullptr, QString());
        QSignalSpy done(&queue, &JobQueue::allJobsFinished);
        Job *job = make(immediate);
        queue.addJob(job);
        workspace = job->executionWorkspace();
        const QString id = job->id().toString();
        queue.cancelJob(id);
        queue.retryJob(id);
        QTRY_VERIFY_WITH_TIMEOUT(!queue.hasRunningJobs(), 15000);
        QVERIFY2(job->status() == JobStatus::Completed, qPrintable(job->errorMessage()));
        QVERIFY(workspace != job->executionWorkspace());
        QVERIFY(!QDir(workspace).exists());
        QVERIFY(!QDir(job->executionWorkspace()).exists());
        QCOMPARE(done.size(), 1);
        QVERIFY(QFileInfo::exists(immediate));
    }
    const QString abandoned = m_dir->filePath("must-not-publish.wav");
    {
        JobQueue queue(m_locator.get(), nullptr, nullptr, QString());
        Job *job = make(abandoned);
        queue.addJob(job);
        workspace = job->executionWorkspace();
        QVERIFY(QDir(workspace).exists());
    }
    QVERIFY(!QDir(workspace).exists());
    QVERIFY(!QFileInfo::exists(abandoned));
}

void CutExportTest::cancelDuringFinalEncodeCanRetry()
{
    const QString output = m_dir->filePath("cancel-final.mp4");
    JobQueue queue(m_locator.get(), nullptr, nullptr, QString());
    Job *job = CutExportBuilder::createJob(m_locator->ffmpegPath(), m_media, output,
        {{0.32, 3.68, {}}}, CutMode::AccurateReencode, {});
    QVERIFY(job);
    auto steps = job->commandSteps();
    steps[0].arguments.prepend("-re");
    const QString staged = job->stagedOutputFile();
    job->setCommandSequence(job->commandProgram(), steps, job->workspaceFiles());
    job->setStagedOutputFile(staged);
    queue.addJob(job);
    const QString workspace = job->executionWorkspace();
    QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(QDir(workspace).filePath(staged)), 5000);
    QVERIFY(!QFileInfo::exists(output));
    queue.cancelJob(job->id().toString());
    QTRY_VERIFY_WITH_TIMEOUT(!queue.hasRunningJobs(), 10000);
    QCOMPARE(job->status(), JobStatus::Cancelled);
    QVERIFY(!QFileInfo::exists(output));
    QVERIFY(!QDir(workspace).exists());
    queue.retryJob(job->id().toString());
    QTRY_VERIFY_WITH_TIMEOUT(!queue.hasRunningJobs(), 15000);
    QVERIFY2(job->status() == JobStatus::Completed, qPrintable(job->errorMessage()));
    verifyOutput(output, 3.36, 84);
}

QTEST_GUILESS_MAIN(CutExportTest)
#include "tst_CutExport.moc"
