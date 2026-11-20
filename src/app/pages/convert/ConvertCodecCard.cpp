#include "ConvertCodecCard.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QVBoxLayout>

#include "ElaCheckBox.h"
#include "ElaComboBox.h"
#include "ElaDoubleSpinBox.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "conversion/RemuxPolicy.h"
#include "ffmpeg/FFmpegCapabilities.h"
#include "i18n/TrId.h"
#include "presets/PresetManager.h"

ConvertCodecCard::ConvertCodecCard(FFmpegCapabilities *caps, PresetManager *presets, QWidget *parent)
    : ElaScrollPageArea(parent)
    , m_caps(caps)
    , m_presets(presets)
{
    setMinimumHeight(0);
    setMaximumHeight(QWIDGETSIZE_MAX);

    auto *codecLayout = new QVBoxLayout(this);
    codecLayout->setContentsMargins(18, 16, 18, 16);
    codecLayout->setSpacing(10);

    m_codecCardTitle = new ElaText(QString(), 15, this);
    m_codecCardTitle->setStyleSheet(QStringLiteral("font-weight: bold;"));
    codecLayout->addWidget(m_codecCardTitle);

    auto *presetRow = new QHBoxLayout();
    m_presetCaption = new ElaText(QString(), 13, this);
    m_presetCombo = new ElaComboBox(this);
    m_presetSaveButton = new ElaPushButton(this);
    m_presetDeleteButton = new ElaPushButton(this);
    connect(m_presetCombo, &ElaComboBox::currentTextChanged, this, &ConvertCodecCard::onPresetComboChanged);
    connect(m_presetSaveButton, &ElaPushButton::clicked, this, &ConvertCodecCard::onSavePresetClicked);
    connect(m_presetDeleteButton, &ElaPushButton::clicked, this, &ConvertCodecCard::onDeletePresetClicked);
    presetRow->addWidget(m_presetCaption);
    presetRow->addWidget(m_presetCombo, 1);
    presetRow->addWidget(m_presetSaveButton);
    presetRow->addWidget(m_presetDeleteButton);
    codecLayout->addLayout(presetRow);

    auto *form = new QFormLayout();
    form->setSpacing(10);

    m_formatCaption = new ElaText(QString(), 13, this);
    m_formatCombo = new ElaComboBox(this);
    m_formatCombo->addItems({QStringLiteral("mp4"), QStringLiteral("mkv"), QStringLiteral("mov"),
                             QStringLiteral("webm"), QStringLiteral("avi"), QStringLiteral("ts"),
                             QStringLiteral("mp3"), QStringLiteral("m4a"), QStringLiteral("aac"),
                             QStringLiteral("flac"), QStringLiteral("wav"), QStringLiteral("ogg")});
    connect(m_formatCombo, &ElaComboBox::currentTextChanged, this, &ConvertCodecCard::onFormatChanged);

    m_codecCaption = new ElaText(QString(), 13, this);
    m_codecCombo = new ElaComboBox(this);
    connect(m_codecCombo, &ElaComboBox::currentTextChanged, this, &ConvertCodecCard::optionsChanged);

    m_audioCodecCaption = new ElaText(QString(), 13, this);
    m_audioCodecCombo = new ElaComboBox(this);
    connect(m_audioCodecCombo, &ElaComboBox::currentTextChanged, this, &ConvertCodecCard::optionsChanged);

    m_rateControlCaption = new ElaText(QString(), 13, this);
    m_rateControlCombo = new ElaComboBox(this);
    m_rateControlCombo->addItem(trId("convert.rateControl.crf"));
    m_rateControlCombo->addItem(trId("convert.rateControl.bitrate"));
    m_rateControlCombo->addItem(trId("convert.rateControl.targetSize"));
    connect(m_rateControlCombo, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this, &ConvertCodecCard::onRateControlChanged);

    m_qualityCaption = new ElaText(QString(), 13, this);
    m_qualitySpin = new ElaSpinBox(this);
    m_qualitySpin->setRange(0, 51);
    m_qualitySpin->setValue(23);
    connect(m_qualitySpin, &ElaSpinBox::valueChanged, this, &ConvertCodecCard::optionsChanged);

    // Video Bitrate Control
    m_videoBitrateCaption = new ElaText(QString(), 13, this);
    m_videoBitrateWidget = new QWidget(this);
    auto *videoBitrateLayout = new QHBoxLayout(m_videoBitrateWidget);
    videoBitrateLayout->setContentsMargins(0, 0, 0, 0);
    videoBitrateLayout->setSpacing(6);
    m_videoBitrateSpin = new ElaSpinBox(m_videoBitrateWidget);
    m_videoBitrateSpin->setRange(100, 200000);
    m_videoBitrateSpin->setSingleStep(500);
    m_videoBitrateSpin->setValue(4000);
    m_videoBitrateSpin->setSuffix(QStringLiteral(" kbps"));

    m_preset1500kBtn = new ElaPushButton(QStringLiteral("1.5M"), m_videoBitrateWidget);
    m_preset3000kBtn = new ElaPushButton(QStringLiteral("3M"), m_videoBitrateWidget);
    m_preset6000kBtn = new ElaPushButton(QStringLiteral("6M"), m_videoBitrateWidget);
    m_preset10000kBtn = new ElaPushButton(QStringLiteral("10M"), m_videoBitrateWidget);
    m_preset1500kBtn->setMaximumWidth(50);
    m_preset3000kBtn->setMaximumWidth(50);
    m_preset6000kBtn->setMaximumWidth(50);
    m_preset10000kBtn->setMaximumWidth(50);
    connect(m_preset1500kBtn, &ElaPushButton::clicked, this, [this]() { m_videoBitrateSpin->setValue(1500); });
    connect(m_preset3000kBtn, &ElaPushButton::clicked, this, [this]() { m_videoBitrateSpin->setValue(3000); });
    connect(m_preset6000kBtn, &ElaPushButton::clicked, this, [this]() { m_videoBitrateSpin->setValue(6000); });
    connect(m_preset10000kBtn, &ElaPushButton::clicked, this, [this]() { m_videoBitrateSpin->setValue(10000); });

    videoBitrateLayout->addWidget(m_videoBitrateSpin, 1);
    videoBitrateLayout->addWidget(m_preset1500kBtn);
    videoBitrateLayout->addWidget(m_preset3000kBtn);
    videoBitrateLayout->addWidget(m_preset6000kBtn);
    videoBitrateLayout->addWidget(m_preset10000kBtn);

    m_bitrateTwoPassCheck = new ElaCheckBox(m_videoBitrateWidget);
    videoBitrateLayout->addWidget(m_bitrateTwoPassCheck);
    connect(m_bitrateTwoPassCheck, &ElaCheckBox::toggled, this, &ConvertCodecCard::optionsChanged);
    connect(m_videoBitrateSpin, QOverload<int>::of(&ElaSpinBox::valueChanged), this, &ConvertCodecCard::optionsChanged);

    m_videoBitrateCaption->setVisible(false);
    m_videoBitrateWidget->setVisible(false);

    // Target Size Control
    m_targetSizeCaption = new ElaText(QString(), 13, this);
    m_targetSizeWidget = new QWidget(this);
    auto *targetSizeLayout = new QHBoxLayout(m_targetSizeWidget);
    targetSizeLayout->setContentsMargins(0, 0, 0, 0);
    targetSizeLayout->setSpacing(6);
    m_targetSizeSpin = new ElaDoubleSpinBox(m_targetSizeWidget);
    m_targetSizeSpin->setRange(1.0, 10240.0);
    m_targetSizeSpin->setSingleStep(5.0);
    m_targetSizeSpin->setValue(25.0);
    m_targetSizeSpin->setDecimals(1);
    m_targetSizeSpin->setSuffix(QStringLiteral(" MB"));

    m_preset25Btn = new ElaPushButton(QStringLiteral("25MB"), m_targetSizeWidget);
    m_preset50Btn = new ElaPushButton(QStringLiteral("50MB"), m_targetSizeWidget);
    m_preset100Btn = new ElaPushButton(QStringLiteral("100MB"), m_targetSizeWidget);
    m_preset25Btn->setMaximumWidth(60);
    m_preset50Btn->setMaximumWidth(60);
    m_preset100Btn->setMaximumWidth(60);
    connect(m_preset25Btn, &ElaPushButton::clicked, this, [this]() { m_targetSizeSpin->setValue(25.0); });
    connect(m_preset50Btn, &ElaPushButton::clicked, this, [this]() { m_targetSizeSpin->setValue(50.0); });
    connect(m_preset100Btn, &ElaPushButton::clicked, this, [this]() { m_targetSizeSpin->setValue(100.0); });

    targetSizeLayout->addWidget(m_targetSizeSpin, 1);
    targetSizeLayout->addWidget(m_preset25Btn);
    targetSizeLayout->addWidget(m_preset50Btn);
    targetSizeLayout->addWidget(m_preset100Btn);

    m_twoPassCheck = new ElaCheckBox(m_targetSizeWidget);
    targetSizeLayout->addWidget(m_twoPassCheck);
    connect(m_twoPassCheck, &ElaCheckBox::toggled, this, &ConvertCodecCard::optionsChanged);
    connect(m_targetSizeSpin, &ElaDoubleSpinBox::valueChanged, this, &ConvertCodecCard::optionsChanged);

    m_targetSizeCaption->setVisible(false);
    m_targetSizeWidget->setVisible(false);

    m_smartCopyCheck = new ElaCheckBox(this);
    m_preserveAllStreamsCheck = new ElaCheckBox(this);
    m_preserveAllStreamsCheck->setChecked(true);
    m_stripMetaCheck = new ElaCheckBox(this);
    connect(m_smartCopyCheck, &ElaCheckBox::toggled, this, &ConvertCodecCard::optionsChanged);
    connect(m_preserveAllStreamsCheck, &ElaCheckBox::toggled, this, &ConvertCodecCard::optionsChanged);
    connect(m_stripMetaCheck, &ElaCheckBox::toggled, this, &ConvertCodecCard::optionsChanged);

    m_hwAccelCheck = new ElaCheckBox(this);
    m_hwDeviceLabel = new ElaText(QString(), 12, this);
    m_hwDeviceLabel->setStyleSheet(QStringLiteral("color: #0078d4; font-weight: bold;"));
    auto *hwRow = new QHBoxLayout();
    hwRow->addWidget(m_hwAccelCheck);
    hwRow->addWidget(m_hwDeviceLabel);
    hwRow->addStretch();
    connect(m_hwAccelCheck, &ElaCheckBox::toggled, this, &ConvertCodecCard::onHwAccelToggled);

    form->addRow(m_formatCaption, m_formatCombo);
    form->addRow(m_codecCaption, m_codecCombo);
    form->addRow(m_audioCodecCaption, m_audioCodecCombo);
    form->addRow(m_rateControlCaption, m_rateControlCombo);
    form->addRow(m_qualityCaption, m_qualitySpin);
    form->addRow(m_videoBitrateCaption, m_videoBitrateWidget);
    form->addRow(m_targetSizeCaption, m_targetSizeWidget);
    form->addRow(new ElaText(QString(), 13, this), m_smartCopyCheck);
    form->addRow(new ElaText(QString(), 13, this), m_preserveAllStreamsCheck);
    form->addRow(new ElaText(QString(), 13, this), m_stripMetaCheck);
    form->addRow(new ElaText(QString(), 13, this), hwRow);
    codecLayout->addLayout(form);

    rebuildCodecLists();
    refreshPresetList();
    retranslateUi();
}

