#pragma once

#include <QFutureWatcher>
#include <QWidget>

#include "media/MediaFile.h"

class FFmpegLocator;
class FFmpegCommandBuilder;
class MediaProbeService;
class JobQueue;
class QStackedWidget;
class ElaPivot;
class ElaText;
class ElaPushButton;
class ElaLineEdit;

class VideoClipTab;
class VideoAudioTab;
class VideoEffectsTab;
class VideoFramesTab;

class VideoPage : public QWidget
{
    Q_OBJECT

public:
    explicit VideoPage(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                       JobQueue *queue, MediaProbeService *probeService = nullptr,
                       QWidget *parent = nullptr);
    ~VideoPage() override;

    void retranslateUi();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void loadInputVideo(const QString &path);
    void probeMediaFile(const QString &path);
    void onMediaProbeFinished();
    QString browseFile(const QString &title, const QString &filters);

    FFmpegLocator *m_locator;
    FFmpegCommandBuilder *m_builder;
    JobQueue *m_queue;
    MediaProbeService *m_probeService;

    QString m_inputFile;
    QFutureWatcher<MediaFile> *m_mediaProbeWatcher = nullptr;
    MediaFile m_mediaInfo;

    ElaText *m_titleLabel = nullptr;
    ElaText *m_inputCaption = nullptr;
    ElaLineEdit *m_inputEdit = nullptr;
    ElaPushButton *m_browseButton = nullptr;

    ElaPivot *m_pivot = nullptr;
    QStackedWidget *m_stackedWidget = nullptr;
    QString m_clipTabTitle;
    QString m_audioTabTitle;
    QString m_effectsTabTitle;
    QString m_framesTabTitle;

    VideoClipTab *m_clipTab = nullptr;
    VideoAudioTab *m_audioTab = nullptr;
    VideoEffectsTab *m_effectsTab = nullptr;
    VideoFramesTab *m_framesTab = nullptr;
    bool m_isDragHover = false;
};
