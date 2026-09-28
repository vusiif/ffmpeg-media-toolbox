#include "VideoPage.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMessageBox>
#include <QMimeData>
#include <QScrollArea>
#include <QUrl>
#include <QVBoxLayout>

#include "ElaCheckBox.h"
#include "ElaComboBox.h"
#include "ElaDoubleSpinBox.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "conversion/RemuxPolicy.h"
#include "ffmpeg/FFmpegCommandBuilder.h"
#include "ffmpeg/FFmpegLocator.h"
#include "i18n/TrId.h"
#include "jobs/Job.h"
#include "jobs/JobQueue.h"
#include "output/OutputNaming.h"

VideoPage::VideoPage(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                     JobQueue *queue, QWidget *parent)
    : QWidget(parent)
    , m_locator(locator)
    , m_builder(builder)
    , m_queue(queue)
{
    setAcceptDrops(true);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(30, 30, 30, 30);
    root->setSpacing(16);

    m_titleLabel = new ElaText(QString(), 13, this);
    m_titleLabel->setTextPixelSize(28);
    root->addWidget(m_titleLabel);

    auto *inputRow = new QHBoxLayout();
    m_inputCaption = new ElaText(QString(), 13, this);
    m_inputEdit = new ElaLineEdit(this);
    m_inputEdit->setReadOnly(true);
    m_browseButton = new ElaPushButton(this);
    connect(m_browseButton, &ElaPushButton::clicked, this, [=]() {
        const QString path = browseFile(trId("nav.video"),
                                        QStringLiteral("Video files (*.mp4 *.mkv *.mov *.webm *.avi *.flv *.wmv);;All files (*)"));
        if (!path.isEmpty()) {
            m_inputFile = path;
            m_inputEdit->setText(path);
        }
    });
    inputRow->addWidget(m_inputCaption);
    inputRow->addWidget(m_inputEdit, 1);
    inputRow->addWidget(m_browseButton);
    root->addLayout(inputRow);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *scrollBody = new QWidget(scroll);
    auto *body = new QVBoxLayout(scrollBody);
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(12);

    auto makeRow = [](QGroupBox *box) {
        auto *row = new QFormLayout(box);
        row->setSpacing(8);
        return row;
    };
    auto makeCaption = [](QGroupBox *box) {
        return new ElaText(QString(), 13, box);
    };
    auto makeQueueButton = [](QGroupBox *box) {
        auto *button = new ElaPushButton(box);
        auto *layout = qobject_cast<QFormLayout *>(box->layout());
        if (layout) {
            layout->addRow(button);
        }
        return button;
    };

    // Trim
    m_trimBox = new QGroupBox(scrollBody);
    auto *trimForm = makeRow(m_trimBox);
    m_trimStartCaption = makeCaption(m_trimBox);
    m_trimStart = new ElaDoubleSpinBox(m_trimBox);
    m_trimStart->setRange(0, 86400);
    m_trimStart->setDecimals(1);
    m_trimEndCaption = makeCaption(m_trimBox);
    m_trimEnd = new ElaDoubleSpinBox(m_trimBox);
    m_trimEnd->setRange(0, 86400);
    m_trimEnd->setDecimals(1);
    m_trimFormatCaption = makeCaption(m_trimBox);
    m_trimFormat = new ElaComboBox(m_trimBox);
    m_trimFormat->addItems({QStringLiteral(""), QStringLiteral("mp4"), QStringLiteral("mkv"),
                            QStringLiteral("mov"), QStringLiteral("webm")});
    m_trimCopyCaption = makeCaption(m_trimBox);
    m_trimCopy = new ElaCheckBox(m_trimBox);
    trimForm->addRow(m_trimStartCaption, m_trimStart);
    trimForm->addRow(m_trimEndCaption, m_trimEnd);
    trimForm->addRow(m_trimFormatCaption, m_trimFormat);
    trimForm->addRow(m_trimCopyCaption, m_trimCopy);
    m_trimButton = makeQueueButton(m_trimBox);
    connect(m_trimButton, &ElaPushButton::clicked, this, &VideoPage::trim);
    body->addWidget(m_trimBox);

    // Extract audio
    m_audioBox = new QGroupBox(scrollBody);
    auto *audioForm = makeRow(m_audioBox);
    m_audioFormatCaption = makeCaption(m_audioBox);
    m_audioFormat = new ElaComboBox(m_audioBox);
    m_audioFormat->addItems({QStringLiteral("mp3"), QStringLiteral("aac"), QStringLiteral("flac"),
                             QStringLiteral("wav"), QStringLiteral("opus"), QStringLiteral("m4a")});
    m_audioBitrateCaption = makeCaption(m_audioBox);
    m_audioBitrate = new ElaSpinBox(m_audioBox);
    m_audioBitrate->setRange(0, 512);
    m_audioBitrate->setValue(192);
    audioForm->addRow(m_audioFormatCaption, m_audioFormat);
    audioForm->addRow(m_audioBitrateCaption, m_audioBitrate);
    m_audioButton = makeQueueButton(m_audioBox);
    connect(m_audioButton, &ElaPushButton::clicked, this, &VideoPage::extractAudio);
    body->addWidget(m_audioBox);

    // Mute
    m_muteBox = new QGroupBox(scrollBody);
    auto *muteForm = makeRow(m_muteBox);
    m_muteFormatCaption = makeCaption(m_muteBox);
    m_muteFormat = new ElaComboBox(m_muteBox);
    m_muteFormat->addItems({QStringLiteral(""), QStringLiteral("mp4"), QStringLiteral("mkv"),
                            QStringLiteral("webm")});
    muteForm->addRow(m_muteFormatCaption, m_muteFormat);
    m_muteButton = makeQueueButton(m_muteBox);
    connect(m_muteButton, &ElaPushButton::clicked, this, &VideoPage::mute);
    body->addWidget(m_muteBox);

    // Screenshot
    m_shotBox = new QGroupBox(scrollBody);
    auto *shotForm = makeRow(m_shotBox);
    m_shotTimeCaption = makeCaption(m_shotBox);
    m_shotTime = new ElaDoubleSpinBox(m_shotBox);
    m_shotTime->setRange(0, 86400);
    m_shotTime->setDecimals(1);
    m_shotFormatCaption = makeCaption(m_shotBox);
    m_shotFormat = new ElaComboBox(m_shotBox);
    m_shotFormat->addItems({QStringLiteral("png"), QStringLiteral("jpeg"), QStringLiteral("webp")});
    shotForm->addRow(m_shotTimeCaption, m_shotTime);
    shotForm->addRow(m_shotFormatCaption, m_shotFormat);
    m_shotButton = makeQueueButton(m_shotBox);
    connect(m_shotButton, &ElaPushButton::clicked, this, &VideoPage::screenshot);
    body->addWidget(m_shotBox);

    // GIF
    m_gifBox = new QGroupBox(scrollBody);
    auto *gifForm = makeRow(m_gifBox);
    m_gifStartCaption = makeCaption(m_gifBox);
    m_gifStart = new ElaDoubleSpinBox(m_gifBox);
    m_gifStart->setRange(0, 86400);
    m_gifStart->setDecimals(1);
    m_gifEndCaption = makeCaption(m_gifBox);
    m_gifEnd = new ElaDoubleSpinBox(m_gifBox);
    m_gifEnd->setRange(0, 86400);
    m_gifEnd->setDecimals(1);
    m_gifFpsCaption = makeCaption(m_gifBox);
    m_gifFps = new ElaSpinBox(m_gifBox);
    m_gifFps->setRange(1, 60);
    m_gifFps->setValue(12);
    m_gifWidthCaption = makeCaption(m_gifBox);
    m_gifWidth = new ElaSpinBox(m_gifBox);
    m_gifWidth->setRange(0, 4096);
    m_gifWidth->setValue(480);
    gifForm->addRow(m_gifStartCaption, m_gifStart);
    gifForm->addRow(m_gifEndCaption, m_gifEnd);
    gifForm->addRow(m_gifFpsCaption, m_gifFps);
    gifForm->addRow(m_gifWidthCaption, m_gifWidth);
    m_gifButton = makeQueueButton(m_gifBox);
    connect(m_gifButton, &ElaPushButton::clicked, this, &VideoPage::makeGif);
    body->addWidget(m_gifBox);

    // Video join
    m_joinBox = new QGroupBox(scrollBody);
    auto *joinLayout = new QVBoxLayout(m_joinBox);
    joinLayout->setSpacing(8);
    m_joinList = new QListWidget(m_joinBox);
    joinLayout->addWidget(m_joinList);
    auto *joinButtons = new QHBoxLayout();
    m_joinAddButton = new ElaPushButton(m_joinBox);
    m_joinRemoveButton = new ElaPushButton(m_joinBox);
    connect(m_joinAddButton, &ElaPushButton::clicked, this, [=]() {
        const QStringList paths = QFileDialog::getOpenFileNames(
            this, trId("video.join"), QString(),
            QStringLiteral("Video files (*.mp4 *.mkv *.mov *.webm *.avi);;All files (*)"));
        for (const QString &path : paths) {
            m_joinFiles.append(path);
            m_joinList->addItem(QFileInfo(path).fileName());
        }
    });
    connect(m_joinRemoveButton, &ElaPushButton::clicked, this, [=]() {
        const QList<QListWidgetItem *> selected = m_joinList->selectedItems();
        for (QListWidgetItem *item : selected) {
            const int row = m_joinList->row(item);
            m_joinFiles.removeAt(row);
            delete m_joinList->takeItem(row);
        }
    });
    joinButtons->addWidget(m_joinAddButton);
    joinButtons->addWidget(m_joinRemoveButton);
    joinButtons->addStretch();
    joinLayout->addLayout(joinButtons);
    m_joinButton = new ElaPushButton(m_joinBox);
    connect(m_joinButton, &ElaPushButton::clicked, this, &VideoPage::joinVideos);
    joinLayout->addWidget(m_joinButton);
    body->addWidget(m_joinBox);

    // Sequence
    m_seqBox = new QGroupBox(scrollBody);
    auto *seqForm = makeRow(m_seqBox);
    m_seqModeCaption = makeCaption(m_seqBox);
    m_seqMode = new ElaComboBox(m_seqBox);
    m_seqMode->addItems({QString(), QString()});
    m_seqPatternCaption = makeCaption(m_seqBox);
    m_seqPattern = new ElaLineEdit(m_seqBox);
    m_seqPattern->setText(QStringLiteral("frame_%04d.png"));
    m_seqFpsCaption = makeCaption(m_seqBox);
    m_seqFps = new ElaDoubleSpinBox(m_seqBox);
    m_seqFps->setRange(0.1, 240);
    m_seqFps->setValue(25);
    seqForm->addRow(m_seqModeCaption, m_seqMode);
    seqForm->addRow(m_seqPatternCaption, m_seqPattern);
    seqForm->addRow(m_seqFpsCaption, m_seqFps);
    m_seqButton = makeQueueButton(m_seqBox);
    connect(m_seqButton, &ElaPushButton::clicked, this, &VideoPage::sequence);
    body->addWidget(m_seqBox);

    // Watermark
    m_wmBox = new QGroupBox(scrollBody);
    auto *wmForm = makeRow(m_wmBox);
    auto *wmPathRow = new QHBoxLayout();
    m_wmPath = new ElaLineEdit(m_wmBox);
    m_wmPath->setReadOnly(true);
    m_wmBrowseButton = new ElaPushButton(m_wmBox);
    connect(m_wmBrowseButton, &ElaPushButton::clicked, this, [=]() {
        const QString path = browseFile(trId("video.watermark"),
                                        QStringLiteral("Image files (*.png *.jpg *.jpeg *.webp *.bmp);;All files (*)"));
        if (!path.isEmpty()) {
            m_wmPath->setText(path);
        }
    });
    wmPathRow->addWidget(m_wmPath, 1);
    wmPathRow->addWidget(m_wmBrowseButton);
    m_wmImageCaption = makeCaption(m_wmBox);
    m_wmPositionCaption = makeCaption(m_wmBox);
    m_wmPosition = new ElaComboBox(m_wmBox);
    m_wmPosition->addItem(QString(), QStringLiteral("bottom-right"));
    m_wmPosition->addItem(QString(), QStringLiteral("bottom-left"));
    m_wmPosition->addItem(QString(), QStringLiteral("top-right"));
    m_wmPosition->addItem(QString(), QStringLiteral("top-left"));
    m_wmPosition->addItem(QString(), QStringLiteral("center"));
    m_wmOpacityCaption = makeCaption(m_wmBox);
    m_wmOpacity = new ElaSpinBox(m_wmBox);
    m_wmOpacity->setRange(0, 100);
    m_wmOpacity->setValue(100);
    wmForm->addRow(m_wmImageCaption, wmPathRow);
    wmForm->addRow(m_wmPositionCaption, m_wmPosition);
    wmForm->addRow(m_wmOpacityCaption, m_wmOpacity);
    m_wmButton = makeQueueButton(m_wmBox);
    connect(m_wmButton, &ElaPushButton::clicked, this, &VideoPage::watermark);
    body->addWidget(m_wmBox);

    // Subtitle mux
    m_subBox = new QGroupBox(scrollBody);
    auto *subForm = makeRow(m_subBox);
    auto *subPathRow = new QHBoxLayout();
    m_subPath = new ElaLineEdit(m_subBox);
    m_subPath->setReadOnly(true);
    m_subPathButton = new ElaPushButton(m_subBox);
    connect(m_subPathButton, &ElaPushButton::clicked, this, [=]() {
        const QString path = browseFile(
            trId("video.subtitle"),
            QStringLiteral("Subtitle files (*.srt *.ass *.ssa *.vtt);;All files (*)"));
        if (!path.isEmpty()) {
            m_subtitleFile = path;
            m_subPath->setText(path);
        }
    });
    subPathRow->addWidget(m_subPath, 1);
    subPathRow->addWidget(m_subPathButton);
    m_subCaption = makeCaption(m_subBox);
    subForm->addRow(m_subCaption, subPathRow);
    m_subButton = makeQueueButton(m_subBox);
    connect(m_subButton, &ElaPushButton::clicked, this, &VideoPage::subtitleMux);
    body->addWidget(m_subBox);

    body->addStretch();
    scroll->setWidget(scrollBody);
    root->addWidget(scroll, 1);

    retranslateUi();
}

