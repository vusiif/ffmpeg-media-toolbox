#include "VideoEffectsTab.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QMessageBox>

#include "ElaCheckBox.h"
#include "ElaComboBox.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "app/dialogs/FilterStudioDialog.h"
#include "app/dialogs/PipCompositorDialog.h"
#include "app/dialogs/SubtitleStudioDialog.h"
#include "conversion/RemuxPolicy.h"
#include "ffmpeg/FFmpegCommandBuilder.h"
#include "i18n/TrId.h"
#include "output/OutputNaming.h"

VideoEffectsTab::VideoEffectsTab(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                                 JobQueue *queue, MediaProbeService *probeService,
                                 QWidget *parent)
    : VideoTabBase(locator, builder, queue, probeService, parent)
{
    initFilterStudioCard();
    initWatermarkCard();
    initPipCard();
    initSubtitleCard();
    m_bodyLayout->addStretch();
}

void VideoEffectsTab::initFilterStudioCard()
{
    m_filterStudioCard = createCard(m_filterStudioTitleLabel);
    auto *filterStudioLayout = qobject_cast<QVBoxLayout *>(m_filterStudioCard->layout());
    auto *filterStudioRow = new QHBoxLayout();
    auto *filterStudioDesc = new ElaText(trId("filter.dialog.subtitle"), 13, m_filterStudioCard);
    filterStudioDesc->setStyleSheet(QStringLiteral("color: #888888;"));
    m_filterStudioButton = new ElaPushButton(trId("filter.action.openStudio"), m_filterStudioCard);
    m_filterStudioButton->setFixedHeight(34);
    m_filterStudioButton->setStyleSheet(QStringLiteral("font-weight: bold; background-color: #0078d4; color: white;"));
    connect(m_filterStudioButton, &ElaPushButton::clicked, this, &VideoEffectsTab::openFilterStudioDialog);
    filterStudioRow->addWidget(filterStudioDesc, 1);
    filterStudioRow->addWidget(m_filterStudioButton);
    filterStudioLayout->addLayout(filterStudioRow);
    m_bodyLayout->addWidget(m_filterStudioCard);
}

void VideoEffectsTab::initWatermarkCard()
{
    m_wmCard = createCard(m_wmTitleLabel);
    auto *wmForm = new QFormLayout();
    wmForm->setSpacing(8);

    auto *wmPathRow = new QHBoxLayout();
    m_wmPath = new ElaLineEdit(m_wmCard);
    m_wmPath->setReadOnly(true);
    m_wmBrowseButton = new ElaPushButton(m_wmCard);
    connect(m_wmBrowseButton, &ElaPushButton::clicked, this, [this]() {
        const QString path = browseFile(trId("video.watermark"),
                                        QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp);;All files (*)"));
        if (!path.isEmpty()) m_wmPath->setText(path);
    });
    wmPathRow->addWidget(m_wmPath, 1);
    wmPathRow->addWidget(m_wmBrowseButton);

    m_wmImageCaption = makeCaption(m_wmCard);
    wmForm->addRow(m_wmImageCaption, wmPathRow);

    m_wmPositionCaption = makeCaption(m_wmCard);
    m_wmPosition = new ElaComboBox(m_wmCard);
    m_wmPosition->addItem(QString(), QStringLiteral("bottom_right"));
    m_wmPosition->addItem(QString(), QStringLiteral("bottom_left"));
    m_wmPosition->addItem(QString(), QStringLiteral("top_right"));
    m_wmPosition->addItem(QString(), QStringLiteral("top_left"));
    m_wmPosition->addItem(QString(), QStringLiteral("center"));
    wmForm->addRow(m_wmPositionCaption, m_wmPosition);

    m_wmOpacityCaption = makeCaption(m_wmCard);
    m_wmOpacity = new ElaSpinBox(m_wmCard);
    m_wmOpacity->setRange(1, 100);
    m_wmOpacity->setValue(100);
    wmForm->addRow(m_wmOpacityCaption, m_wmOpacity);

    qobject_cast<QVBoxLayout *>(m_wmCard->layout())->addLayout(wmForm);
    m_wmButton = createCardButton(m_wmCard);
    connect(m_wmButton, &ElaPushButton::clicked, this, &VideoEffectsTab::watermark);
    m_bodyLayout->addWidget(m_wmCard);
}

