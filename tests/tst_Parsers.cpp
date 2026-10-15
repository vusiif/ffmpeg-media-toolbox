#include <QtTest>

#include "ffmpeg/FFmpegCapabilities.h"
#include "ffmpeg/FFmpegLocator.h"
#include "media/MediaProbeService.h"

class ParsersTest : public QObject
{
    Q_OBJECT

private slots:
    void capabilitiesListEntries();
    void capabilitiesDeviceRowKeepsRealName();
    void capabilitiesEncoderList();
    void capabilitiesHwAccelsPlainList();
    void capabilitiesEmptyOutput();

    void probeJsonFullFile();
    void probeJsonAudioOnlyDetectsAudio();
    void probeJsonImageDetectsImage();
    void probeJsonInvalidReturnsEmpty();
    void detectMediaTypeByFormatName();

    void filterEncodersKeepsCandidatesBeforeScan();
    void encoderCandidateListsAreNonEmpty();

    void scanRunsAsynchronouslyAndKeepsFilteringSemantics();
    void overlappingScanRequestsCoalesce();
    void destroyWhileScanningIsSafe();
};

void ParsersTest::capabilitiesListEntries()
{
    const QString output = QStringLiteral(
        "File formats:\n"
        " D.. = Demuxing supported\n"
        " .E. = Muxing supported\n"
        " ..d = Is a device\n"
        " ---\n"
        " D   3dostr          3DO STR\n"
        " DE  ac3             raw AC-3\n"
        "  E  mp4             MP4 (MPEG-4 Part 14)\n"
        " D   mov,mp4,m4a,3gp,3g2,mj2 QuickTime / MOV\n"
        " --\n");

    const QSet<QString> names = FFmpegCapabilities::parseListOutput(output);
    QCOMPARE(names.size(), 4);
    QVERIFY(names.contains(QStringLiteral("3dostr")));
    QVERIFY(names.contains(QStringLiteral("ac3")));
    QVERIFY(names.contains(QStringLiteral("mp4")));
    QVERIFY(names.contains(QStringLiteral("mov,mp4,m4a,3gp,3g2,mj2")));
}

void ParsersTest::capabilitiesDeviceRowKeepsRealName()
{
    const QString output = QStringLiteral(
        "File formats:\n"
        " D.. = Demuxing supported\n"
        " ..d = Is a device\n"
        " ---\n"
        " D d dshow           DirectShow capture\n"
        " D d gdigrab         GDI API Windows frame grabber\n");

    const QSet<QString> names = FFmpegCapabilities::parseListOutput(output);
    QCOMPARE(names.size(), 2);
    QVERIFY(names.contains(QStringLiteral("dshow")));
    QVERIFY(names.contains(QStringLiteral("gdigrab")));
    QVERIFY(!names.contains(QStringLiteral("d")));
}

void ParsersTest::capabilitiesEncoderList()
{
    const QString output = QStringLiteral(
        "Encoders:\n"
        " V..... = Video\n"
        " A..... = Audio\n"
        " ------\n"
        " V....D libx264             libx264 H.264 / AVC (codec h264)\n"
        " A....D aac                  AAC (Advanced Audio Coding)\n"
        " V..... libsvtav1            SVT-AV1 encoder (codec av1)\n"
        " S..... srt                  SubRip subtitle (codec subrip)\n");

    const QSet<QString> names = FFmpegCapabilities::parseListOutput(output);
    QCOMPARE(names.size(), 4);
    QVERIFY(names.contains(QStringLiteral("libx264")));
    QVERIFY(names.contains(QStringLiteral("aac")));
    QVERIFY(names.contains(QStringLiteral("libsvtav1")));
    QVERIFY(names.contains(QStringLiteral("srt")));
}

void ParsersTest::capabilitiesHwAccelsPlainList()
{
    const QString output = QStringLiteral(
        "Hardware acceleration methods:\n"
        "cuda\n"
        "vaapi\n"
        "dxva2\n"
        "qsv\n"
        "d3d11va\n"
        "opencl\n"
        "vulkan\n"
        "d3d12va\n"
        "amf\n");

    const QSet<QString> names = FFmpegCapabilities::parseListOutput(output);
    QCOMPARE(names.size(), 9);
    QVERIFY(names.contains(QStringLiteral("cuda")));
    QVERIFY(names.contains(QStringLiteral("amf")));
    QVERIFY(!names.contains(QStringLiteral("hardware acceleration methods:")));
}

void ParsersTest::capabilitiesEmptyOutput()
{
    QVERIFY(FFmpegCapabilities::parseListOutput(QString()).isEmpty());
    QVERIFY(FFmpegCapabilities::parseListOutput(QStringLiteral("nothing here\n")).isEmpty());
}

