#include <QtTest>

#include <QFile>
#include <QTemporaryFile>

#include "subtitles/SubtitleEngine.h"
#include "subtitles/WhisperPipeline.h"

class SubtitleEngineTest : public QObject
{
    Q_OBJECT

private slots:
    void timestampParsingAndFormatting();
    void srtParsingAndSerializationRoundTrip();
    void vttParsingAndSerialization();
    void assSerialization();
    void timingTransformations();
    void timeLookup();
    void whisperCommandGeneration();
};

void SubtitleEngineTest::timestampParsingAndFormatting()
{
    // SRT format
    const qint64 t1 = SubtitleItem::parseTime(QStringLiteral("00:01:23,456"));
    QCOMPARE(t1, 83456);
    QCOMPARE(SubtitleItem::formatTime(t1, false), QStringLiteral("00:01:23,456"));

    // VTT format
    const qint64 t2 = SubtitleItem::parseTime(QStringLiteral("00:01:23.456"));
    QCOMPARE(t2, 83456);
    QCOMPARE(SubtitleItem::formatTime(t2, true), QStringLiteral("00:01:23.456"));

    // Short format (MM:SS.mmm)
    const qint64 t3 = SubtitleItem::parseTime(QStringLiteral("02:15.500"));
    QCOMPARE(t3, 135500);

    // Hours
    const qint64 t4 = SubtitleItem::parseTime(QStringLiteral("02:00:00,000"));
    QCOMPARE(t4, 7200000);
    QCOMPARE(SubtitleItem::formatTime(t4, false), QStringLiteral("02:00:00,000"));
}

void SubtitleEngineTest::srtParsingAndSerializationRoundTrip()
{
    const QString sampleSrt = QStringLiteral(
        "1\n"
        "00:00:01,000 --> 00:00:04,000\n"
        "Hello World!\n\n"
        "2\n"
        "00:00:05,500 --> 00:00:08,200\n"
        "This is line 1.\n"
        "This is line 2.\n\n"
        "3\n"
        "00:00:10,000 --> 00:00:12,000\n"
        "Final farewell.\n");

    const QList<SubtitleItem> items = SubtitleEngine::parseSrt(sampleSrt);
    QCOMPARE(items.size(), 3);

    QCOMPARE(items[0].id, 1);
    QCOMPARE(items[0].startTimeMs, 1000);
    QCOMPARE(items[0].endTimeMs, 4000);
    QCOMPARE(items[0].text, QStringLiteral("Hello World!"));

    QCOMPARE(items[1].startTimeMs, 5500);
    QCOMPARE(items[1].endTimeMs, 8200);
    QCOMPARE(items[1].text, QStringLiteral("This is line 1.\nThis is line 2."));

    // Round trip
    const QString serialized = SubtitleEngine::serializeSrt(items);
    const QList<SubtitleItem> roundTripItems = SubtitleEngine::parseSrt(serialized);
    QCOMPARE(roundTripItems.size(), 3);
    QCOMPARE(roundTripItems[0].text, items[0].text);
    QCOMPARE(roundTripItems[1].text, items[1].text);
    QCOMPARE(roundTripItems[1].durationMs(), 2700);
}

void SubtitleEngineTest::vttParsingAndSerialization()
{
    const QString sampleVtt = QStringLiteral(
        "WEBVTT - Sample subtitle\n\n"
        "00:00:02.000 --> 00:00:05.500 position:10%\n"
        "<v Speaker1><b>Hello</b> WebVTT world!</v>\n\n"
        "00:00:06.000 --> 00:00:09.000\n"
        "Second cue subtitle.\n");

    const QList<SubtitleItem> items = SubtitleEngine::parseVtt(sampleVtt);
    QCOMPARE(items.size(), 2);

    QCOMPARE(items[0].startTimeMs, 2000);
    QCOMPARE(items[0].endTimeMs, 5500);
    // HTML tags stripped
    QCOMPARE(items[0].text, QStringLiteral("Hello WebVTT world!"));

    const QString outVtt = SubtitleEngine::serializeVtt(items);
    QVERIFY(outVtt.startsWith(QStringLiteral("WEBVTT")));
    QVERIFY(outVtt.contains(QStringLiteral("00:00:02.000 --> 00:00:05.500")));
}

void SubtitleEngineTest::assSerialization()
{
    QList<SubtitleItem> items;
    SubtitleItem sub1;
    sub1.startTimeMs = 1500;
    sub1.endTimeMs = 4500;
    sub1.text = QStringLiteral("Line A\nLine B");
    items.append(sub1);

    const QString ass = SubtitleEngine::serializeAss(items, QStringLiteral("My Movie"));
    QVERIFY(ass.contains(QStringLiteral("[Script Info]")));
    QVERIFY(ass.contains(QStringLiteral("[V4+ Styles]")));
    QVERIFY(ass.contains(QStringLiteral("[Events]")));
    QVERIFY(ass.contains(QStringLiteral("Dialogue: 0,0:00:01.50,0:00:04.50,Default,,0,0,0,,Line A\\NLine B")));
}

