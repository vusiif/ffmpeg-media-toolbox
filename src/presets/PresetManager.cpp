#include "PresetManager.h"

#include "app/AppSettings.h"
#include "i18n/TrId.h"

#include <QSettings>

namespace {
QSettings &settings()
{
    static QSettings s(AppSettings::organization(), AppSettings::application());
    return s;
}

QString localizedOrFallback(const QString &key, const QString &fallback)
{
    const QString text = trId(key.toUtf8().constData());
    return (text == key || text.isEmpty()) ? fallback : text;
}
} // namespace

PresetManager::PresetManager(QObject *parent)
    : QObject(parent)
{
}

QList<PresetManager::PresetInfo> PresetManager::defaultBuiltInPresets()
{
    QList<PresetInfo> list;

    // 1. WeChat Share (<=25MB)
    {
        PresetInfo p;
        p.id = QStringLiteral("preset.scenario.wechat");
        p.name = localizedOrFallback(p.id, QStringLiteral("WeChat Share (<=25MB)"));
        p.isBuiltIn = true;
        p.values.insert(QStringLiteral("format"), QStringLiteral("mp4"));
        p.values.insert(QStringLiteral("videoCodec"), QStringLiteral("H.264"));
        p.values.insert(QStringLiteral("audioCodec"), QStringLiteral("AAC"));
        p.values.insert(QStringLiteral("rateControl"), 1); // target size
        p.values.insert(QStringLiteral("targetSizeMB"), 25.0);
        p.values.insert(QStringLiteral("twoPass"), true);
        p.values.insert(QStringLiteral("speedPreset"), QStringLiteral("medium"));
        p.values.insert(QStringLiteral("pixFmt"), QStringLiteral("yuv420p"));
        p.values.insert(QStringLiteral("preserveAllStreams"), true);
        p.values.insert(QStringLiteral("audioBitrate"), QStringLiteral("128k"));
        list.append(p);
    }

    // 2. Discord Clip (<=10MB)
    {
        PresetInfo p;
        p.id = QStringLiteral("preset.scenario.discord");
        p.name = localizedOrFallback(p.id, QStringLiteral("Discord Clip (<=10MB)"));
        p.isBuiltIn = true;
        p.values.insert(QStringLiteral("format"), QStringLiteral("mp4"));
        p.values.insert(QStringLiteral("videoCodec"), QStringLiteral("H.264"));
        p.values.insert(QStringLiteral("audioCodec"), QStringLiteral("AAC"));
        p.values.insert(QStringLiteral("rateControl"), 1); // target size
        p.values.insert(QStringLiteral("targetSizeMB"), 10.0);
        p.values.insert(QStringLiteral("twoPass"), true);
        p.values.insert(QStringLiteral("speedPreset"), QStringLiteral("fast"));
        p.values.insert(QStringLiteral("pixFmt"), QStringLiteral("yuv420p"));
        p.values.insert(QStringLiteral("preserveAllStreams"), true);
        p.values.insert(QStringLiteral("audioBitrate"), QStringLiteral("96k"));
        list.append(p);
    }

    // 3. Bilibili HQ (HEVC)
    {
        PresetInfo p;
        p.id = QStringLiteral("preset.scenario.bilibili");
        p.name = localizedOrFallback(p.id, QStringLiteral("Bilibili HQ (HEVC)"));
        p.isBuiltIn = true;
        p.values.insert(QStringLiteral("format"), QStringLiteral("mp4"));
        p.values.insert(QStringLiteral("videoCodec"), QStringLiteral("H.265 / HEVC"));
        p.values.insert(QStringLiteral("audioCodec"), QStringLiteral("AAC"));
        p.values.insert(QStringLiteral("rateControl"), 0); // CRF
        p.values.insert(QStringLiteral("crf"), 18);
        p.values.insert(QStringLiteral("speedPreset"), QStringLiteral("slow"));
        p.values.insert(QStringLiteral("pixFmt"), QStringLiteral("yuv420p"));
        p.values.insert(QStringLiteral("preserveAllStreams"), true);
        p.values.insert(QStringLiteral("audioBitrate"), QStringLiteral("320k"));
        list.append(p);
    }

    // 4. Mobile Space Saver (HEVC)
    {
        PresetInfo p;
        p.id = QStringLiteral("preset.scenario.mobile");
        p.name = localizedOrFallback(p.id, QStringLiteral("Mobile Space Saver (HEVC)"));
        p.isBuiltIn = true;
        p.values.insert(QStringLiteral("format"), QStringLiteral("mp4"));
        p.values.insert(QStringLiteral("videoCodec"), QStringLiteral("H.265 / HEVC"));
        p.values.insert(QStringLiteral("audioCodec"), QStringLiteral("AAC"));
        p.values.insert(QStringLiteral("rateControl"), 0); // CRF
        p.values.insert(QStringLiteral("crf"), 26);
        p.values.insert(QStringLiteral("speedPreset"), QStringLiteral("medium"));
        p.values.insert(QStringLiteral("pixFmt"), QStringLiteral("yuv420p"));
        p.values.insert(QStringLiteral("preserveAllStreams"), true);
        p.values.insert(QStringLiteral("audioBitrate"), QStringLiteral("128k"));
        list.append(p);
    }

    // 5. Web Optimized (H.264)
    {
        PresetInfo p;
        p.id = QStringLiteral("preset.scenario.web");
        p.name = localizedOrFallback(p.id, QStringLiteral("Web Optimized (H.264)"));
        p.isBuiltIn = true;
        p.values.insert(QStringLiteral("format"), QStringLiteral("mp4"));
        p.values.insert(QStringLiteral("videoCodec"), QStringLiteral("H.264"));
        p.values.insert(QStringLiteral("audioCodec"), QStringLiteral("AAC"));
        p.values.insert(QStringLiteral("rateControl"), 0); // CRF
        p.values.insert(QStringLiteral("crf"), 22);
        p.values.insert(QStringLiteral("speedPreset"), QStringLiteral("medium"));
        p.values.insert(QStringLiteral("pixFmt"), QStringLiteral("yuv420p"));
        p.values.insert(QStringLiteral("preserveAllStreams"), true);
        p.values.insert(QStringLiteral("audioBitrate"), QStringLiteral("192k"));
        list.append(p);
    }

    // 6. Audio Extract (MP3 320k)
    {
        PresetInfo p;
        p.id = QStringLiteral("preset.scenario.audioMp3");
        p.name = localizedOrFallback(p.id, QStringLiteral("Audio Extract (MP3 320k)"));
        p.isBuiltIn = true;
        p.values.insert(QStringLiteral("format"), QStringLiteral("mp3"));
        p.values.insert(QStringLiteral("audioCodec"), QStringLiteral("MP3"));
        p.values.insert(QStringLiteral("audioBitrate"), QStringLiteral("320k"));
        list.append(p);
    }

    // 7. Audio Lossless (FLAC)
    {
        PresetInfo p;
        p.id = QStringLiteral("preset.scenario.audioFlac");
        p.name = localizedOrFallback(p.id, QStringLiteral("Audio Lossless (FLAC)"));
        p.isBuiltIn = true;
        p.values.insert(QStringLiteral("format"), QStringLiteral("flac"));
        p.values.insert(QStringLiteral("audioCodec"), QStringLiteral("FLAC"));
        list.append(p);
    }

    return list;
}