QString VideoPage::browseFile(const QString &title, const QString &filters)
{
    return QFileDialog::getOpenFileName(this, title, QString(), filters);
}

void VideoPage::enqueue(const QString &name, const QStringList &args)
{
    if (args.isEmpty()) return;
    auto *job = new Job();
    job->setName(name);
    job->setCommand(m_locator->ffmpegPath(), args);
    m_queue->addJob(job);
}

void VideoPage::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void VideoPage::dropEvent(QDropEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;
    m_inputFile = urls.first().toLocalFile();
    m_inputEdit->setText(m_inputFile);
    event->acceptProposedAction();
}

void VideoPage::trim()
{
    if (m_inputFile.isEmpty()) return;
    const QString ext = m_trimFormat->currentText().isEmpty()
        ? QFileInfo(m_inputFile).suffix()
        : m_trimFormat->currentText();
    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}_trim.{format}"), ext);
    enqueue(trId("video.job.trim").arg(QFileInfo(m_inputFile).fileName()),
            m_builder->buildTrim(m_inputFile, output,
                                 m_trimStart->value(), m_trimEnd->value(),
                                 m_trimFormat->currentText(),
                                 QString(), QString(), -1,
                                 m_trimCopy->isChecked()));
}

void VideoPage::extractAudio()
{
    if (m_inputFile.isEmpty()) return;
    const QString format = m_audioFormat->currentText();
    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}.{format}"), format);
    enqueue(trId("video.job.audio").arg(QFileInfo(m_inputFile).fileName()),
            m_builder->buildExtractAudio(m_inputFile, output, format, QString(),
                                         m_audioBitrate->value()));
}

