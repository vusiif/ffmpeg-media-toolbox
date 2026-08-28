#include "LanguageManager.h"
#include <QQmlEngine>
#include <QDebug>
#include <QFile>

LanguageManager::LanguageManager(QObject *parent)
    : QObject(parent)
{
}

QString LanguageManager::language() const
{
    return m_language;
}

void LanguageManager::setLanguage(const QString &lang)
{
    if (m_language == lang) return;
    m_language = lang;
    loadTranslation(lang);
    emit languageChanged();
}

void LanguageManager::setQmlEngine(QQmlEngine *engine)
{
    m_engine = engine;
}

void LanguageManager::loadTranslation(const QString &lang)
{
    QCoreApplication::removeTranslator(&m_translator);

    if (lang.isEmpty() || lang == "en") {
        if (m_engine) m_engine->retranslate();
        return;
    }

    const QString qmPath = QCoreApplication::applicationDirPath()
                           + QStringLiteral("/ffmedia_%1.qm").arg(lang);

    if (!QFile::exists(qmPath)) {
        qWarning() << "Translation file not found:" << qmPath;
        if (m_engine) m_engine->retranslate();
        return;
    }

    if (m_translator.load(qmPath)) {
        QCoreApplication::installTranslator(&m_translator);
        qDebug() << "Loaded translation:" << qmPath;
    } else {
        qWarning() << "Failed to load translation:" << qmPath;
    }

    if (m_engine) m_engine->retranslate();
}
