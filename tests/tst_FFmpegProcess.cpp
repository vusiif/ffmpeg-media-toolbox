#include <QtTest>

#include <QSignalSpy>
#include <QStandardPaths>

#include "ffmpeg/FFmpegProcess.h"

class FFmpegProcessTest : public QObject
{
    Q_OBJECT

private slots:
    void progressArgumentsAreOrdered();
    void buildInvocationArgumentsPrependsProgress();

    void lineWithoutSeparatorIsIgnored();
    void frameAndFpsAreParsedWithoutFraction();
    void outTimeComputesFractionAgainstDuration();
    void outTimeIsClampedToOne();
    void outTimeWithoutDurationStoresValueOnly();
    void speedStripsUnitSuffix();
    void progressEndForcesFullFraction();
    void progressContinueDoesNotTouchFraction();

    void realProcessFinishesOnceOnSuccess();
    void realProcessFinishesOnceOnFailure();
    void cancelStaysCancelledAndFinishesOnce();
};

namespace {
QString ffmpegExecutable()
{
    return QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
}
}

void FFmpegProcessTest::progressArgumentsAreOrdered()
{
    // `-progress` must be immediately followed by its value, and `-nostats`
    // must come from the same prefix (AI_PLAN §17).
    QCOMPARE(FFmpegProcess::progressArguments(),
             QStringList({QStringLiteral("-progress"), QStringLiteral("pipe:1"),
                          QStringLiteral("-nostats")}));
}

void FFmpegProcessTest::buildInvocationArgumentsPrependsProgress()
{
    QCOMPARE(FFmpegProcess::buildInvocationArguments({QStringLiteral("-i"),
                                                      QStringLiteral("in.mp4")}),
             QStringList({QStringLiteral("-progress"), QStringLiteral("pipe:1"),
                          QStringLiteral("-nostats"), QStringLiteral("-i"),
                          QStringLiteral("in.mp4")}));
    QVERIFY(FFmpegProcess::buildInvocationArguments({}).size() == 3);
}


void FFmpegProcessTest::lineWithoutSeparatorIsIgnored()
{
    FFmpegProgress progress;
    double fraction = 0.5;
    QVERIFY(!FFmpegProcess::parseProgressLine(QStringLiteral("garbage line"), progress, 10.0,
                                              fraction));
    QCOMPARE(fraction, 0.5);
    QCOMPARE(progress.frame, 0);
}

void FFmpegProcessTest::frameAndFpsAreParsedWithoutFraction()
{
    FFmpegProgress progress;
    double fraction = 0.0;

    QVERIFY(!FFmpegProcess::parseProgressLine(QStringLiteral("frame=123"), progress, 10.0,
                                              fraction));
    QCOMPARE(progress.frame, 123);
    QCOMPARE(fraction, 0.0);

    QVERIFY(!FFmpegProcess::parseProgressLine(QStringLiteral("fps=29.97"), progress, 10.0,
                                              fraction));
    QVERIFY(qAbs(progress.fps - 29.97) < 0.001);
}

void FFmpegProcessTest::outTimeComputesFractionAgainstDuration()
{
    FFmpegProgress progress;
    double fraction = 0.0;

    QVERIFY(FFmpegProcess::parseProgressLine(QStringLiteral("out_time_us=5000000"), progress, 10.0,
                                             fraction));
    QCOMPARE(progress.outTimeUs, 5000000);
    QVERIFY(qAbs(fraction - 0.5) < 0.0001);
    QVERIFY(qAbs(progress.percent - 50.0) < 0.001);
}

void FFmpegProcessTest::outTimeIsClampedToOne()
{
    FFmpegProgress progress;
    double fraction = 0.0;

    QVERIFY(FFmpegProcess::parseProgressLine(QStringLiteral("out_time_us=99000000"), progress, 2.0,
                                             fraction));
    QCOMPARE(fraction, 1.0);
    QCOMPARE(progress.percent, 100.0);
}

void FFmpegProcessTest::outTimeWithoutDurationStoresValueOnly()
{
    FFmpegProgress progress;
    double fraction = 0.25;

    QVERIFY(!FFmpegProcess::parseProgressLine(QStringLiteral("out_time_us=123456"), progress, 0,
                                              fraction));
    QCOMPARE(progress.outTimeUs, 123456);
    QCOMPARE(fraction, 0.25);
}