void VideoPage::mute()
{
    if (m_inputFile.isEmpty()) return;
    const QString ext = m_muteFormat->currentText().isEmpty()
        ? QFileInfo(m_inputFile).suffix()
        : m_muteFormat->currentText();
    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}_muted.{format}"), ext);
    enqueue(trId("video.job.mute").arg(QFileInfo(m_inputFile).fileName()),
            m_builder->buildMute(m_inputFile, output, m_muteFormat->currentText(),
                                 QString(), -1, true));
}

void VideoPage::screenshot()
{
    if (m_inputFile.isEmpty()) return;
    const QString format = m_shotFormat->currentText();
    QString ext = format;
    if (format == QLatin1String("jpeg")) ext = QStringLiteral("jpg");
    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}_shot.{format}"), ext);
    enqueue(trId("video.job.screenshot").arg(QFileInfo(m_inputFile).fileName()),
            m_builder->buildScreenshot(m_inputFile, output, m_shotTime->value(), format));
}

void VideoPage::makeGif()
{
    if (m_inputFile.isEmpty()) return;
    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}.{format}"),
                                               QStringLiteral("gif"));
    enqueue(trId("video.job.gif").arg(QFileInfo(m_inputFile).fileName()),
            m_builder->buildGif(m_inputFile, output,
                                m_gifFps->value(), m_gifWidth->value(),
                                m_gifStart->value(), m_gifEnd->value()));
}

