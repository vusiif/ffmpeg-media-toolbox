#pragma once

#include <QObject>
#include <QCoreApplication>
#include <QTranslator>
#include <QString>

class LanguageManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)

public:
    explicit LanguageManager(QObject *parent = nullptr);

    QString language() const;
    void setLanguage(const QString &lang);

    Q_INVOKABLE void loadTranslation(const QString &lang);

signals:
    void languageChanged();

private:
    QTranslator m_translator;
    QString m_language;
};