void VideoEffectsTab::initPipCard()
{
    m_pipCard = createCard(m_pipTitleLabel);
    auto *pipForm = new QFormLayout();
    pipForm->setSpacing(8);

    auto *pipPathRow = new QHBoxLayout();
    m_pipPath = new ElaLineEdit(m_pipCard);
    m_pipPath->setReadOnly(true);
    m_pipBrowseButton = new ElaPushButton(m_pipCard);
    connect(m_pipBrowseButton, &ElaPushButton::clicked, this, [this]() {
        const QString path = browseFile(trId("video.pip"),
                                        QStringLiteral("Video files (*.mp4 *.mkv *.mov *.webm *.avi *.flv *.wmv);;All files (*)"));
        if (!path.isEmpty()) {
            m_pipFile = path;
            m_pipPath->setText(path);
        }
    });
    pipPathRow->addWidget(m_pipPath, 1);
    pipPathRow->addWidget(m_pipBrowseButton);

    m_pipVideoCaption = makeCaption(m_pipCard);
    pipForm->addRow(m_pipVideoCaption, pipPathRow);

    m_pipSizeCaption = makeCaption(m_pipCard);
    auto *sizeRow = new QHBoxLayout();
    sizeRow->setSpacing(6);
    m_pipWidth = new ElaSpinBox(m_pipCard);
    m_pipWidth->setRange(0, 7680);
    m_pipWidth->setValue(320);
    m_pipHeight = new ElaSpinBox(m_pipCard);
    m_pipHeight->setRange(0, 4320);
    m_pipHeight->setValue(0);
    sizeRow->addWidget(m_pipWidth);
    sizeRow->addWidget(new ElaText(QStringLiteral("x"), 12, m_pipCard));
    sizeRow->addWidget(m_pipHeight);
    sizeRow->addStretch();
    pipForm->addRow(m_pipSizeCaption, sizeRow);

    m_pipPositionCaption = makeCaption(m_pipCard);
    m_pipPosition = new ElaComboBox(m_pipCard);
    m_pipPosition->addItem(QString(), QStringLiteral("bottom_right"));
    m_pipPosition->addItem(QString(), QStringLiteral("bottom_left"));
    m_pipPosition->addItem(QString(), QStringLiteral("top_right"));
    m_pipPosition->addItem(QString(), QStringLiteral("top_left"));
    m_pipPosition->addItem(QString(), QStringLiteral("center"));
    pipForm->addRow(m_pipPositionCaption, m_pipPosition);

    m_pipMarginCaption = makeCaption(m_pipCard);
    m_pipMargin = new ElaSpinBox(m_pipCard);
    m_pipMargin->setRange(0, 500);
    m_pipMargin->setValue(20);
    pipForm->addRow(m_pipMarginCaption, m_pipMargin);

    m_pipQualityCaption = makeCaption(m_pipCard);
    m_pipQualitySpin = new ElaSpinBox(m_pipCard);
    m_pipQualitySpin->setRange(0, 51);
    m_pipQualitySpin->setValue(23);
    pipForm->addRow(m_pipQualityCaption, m_pipQualitySpin);

    qobject_cast<QVBoxLayout *>(m_pipCard->layout())->addLayout(pipForm);

    m_openPipStudioBtn = new ElaPushButton(trId("video.pip.openStudio"), m_pipCard);
    m_openPipStudioBtn->setFixedHeight(36);
    connect(m_openPipStudioBtn, &ElaPushButton::clicked, this, &VideoEffectsTab::openPipCompositorStudio);

    m_pipButton = createCardButton(m_pipCard);
    connect(m_pipButton, &ElaPushButton::clicked, this, &VideoEffectsTab::pip);

    auto *pipBtnRow = new QHBoxLayout();
    pipBtnRow->addWidget(m_openPipStudioBtn);
    pipBtnRow->addWidget(m_pipButton);
    qobject_cast<QVBoxLayout *>(m_pipCard->layout())->addLayout(pipBtnRow);

    m_bodyLayout->addWidget(m_pipCard);
}

