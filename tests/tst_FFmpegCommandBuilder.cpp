#include <QtTest>

#include "ffmpeg/FFmpegCommandBuilder.h"

class FFmpegCommandBuilderTest : public QObject
{
    Q_OBJECT

private slots:
    void conversionRejectsEmptyPaths();
    void conversionWithoutCodecs();
    void conversionWithCodecsAndCrf();
    void conversionCopyOverridesCodecs();
    void conversionStripsMetadataOnRequest();
    void conversionKeepsMetadataByDefault();
    void trimPlacesSeekBeforeInput();
    void trimZeroTimesOmitSeek();
    void trimCopyWithFormat();
    void extractAudioWithBitrate();
    void extractAudioWithCodecOnly();
    void muteWithCopy();
    void muteWithCrf();
    void screenshotSeeksAndGrabsOneFrame();
    void screenshotZeroTimeOmitsSeek();
    void gifBuildsFpsScaleFilter();
    void gifDefaultsFpsAndDropsScale();
    void videoJoinBuildsConcatFilter();
    void videoJoinNeedsAtLeastTwoInputs();
    void gridJoinBuildsXstackLayout();
    void gridJoinAddsPaddingAndColor();
    void gridJoinClampsColumns();
    void gridJoinAddsOuterMargin();
    void imageSequenceToVideo();
    void videoToImageSequence();
    void videoToImageSequenceZeroFpsDropsFilter();
    void watermarkBottomRightDefault();
    void watermarkEachPosition();
    void watermarkClampsOpacity();
    void watermarkAppendsOutputFormat();
    void formatTimePadsComponents();
    void escapeFilterValueEscapesMetaChars();

private:
    FFmpegCommandBuilder m_builder;
};

void FFmpegCommandBuilderTest::conversionRejectsEmptyPaths()
{
    QVERIFY(m_builder.buildConversion(QString(), QStringLiteral("out.mkv")).isEmpty());
    QVERIFY(m_builder.buildConversion(QStringLiteral("in.mp4"), QString()).isEmpty());
}

void FFmpegCommandBuilderTest::conversionWithoutCodecs()
{
    QCOMPARE(m_builder.buildConversion(QStringLiteral("in.mp4"), QStringLiteral("out.mkv")),
             QStringList({QStringLiteral("-y"), QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("out.mkv")}));
}

void FFmpegCommandBuilderTest::conversionWithCodecsAndCrf()
{
    QCOMPARE(m_builder.buildConversion(QStringLiteral("in.mp4"), QStringLiteral("out.mkv"),
                                       QStringLiteral("libx264"),
                                       QStringLiteral("aac"), 23, false),
             QStringList({QStringLiteral("-y"), QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("-c:v"), QStringLiteral("libx264"),
                          QStringLiteral("-c:a"), QStringLiteral("aac"),
                          QStringLiteral("-crf"), QStringLiteral("23"),
                          QStringLiteral("out.mkv")}));
}

void FFmpegCommandBuilderTest::conversionCopyOverridesCodecs()
{
    QCOMPARE(m_builder.buildConversion(QStringLiteral("in.mp4"), QStringLiteral("out.mkv"),
                                       QStringLiteral("libx264"),
                                       QStringLiteral("aac"), 23, true),
             QStringList({QStringLiteral("-y"), QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("-c"), QStringLiteral("copy"),
                          QStringLiteral("out.mkv")}));
}

void FFmpegCommandBuilderTest::conversionStripsMetadataOnRequest()
{
    QCOMPARE(m_builder.buildConversion(QStringLiteral("in.mp4"), QStringLiteral("out.mkv"),
                                       QStringLiteral("libx264"), QString(), 23, false, true),
             QStringList({QStringLiteral("-y"), QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("-c:v"), QStringLiteral("libx264"),
                          QStringLiteral("-crf"), QStringLiteral("23"),
                          QStringLiteral("-map_metadata"), QStringLiteral("-1"),
                          QStringLiteral("out.mkv")}));

    QCOMPARE(m_builder.buildConversion(QStringLiteral("in.mp4"), QStringLiteral("out.mkv"),
                                       QString(), QString(), -1, true, true),
             QStringList({QStringLiteral("-y"), QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("-c"), QStringLiteral("copy"),
                          QStringLiteral("-map_metadata"), QStringLiteral("-1"),
                          QStringLiteral("out.mkv")}));
}

