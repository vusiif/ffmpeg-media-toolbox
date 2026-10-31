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
    void conversionAdvancedAudioSettings();
    void conversionAdvancedCodecAndFilters();
    void conversionAdvancedIgnoredWhenCopyStreams();
    void conversionColorSpaceTags();
    void conversionHdrTonemapChainsWithVideoFilter();
    void conversionHdrTonemapAlone();
    void conversionHwAccelPrependedBeforeInput();
    void conversionTargetFileSizeCalculatesBitrate();
    void cropdetectCommandAndParser();
    void subtitleMuxConvertsForMp4();
    void subtitleMuxCopiesWhenCodecIsEmpty();
    void subtitleMuxRejectsEmptyPaths();
    void escapeFilterPathQuotesForFfmpeg();
    void subtitleBurninBuildsFilterAndMaps();
    void subtitleBurninOmitsCrfAndRejectsEmpty();
    void imageOperationCropsOnly();
    void imageOperationResizesOnly();
    void imageOperationRotates();
    void imageOperationChainsFiltersInOrder();
    void imageOperationAddsFormatAndQuality();
    void imageOperationRejectsEmptyPaths();
    void videoAdjustBuildsVfAndCrf();
    void videoAdjustWithoutOperationReturnsEmpty();
    void videoAdjustRejectsEmptyPaths();
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
    void pipDefaultScalingAndPosition();
    void pipCustomScaleAndMargin();
    void pipEachPosition();
    void pipRejectsEmptyPaths();
    void pipWithCrf();
    void formatTimePadsComponents();
    void escapeFilterValueEscapesMetaChars();
    void canFallbackToSoftwareReplacesHwEncodersAndDropsHwaccel();
    void canFallbackToSoftwareLeavesPlainSoftwareCommandUntouched();

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

void FFmpegCommandBuilderTest::conversionAdvancedAudioSettings()
{
    FFmpegCommandBuilder::ConversionOptions opts;
    opts.inputPath = QStringLiteral("in.mp4");
    opts.outputPath = QStringLiteral("out.mp4");
    opts.audioCodec = QStringLiteral("aac");
    opts.audioBitrateKbps = 192;
    opts.audioSampleRate = 48000;
    opts.audioChannels = 2;
    opts.audioNormalize = true;

    QCOMPARE(m_builder.buildConversion(opts),
             QStringList({QStringLiteral("-y"),
                          QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("-c:a"), QStringLiteral("aac"),
                          QStringLiteral("-b:a"), QStringLiteral("192k"),
                          QStringLiteral("-ar"), QStringLiteral("48000"),
                          QStringLiteral("-ac"), QStringLiteral("2"),
                          QStringLiteral("-af"), QStringLiteral("loudnorm"),
                          QStringLiteral("out.mp4")}));
}

void FFmpegCommandBuilderTest::conversionAdvancedCodecAndFilters()
{
    FFmpegCommandBuilder::ConversionOptions opts;
    opts.inputPath = QStringLiteral("in.mp4");
    opts.outputPath = QStringLiteral("out.mkv");
    opts.videoCodec = QStringLiteral("libx264");
    opts.crf = 20;
    opts.preset = QStringLiteral("fast");
    opts.pixelFormat = QStringLiteral("yuv420p");
    opts.videoFilter = QStringLiteral("unsharp");
    opts.audioFilter = QStringLiteral("volume=1.5");

    QCOMPARE(m_builder.buildConversion(opts),
             QStringList({QStringLiteral("-y"),
                          QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("-c:v"), QStringLiteral("libx264"),
                          QStringLiteral("-crf"), QStringLiteral("20"),
                          QStringLiteral("-preset"), QStringLiteral("fast"),
                          QStringLiteral("-pix_fmt"), QStringLiteral("yuv420p"),
                          QStringLiteral("-vf"), QStringLiteral("unsharp"),
                          QStringLiteral("-af"), QStringLiteral("volume=1.5"),
                          QStringLiteral("out.mkv")}));
}

