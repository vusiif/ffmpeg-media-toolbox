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
    void testParseSilenceDetect();
    void testParseBlackDetect();
    void testParseFreezeDetect();
    void testSegmentColorAndType();
    void testGenerateQCReport();
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

void tst_CorruptedVideoScanner::testParseSilenceDetect()
{
    const QString startLine = QStringLiteral("[Parsed_silencedetect_0 @ 0000021535a5c8c0] silence_start: 12.345");
    double start = 0.0, end = 0.0, dur = 0.0;
    bool isStart = false;

    QVERIFY(CorruptedVideoScanner::parseSilenceDetect(startLine, start, end, dur, isStart));
    QVERIFY(isStart);
    QCOMPARE(start, 12.345);

    const QString endLine = QStringLiteral("[Parsed_silencedetect_0 @ 0000021535a5c8c0] silence_end: 18.500 | silence_duration: 6.155");
    QVERIFY(CorruptedVideoScanner::parseSilenceDetect(endLine, start, end, dur, isStart));
    QVERIFY(!isStart);
    QCOMPARE(end, 18.500);
    QCOMPARE(dur, 6.155);
    QCOMPARE(start, 12.345);

    const QString invalidLine = QStringLiteral("frame=   50 fps=0.0 time=00:00:02.00");
    QVERIFY(!CorruptedVideoScanner::parseSilenceDetect(invalidLine, start, end, dur, isStart));
}

void tst_CorruptedVideoScanner::testParseBlackDetect()
{
    const QString line = QStringLiteral("[Parsed_blackdetect_0 @ 0000027b3532c740] black_start:10.500 black_end:14.200 black_duration:3.700");
    double start = 0.0, end = 0.0, dur = 0.0;

    QVERIFY(CorruptedVideoScanner::parseBlackDetect(line, start, end, dur));
    QCOMPARE(start, 10.500);
    QCOMPARE(end, 14.200);
    QCOMPARE(dur, 3.700);

    const QString invalidLine = QStringLiteral("[Parsed_crop_1 @ 0x123] crop=1280:720");
    QVERIFY(!CorruptedVideoScanner::parseBlackDetect(invalidLine, start, end, dur));
}

void tst_CorruptedVideoScanner::testParseFreezeDetect()
{
    const QString startLine = QStringLiteral("[Parsed_freezedetect_1 @ 000002e57ce3eec0] lavfi.freezedetect.freeze_start: 4.2");
    double start = 0.0, end = 0.0, dur = 0.0;
    bool isStart = false;

    QVERIFY(CorruptedVideoScanner::parseFreezeDetect(startLine, start, end, dur, isStart));
    QVERIFY(isStart);
    QCOMPARE(start, 4.2);

    const QString durLine = QStringLiteral("[Parsed_freezedetect_1 @ 000002e57ce3eec0] lavfi.freezedetect.freeze_duration: 3.5");
    CorruptedVideoScanner::parseFreezeDetect(durLine, start, end, dur, isStart);
    QCOMPARE(dur, 3.5);

    const QString endLine = QStringLiteral("[Parsed_freezedetect_1 @ 000002e57ce3eec0] lavfi.freezedetect.freeze_end: 7.7");
    QVERIFY(CorruptedVideoScanner::parseFreezeDetect(endLine, start, end, dur, isStart));
    QVERIFY(!isStart);
    QCOMPARE(end, 7.7);
    QCOMPARE(start, 4.2);
}

void tst_CorruptedVideoScanner::testSegmentColorAndType()
{
    CorruptedSegment segDecode;
    segDecode.type = QCIssueType::DecodeError;
    QCOMPARE(segDecode.typeName(), QStringLiteral("Decode Error"));
    QVERIFY(segDecode.color().isValid());

    CorruptedSegment segSilence;
    segSilence.type = QCIssueType::Silence;
    QCOMPARE(segSilence.typeName(), QStringLiteral("Silence"));
    QVERIFY(segSilence.color().isValid());

    CorruptedSegment segBlack;
    segBlack.type = QCIssueType::BlackFrame;
    QCOMPARE(segBlack.typeName(), QStringLiteral("Black Frame"));
    QVERIFY(segBlack.color().isValid());

    CorruptedSegment segFreeze;
    segFreeze.type = QCIssueType::FreezeFrame;
    QCOMPARE(segFreeze.typeName(), QStringLiteral("Freeze Frame"));
    QVERIFY(segFreeze.color().isValid());
}

void tst_CorruptedVideoScanner::testGenerateQCReport()
{
    QVector<CorruptedSegment> segs;
    CorruptedSegment s1;
    s1.type = QCIssueType::Silence;
    s1.start = 5.0;
    s1.end = 8.5;
    s1.errorMsg = QStringLiteral("Silence detected (3.50s)");
    segs.append(s1);

    CorruptedSegment s2;
    s2.type = QCIssueType::BlackFrame;
    s2.start = 12.0;
    s2.end = 15.0;
    s2.errorMsg = QStringLiteral("Black frame detected (3.00s)");
    segs.append(s2);

    const QString report = CorruptedVideoScanner::generateReport(QStringLiteral("test.mp4"), 60.0, segs);
    QVERIFY(report.contains(QStringLiteral("Quality Control Report")));
    QVERIFY(report.contains(QStringLiteral("Silence Intervals:       1")));
    QVERIFY(report.contains(QStringLiteral("Black Frame Intervals:   1")));
    QVERIFY(report.contains(QStringLiteral("Detailed Issues Timeline:")));
    QVERIFY(report.contains(QStringLiteral("test.mp4")));
}

QTEST_MAIN(tst_CorruptedVideoScanner)
#include "tst_CorruptedVideoScanner.moc"
