#include "VideoFramesTab.h"

#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>

#include "ElaComboBox.h"
#include "ElaDoubleSpinBox.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "app/dialogs/GifStudioDialog.h"
#include "ffmpeg/FFmpegCommandBuilder.h"
#include "i18n/TrId.h"
#include "output/OutputNaming.h"

VideoFramesTab::VideoFramesTab(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                             JobQueue *queue, MediaProbeService *probeService,
                             QWidget *parent)
    : VideoTabBase(locator, builder, queue, probeService, parent)
{
    initScreenshotCard();
    initGifCard();
    initSequenceCard();
    m_bodyLayout->addStretch();
}

void VideoFramesTab::initScreenshotCard()
{
    m_shotCard = createCard(m_shotTitleLabel);
    auto *shotForm = new QFormLayout();
    shotForm->setSpacing(8);
    m_shotTimeCaption = makeCaption(m_shotCard);
    m_shotTime = new ElaDoubleSpinBox(m_shotCard);
    m_shotTime->setRange(0, 86400);
    m_shotTime->setDecimals(1);
    m_shotFormatCaption = makeCaption(m_shotCard);
    m_shotFormat = new ElaComboBox(m_shotCard);
    m_shotFormat->addItems({QStringLiteral("png"), QStringLiteral("jpeg"), QStringLiteral("webp")});
    shotForm->addRow(m_shotTimeCaption, m_shotTime);
    shotForm->addRow(m_shotFormatCaption, m_shotFormat);
    qobject_cast<QVBoxLayout *>(m_shotCard->layout())->addLayout(shotForm);
    m_shotButton = createCardButton(m_shotCard);
    connect(m_shotButton, &ElaPushButton::clicked, this, &VideoFramesTab::screenshot);
    m_bodyLayout->addWidget(m_shotCard);
}

void VideoFramesTab::initGifCard()
{
    m_gifCard = createCard(m_gifTitleLabel);
    auto *gifForm = new QFormLayout();
    gifForm->setSpacing(8);
    m_gifStartCaption = makeCaption(m_gifCard);
    m_gifStart = new ElaDoubleSpinBox(m_gifCard);
    m_gifStart->setRange(0, 86400);
    m_gifStart->setDecimals(1);
    m_gifEndCaption = makeCaption(m_gifCard);
    m_gifEnd = new ElaDoubleSpinBox(m_gifCard);
    m_gifEnd->setRange(0, 86400);
    m_gifEnd->setDecimals(1);
    m_gifFpsCaption = makeCaption(m_gifCard);
    m_gifFps = new ElaSpinBox(m_gifCard);
    m_gifFps->setRange(1, 60);
    m_gifFps->setValue(12);
    m_gifWidthCaption = makeCaption(m_gifCard);
    m_gifWidth = new ElaSpinBox(m_gifCard);
    m_gifWidth->setRange(64, 3840);
    m_gifWidth->setValue(480);
    gifForm->addRow(m_gifStartCaption, m_gifStart);
    gifForm->addRow(m_gifEndCaption, m_gifEnd);
    gifForm->addRow(m_gifFpsCaption, m_gifFps);
    gifForm->addRow(m_gifWidthCaption, m_gifWidth);
    qobject_cast<QVBoxLayout *>(m_gifCard->layout())->addLayout(gifForm);

    m_openGifStudioBtn = new ElaPushButton(trId("video.gif.openStudio"), m_gifCard);
    m_openGifStudioBtn->setFixedHeight(36);
    connect(m_openGifStudioBtn, &ElaPushButton::clicked, this, &VideoFramesTab::openGifStudio);

    m_gifButton = createCardButton(m_gifCard);
    connect(m_gifButton, &ElaPushButton::clicked, this, &VideoFramesTab::makeGif);

    auto *gifBtnRow = new QHBoxLayout();
    gifBtnRow->addWidget(m_openGifStudioBtn);
    gifBtnRow->addWidget(m_gifButton);
    qobject_cast<QVBoxLayout *>(m_gifCard->layout())->addLayout(gifBtnRow);

    m_bodyLayout->addWidget(m_gifCard);
}

void VideoFramesTab::initSequenceCard()
{
    m_seqCard = createCard(m_seqTitleLabel);
    auto *seqForm = new QFormLayout();
    seqForm->setSpacing(8);
    m_seqModeCaption = makeCaption(m_seqCard);
    m_seqMode = new ElaComboBox(m_seqCard);
    m_seqMode->addItems({QString(), QString()});

    m_seqPatternCaption = makeCaption(m_seqCard);
    m_seqPattern = new ElaLineEdit(m_seqCard);
    m_seqPattern->setText(QStringLiteral("frame_%04d.png"));

    m_seqFpsCaption = makeCaption(m_seqCard);
    m_seqFps = new ElaDoubleSpinBox(m_seqCard);
    m_seqFps->setRange(0.1, 120);
    m_seqFps->setValue(24);

    seqForm->addRow(m_seqModeCaption, m_seqMode);
    seqForm->addRow(m_seqPatternCaption, m_seqPattern);
    seqForm->addRow(m_seqFpsCaption, m_seqFps);
    qobject_cast<QVBoxLayout *>(m_seqCard->layout())->addLayout(seqForm);
    m_seqButton = createCardButton(m_seqCard);
    connect(m_seqButton, &ElaPushButton::clicked, this, &VideoFramesTab::sequence);
    m_bodyLayout->addWidget(m_seqCard);
}

void VideoFramesTab::screenshot()
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

void VideoFramesTab::makeGif()
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

void VideoFramesTab::openGifStudio()
{
    if (m_gifDialog) {
        if (!m_inputFile.isEmpty()) m_gifDialog->setVideoFile(m_inputFile);
        m_gifDialog->raise();
        m_gifDialog->activateWindow();
        return;
    }
    m_gifDialog = new GifStudioDialog(m_queue, m_locator, this);
    m_gifDialog->setAttribute(Qt::WA_DeleteOnClose);
    if (!m_inputFile.isEmpty()) {
        m_gifDialog->setVideoFile(m_inputFile);
    }
    m_gifDialog->show();
}

void VideoFramesTab::sequence()
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

void VideoFramesTab::retranslateUi()
{
    m_shotTitleLabel->setText(trId("video.screenshot"));
    m_shotTimeCaption->setText(trId("video.shot.time") + QStringLiteral(":"));
    m_shotFormatCaption->setText(trId("common.format") + QStringLiteral(":"));
    m_shotButton->setText(trId("convert.addToQueue"));

    m_gifTitleLabel->setText(trId("video.gif"));
    m_gifStartCaption->setText(trId("common.start") + QStringLiteral(":"));
    m_gifEndCaption->setText(trId("common.end") + QStringLiteral(":"));
    m_gifFpsCaption->setText(trId("common.fps") + QStringLiteral(":"));
    m_gifWidthCaption->setText(trId("common.width") + QStringLiteral(":"));
    m_openGifStudioBtn->setText(trId("video.gif.openStudio"));
    m_gifButton->setText(trId("convert.addToQueue"));

    m_seqTitleLabel->setText(trId("video.sequence"));
    m_seqModeCaption->setText(trId("common.mode") + QStringLiteral(":"));
    m_seqMode->setItemText(0, trId("video.seq.imagesToVideo"));
    m_seqMode->setItemText(1, trId("video.seq.videoToImages"));
    m_seqPatternCaption->setText(trId("video.seq.pattern") + QStringLiteral(":"));
    m_seqFpsCaption->setText(trId("common.fps") + QStringLiteral(":"));
    m_seqButton->setText(trId("convert.addToQueue"));
}
