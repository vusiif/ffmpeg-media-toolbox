#include <QtTest>
#include "cut/SmartCutEngine.h"

class tst_SmartCutEngine : public QObject
{
    Q_OBJECT

private slots:
    void testBuildKeyframeProbeCommand();
    void testParseKeyframeOutput();
    void testKeyframeNavigation();
    void testKeyframeAlignment();
    void testBuildLosslessCut();
    void testBuildAccurateReencode();
    void testPlanSmartCutAligned();
    void testPlanSmartCutSingleGop();
    void testPlanSmartCutMultiGop();
    void testConcatDemuxerAndList();
    void testTimeFormatting();
};

void tst_SmartCutEngine::testBuildKeyframeProbeCommand()
{
    const QString ffprobe = QStringLiteral("C:/ffmpeg/ffprobe.exe");
    const QString input = QStringLiteral("C:/videos/sample.mp4");
    const QStringList args = SmartCutEngine::buildKeyframeProbeCommand(ffprobe, input);

    QCOMPARE(args.first(), ffprobe);
    QVERIFY(args.contains(QStringLiteral("-select_streams")));
    QVERIFY(args.contains(QStringLiteral("-skip_frame")));
    QVERIFY(args.contains(QStringLiteral("nokey")));
    QVERIFY(args.contains(QStringLiteral("frame=pts_time")));
    QVERIFY(args.contains(QStringLiteral("csv=p=0")));
    QCOMPARE(args.last(), input);
}

void tst_SmartCutEngine::testParseKeyframeOutput()
{
    const QString sampleOutput = QStringLiteral(
        "0.000000,\n"
        "1.000000\n"
        "2.502000\n"
        "2.503000\n" // within 0.005s deduplication
        "5.000000\n"
        "bad_line_xyz\n"
        "10.000000\n"
    );

    const QVector<double> kfs = SmartCutEngine::parseKeyframeOutput(sampleOutput);
    QCOMPARE(kfs.size(), 5);
    QCOMPARE(kfs[0], 0.0);
    QCOMPARE(kfs[1], 1.0);
    QCOMPARE(kfs[2], 2.502);
    QCOMPARE(kfs[3], 5.0);
    QCOMPARE(kfs[4], 10.0);

    // Empty output
    QVERIFY(SmartCutEngine::parseKeyframeOutput(QString()).isEmpty());
}

void tst_SmartCutEngine::testKeyframeNavigation()
{
    const QVector<double> kfs = { 0.0, 2.0, 4.0, 6.0, 10.0 };

    // Previous Keyframe
    QCOMPARE(SmartCutEngine::findPrevKeyframe(3.5, kfs), 2.0);
    QCOMPARE(SmartCutEngine::findPrevKeyframe(4.0, kfs), 4.0);
    QCOMPARE(SmartCutEngine::findPrevKeyframe(0.5, kfs), 0.0);
    QCOMPARE(SmartCutEngine::findPrevKeyframe(12.0, kfs), 10.0);

    // Next Keyframe
    QCOMPARE(SmartCutEngine::findNextKeyframe(3.5, kfs), 4.0);
    QCOMPARE(SmartCutEngine::findNextKeyframe(4.0, kfs), 4.0);
    QCOMPARE(SmartCutEngine::findNextKeyframe(5.5, kfs), 6.0);
    QCOMPARE(SmartCutEngine::findNextKeyframe(12.0, kfs), 10.0);

    // Nearest Keyframe
    QCOMPARE(SmartCutEngine::findNearestKeyframe(2.8, kfs), 2.0); // 2.8 - 2.0 = 0.8; 4.0 - 2.8 = 1.2
    QCOMPARE(SmartCutEngine::findNearestKeyframe(3.2, kfs), 4.0); // 4.0 - 3.2 = 0.8
}

void tst_SmartCutEngine::testKeyframeAlignment()
{
    const QVector<double> kfs = { 0.0, 2.5, 5.0, 7.5 };

    QVERIFY(SmartCutEngine::isKeyframeAligned(2.5, kfs, 0.05));
    QVERIFY(SmartCutEngine::isKeyframeAligned(2.53, kfs, 0.05));
    QVERIFY(!SmartCutEngine::isKeyframeAligned(2.65, kfs, 0.05));
    QVERIFY(SmartCutEngine::isKeyframeAligned(5.02, kfs, 0.05));
    QVERIFY(!SmartCutEngine::isKeyframeAligned(3.8, kfs, 0.05));
}

void tst_SmartCutEngine::testBuildLosslessCut()
{
    const QString input = QStringLiteral("input.mp4");
    const QString output = QStringLiteral("output.mp4");
    const QStringList args = SmartCutEngine::buildLosslessCut(input, output, 5.0, 15.0, QStringLiteral("mp4"));

    QVERIFY(args.contains(QStringLiteral("-ss")));
    QVERIFY(args.contains(QStringLiteral("-t")));
    QVERIFY(args.contains(QStringLiteral("-c")));
    QVERIFY(args.contains(QStringLiteral("copy")));
    QVERIFY(args.contains(QStringLiteral("-avoid_negative_ts")));
    QVERIFY(args.contains(QStringLiteral("make_zero")));
    QVERIFY(args.contains(QStringLiteral("-f")));
    QVERIFY(args.contains(QStringLiteral("mp4")));
    QCOMPARE(args.last(), output);
}