void ConvertCodecCard::applyTo(FFmpegCommandBuilder::ConversionOptions &opts, bool copy, double durationSeconds) const
{
    opts.videoCodec = m_codecCombo->currentText();
    opts.audioCodec = m_audioCodecCombo->currentText();
    if (m_rateControlCombo && m_rateControlCombo->currentIndex() == 1) {
        opts.videoBitrateKbps = m_videoBitrateSpin->value();
        opts.crf = -1;
        opts.targetSizeMB = -1.0;
        opts.twoPass = m_bitrateTwoPassCheck->isChecked();
    } else if (m_rateControlCombo && m_rateControlCombo->currentIndex() == 2) {
        opts.targetSizeMB = m_targetSizeSpin->value();
        opts.durationSeconds = durationSeconds;
        opts.crf = -1;
        opts.containerOverheadMargin = 0.02;
        opts.twoPass = m_twoPassCheck->isChecked();
    } else {
        opts.crf = m_qualitySpin->value();
        opts.videoBitrateKbps = -1;
        opts.targetSizeMB = -1.0;
    }
    opts.copyStreams = copy;
    opts.preserveAllStreams = copy || m_preserveAllStreamsCheck->isChecked();
    opts.stripMetadata = m_stripMetaCheck->isChecked();
    const auto subCodec = RemuxPolicy::subtitleCodecForContainer(m_formatCombo->currentText());
    if (subCodec.has_value() && !subCodec->isEmpty()) {
        opts.subtitleCodec = *subCodec;
    }
    if (m_hwAccelCheck->isChecked() && m_caps) {
        opts.hwAccel = m_caps->recommendedHwAccelForEncoder(opts.videoCodec);
    }
}

