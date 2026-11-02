#include <QTest>
#include <QVariantMap>
#include <QStringList>
#include "audio/AudioTagEngine.h"

using namespace Audio;

class tst_AudioTagEngine : public QObject
{
    Q_OBJECT

private slots:
    void testDefaultMetadata();
    void testParseTagsMap();
    void testGuessFromFileName();
    void testQualityPresets();
    void testFormatConversions();
    void testBuildExtractArgsWithoutCover();
    void testBuildExtractArgsWithCoverMp3();
    void testBuildExtractArgsWithCoverFlac();
    void testBuildCoverExtractArgs();
};

void tst_AudioTagEngine::testDefaultMetadata()
{
    AudioMetadata meta;
    QVERIFY(meta.isEmpty());

    meta.title = QStringLiteral("Test Song");
    QVERIFY(!meta.isEmpty());

    meta.clear();
    QVERIFY(meta.isEmpty());
}

void tst_AudioTagEngine::testParseTagsMap()
{
    QVariantMap rawTags;
    rawTags[QStringLiteral("TITLE")] = QStringLiteral("Starman");
    rawTags[QStringLiteral("artist")] = QStringLiteral("David Bowie");
    rawTags[QStringLiteral("ALBUM")] = QStringLiteral("The Rise and Fall of Ziggy Stardust");
    rawTags[QStringLiteral("date")] = QStringLiteral("1972");
    rawTags[QStringLiteral("genre")] = QStringLiteral("Glam Rock");
    rawTags[QStringLiteral("track")] = QStringLiteral("4");

    AudioMetadata meta = AudioMetadata::parseTags(rawTags);
    QCOMPARE(meta.title, QStringLiteral("Starman"));
    QCOMPARE(meta.artist, QStringLiteral("David Bowie"));
    QCOMPARE(meta.album, QStringLiteral("The Rise and Fall of Ziggy Stardust"));
    QCOMPARE(meta.year, QStringLiteral("1972"));
    QCOMPARE(meta.genre, QStringLiteral("Glam Rock"));
    QCOMPARE(meta.track, QStringLiteral("4"));
}

void tst_AudioTagEngine::testGuessFromFileName()
{
    AudioMetadata meta1 = AudioMetadata::guessFromFileName(QStringLiteral("01 - Queen - Bohemian Rhapsody.flac"));
    QCOMPARE(meta1.artist, QStringLiteral("Queen"));
    QCOMPARE(meta1.title, QStringLiteral("Bohemian Rhapsody"));

    AudioMetadata meta2 = AudioMetadata::guessFromFileName(QStringLiteral("Taylor Swift - Blank Space.mp3"));
    QCOMPARE(meta2.artist, QStringLiteral("Taylor Swift"));
    QCOMPARE(meta2.title, QStringLiteral("Blank Space"));

    AudioMetadata meta3 = AudioMetadata::guessFromFileName(QStringLiteral("JustATrackName.wav"));
    QCOMPARE(meta3.title, QStringLiteral("JustATrackName"));
    QVERIFY(meta3.artist.isEmpty());
}

void tst_AudioTagEngine::testQualityPresets()
{
    AudioExtractOptions opts;
    opts.applyPreset(AudioQualityPreset::HighMp3_320k);
    QCOMPARE(opts.format, AudioFormat::MP3);
    QCOMPARE(opts.bitrateKbps, 320);

    opts.applyPreset(AudioQualityPreset::LosslessFlac);
    QCOMPARE(opts.format, AudioFormat::FLAC);
    QCOMPARE(opts.bitrateKbps, 0);

    opts.applyPreset(AudioQualityPreset::LosslessWav);
    QCOMPARE(opts.format, AudioFormat::WAV);

    opts.applyPreset(AudioQualityPreset::VoiceOpus_64k);
    QCOMPARE(opts.format, AudioFormat::OPUS);
    QCOMPARE(opts.bitrateKbps, 64);
}

void tst_AudioTagEngine::testFormatConversions()
{
    QCOMPARE(AudioTagEngine::formatToString(AudioFormat::MP3), QStringLiteral("mp3"));
    QCOMPARE(AudioTagEngine::formatToString(AudioFormat::FLAC), QStringLiteral("flac"));
    QCOMPARE(AudioTagEngine::formatToString(AudioFormat::AAC), QStringLiteral("aac"));
    QCOMPARE(AudioTagEngine::formatToString(AudioFormat::WAV), QStringLiteral("wav"));
    QCOMPARE(AudioTagEngine::formatToString(AudioFormat::OPUS), QStringLiteral("opus"));

    QCOMPARE(AudioTagEngine::formatFromString(QStringLiteral("MP3")), AudioFormat::MP3);
    QCOMPARE(AudioTagEngine::formatFromString(QStringLiteral("flac")), AudioFormat::FLAC);

    QCOMPARE(AudioTagEngine::codecForFormat(AudioFormat::MP3), QStringLiteral("libmp3lame"));
    QCOMPARE(AudioTagEngine::codecForFormat(AudioFormat::FLAC), QStringLiteral("flac"));
    QCOMPARE(AudioTagEngine::codecForFormat(AudioFormat::OPUS), QStringLiteral("libopus"));
    QCOMPARE(AudioTagEngine::codecForFormat(AudioFormat::WAV), QStringLiteral("pcm_s16le"));
}

