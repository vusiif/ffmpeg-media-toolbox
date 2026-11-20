#include "VideoTabBase.h"

#include <QFileDialog>
#include <QHBoxLayout>

#include "ElaMessageBar.h"
#include "ElaPushButton.h"
#include "ElaScrollArea.h"
#include "ElaScrollPageArea.h"
#include "ElaText.h"
#include "ffmpeg/FFmpegCommandBuilder.h"
#include "ffmpeg/FFmpegLocator.h"
#include "i18n/TrId.h"
#include "jobs/Job.h"
#include "jobs/JobQueue.h"

VideoTabBase::VideoTabBase(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                           JobQueue *queue, MediaProbeService *probeService,
                           QWidget *parent)
    : QWidget(parent)
    , m_locator(locator)
    , m_builder(builder)
    , m_queue(queue)
    , m_probeService(probeService)
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 4, 0, 0);
    rootLayout->setSpacing(0);

    auto *scroll = new ElaScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setIsGrabGesture(true);

    auto *bodyWidget = new QWidget(scroll);
    m_bodyLayout = new QVBoxLayout(bodyWidget);
    m_bodyLayout->setContentsMargins(0, 0, 10, 10);
    m_bodyLayout->setSpacing(10);

    scroll->setWidget(bodyWidget);
    rootLayout->addWidget(scroll);
}

void VideoTabBase::setInputFile(const QString &path)
{
    m_inputFile = path;
}

void VideoTabBase::setMediaInfo(const MediaFile &info)
{
    m_mediaInfo = info;
}

void VideoTabBase::enqueue(const QString &name, const QStringList &args)
{
    if (args.isEmpty() || !m_queue || !m_locator) return;
    auto *job = new Job();
    job->setName(name);
    job->setCommand(m_locator->ffmpegPath(), args);
    m_queue->addJob(job);
    ElaMessageBar::success(ElaMessageBarType::TopRight,
                           trId("video.title"),
                           trId("common.taskAdded") + QStringLiteral(": ") + name,
                           2500, this);
}

QString VideoTabBase::browseFile(const QString &title, const QString &filters)
{
    return QFileDialog::getOpenFileName(this, title, QString(), filters);
}

ElaScrollPageArea *VideoTabBase::createCard(ElaText *&titleLabel)
{
    auto *card = new ElaScrollPageArea(this);
    card->setMinimumHeight(0);
    card->setMaximumHeight(QWIDGETSIZE_MAX);

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 12, 16, 14);
    layout->setSpacing(10);

    titleLabel = new ElaText(QString(), 15, card);
    titleLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));
    layout->addWidget(titleLabel);
    return card;
}

ElaText *VideoTabBase::makeCaption(QWidget *parent, const QString &text)
{
    auto *caption = new ElaText(text, 13, parent);
    return caption;
}

ElaPushButton *VideoTabBase::createCardButton(ElaScrollPageArea *card, const QString &text)
{
    auto *button = new ElaPushButton(text, card);
    button->setFixedHeight(36);
    button->setMinimumWidth(140);
    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    btnRow->addWidget(button);
    if (auto *layout = qobject_cast<QVBoxLayout *>(card->layout())) {
        layout->addLayout(btnRow);
    }
    return button;
}