void FFmpegCommandBuilderTest::conversionAdvancedIgnoredWhenCopyStreams()
{
    FFmpegCommandBuilder::ConversionOptions opts;
    opts.inputPath = QStringLiteral("in.mp4");
    opts.outputPath = QStringLiteral("out.mp4");
    opts.copyStreams = true;
    opts.preset = QStringLiteral("slow");
    opts.audioBitrateKbps = 320;
    opts.videoFilter = QStringLiteral("scale=1280:720");

    // Copy stream overrides encoders, presets, filters, bitrates.
    QCOMPARE(m_builder.buildConversion(opts),
             QStringList({QStringLiteral("-y"),
                          QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("-c"), QStringLiteral("copy"),
                          QStringLiteral("out.mp4")}));
}

void FFmpegCommandBuilderTest::conversionColorSpaceTags()
{
    FFmpegCommandBuilder::ConversionOptions opts;
    opts.inputPath = QStringLiteral("in.mp4");
    opts.outputPath = QStringLiteral("out.mp4");
    opts.videoCodec = QStringLiteral("libx264");
    opts.colorSpace = QStringLiteral("bt709");
    opts.colorPrimaries = QStringLiteral("bt709");
    opts.colorTrc = QStringLiteral("bt709");

    QCOMPARE(m_builder.buildConversion(opts),
             QStringList({QStringLiteral("-y"),
                          QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("-c:v"), QStringLiteral("libx264"),
                          QStringLiteral("-colorspace"), QStringLiteral("bt709"),
                          QStringLiteral("-color_primaries"), QStringLiteral("bt709"),
                          QStringLiteral("-color_trc"), QStringLiteral("bt709"),
                          QStringLiteral("out.mp4")}));
}

void FFmpegCommandBuilderTest::conversionHdrTonemapChainsWithVideoFilter()
{
    FFmpegCommandBuilder::ConversionOptions opts;
    opts.inputPath = QStringLiteral("in.mp4");
    opts.outputPath = QStringLiteral("out.mp4");
    opts.videoCodec = QStringLiteral("libx264");
    opts.hdrTonemap = QStringLiteral("hable");
    opts.videoFilter = QStringLiteral("scale=1920:1080");

    QCOMPARE(m_builder.buildConversion(opts),
             QStringList({QStringLiteral("-y"),
                          QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("-c:v"), QStringLiteral("libx264"),
                          QStringLiteral("-vf"), QStringLiteral("tonemap=hable,scale=1920:1080"),
                          QStringLiteral("out.mp4")}));
}

void FFmpegCommandBuilderTest::conversionHdrTonemapAlone()
{
    FFmpegCommandBuilder::ConversionOptions opts;
    opts.inputPath = QStringLiteral("in.mp4");
    opts.outputPath = QStringLiteral("out.mp4");
    opts.videoCodec = QStringLiteral("libx264");
    opts.hdrTonemap = QStringLiteral("mobius");

    QCOMPARE(m_builder.buildConversion(opts),
             QStringList({QStringLiteral("-y"),
                          QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("-c:v"), QStringLiteral("libx264"),
                          QStringLiteral("-vf"), QStringLiteral("tonemap=mobius"),
                          QStringLiteral("out.mp4")}));
}

void FFmpegCommandBuilderTest::conversionHwAccelPrependedBeforeInput()
{
    FFmpegCommandBuilder::ConversionOptions opts;
    opts.inputPath = QStringLiteral("in.mp4");
    opts.outputPath = QStringLiteral("out.mp4");
    opts.videoCodec = QStringLiteral("h264_nvenc");
    opts.hwAccel = QStringLiteral("cuda");
    opts.crf = 23;

    QCOMPARE(m_builder.buildConversion(opts),
             QStringList({QStringLiteral("-y"),
                          QStringLiteral("-hwaccel"), QStringLiteral("cuda"),
                          QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("-c:v"), QStringLiteral("h264_nvenc"),
                          QStringLiteral("-crf"), QStringLiteral("23"),
                          QStringLiteral("out.mp4")}));
}