void VideoEffectsTab::initSubtitleCard()
{
    m_subCard = createCard(m_subTitleLabel);
    auto *subForm = new QFormLayout();
    subForm->setSpacing(8);

    auto *subPathRow = new QHBoxLayout();
    m_subPath = new ElaLineEdit(m_subCard);
    m_subPath->setReadOnly(true);
    m_subPathButton = new ElaPushButton(m_subCard);
    connect(m_subPathButton, &ElaPushButton::clicked, this, [this]() {
        const QString path = browseFile(trId("video.subtitle"),
                                        QStringLiteral("Subtitle files (*.srt *.ass *.ssa *.vtt);;All files (*)"));
        if (!path.isEmpty()) {
            m_subtitleFile = path;
            m_subPath->setText(path);
        }
    });
    subPathRow->addWidget(m_subPath, 1);
    subPathRow->addWidget(m_subPathButton);
    m_subCaption = makeCaption(m_subCard);
    subForm->addRow(m_subCaption, subPathRow);

    m_subBurnCheck = new ElaCheckBox(m_subCard);
    subForm->addRow(new ElaText(QString(), 13, m_subCard), m_subBurnCheck);

    m_subQualityCaption = makeCaption(m_subCard);
    m_subQualitySpin = new ElaSpinBox(m_subCard);
    m_subQualitySpin->setRange(0, 51);
    m_subQualitySpin->setValue(23);
    m_subQualityCaption->setVisible(false);
    m_subQualitySpin->setVisible(false);
    subForm->addRow(m_subQualityCaption, m_subQualitySpin);
    connect(m_subBurnCheck, &ElaCheckBox::toggled, this, [this](bool checked) {
        m_subQualityCaption->setVisible(checked);
        m_subQualitySpin->setVisible(checked);
    });

    qobject_cast<QVBoxLayout *>(m_subCard->layout())->addLayout(subForm);

    m_openSubStudioBtn = new ElaPushButton(trId("video.sub.openStudio"), m_subCard);
    m_openSubStudioBtn->setFixedHeight(36);
    connect(m_openSubStudioBtn, &ElaPushButton::clicked, this, &VideoEffectsTab::openSubtitleStudio);

    m_subButton = createCardButton(m_subCard);
    connect(m_subButton, &ElaPushButton::clicked, this, &VideoEffectsTab::subtitleMux);

    auto *subBtnRow = new QHBoxLayout();
    subBtnRow->addWidget(m_openSubStudioBtn);
    subBtnRow->addWidget(m_subButton);
    qobject_cast<QVBoxLayout *>(m_subCard->layout())->addLayout(subBtnRow);

    // Subtitle Extraction Section
    auto *subExtractSep = new QFrame(m_subCard);
    subExtractSep->setFrameShape(QFrame::HLine);
    subExtractSep->setFrameShadow(QFrame::Sunken);
    subExtractSep->setStyleSheet(QStringLiteral("color: rgba(255, 255, 255, 0.15); margin-top: 10px; margin-bottom: 6px;"));
    qobject_cast<QVBoxLayout *>(m_subCard->layout())->addWidget(subExtractSep);

    m_subExtractSectionCaption = new ElaText(trId("video.sub.extractSection"), 13, m_subCard);
    m_subExtractSectionCaption->setTextPixelSize(14);
    m_subExtractSectionCaption->setStyleSheet(QStringLiteral("font-weight: bold; color: #3b82f6;"));
    qobject_cast<QVBoxLayout *>(m_subCard->layout())->addWidget(m_subExtractSectionCaption);

    auto *subExtractForm = new QFormLayout();
    subExtractForm->setSpacing(8);
    m_subStreamCaption = makeCaption(m_subCard);
    m_subStreamCombo = new ElaComboBox(m_subCard);
    m_subStreamCombo->addItem(trId("video.sub.noTracks"));
    m_subStreamCombo->setEnabled(false);
    subExtractForm->addRow(m_subStreamCaption, m_subStreamCombo);
    qobject_cast<QVBoxLayout *>(m_subCard->layout())->addLayout(subExtractForm);

    auto *subExtractBtnRow = new QHBoxLayout();
    m_subExtractToStudioButton = new ElaPushButton(trId("video.sub.extractToStudio"), m_subCard);
    m_subExtractToStudioButton->setFixedHeight(36);
    m_subExtractToStudioButton->setEnabled(false);
    connect(m_subExtractToStudioButton, &ElaPushButton::clicked, this, &VideoEffectsTab::extractSubtitleToStudio);

    m_subExtractButton = new ElaPushButton(trId("video.sub.extractBtn"), m_subCard);
    m_subExtractButton->setFixedHeight(36);
    m_subExtractButton->setEnabled(false);
    connect(m_subExtractButton, &ElaPushButton::clicked, this, &VideoEffectsTab::extractSubtitle);

    subExtractBtnRow->addWidget(m_subExtractToStudioButton);
    subExtractBtnRow->addWidget(m_subExtractButton);
    qobject_cast<QVBoxLayout *>(m_subCard->layout())->addLayout(subExtractBtnRow);

    m_bodyLayout->addWidget(m_subCard);
}

