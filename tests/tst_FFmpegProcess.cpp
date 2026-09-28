#include <QtTest>

#include "ffmpeg/FFmpegProcess.h"

class FFmpegProcessTest : public QObject
{
    Q_OBJECT

private slots:
    void lineWithoutSeparatorIsIgnored();
    void frameAndFpsAreParsedWithoutFraction();
    void outTimeComputesFractionAgainstDuration();
    void outTimeIsClampedToOne();
    void outTimeWithoutDurationStoresValueOnly();
    void speedStripsUnitSuffix();
    void progressEndForcesFullFraction();
    void progressContinueDoesNotTouchFraction();
};

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

QTEST_GUILESS_MAIN(FFmpegProcessTest)
#include "tst_FFmpegProcess.moc"
