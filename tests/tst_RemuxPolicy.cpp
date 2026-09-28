#include <QtTest>

#include "conversion/RemuxPolicy.h"
#include "media/MediaFile.h"

namespace {

MediaFile makeMedia(const QString &videoCodec, const QString &audioCodec)
{
    MediaFile media;
    media.path = QStringLiteral("C:/media/in.mkv");
    if (!videoCodec.isEmpty()) {
        VideoStream video;
        video.codec = videoCodec;
        media.videoStreams.append(video);
    }
    if (!audioCodec.isEmpty()) {
        AudioStream audio;
        audio.codec = audioCodec;
        media.audioStreams.append(audio);
    }
    return media;
}

} // namespace

class RemuxPolicyTest : public QObject
{
    Q_OBJECT

private slots:
    void unknownFormatCopiesNothing();
    void emptyCodecNeverCopies();
    void mp4SupportsCommonCodecs();
    void mkvIsPermissive();
    void webmRejectsH264();
    void audioOnlyContainerRejectsVideo();
    void canCopyAllStreamsRequiresEveryStream();
    void canCopyAllStreamsRejectsEmptyMedia();
    void canCopyByExtensionOnlyWithinSameContainer();
    void canCopyByExtensionRejectsMissingSuffix();
    void subtitleCodecPerContainer();
};

void RemuxPolicyTest::unknownFormatCopiesNothing()
{
    QVERIFY(!RemuxPolicy::containerSupportsVideoCodec(QStringLiteral("xyz"), QStringLiteral("h264")));
    QVERIFY(!RemuxPolicy::containerSupportsAudioCodec(QStringLiteral("xyz"), QStringLiteral("aac")));
    QVERIFY(!RemuxPolicy::containerSupportsVideoCodec(QString(), QStringLiteral("h264")));
}

void RemuxPolicyTest::emptyCodecNeverCopies()
{
    QVERIFY(!RemuxPolicy::containerSupportsVideoCodec(QStringLiteral("mp4"), QString()));
    QVERIFY(!RemuxPolicy::containerSupportsAudioCodec(QStringLiteral("mkv"), QString()));
}

void RemuxPolicyTest::mp4SupportsCommonCodecs()
{
    QVERIFY(RemuxPolicy::containerSupportsVideoCodec(QStringLiteral("mp4"), QStringLiteral("h264")));
    QVERIFY(RemuxPolicy::containerSupportsVideoCodec(QStringLiteral("mp4"), QStringLiteral("hevc")));
    QVERIFY(RemuxPolicy::containerSupportsVideoCodec(QStringLiteral("mov"), QStringLiteral("h264")));
    QVERIFY(RemuxPolicy::containerSupportsAudioCodec(QStringLiteral("mp4"), QStringLiteral("aac")));

    QVERIFY(!RemuxPolicy::containerSupportsVideoCodec(QStringLiteral("mp4"), QStringLiteral("vp8")));
    QVERIFY(!RemuxPolicy::containerSupportsVideoCodec(QStringLiteral("mp4"), QStringLiteral("vorbis")));
    QVERIFY(!RemuxPolicy::containerSupportsAudioCodec(QStringLiteral("mp4"), QStringLiteral("vorbis")));
}

void RemuxPolicyTest::mkvIsPermissive()
{
    QVERIFY(RemuxPolicy::containerSupportsVideoCodec(QStringLiteral("mkv"), QStringLiteral("vp9")));
    QVERIFY(RemuxPolicy::containerSupportsVideoCodec(QStringLiteral("mkv"), QStringLiteral("theora")));
    QVERIFY(RemuxPolicy::containerSupportsAudioCodec(QStringLiteral("mkv"), QStringLiteral("flac")));
    QVERIFY(RemuxPolicy::containerSupportsAudioCodec(QStringLiteral("mkv"), QStringLiteral("vorbis")));
}

void RemuxPolicyTest::webmRejectsH264()
{
    QVERIFY(RemuxPolicy::containerSupportsVideoCodec(QStringLiteral("webm"), QStringLiteral("vp9")));
    QVERIFY(RemuxPolicy::containerSupportsAudioCodec(QStringLiteral("webm"), QStringLiteral("opus")));

    QVERIFY(!RemuxPolicy::containerSupportsVideoCodec(QStringLiteral("webm"), QStringLiteral("h264")));
    QVERIFY(!RemuxPolicy::containerSupportsAudioCodec(QStringLiteral("webm"), QStringLiteral("aac")));
}