void ParsersTest::probeJsonFullFile()
{
    const QString json = QStringLiteral(
        "{"
        "\"format\": {"
        "  \"format_name\": \"mov,mp4,m4a,3gp,3g2,mj2\","
        "  \"size\": \"123456\","
        "  \"duration\": \"12.500000\","
        "  \"bit_rate\": \"800000\","
        "  \"tags\": { \"title\": \"Demo\", \"artist\": \"Nobody\" }"
        "},"
        "\"streams\": ["
        "  { \"index\": 0, \"codec_type\": \"video\", \"codec_name\": \"h264\","
        "    \"width\": 1920, \"height\": 1080, \"pix_fmt\": \"yuv420p\","
        "    \"bit_rate\": \"600000\", \"r_frame_rate\": \"30000/1001\","
        "    \"tags\": { \"language\": \"und\" } },"
        "  { \"index\": 1, \"codec_type\": \"audio\", \"codec_name\": \"aac\","
        "    \"sample_rate\": \"48000\", \"channels\": 2,"
        "    \"channel_layout\": \"stereo\", \"bit_rate\": \"128000\","
        "    \"tags\": {} },"
        "  { \"index\": 2, \"codec_type\": \"subtitle\", \"codec_name\": \"mov_text\","
        "    \"tags\": { \"language\": \"eng\" } }"
        "]"
        "}");

    const MediaFile media = MediaProbeService::parseProbeJson(json, QStringLiteral("C:/in.mp4"));

    QVERIFY(media.isValid());
    QCOMPARE(media.path, QStringLiteral("C:/in.mp4"));
    QCOMPARE(media.type, MediaType::Video);
    QCOMPARE(media.formatName, QStringLiteral("mov,mp4,m4a,3gp,3g2,mj2"));
    QCOMPARE(media.fileSize, 123456);
    QCOMPARE(media.duration, 12.5);
    QCOMPARE(media.bitrate, 800000);
    QCOMPARE(media.formatMetadata.value(QStringLiteral("title")).toString(),
             QStringLiteral("Demo"));
    QCOMPARE(media.formatMetadata.value(QStringLiteral("artist")).toString(),
             QStringLiteral("Nobody"));

    QCOMPARE(media.videoStreams.size(), 1);
    const VideoStream &video = media.primaryVideo();
    QCOMPARE(video.index, 0);
    QCOMPARE(video.codec, QStringLiteral("h264"));
    QCOMPARE(video.width, 1920);
    QCOMPARE(video.height, 1080);
    QCOMPARE(video.pixelFormat, QStringLiteral("yuv420p"));
    QCOMPARE(video.bitrate, 600000);
    QVERIFY(qAbs(video.frameRate - 30000.0 / 1001.0) < 0.001);
    QCOMPARE(video.metadata.value(QStringLiteral("language")).toString(),
             QStringLiteral("und"));

    QCOMPARE(media.audioStreams.size(), 1);
    const AudioStream &audio = media.primaryAudio();
    QCOMPARE(audio.codec, QStringLiteral("aac"));
    QCOMPARE(audio.sampleRate, 48000);
    QCOMPARE(audio.channels, 2);
    QCOMPARE(audio.channelLayout, QStringLiteral("stereo"));
    QCOMPARE(audio.bitrate, 128000);

    QCOMPARE(media.subtitleStreams.size(), 1);
    QCOMPARE(media.subtitleStreams.first().codec, QStringLiteral("mov_text"));
    QCOMPARE(media.subtitleStreams.first().language, QStringLiteral("eng"));
}

void ParsersTest::probeJsonAudioOnlyDetectsAudio()
{
    const QString json = QStringLiteral(
        "{"
        "\"format\": { \"format_name\": \"mp3\", \"duration\": \"3.0\","
        "  \"size\": \"100\", \"bit_rate\": \"320000\" },"
        "\"streams\": ["
        "  { \"index\": 0, \"codec_type\": \"audio\", \"codec_name\": \"mp3\","
        "    \"sample_rate\": \"44100\", \"channels\": 2 }"
        "]"
        "}");

    const MediaFile media = MediaProbeService::parseProbeJson(json, QStringLiteral("song.mp3"));
    QCOMPARE(media.type, MediaType::Audio);
    QVERIFY(!media.hasVideo());
    QVERIFY(media.hasAudio());
}

void ParsersTest::probeJsonImageDetectsImage()
{
    const QString json = QStringLiteral(
        "{"
        "\"format\": { \"format_name\": \"png_pipe\", \"duration\": \"0.0\","
        "  \"size\": \"2048\", \"bit_rate\": \"N/A\" },"
        "\"streams\": ["
        "  { \"index\": 0, \"codec_type\": \"video\", \"codec_name\": \"png\","
        "    \"width\": 64, \"height\": 64, \"r_frame_rate\": \"25/1\" }"
        "]"
        "}");

    const MediaFile media = MediaProbeService::parseProbeJson(json, QStringLiteral("pic.png"));
    QCOMPARE(media.videoStreams.size(), 1);
    QCOMPARE(media.videoStreams.first().frameRate, 25.0);
}

