#include "SettingsPage.h"

#include "i18n/TrId.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "ElaCheckBox.h"
#include "ElaComboBox.h"
#include "ElaPushButton.h"
#include "ElaScrollArea.h"
#include "ElaScrollPageArea.h"
#include "ElaText.h"
#include "app/AppSettings.h"
#include "app/RecentFiles.h"
#include "ffmpeg/FFmpegCapabilities.h"
#include "ffmpeg/FFmpegLocator.h"
#include "i18n/LanguageManager.h"
#include "jobs/JobQueue.h"

#include <QSettings>

SettingsPage::SettingsPage(FFmpegLocator *locator, FFmpegCapabilities *caps,
                           RecentFiles *recentFiles, JobQueue *jobQueue,
                           LanguageManager *languageManager,
                           QWidget *parent)
    : QWidget(parent)
    , m_locator(locator)
    , m_caps(caps)
    , m_recentFiles(recentFiles)
    , m_jobQueue(jobQueue)
    , m_languageManager(languageManager)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(30, 24, 30, 24);
    root->setSpacing(16);

    m_titleLabel = new ElaText(QString(), 13, this);
    m_titleLabel->setTextPixelSize(28);
    root->addWidget(m_titleLabel);

    auto *scrollArea = new ElaScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setIsGrabGesture(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet(QStringLiteral("QScrollArea { background: transparent; border: none; }"));

    auto *container = new QWidget(scrollArea);
    container->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 16, 0);
    containerLayout->setSpacing(16);

    // Card 1: FFmpeg Engine Environment
    m_engineCard = new ElaScrollPageArea(container);
    m_engineCard->setMinimumHeight(0);
    m_engineCard->setMaximumHeight(QWIDGETSIZE_MAX);
    auto *engineLayout = new QVBoxLayout(m_engineCard);
    engineLayout->setContentsMargins(18, 16, 18, 16);
    engineLayout->setSpacing(14);

    auto *engineHeaderLayout = new QHBoxLayout();
    m_engineCardTitle = new ElaText(QString(), 15, m_engineCard);
    m_engineCardTitle->setStyleSheet(QStringLiteral("font-weight: bold;"));
    m_rescanButton = new ElaPushButton(m_engineCard);
    m_rescanButton->setFixedHeight(32);
    connect(m_rescanButton, &ElaPushButton::clicked, this, [=]() {
        m_locator->autoDetect();
        m_caps->scan();
        refresh();
    });
    engineHeaderLayout->addWidget(m_engineCardTitle);
    engineHeaderLayout->addStretch();
    engineHeaderLayout->addWidget(m_rescanButton);
    engineLayout->addLayout(engineHeaderLayout);

    auto *engineForm = new QFormLayout();
    engineForm->setSpacing(12);

    m_statusCaption = new ElaText(QString(), 13, m_engineCard);
    m_statusValue = new ElaText(QString(), 13, m_engineCard);
    m_statusValue->setTextInteractionFlags(Qt::TextSelectableByMouse);

    m_ffmpegPathCaption = new ElaText(QString(), 13, m_engineCard);
    m_ffmpegPathValue = new ElaText(QString(), 13, m_engineCard);
    m_ffmpegPathValue->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_ffmpegPathValue->setWordWrap(true);

    m_ffprobePathCaption = new ElaText(QString(), 13, m_engineCard);
    m_ffprobePathValue = new ElaText(QString(), 13, m_engineCard);
    m_ffprobePathValue->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_ffprobePathValue->setWordWrap(true);

    m_versionCaption = new ElaText(QString(), 13, m_engineCard);
    m_versionValue = new ElaText(QString(), 13, m_engineCard);
    m_versionValue->setTextInteractionFlags(Qt::TextSelectableByMouse);

    m_capsCaption = new ElaText(QString(), 13, m_engineCard);
    m_capsValue = new ElaText(QString(), 13, m_engineCard);
    m_capsValue->setWordWrap(true);
    m_capsValue->setTextInteractionFlags(Qt::TextSelectableByMouse);

    engineForm->addRow(m_statusCaption, m_statusValue);
    engineForm->addRow(m_ffmpegPathCaption, m_ffmpegPathValue);
    engineForm->addRow(m_ffprobePathCaption, m_ffprobePathValue);
    engineForm->addRow(m_versionCaption, m_versionValue);
    engineForm->addRow(m_capsCaption, m_capsValue);
    engineLayout->addLayout(engineForm);

    containerLayout->addWidget(m_engineCard);

    connect(m_locator, &FFmpegLocator::pathsChanged, this, &SettingsPage::refresh);
    connect(m_caps, &FFmpegCapabilities::capabilitiesChanged, this, &SettingsPage::refresh);

    // Card 2: Preferences
    m_prefsCard = new ElaScrollPageArea(container);
    m_prefsCard->setMinimumHeight(0);
    m_prefsCard->setMaximumHeight(QWIDGETSIZE_MAX);
    auto *prefsLayout = new QVBoxLayout(m_prefsCard);
    prefsLayout->setContentsMargins(18, 16, 18, 16);
    prefsLayout->setSpacing(14);

    m_prefsCardTitle = new ElaText(QString(), 15, m_prefsCard);
    m_prefsCardTitle->setStyleSheet(QStringLiteral("font-weight: bold;"));
    prefsLayout->addWidget(m_prefsCardTitle);

    auto *prefsForm = new QFormLayout();
    prefsForm->setSpacing(14);

    m_languageCaption = new ElaText(QString(), 13, m_prefsCard);
    m_languageCombo = new ElaComboBox(m_prefsCard);
    m_languageCombo->addItems({QStringLiteral("English"), QStringLiteral("中文")});
    m_languageCombo->setCurrentIndex(
        m_languageManager->language().startsWith(QLatin1String("zh")) ? 1 : 0);
    connect(m_languageCombo, &ElaComboBox::currentIndexChanged, this, [=](int index) {
        m_languageManager->setLanguage(index == 0 ? QStringLiteral("en") : QStringLiteral("zh_CN"));
    });

    m_concurrencyCaption = new ElaText(QString(), 13, m_prefsCard);
    m_concurrencyCombo = new ElaComboBox(m_prefsCard);
    m_concurrencyCombo->addItems({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("4")});
    const int concurrencyIndex =
        m_concurrencyCombo->findText(QString::number(m_jobQueue->maxConcurrent()));
    m_concurrencyCombo->setCurrentIndex(concurrencyIndex >= 0 ? concurrencyIndex : 0);
    connect(m_concurrencyCombo, &ElaComboBox::currentIndexChanged, this, [this](int index) {
        if (index < 0) return;
        const int value = m_concurrencyCombo->itemText(index).toInt();
        m_jobQueue->setMaxConcurrent(value);
        QSettings settings(AppSettings::organization(), AppSettings::application());
        settings.setValue(QStringLiteral("jobs/maxConcurrent"), value);
    });

    m_notifyCaption = new ElaText(QString(), 13, m_prefsCard);
    m_notifyCheck = new ElaCheckBox(m_prefsCard);
    QSettings settings(AppSettings::organization(), AppSettings::application());
    m_notifyCheck->setChecked(settings.value(QStringLiteral("app/notifyOnFinish"), true).toBool());
    connect(m_notifyCheck, &ElaCheckBox::toggled, this, [](bool checked) {
        QSettings s(AppSettings::organization(), AppSettings::application());
        s.setValue(QStringLiteral("app/notifyOnFinish"), checked);
    });

    m_minimizeToTrayCaption = new ElaText(QString(), 13, m_prefsCard);
    m_minimizeToTrayCheck = new ElaCheckBox(m_prefsCard);
    m_minimizeToTrayCheck->setChecked(settings.value(QStringLiteral("app/minimizeToTray"), true).toBool());
    connect(m_minimizeToTrayCheck, &ElaCheckBox::toggled, this, [](bool checked) {
        QSettings s(AppSettings::organization(), AppSettings::application());
        s.setValue(QStringLiteral("app/minimizeToTray"), checked);
    });

    m_historyCaption = new ElaText(QString(), 13, m_prefsCard);
    m_clearHistoryButton = new ElaPushButton(m_prefsCard);
    m_clearHistoryButton->setFixedHeight(32);
    connect(m_clearHistoryButton, &ElaPushButton::clicked, m_recentFiles, &RecentFiles::clear);
    const auto syncHistoryButton = [this]() {
        m_clearHistoryButton->setEnabled(!m_recentFiles->files().isEmpty());
    };
    connect(m_recentFiles, &RecentFiles::changed, this, syncHistoryButton);
    syncHistoryButton();

    prefsForm->addRow(m_languageCaption, m_languageCombo);
    prefsForm->addRow(m_concurrencyCaption, m_concurrencyCombo);
    prefsForm->addRow(m_notifyCaption, m_notifyCheck);
    prefsForm->addRow(m_minimizeToTrayCaption, m_minimizeToTrayCheck);
    prefsForm->addRow(m_historyCaption, m_clearHistoryButton);
    prefsLayout->addLayout(prefsForm);

    containerLayout->addWidget(m_prefsCard);
    containerLayout->addStretch();

    scrollArea->setWidget(container);
    root->addWidget(scrollArea, 1);

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
    m_engineCardTitle->setText(trId("settings.engineCard"));
    m_prefsCardTitle->setText(trId("settings.prefsCard"));

    m_statusCaption->setText(trId("ffmpeg.status") + QStringLiteral(":"));
    m_ffmpegPathCaption->setText(trId("ffmpeg.ffmpegPath") + QStringLiteral(":"));
    m_ffprobePathCaption->setText(trId("ffmpeg.ffprobePath") + QStringLiteral(":"));
    m_versionCaption->setText(trId("ffmpeg.version") + QStringLiteral(":"));
    m_capsCaption->setText(trId("media.title") + QStringLiteral(":"));
    m_languageCaption->setText(trId("settings.language") + QStringLiteral(":"));
    m_concurrencyCaption->setText(trId("settings.concurrency") + QStringLiteral(":"));
    m_notifyCaption->setText(trId("settings.notify") + QStringLiteral(":"));
    m_notifyCheck->setText(trId("settings.notify.desc"));
    m_minimizeToTrayCaption->setText(trId("settings.minimizeToTray") + QStringLiteral(":"));
    m_minimizeToTrayCheck->setText(trId("settings.minimizeToTray.desc"));
    m_historyCaption->setText(trId("common.recent") + QStringLiteral(":"));
    m_rescanButton->setText(trId("ffmpeg.rescan"));
    m_clearHistoryButton->setText(trId("settings.clearHistory"));
    refresh();
}
