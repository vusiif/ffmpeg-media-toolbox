#include "TranslationManager.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCoreApplication>

static constexpr auto kSettingsGroup = "Language";
static constexpr auto kKeyLang = "current";

TranslationManager::TranslationManager(QObject *parent)
    : QObject(parent)
{
    m_allTranslations[QStringLiteral("en")] = {};
    m_allTranslations[QStringLiteral("zh")] = {};

    auto loadFromResource = [&](const QString &lang) {
        QFile file(QStringLiteral(":/translations/%1.json").arg(lang));
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            QJsonObject obj = doc.object();
            QMap<QString, QString> map;
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                map[it.key()] = it.value().toString();
            }
            m_allTranslations[lang] = map;
        }
    };

    loadFromResource(QStringLiteral("en"));
    loadFromResource(QStringLiteral("zh"));

    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    QString savedLang = settings.value(kKeyLang).toString();
    settings.endGroup();

    if (savedLang.isEmpty()) {
        QString sysLang = QLocale::system().name().left(2).toLower();
        if (m_allTranslations.contains(sysLang)) {
            savedLang = sysLang;
        } else {
            savedLang = QStringLiteral("en");
        }
    }

    setLanguage(savedLang);
}

QString TranslationManager::currentLanguage() const { return m_currentLang; }

void TranslationManager::setLanguage(const QString &lang)
{
    if (!m_allTranslations.contains(lang)) {
        return;
    }

    if (m_currentLang == lang) {
        return;
    }

    m_currentLang = lang;
    m_translations = m_allTranslations.value(lang);

    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    settings.setValue(kKeyLang, lang);
    settings.endGroup();

    emit languageChanged();
}

QString TranslationManager::tr(const QString &key) const
{
    if (m_translations.contains(key)) {
        return m_translations.value(key);
    }
    return key;
}

QStringList TranslationManager::availableLanguages()
{
    return {QStringLiteral("en"), QStringLiteral("zh")};
}