void FFmpegCommandBuilderTest::conversionKeepsMetadataByDefault()
{
    const QStringList args = m_builder.buildConversion(QStringLiteral("in.mp4"),
                                                       QStringLiteral("out.mkv"),
                                                       QStringLiteral("libx264"));
    QVERIFY(!args.contains(QStringLiteral("-map_metadata")));
}

void FFmpegCommandBuilderTest::trimPlacesSeekBeforeInput()
{
    QCOMPARE(m_builder.buildTrim(QStringLiteral("in.mp4"), QStringLiteral("out.mp4"),
                                5.0, 10.0),
             QStringList({QStringLiteral("-y"), QStringLiteral("-ss"), QStringLiteral("00:00:05"),
                          QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("-to"), QStringLiteral("00:00:10"),
                          QStringLiteral("out.mp4")}));
}

void FFmpegCommandBuilderTest::trimZeroTimesOmitSeek()
{
    QCOMPARE(m_builder.buildTrim(QStringLiteral("in.mp4"), QStringLiteral("out.mp4"), 0, 0),
             QStringList({QStringLiteral("-y"), QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("out.mp4")}));
}

void FFmpegCommandBuilderTest::trimCopyWithFormat()
{
    QCOMPARE(m_builder.buildTrim(QStringLiteral("in.mp4"), QStringLiteral("out.mkv"),
                                0, 0, QStringLiteral("mkv"),
                                QString(), QString(), -1, true),
             QStringList({QStringLiteral("-y"), QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("-c"), QStringLiteral("copy"),
                          QStringLiteral("-f"), QStringLiteral("mkv"),
                          QStringLiteral("out.mkv")}));
}

void FFmpegCommandBuilderTest::extractAudioWithBitrate()
{
    QCOMPARE(m_builder.buildExtractAudio(QStringLiteral("in.mp4"), QStringLiteral("out.mp3"),
                                         QStringLiteral("mp3"), QString(), 192),
             QStringList({QStringLiteral("-y"), QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("-vn"),
                          QStringLiteral("-b:a"), QStringLiteral("192k"),
                          QStringLiteral("-f"), QStringLiteral("mp3"),
                          QStringLiteral("out.mp3")}));
}

void FFmpegCommandBuilderTest::extractAudioWithCodecOnly()
{
    QCOMPARE(m_builder.buildExtractAudio(QStringLiteral("in.mp4"), QStringLiteral("out.m4a"),
                                         QString(), QStringLiteral("copy")),
             QStringList({QStringLiteral("-y"), QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("-vn"),
                          QStringLiteral("-c:a"), QStringLiteral("copy"),
                          QStringLiteral("out.m4a")}));
}

void FFmpegCommandBuilderTest::muteWithCopy()
{
    QCOMPARE(m_builder.buildMute(QStringLiteral("in.mp4"), QStringLiteral("out.mp4"),
                                 QString(), QString(), -1, true),
             QStringList({QStringLiteral("-y"), QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("-an"),
                          QStringLiteral("-c:v"), QStringLiteral("copy"),
                          QStringLiteral("out.mp4")}));
}

void FFmpegCommandBuilderTest::muteWithCrf()
{
    QCOMPARE(m_builder.buildMute(QStringLiteral("in.mp4"), QStringLiteral("out.mp4"),
                                 QString(), QStringLiteral("libx264"), 20, false),
             QStringList({QStringLiteral("-y"), QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("-an"),
                          QStringLiteral("-c:v"), QStringLiteral("libx264"),
                          QStringLiteral("-crf"), QStringLiteral("20"),
                          QStringLiteral("out.mp4")}));
}

void FFmpegCommandBuilderTest::screenshotSeeksAndGrabsOneFrame()
{
    QCOMPARE(m_builder.buildScreenshot(QStringLiteral("in.mp4"), QStringLiteral("shot.png"),
                                       12.5, QStringLiteral("png")),
             QStringList({QStringLiteral("-y"), QStringLiteral("-ss"), QStringLiteral("00:00:12"),
                          QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("-frames:v"), QStringLiteral("1"),
                          QStringLiteral("-f"), QStringLiteral("png"),
                          QStringLiteral("shot.png")}));
}

void FFmpegCommandBuilderTest::screenshotZeroTimeOmitsSeek()
{
    QCOMPARE(m_builder.buildScreenshot(QStringLiteral("in.mp4"), QStringLiteral("shot.jpg"), 0),
             QStringList({QStringLiteral("-y"), QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("-frames:v"), QStringLiteral("1"),
                          QStringLiteral("shot.jpg")}));
}