void ConvertCodecCard::savePresetValues(QVariantMap &values) const
{
    values.insert(QStringLiteral("format"), m_formatCombo->currentText());
    values.insert(QStringLiteral("videoCodec"), m_codecCombo->currentText());
    values.insert(QStringLiteral("audioCodec"), m_audioCodecCombo->currentText());
    values.insert(QStringLiteral("rateControl"), m_rateControlCombo->currentIndex());
    values.insert(QStringLiteral("videoBitrate"), m_videoBitrateSpin->value());
    values.insert(QStringLiteral("bitrateTwoPass"), m_bitrateTwoPassCheck->isChecked());
    values.insert(QStringLiteral("targetSizeMB"), m_targetSizeSpin->value());
    values.insert(QStringLiteral("twoPass"), m_twoPassCheck->isChecked());
    values.insert(QStringLiteral("crf"), m_qualitySpin->value());
    values.insert(QStringLiteral("smartCopy"), m_smartCopyCheck->isChecked());
    values.insert(QStringLiteral("preserveAllStreams"), m_preserveAllStreamsCheck->isChecked());
    values.insert(QStringLiteral("stripMetadata"), m_stripMetaCheck->isChecked());
    values.insert(QStringLiteral("hwAccel"), m_hwAccelCheck->isChecked());
}

