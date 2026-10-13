#include <QTest>
#include "media/CorruptedVideoScanner.h"

class tst_CorruptedVideoScanner : public QObject
{
    Q_OBJECT

private slots:
    void testExtractTimestampHms();
    void testExtractTimestampSec();
    void testExtractTimestampPts();
    void testMergeErrorTimestamps();
};

void tst_CorruptedVideoScanner::testExtractTimestampHms()
{
    const QString line1 = QStringLiteral("[h264 @ 0x7fa2] time=00:01:23.45 error while decoding MB 12 34");
    const double t1 = CorruptedVideoScanner::extractTimestampFromLine(line1);
    QCOMPARE(t1, 83.45);

    const QString line2 = QStringLiteral("frame=  120 fps= 30 time=01:00:00.00 error corrupted frame");
    const double t2 = CorruptedVideoScanner::extractTimestampFromLine(line2);
    QCOMPARE(t2, 3600.0);
}

void tst_CorruptedVideoScanner::testExtractTimestampSec()
{
    const QString line = QStringLiteral("time=45.67 [hevc @ 0x123] invalid NAL unit");
    const double t = CorruptedVideoScanner::extractTimestampFromLine(line);
    QCOMPARE(t, 45.67);
}

void tst_CorruptedVideoScanner::testExtractTimestampPts()
{
    const QString line = QStringLiteral("[mov,mp4 @ 0x123] pts: 12.345 error while decoding");
    const double t = CorruptedVideoScanner::extractTimestampFromLine(line);
    QCOMPARE(t, 12.345);
}

void tst_CorruptedVideoScanner::testMergeErrorTimestamps()
{
    // Three errors within 0.5s of each other (10.0, 10.2, 10.4) -> should merge into one segment
    // One distant error at 25.0 -> should form a second segment
    const QVector<QPair<double, QString>> raw = {
        {10.0, QStringLiteral("error A")},
        {10.2, QStringLiteral("error B")},
        {10.4, QStringLiteral("error C")},
        {25.0, QStringLiteral("error D")}
    };

    const QVector<CorruptedSegment> segs = CorruptedVideoScanner::mergeErrorTimestamps(raw, 0.5, 0.08);
    QCOMPARE(segs.size(), 2);

    // Segment 1
    QCOMPARE(segs[0].start, 10.0);
    QVERIFY(segs[0].end >= 10.48);
    QVERIFY(segs[0].errorMsg.contains(QStringLiteral("error A")));
    QVERIFY(segs[0].errorMsg.contains(QStringLiteral("error B")));

    // Segment 2
    QCOMPARE(segs[1].start, 25.0);
    QVERIFY(segs[1].end >= 25.08);
    QVERIFY(segs[1].errorMsg.contains(QStringLiteral("error D")));
}

QTEST_MAIN(tst_CorruptedVideoScanner)
#include "tst_CorruptedVideoScanner.moc"
