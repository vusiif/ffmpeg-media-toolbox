#include "LanguageManager.h"
#include <QDir>
#include <QCoreApplication>
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

    // Try filesystem first (build directory), then resource
    const QString fsPath = QCoreApplication::applicationDirPath()
                           + QStringLiteral("/ffmedia_%1.qm").arg(lang);
    const QString qmPath = QStringLiteral(":/i18n/ffmedia_%1.qm").arg(lang);

    bool loaded = false;
    if (QFile::exists(fsPath)) {
        loaded = m_translator.load(fsPath);
        if (loaded) qDebug() << "Loaded translation from filesystem:" << fsPath;
    }
    if (!loaded) {
        loaded = m_translator.load(qmPath);
        if (loaded) qDebug() << "Loaded translation from resource:" << qmPath;
    }

    if (loaded) {
        QCoreApplication::installTranslator(&m_translator);
        qDebug() << "Translator installed. Test translation:"
                 << m_translator.translate("Main", "app.title");
    } else {
        qWarning() << "Failed to load translation for:" << lang;
    }
}
