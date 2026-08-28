#pragma once

#include <QObject>
#include <QCoreApplication>
#include <QTranslator>
#include <QString>

class QQmlEngine;

class LanguageManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)

public:
    explicit LanguageManager(QObject *parent = nullptr);

    QString language() const;
    void setLanguage(const QString &lang);

    void setQmlEngine(QQmlEngine *engine);

signals:
    void languageChanged();

private:
    void loadTranslation(const QString &lang);

    QTranslator m_translator;
    QString m_language;
    QQmlEngine *m_engine = nullptr;
};