void VideoPage::joinVideos()
{
    if (m_joinFiles.size() < 2) return;
    const QString output = OutputNaming::build(m_joinFiles.first(),
                                               QStringLiteral("{name}_joined.{format}"),
                                               QStringLiteral("mp4"));
    enqueue(trId("video.job.join").arg(m_joinFiles.size()),
            m_builder->buildVideoJoin(m_joinFiles, output));
}

void VideoPage::sequence()
{
    const bool imagesToVideo = m_seqMode->currentIndex() == 0;
    const QString pattern = m_seqPattern->text().trimmed();
    if (pattern.isEmpty()) return;

    if (imagesToVideo) {
        const QString output = QFileInfo(pattern).absolutePath() + QStringLiteral("/sequence.mp4");
        enqueue(trId("video.job.seqToVideo"),
                m_builder->buildImageSequenceToVideo(pattern, output,
                                                     static_cast<int>(m_seqFps->value())));
    } else {
        if (m_inputFile.isEmpty()) return;
        const QString output = QFileInfo(m_inputFile).absolutePath() + QStringLiteral("/") + pattern;
        enqueue(trId("video.job.videoToSeq"),
                m_builder->buildVideoToImageSequence(m_inputFile, output, m_seqFps->value()));
    }
}

void VideoPage::watermark()
{
    const QString wmPath = m_wmPath->text();
    if (m_inputFile.isEmpty() || wmPath.isEmpty()) return;
    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}_wm.{format}"),
                                               QFileInfo(m_inputFile).suffix());
    enqueue(trId("video.job.watermark").arg(QFileInfo(m_inputFile).fileName()),
            m_builder->buildWatermark(m_inputFile, wmPath, output,
                                      m_wmPosition->currentData().toString(),
                                      m_wmOpacity->value()));
}

