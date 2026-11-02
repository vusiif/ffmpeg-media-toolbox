#include <QTest>
#include <QString>
#include <QStringList>

#include "compositor/MultiTrackCompositor.h"

using namespace Compositor;

class tst_MultiTrackCompositor : public QObject
{
    Q_OBJECT

private slots:
    void testDefaultConfig();
    void testOverlayPositionExpressions();
    void testPictureInPictureVideoFilter();
    void testSideBySideVideoFilter();
    void testTopBottomVideoFilter();
    void testAudioMixingPolicies();
    void testBuildCommand();
};

void tst_MultiTrackCompositor::testDefaultConfig()
{
    CompositorConfig cfg;
    QCOMPARE(cfg.layout, LayoutMode::PictureInPicture);
    QCOMPARE(cfg.position, PipPosition::BottomRight);
    QCOMPARE(cfg.scalePercent, 0.25);
    QCOMPARE(cfg.margin, 16);
    QCOMPARE(cfg.drawBorder, false);
    QCOMPARE(cfg.hasTimeRange, false);
    QCOMPARE(cfg.audioPolicy, AudioMixPolicy::MixDual);
    QCOMPARE(cfg.mainVolume, 1.0);
    QCOMPARE(cfg.pipVolume, 0.8);
    QVERIFY(!cfg.isModified());

    cfg.scalePercent = 0.35;
    QVERIFY(cfg.isModified());

    cfg.reset();
    QVERIFY(!cfg.isModified());
    QCOMPARE(cfg.scalePercent, 0.25);
}

void tst_MultiTrackCompositor::testOverlayPositionExpressions()
{
    // BottomRight
    QString br = MultiTrackCompositor::overlayPositionExpression(PipPosition::BottomRight, 10);
    QCOMPARE(br, QStringLiteral("main_w-overlay_w-10:main_h-overlay_h-10"));

    // BottomLeft
    QString bl = MultiTrackCompositor::overlayPositionExpression(PipPosition::BottomLeft, 20);
    QCOMPARE(bl, QStringLiteral("20:main_h-overlay_h-20"));

    // TopRight
    QString tr = MultiTrackCompositor::overlayPositionExpression(PipPosition::TopRight, 15);
    QCOMPARE(tr, QStringLiteral("main_w-overlay_w-15:15"));

    // TopLeft
    QString tl = MultiTrackCompositor::overlayPositionExpression(PipPosition::TopLeft, 25);
    QCOMPARE(tl, QStringLiteral("25:25"));

    // Center
    QString center = MultiTrackCompositor::overlayPositionExpression(PipPosition::Center, 0);
    QCOMPARE(center, QStringLiteral("(main_w-overlay_w)/2:(main_h-overlay_h)/2"));

    // CustomCoordinate
    QString custom = MultiTrackCompositor::overlayPositionExpression(PipPosition::CustomCoordinate, 0, 100, 200);
    QCOMPARE(custom, QStringLiteral("100:200"));
}

void tst_MultiTrackCompositor::testPictureInPictureVideoFilter()
{
    // 1. Default scale2ref with 25% scale
    CompositorConfig cfg;
    cfg.layout = LayoutMode::PictureInPicture;
    cfg.position = PipPosition::BottomRight;
    cfg.margin = 16;

    QString vf = MultiTrackCompositor::buildVideoFilter(cfg);
    QVERIFY(vf.contains(QStringLiteral("scale2ref=")));
    QVERIFY(vf.contains(QStringLiteral("overlay=")));
    QVERIFY(vf.contains(QStringLiteral("[outv]")));

    // 2. Custom width
    cfg.customWidth = 480;
    vf = MultiTrackCompositor::buildVideoFilter(cfg);
    QVERIFY(vf.contains(QStringLiteral("scale=480:-2")));

    // 3. Border
    cfg.drawBorder = true;
    cfg.borderWidth = 3;
    cfg.borderColor = QStringLiteral("yellow");
    vf = MultiTrackCompositor::buildVideoFilter(cfg);
    QVERIFY(vf.contains(QStringLiteral("drawbox=")));
    QVERIFY(vf.contains(QStringLiteral("color=yellow")));
    QVERIFY(vf.contains(QStringLiteral("t=3")));

    // 4. Time range
    cfg.hasTimeRange = true;
    cfg.startTimeSec = 5.0;
    cfg.durationSec = 10.0;
    vf = MultiTrackCompositor::buildVideoFilter(cfg);
    QVERIFY(vf.contains(QStringLiteral("enable='between(t,5.00,15.00)'")));
}

void tst_MultiTrackCompositor::testSideBySideVideoFilter()
{
    CompositorConfig cfg;
    cfg.layout = LayoutMode::SideBySide;
    cfg.splitWidth = 1920;
    cfg.splitHeight = 1080;
    cfg.splitRatio = 0.5;

    QString vf = MultiTrackCompositor::buildVideoFilter(cfg);
    QVERIFY(vf.contains(QStringLiteral("hstack=inputs=2[outv]")));
    QVERIFY(vf.contains(QStringLiteral("scale=960:1080")));
    QVERIFY(vf.contains(QStringLiteral("pad=960:1080")));
}

