#include "ConvertSourceCard.h"

#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMimeData>
#include <QPainter>
#include <QVBoxLayout>
#include <QtConcurrent>

#include "ElaComboBox.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include "ElaText.h"
#include "app/RecentFiles.h"
#include "i18n/TrId.h"
#include "media/MediaProbeService.h"

ConvertSourceCard::ConvertSourceCard(MediaProbeService *probe, RecentFiles *recentFiles, QWidget *parent)
    : ElaScrollPageArea(parent)
    , m_probe(probe)
    , m_recentFiles(recentFiles)
{
    setMinimumHeight(0);
    setMaximumHeight(QWIDGETSIZE_MAX);
    setAcceptDrops(true);

    auto *inputLayout = new QVBoxLayout(this);
    inputLayout->setContentsMargins(18, 16, 18, 16);
    inputLayout->setSpacing(10);

    m_inputCardTitle = new ElaText(QString(), 15, this);
    m_inputCardTitle->setStyleSheet(QStringLiteral("font-weight: bold;"));
    inputLayout->addWidget(m_inputCardTitle);

    auto *inputRow = new QHBoxLayout();
    m_inputCaption = new ElaText(QString(), 13, this);
    m_inputEdit = new ElaLineEdit(this);
    m_inputEdit->setReadOnly(true);
    m_browseButton = new ElaPushButton(this);
    connect(m_browseButton, &ElaPushButton::clicked, this, &ConvertSourceCard::browseInput);
    inputRow->addWidget(m_inputCaption);
    inputRow->addWidget(m_inputEdit, 1);
    inputRow->addWidget(m_browseButton);
    inputLayout->addLayout(inputRow);

    auto *recentRow = new QHBoxLayout();
    m_recentCaption = new ElaText(QString(), 13, this);
    m_recentCombo = new ElaComboBox(this);
    m_recentCaption->setVisible(false);
    m_recentCombo->setVisible(false);
    recentRow->addWidget(m_recentCaption);
    recentRow->addWidget(m_recentCombo, 1);
    inputLayout->addLayout(recentRow);

    m_infoLabel = new ElaText(QString(), 13, this);
    m_infoLabel->setTextPixelSize(12);
    inputLayout->addWidget(m_infoLabel);

    m_metaCaption = new ElaText(QString(), 13, this);
    m_metaCaption->setStyleSheet(QStringLiteral("font-weight: bold;"));
    m_metaValue = new ElaText(QString(), 13, this);
    m_metaValue->setWordWrap(true);
    m_metaCaption->setVisible(false);
    m_metaValue->setVisible(false);
    inputLayout->addWidget(m_metaCaption);
    inputLayout->addWidget(m_metaValue);

    m_probeWatcher = new QFutureWatcher<MediaFile>(this);
    connect(m_probeWatcher, &QFutureWatcher<MediaFile>::finished,
            this, &ConvertSourceCard::onProbeWatcherFinished);

    if (m_recentFiles) {
        connect(m_recentFiles, &RecentFiles::changed, this, &ConvertSourceCard::refreshRecentList);
    }
    connect(m_recentCombo, &ElaComboBox::currentIndexChanged, this, [this](int index) {
        if (index < 0) return;
        const QString path = m_recentCombo->itemText(index);
        if (path.isEmpty() || path == m_inputFile) return;
        loadRecentFile(path);
        if (m_recentFiles) {
            m_recentFiles->add(path);
        }
    });

    refreshRecentList();
    retranslateUi();
}

ConvertSourceCard::~ConvertSourceCard()
{
    if (m_probeWatcher) {
        disconnect(m_probeWatcher, nullptr, this, nullptr);
        if (m_probeWatcher->isRunning()) {
            m_probeWatcher->cancel();
            m_probeWatcher->waitForFinished();
        }
    }
}

QString ConvertSourceCard::inputFile() const
{
    return m_inputFile;
}

void ConvertSourceCard::setInputFile(const QString &path)
{
    if (m_inputFile == path) return;
    loadRecentFile(path);
    if (m_recentFiles && !path.isEmpty()) {
        m_recentFiles->add(path);
    }
}

void ConvertSourceCard::browseInput()
{
    const QString filter = QStringLiteral(
        "Media Files (*.mp4 *.mkv *.mov *.avi *.ts *.webm *.flv *.wmv *.mp3 *.m4a *.aac *.flac *.wav *.ogg);;All Files (*)");
    const QString path = QFileDialog::getOpenFileName(this, trId("convert.input"), QString(), filter);
    if (path.isEmpty()) return;
    setInputFile(path);
}

void ConvertSourceCard::loadRecentFile(const QString &path)
{
    m_inputFile = path;
    m_inputEdit->setText(path);
    m_infoLabel->clear();
    showMetadata({});
    emit inputFileChanged(m_inputFile);
    startProbe(false);
}