void ConvertCodecCard::loadPresetValues(const QVariantMap &values)
{
    m_formatCombo->setCurrentText(values.value(QStringLiteral("format"), QStringLiteral("mp4")).toString());
    m_codecCombo->setCurrentText(values.value(QStringLiteral("videoCodec"), QStringLiteral("libx264")).toString());
    m_audioCodecCombo->setCurrentText(values.value(QStringLiteral("audioCodec"), QStringLiteral("aac")).toString());
    m_rateControlCombo->setCurrentIndex(values.value(QStringLiteral("rateControl"), 0).toInt());
    m_videoBitrateSpin->setValue(values.value(QStringLiteral("videoBitrate"), 4000).toInt());
    m_bitrateTwoPassCheck->setChecked(values.value(QStringLiteral("bitrateTwoPass"), false).toBool());
    m_targetSizeSpin->setValue(values.value(QStringLiteral("targetSizeMB"), 25.0).toDouble());
    m_twoPassCheck->setChecked(values.value(QStringLiteral("twoPass"), false).toBool());
    m_qualitySpin->setValue(values.value(QStringLiteral("crf"), 23).toInt());
    m_smartCopyCheck->setChecked(values.value(QStringLiteral("smartCopy"), false).toBool());
    m_preserveAllStreamsCheck->setChecked(values.value(QStringLiteral("preserveAllStreams"), true).toBool());
    m_stripMetaCheck->setChecked(values.value(QStringLiteral("stripMetadata"), false).toBool());
    m_hwAccelCheck->setChecked(values.value(QStringLiteral("hwAccel"), true).toBool());
}