void VideoEffectsTab::setMediaInfo(const MediaFile &info)
{
    VideoTabBase::setMediaInfo(info);
    updateSubtitleStreamsUI();
}

void VideoEffectsTab::updateSubtitleStreamsUI()
{
    if (!m_subStreamCombo) return;

    m_subStreamCombo->clear();
    if (m_mediaInfo.subtitleStreams.isEmpty()) {
        m_subStreamCombo->addItem(trId("video.sub.noTracks"));
        m_subStreamCombo->setEnabled(false);
        if (m_subExtractButton) m_subExtractButton->setEnabled(false);
        if (m_subExtractToStudioButton) m_subExtractToStudioButton->setEnabled(false);
    } else {
        for (int i = 0; i < m_mediaInfo.subtitleStreams.size(); ++i) {
            const auto &s = m_mediaInfo.subtitleStreams[i];
            QString label = QStringLiteral("Track %1: %2").arg(i).arg(s.codec.isEmpty() ? QStringLiteral("sub") : s.codec);
            if (!s.language.isEmpty()) {
                label += QStringLiteral(" [%1]").arg(s.language);
            }
            if (!s.title.isEmpty()) {
                label += QStringLiteral(" - %1").arg(s.title);
            }
            m_subStreamCombo->addItem(label, i);
        }
        m_subStreamCombo->setEnabled(true);
        if (m_subExtractButton) m_subExtractButton->setEnabled(true);
        if (m_subExtractToStudioButton) m_subExtractToStudioButton->setEnabled(true);
    }
}

void VideoEffectsTab::watermark()
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

void VideoEffectsTab::openFilterStudioDialog()
{
    if (m_filterDialog) {
        if (!m_inputFile.isEmpty()) m_filterDialog->loadFile(m_inputFile);
        m_filterDialog->raise();
        m_filterDialog->activateWindow();
        return;
    }
    m_filterDialog = new FilterStudioDialog(m_locator, m_probeService, m_queue, this);
    m_filterDialog->setAttribute(Qt::WA_DeleteOnClose);
    if (!m_inputFile.isEmpty()) {
        m_filterDialog->loadFile(m_inputFile);
    }
    m_filterDialog->show();
}