void tst_MultiTrackCompositor::testTopBottomVideoFilter()
{
    CompositorConfig cfg;
    cfg.layout = LayoutMode::TopBottom;
    cfg.splitWidth = 1920;
    cfg.splitHeight = 1080;
    cfg.splitRatio = 0.5;

    QString vf = MultiTrackCompositor::buildVideoFilter(cfg);
    QVERIFY(vf.contains(QStringLiteral("vstack=inputs=2[outv]")));
    QVERIFY(vf.contains(QStringLiteral("scale=1920:540")));
    QVERIFY(vf.contains(QStringLiteral("pad=1920:540")));
}

void tst_MultiTrackCompositor::testAudioMixingPolicies()
{
    CompositorConfig cfg;

    // 1. MixDual
    cfg.audioPolicy = AudioMixPolicy::MixDual;
    cfg.mainVolume = 1.0;
    cfg.pipVolume = 0.8;
    QString af = MultiTrackCompositor::buildAudioFilter(cfg);
    QVERIFY(af.contains(QStringLiteral("volume=1.00")));
    QVERIFY(af.contains(QStringLiteral("volume=0.80")));
    QVERIFY(af.contains(QStringLiteral("amix=inputs=2")));

    // 2. Ducking
    cfg.audioPolicy = AudioMixPolicy::Ducking;
    af = MultiTrackCompositor::buildAudioFilter(cfg);
    QVERIFY(af.contains(QStringLiteral("volume=0.25"))); // 1.0 * 0.25
    QVERIFY(af.contains(QStringLiteral("volume=0.80")));

    // 3. MainOnly / PipOnly / MuteAll should return empty audio filter
    cfg.audioPolicy = AudioMixPolicy::MainOnly;
    QVERIFY(MultiTrackCompositor::buildAudioFilter(cfg).isEmpty());

    cfg.audioPolicy = AudioMixPolicy::PipOnly;
    QVERIFY(MultiTrackCompositor::buildAudioFilter(cfg).isEmpty());

    cfg.audioPolicy = AudioMixPolicy::MuteAll;
    QVERIFY(MultiTrackCompositor::buildAudioFilter(cfg).isEmpty());
}

void tst_MultiTrackCompositor::testBuildCommand()
{
    // Empty inputs check
    QVERIFY(MultiTrackCompositor::buildCommand(QString(), QStringLiteral("b.mp4"), QStringLiteral("out.mp4"), CompositorConfig()).isEmpty());
    QVERIFY(MultiTrackCompositor::buildCommand(QStringLiteral("a.mp4"), QString(), QStringLiteral("out.mp4"), CompositorConfig()).isEmpty());
    QVERIFY(MultiTrackCompositor::buildCommand(QStringLiteral("a.mp4"), QStringLiteral("b.mp4"), QString(), CompositorConfig()).isEmpty());

    CompositorConfig cfg;
    cfg.audioPolicy = AudioMixPolicy::MixDual;
    cfg.videoCodec = QStringLiteral("libx264");
    cfg.crf = 22;
    cfg.shortest = true;

    QStringList cmd = MultiTrackCompositor::buildCommand(
        QStringLiteral("C:/main.mp4"),
        QStringLiteral("C:/pip.mp4"),
        QStringLiteral("C:/out.mp4"),
        cfg
    );

    QVERIFY(cmd.contains(QStringLiteral("-y")));
    QVERIFY(cmd.contains(QStringLiteral("C:/main.mp4")));
    QVERIFY(cmd.contains(QStringLiteral("C:/pip.mp4")));
    QVERIFY(cmd.contains(QStringLiteral("-filter_complex")));
    QVERIFY(cmd.contains(QStringLiteral("-map")));
    QVERIFY(cmd.contains(QStringLiteral("[outv]")));
    QVERIFY(cmd.contains(QStringLiteral("[outa]")));
    QVERIFY(cmd.contains(QStringLiteral("-c:v")));
    QVERIFY(cmd.contains(QStringLiteral("libx264")));
    QVERIFY(cmd.contains(QStringLiteral("-crf")));
    QVERIFY(cmd.contains(QStringLiteral("22")));
    QVERIFY(cmd.contains(QStringLiteral("-shortest")));
    QCOMPARE(cmd.last(), QStringLiteral("C:/out.mp4"));

    // Check MuteAll command
    cfg.audioPolicy = AudioMixPolicy::MuteAll;
    cmd = MultiTrackCompositor::buildCommand(QStringLiteral("a.mp4"), QStringLiteral("b.mp4"), QStringLiteral("out.mp4"), cfg);
    QVERIFY(cmd.contains(QStringLiteral("-an")));
    QVERIFY(!cmd.contains(QStringLiteral("[outa]")));
}

QTEST_MAIN(tst_MultiTrackCompositor)
#include "tst_MultiTrackCompositor.moc"