QString ConvertCodecCard::format() const
{
    return m_formatCombo->currentText();
}

QString ConvertCodecCard::videoCodec() const
{
    return m_codecCombo->currentText();
}

QString ConvertCodecCard::audioCodec() const
{
    return m_audioCodecCombo->currentText();
}

int ConvertCodecCard::crf() const
{
    return m_qualitySpin->value();
}

int ConvertCodecCard::rateControlIndex() const
{
    return m_rateControlCombo ? m_rateControlCombo->currentIndex() : 0;
}

double ConvertCodecCard::targetSizeMB() const
{
    return m_targetSizeSpin->value();
}

bool ConvertCodecCard::isSmartCopy() const
{
    return m_smartCopyCheck->isChecked();
}

void ConvertCodecCard::onFormatChanged()
{
    emit optionsChanged();
}

void ConvertCodecCard::onRateControlChanged(int idx)
{
    m_qualityCaption->setVisible(idx == 0);
    m_qualitySpin->setVisible(idx == 0);
    m_videoBitrateCaption->setVisible(idx == 1);
    m_videoBitrateWidget->setVisible(idx == 1);
    m_targetSizeCaption->setVisible(idx == 2);
    m_targetSizeWidget->setVisible(idx == 2);
    emit optionsChanged();
}

void ConvertCodecCard::onHwAccelToggled(bool checked)
{
    if (!m_caps) return;
    if (checked) {
        const QString hwEnc = m_caps->bestHwEncoder(m_codecCombo->currentText());
        if (!hwEnc.isEmpty()) {
            const int idx = m_codecCombo->findText(hwEnc);
            if (idx >= 0) m_codecCombo->setCurrentIndex(idx);
        }
    } else {
        const QString cur = m_codecCombo->currentText();
        if (cur.contains(QLatin1String("nvenc")) || cur.contains(QLatin1String("qsv")) ||
            cur.contains(QLatin1String("amf")) || cur.contains(QLatin1String("vaapi")) ||
            cur.contains(QLatin1String("videotoolbox"))) {
            QString swEnc = QStringLiteral("libx264");
            if (cur.contains(QLatin1String("hevc")) || cur.contains(QLatin1String("265"))) {
                swEnc = QStringLiteral("libx265");
            } else if (cur.contains(QLatin1String("av1"))) {
                swEnc = QStringLiteral("libsvtav1");
            }
            const int idx = m_codecCombo->findText(swEnc);
            if (idx >= 0) m_codecCombo->setCurrentIndex(idx);
        }
    }
    emit optionsChanged();
}

void ConvertCodecCard::onPresetComboChanged(const QString &name)
{
    if (!m_presets) return;
    const bool isBuiltIn = m_presets->isBuiltIn(name);
    m_presetDeleteButton->setEnabled(!isBuiltIn && !name.isEmpty());
    if (name.isEmpty()) return;
    emit presetLoaded(name);
}

void ConvertCodecCard::onSavePresetClicked()
{
    emit savePresetRequested();
}

void ConvertCodecCard::onDeletePresetClicked()
{
    if (!m_presets) return;
    m_presets->remove(m_presetCombo->currentText());
    refreshPresetList();
}

void ConvertCodecCard::refreshPresetList()
{
    if (!m_presets) return;
    m_presetCombo->blockSignals(true);
    m_presetCombo->clear();
    m_presetCombo->addItems(m_presets->names());
    m_presetCombo->blockSignals(false);
}

