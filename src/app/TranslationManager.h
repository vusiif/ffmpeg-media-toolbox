#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QSettings>

class TranslationManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentLanguage READ currentLanguage NOTIFY languageChanged)
    Q_PROPERTY(int languageVersion READ languageVersion NOTIFY languageChanged)

public:
    explicit TranslationManager(QObject *parent = nullptr);

    QString currentLanguage() const;
    int languageVersion() const;

    Q_INVOKABLE void setLanguage(const QString &lang);
    Q_INVOKABLE QString tr(const QString &key, int version = 0) const;

    static QStringList availableLanguages();

signals:
    void languageChanged();

private:
    void loadTranslation(const QString &lang);

    QString m_currentLang;
    int m_languageVersion = 0;
    QMap<QString, QString> m_translations;
    QMap<QString, QMap<QString, QString>> m_allTranslations;
};