void ConvertSourceCard::startProbe(bool enqueueAfter)
{
    if (m_inputFile.isEmpty() || !m_probe) return;
    if (m_probeWatcher->isRunning()) {
        m_probeWatcher->cancel();
        m_probeWatcher->waitForFinished();
    }
    m_enqueueAfterProbe = enqueueAfter;
    m_infoLabel->setText(trId("convert.probing"));
    const QString path = m_inputFile;
    auto *service = m_probe;
    m_probeWatcher->setFuture(QtConcurrent::run([service, path]() {
        return service->probe(path);
    }));
}

void ConvertSourceCard::onProbeWatcherFinished()
{
    const MediaFile info = m_probeWatcher->result();
    m_probeInfo = info;

    if (!info.isValid()) {
        m_infoLabel->setText(trId("convert.invalidFile"));
        showMetadata({});
        emit probeFinished(info, m_enqueueAfterProbe);
        return;
    }

    QStringList parts;
    parts << info.formatName;
    parts << QStringLiteral("%1s").arg(info.duration, 0, 'f', 1);
    if (info.hasVideo()) {
        const VideoStream &video = info.primaryVideo();
        parts << QStringLiteral("%1x%2").arg(video.width).arg(video.height);
        parts << video.codec;
        if (video.isHdr) {
            parts << QStringLiteral("HDR (%1)").arg(video.colorTransfer.isEmpty() ? QStringLiteral("BT.2020") : video.colorTransfer);
        }
        if (info.videoStreams.size() > 1) {
            parts << QStringLiteral("(+%1 video)").arg(info.videoStreams.size() - 1);
        }
    }
    if (info.hasAudio()) {
        if (info.audioStreams.size() > 1) {
            parts << QStringLiteral("%1 audios (%2)").arg(info.audioStreams.size()).arg(info.primaryAudio().codec);
        } else {
            parts << info.primaryAudio().codec;
        }
    }
    if (!info.subtitleStreams.isEmpty()) {
        parts << QStringLiteral("%1 subs").arg(info.subtitleStreams.size());
    }
    m_infoLabel->setText(parts.join(QStringLiteral(" | ")));
    showMetadata(info.formatMetadata);

    emit probeFinished(info, m_enqueueAfterProbe);
}

void ConvertSourceCard::showMetadata(const QVariantMap &tags)
{
    if (tags.isEmpty()) {
        m_metaCaption->setVisible(false);
        m_metaValue->setVisible(false);
        m_metaValue->clear();
        return;
    }
    QStringList lines;
    for (auto it = tags.constBegin(); it != tags.constEnd(); ++it) {
        lines << QStringLiteral("%1: %2").arg(it.key(), it.value().toString());
    }
    m_metaValue->setText(lines.join(QStringLiteral("\n")));
    m_metaCaption->setVisible(true);
    m_metaValue->setVisible(true);
}

void ConvertSourceCard::refreshRecentList()
{
    if (!m_recentFiles) return;
    const QStringList files = m_recentFiles->files();
    m_recentCombo->blockSignals(true);
    m_recentCombo->clear();
    m_recentCombo->addItems(files);
    m_recentCombo->blockSignals(false);
    const bool hasRecent = !files.isEmpty();
    m_recentCaption->setVisible(hasRecent);
    m_recentCombo->setVisible(hasRecent);
}

void ConvertSourceCard::retranslateUi()
{
    if (m_inputCardTitle) m_inputCardTitle->setText(trId("convert.card.input"));
    m_inputCaption->setText(trId("convert.input") + QStringLiteral(":"));
    m_recentCaption->setText(trId("common.recent") + QStringLiteral(":"));
    m_browseButton->setText(trId("common.browse"));
    m_metaCaption->setText(trId("metadata.title"));
    m_metaCaption->setVisible(!m_metaValue->text().isEmpty());
}

void ConvertSourceCard::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        m_isDragHover = true;
        event->acceptProposedAction();
        update();
    }
}

void ConvertSourceCard::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event)
    m_isDragHover = false;
    update();
}

void ConvertSourceCard::dropEvent(QDropEvent *event)
{
    m_isDragHover = false;
    update();
    const auto urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;
    const QString local = urls.first().toLocalFile();
    if (!local.isEmpty()) {
        setInputFile(local);
    }
    event->acceptProposedAction();
}

void ConvertSourceCard::paintEvent(QPaintEvent *event)
{
    ElaScrollPageArea::paintEvent(event);
    if (m_isDragHover) {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor(0, 120, 212, 220), 2, Qt::DashLine));
        p.setBrush(QColor(0, 120, 212, 25));
        p.drawRoundedRect(rect().adjusted(4, 4, -4, -4), 8, 8);
    }
}