QList<PresetManager::PresetInfo> PresetManager::allPresets() const
{
    QList<PresetInfo> list = defaultBuiltInPresets();

    settings().beginGroup(QStringLiteral("presets"));
    const QStringList userGroups = settings().childGroups();
    for (const QString &ug : userGroups) {
        settings().beginGroup(ug);
        PresetInfo p;
        p.id = ug;
        p.name = ug;
        p.isBuiltIn = false;
        const QStringList keys = settings().childKeys();
        for (const QString &k : keys) {
            p.values.insert(k, settings().value(k));
        }
        settings().endGroup();
        list.append(p);
    }
    settings().endGroup();

    return list;
}

bool PresetManager::isBuiltIn(const QString &name) const
{
    if (name.trimmed().isEmpty()) return false;
    const auto defaults = defaultBuiltInPresets();
    for (const auto &p : defaults) {
        if (p.name == name || p.id == name) {
            return true;
        }
    }
    return false;
}

QStringList PresetManager::names() const
{
    QStringList list;
    const auto defaults = defaultBuiltInPresets();
    for (const auto &p : defaults) {
        list.append(p.name);
    }

    settings().beginGroup(QStringLiteral("presets"));
    const QStringList userGroups = settings().childGroups();
    settings().endGroup();

    for (const QString &ug : userGroups) {
        if (!list.contains(ug)) {
            list.append(ug);
        }
    }
    return list;
}

void PresetManager::save(const QString &name, const QVariantMap &values)
{
    if (name.trimmed().isEmpty()) return;

    settings().beginGroup(QStringLiteral("presets"));
    settings().beginGroup(name.trimmed());
    settings().remove(QString());
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        settings().setValue(it.key(), it.value());
    }
    settings().endGroup();
    settings().endGroup();
    emit presetsChanged();
}

QVariantMap PresetManager::load(const QString &name) const
{
    if (name.trimmed().isEmpty()) return {};

    const auto defaults = defaultBuiltInPresets();
    for (const auto &p : defaults) {
        if (p.name == name || p.id == name) {
            return p.values;
        }
    }

    QVariantMap map;
    settings().beginGroup(QStringLiteral("presets"));
    settings().beginGroup(name);
    const QStringList keys = settings().childKeys();
    for (const QString &key : keys) {
        map.insert(key, settings().value(key));
    }
    settings().endGroup();
    settings().endGroup();
    return map;
}

void PresetManager::remove(const QString &name)
{
    if (isBuiltIn(name)) {
        return; // Built-in presets are protected from deletion
    }

    settings().beginGroup(QStringLiteral("presets"));
    settings().remove(name);
    settings().endGroup();
    emit presetsChanged();
}
