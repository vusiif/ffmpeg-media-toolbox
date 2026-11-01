#include <QtTest>

#include "jobs/Job.h"

class JobTest : public QObject
{
    Q_OBJECT

private slots:
    void initialState();
    void statusChangeEmitsAndStampsStart();
    void terminalStatusStampsEnd();
    void repeatedStatusIsIgnored();
    void progressChangeEmits();
    void nearEqualProgressIsIgnored();
    void commandDisplayQuotesArgsWithSpaces();
    void logsAreAppendedAndCapped();
    void serializationRoundTrip();
    void deserializationConvertsRunningToCancelled();
};

void JobTest::initialState()
{
    Job job;
    QCOMPARE(job.status(), JobStatus::Pending);
    QCOMPARE(job.progress(), 0.0);
    QVERIFY(job.name().isEmpty());
    QVERIFY(job.startTime().isNull());
    QVERIFY(job.endTime().isNull());
    QVERIFY(job.inputFiles().isEmpty());
    QVERIFY(job.outputFile().isEmpty());
}

void JobTest::statusChangeEmitsAndStampsStart()
{
    Job job;
    QSignalSpy spy(&job, &Job::statusChanged);

    job.setStatus(JobStatus::Running);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(job.status(), JobStatus::Running);
    QVERIFY(!job.startTime().isNull());
    QVERIFY(job.endTime().isNull());

    job.setStatus(JobStatus::Completed);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(job.status(), JobStatus::Completed);
}

void JobTest::terminalStatusStampsEnd()
{
    const QList<JobStatus> terminals = {JobStatus::Completed, JobStatus::Failed,
                                        JobStatus::Cancelled};
    for (JobStatus terminal : terminals) {
        Job job;
        job.setStatus(JobStatus::Running);
        job.setStatus(terminal);
        QVERIFY(!job.endTime().isNull());
    }
}

void JobTest::repeatedStatusIsIgnored()
{
    Job job;
    QSignalSpy spy(&job, &Job::statusChanged);

    job.setStatus(JobStatus::Running);
    job.setStatus(JobStatus::Running);
    QCOMPARE(spy.count(), 1);
}

void JobTest::progressChangeEmits()
{
    Job job;
    QSignalSpy spy(&job, &Job::progressChanged);

    job.setProgress(0.5);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(job.progress(), 0.5);
}

void JobTest::nearEqualProgressIsIgnored()
{
    Job job;
    job.setProgress(0.5);

    QSignalSpy spy(&job, &Job::progressChanged);
    job.setProgress(0.5);
    QCOMPARE(spy.count(), 0);
}

void JobTest::commandDisplayQuotesArgsWithSpaces()
{
    Job job;
    QSignalSpy spy(&job, &Job::commandChanged);

    job.setCommand(QStringLiteral("ffmpeg"),
                   {QStringLiteral("-i"), QStringLiteral("my file.mp4"),
                    QStringLiteral("out.mp4")});

    QCOMPARE(spy.count(), 1);
    QCOMPARE(job.commandProgram(), QStringLiteral("ffmpeg"));
    QCOMPARE(job.commandArguments().size(), 3);
    // Preview mirrors the real invocation, progress plumbing included.
    QCOMPARE(job.commandDisplayString(),
             QStringLiteral("ffmpeg -progress pipe:1 -nostats -i \"my file.mp4\" out.mp4"));
}

void JobTest::logsAreAppendedAndCapped()
{
    Job job;
    job.addLog(QStringLiteral("first"));
    QCOMPARE(job.logs().size(), 1);
    QCOMPARE(job.logs().first().message, QStringLiteral("first"));

    for (int i = 0; i < 10000; ++i) {
        job.addLog(QStringLiteral("msg"));
    }
    QCOMPARE(job.logs().size(), 10000);
    QCOMPARE(job.logs().first().message, QStringLiteral("msg"));
}

void JobTest::serializationRoundTrip()
{
    Job original;
    original.setName(QStringLiteral("Test Task"));
    original.setType(JobType::Conversion);
    original.setStatus(JobStatus::Completed);
    original.setProgress(1.0);
    original.setTotalDuration(120.5);
    original.setInputFiles({QStringLiteral("in1.mp4"), QStringLiteral("in2.mp4")});
    original.setOutputFile(QStringLiteral("out.mp4"));
    original.setCommand(QStringLiteral("ffmpeg"), {QStringLiteral("-i"), QStringLiteral("in1.mp4"), QStringLiteral("out.mp4")});
    original.addLog(QStringLiteral("Log message 1"));
    original.addLog(QStringLiteral("Log message 2"));

    const QJsonObject json = original.toJson();
    Job *restored = Job::fromJson(json);
    QVERIFY(restored != nullptr);

    QCOMPARE(restored->id(), original.id());
    QCOMPARE(restored->name(), QStringLiteral("Test Task"));
    QCOMPARE(restored->type(), JobType::Conversion);
    QCOMPARE(restored->status(), JobStatus::Completed);
    QCOMPARE(restored->progress(), 1.0);
    QCOMPARE(restored->totalDuration(), 120.5);
    QCOMPARE(restored->inputFiles(), QStringList({QStringLiteral("in1.mp4"), QStringLiteral("in2.mp4")}));
    QCOMPARE(restored->outputFile(), QStringLiteral("out.mp4"));
    QCOMPARE(restored->commandProgram(), QStringLiteral("ffmpeg"));
    QCOMPARE(restored->commandArguments(), QStringList({QStringLiteral("-i"), QStringLiteral("in1.mp4"), QStringLiteral("out.mp4")}));
    QCOMPARE(restored->logs().size(), 2);
    QCOMPARE(restored->logs().at(0).message, QStringLiteral("Log message 1"));
    QCOMPARE(restored->logs().at(1).message, QStringLiteral("Log message 2"));

    delete restored;
}

void JobTest::deserializationConvertsRunningToCancelled()
{
    Job runningJob;
    runningJob.setName(QStringLiteral("Interrupted Job"));
    runningJob.setStatus(JobStatus::Running);

    const QJsonObject json = runningJob.toJson();
    Job *restored = Job::fromJson(json);
    QVERIFY(restored != nullptr);

    QCOMPARE(restored->name(), QStringLiteral("Interrupted Job"));
    QCOMPARE(restored->status(), JobStatus::Cancelled);
    QVERIFY(!restored->errorMessage().isEmpty());

    delete restored;
}

QTEST_GUILESS_MAIN(JobTest)
#include "tst_Job.moc"