void FFmpegCommandBuilderTest::gifBuildsFpsScaleFilter()
{
    QCOMPARE(m_builder.buildGif(QStringLiteral("in.mp4"), QStringLiteral("out.gif"),
                                12, 480, 1.0, 3.0),
             QStringList({QStringLiteral("-y"),
                          QStringLiteral("-ss"), QStringLiteral("00:00:01"),
                          QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("-to"), QStringLiteral("00:00:03"),
                          QStringLiteral("-vf"),
                          QStringLiteral("fps=12,scale=480:-1:flags=lanczos"),
                          QStringLiteral("-loop"), QStringLiteral("0"),
                          QStringLiteral("out.gif")}));
}

void FFmpegCommandBuilderTest::gifDefaultsFpsAndDropsScale()
{
    QCOMPARE(m_builder.buildGif(QStringLiteral("in.mp4"), QStringLiteral("out.gif"), 0, 0, 0, 0),
             QStringList({QStringLiteral("-y"), QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("-vf"), QStringLiteral("fps=12"),
                          QStringLiteral("-loop"), QStringLiteral("0"),
                          QStringLiteral("out.gif")}));
}

void FFmpegCommandBuilderTest::videoJoinBuildsConcatFilter()
{
    QCOMPARE(m_builder.buildVideoJoin({QStringLiteral("a.mp4"), QStringLiteral("b.mp4"),
                                       QStringLiteral("c.mp4")},
                                      QStringLiteral("out.mp4")),
             QStringList({QStringLiteral("-y"),
                          QStringLiteral("-i"), QStringLiteral("a.mp4"),
                          QStringLiteral("-i"), QStringLiteral("b.mp4"),
                          QStringLiteral("-i"), QStringLiteral("c.mp4"),
                          QStringLiteral("-filter_complex"),
                          QStringLiteral("[0:v][0:a][1:v][1:a][2:v][2:a]"
                                         "concat=n=3:v=1:a=1[outv][outa]"),
                          QStringLiteral("-map"), QStringLiteral("[outv]"),
                          QStringLiteral("-map"), QStringLiteral("[outa]"),
                          QStringLiteral("out.mp4")}));
}

void FFmpegCommandBuilderTest::videoJoinNeedsAtLeastTwoInputs()
{
    QVERIFY(m_builder.buildVideoJoin({QStringLiteral("a.mp4")}, QStringLiteral("out.mp4")).isEmpty());
    QVERIFY(m_builder.buildVideoJoin({}, QStringLiteral("out.mp4")).isEmpty());
    QVERIFY(m_builder.buildVideoJoin({QStringLiteral("a.mp4"), QStringLiteral("b.mp4")},
                                     QString()).isEmpty());
}

void FFmpegCommandBuilderTest::gridJoinBuildsXstackLayout()
{
    QCOMPARE(m_builder.buildGridJoin({QStringLiteral("a.png"), QStringLiteral("b.png")},
                                       QStringLiteral("out.png"), 2, 0, QString()),
             QStringList({QStringLiteral("-y"),
                          QStringLiteral("-i"), QStringLiteral("a.png"),
                          QStringLiteral("-i"), QStringLiteral("b.png"),
                          QStringLiteral("-filter_complex"),
                          QStringLiteral("[0:v][1:v]xstack=inputs=2:layout=0_0|1*w0_0[outv]"),
                          QStringLiteral("-map"), QStringLiteral("[outv]"),
                          QStringLiteral("out.png")}));
}

void FFmpegCommandBuilderTest::gridJoinAddsPaddingAndColor()
{
    QCOMPARE(m_builder.buildGridJoin({QStringLiteral("a.png"), QStringLiteral("b.png")},
                                       QStringLiteral("out.png"), 2, 10,
                                       QStringLiteral("white")),
             QStringList({QStringLiteral("-y"),
                          QStringLiteral("-i"), QStringLiteral("a.png"),
                          QStringLiteral("-i"), QStringLiteral("b.png"),
                          QStringLiteral("-filter_complex"),
                          QStringLiteral("[0:v]pad=iw+10:ih+10:10:10:color=white[p0];"
                                         "[1:v]pad=iw+10:ih+10:10:10:color=white[p1];"
                                         "[p0][p1]xstack=inputs=2:layout=0_0|1*w0_0[outv]"),
                          QStringLiteral("-map"), QStringLiteral("[outv]"),
                          QStringLiteral("out.png")}));
}