void tst_AudioTagEngine::testBuildExtractArgsWithoutCover()
{
    AudioExtractOptions opts;
    opts.format = AudioFormat::MP3;
    opts.bitrateKbps = 320;
    opts.sampleRate = 44100;
    opts.channels = 2;
    opts.metadata.title = QStringLiteral("Live Concert");
    opts.metadata.artist = QStringLiteral("Symphony Orchestra");

    QStringList args = AudioTagEngine::buildExtractCommand(
        QStringLiteral("C:/video/concert.mp4"),
        QStringLiteral("C:/audio/concert.mp3"),
        opts
    );

    QVERIFY(args.contains(QStringLiteral("-i")));
    QVERIFY(args.contains(QStringLiteral("C:/video/concert.mp4")));
    QVERIFY(args.contains(QStringLiteral("libmp3lame")));
    QVERIFY(args.contains(QStringLiteral("320k")));
    QVERIFY(args.contains(QStringLiteral("44100")));
    QVERIFY(args.contains(QStringLiteral("title=Live Concert")));
    QVERIFY(args.contains(QStringLiteral("artist=Symphony Orchestra")));
    QCOMPARE(args.last(), QStringLiteral("C:/audio/concert.mp3"));
}

void tst_AudioTagEngine::testBuildExtractArgsWithCoverMp3()
{
    AudioExtractOptions opts;
    opts.format = AudioFormat::MP3;
    opts.bitrateKbps = 256;
    opts.metadata.title = QStringLiteral("Song with Cover");
    opts.metadata.coverImagePath = QStringLiteral("C:/music/cover.jpg");

    QStringList args = AudioTagEngine::buildExtractCommand(
        QStringLiteral("C:/music/track.wav"),
        QStringLiteral("C:/music/track.mp3"),
        opts
    );

    // Verify both inputs
    int inputIdx1 = args.indexOf(QStringLiteral("-i"));
    QVERIFY(inputIdx1 >= 0);
    int inputIdx2 = args.indexOf(QStringLiteral("-i"), inputIdx1 + 1);
    QVERIFY(inputIdx2 >= 0);
    QCOMPARE(args.at(inputIdx2 + 1), QStringLiteral("C:/music/cover.jpg"));

    // Verify map options
    QVERIFY(args.contains(QStringLiteral("0:a:0?")));
    QVERIFY(args.contains(QStringLiteral("1:0")));
    QVERIFY(args.contains(QStringLiteral("-id3v2_version")));
    QVERIFY(args.contains(QStringLiteral("3")));
    QVERIFY(args.contains(QStringLiteral("title=Album cover")));
}

void tst_AudioTagEngine::testBuildExtractArgsWithCoverFlac()
{
    AudioExtractOptions opts;
    opts.format = AudioFormat::FLAC;
    opts.metadata.coverImagePath = QStringLiteral("C:/music/cover.png");

    QStringList args = AudioTagEngine::buildExtractCommand(
        QStringLiteral("C:/music/track.wav"),
        QStringLiteral("C:/music/track.flac"),
        opts
    );

    QVERIFY(args.contains(QStringLiteral("-disposition:v:0")));
    QVERIFY(args.contains(QStringLiteral("attached_pic")));
}

void tst_AudioTagEngine::testBuildCoverExtractArgs()
{
    QStringList args = AudioTagEngine::buildCoverExtractCommand(
        QStringLiteral("C:/music/album.flac"),
        QStringLiteral("C:/music/extracted_cover.jpg")
    );

    QVERIFY(args.contains(QStringLiteral("-i")));
    QVERIFY(args.contains(QStringLiteral("C:/music/album.flac")));
    QVERIFY(args.contains(QStringLiteral("-an")));
    QVERIFY(args.contains(QStringLiteral("-vcodec")));
    QVERIFY(args.contains(QStringLiteral("copy")));
    QCOMPARE(args.last(), QStringLiteral("C:/music/extracted_cover.jpg"));
}

QTEST_MAIN(tst_AudioTagEngine)
#include "tst_AudioTagEngine.moc"
