#pragma once

#include <QWidget>

class FFmpegLocator;
class FFmpegCapabilities;
class JobQueue;
class UntruncLocator;
class ElaScrollArea;
class ElaScrollPageArea;
class ElaText;
class ElaPushButton;

class HomePage : public QWidget
{
    Q_OBJECT

public:
    explicit HomePage(FFmpegLocator *locator, FFmpegCapabilities *caps,
                      JobQueue *queue = nullptr, UntruncLocator *untrunc = nullptr,
                      QWidget *parent = nullptr);

    void setTargetPages(QWidget *convertPage, QWidget *videoPage,
                        QWidget *repairPage, QWidget *batchPage,
                        QWidget *queuePage);

    void retranslateUi();

signals:
    void navigationRequested(QWidget *page);

private:
    void refreshHwStatus();
    void refreshUntruncStatus();
    void refreshQueueStatus();

    FFmpegLocator *m_locator;
    FFmpegCapabilities *m_caps;
    JobQueue *m_queue;
    UntruncLocator *m_untrunc;

    QWidget *m_convertPage = nullptr;
    QWidget *m_videoPage = nullptr;
    QWidget *m_repairPage = nullptr;
    QWidget *m_batchPage = nullptr;
    QWidget *m_queuePage = nullptr;

    ElaScrollArea *m_scrollArea = nullptr;

    // Header
    ElaText *m_titleLabel = nullptr;
    ElaText *m_subtitleLabel = nullptr;

    // Engine Card
    ElaScrollPageArea *m_envCard = nullptr;
    ElaText *m_envTitle = nullptr;
    ElaText *m_versionCaption = nullptr;
    ElaText *m_versionValue = nullptr;
    ElaText *m_hwCaption = nullptr;
    ElaText *m_hwValue = nullptr;
    ElaText *m_codecsCaption = nullptr;
    ElaText *m_codecsValue = nullptr;
    ElaText *m_untruncCaption = nullptr;
    ElaText *m_untruncValue = nullptr;
    ElaPushButton *m_rescanButton = nullptr;

    // Queue Status Card
    ElaScrollPageArea *m_queueCard = nullptr;
    ElaText *m_queueTitle = nullptr;
    ElaText *m_queueRunningText = nullptr;
    ElaText *m_queuePendingText = nullptr;
    ElaText *m_queueCompletedText = nullptr;
    ElaPushButton *m_viewQueueButton = nullptr;

    // Quick Launch Section
    ElaText *m_quickLaunchTitle = nullptr;

    // Launch Cards
    ElaScrollPageArea *m_cardConvert = nullptr;
    ElaText *m_cardConvertTitle = nullptr;
    ElaText *m_cardConvertDesc = nullptr;
    ElaPushButton *m_cardConvertBtn = nullptr;

    ElaScrollPageArea *m_cardVideo = nullptr;
    ElaText *m_cardVideoTitle = nullptr;
    ElaText *m_cardVideoDesc = nullptr;
    ElaPushButton *m_cardVideoBtn = nullptr;

    ElaScrollPageArea *m_cardRepair = nullptr;
    ElaText *m_cardRepairTitle = nullptr;
    ElaText *m_cardRepairDesc = nullptr;
    ElaPushButton *m_cardRepairBtn = nullptr;

    ElaScrollPageArea *m_cardBatch = nullptr;
    ElaText *m_cardBatchTitle = nullptr;
    ElaText *m_cardBatchDesc = nullptr;
    ElaPushButton *m_cardBatchBtn = nullptr;
};
