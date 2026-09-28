#pragma once

#include "ElaWindow.h"

class FFmpegLocator;
class FFmpegCapabilities;
class FFmpegCommandBuilder;
class MediaProbeService;
class JobQueue;
class LanguageManager;
class HomePage;
class ConvertPage;
class ImagePage;
class JoinPage;
class BatchPage;
class QueuePage;
class SettingsPage;
class VideoPage;

class MainWindow : public ElaWindow
{
    Q_OBJECT

public:
    explicit MainWindow(LanguageManager *languageManager, QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void initPages();
    void retranslateUi();

    FFmpegLocator *m_ffmpegLocator = nullptr;
    FFmpegCapabilities *m_ffmpegCaps = nullptr;
    FFmpegCommandBuilder *m_commandBuilder = nullptr;
    MediaProbeService *m_mediaProbe = nullptr;
    JobQueue *m_jobQueue = nullptr;
    LanguageManager *m_languageManager = nullptr;

    HomePage *m_homePage = nullptr;
    ConvertPage *m_convertPage = nullptr;
    ImagePage *m_imagePage = nullptr;
    JoinPage *m_joinPage = nullptr;
    BatchPage *m_batchPage = nullptr;
    QueuePage *m_queuePage = nullptr;
    SettingsPage *m_settingsPage = nullptr;
    VideoPage *m_videoPage = nullptr;

    QString m_settingsKey;
};