void VideoEffectsTab::openSubtitleStudio()
{
    if (m_subDialog) {
        if (!m_inputFile.isEmpty()) m_subDialog->setVideoFile(m_inputFile);
        m_subDialog->raise();
        m_subDialog->activateWindow();
        return;
    }
    m_subDialog = new SubtitleStudioDialog(m_queue, m_locator, this);
    m_subDialog->setAttribute(Qt::WA_DeleteOnClose);
    if (!m_inputFile.isEmpty()) {
        m_subDialog->setVideoFile(m_inputFile);
    }
    m_subDialog->show();
}

void VideoEffectsTab::openPipCompositorStudio()
{
    if (m_pipDialog) {
        if (!m_inputFile.isEmpty()) m_pipDialog->setMainVideo(m_inputFile);
        if (!m_pipFile.isEmpty()) m_pipDialog->setPipVideo(m_pipFile);
        m_pipDialog->raise();
        m_pipDialog->activateWindow();
        return;
    }
    m_pipDialog = new PipCompositorDialog(m_queue, m_locator, this);
    m_pipDialog->setAttribute(Qt::WA_DeleteOnClose);
    if (!m_inputFile.isEmpty()) {
        m_pipDialog->setMainVideo(m_inputFile);
    }
    if (!m_pipFile.isEmpty()) {
        m_pipDialog->setPipVideo(m_pipFile);
    }
    m_pipDialog->show();
}

void VideoEffectsTab::subtitleMux()
{
    if (m_inputFile.isEmpty() || m_subtitleFile.isEmpty()) return;

    const QString suffix = QFileInfo(m_inputFile).suffix();
    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}_sub.{format}"),
                                               suffix);

    if (m_subBurnCheck->isChecked()) {
        enqueue(trId("video.job.subtitleBurn").arg(QFileInfo(m_subtitleFile).fileName()),
                m_builder->buildSubtitleBurnin(m_inputFile, m_subtitleFile, output,
                                               m_subQualitySpin->value()));
        return;
    }

    const auto codec = RemuxPolicy::subtitleCodecForContainer(suffix);
    if (!codec) {
        QMessageBox::warning(this, trId("video.subtitle"),
                             trId("video.subUnsupported"));
        return;
    }

    enqueue(trId("video.job.subtitle").arg(QFileInfo(m_subtitleFile).fileName()),
            m_builder->buildSubtitleMux(m_inputFile, m_subtitleFile, output,
                                        *codec));
}

void VideoEffectsTab::extractSubtitle()
{
    if (m_inputFile.isEmpty() || m_mediaInfo.subtitleStreams.isEmpty()) {
        QMessageBox::information(this, trId("video.sub.extractSection"), trId("video.sub.noTracks"));
        return;
    }

    const int streamIdx = m_subStreamCombo->currentData().toInt();
    if (streamIdx < 0 || streamIdx >= m_mediaInfo.subtitleStreams.size()) return;

    const auto &stream = m_mediaInfo.subtitleStreams[streamIdx];
    QString defaultExt = QStringLiteral("srt");
    if (stream.codec.contains(QLatin1String("ass"), Qt::CaseInsensitive) ||
        stream.codec.contains(QLatin1String("ssa"), Qt::CaseInsensitive)) {
        defaultExt = QStringLiteral("ass");
    } else if (stream.codec.contains(QLatin1String("vtt"), Qt::CaseInsensitive)) {
        defaultExt = QStringLiteral("vtt");
    }

    const QString defaultName = OutputNaming::build(
        m_inputFile,
        QStringLiteral("{name}_track%1.{format}").arg(streamIdx),
        defaultExt);

    const QString savePath = QFileDialog::getSaveFileName(
        this,
        trId("video.sub.saveExtractedTitle"),
        defaultName,
        QStringLiteral("SubRip Subtitle (*.srt);;WebVTT Subtitle (*.vtt);;Advanced SubStation Alpha (*.ass);;All Files (*.*)"));
    if (savePath.isEmpty()) return;

    const QStringList args = m_builder->buildSubtitleExtract(m_inputFile, streamIdx, savePath);
    enqueue(trId("video.job.extractSub").arg(QFileInfo(savePath).fileName()), args);
}