void ConvertCodecCard::rebuildCodecLists()
{
    if (!m_caps) return;
    const QString videoCodec = m_codecCombo->currentText();
    const QString audioCodec = m_audioCodecCombo->currentText();

    m_codecCombo->blockSignals(true);
    m_codecCombo->clear();
    m_codecCombo->addItems({QString(), QStringLiteral("copy")});
    m_codecCombo->addItems(m_caps->filterEncoders(FFmpegCapabilities::videoEncoderCandidates()));
    const int videoIndex = m_codecCombo->findText(videoCodec);
    if (videoIndex >= 0) {
        m_codecCombo->setCurrentIndex(videoIndex);
    } else {
        int defIdx = m_codecCombo->findText(QStringLiteral("libx264"));
        if (defIdx < 0 && m_codecCombo->count() > 2) defIdx = 2;
        if (defIdx >= 0) m_codecCombo->setCurrentIndex(defIdx);
    }
    m_codecCombo->blockSignals(false);

    m_audioCodecCombo->blockSignals(true);
    m_audioCodecCombo->clear();
    m_audioCodecCombo->addItems({QString(), QStringLiteral("copy")});
    m_audioCodecCombo->addItems(m_caps->filterEncoders(FFmpegCapabilities::audioEncoderCandidates()));
    const int audioIndex = m_audioCodecCombo->findText(audioCodec);
    if (audioIndex >= 0) {
        m_audioCodecCombo->setCurrentIndex(audioIndex);
    } else {
        int defIdx = m_audioCodecCombo->findText(QStringLiteral("aac"));
        if (defIdx < 0 && m_audioCodecCombo->count() > 2) defIdx = 2;
        if (defIdx >= 0) m_audioCodecCombo->setCurrentIndex(defIdx);
    }
    m_audioCodecCombo->blockSignals(false);

    if (m_caps->hasHwAcceleration()) {
        m_hwDeviceLabel->setText(QStringLiteral("(%1)").arg(m_caps->primaryHwDeviceName()));
        m_hwAccelCheck->setEnabled(true);
        m_hwAccelCheck->setChecked(true);
    } else {
        m_hwDeviceLabel->setText(QStringLiteral("(%1)").arg(trId("convert.hwAccel.none")));
        m_hwAccelCheck->setEnabled(false);
        m_hwAccelCheck->setChecked(false);
    }

    emit optionsChanged();
}

void ConvertCodecCard::retranslateUi()
{
    if (m_codecCardTitle) m_codecCardTitle->setText(trId("convert.card.encoding"));
    m_presetCaption->setText(trId("convert.preset") + QStringLiteral(":"));
    m_presetSaveButton->setText(trId("common.save"));
    m_presetDeleteButton->setText(trId("common.delete"));
    refreshPresetList();

    m_formatCaption->setText(trId("convert.outputFormat") + QStringLiteral(":"));
    m_codecCaption->setText(trId("convert.codec") + QStringLiteral(":"));
    m_audioCodecCaption->setText(trId("convert.audioCodec") + QStringLiteral(":"));
    m_rateControlCaption->setText(trId("convert.rateControl") + QStringLiteral(":"));
    m_rateControlCombo->setItemText(0, trId("convert.rateControl.crf"));
    m_rateControlCombo->setItemText(1, trId("convert.rateControl.bitrate"));
    m_rateControlCombo->setItemText(2, trId("convert.rateControl.targetSize"));
    m_qualityCaption->setText(trId("convert.quality") + QStringLiteral(":"));
    m_videoBitrateCaption->setText(trId("convert.videoBitrate") + QStringLiteral(":"));
    m_bitrateTwoPassCheck->setText(trId("convert.twoPass"));
    m_targetSizeCaption->setText(trId("convert.targetSize") + QStringLiteral(":"));
    m_twoPassCheck->setText(trId("convert.twoPass"));
    m_smartCopyCheck->setText(trId("common.smartCopy"));
    m_preserveAllStreamsCheck->setText(trId("convert.preserveAllStreams"));
    m_stripMetaCheck->setText(trId("metadata.remove"));
    m_hwAccelCheck->setText(trId("convert.hwAccel"));
    if (m_caps && m_caps->hasHwAcceleration()) {
        m_hwDeviceLabel->setText(QStringLiteral("(%1)").arg(m_caps->primaryHwDeviceName()));
    } else {
        m_hwDeviceLabel->setText(QStringLiteral("(%1)").arg(trId("convert.hwAccel.none")));
    }
}