void FFmpegCommandBuilderTest::conversionTargetFileSizeCalculatesBitrate()
{
    FFmpegCommandBuilder::ConversionOptions opts;
    opts.inputPath = QStringLiteral("in.mp4");
    opts.outputPath = QStringLiteral("out.mp4");
    opts.videoCodec = QStringLiteral("libx264");
    opts.targetSizeMB = 25.0;
    opts.durationSeconds = 100.0;
    // 25 * 8192 / 100 = 2048 kbps total. Default 128 kbps audio -> 1920 kbps video.
    // maxrate = 1920 * 1.5 = 2880k, bufsize = 1920 * 2 = 3840k.
    const QStringList args = m_builder.buildConversion(opts);
    QVERIFY(!args.contains(QStringLiteral("-crf")));
    QVERIFY(args.contains(QStringLiteral("-b:v")));
    const int bIdx = args.indexOf(QStringLiteral("-b:v"));
    QCOMPARE(args.value(bIdx + 1), QStringLiteral("1920k"));
    QVERIFY(args.contains(QStringLiteral("-maxrate")));
    const int maxIdx = args.indexOf(QStringLiteral("-maxrate"));
    QCOMPARE(args.value(maxIdx + 1), QStringLiteral("2880k"));
    QVERIFY(args.contains(QStringLiteral("-bufsize")));
    const int bufIdx = args.indexOf(QStringLiteral("-bufsize"));
    QCOMPARE(args.value(bufIdx + 1), QStringLiteral("3840k"));
    QVERIFY(args.contains(QStringLiteral("-b:a")));
    const int aIdx = args.indexOf(QStringLiteral("-b:a"));
    QCOMPARE(args.value(aIdx + 1), QStringLiteral("128k"));
}

void FFmpegCommandBuilderTest::cropdetectCommandAndParser()
{
    const QStringList args = m_builder.buildCropdetect(QStringLiteral("input.mp4"), 15.0, 30);
    QCOMPARE(args, QStringList({
        QStringLiteral("-y"),
        QStringLiteral("-ss"), QStringLiteral("00:00:15"),
        QStringLiteral("-i"), QStringLiteral("input.mp4"),
        QStringLiteral("-vf"), QStringLiteral("cropdetect=24:16:0"),
        QStringLiteral("-vframes"), QStringLiteral("30"),
        QStringLiteral("-f"), QStringLiteral("null"),
        QStringLiteral("-")
    }));

    const QString log = QStringLiteral(
        "[Parsed_cropdetect_0 @ 000001a] x1:0 x2:1919 y1:140 y2:939 w:1920 h:800 x:0 y:140 pts:1200 t:1.200 crop=1920:800:0:140\n"
        "[Parsed_cropdetect_0 @ 000001a] x1:0 x2:1919 y1:140 y2:939 w:1920 h:800 x:0 y:140 pts:1240 t:1.240 crop=1920:800:0:140\n"
    );
    const FFmpegCommandBuilder::CropArea crop = FFmpegCommandBuilder::parseCropdetectOutput(log);
    QVERIFY(crop.isValid());
    QCOMPARE(crop.w, 1920);
    QCOMPARE(crop.h, 800);
    QCOMPARE(crop.x, 0);
    QCOMPARE(crop.y, 140);
}

void FFmpegCommandBuilderTest::subtitleMuxConvertsForMp4()
{
    QCOMPARE(m_builder.buildSubtitleMux(QStringLiteral("in.mp4"),
                                        QStringLiteral("subs.srt"),
                                        QStringLiteral("out.mp4"),
                                        QStringLiteral("mov_text")),
             QStringList({QStringLiteral("-y"),
                          QStringLiteral("-i"), QStringLiteral("in.mp4"),
                          QStringLiteral("-i"), QStringLiteral("subs.srt"),
                          QStringLiteral("-map"), QStringLiteral("0"),
                          QStringLiteral("-map"), QStringLiteral("1"),
                          QStringLiteral("-c"), QStringLiteral("copy"),
                          QStringLiteral("-c:s"), QStringLiteral("mov_text"),
                          QStringLiteral("out.mp4")}));
}

void FFmpegCommandBuilderTest::subtitleMuxCopiesWhenCodecIsEmpty()
{
    QCOMPARE(m_builder.buildSubtitleMux(QStringLiteral("in.mkv"),
                                        QStringLiteral("subs.ass"),
                                        QStringLiteral("out.mkv"),
                                        QString()),
             QStringList({QStringLiteral("-y"),
                          QStringLiteral("-i"), QStringLiteral("in.mkv"),
                          QStringLiteral("-i"), QStringLiteral("subs.ass"),
                          QStringLiteral("-map"), QStringLiteral("0"),
                          QStringLiteral("-map"), QStringLiteral("1"),
                          QStringLiteral("-c"), QStringLiteral("copy"),
                          QStringLiteral("out.mkv")}));
}

