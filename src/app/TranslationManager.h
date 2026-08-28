#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QSettings>

class TranslationManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentLanguage READ currentLanguage NOTIFY languageChanged)

public:
    explicit TranslationManager(QObject *parent = nullptr);

    QString currentLanguage() const;

    Q_INVOKABLE void setLanguage(const QString &lang);
    Q_INVOKABLE QString tr(const QString &key) const;

    static QStringList availableLanguages();

signals:
    void languageChanged();

private:
    void loadTranslation(const QString &lang);

    QString m_currentLang;
    QMap<QString, QString> m_translations;
    QMap<QString, QMap<QString, QString>> m_allTranslations;
};
