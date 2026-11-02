#include <QTest>
#include "quality/VideoQualityEvaluator.h"

class tst_VideoQualityEvaluator : public QObject
{
    Q_OBJECT

private slots:
    void testBuildCommandDefault();
    void testBuildCommandWithOptions();
    void testParseVmafScore();
    void testParseSsimScore();
    void testParsePsnrScore();
    void testQualityResultRating();
    void testGenerateReport();
};

void tst_VideoQualityEvaluator::testBuildCommandDefault()
{
    const QString ref = QStringLiteral("original.mp4");
    const QString dist = QStringLiteral("compressed.mp4");

    const QStringList args = VideoQualityEvaluator::buildCommand(ref, dist, VideoQualityOptions());

    QVERIFY(args.contains(QStringLiteral("-i")));
    QCOMPARE(args.count(QStringLiteral("-i")), 2);
    QVERIFY(args.contains(ref));
    QVERIFY(args.contains(dist));
    QVERIFY(args.contains(QStringLiteral("-filter_complex")));

    const int filterIdx = args.indexOf(QStringLiteral("-filter_complex"));
    const QString filter = args.at(filterIdx + 1);
    QVERIFY(filter.contains(QStringLiteral("libvmaf")));
    QVERIFY(filter.contains(QStringLiteral("ssim")));
    QVERIFY(filter.contains(QStringLiteral("psnr")));
    QVERIFY(args.contains(QStringLiteral("null")));
}

void tst_VideoQualityEvaluator::testBuildCommandWithOptions()
{
    VideoQualityOptions opts;
    opts.sampleStart = 15.0;
    opts.sampleDuration = 10.0;
    opts.subsample = 2;
    opts.threads = 4;
    opts.calculatePsnr = false;

    const QStringList args = VideoQualityEvaluator::buildCommand(
        QStringLiteral("ref.mkv"), QStringLiteral("dist.mp4"), opts, 1920, 1080);

    QVERIFY(args.contains(QStringLiteral("-ss")));
    QVERIFY(args.contains(QStringLiteral("-t")));

    const int filterIdx = args.indexOf(QStringLiteral("-filter_complex"));
    const QString filter = args.at(filterIdx + 1);
    QVERIFY(filter.contains(QStringLiteral("scale=1920:1080")));
    QVERIFY(filter.contains(QStringLiteral("n_subsample=2")));
    QVERIFY(filter.contains(QStringLiteral("n_threads=4")));
    QVERIFY(filter.contains(QStringLiteral("ssim")));
    QVERIFY(!filter.contains(QStringLiteral("psnr")));
}

void tst_VideoQualityEvaluator::testParseVmafScore()
{
    const QString line = QStringLiteral("[Parsed_libvmaf_0 @ 0000022bc112b180] VMAF score: 96.505953");
    double score = 0.0;
    QVERIFY(VideoQualityEvaluator::parseVmaf(line, score));
    QCOMPARE(score, 96.505953);

    const QString invalidLine = QStringLiteral("frame=   50 fps=0.0");
    QVERIFY(!VideoQualityEvaluator::parseVmaf(invalidLine, score));
}

void tst_VideoQualityEvaluator::testParseSsimScore()
{
    const QString line1 = QStringLiteral("[Parsed_ssim_1 @ 00000266cae49600] SSIM R:1.000000 (inf) G:1.000000 (inf) B:1.000000 (inf) All:1.000000 (inf)");
    double score = 0.0;
    QVERIFY(VideoQualityEvaluator::parseSsim(line1, score));
    QCOMPARE(score, 1.0);

    const QString line2 = QStringLiteral("[Parsed_ssim_0 @ 0x123] SSIM Y:0.985000 U:0.991000 V:0.992000 All:0.987654 (18.860000)");
    QVERIFY(VideoQualityEvaluator::parseSsim(line2, score));
    QCOMPARE(score, 0.987654);
}

void tst_VideoQualityEvaluator::testParsePsnrScore()
{
    const QString line1 = QStringLiteral("[Parsed_psnr_1 @ 0x123] PSNR y:42.500000 u:46.200000 v:47.100000 average:43.654321 min:39.100000 max:49.200000");
    double score = 0.0;
    QVERIFY(VideoQualityEvaluator::parsePsnr(line1, score));
    QCOMPARE(score, 43.654321);

    const QString lineInf = QStringLiteral("[Parsed_psnr_2 @ 0x123] PSNR r:inf g:inf b:inf average:inf min:inf max:inf");
    QVERIFY(VideoQualityEvaluator::parsePsnr(lineInf, score));
    QCOMPARE(score, 100.0);
}

void tst_VideoQualityEvaluator::testQualityResultRating()
{
    VideoQualityResult r1;
    r1.vmaf = 96.2;
    QCOMPARE(r1.vmafRating(), QStringLiteral("Visually Lossless"));
    QVERIFY(r1.ratingColor().isValid());

    VideoQualityResult r2;
    r2.vmaf = 88.5;
    QCOMPARE(r2.vmafRating(), QStringLiteral("High Quality"));

    VideoQualityResult r3;
    r3.vmaf = 79.0;
    QCOMPARE(r3.vmafRating(), QStringLiteral("Good"));

    VideoQualityResult r4;
    r4.vmaf = 64.0;
    QCOMPARE(r4.vmafRating(), QStringLiteral("Noticeable Degradation"));

    r1.ssim = 0.985;
    r1.psnr = 44.5;
    const QString summary = r1.summaryText();
    QVERIFY(summary.contains(QStringLiteral("VMAF: 96.2")));
    QVERIFY(summary.contains(QStringLiteral("Visually Lossless")));
    QVERIFY(summary.contains(QStringLiteral("SSIM: 0.9850")));
    QVERIFY(summary.contains(QStringLiteral("PSNR: 44.50 dB")));
}

void tst_VideoQualityEvaluator::testGenerateReport()
{
    VideoQualityResult res;
    res.success = true;
    res.vmaf = 93.45;
    res.ssim = 0.9821;
    res.psnr = 42.15;

    VideoQualityOptions opts;
    opts.sampleStart = 5.0;
    opts.sampleDuration = 10.0;

    const QString report = VideoQualityEvaluator::generateReport(
        QStringLiteral("source.mp4"), QStringLiteral("output.mp4"), res, opts);

    QVERIFY(report.contains(QStringLiteral("Video Quality Report")));
    QVERIFY(report.contains(QStringLiteral("source.mp4")));
    QVERIFY(report.contains(QStringLiteral("output.mp4")));
    QVERIFY(report.contains(QStringLiteral("VMAF Score: 93.45")));
    QVERIFY(report.contains(QStringLiteral("High Quality")));
    QVERIFY(report.contains(QStringLiteral("SSIM:       0.9821")));
    QVERIFY(report.contains(QStringLiteral("PSNR:       42.15 dB")));
}

QTEST_MAIN(tst_VideoQualityEvaluator)
#include "tst_VideoQualityEvaluator.moc"