void FFmpegCommandBuilderTest::subtitleMuxRejectsEmptyPaths()
{
    QVERIFY(m_builder.buildSubtitleMux(QString(), QStringLiteral("s.srt"),
                                       QStringLiteral("o.mp4"),
                                       QStringLiteral("mov_text")).isEmpty());
    QVERIFY(m_builder.buildSubtitleMux(QStringLiteral("in.mp4"), QString(),
                                       QStringLiteral("o.mp4"),
                                       QStringLiteral("mov_text")).isEmpty());
    QVERIFY(m_builder.buildSubtitleMux(QStringLiteral("in.mp4"),
                                       QStringLiteral("s.srt"), QString(),
                                       QStringLiteral("mov_text")).isEmpty());
}

void FFmpegCommandBuilderTest::escapeFilterPathQuotesForFfmpeg()
{
    // Windows path: slashes normalized, colon escaped, whole thing quoted.
    QCOMPARE(FFmpegCommandBuilder::escapeFilterPath(QStringLiteral("C:\\media\\sub.srt")),
             QStringLiteral("'C\\:/media/sub.srt'"));

    // Plain relative paths stay as they are (still quoted).
    QCOMPARE(FFmpegCommandBuilder::escapeFilterPath(QStringLiteral("subs.srt")),
             QStringLiteral("'subs.srt'"));

    // Embedded quotes must not break out of the wrapping quotes.
    QCOMPARE(FFmpegCommandBuilder::escapeFilterPath(QStringLiteral("it's.srt")),
             QStringLiteral("'it\\'s.srt'"));
}

void FFmpegCommandBuilderTest::subtitleBurninBuildsFilterAndMaps()
{
    QCOMPARE(m_builder.buildSubtitleBurnin(QStringLiteral("C:\\media\\in.mp4"),
                                           QStringLiteral("C:\\media\\sub.srt"),
                                           QStringLiteral("C:\\media\\out.mp4"),
                                           23),
             QStringList({QStringLiteral("-y"),
                          QStringLiteral("-i"), QStringLiteral("C:\\media\\in.mp4"),
                          QStringLiteral("-i"), QStringLiteral("C:\\media\\sub.srt"),
                          QStringLiteral("-filter_complex"),
                          QStringLiteral("[0:v][1:s]subtitles=filename='C\\:/media/sub.srt'[v]"),
                          QStringLiteral("-map"), QStringLiteral("[v]"),
                          QStringLiteral("-map"), QStringLiteral("0:a?"),
                          QStringLiteral("-c:a"), QStringLiteral("copy"),
                          QStringLiteral("-crf"), QStringLiteral("23"),
                          QStringLiteral("C:\\media\\out.mp4")}));
}

void FFmpegCommandBuilderTest::subtitleBurninOmitsCrfAndRejectsEmpty()
{
    const QStringList args = m_builder.buildSubtitleBurnin(QStringLiteral("in.mp4"),
                                                           QStringLiteral("subs.srt"),
                                                           QStringLiteral("out.mp4"), -1);
    QVERIFY(!args.contains(QStringLiteral("-crf")));

    QVERIFY(m_builder.buildSubtitleBurnin(QString(), QStringLiteral("s.srt"),
                                          QStringLiteral("o.mp4"), 23).isEmpty());
    QVERIFY(m_builder.buildSubtitleBurnin(QStringLiteral("in.mp4"), QString(),
                                          QStringLiteral("o.mp4"), 23).isEmpty());
    QVERIFY(m_builder.buildSubtitleBurnin(QStringLiteral("in.mp4"),
                                          QStringLiteral("s.srt"), QString(),
                                          23).isEmpty());
}

void FFmpegCommandBuilderTest::imageOperationCropsOnly()
{
    // crop takes W:H:X:Y.
    QCOMPARE(m_builder.buildImageOperation(QStringLiteral("in.png"),
                                           QStringLiteral("out.png"),
                                           10, 20, 30, 40),
             QStringList({QStringLiteral("-y"), QStringLiteral("-i"),
                          QStringLiteral("in.png"),
                          QStringLiteral("-vf"), QStringLiteral("crop=30:40:10:20"),
                          QStringLiteral("out.png")}));
}

