#pragma once

#include <QWidget>

class FFmpegLocator;
class FFmpegCapabilities;
class RecentFiles;
class JobQueue;
class LanguageManager;
class ElaText;
class ElaPushButton;
class ElaComboBox;
class ElaScrollPageArea;

class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(FFmpegLocator *locator, FFmpegCapabilities *caps,
                          RecentFiles *recentFiles, JobQueue *jobQueue,
                          LanguageManager *languageManager,
                          QWidget *parent = nullptr);

    void retranslateUi();

private:
    void refresh();

    FFmpegLocator *m_locator;
    FFmpegCapabilities *m_caps;
    RecentFiles *m_recentFiles;
    JobQueue *m_jobQueue;
    LanguageManager *m_languageManager;

    ElaText *m_titleLabel = nullptr;

    // Engine Card
    ElaScrollPageArea *m_engineCard = nullptr;
    ElaText *m_engineCardTitle = nullptr;
    ElaText *m_statusCaption = nullptr;
    ElaText *m_statusValue = nullptr;
    ElaText *m_ffmpegPathCaption = nullptr;
    ElaText *m_ffmpegPathValue = nullptr;
    ElaText *m_ffprobePathCaption = nullptr;
    ElaText *m_ffprobePathValue = nullptr;
    ElaText *m_versionCaption = nullptr;
    ElaText *m_versionValue = nullptr;
    ElaText *m_capsCaption = nullptr;
    ElaText *m_capsValue = nullptr;
    ElaPushButton *m_rescanButton = nullptr;

    // Preferences Card
    ElaScrollPageArea *m_prefsCard = nullptr;
    ElaText *m_prefsCardTitle = nullptr;
    ElaText *m_languageCaption = nullptr;
    ElaComboBox *m_languageCombo = nullptr;
    ElaText *m_concurrencyCaption = nullptr;
    ElaComboBox *m_concurrencyCombo = nullptr;
    ElaText *m_historyCaption = nullptr;
    ElaPushButton *m_clearHistoryButton = nullptr;
};
