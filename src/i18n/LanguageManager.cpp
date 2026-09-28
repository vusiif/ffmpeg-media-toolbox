#include "LanguageManager.h"
#include <QDebug>
#include <QFile>

LanguageManager::LanguageManager(QObject *parent)
    : QObject(parent)
{
    loadTranslation(m_language);
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

void LanguageManager::loadTranslation(const QString &lang)
{
    QCoreApplication::removeTranslator(&m_translator);

    if (lang.isEmpty()) {
        return;
    }

    const QString name = QStringLiteral("ffmedia_%1.qm").arg(lang);
    const QString resourcePath = QStringLiteral(":/i18n/") + name;
    const QString filePath = QCoreApplication::applicationDirPath() + QStringLiteral("/") + name;

    if (QFile::exists(resourcePath) && m_translator.load(resourcePath)) {
        QCoreApplication::installTranslator(&m_translator);
        qDebug() << "Loaded translation:" << resourcePath;
    } else if (QFile::exists(filePath) && m_translator.load(filePath)) {
        QCoreApplication::installTranslator(&m_translator);
        qDebug() << "Loaded translation:" << filePath;
    } else {
        qWarning() << "Translation not found for" << lang << "- tried" << resourcePath << filePath;
    }
}
