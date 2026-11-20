#include "BatchPipelineCard.h"

#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QVBoxLayout>

#include "ElaCheckBox.h"
#include "ElaComboBox.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "ffmpeg/FFmpegCommandBuilder.h"
#include "i18n/TrId.h"
#include "pipeline/PipelineManager.h"

namespace {
double parseTimestamp(const QString &t)
{
    const QString str = t.trimmed();
    if (str.isEmpty()) return 0.0;
    const auto parts = str.split(QLatin1Char(':'));
    if (parts.size() == 3) {
        return parts[0].toDouble() * 3600.0 + parts[1].toDouble() * 60.0 + parts[2].toDouble();
    } else if (parts.size() == 2) {
        return parts[0].toDouble() * 60.0 + parts[1].toDouble();
    }
    return str.toDouble();
}
} // namespace

BatchPipelineCard::BatchPipelineCard(QWidget *parent)
    : ElaScrollPageArea(parent)
    , m_pipelineManager(new PipelineManager(this))
{
    setMinimumHeight(0);
    setMaximumHeight(QWIDGETSIZE_MAX);

    auto *pipelineCardLayout = new QVBoxLayout(this);
    pipelineCardLayout->setContentsMargins(16, 16, 16, 16);
    pipelineCardLayout->setSpacing(10);

    m_pipelineToggle = new ElaCheckBox(this);
    connect(m_pipelineToggle, &ElaCheckBox::toggled, this, [this](bool checked) {
        m_pipelineContainer->setVisible(checked);
        emit pipelineChanged();
    });
    pipelineCardLayout->addWidget(m_pipelineToggle);

    m_pipelineContainer = new QWidget(this);
    auto *pipelineForm = new QFormLayout(m_pipelineContainer);
    pipelineForm->setContentsMargins(0, 8, 0, 0);
    pipelineForm->setSpacing(10);

    // Template selection row
    m_pipelineTemplateCaption = new ElaText(QString(), 13, m_pipelineContainer);
    auto *tmplRow = new QHBoxLayout();
    m_pipelineTemplateCombo = new ElaComboBox(m_pipelineContainer);
    m_pipelineSaveBtn = new ElaPushButton(m_pipelineContainer);
    m_pipelineDeleteBtn = new ElaPushButton(m_pipelineContainer);
    tmplRow->addWidget(m_pipelineTemplateCombo, 1);
    tmplRow->addWidget(m_pipelineSaveBtn);
    tmplRow->addWidget(m_pipelineDeleteBtn);
    pipelineForm->addRow(m_pipelineTemplateCaption, tmplRow);

    connect(m_pipelineTemplateCombo, &ElaComboBox::currentTextChanged, this, &BatchPipelineCard::onLoadTemplateClicked);
    connect(m_pipelineSaveBtn, &ElaPushButton::clicked, this, &BatchPipelineCard::onSaveTemplateClicked);
    connect(m_pipelineDeleteBtn, &ElaPushButton::clicked, this, &BatchPipelineCard::onDeleteTemplateClicked);
    connect(m_pipelineManager, &PipelineManager::templatesChanged, this, &BatchPipelineCard::refreshPipelineTemplates);

    // Step 1: Trim
    m_pipeTrimCheck = new ElaCheckBox(m_pipelineContainer);
    auto *trimRow = new QHBoxLayout();
    m_pipeStartEdit = new ElaLineEdit(m_pipelineContainer);
    m_pipeStartEdit->setPlaceholderText(QStringLiteral("00:00:00.000"));
    m_pipeEndEdit = new ElaLineEdit(m_pipelineContainer);
    m_pipeEndEdit->setPlaceholderText(QStringLiteral("00:01:00.000"));
    trimRow->addWidget(new ElaText(QStringLiteral("Start:"), 12, m_pipelineContainer));
    trimRow->addWidget(m_pipeStartEdit);
    trimRow->addWidget(new ElaText(QStringLiteral("End:"), 12, m_pipelineContainer));
    trimRow->addWidget(m_pipeEndEdit);
    pipelineForm->addRow(m_pipeTrimCheck, trimRow);

    // Step 2: Crop
    m_pipeCropCheck = new ElaCheckBox(m_pipelineContainer);
    auto *cropRow = new QHBoxLayout();
    m_pipeCropPresetCombo = new ElaComboBox(m_pipelineContainer);
    m_pipeCropPresetCombo->addItem(QStringLiteral("Custom"), QStringLiteral("custom"));
    m_pipeCropPresetCombo->addItem(QStringLiteral("16:9"), QStringLiteral("16:9"));
    m_pipeCropPresetCombo->addItem(QStringLiteral("4:3"), QStringLiteral("4:3"));
    m_pipeCropPresetCombo->addItem(QStringLiteral("1:1"), QStringLiteral("1:1"));
    m_pipeCropPresetCombo->addItem(QStringLiteral("9:16 (Shorts)"), QStringLiteral("9:16"));
    m_pipeCropPresetCombo->addItem(QStringLiteral("2.35:1 (Cinema)"), QStringLiteral("2.35:1"));
    m_pipeCropXSpin = new ElaSpinBox(m_pipelineContainer);
    m_pipeCropXSpin->setRange(0, 7680);
    m_pipeCropYSpin = new ElaSpinBox(m_pipelineContainer);
    m_pipeCropYSpin->setRange(0, 4320);
    m_pipeCropWSpin = new ElaSpinBox(m_pipelineContainer);
    m_pipeCropWSpin->setRange(0, 7680);
    m_pipeCropHSpin = new ElaSpinBox(m_pipelineContainer);
    m_pipeCropHSpin->setRange(0, 4320);
    cropRow->addWidget(m_pipeCropPresetCombo);
    cropRow->addWidget(new ElaText(QStringLiteral("X:"), 12, m_pipelineContainer));
    cropRow->addWidget(m_pipeCropXSpin);
    cropRow->addWidget(new ElaText(QStringLiteral("Y:"), 12, m_pipelineContainer));
    cropRow->addWidget(m_pipeCropYSpin);
    cropRow->addWidget(new ElaText(QStringLiteral("W:"), 12, m_pipelineContainer));
    cropRow->addWidget(m_pipeCropWSpin);
    cropRow->addWidget(new ElaText(QStringLiteral("H:"), 12, m_pipelineContainer));
    cropRow->addWidget(m_pipeCropHSpin);
    pipelineForm->addRow(m_pipeCropCheck, cropRow);

    // Step 3: Scale
    m_pipeScaleCheck = new ElaCheckBox(m_pipelineContainer);
    auto *scaleRow = new QHBoxLayout();
    m_pipeScaleCombo = new ElaComboBox(m_pipelineContainer);
    m_pipeScaleCombo->addItem(QStringLiteral("1080p (1920x-2)"), QStringLiteral("1080p"));
    m_pipeScaleCombo->addItem(QStringLiteral("720p (1280x-2)"), QStringLiteral("720p"));
    m_pipeScaleCombo->addItem(QStringLiteral("4K (3840x-2)"), QStringLiteral("4k"));
    m_pipeScaleCombo->addItem(QStringLiteral("480p (854x-2)"), QStringLiteral("480p"));
    m_pipeScaleCombo->addItem(QStringLiteral("Custom"), QStringLiteral("custom"));
    m_pipeScaleWSpin = new ElaSpinBox(m_pipelineContainer);
    m_pipeScaleWSpin->setRange(-2, 7680);
    m_pipeScaleWSpin->setValue(1920);
    m_pipeScaleHSpin = new ElaSpinBox(m_pipelineContainer);
    m_pipeScaleHSpin->setRange(-2, 4320);
    m_pipeScaleHSpin->setValue(-2);

    connect(m_pipeScaleCombo, &ElaComboBox::currentIndexChanged, this, [this](int idx) {
        if (idx == 0) {
            m_pipeScaleWSpin->setValue(1920);
            m_pipeScaleHSpin->setValue(-2);
        } else if (idx == 1) {
            m_pipeScaleWSpin->setValue(1280);
            m_pipeScaleHSpin->setValue(-2);
        } else if (idx == 2) {
            m_pipeScaleWSpin->setValue(3840);
            m_pipeScaleHSpin->setValue(-2);
        } else if (idx == 3) {
            m_pipeScaleWSpin->setValue(854);
            m_pipeScaleHSpin->setValue(-2);
        }
    });

    scaleRow->addWidget(m_pipeScaleCombo);
    scaleRow->addWidget(new ElaText(QStringLiteral("W:"), 12, m_pipelineContainer));
    scaleRow->addWidget(m_pipeScaleWSpin);
    scaleRow->addWidget(new ElaText(QStringLiteral("H:"), 12, m_pipelineContainer));
    scaleRow->addWidget(m_pipeScaleHSpin);
    pipelineForm->addRow(m_pipeScaleCheck, scaleRow);

    // Step 4: FPS
    m_pipeFpsCheck = new ElaCheckBox(m_pipelineContainer);
    m_pipeFpsCombo = new ElaComboBox(m_pipelineContainer);
    m_pipeFpsCombo->addItem(QStringLiteral("30 fps"), 30);
    m_pipeFpsCombo->addItem(QStringLiteral("60 fps"), 60);
    m_pipeFpsCombo->addItem(QStringLiteral("24 fps"), 24);
    m_pipeFpsCombo->addItem(QStringLiteral("25 fps"), 25);
    m_pipeFpsCombo->addItem(QStringLiteral("50 fps"), 50);
    pipelineForm->addRow(m_pipeFpsCheck, m_pipeFpsCombo);

    // Step 5: Watermark
    m_pipeWatermarkCheck = new ElaCheckBox(m_pipelineContainer);
    auto *wmRow = new QHBoxLayout();
    m_pipeWatermarkPathEdit = new ElaLineEdit(m_pipelineContainer);
    m_pipeWatermarkBrowseBtn = new ElaPushButton(m_pipelineContainer);
    connect(m_pipeWatermarkBrowseBtn, &ElaPushButton::clicked, this, &BatchPipelineCard::onBrowseWatermark);
    m_pipeWatermarkPosCombo = new ElaComboBox(m_pipelineContainer);
    m_pipeWatermarkPosCombo->addItem(QStringLiteral("Top-Right"), QStringLiteral("top-right"));
    m_pipeWatermarkPosCombo->addItem(QStringLiteral("Top-Left"), QStringLiteral("top-left"));
    m_pipeWatermarkPosCombo->addItem(QStringLiteral("Bottom-Right"), QStringLiteral("bottom-right"));
    m_pipeWatermarkPosCombo->addItem(QStringLiteral("Bottom-Left"), QStringLiteral("bottom-left"));
    m_pipeWatermarkPosCombo->addItem(QStringLiteral("Center"), QStringLiteral("center"));
    m_pipeWatermarkOpacitySpin = new ElaSpinBox(m_pipelineContainer);
    m_pipeWatermarkOpacitySpin->setRange(10, 100);
    m_pipeWatermarkOpacitySpin->setValue(80);
    m_pipeWatermarkOpacitySpin->setSuffix(QStringLiteral("%"));
    wmRow->addWidget(m_pipeWatermarkPathEdit, 1);
    wmRow->addWidget(m_pipeWatermarkBrowseBtn);
    wmRow->addWidget(m_pipeWatermarkPosCombo);
    wmRow->addWidget(new ElaText(QStringLiteral("Opacity:"), 12, m_pipelineContainer));
    wmRow->addWidget(m_pipeWatermarkOpacitySpin);
    pipelineForm->addRow(m_pipeWatermarkCheck, wmRow);

    // Step 6: Audio
    m_pipeAudioCheck = new ElaCheckBox(m_pipelineContainer);
    auto *audioRow = new QHBoxLayout();
    m_pipeAudioVolSpin = new ElaSpinBox(m_pipelineContainer);
    m_pipeAudioVolSpin->setRange(10, 300);
    m_pipeAudioVolSpin->setValue(100);
    m_pipeAudioVolSpin->setSuffix(QStringLiteral("%"));
    m_pipeAudioLoudnormCheck = new ElaCheckBox(m_pipelineContainer);
    m_pipeAudioMuteCheck = new ElaCheckBox(m_pipelineContainer);
    audioRow->addWidget(new ElaText(QStringLiteral("Vol:"), 12, m_pipelineContainer));
    audioRow->addWidget(m_pipeAudioVolSpin);
    audioRow->addWidget(m_pipeAudioLoudnormCheck);
    audioRow->addWidget(m_pipeAudioMuteCheck);
    audioRow->addStretch();
    pipelineForm->addRow(m_pipeAudioCheck, audioRow);

    m_pipelineContainer->setVisible(false);
    pipelineCardLayout->addWidget(m_pipelineContainer);

    refreshPipelineTemplates();
    retranslateUi();
}

