#include <QtTest>

#include "media/MediaFile.h"
#include "output/OutputEstimate.h"

namespace {

MediaFile makeVideoFile()
{
    MediaFile media;
    media.path = QStringLiteral("C:/media/in.mp4");
    media.duration = 10.0;
    media.fileSize = 123456;

    VideoStream video;
    video.width = 1920;
    video.height = 1080;
    video.frameRate = 24.0;
    video.bitrate = 4000000;
    media.videoStreams.append(video);

    AudioStream audio;
    audio.sampleRate = 48000;
    audio.channels = 2;
    audio.bitrate = 128000;
    media.audioStreams.append(audio);

    return media;
}

} // namespace

class OutputEstimateTest : public QObject
{
    Q_OBJECT

private slots:
    void copyReturnsInputFileSize();
    void copyFallsBackToStreamBitrates();
    void reencodeFollowsCrfModel();
    void higherCrfYieldsSmallerEstimate();
    void audioOnlyEstimate();
    void losslessAudioEstimates();
    void noDurationReturnsZero();
    void invalidMediaReturnsZero();
    void missingVideoDimensionsReturnZero();
    void copyWithoutKnownBitrateReturnsZero();
    void humanizeBytesFormats();
};

void OutputEstimateTest::copyReturnsInputFileSize()
{
    QCOMPARE(OutputEstimate::estimateBytes(makeVideoFile(), QStringLiteral("libx264"),
                                           QStringLiteral("aac"), 23, true),
             123456);
}

void OutputEstimateTest::copyFallsBackToStreamBitrates()
{
    MediaFile media = makeVideoFile();
    media.fileSize = 0;

    // (4 Mbps video + 128 kbps audio) * 10 s / 8
    QCOMPARE(OutputEstimate::estimateBytes(media, QString(), QString(), -1, true),
             5160000);
}

void OutputEstimateTest::reencodeFollowsCrfModel()
{
    // CRF 23 -> 0.08 bpp * 1920 * 1080 * 24 fps + 128 kbps audio, over 10 s.
    const qint64 estimate = OutputEstimate::estimateBytes(makeVideoFile(),
                                                          QStringLiteral("libx264"),
                                                          QStringLiteral("aac"), 23, false);
    QVERIFY(qAbs(estimate - 5136640) <= 2);
}

void OutputEstimateTest::higherCrfYieldsSmallerEstimate()
{
    const qint64 lowCrf = OutputEstimate::estimateBytes(makeVideoFile(),
                                                        QStringLiteral("libx264"),
                                                        QStringLiteral("aac"), 18, false);
    const qint64 highCrf = OutputEstimate::estimateBytes(makeVideoFile(),
                                                         QStringLiteral("libx264"),
                                                         QStringLiteral("aac"), 30, false);
    QVERIFY(lowCrf > 0);
    QVERIFY(highCrf > 0);
    QVERIFY(highCrf < lowCrf);
}

void OutputEstimateTest::audioOnlyEstimate()
{
    MediaFile media;
    media.path = QStringLiteral("C:/media/song.mp3");
    media.duration = 10.0;

    AudioStream audio;
    audio.sampleRate = 44100;
    audio.channels = 2;
    audio.bitrate = 128000;
    media.audioStreams.append(audio);

    // 128 kbps * 10 s / 8
    QCOMPARE(OutputEstimate::estimateBytes(media, QString(), QStringLiteral("aac"), 23, false),
             160000);
}

void OutputEstimateTest::losslessAudioEstimates()
{
    MediaFile media;
    media.path = QStringLiteral("C:/media/take.wav");
    media.duration = 10.0;

    AudioStream audio;
    audio.sampleRate = 48000;
    audio.channels = 2;
    audio.bitrate = 0;
    media.audioStreams.append(audio);

    // pcm_s16le: 48000 * 2 * 16 bits * 10 s / 8
    QCOMPARE(OutputEstimate::estimateBytes(media, QString(), QStringLiteral("pcm_s16le"),
                                           -1, false),
             1920000);

    // flac: 60% of pcm_s16le
    QCOMPARE(OutputEstimate::estimateBytes(media, QString(), QStringLiteral("flac"),
                                           -1, false),
             1152000);
}

void OutputEstimateTest::noDurationReturnsZero()
{
    MediaFile media = makeVideoFile();
    media.duration = 0;
    QCOMPARE(OutputEstimate::estimateBytes(media, QString(), QString(), -1, false), 0);
    QCOMPARE(OutputEstimate::estimateBytes(media, QString(), QString(), -1, true), 0);
}

void OutputEstimateTest::invalidMediaReturnsZero()
{
    QCOMPARE(OutputEstimate::estimateBytes(MediaFile(), QStringLiteral("libx264"),
                                           QStringLiteral("aac"), 23, false),
             0);
}

void OutputEstimateTest::missingVideoDimensionsReturnZero()
{
    MediaFile media = makeVideoFile();
    media.videoStreams.first().width = 0;
    QCOMPARE(OutputEstimate::estimateBytes(media, QStringLiteral("libx264"),
                                           QStringLiteral("aac"), 23, false),
             0);
}

void OutputEstimateTest::copyWithoutKnownBitrateReturnsZero()
{
    MediaFile media = makeVideoFile();
    media.fileSize = 0;
    media.videoStreams.first().bitrate = 0;
    QCOMPARE(OutputEstimate::estimateBytes(media, QString(), QString(), -1, true), 0);
}

void OutputEstimateTest::humanizeBytesFormats()
{
    QCOMPARE(OutputEstimate::humanizeBytes(0), QString());
    QCOMPARE(OutputEstimate::humanizeBytes(-5), QString());
    QCOMPARE(OutputEstimate::humanizeBytes(1023), QStringLiteral("1023 B"));
    QCOMPARE(OutputEstimate::humanizeBytes(1024), QStringLiteral("1.0 KB"));
    QCOMPARE(OutputEstimate::humanizeBytes(1572864), QStringLiteral("1.5 MB"));
    QCOMPARE(OutputEstimate::humanizeBytes(1610612736), QStringLiteral("1.5 GB"));
}

QTEST_GUILESS_MAIN(OutputEstimateTest)
#include "tst_OutputEstimate.moc"