void tst_SmartCutEngine::testBuildAccurateReencode()
{
    const QString input = QStringLiteral("input.mp4");
    const QString output = QStringLiteral("output.mp4");
    const QStringList args = SmartCutEngine::buildAccurateReencode(input, output, 5.0, 15.0,
                                                                  QStringLiteral("libx264"), 20);

    QVERIFY(args.contains(QStringLiteral("-c:v")));
    QVERIFY(args.contains(QStringLiteral("libx264")));
    QVERIFY(args.contains(QStringLiteral("-crf")));
    QVERIFY(args.contains(QStringLiteral("20")));
    QVERIFY(args.contains(QStringLiteral("-c:a")));
    QVERIFY(args.contains(QStringLiteral("copy")));
    QCOMPARE(args.last(), output);
}

void tst_SmartCutEngine::testPlanSmartCutAligned()
{
    const QVector<double> kfs = { 0.0, 2.0, 4.0, 6.0, 8.0 };
    const SmartCutPlan plan = SmartCutEngine::planSmartCut(QStringLiteral("in.mp4"),
                                                          QStringLiteral("out.mp4"),
                                                          2.0, 6.0, kfs);

    QVERIFY(plan.isValid);
    QCOMPARE(plan.chunks.size(), 1);
    QCOMPARE(plan.chunks.first().type, SmartChunk::ChunkType::StreamCopy);
    QCOMPARE(plan.chunks.first().startSec, 2.0);
    QCOMPARE(plan.chunks.first().endSec, 6.0);
    QVERIFY(!plan.needsConcat);
}

void tst_SmartCutEngine::testPlanSmartCutSingleGop()
{
    const QVector<double> kfs = { 0.0, 2.0, 4.0, 6.0, 8.0 };
    // [2.2, 3.8] is entirely within the GOP [2.0, 4.0)
    const SmartCutPlan plan = SmartCutEngine::planSmartCut(QStringLiteral("in.mp4"),
                                                          QStringLiteral("out.mp4"),
                                                          2.2, 3.8, kfs);

    QVERIFY(plan.isValid);
    QCOMPARE(plan.chunks.size(), 1);
    QCOMPARE(plan.chunks.first().type, SmartChunk::ChunkType::Reencode);
    QCOMPARE(plan.chunks.first().startSec, 2.2);
    QCOMPARE(plan.chunks.first().endSec, 3.8);
    QVERIFY(!plan.needsConcat);
}

void tst_SmartCutEngine::testPlanSmartCutMultiGop()
{
    const QVector<double> kfs = { 0.0, 2.0, 4.0, 6.0, 8.0 };
    // [1.5, 6.5] crosses GOPs:
    // Head: 1.5 -> 2.0 (reencode)
    // Body: 2.0 -> 6.0 (stream copy)
    // Tail: 6.0 -> 6.5 (reencode)
    const SmartCutPlan plan = SmartCutEngine::planSmartCut(QStringLiteral("in.mp4"),
                                                          QStringLiteral("C:/temp/out.mp4"),
                                                          1.5, 6.5, kfs);

    QVERIFY(plan.isValid);
    QCOMPARE(plan.chunks.size(), 3);
    QCOMPARE(plan.chunks[0].type, SmartChunk::ChunkType::Reencode);
    QCOMPARE(plan.chunks[0].startSec, 1.5);
    QCOMPARE(plan.chunks[0].endSec, 2.0);

    QCOMPARE(plan.chunks[1].type, SmartChunk::ChunkType::StreamCopy);
    QCOMPARE(plan.chunks[1].startSec, 2.0);
    QCOMPARE(plan.chunks[1].endSec, 6.0);

    QCOMPARE(plan.chunks[2].type, SmartChunk::ChunkType::Reencode);
    QCOMPARE(plan.chunks[2].startSec, 6.0);
    QCOMPARE(plan.chunks[2].endSec, 6.5);

    QVERIFY(plan.needsConcat);
    QVERIFY(!plan.concatListFile.isEmpty());
}

void tst_SmartCutEngine::testConcatDemuxerAndList()
{
    const QStringList files = { QStringLiteral("C:/a/part1.mp4"), QStringLiteral("C:/a/part2.mp4") };
    const QString listContent = SmartCutEngine::generateConcatFileList(files);

    QVERIFY(listContent.contains(QStringLiteral("file 'C:/a/part1.mp4'")));
    QVERIFY(listContent.contains(QStringLiteral("file 'C:/a/part2.mp4'")));

    const QStringList concatCmd = SmartCutEngine::buildConcatDemuxer(QStringLiteral("list.txt"), QStringLiteral("out.mp4"));
    QVERIFY(concatCmd.contains(QStringLiteral("concat")));
    QVERIFY(concatCmd.contains(QStringLiteral("-safe")));
    QVERIFY(concatCmd.contains(QStringLiteral("0")));
    QVERIFY(concatCmd.contains(QStringLiteral("copy")));
    QCOMPARE(concatCmd.last(), QStringLiteral("out.mp4"));
}

void tst_SmartCutEngine::testTimeFormatting()
{
    QCOMPARE(SmartCutEngine::formatTime(0.0), QStringLiteral("00:00:00.000"));
    QCOMPARE(SmartCutEngine::formatTime(65.123), QStringLiteral("00:01:05.123"));
    QCOMPARE(SmartCutEngine::formatTime(3661.500), QStringLiteral("01:01:01.500"));

    QCOMPARE(SmartCutEngine::formatDuration(45.0), QStringLiteral("00:45"));
    QCOMPARE(SmartCutEngine::formatDuration(125.0), QStringLiteral("02:05"));
    QCOMPARE(SmartCutEngine::formatDuration(3665.0), QStringLiteral("01:01:05"));
}

QTEST_MAIN(tst_SmartCutEngine)
#include "tst_SmartCutEngine.moc"
