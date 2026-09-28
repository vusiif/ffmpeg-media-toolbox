#include "SettingsPage.h"

#include "i18n/TrId.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "ElaComboBox.h"
#include "ElaPushButton.h"
#include "ElaText.h"
#include "ffmpeg/FFmpegCapabilities.h"
#include "ffmpeg/FFmpegLocator.h"
#include "i18n/LanguageManager.h"

SettingsPage::SettingsPage(FFmpegLocator *locator, FFmpegCapabilities *caps,
                           LanguageManager *languageManager, QWidget *parent)
    : QWidget(parent)
    , m_locator(locator)
    , m_caps(caps)
    , m_languageManager(languageManager)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(30, 30, 30, 30);
    root->setSpacing(16);

    m_titleLabel = new ElaText(QString(), 13, this);
    m_titleLabel->setTextPixelSize(28);
    root->addWidget(m_titleLabel);

    auto *form = new QFormLayout();
    form->setSpacing(12);

    m_statusCaption = new ElaText(QString(), 13, this);
    m_statusValue = new ElaText(QString(), 13, this);
    m_ffmpegPathCaption = new ElaText(QString(), 13, this);
    m_ffmpegPathValue = new ElaText(QString(), 13, this);
    m_ffprobePathCaption = new ElaText(QString(), 13, this);
    m_ffprobePathValue = new ElaText(QString(), 13, this);
    m_versionCaption = new ElaText(QString(), 13, this);
    m_versionValue = new ElaText(QString(), 13, this);
    m_capsCaption = new ElaText(QString(), 13, this);
    m_capsValue = new ElaText(QString(), 13, this);
    m_capsValue->setWordWrap(true);

    m_rescanButton = new ElaPushButton(this);
    connect(m_rescanButton, &ElaPushButton::clicked, this, [=]() {
        m_locator->autoDetect();
        m_caps->scan();
        refresh();
    });

    m_languageCaption = new ElaText(QString(), 13, this);
    m_languageCombo = new ElaComboBox(this);
    m_languageCombo->addItems({QStringLiteral("English"), QStringLiteral("中文")});
    m_languageCombo->setCurrentIndex(
        m_languageManager->language().startsWith(QLatin1String("zh")) ? 1 : 0);
    connect(m_languageCombo, &ElaComboBox::currentIndexChanged, this, [=](int index) {
        m_languageManager->setLanguage(index == 0 ? QStringLiteral("en") : QStringLiteral("zh_CN"));
    });

    form->addRow(m_statusCaption, m_statusValue);
    form->addRow(m_ffmpegPathCaption, m_ffmpegPathValue);
    form->addRow(m_ffprobePathCaption, m_ffprobePathValue);
    form->addRow(m_versionCaption, m_versionValue);
    form->addRow(m_capsCaption, m_capsValue);
    form->addRow(m_languageCaption, m_languageCombo);
    root->addLayout(form);

    root->addWidget(m_rescanButton, 0, Qt::AlignRight);
    root->addStretch();

    refresh();
    retranslateUi();
}

void SettingsPage::refresh()
{
    const bool valid = m_locator->isValid();
    m_statusValue->setText(valid ? trId("ffmpeg.found") : trId("ffmpeg.notFound"));
    m_ffmpegPathValue->setText(m_locator->ffmpegPath());
    m_ffprobePathValue->setText(m_locator->ffprobePath());
    m_versionValue->setText(m_locator->ffmpegVersion());
    m_capsValue->setText(trId("settings.caps")
                             .arg(m_caps->formatCount())
                             .arg(m_caps->encoderCount())
                             .arg(m_caps->decoderCount())
                             .arg(m_caps->filterCount())
                             .arg(m_caps->hwAccelCount()));
}

void SettingsPage::retranslateUi()
{
    m_titleLabel->setText(trId("settings.title"));
    m_statusCaption->setText(trId("ffmpeg.status") + QStringLiteral(":"));
    m_ffmpegPathCaption->setText(trId("ffmpeg.ffmpegPath") + QStringLiteral(":"));
    m_ffprobePathCaption->setText(trId("ffmpeg.ffprobePath") + QStringLiteral(":"));
    m_versionCaption->setText(trId("ffmpeg.version") + QStringLiteral(":"));
    m_capsCaption->setText(trId("media.title") + QStringLiteral(":"));
    m_languageCaption->setText(trId("settings.language") + QStringLiteral(":"));
    m_rescanButton->setText(trId("ffmpeg.rescan"));
    refresh();
}