void VideoPage::subtitleMux()
{
    if (m_inputFile.isEmpty() || m_subtitleFile.isEmpty()) return;

    const QString suffix = QFileInfo(m_inputFile).suffix();
    const auto codec = RemuxPolicy::subtitleCodecForContainer(suffix);
    if (!codec) {
        QMessageBox::warning(this, trId("video.subtitle"),
                             trId("video.subUnsupported"));
        return;
    }

    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}_sub.{format}"),
                                               suffix);
    enqueue(trId("video.job.subtitle").arg(QFileInfo(m_subtitleFile).fileName()),
            m_builder->buildSubtitleMux(m_inputFile, m_subtitleFile, output,
                                        *codec));
}

void VideoPage::retranslateUi()
{
    m_titleLabel->setText(trId("video.title"));
    m_inputCaption->setText(trId("convert.input") + QStringLiteral(":"));
    m_browseButton->setText(trId("common.browse"));

    m_trimBox->setTitle(trId("video.trim"));
    m_trimStartCaption->setText(trId("common.start") + QStringLiteral(":"));
    m_trimEndCaption->setText(trId("common.end") + QStringLiteral(":"));
    m_trimFormatCaption->setText(trId("common.format") + QStringLiteral(":"));
    m_trimCopyCaption->setText(trId("video.trim.copy") + QStringLiteral(":"));
    m_trimButton->setText(trId("convert.addToQueue"));

    m_audioBox->setTitle(trId("video.audio"));
    m_audioFormatCaption->setText(trId("common.format") + QStringLiteral(":"));
    m_audioBitrateCaption->setText(trId("video.audio.bitrate") + QStringLiteral(":"));
    m_audioButton->setText(trId("convert.addToQueue"));

    m_muteBox->setTitle(trId("video.mute"));
    m_muteFormatCaption->setText(trId("common.format") + QStringLiteral(":"));
    m_muteButton->setText(trId("convert.addToQueue"));

    m_shotBox->setTitle(trId("video.screenshot"));
    m_shotTimeCaption->setText(trId("video.shot.time") + QStringLiteral(":"));
    m_shotFormatCaption->setText(trId("common.format") + QStringLiteral(":"));
    m_shotButton->setText(trId("convert.addToQueue"));

    m_gifBox->setTitle(trId("video.gif"));
    m_gifStartCaption->setText(trId("common.start") + QStringLiteral(":"));
    m_gifEndCaption->setText(trId("common.end") + QStringLiteral(":"));
    m_gifFpsCaption->setText(trId("common.fps") + QStringLiteral(":"));
    m_gifWidthCaption->setText(trId("common.width") + QStringLiteral(":"));
    m_gifButton->setText(trId("convert.addToQueue"));

    m_joinBox->setTitle(trId("video.join"));
    m_joinAddButton->setText(trId("common.open"));
    m_joinRemoveButton->setText(trId("common.delete"));
    m_joinButton->setText(trId("convert.addToQueue"));

    m_seqBox->setTitle(trId("video.sequence"));
    m_seqModeCaption->setText(trId("common.mode") + QStringLiteral(":"));
    m_seqMode->setItemText(0, trId("video.seq.imagesToVideo"));
    m_seqMode->setItemText(1, trId("video.seq.videoToImages"));
    m_seqPatternCaption->setText(trId("video.seq.pattern") + QStringLiteral(":"));
    m_seqFpsCaption->setText(trId("common.fps") + QStringLiteral(":"));
    m_seqButton->setText(trId("convert.addToQueue"));

    m_wmBox->setTitle(trId("video.watermark"));
    m_wmImageCaption->setText(trId("common.image") + QStringLiteral(":"));
    m_wmPositionCaption->setText(trId("common.position") + QStringLiteral(":"));
    m_wmPosition->setItemText(0, trId("video.wm.bottomRight"));
    m_wmPosition->setItemText(1, trId("video.wm.bottomLeft"));
    m_wmPosition->setItemText(2, trId("video.wm.topRight"));
    m_wmPosition->setItemText(3, trId("video.wm.topLeft"));
    m_wmPosition->setItemText(4, trId("video.wm.center"));
    m_wmOpacityCaption->setText(trId("common.opacity") + QStringLiteral(":"));
    m_wmButton->setText(trId("convert.addToQueue"));
    m_wmBrowseButton->setText(trId("common.browse"));

    m_subBox->setTitle(trId("video.subtitle"));
    m_subCaption->setText(trId("video.sub.file") + QStringLiteral(":"));
    m_subPathButton->setText(trId("common.browse"));
    m_subButton->setText(trId("convert.addToQueue"));
}