void FFmpegProcessTest::speedStripsUnitSuffix()
{
    FFmpegProgress progress;
    double fraction = 0.0;

    QVERIFY(!FFmpegProcess::parseProgressLine(QStringLiteral("speed=2.5x"), progress, 10.0,
                                              fraction));
    QVERIFY(qAbs(progress.speed - 2.5) < 0.001);

    QVERIFY(!FFmpegProcess::parseProgressLine(QStringLiteral("speed=N/A"), progress, 10.0,
                                              fraction));
    QCOMPARE(progress.speed, 0.0);
}

void FFmpegProcessTest::progressEndForcesFullFraction()
{
    FFmpegProgress progress;
    double fraction = 0.1;

    QVERIFY(FFmpegProcess::parseProgressLine(QStringLiteral("progress=end"), progress, 0,
                                             fraction));
    QCOMPARE(fraction, 1.0);
}

void FFmpegProcessTest::progressContinueDoesNotTouchFraction()
{
    FFmpegProgress progress;
    double fraction = 0.1;

    QVERIFY(!FFmpegProcess::parseProgressLine(QStringLiteral("progress=continue"), progress, 10.0,
                                              fraction));
    QCOMPARE(fraction, 0.1);
}

void FFmpegProcessTest::realProcessFinishesOnceOnSuccess()
{
    const QString ffmpeg = ffmpegExecutable();
    if (ffmpeg.isEmpty()) {
        QSKIP("ffmpeg not available on PATH");
    }

    FFmpegProcess process;
    QSignalSpy spy(&process, &FFmpegProcess::finished);

    QVERIFY(process.start(ffmpeg,
                          {QStringLiteral("-f"), QStringLiteral("lavfi"),
                           QStringLiteral("-i"), QStringLiteral("testsrc=duration=0.2:size=64x64:rate=10"),
                           QStringLiteral("-f"), QStringLiteral("null"), QStringLiteral("-")},
                          0.2));

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 15000);
    QCOMPARE(spy.at(0).at(0).toBool(), true);
    QCOMPARE(process.state(), FFmpegProcess::State::Finished);

    QTest::qWait(200);
    QCOMPARE(spy.count(), 1);
}

void FFmpegProcessTest::realProcessFinishesOnceOnFailure()
{
    const QString ffmpeg = ffmpegExecutable();
    if (ffmpeg.isEmpty()) {
        QSKIP("ffmpeg not available on PATH");
    }

    FFmpegProcess process;
    QSignalSpy spy(&process, &FFmpegProcess::finished);

    QVERIFY(process.start(ffmpeg,
                          {QStringLiteral("-i"), QStringLiteral("definitely_missing_12345.mkv"),
                           QStringLiteral("-f"), QStringLiteral("null"), QStringLiteral("-")},
                          0));

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 15000);
    QCOMPARE(spy.at(0).at(0).toBool(), false);
    QCOMPARE(process.state(), FFmpegProcess::State::Failed);

    QTest::qWait(200);
    QCOMPARE(spy.count(), 1);
}

void FFmpegProcessTest::cancelStaysCancelledAndFinishesOnce()
{
    const QString ffmpeg = ffmpegExecutable();
    if (ffmpeg.isEmpty()) {
        QSKIP("ffmpeg not available on PATH");
    }

    FFmpegProcess process;
    QSignalSpy spy(&process, &FFmpegProcess::finished);

    QVERIFY(process.start(ffmpeg,
                          {QStringLiteral("-f"), QStringLiteral("lavfi"),
                           QStringLiteral("-i"), QStringLiteral("testsrc=duration=120:size=128x128:rate=10"),
                           QStringLiteral("-f"), QStringLiteral("null"), QStringLiteral("-")},
                          120.0));
    QCOMPARE(process.state(), FFmpegProcess::State::Running);

    process.cancel();
    QCOMPARE(process.state(), FFmpegProcess::State::Cancelled);

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 15000);
    QCOMPARE(spy.at(0).at(0).toBool(), false);
    // Cancellation must survive terminate/kill/finished.
    QCOMPARE(process.state(), FFmpegProcess::State::Cancelled);

    QTest::qWait(300);
    QCOMPARE(spy.count(), 1);
}

QTEST_GUILESS_MAIN(FFmpegProcessTest)
#include "tst_FFmpegProcess.moc"