void FFmpegCommandBuilderTest::gridJoinClampsColumns()
{
    QCOMPARE(m_builder.buildGridJoin({QStringLiteral("a.png"), QStringLiteral("b.png")},
                                       QStringLiteral("out.png"), 0, 5, QString()),
             QStringList({QStringLiteral("-y"),
                          QStringLiteral("-i"), QStringLiteral("a.png"),
                          QStringLiteral("-i"), QStringLiteral("b.png"),
                          QStringLiteral("-filter_complex"),
                          QStringLiteral("[0:v]pad=iw+5:ih+5:5:5:color=black[p0];"
                                         "[1:v]pad=iw+5:ih+5:5:5:color=black[p1];"
                                         "[p0][p1]xstack=inputs=2:layout=0_0|0_1*h0[outv]"),
                          QStringLiteral("-map"), QStringLiteral("[outv]"),
                          QStringLiteral("out.png")}));
}

void FFmpegCommandBuilderTest::gridJoinAddsOuterMargin()
{
    QCOMPARE(m_builder.buildGridJoin({QStringLiteral("a.png"), QStringLiteral("b.png")},
                                     QStringLiteral("out.png"), 2, 0,
                                     QStringLiteral("white"), 10),
             QStringList({QStringLiteral("-y"),
                          QStringLiteral("-i"), QStringLiteral("a.png"),
                          QStringLiteral("-i"), QStringLiteral("b.png"),
                          QStringLiteral("-filter_complex"),
                          QStringLiteral("[0:v][1:v]xstack=inputs=2:layout=0_0|1*w0_0[outv];"
                                         "[outv]pad=iw+20:ih+20:10:10:color=white[outp]"),
                          QStringLiteral("-map"), QStringLiteral("[outp]"),
                          QStringLiteral("out.png")}));
}

void FFmpegCommandBuilderTest::imageSequenceToVideo()
{
    QCOMPARE(m_builder.buildImageSequenceToVideo(QStringLiteral("frames/%04d.png"),
                                                 QStringLiteral("out.mp4"), 25),
             QStringList({QStringLiteral("-y"),
                          QStringLiteral("-framerate"), QStringLiteral("25"),
                          QStringLiteral("-i"), QStringLiteral("frames/%04d.png"),
                          QStringLiteral("-pix_fmt"), QStringLiteral("yuv420p"),
                          QStringLiteral("out.mp4")}));

    QCOMPARE(m_builder.buildImageSequenceToVideo(QStringLiteral("frames/%04d.png"),
                                                 QStringLiteral("out.mp4"), 0),
             QStringList({QStringLiteral("-y"),
                          QStringLiteral("-framerate"), QStringLiteral("25"),
                          QStringLiteral("-i"), QStringLiteral("frames/%04d.png"),
                          QStringLiteral("-pix_fmt"), QStringLiteral("yuv420p"),
                          QStringLiteral("out.mp4")}));
}

void FFmpegCommandBuilderTest::videoToImageSequence()
{
    QCOMPARE(m_builder.buildVideoToImageSequence(QStringLiteral("in.mp4"),
                                                 QStringLiteral("f/%04d.png"), 5.0),
             QStringList({QStringLiteral("-y"), QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("-vf"), QStringLiteral("fps=5"),
                          QStringLiteral("f/%04d.png")}));
}

void FFmpegCommandBuilderTest::videoToImageSequenceZeroFpsDropsFilter()
{
    QCOMPARE(m_builder.buildVideoToImageSequence(QStringLiteral("in.mp4"),
                                                 QStringLiteral("f/%04d.png"), 0),
             QStringList({QStringLiteral("-y"), QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("f/%04d.png")}));
}

void FFmpegCommandBuilderTest::watermarkBottomRightDefault()
{
    QCOMPARE(m_builder.buildWatermark(QStringLiteral("in.mp4"), QStringLiteral("wm.png"),
                                      QStringLiteral("out.mp4"),
                                      QStringLiteral("bottom-right"), 100),
             QStringList({QStringLiteral("-y"),
                          QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("-i"), QStringLiteral("wm.png"),
                          QStringLiteral("-filter_complex"),
                          QStringLiteral("[1:v]format=rgba,colorchannelmixer=aa=1[wm];"
                                         "[0:v][wm]overlay=main_w-overlay_w-10:main_h-overlay_h-10"),
                          QStringLiteral("-map"), QStringLiteral("0:a?"),
                          QStringLiteral("out.mp4")}));
}

