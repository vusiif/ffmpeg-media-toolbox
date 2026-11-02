#include <QTest>
#include <QString>
#include <QStringList>

#include "animation/AnimatedImageEngine.h"

using namespace Animation;

class tst_AnimatedImageEngine : public QObject
{
    Q_OBJECT

private slots:
    void testDefaultConfig();
    void testDitherModes();
    void testTimeFormatting();
    void testGifFilterGraph();
    void testWebPAndApngFilterGraph();
    void testDirectionAndSpeed();
    void testFileEstimationAndFrames();
    void testBuildCommand();
};

void tst_AnimatedImageEngine::testDefaultConfig()
{
    AnimatedImageConfig cfg;
    QCOMPARE(cfg.format, OutputFormat::GIF);
    QCOMPARE(cfg.fps, 15);
    QCOMPARE(cfg.width, 480);
    QCOMPARE(cfg.height, -1);
    QCOMPARE(cfg.maxColors, 256);
    QCOMPARE(cfg.statsMode, PaletteStatsMode::Diff);
    QCOMPARE(cfg.dither, DitherMode::Bayer);
    QCOMPARE(cfg.bayerScale, 3);
    QCOMPARE(cfg.loopCount, 0);
    QVERIFY(!cfg.isModified());

    cfg.fps = 24;
    QVERIFY(cfg.isModified());

    cfg.reset();
    QVERIFY(!cfg.isModified());
    QCOMPARE(cfg.fps, 15);
}

void tst_AnimatedImageEngine::testDitherModes()
{
    QCOMPARE(AnimatedImageEngine::ditherModeString(DitherMode::Bayer, 3), QStringLiteral("bayer:bayer_scale=3"));
    QCOMPARE(AnimatedImageEngine::ditherModeString(DitherMode::Bayer, 1), QStringLiteral("bayer:bayer_scale=1"));
    QCOMPARE(AnimatedImageEngine::ditherModeString(DitherMode::Bayer, 5), QStringLiteral("bayer:bayer_scale=5"));
    QCOMPARE(AnimatedImageEngine::ditherModeString(DitherMode::FloydSteinberg, 0), QStringLiteral("floyd_steinberg"));
    QCOMPARE(AnimatedImageEngine::ditherModeString(DitherMode::Sierra2_4a, 0), QStringLiteral("sierra2_4a"));
    QCOMPARE(AnimatedImageEngine::ditherModeString(DitherMode::None, 0), QStringLiteral("none"));
}

void tst_AnimatedImageEngine::testTimeFormatting()
{
    QCOMPARE(AnimatedImageEngine::formatTime(0.0), QStringLiteral("00:00:00.000"));
    QCOMPARE(AnimatedImageEngine::formatTime(5.5), QStringLiteral("00:00:05.500"));
    QCOMPARE(AnimatedImageEngine::formatTime(65.123), QStringLiteral("00:01:05.123"));
    QCOMPARE(AnimatedImageEngine::formatTime(3665.040), QStringLiteral("01:01:05.040"));
}

void tst_AnimatedImageEngine::testGifFilterGraph()
{
    AnimatedImageConfig cfg;
    cfg.format = OutputFormat::GIF;
    cfg.fps = 15;
    cfg.width = 480;
    cfg.maxColors = 128;
    cfg.statsMode = PaletteStatsMode::Diff;
    cfg.dither = DitherMode::Bayer;
    cfg.bayerScale = 4;
    cfg.diffModeRectangle = true;

    const QString fg = AnimatedImageEngine::buildFilterGraph(cfg);
    QVERIFY(fg.contains(QStringLiteral("fps=15")));
    QVERIFY(fg.contains(QStringLiteral("scale=480:-1:flags=lanczos")));
    QVERIFY(fg.contains(QStringLiteral("split[v0][v1]")));
    QVERIFY(fg.contains(QStringLiteral("palettegen=max_colors=128:stats_mode=diff:reserve_transparent=1[pal]")));
    QVERIFY(fg.contains(QStringLiteral("paletteuse=dither=bayer:bayer_scale=4:diff_mode=rectangle")));
}

void tst_AnimatedImageEngine::testWebPAndApngFilterGraph()
{
    AnimatedImageConfig cfg;
    cfg.format = OutputFormat::WebP;
    cfg.fps = 20;
    cfg.width = 360;

    QString fg = AnimatedImageEngine::buildFilterGraph(cfg);
    QVERIFY(fg.contains(QStringLiteral("fps=20")));
    QVERIFY(fg.contains(QStringLiteral("scale=360:-1:flags=lanczos")));
    QVERIFY(!fg.contains(QStringLiteral("palettegen")));

    cfg.format = OutputFormat::APNG;
    fg = AnimatedImageEngine::buildFilterGraph(cfg);
    QVERIFY(fg.contains(QStringLiteral("fps=20")));
    QVERIFY(fg.contains(QStringLiteral("scale=360:-1:flags=lanczos")));
    QVERIFY(!fg.contains(QStringLiteral("palettegen")));
}