bool BatchPipelineCard::isPipelineEnabled() const
{
    return m_pipelineToggle->isChecked();
}

PipelineStep BatchPipelineCard::currentPipelineStep(const QString &format,
                                                    const QString &vcodec,
                                                    const QString &acodec,
                                                    int crf,
                                                    const QString &audioBitrate,
                                                    const QString &speedPreset,
                                                    bool hwAccel) const
{
    PipelineStep step;

    // 1. Trim
    step.enableTrim = m_pipeTrimCheck->isChecked();
    step.startSec = parseTimestamp(m_pipeStartEdit->text());
    step.endSec = parseTimestamp(m_pipeEndEdit->text());

    // 2. Crop
    step.enableCrop = m_pipeCropCheck->isChecked();
    step.cropX = m_pipeCropXSpin->value();
    step.cropY = m_pipeCropYSpin->value();
    step.cropW = m_pipeCropWSpin->value();
    step.cropH = m_pipeCropHSpin->value();

    // 3. Scale
    step.enableScale = m_pipeScaleCheck->isChecked();
    step.scaleWidth = m_pipeScaleWSpin->value();
    step.scaleHeight = m_pipeScaleHSpin->value();

    // 4. FPS
    step.enableFps = m_pipeFpsCheck->isChecked();
    step.fps = m_pipeFpsCombo->currentData().toInt();

    // 5. Watermark
    step.enableWatermark = m_pipeWatermarkCheck->isChecked();
    step.watermarkPath = m_pipeWatermarkPathEdit->text().trimmed();
    step.watermarkPosition = m_pipeWatermarkPosCombo->currentData().toString();
    step.watermarkOpacity = m_pipeWatermarkOpacitySpin->value();

    // 6. Audio
    step.enableAudio = m_pipeAudioCheck->isChecked();
    step.volumeFactor = m_pipeAudioVolSpin->value() / 100.0;
    step.loudnorm = m_pipeAudioLoudnormCheck->isChecked();
    step.mute = m_pipeAudioMuteCheck->isChecked();

    // 7. Base encoding settings
    step.outputFormat = format;
    step.videoCodec = vcodec;
    step.audioCodec = acodec;
    step.crf = crf;
    step.audioBitrate = audioBitrate;
    step.speedPreset = speedPreset;
    step.hwAccel = hwAccel;
    step.preserveAllStreams = true;

    return step;
}

