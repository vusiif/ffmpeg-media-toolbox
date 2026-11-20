#pragma once

#include <QWidget>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>

#include "media/MediaFile.h"

class FFmpegLocator;
class FFmpegCommandBuilder;
class JobQueue;
class MediaProbeService;
class ElaScrollArea;
class ElaScrollPageArea;
class ElaPushButton;
class ElaText;

class VideoTabBase : public QWidget
{
    Q_OBJECT

public:
    explicit VideoTabBase(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                          JobQueue *queue, MediaProbeService *probeService = nullptr,
                          QWidget *parent = nullptr);
    ~VideoTabBase() override = default;

    virtual void setInputFile(const QString &path);
    virtual void setMediaInfo(const MediaFile &info);
    virtual void retranslateUi() = 0;

protected:
    void enqueue(const QString &name, const QStringList &args);
    QString browseFile(const QString &title, const QString &filters);

    ElaScrollPageArea *createCard(ElaText *&titleLabel);
    ElaText *makeCaption(QWidget *parent, const QString &text = QString());
    ElaPushButton *createCardButton(ElaScrollPageArea *card, const QString &text = QString());

    FFmpegLocator *m_locator = nullptr;
    FFmpegCommandBuilder *m_builder = nullptr;
    JobQueue *m_queue = nullptr;
    MediaProbeService *m_probeService = nullptr;

    QString m_inputFile;
    MediaFile m_mediaInfo;
    QVBoxLayout *m_bodyLayout = nullptr;
};
