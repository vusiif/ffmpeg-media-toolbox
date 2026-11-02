#include <QtTest>
#include "filters/MediaFilterStudio.h"

class tst_MediaFilterStudio : public QObject
{
    Q_OBJECT

private slots:
    void testCompileVideoFiltersDefault();
    void testCompileVideoFiltersLUT();
    void testCompileVideoFiltersEQ();
    void testCompileVideoFiltersTemperature();
    void testCompileVideoFiltersEnhancements();
    void testCompileAudioFiltersLoudnorm();
    void testCompileAudioFiltersDenoiseAndEQ();
    void testColorPresets();
    void testLoudnessStandards();
    void testSamplePreviewCommand();
    void testFilterExportCommand();
};

void tst_MediaFilterStudio::testCompileVideoFiltersDefault()
{
    ColorGradingOptions opts;
    QVERIFY(!opts.isModified());
    const QString vf = MediaFilterStudio::compileVideoFilters(opts);
    QVERIFY(vf.isEmpty());
}

void tst_MediaFilterStudio::testCompileVideoFiltersLUT()
{
    ColorGradingOptions opts;
    opts.lutFilePath = QStringLiteral("C:\\LUTs\\Cinematic.cube");
    QVERIFY(opts.isModified());

    const QString vf = MediaFilterStudio::compileVideoFilters(opts);
    QVERIFY(vf.contains(QStringLiteral("lut3d=")));
    QVERIFY(vf.contains(QStringLiteral("C:/LUTs/Cinematic.cube")));
    QVERIFY(vf.contains(QStringLiteral("interp=tetrahedral")));
}

void tst_MediaFilterStudio::testCompileVideoFiltersEQ()
{
    ColorGradingOptions opts;
    opts.brightness = 0.05;
    opts.contrast = 1.20;
    opts.saturation = 1.10;
    opts.gamma = 0.95;

    const QString vf = MediaFilterStudio::compileVideoFilters(opts);
    QVERIFY(vf.contains(QStringLiteral("eq=brightness=0.05:contrast=1.20:saturation=1.10:gamma=0.95")));
}

void tst_MediaFilterStudio::testCompileVideoFiltersTemperature()
{
    ColorGradingOptions opts;
    opts.temperature = 0.3; // warm
    opts.tint = -0.05;

    const QString vf = MediaFilterStudio::compileVideoFilters(opts);
    QVERIFY(vf.contains(QStringLiteral("colorbalance=")));
    QVERIFY(vf.contains(QStringLiteral("rm=")));
    QVERIFY(vf.contains(QStringLiteral("rh=")));
}

void tst_MediaFilterStudio::testCompileVideoFiltersEnhancements()
{
    ColorGradingOptions opts;
    opts.enableDenoise = true;
    opts.denoiseStrength = 5.0;
    opts.enableSharpen = true;
    opts.sharpenAmount = 1.2;
    opts.enableVignette = true;
    opts.vignetteStrength = 0.5;
    opts.enableDeinterlace = true;

    const QString vf = MediaFilterStudio::compileVideoFilters(opts);
    QVERIFY(vf.contains(QStringLiteral("yadif=0:-1:0")));
    QVERIFY(vf.contains(QStringLiteral("hqdn3d=5.0:3.8:7.5:5.6")));
    QVERIFY(vf.contains(QStringLiteral("unsharp=5:5:1.20:5:5:0.0")));
    QVERIFY(vf.contains(QStringLiteral("vignette=")));
}

void tst_MediaFilterStudio::testCompileAudioFiltersLoudnorm()
{
    AudioFxOptions opts;
    opts.enableLoudnorm = true;
    opts.targetLufs = -14.0;

    const QString af = MediaFilterStudio::compileAudioFilters(opts);
    QVERIFY(af.contains(QStringLiteral("loudnorm=I=-14.0:TP=-1.5:LRA=11")));
}