void BatchPipelineCard::applyPipelineStepToUi(const PipelineStep &step)
{
    m_pipeTrimCheck->setChecked(step.enableTrim);
    m_pipeStartEdit->setText(step.startSec > 0.0 ? FFmpegCommandBuilder::formatTime(step.startSec) : QString());
    m_pipeEndEdit->setText(step.endSec > 0.0 ? FFmpegCommandBuilder::formatTime(step.endSec) : QString());

    m_pipeCropCheck->setChecked(step.enableCrop);
    m_pipeCropXSpin->setValue(step.cropX);
    m_pipeCropYSpin->setValue(step.cropY);
    m_pipeCropWSpin->setValue(step.cropW);
    m_pipeCropHSpin->setValue(step.cropH);

    m_pipeScaleCheck->setChecked(step.enableScale);
    m_pipeScaleWSpin->setValue(step.scaleWidth);
    m_pipeScaleHSpin->setValue(step.scaleHeight);

    m_pipeFpsCheck->setChecked(step.enableFps);
    const int fpsIdx = m_pipeFpsCombo->findData(step.fps);
    if (fpsIdx >= 0) m_pipeFpsCombo->setCurrentIndex(fpsIdx);

    m_pipeWatermarkCheck->setChecked(step.enableWatermark);
    m_pipeWatermarkPathEdit->setText(step.watermarkPath);
    const int posIdx = m_pipeWatermarkPosCombo->findData(step.watermarkPosition);
    if (posIdx >= 0) m_pipeWatermarkPosCombo->setCurrentIndex(posIdx);
    m_pipeWatermarkOpacitySpin->setValue(step.watermarkOpacity);

    m_pipeAudioCheck->setChecked(step.enableAudio);
    m_pipeAudioVolSpin->setValue(qRound(step.volumeFactor * 100.0));
    m_pipeAudioLoudnormCheck->setChecked(step.loudnorm);
    m_pipeAudioMuteCheck->setChecked(step.mute);

    emit baseSettingsChanged(step.outputFormat, step.videoCodec, step.audioCodec, step.crf);
}

