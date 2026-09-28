#include "PresetManager.h"

#include <QSettings>

namespace {
QSettings &settings()
{
    static QSettings s(QStringLiteral("FFMedia"), QStringLiteral("FFmpeg Media Toolbox"));
    return s;
}
} // namespace

PresetManager::PresetManager(QObject *parent)
    : QObject(parent)
{
}

QStringList PresetManager::names() const
{
    settings().beginGroup(QStringLiteral("presets"));
    const QStringList list = settings().childGroups();
    settings().endGroup();
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
    settings().beginGroup(QStringLiteral("presets"));
    settings().remove(name);
    settings().endGroup();
    emit presetsChanged();
}