void tst_MediaFilterStudio::testCompileAudioFiltersDenoiseAndEQ()
{
    AudioFxOptions opts;
    opts.enableDenoise = true;
    opts.noiseReductionDb = 15.0;
    opts.enableHighpass = true;
    opts.highpassFreq = 80;
    opts.enableLowpass = true;
    opts.lowpassFreq = 12000;
    opts.volumeDb = 3.5;

    const QString af = MediaFilterStudio::compileAudioFilters(opts);
    QVERIFY(af.contains(QStringLiteral("highpass=f=80")));
    QVERIFY(af.contains(QStringLiteral("lowpass=f=12000")));
    QVERIFY(af.contains(QStringLiteral("afftdn=nr=15.0:nf=-50:tn=1")));
    QVERIFY(af.contains(QStringLiteral("volume=3.5dB")));
}

void tst_MediaFilterStudio::testColorPresets()
{
    ColorGradingOptions opts;

    opts.applyPreset(ColorPreset::CinematicTealOrange);
    QCOMPARE(opts.contrast, 1.15);
    QCOMPARE(opts.saturation, 1.10);
    QVERIFY(opts.enableSharpen);
    QVERIFY(opts.enableVignette);

    opts.applyPreset(ColorPreset::BwHighContrast);
    QCOMPARE(opts.saturation, 0.0);
    QCOMPARE(opts.contrast, 1.40);

    opts.applyPreset(ColorPreset::WarmVintage);
    QCOMPARE(opts.gamma, 1.10);
    QVERIFY(opts.temperature > 0.0);
}

void tst_MediaFilterStudio::testLoudnessStandards()
{
    AudioFxOptions opts;

    opts.applyStandard(LoudnessStandard::YouTube);
    QCOMPARE(opts.targetLufs, -14.0);
    QVERIFY(opts.enableLoudnorm);

    opts.applyStandard(LoudnessStandard::ApplePodcast);
    QCOMPARE(opts.targetLufs, -16.0);

    opts.applyStandard(LoudnessStandard::EbuR128);
    QCOMPARE(opts.targetLufs, -23.0);

    opts.applyStandard(LoudnessStandard::AtscA85);
    QCOMPARE(opts.targetLufs, -24.0);
}

void tst_MediaFilterStudio::testSamplePreviewCommand()
{
    ColorGradingOptions vOpts;
    vOpts.brightness = 0.05;
    AudioFxOptions aOpts;
    aOpts.enableLoudnorm = true;

    const QStringList args = MediaFilterStudio::buildSamplePreviewCommand(
        QStringLiteral("in.mp4"), QStringLiteral("out.mp4"), vOpts, aOpts, 10.0, 5.0);

    QVERIFY(args.contains(QStringLiteral("-ss")));
    QVERIFY(args.contains(QStringLiteral("-t")));
    QVERIFY(args.contains(QStringLiteral("-vf")));
    QVERIFY(args.contains(QStringLiteral("-af")));
    QVERIFY(args.contains(QStringLiteral("libx264")));
    QVERIFY(args.contains(QStringLiteral("ultrafast")));
    QCOMPARE(args.last(), QStringLiteral("out.mp4"));
}

void tst_MediaFilterStudio::testFilterExportCommand()
{
    ColorGradingOptions vOpts;
    vOpts.contrast = 1.2;
    AudioFxOptions aOpts;
    aOpts.volumeDb = 2.0;

    const QStringList args = MediaFilterStudio::buildFilterExportCommand(
        QStringLiteral("in.mp4"), QStringLiteral("out.mp4"), vOpts, aOpts,
        QStringLiteral("libx264"), QStringLiteral("aac"), 20);

    QVERIFY(args.contains(QStringLiteral("-vf")));
    QVERIFY(args.contains(QStringLiteral("-af")));
    QVERIFY(args.contains(QStringLiteral("-c:v")));
    QVERIFY(args.contains(QStringLiteral("libx264")));
    QVERIFY(args.contains(QStringLiteral("-crf")));
    QVERIFY(args.contains(QStringLiteral("20")));
    QVERIFY(args.contains(QStringLiteral("-c:a")));
    QVERIFY(args.contains(QStringLiteral("aac")));
    QCOMPARE(args.last(), QStringLiteral("out.mp4"));
}

QTEST_MAIN(tst_MediaFilterStudio)
#include "tst_MediaFilterStudio.moc"