void BatchPipelineCard::refreshPipelineTemplates()
{
    const QString current = m_pipelineTemplateCombo->currentText();
    m_pipelineTemplateCombo->blockSignals(true);
    m_pipelineTemplateCombo->clear();
    m_pipelineTemplateCombo->addItem(QString());
    m_pipelineTemplateCombo->addItems(m_pipelineManager->names());
    m_pipelineTemplateCombo->blockSignals(false);
    if (!current.isEmpty()) {
        m_pipelineTemplateCombo->setCurrentText(current);
    }
    const bool isBuiltIn = m_pipelineManager->isBuiltIn(m_pipelineTemplateCombo->currentText());
    m_pipelineDeleteBtn->setEnabled(!isBuiltIn && !m_pipelineTemplateCombo->currentText().isEmpty());
}

void BatchPipelineCard::onLoadTemplateClicked(const QString &name)
{
    const bool isBuiltIn = m_pipelineManager->isBuiltIn(name);
    m_pipelineDeleteBtn->setEnabled(!isBuiltIn && !name.isEmpty());

    if (name.isEmpty()) return;
    const BatchPipeline pipeline = m_pipelineManager->load(name);
    applyPipelineStepToUi(pipeline.settings());
}

void BatchPipelineCard::onSaveTemplateClicked()
{
    const QString name = QInputDialog::getText(this, trId("batch.pipeline.saveTemplate"),
                                               trId("batch.pipeline.templatePrompt") + QStringLiteral(":"),
                                               QLineEdit::Normal);
    if (name.trimmed().isEmpty()) return;

    BatchPipeline pipeline(name.trimmed(), currentPipelineStep());
    m_pipelineManager->save(name.trimmed(), pipeline);
    refreshPipelineTemplates();
    m_pipelineTemplateCombo->setCurrentText(name.trimmed());
}