void FFmpegCommandBuilderTest::imageOperationResizesOnly()
{
    QCOMPARE(m_builder.buildImageOperation(QStringLiteral("in.png"),
                                           QStringLiteral("out.png"),
                                           0, 0, 0, 0, 640, 480),
             QStringList({QStringLiteral("-y"), QStringLiteral("-i"),
                          QStringLiteral("in.png"),
                          QStringLiteral("-vf"), QStringLiteral("scale=640:480"),
                          QStringLiteral("out.png")}));
}

void FFmpegCommandBuilderTest::imageOperationRotates()
{
    const auto rotateArgs = [this](int rotate) {
        return m_builder.buildImageOperation(QStringLiteral("in.png"),
                                             QStringLiteral("out.png"),
                                             0, 0, 0, 0, 0, 0, rotate);
    };

    QCOMPARE(rotateArgs(90).at(4), QStringLiteral("transpose=1"));
    QCOMPARE(rotateArgs(180).at(4), QStringLiteral("transpose=1,transpose=1"));
    QCOMPARE(rotateArgs(270).at(4), QStringLiteral("transpose=2"));
    QCOMPARE(rotateArgs(0).at(3), QStringLiteral("out.png")); // no -vf at all
}

void FFmpegCommandBuilderTest::imageOperationChainsFiltersInOrder()
{
    QCOMPARE(m_builder.buildImageOperation(QStringLiteral("in.png"),
                                           QStringLiteral("out.png"),
                                           10, 20, 30, 40, 640, 480, 90,
                                           true, true),
             QStringList({QStringLiteral("-y"), QStringLiteral("-i"),
                          QStringLiteral("in.png"),
                          QStringLiteral("-vf"),
                          QStringLiteral("crop=30:40:10:20,scale=640:480,"
                                         "transpose=1,hflip,vflip"),
                          QStringLiteral("out.png")}));
}

void FFmpegCommandBuilderTest::imageOperationAddsFormatAndQuality()
{
    QCOMPARE(m_builder.buildImageOperation(QStringLiteral("in.png"),
                                           QStringLiteral("out.webp"),
                                           0, 0, 0, 0, 640, 480,
                                           0, false, false,
                                           QStringLiteral("webp"), 7),
             QStringList({QStringLiteral("-y"), QStringLiteral("-i"),
                          QStringLiteral("in.png"),
                          QStringLiteral("-vf"), QStringLiteral("scale=640:480"),
                          QStringLiteral("-f"), QStringLiteral("webp"),
                          QStringLiteral("-q:v"), QStringLiteral("7"),
                          QStringLiteral("out.webp")}));
}

void FFmpegCommandBuilderTest::imageOperationRejectsEmptyPaths()
{
    QVERIFY(m_builder.buildImageOperation(QString(), QStringLiteral("out.png"),
                                          10, 20, 30, 40).isEmpty());
    QVERIFY(m_builder.buildImageOperation(QStringLiteral("in.png"), QString(),
                                          10, 20, 30, 40).isEmpty());
}

void FFmpegCommandBuilderTest::videoAdjustBuildsVfAndCrf()
{
    QCOMPARE(m_builder.buildVideoAdjust(QStringLiteral("in.mkv"),
                                        QStringLiteral("out.mkv"),
                                        0, 0, 0, 0, 1280, 720, 90, 20),
             QStringList({QStringLiteral("-y"), QStringLiteral("-i"),
                          QStringLiteral("in.mkv"),
                          QStringLiteral("-vf"),
                          QStringLiteral("scale=1280:720,transpose=1"),
                          QStringLiteral("-crf"), QStringLiteral("20"),
                          QStringLiteral("out.mkv")}));
}

