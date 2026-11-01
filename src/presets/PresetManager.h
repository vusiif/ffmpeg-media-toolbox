#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

class PresetManager : public QObject
{
    Q_OBJECT

public:
    struct PresetInfo {
        QString id;
        QString name;
        QVariantMap values;
        bool isBuiltIn = false;
    };

    explicit PresetManager(QObject *parent = nullptr);

    QStringList names() const;
    QList<PresetInfo> allPresets() const;

    bool isBuiltIn(const QString &name) const;
    void save(const QString &name, const QVariantMap &values);
    QVariantMap load(const QString &name) const;
    void remove(const QString &name);

signals:
    void presetsChanged();

private:
    static QList<PresetInfo> defaultBuiltInPresets();
};