void FFmpegCommandBuilderTest::watermarkEachPosition()
{
    const struct {
        const char *position;
        const char *overlay;
    } cases[] = {
        {"top-left", "10:10"},
        {"top-right", "main_w-overlay_w-10:10"},
        {"bottom-left", "10:main_h-overlay_h-10"},
        {"center", "(main_w-overlay_w)/2:(main_h-overlay_h)/2"},
        {"bottom-right", "main_w-overlay_w-10:main_h-overlay_h-10"},
    };

    for (const auto &c : cases) {
        const QStringList args =
            m_builder.buildWatermark(QStringLiteral("in.mp4"), QStringLiteral("wm.png"),
                                     QStringLiteral("out.mp4"),
                                     QString::fromLatin1(c.position), 100);
        QCOMPARE(args.size(), 10);
        QVERIFY2(args.at(6).contains(QString::fromLatin1(c.overlay)),
                 qPrintable(QStringLiteral("position %1: %2")
                                .arg(QString::fromLatin1(c.position), args.at(6))));
    }
}

void FFmpegCommandBuilderTest::watermarkClampsOpacity()
{
    const QStringList high =
        m_builder.buildWatermark(QStringLiteral("in.mp4"), QStringLiteral("wm.png"),
                                 QStringLiteral("out.mp4"),
                                 QStringLiteral("center"), 200);
    QVERIFY(high.at(6).contains(QStringLiteral("aa=1")));

    const QStringList low =
        m_builder.buildWatermark(QStringLiteral("in.mp4"), QStringLiteral("wm.png"),
                                 QStringLiteral("out.mp4"),
                                 QStringLiteral("center"), -5);
    QVERIFY(low.at(6).contains(QStringLiteral("aa=0")));

    const QStringList half =
        m_builder.buildWatermark(QStringLiteral("in.mp4"), QStringLiteral("wm.png"),
                                 QStringLiteral("out.mp4"),
                                 QStringLiteral("center"), 50);
    QVERIFY(half.at(6).contains(QStringLiteral("aa=0.5")));
}

void FFmpegCommandBuilderTest::watermarkAppendsOutputFormat()
{
    QCOMPARE(m_builder.buildWatermark(QStringLiteral("in.mp4"), QStringLiteral("wm.png"),
                                      QStringLiteral("out.mkv"),
                                      QStringLiteral("center"), 100,
                                      QStringLiteral("mkv")),
             QStringList({QStringLiteral("-y"),
                          QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("-i"), QStringLiteral("wm.png"),
                          QStringLiteral("-filter_complex"),
                          QStringLiteral("[1:v]format=rgba,colorchannelmixer=aa=1[wm];"
                                         "[0:v][wm]overlay=(main_w-overlay_w)/2:(main_h-overlay_h)/2"),
                          QStringLiteral("-map"), QStringLiteral("0:a?"),
                          QStringLiteral("-f"), QStringLiteral("mkv"),
                          QStringLiteral("out.mkv")}));
}

void FFmpegCommandBuilderTest::formatTimePadsComponents()
{
    QCOMPARE(FFmpegCommandBuilder::formatTime(0), QStringLiteral("00:00:00"));
    QCOMPARE(FFmpegCommandBuilder::formatTime(59), QStringLiteral("00:00:59"));
    QCOMPARE(FFmpegCommandBuilder::formatTime(60), QStringLiteral("00:01:00"));
    QCOMPARE(FFmpegCommandBuilder::formatTime(3600), QStringLiteral("01:00:00"));
    QCOMPARE(FFmpegCommandBuilder::formatTime(3661.7), QStringLiteral("01:01:01"));
    QCOMPARE(FFmpegCommandBuilder::formatTime(7325.2), QStringLiteral("02:02:05"));
}

void FFmpegCommandBuilderTest::escapeFilterValueEscapesMetaChars()
{
    QCOMPARE(FFmpegCommandBuilder::escapeFilterValue(QStringLiteral("plain")),
             QStringLiteral("plain"));
    QCOMPARE(FFmpegCommandBuilder::escapeFilterValue(QStringLiteral("a'b")),
             QStringLiteral("a\\'b"));
    QCOMPARE(FFmpegCommandBuilder::escapeFilterValue(QStringLiteral("a;b")),
             QStringLiteral("a\\;b"));
    QCOMPARE(FFmpegCommandBuilder::escapeFilterValue(QStringLiteral("a\\b")),
             QStringLiteral("a\\\\b"));
}

QTEST_GUILESS_MAIN(FFmpegCommandBuilderTest)
#include "tst_FFmpegCommandBuilder.moc"
