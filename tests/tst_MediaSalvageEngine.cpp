#include <QTest>
#include <QSignalSpy>
#include "repair/MediaSalvageEngine.h"
#include "ffmpeg/FFmpegLocator.h"

class tst_MediaSalvageEngine : public QObject
{
    Q_OBJECT

private slots:
    void testArgumentsFastRemux();
    void testArgumentsForceTranscode();
    void testSuggestOutputPath();
    void testProgressParsing();
};

void tst_MediaSalvageEngine::testArgumentsFastRemux()
{
    const QString input = QStringLiteral("C:/videos/corrupted.mp4");
    const QString output = QStringLiteral("C:/videos/corrupted_repaired.mp4");
    const QStringList args = MediaSalvageEngine::buildArguments(input, output, MediaSalvageEngine::SalvageMode::FastRemux);

    QVERIFY(args.contains(QStringLiteral("-err_detect")));
    QVERIFY(args.contains(QStringLiteral("ignore_err")));
    QVERIFY(args.contains(QStringLiteral("+genpts+discardcorrupt")));
    QVERIFY(args.contains(QStringLiteral("copy")));
    QVERIFY(args.contains(QStringLiteral("+faststart")));
    QCOMPARE(args.last(), output);
}

void tst_MediaSalvageEngine::testArgumentsForceTranscode()
{
    const QString input = QStringLiteral("C:/videos/broken.avi");
    const QString output = QStringLiteral("C:/videos/broken_salvaged.mp4");
    const QStringList args = MediaSalvageEngine::buildArguments(input, output, MediaSalvageEngine::SalvageMode::ForceTranscode);

    QVERIFY(args.contains(QStringLiteral("-err_detect")));
    QVERIFY(args.contains(QStringLiteral("ignore_err")));
    QVERIFY(args.contains(QStringLiteral("+genpts+discardcorrupt")));
    QVERIFY(args.contains(QStringLiteral("libx264")));
    QVERIFY(args.contains(QStringLiteral("aac")));
    QVERIFY(args.contains(QStringLiteral("+faststart")));
    QCOMPARE(args.last(), output);
}

void tst_MediaSalvageEngine::testSuggestOutputPath()
{
    const QString in1 = QStringLiteral("C:/media/test_corrupt.mov");
    const QString out1 = MediaSalvageEngine::suggestOutputPath(in1, MediaSalvageEngine::SalvageMode::FastRemux);
    QCOMPARE(out1, QStringLiteral("C:/media/test_corrupt_repaired.mov"));

    const QString out2 = MediaSalvageEngine::suggestOutputPath(in1, MediaSalvageEngine::SalvageMode::ForceTranscode);
    QCOMPARE(out2, QStringLiteral("C:/media/test_corrupt_salvaged.mp4"));
}

void tst_MediaSalvageEngine::testProgressParsing()
{
    FFmpegLocator locator;
    MediaSalvageEngine engine(&locator);
    engine.m_totalDuration = 100.0;

    QSignalSpy spy(&engine, &MediaSalvageEngine::progressChanged);

    // 50 seconds in microseconds: 50,000,000 us
    engine.parseProgressLine(QStringLiteral("out_time_us=50000000"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toDouble(), 0.50);

    // end line
    engine.parseProgressLine(QStringLiteral("progress=end"));
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.last().first().toDouble(), 1.0);
}

QTEST_MAIN(tst_MediaSalvageEngine)
#include "tst_MediaSalvageEngine.moc"
