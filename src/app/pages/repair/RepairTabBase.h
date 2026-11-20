#pragma once

#include <QWidget>
#include <QString>

class FFmpegLocator;
class MediaProbeService;
class JobQueue;
class ElaScrollPageArea;
class ElaText;
class ElaPushButton;

class RepairTabBase : public QWidget
{
    Q_OBJECT

public:
    explicit RepairTabBase(FFmpegLocator *ffmpegLocator,
                           MediaProbeService *probeService,
                           JobQueue *jobQueue,
                           QWidget *parent = nullptr);
    ~RepairTabBase() override = default;

    virtual void retranslateUi() = 0;
    virtual void handleDroppedFile(const QString &filePath) { Q_UNUSED(filePath); }

protected:
    static void openFolderAndSelectFile(const QString &filePath);
    static void playMediaFile(const QString &filePath);

    ElaScrollPageArea *createCard(QWidget *parentWidget = nullptr);
    ElaText *makeCaption(const QString &text, QWidget *parentWidget = nullptr);
    ElaPushButton *createCardButton(const QString &text, int height = 32, QWidget *parentWidget = nullptr);

    FFmpegLocator *m_ffmpegLocator = nullptr;
    MediaProbeService *m_probeService = nullptr;
    JobQueue *m_jobQueue = nullptr;
};
