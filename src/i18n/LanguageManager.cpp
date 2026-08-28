#include "LanguageManager.h"
#include <QDir>
#include <QDebug>

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

void LanguageManager::loadTranslation(const QString &lang)
{
    QCoreApplication::removeTranslator(&m_translator);

    if (lang.isEmpty() || lang == "en") {
        return;
    }

    // qt_add_translations embeds .qm as resources at :/i18n/
    // Filename pattern: ffmedia_<locale>.qm  e.g. ffmedia_zh_CN.qm
    const QString qmPath = QStringLiteral(":/i18n/ffmedia_%1.qm").arg(lang);

    if (m_translator.load(qmPath)) {
        QCoreApplication::installTranslator(&m_translator);
        qDebug() << "Loaded translation:" << qmPath;
    } else {
        qWarning() << "Failed to load translation:" << qmPath;
    }
}
