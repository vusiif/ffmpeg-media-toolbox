#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

class PresetManager : public QObject
{
    Q_OBJECT

public:
    explicit PresetManager(QObject *parent = nullptr);

    QStringList names() const;
    void save(const QString &name, const QVariantMap &values);
    QVariantMap load(const QString &name) const;
    void remove(const QString &name);

signals:
    void presetsChanged();
};