void tst_AnimatedImageEngine::testDirectionAndSpeed()
{
    AnimatedImageConfig cfg;
    cfg.format = OutputFormat::WebP;
    cfg.speedMultiplier = 1.5;
    cfg.direction = PlaybackDirection::Reverse;

    QString fg = AnimatedImageEngine::buildFilterGraph(cfg);
    QVERIFY(fg.contains(QStringLiteral("setpts=PTS/1.50")));
    QVERIFY(fg.contains(QStringLiteral("reverse")));

    // Ping-Pong test
    cfg.direction = PlaybackDirection::PingPong;
    fg = AnimatedImageEngine::buildFilterGraph(cfg);
    QVERIFY(fg.contains(QStringLiteral("split[fwd][bwd]")));
    QVERIFY(fg.contains(QStringLiteral("[bwd]reverse[rev]")));
    QVERIFY(fg.contains(QStringLiteral("concat=n=2:v=1:a=0")));
}

void tst_AnimatedImageEngine::testFileEstimationAndFrames()
{
    AnimatedImageConfig cfg;
    cfg.durationSec = 2.0;
    cfg.fps = 15;
    cfg.speedMultiplier = 1.0;
    cfg.direction = PlaybackDirection::Normal;

    QCOMPARE(cfg.totalFrames(), 30); // 2.0 * 15 = 30
    QVERIFY(cfg.estimateFileSizeBytes() > 50000);

    // Ping-Pong doubles frame count
    cfg.direction = PlaybackDirection::PingPong;
    QCOMPARE(cfg.totalFrames(), 60);

    // Speedup 2x halves frame count
    cfg.speedMultiplier = 2.0;
    QCOMPARE(cfg.totalFrames(), 30);
}

void tst_AnimatedImageEngine::testBuildCommand()
{
    // Empty path check
    QVERIFY(AnimatedImageEngine::buildCommand(QString(), QStringLiteral("out.gif"), AnimatedImageConfig()).isEmpty());
    QVERIFY(AnimatedImageEngine::buildCommand(QStringLiteral("in.mp4"), QString(), AnimatedImageConfig()).isEmpty());

    // GIF command
    AnimatedImageConfig cfg;
    cfg.format = OutputFormat::GIF;
    cfg.startSec = 1.5;
    cfg.durationSec = 2.0;
    cfg.loopCount = 0;

    QStringList cmd = AnimatedImageEngine::buildCommand(
        QStringLiteral("C:/video.mp4"),
        QStringLiteral("C:/anim.gif"),
        cfg
    );

    QVERIFY(cmd.contains(QStringLiteral("-y")));
    QVERIFY(cmd.contains(QStringLiteral("-ss")));
    QVERIFY(cmd.contains(QStringLiteral("00:00:01.500")));
    QVERIFY(cmd.contains(QStringLiteral("-i")));
    QVERIFY(cmd.contains(QStringLiteral("C:/video.mp4")));
    QVERIFY(cmd.contains(QStringLiteral("-t")));
    QVERIFY(cmd.contains(QStringLiteral("00:00:02.000")));
    QVERIFY(cmd.contains(QStringLiteral("-filter_complex")));
    QVERIFY(cmd.contains(QStringLiteral("-loop")));
    QVERIFY(cmd.contains(QStringLiteral("0")));
    QCOMPARE(cmd.last(), QStringLiteral("C:/anim.gif"));

    // WebP command
    cfg.format = OutputFormat::WebP;
    cfg.webpLossless = false;
    cfg.webpQuality = 80;
    cmd = AnimatedImageEngine::buildCommand(QStringLiteral("a.mp4"), QStringLiteral("b.webp"), cfg);
    QVERIFY(cmd.contains(QStringLiteral("-vcodec")));
    QVERIFY(cmd.contains(QStringLiteral("libwebp")));
    QVERIFY(cmd.contains(QStringLiteral("-lossless")));
    QVERIFY(cmd.contains(QStringLiteral("0")));
    QVERIFY(cmd.contains(QStringLiteral("-q:v")));
    QVERIFY(cmd.contains(QStringLiteral("80")));

    // APNG command
    cfg.format = OutputFormat::APNG;
    cmd = AnimatedImageEngine::buildCommand(QStringLiteral("a.mp4"), QStringLiteral("b.png"), cfg);
    QVERIFY(cmd.contains(QStringLiteral("-vcodec")));
    QVERIFY(cmd.contains(QStringLiteral("apng")));
    QVERIFY(cmd.contains(QStringLiteral("-plays")));
}

QTEST_MAIN(tst_AnimatedImageEngine)
#include "tst_AnimatedImageEngine.moc"