void RemuxPolicyTest::audioOnlyContainerRejectsVideo()
{
    QVERIFY(!RemuxPolicy::containerSupportsVideoCodec(QStringLiteral("mp3"), QStringLiteral("h264")));
    QVERIFY(RemuxPolicy::containerSupportsAudioCodec(QStringLiteral("mp3"), QStringLiteral("mp3")));
    QVERIFY(RemuxPolicy::containerSupportsAudioCodec(QStringLiteral("wav"), QStringLiteral("pcm_s16le")));
    QVERIFY(!RemuxPolicy::containerSupportsAudioCodec(QStringLiteral("wav"), QStringLiteral("aac")));
}

void RemuxPolicyTest::canCopyAllStreamsRequiresEveryStream()
{
    QVERIFY(RemuxPolicy::canCopyAllStreams(makeMedia(QStringLiteral("h264"), QStringLiteral("aac")),
                                            QStringLiteral("mp4")));

    QVERIFY(!RemuxPolicy::canCopyAllStreams(makeMedia(QStringLiteral("vp8"), QStringLiteral("aac")),
                                             QStringLiteral("mp4")));
    QVERIFY(!RemuxPolicy::canCopyAllStreams(makeMedia(QStringLiteral("h264"), QStringLiteral("vorbis")),
                                             QStringLiteral("mp4")));

    QVERIFY(RemuxPolicy::canCopyAllStreams(makeMedia(QStringLiteral("vp9"), QStringLiteral("opus")),
                                            QStringLiteral("webm")));
    QVERIFY(RemuxPolicy::canCopyAllStreams(makeMedia(QStringLiteral("h264"), QString()),
                                            QStringLiteral("mp4")));
}

void RemuxPolicyTest::canCopyAllStreamsRejectsEmptyMedia()
{
    QVERIFY(!RemuxPolicy::canCopyAllStreams(MediaFile(), QStringLiteral("mp4")));
    QVERIFY(!RemuxPolicy::canCopyAllStreams(makeMedia(QString(), QString()), QStringLiteral("mp4")));

    MediaFile invalid;
    invalid.path = QString();
    VideoStream video;
    video.codec = QStringLiteral("h264");
    invalid.videoStreams.append(video);
    QVERIFY(!RemuxPolicy::canCopyAllStreams(invalid, QStringLiteral("mp4")));
}

void RemuxPolicyTest::canCopyByExtensionOnlyWithinSameContainer()
{
    QVERIFY(RemuxPolicy::canCopyByExtension(QStringLiteral("C:/media/in.mkv"),
                                            QStringLiteral("mkv")));
    QVERIFY(RemuxPolicy::canCopyByExtension(QStringLiteral("C:/media/IN.MKV"),
                                            QStringLiteral("mkv")));

    QVERIFY(!RemuxPolicy::canCopyByExtension(QStringLiteral("C:/media/in.mkv"),
                                             QStringLiteral("mp4")));
    QVERIFY(!RemuxPolicy::canCopyByExtension(QStringLiteral("C:/media/in.webm"),
                                             QStringLiteral("mkv")));
}

void RemuxPolicyTest::canCopyByExtensionRejectsMissingSuffix()
{
    QVERIFY(!RemuxPolicy::canCopyByExtension(QStringLiteral("C:/media/in"), QStringLiteral("mp4")));
    QVERIFY(!RemuxPolicy::canCopyByExtension(QStringLiteral("C:/media/in.mkv"), QString()));
}

void RemuxPolicyTest::subtitleCodecPerContainer()
{
    // mp4/mov need a conversion codec...
    QCOMPARE(*RemuxPolicy::subtitleCodecForContainer(QStringLiteral("mp4")),
             QStringLiteral("mov_text"));
    QCOMPARE(*RemuxPolicy::subtitleCodecForContainer(QStringLiteral("MOV")),
             QStringLiteral("mov_text"));

    // ...mkv accepts the text streams as they are (present, empty codec)...
    const auto mkv = RemuxPolicy::subtitleCodecForContainer(QStringLiteral("mkv"));
    QVERIFY(mkv.has_value());
    QCOMPARE(*mkv, QString());

    // ...webm needs webvtt...
    QCOMPARE(*RemuxPolicy::subtitleCodecForContainer(QStringLiteral("webm")),
             QStringLiteral("webvtt"));

    // ...and containers that cannot carry subtitles refuse outright.
    QVERIFY(!RemuxPolicy::subtitleCodecForContainer(QStringLiteral("avi")).has_value());
    QVERIFY(!RemuxPolicy::subtitleCodecForContainer(QStringLiteral("mp3")).has_value());
    QVERIFY(!RemuxPolicy::subtitleCodecForContainer(QString()).has_value());
}

QTEST_GUILESS_MAIN(RemuxPolicyTest)
#include "tst_RemuxPolicy.moc"