void SubtitleEngineTest::timingTransformations()
{
    QList<SubtitleItem> items;
    SubtitleItem s1;
    s1.id = 1;
    s1.startTimeMs = 2000;
    s1.endTimeMs = 5000;
    s1.text = QStringLiteral("First Part");

    SubtitleItem s2;
    s2.id = 2;
    s2.startTimeMs = 6000;
    s2.endTimeMs = 8000;
    s2.text = QStringLiteral("Second Part");

    items.append(s1);
    items.append(s2);

    // 1. Shift +1500ms
    SubtitleEngine::shiftTime(items, 1500);
    QCOMPARE(items[0].startTimeMs, 3500);
    QCOMPARE(items[0].endTimeMs, 6500);
    QCOMPARE(items[1].startTimeMs, 7500);
    QCOMPARE(items[1].endTimeMs, 9500);

    // 2. Shift negative with clamp
    SubtitleEngine::shiftTime(items, -10000);
    QCOMPARE(items[0].startTimeMs, 0);
    QVERIFY(items[0].endTimeMs >= 0);

    // Reset items
    items.clear();
    items.append(s1);
    items.append(s2);

    // 3. Merge items 0 and 1
    QVERIFY(SubtitleEngine::mergeItems(items, 0, 1));
    QCOMPARE(items.size(), 1);
    QCOMPARE(items[0].startTimeMs, 2000);
    QCOMPARE(items[0].endTimeMs, 8000);
    QCOMPARE(items[0].text, QStringLiteral("First Part Second Part"));

    // 4. Split item at 5000ms
    QVERIFY(SubtitleEngine::splitItem(items, 0, 5000));
    QCOMPARE(items.size(), 2);
    QCOMPARE(items[0].startTimeMs, 2000);
    QCOMPARE(items[0].endTimeMs, 5000);
    QCOMPARE(items[1].startTimeMs, 5000);
    QCOMPARE(items[1].endTimeMs, 8000);
}

void SubtitleEngineTest::timeLookup()
{
    QList<SubtitleItem> items;
    SubtitleItem s1;
    s1.startTimeMs = 1000;
    s1.endTimeMs = 3000;
    s1.text = QStringLiteral("Cue 1");

    SubtitleItem s2;
    s2.startTimeMs = 5000;
    s2.endTimeMs = 7000;
    s2.text = QStringLiteral("Cue 2");

    items.append(s1);
    items.append(s2);

    QCOMPARE(SubtitleEngine::findItemAtTime(items, 500), -1);
    QCOMPARE(SubtitleEngine::findItemAtTime(items, 1500), 0);
    QCOMPARE(SubtitleEngine::findItemAtTime(items, 3000), 0);
    QCOMPARE(SubtitleEngine::findItemAtTime(items, 4000), -1);
    QCOMPARE(SubtitleEngine::findItemAtTime(items, 6200), 1);
}

void SubtitleEngineTest::whisperCommandGeneration()
{
    const QStringList extractArgs = WhisperPipeline::buildAudioExtractionCommand(
        QStringLiteral("movie.mp4"), QStringLiteral("speech.wav"));

    QVERIFY(extractArgs.contains(QStringLiteral("-ar")));
    QVERIFY(extractArgs.contains(QStringLiteral("16000")));
    QVERIFY(extractArgs.contains(QStringLiteral("-ac")));
    QVERIFY(extractArgs.contains(QStringLiteral("1")));
    QVERIFY(extractArgs.contains(QStringLiteral("pcm_s16le")));

    WhisperOptions opts;
    opts.language = QStringLiteral("zh");
    opts.modelPath = QStringLiteral("models/ggml-base.bin");
    opts.threadCount = 6;
    opts.translateToEnglish = true;

    const QStringList whisperArgs = WhisperPipeline::buildWhisperCommand(
        opts, QStringLiteral("speech.wav"), QStringLiteral("output_base"));

    QVERIFY(whisperArgs.contains(QStringLiteral("-m")));
    QVERIFY(whisperArgs.contains(QStringLiteral("models/ggml-base.bin")));
    QVERIFY(whisperArgs.contains(QStringLiteral("-l")));
    QVERIFY(whisperArgs.contains(QStringLiteral("zh")));
    QVERIFY(whisperArgs.contains(QStringLiteral("-tr")));
    QVERIFY(whisperArgs.contains(QStringLiteral("-t")));
    QVERIFY(whisperArgs.contains(QStringLiteral("6")));
    QVERIFY(whisperArgs.contains(QStringLiteral("-osrt")));
}

QTEST_MAIN(SubtitleEngineTest)
#include "tst_SubtitleEngine.moc"