void BatchPipelineCard::onDeleteTemplateClicked()
{
    const QString name = m_pipelineTemplateCombo->currentText();
    if (name.isEmpty()) return;
    m_pipelineManager->remove(name);
    refreshPipelineTemplates();
}

void BatchPipelineCard::onBrowseWatermark()
{
    const QString file = QFileDialog::getOpenFileName(this, trId("batch.pipe.selectWatermark"), QString(),
                                                      QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp *.webp)"));
    if (!file.isEmpty()) m_pipeWatermarkPathEdit->setText(file);
}

void BatchPipelineCard::retranslateUi()
{
    m_pipelineToggle->setText(trId("batch.pipeline.title"));
    m_pipelineTemplateCaption->setText(trId("batch.pipeline.template") + QStringLiteral(":"));
    m_pipelineSaveBtn->setText(trId("common.save"));
    m_pipelineDeleteBtn->setText(trId("common.delete"));
    m_pipeTrimCheck->setText(trId("batch.pipe.trim"));
    m_pipeCropCheck->setText(trId("batch.pipe.crop"));
    m_pipeScaleCheck->setText(trId("batch.pipe.scale"));
    m_pipeFpsCheck->setText(trId("batch.pipe.fps"));
    m_pipeWatermarkCheck->setText(trId("batch.pipe.watermark"));
    m_pipeWatermarkBrowseBtn->setText(trId("common.browse"));
    m_pipeAudioCheck->setText(trId("batch.pipe.audio"));
    m_pipeAudioLoudnormCheck->setText(trId("batch.pipe.loudnorm"));
    m_pipeAudioMuteCheck->setText(trId("batch.pipe.mute"));
}
