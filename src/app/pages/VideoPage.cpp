#include "VideoPage.h"

#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMimeData>
#include <QPainter>
#include <QStyleOption>
#include <QStackedWidget>
#include <QUrl>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>

#include "ElaLineEdit.h"
#include "ElaPivot.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaText.h"
#include "ffmpeg/FFmpegCommandBuilder.h"
#include "ffmpeg/FFmpegLocator.h"
#include "i18n/TrId.h"
#include "media/MediaProbeService.h"
#include "video/VideoClipTab.h"
#include "video/VideoAudioTab.h"
#include "video/VideoEffectsTab.h"
#include "video/VideoFramesTab.h"

VideoPage::VideoPage(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                     JobQueue *queue, MediaProbeService *probeService, QWidget *parent)
    : QWidget(parent)
    , m_locator(locator)
    , m_builder(builder)
    , m_queue(queue)
    , m_probeService(probeService)
{
    setAcceptDrops(true);

    m_mediaProbeWatcher = new QFutureWatcher<MediaFile>(this);
    connect(m_mediaProbeWatcher, &QFutureWatcher<MediaFile>::finished, this, &VideoPage::onMediaProbeFinished);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(30, 30, 30, 30);
    root->setSpacing(14);

    m_titleLabel = new ElaText(QString(), 13, this);
    m_titleLabel->setTextPixelSize(28);
    root->addWidget(m_titleLabel);

    // Common Source Video Input Card
    auto *inputCard = new ElaScrollPageArea(this);
    inputCard->setMinimumHeight(0);
    inputCard->setMaximumHeight(QWIDGETSIZE_MAX);
    auto *inputCardLayout = new QHBoxLayout(inputCard);
    inputCardLayout->setContentsMargins(18, 12, 18, 12);
    inputCardLayout->setSpacing(12);

    m_inputCaption = new ElaText(QString(), 13, inputCard);
    m_inputEdit = new ElaLineEdit(inputCard);
    m_inputEdit->setReadOnly(true);
    m_browseButton = new ElaPushButton(inputCard);
    connect(m_browseButton, &ElaPushButton::clicked, this, [this]() {
        const QString path = browseFile(trId("nav.video"),
                                        QStringLiteral("Video files (*.mp4 *.mkv *.mov *.webm *.avi *.flv *.wmv);;All files (*)"));
        if (!path.isEmpty()) {
            loadInputVideo(path);
        }
    });
    inputCardLayout->addWidget(m_inputCaption);
    inputCardLayout->addWidget(m_inputEdit, 1);
    inputCardLayout->addWidget(m_browseButton);
    root->addWidget(inputCard);

    // Segmented Navigation Switcher
    m_pivot = new ElaPivot(this);
    m_pivot->setPivotSpacing(16);
    m_pivot->setMarkWidth(90);
    root->addWidget(m_pivot);

    m_stackedWidget = new QStackedWidget(this);
    connect(m_pivot, &ElaPivot::pivotClicked, m_stackedWidget, &QStackedWidget::setCurrentIndex);
    root->addWidget(m_stackedWidget, 1);

    m_clipTab = new VideoClipTab(m_locator, m_builder, m_queue, m_probeService, this);
    m_audioTab = new VideoAudioTab(m_locator, m_builder, m_queue, m_probeService, this);
    m_effectsTab = new VideoEffectsTab(m_locator, m_builder, m_queue, m_probeService, this);
    m_framesTab = new VideoFramesTab(m_locator, m_builder, m_queue, m_probeService, this);

    m_stackedWidget->addWidget(m_clipTab);
    m_stackedWidget->addWidget(m_audioTab);
    m_stackedWidget->addWidget(m_effectsTab);
    m_stackedWidget->addWidget(m_framesTab);

    retranslateUi();
}

QString VideoPage::browseFile(const QString &title, const QString &filters)
{
    return QFileDialog::getOpenFileName(this, title, QString(), filters);
}

void VideoPage::loadInputVideo(const QString &path)
{
    if (path.isEmpty()) return;
    m_inputFile = path;
    m_inputEdit->setText(path);

    m_clipTab->setInputFile(path);
    m_audioTab->setInputFile(path);
    m_effectsTab->setInputFile(path);
    m_framesTab->setInputFile(path);

    probeMediaFile(path);
}

void VideoPage::probeMediaFile(const QString &path)
{
    if (path.isEmpty() || !m_probeService) return;
    if (m_mediaProbeWatcher->isRunning()) {
        m_mediaProbeWatcher->cancel();
        m_mediaProbeWatcher->waitForFinished();
    }
    MediaProbeService *probe = m_probeService;
    m_mediaProbeWatcher->setFuture(QtConcurrent::run([probe, path]() {
        return probe->probe(path);
    }));
}

void VideoPage::onMediaProbeFinished()
{
    m_mediaInfo = m_mediaProbeWatcher->result();
    m_clipTab->setMediaInfo(m_mediaInfo);
    m_audioTab->setMediaInfo(m_mediaInfo);
    m_effectsTab->setMediaInfo(m_mediaInfo);
    m_framesTab->setMediaInfo(m_mediaInfo);
}

void VideoPage::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        m_isDragHover = true;
        event->acceptProposedAction();
        update();
    }
}

void VideoPage::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event)
    m_isDragHover = false;
    update();
}

void VideoPage::dropEvent(QDropEvent *event)
{
    m_isDragHover = false;
    update();
    const QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;
    loadInputVideo(urls.first().toLocalFile());
    event->acceptProposedAction();
}

void VideoPage::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    if (m_isDragHover) {
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor(0, 120, 212, 220), 2, Qt::DashLine));
        p.setBrush(QColor(0, 120, 212, 20));
        p.drawRoundedRect(rect().adjusted(10, 10, -10, -10), 10, 10);
    }
}

void VideoPage::retranslateUi()
{
    const int currentIdx = m_pivot->getCurrentIndex();
    if (!m_clipTabTitle.isEmpty()) {
        m_pivot->removePivot(m_clipTabTitle);
        m_pivot->removePivot(m_audioTabTitle);
        m_pivot->removePivot(m_effectsTabTitle);
        m_pivot->removePivot(m_framesTabTitle);
    }
    m_clipTabTitle = trId("video.tab.clip");
    m_audioTabTitle = trId("video.tab.audio");
    m_effectsTabTitle = trId("video.tab.effects");
    m_framesTabTitle = trId("video.tab.frames");
    m_pivot->appendPivot(m_clipTabTitle);
    m_pivot->appendPivot(m_audioTabTitle);
    m_pivot->appendPivot(m_effectsTabTitle);
    m_pivot->appendPivot(m_framesTabTitle);
    if (currentIdx >= 0 && currentIdx < m_stackedWidget->count()) {
        m_pivot->setCurrentIndex(currentIdx);
    }

    m_titleLabel->setText(trId("video.title"));
    m_inputCaption->setText(trId("common.source") + QStringLiteral(":"));
    m_browseButton->setText(trId("common.browse"));

    if (m_clipTab) m_clipTab->retranslateUi();
    if (m_audioTab) m_audioTab->retranslateUi();
    if (m_effectsTab) m_effectsTab->retranslateUi();
    if (m_framesTab) m_framesTab->retranslateUi();
}

void VideoPage::prewarm()
{
    if (m_clipTab) {
        m_clipTab->prewarm();
    }
    ensurePolished();
    if (layout()) {
        layout()->activate();
    }
}