void FFmpegCommandBuilderTest::videoAdjustWithoutOperationReturnsEmpty()
{
    // Nothing selected: no point queueing a job.
    QVERIFY(m_builder.buildVideoAdjust(QStringLiteral("in.mkv"),
                                       QStringLiteral("out.mkv"),
                                       0, 0, 0, 0, 0, 0, 0, 23).isEmpty());

    // Negative crf keeps the encoder default but still builds the filter.
    const QStringList args = m_builder.buildVideoAdjust(QStringLiteral("in.mkv"),
                                                        QStringLiteral("out.mkv"),
                                                        0, 0, 0, 0, 0, 0, 270, -1);
    QCOMPARE(args.at(4), QStringLiteral("transpose=2"));
    QVERIFY(!args.contains(QStringLiteral("-crf")));
}

void FFmpegCommandBuilderTest::videoAdjustRejectsEmptyPaths()
{
    QVERIFY(m_builder.buildVideoAdjust(QString(), QStringLiteral("out.mkv"),
                                       10, 10, 100, 100, 0, 0, 0, 23).isEmpty());
    QVERIFY(m_builder.buildVideoAdjust(QStringLiteral("in.mkv"), QString(),
                                       10, 10, 100, 100, 0, 0, 0, 23).isEmpty());
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

void FFmpegCommandBuilderTest::pipDefaultScalingAndPosition()
{
    // Defaults: pipWidth=0, pipHeight=0 -> iw/3:-1; position=bottom-right, margin=10; crf=-1 (omitted)
    QCOMPARE(m_builder.buildPip(QStringLiteral("main.mp4"), QStringLiteral("pip.mp4"),
                                QStringLiteral("out.mp4")),
             QStringList({QStringLiteral("-y"),
                          QStringLiteral("-i"), QStringLiteral("main.mp4"),
                          QStringLiteral("-i"), QStringLiteral("pip.mp4"),
                          QStringLiteral("-filter_complex"),
                          QStringLiteral("[1:v]scale=iw/3:-1[pip];[0:v][pip]overlay=main_w-overlay_w-10:main_h-overlay_h-10"),
                          QStringLiteral("-map"), QStringLiteral("0:a?"),
                          QStringLiteral("out.mp4")}));
}

void FFmpegCommandBuilderTest::pipCustomScaleAndMargin()
{
    // Explicit width and height, custom margin 20
    QCOMPARE(m_builder.buildPip(QStringLiteral("main.mp4"), QStringLiteral("pip.mp4"),
                                QStringLiteral("out.mp4"),
                                320, 180, QStringLiteral("top-left"), 20),
             QStringList({QStringLiteral("-y"),
                          QStringLiteral("-i"), QStringLiteral("main.mp4"),
                          QStringLiteral("-i"), QStringLiteral("pip.mp4"),
                          QStringLiteral("-filter_complex"),
                          QStringLiteral("[1:v]scale=320:180[pip];[0:v][pip]overlay=20:20"),
                          QStringLiteral("-map"), QStringLiteral("0:a?"),
                          QStringLiteral("out.mp4")}));

    // Width only (height -1)
    const QStringList wOnly = m_builder.buildPip(QStringLiteral("main.mp4"), QStringLiteral("pip.mp4"),
                                                 QStringLiteral("out.mp4"),
                                                 480, 0, QStringLiteral("top-left"), 10);
    QCOMPARE(wOnly.at(6), QStringLiteral("[1:v]scale=480:-1[pip];[0:v][pip]overlay=10:10"));

    // Height only (width -1)
    const QStringList hOnly = m_builder.buildPip(QStringLiteral("main.mp4"), QStringLiteral("pip.mp4"),
                                                 QStringLiteral("out.mp4"),
                                                 0, 240, QStringLiteral("top-left"), 10);
    QCOMPARE(hOnly.at(6), QStringLiteral("[1:v]scale=-1:240[pip];[0:v][pip]overlay=10:10"));
}

void FFmpegCommandBuilderTest::pipEachPosition()
{
    const auto extractOverlay = [this](const QString &pos) {
        const QStringList args = m_builder.buildPip(QStringLiteral("main.mp4"), QStringLiteral("pip.mp4"),
                                                    QStringLiteral("out.mp4"),
                                                    320, 180, pos, 15);
        // args[6] is the -filter_complex string
        return args.at(6).section(QStringLiteral(";[0:v][pip]overlay="), 1);
    };

    QCOMPARE(extractOverlay(QStringLiteral("top-left")), QStringLiteral("15:15"));
    QCOMPARE(extractOverlay(QStringLiteral("top-right")), QStringLiteral("main_w-overlay_w-15:15"));
    QCOMPARE(extractOverlay(QStringLiteral("bottom-left")), QStringLiteral("15:main_h-overlay_h-15"));
    QCOMPARE(extractOverlay(QStringLiteral("bottom-right")), QStringLiteral("main_w-overlay_w-15:main_h-overlay_h-15"));
    QCOMPARE(extractOverlay(QStringLiteral("center")), QStringLiteral("(main_w-overlay_w)/2:(main_h-overlay_h)/2"));
}

void FFmpegCommandBuilderTest::pipRejectsEmptyPaths()
{
    QVERIFY(m_builder.buildPip(QString(), QStringLiteral("pip.mp4"), QStringLiteral("out.mp4")).isEmpty());
    QVERIFY(m_builder.buildPip(QStringLiteral("main.mp4"), QString(), QStringLiteral("out.mp4")).isEmpty());
    QVERIFY(m_builder.buildPip(QStringLiteral("main.mp4"), QStringLiteral("pip.mp4"), QString()).isEmpty());
}

void FFmpegCommandBuilderTest::pipWithCrf()
{
    const QStringList args = m_builder.buildPip(QStringLiteral("main.mp4"), QStringLiteral("pip.mp4"),
                                                QStringLiteral("out.mp4"),
                                                320, 240, QStringLiteral("center"), 0, 21);
    QVERIFY(args.contains(QStringLiteral("-crf")));
    const int idx = args.indexOf(QStringLiteral("-crf"));
    QCOMPARE(args.at(idx + 1), QStringLiteral("21"));
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

void FFmpegCommandBuilderTest::canFallbackToSoftwareReplacesHwEncodersAndDropsHwaccel()
{
    const QStringList hwCmd = {
        QStringLiteral("-y"),
        QStringLiteral("-hwaccel"), QStringLiteral("cuda"),
        QStringLiteral("-i"), QStringLiteral("input.mp4"),
        QStringLiteral("-c:v"), QStringLiteral("h264_nvenc"),
        QStringLiteral("-preset"), QStringLiteral("p4"),
        QStringLiteral("-c:a"), QStringLiteral("aac"),
        QStringLiteral("output.mp4")
    };

    QStringList swCmd;
    const bool changed = FFmpegCommandBuilder::canFallbackToSoftware(hwCmd, &swCmd);
    QVERIFY(changed);

    QVERIFY(!swCmd.contains(QStringLiteral("-hwaccel")));
    QVERIFY(!swCmd.contains(QStringLiteral("cuda")));
    QVERIFY(!swCmd.contains(QStringLiteral("h264_nvenc")));
    QVERIFY(!swCmd.contains(QStringLiteral("p4")));

    // Replaced with libx264 and medium
    const int cvIdx = swCmd.indexOf(QStringLiteral("-c:v"));
    QVERIFY(cvIdx >= 0 && cvIdx + 1 < swCmd.size());
    QCOMPARE(swCmd.at(cvIdx + 1), QStringLiteral("libx264"));

    const int presetIdx = swCmd.indexOf(QStringLiteral("-preset"));
    QVERIFY(presetIdx >= 0 && presetIdx + 1 < swCmd.size());
    QCOMPARE(swCmd.at(presetIdx + 1), QStringLiteral("medium"));

    // Audio and other options preserved
    QVERIFY(swCmd.contains(QStringLiteral("aac")));
    QVERIFY(swCmd.contains(QStringLiteral("output.mp4")));
}

void FFmpegCommandBuilderTest::canFallbackToSoftwareLeavesPlainSoftwareCommandUntouched()
{
    const QStringList swCmd = {
        QStringLiteral("-y"),
        QStringLiteral("-i"), QStringLiteral("input.mp4"),
        QStringLiteral("-c:v"), QStringLiteral("libx264"),
        QStringLiteral("-c:a"), QStringLiteral("aac"),
        QStringLiteral("output.mp4")
    };

    QStringList result;
    const bool changed = FFmpegCommandBuilder::canFallbackToSoftware(swCmd, &result);
    QVERIFY(!changed);
}

QTEST_GUILESS_MAIN(FFmpegCommandBuilderTest)
#include "tst_FFmpegCommandBuilder.moc"
