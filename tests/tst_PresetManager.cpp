#include <QtTest>

#include "presets/PresetManager.h"

class PresetManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void builtInPresetsAvailable();
    void isBuiltInProtectsFromRemoval();
    void loadBuiltInPresetValues();
    void saveAndRemoveCustomPreset();
};

void PresetManagerTest::builtInPresetsAvailable()
{
    PresetManager manager;
    const QStringList names = manager.names();
    QVERIFY(names.size() >= 7);

    // Verify key scenario presets are included
    bool hasWeChat = false;
    bool hasDiscord = false;
    bool hasBilibili = false;
    bool hasMobile = false;
    bool hasWeb = false;
    bool hasAudio = false;

    for (const QString &name : names) {
        if (name.contains(QLatin1String("25MB"), Qt::CaseInsensitive) || name.contains(QLatin1String("WeChat"), Qt::CaseInsensitive)) {
            hasWeChat = true;
        }
        if (name.contains(QLatin1String("10MB"), Qt::CaseInsensitive) || name.contains(QLatin1String("Discord"), Qt::CaseInsensitive)) {
            hasDiscord = true;
        }
        if (name.contains(QLatin1String("Bilibili"), Qt::CaseInsensitive) || name.contains(QLatin1String("B站"), Qt::CaseInsensitive)) {
            hasBilibili = true;
        }
        if (name.contains(QLatin1String("Mobile"), Qt::CaseInsensitive) || name.contains(QLatin1String("移动端"), Qt::CaseInsensitive)) {
            hasMobile = true;
        }
        if (name.contains(QLatin1String("Web"), Qt::CaseInsensitive)) {
            hasWeb = true;
        }
        if (name.contains(QLatin1String("MP3"), Qt::CaseInsensitive)) {
            hasAudio = true;
        }
    }

    QVERIFY(hasWeChat);
    QVERIFY(hasDiscord);
    QVERIFY(hasBilibili);
    QVERIFY(hasMobile);
    QVERIFY(hasWeb);
    QVERIFY(hasAudio);
}

void PresetManagerTest::isBuiltInProtectsFromRemoval()
{
    PresetManager manager;
    const QStringList names = manager.names();
    QVERIFY(!names.isEmpty());

    const QString firstBuiltIn = names.first();
    QVERIFY(manager.isBuiltIn(firstBuiltIn));

    // Attempting to remove a built-in preset must be a safe no-op
    manager.remove(firstBuiltIn);
    QVERIFY(manager.names().contains(firstBuiltIn));
}

void PresetManagerTest::loadBuiltInPresetValues()
{
    PresetManager manager;
    const QVariantMap wechat = manager.load(QStringLiteral("preset.scenario.wechat"));
    QCOMPARE(wechat.value(QStringLiteral("format")).toString(), QStringLiteral("mp4"));
    QCOMPARE(wechat.value(QStringLiteral("videoCodec")).toString(), QStringLiteral("H.264"));
    QCOMPARE(wechat.value(QStringLiteral("rateControl")).toInt(), 1);
    QCOMPARE(wechat.value(QStringLiteral("targetSizeMB")).toDouble(), 25.0);
    QCOMPARE(wechat.value(QStringLiteral("twoPass")).toBool(), true);

    const QVariantMap bilibili = manager.load(QStringLiteral("preset.scenario.bilibili"));
    QCOMPARE(bilibili.value(QStringLiteral("videoCodec")).toString(), QStringLiteral("H.265 / HEVC"));
    QCOMPARE(bilibili.value(QStringLiteral("crf")).toInt(), 18);
    QCOMPARE(bilibili.value(QStringLiteral("audioBitrate")).toString(), QStringLiteral("320k"));
}

void PresetManagerTest::saveAndRemoveCustomPreset()
{
    PresetManager manager;
    const QString customName = QStringLiteral("My Test Custom Preset");

    QVariantMap values;
    values.insert(QStringLiteral("format"), QStringLiteral("mkv"));
    values.insert(QStringLiteral("videoCodec"), QStringLiteral("VP9"));
    values.insert(QStringLiteral("crf"), 20);

    manager.save(customName, values);
    QVERIFY(manager.names().contains(customName));
    QVERIFY(!manager.isBuiltIn(customName));

    const QVariantMap loaded = manager.load(customName);
    QCOMPARE(loaded.value(QStringLiteral("format")).toString(), QStringLiteral("mkv"));
    QCOMPARE(loaded.value(QStringLiteral("videoCodec")).toString(), QStringLiteral("VP9"));
    QCOMPARE(loaded.value(QStringLiteral("crf")).toInt(), 20);

    manager.remove(customName);
    QVERIFY(!manager.names().contains(customName));
}

QTEST_GUILESS_MAIN(PresetManagerTest)
#include "tst_PresetManager.moc"