void VideoEffectsTab::extractSubtitleToStudio()
{
    if (m_inputFile.isEmpty() || m_mediaInfo.subtitleStreams.isEmpty()) {
        QMessageBox::information(this, trId("video.sub.extractSection"), trId("video.sub.noTracks"));
        return;
    }

    const int streamIdx = m_subStreamCombo->currentData().toInt();
    if (streamIdx < 0 || streamIdx >= m_mediaInfo.subtitleStreams.size()) return;

    if (!m_subDialog) {
        m_subDialog = new SubtitleStudioDialog(m_queue, m_locator, this);
        m_subDialog->setAttribute(Qt::WA_DeleteOnClose);
    }
    m_subDialog->setVideoFile(m_inputFile);
    m_subDialog->show();
    m_subDialog->raise();
    m_subDialog->activateWindow();
    m_subDialog->extractAndLoadStream(streamIdx);
}

void VideoEffectsTab::pip()
{
    if (m_inputFile.isEmpty() || m_pipFile.isEmpty()) return;

    const QString output = OutputNaming::build(m_inputFile,
                                               QStringLiteral("{name}_pip.{format}"),
                                               QFileInfo(m_inputFile).suffix());
    enqueue(trId("video.job.pip").arg(QFileInfo(m_inputFile).fileName()),
            m_builder->buildPip(
                m_inputFile, m_pipFile, output,
                m_pipWidth->value(), m_pipHeight->value(),
                m_pipPosition->currentData().toString(),
                m_pipMargin->value(),
                m_pipQualitySpin->value()));
}

void VideoEffectsTab::retranslateUi()
{
    m_filterStudioTitleLabel->setText(trId("filter.dialog.title"));
    m_filterStudioButton->setText(trId("filter.action.openStudio"));

    m_wmTitleLabel->setText(trId("video.watermark"));
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

    m_subTitleLabel->setText(trId("video.subtitle"));
    m_subCaption->setText(trId("video.sub.file") + QStringLiteral(":"));
    m_subPathButton->setText(trId("common.browse"));
    m_subBurnCheck->setText(trId("video.sub.burn"));
    m_openSubStudioBtn->setText(trId("video.sub.openStudio"));
    m_subQualityCaption->setText(trId("convert.quality") + QStringLiteral(":"));
    m_subButton->setText(trId("convert.addToQueue"));
    m_subExtractSectionCaption->setText(trId("video.sub.extractSection"));
    m_subStreamCaption->setText(trId("video.sub.stream") + QStringLiteral(":"));
    m_subExtractToStudioButton->setText(trId("video.sub.extractToStudio"));
    m_subExtractButton->setText(trId("video.sub.extractBtn"));
    updateSubtitleStreamsUI();

    m_pipTitleLabel->setText(trId("video.pip"));
    m_pipVideoCaption->setText(trId("video.pip.video") + QStringLiteral(":"));
    m_pipBrowseButton->setText(trId("common.browse"));
    m_pipSizeCaption->setText(trId("common.size") + QStringLiteral(" (W x H):"));
    m_pipPositionCaption->setText(trId("common.position") + QStringLiteral(":"));
    m_pipPosition->setItemText(0, trId("video.wm.bottomRight"));
    m_pipPosition->setItemText(1, trId("video.wm.bottomLeft"));
    m_pipPosition->setItemText(2, trId("video.wm.topRight"));
    m_pipPosition->setItemText(3, trId("video.wm.topLeft"));
    m_pipPosition->setItemText(4, trId("video.wm.center"));
    m_pipMarginCaption->setText(trId("video.pip.margin") + QStringLiteral(":"));
    m_pipQualityCaption->setText(trId("convert.quality") + QStringLiteral(":"));
    m_openPipStudioBtn->setText(trId("video.pip.openStudio"));
    m_pipButton->setText(trId("convert.addToQueue"));
}