void ParsersTest::probeJsonInvalidReturnsEmpty()
{
    QVERIFY(!MediaProbeService::parseProbeJson(QString(), QStringLiteral("x")).isValid());
    QVERIFY(!MediaProbeService::parseProbeJson(QStringLiteral("not json"),
                                               QStringLiteral("x")).isValid());
}

void ParsersTest::detectMediaTypeByFormatName()
{
    QCOMPARE(MediaProbeService::detectMediaType(QStringLiteral("png_pipe"), {}, {}),
             MediaType::Image);
    QCOMPARE(MediaProbeService::detectMediaType(QStringLiteral("image2"), {}, {}),
             MediaType::Image);
    QCOMPARE(MediaProbeService::detectMediaType(QStringLiteral("matroska"), {}, {}),
             MediaType::Unknown);
    QCOMPARE(MediaProbeService::detectMediaType(QStringLiteral("mp3"), {},
                                                {AudioStream()}),
             MediaType::Audio);
    QCOMPARE(MediaProbeService::detectMediaType(QStringLiteral("mp4"), {VideoStream()},
                                                {AudioStream()}),
             MediaType::Video);
}

void ParsersTest::filterEncodersKeepsCandidatesBeforeScan()
{
    // Until the first scan() the encoder sets are empty; candidates must pass
    // through so the codec combo is never left empty.
    FFmpegCapabilities caps(nullptr);
    QVERIFY(!caps.isLoaded());

    const QStringList candidates = {QStringLiteral("libx264"),
                                    QStringLiteral("h264_nvenc")};
    QCOMPARE(caps.filterEncoders(candidates), candidates);
    QVERIFY(caps.filterEncoders({}).isEmpty());
}

void ParsersTest::encoderCandidateListsAreNonEmpty()
{
    const QStringList video = FFmpegCapabilities::videoEncoderCandidates();
    const QStringList audio = FFmpegCapabilities::audioEncoderCandidates();
    QVERIFY(!video.isEmpty());
    QVERIFY(!audio.isEmpty());
    QVERIFY(video.contains(QStringLiteral("libx264")));
    QVERIFY(video.contains(QStringLiteral("h264_nvenc")));
    QVERIFY(audio.contains(QStringLiteral("aac")));
}

void ParsersTest::scanRunsAsynchronouslyAndKeepsFilteringSemantics()
{
    FFmpegLocator locator;
    locator.waitForDetection();
    if (!locator.isValid()) {
        QSKIP("ffmpeg/ffprobe not available");
    }

    FFmpegCapabilities caps(&locator);
    QVERIFY(!caps.isLoaded());

    QVERIFY(caps.scan());
    // scan() must return immediately; the result only lands via the event loop.
    QVERIFY(!caps.isLoaded());

    QTRY_VERIFY_WITH_TIMEOUT(caps.isLoaded(), 90000);
    QVERIFY(!caps.isScanning());
    QVERIFY(caps.encoderCount() > 0);

    // Filtering semantics: output is always a subset of the candidates, and a
    // software encoder is offered exactly when the scan found it.
    const QStringList in = {QStringLiteral("libx264"), QStringLiteral("h264_nvenc")};
    const QStringList out = caps.filterEncoders(in);
    QVERIFY(out.size() <= in.size());
    for (const QString &encoder : out) {
        QVERIFY(in.contains(encoder));
    }
    QCOMPARE(out.contains(QStringLiteral("libx264")),
             caps.hasEncoder(QStringLiteral("libx264")));
}

void ParsersTest::overlappingScanRequestsCoalesce()
{
    FFmpegLocator locator;
    locator.waitForDetection();
    if (!locator.isValid()) {
        QSKIP("ffmpeg/ffprobe not available");
    }

    FFmpegCapabilities caps(&locator);
    QVERIFY(caps.scan());
    QVERIFY(caps.scan());
    QVERIFY(caps.scan());
    QVERIFY(caps.isScanning());

    QTRY_VERIFY_WITH_TIMEOUT(caps.isLoaded(), 90000);
    QTRY_VERIFY_WITH_TIMEOUT(!caps.isScanning(), 90000);
    QVERIFY(caps.encoderCount() > 0);
}

void ParsersTest::destroyWhileScanningIsSafe()
{
    FFmpegLocator locator;
    locator.waitForDetection();
    if (!locator.isValid()) {
        QSKIP("ffmpeg/ffprobe not available");
    }

    auto *caps = new FFmpegCapabilities(&locator);
    QVERIFY(caps->scan());
    // Deleting mid-flight must not leave the worker touching freed memory.
    delete caps;
    QTest::qWait(200);
    QVERIFY(true);
}

QTEST_GUILESS_MAIN(ParsersTest)
#include "tst_Parsers.moc"
