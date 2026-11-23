#include "BatchOptionsCard.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QVariantMap>

#include "ElaCheckBox.h"
#include "ElaComboBox.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "ffmpeg/FFmpegCapabilities.h"
#include "conversion/RemuxPolicy.h"
#include "i18n/TrId.h"
#include "presets/PresetManager.h"

BatchOptionsCard::BatchOptionsCard(FFmpegCapabilities *caps, PresetManager *presets, QWidget *parent)
    : ElaScrollPageArea(parent)
    , m_caps(caps)
    , m_presets(presets)
{
    setMinimumHeight(0);
    setMaximumHeight(QWIDGETSIZE_MAX);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    // Form layout for base presets, format, and codecs
    auto *formLayout = new QFormLayout();
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(10);

    m_presetCaption = new ElaText(QString(), 13, this);
    m_presetCombo = new ElaComboBox(this);
    if (m_presets) {
        connect(m_presets, &PresetManager::presetsChanged, this, &BatchOptionsCard::refreshPresetList);
    }
    connect(m_presetCombo, &ElaComboBox::currentTextChanged, this, &BatchOptionsCard::loadPreset);
    formLayout->addRow(m_presetCaption, m_presetCombo);

    m_formatCaption = new ElaText(QString(), 13, this);
    m_formatCombo = new ElaComboBox(this);
    m_formatCombo->addItems({QStringLiteral("mp4"), QStringLiteral("mkv"), QStringLiteral("mov"),
                             QStringLiteral("webm"), QStringLiteral("avi"), QStringLiteral("ts"),
                             QStringLiteral("mp3"), QStringLiteral("m4a"), QStringLiteral("aac"),
                             QStringLiteral("flac"), QStringLiteral("wav"), QStringLiteral("ogg")});
    connect(m_formatCombo, &ElaComboBox::currentTextChanged, this, &BatchOptionsCard::onFormatChanged);
    formLayout->addRow(m_formatCaption, m_formatCombo);

    m_codecCaption = new ElaText(QString(), 13, this);
    m_codecCombo = new ElaComboBox(this);
    connect(m_codecCombo, &ElaComboBox::currentTextChanged, this, &BatchOptionsCard::optionsChanged);
    formLayout->addRow(m_codecCaption, m_codecCombo);

    m_audioCodecCaption = new ElaText(QString(), 13, this);
    m_audioCodecCombo = new ElaComboBox(this);
    connect(m_audioCodecCombo, &ElaComboBox::currentTextChanged, this, &BatchOptionsCard::optionsChanged);
    formLayout->addRow(m_audioCodecCaption, m_audioCodecCombo);

    m_qualityCaption = new ElaText(QString(), 13, this);
    m_qualitySpin = new ElaSpinBox(this);
    m_qualitySpin->setRange(0, 51);
    m_qualitySpin->setValue(23);
    m_qualitySpin->setFixedWidth(120);
    connect(m_qualitySpin, &ElaSpinBox::valueChanged, this, &BatchOptionsCard::optionsChanged);
    formLayout->addRow(m_qualityCaption, m_qualitySpin);

    m_quickTargetCaption = new ElaText(QString(), 13, this);
    m_qualityWidget = new QWidget(this);
    auto *qualityLayout = new QHBoxLayout(m_qualityWidget);
    qualityLayout->setContentsMargins(0, 0, 0, 0);
    qualityLayout->setSpacing(6);

    m_quickSmallBtn = new ElaPushButton(m_qualityWidget);
    m_quickBalancedBtn = new ElaPushButton(m_qualityWidget);
    m_quickHighQualityBtn = new ElaPushButton(m_qualityWidget);
    m_quickCopyBtn = new ElaPushButton(m_qualityWidget);

    for (auto *btn : {m_quickSmallBtn, m_quickBalancedBtn, m_quickHighQualityBtn, m_quickCopyBtn}) {
        btn->setFixedHeight(28);
        btn->setBorderRadius(5);
        qualityLayout->addWidget(btn);
    }
    qualityLayout->addStretch();

    connect(m_quickSmallBtn, &ElaPushButton::clicked, this, [this]() {
        m_qualitySpin->setValue(28);
        m_smartCopyCheck->setChecked(false);
        emit optionsChanged();
    });
    connect(m_quickBalancedBtn, &ElaPushButton::clicked, this, [this]() {
        m_qualitySpin->setValue(23);
        m_smartCopyCheck->setChecked(false);
        emit optionsChanged();
    });
    connect(m_quickHighQualityBtn, &ElaPushButton::clicked, this, [this]() {
        m_qualitySpin->setValue(18);
        m_smartCopyCheck->setChecked(false);
        emit optionsChanged();
    });
    connect(m_quickCopyBtn, &ElaPushButton::clicked, this, [this]() {
        m_smartCopyCheck->setChecked(true);
        emit optionsChanged();
    });

    formLayout->addRow(m_quickTargetCaption, m_qualityWidget);

    m_smartCopyCheck = new ElaCheckBox(this);
    connect(m_smartCopyCheck, &ElaCheckBox::toggled, this, &BatchOptionsCard::optionsChanged);
    formLayout->addRow(new ElaText(QString(), 13, this), m_smartCopyCheck);

    m_stripMetaCheck = new ElaCheckBox(this);
    connect(m_stripMetaCheck, &ElaCheckBox::toggled, this, &BatchOptionsCard::optionsChanged);
    formLayout->addRow(new ElaText(QString(), 13, this), m_stripMetaCheck);

    m_hwAccelCheck = new ElaCheckBox(this);
    m_hwDeviceLabel = new ElaText(QString(), 12, this);
    m_hwDeviceLabel->setStyleSheet(QStringLiteral("color: #0078d4; font-weight: bold;"));
    auto *hwRow = new QHBoxLayout();
    hwRow->addWidget(m_hwAccelCheck);
    hwRow->addWidget(m_hwDeviceLabel);
    hwRow->addStretch();
    connect(m_hwAccelCheck, &ElaCheckBox::toggled, this, &BatchOptionsCard::onHwAccelToggled);
    formLayout->addRow(new ElaText(QString(), 13, this), hwRow);

    rootLayout->addLayout(formLayout);

    // Collapsible Advanced section
    m_advToggle = new ElaCheckBox(this);
    rootLayout->addWidget(m_advToggle);

    m_advContainer = new QWidget(this);
    auto *advForm = new QFormLayout(m_advContainer);
    advForm->setContentsMargins(0, 8, 0, 0);
    advForm->setSpacing(8);

    m_audioBitrateCaption = new ElaText(QString(), 13, m_advContainer);
    m_audioBitrateCombo = new ElaComboBox(m_advContainer);
    m_audioBitrateCombo->addItem(QString(), QString());
    for (const QString &b : {QStringLiteral("64"), QStringLiteral("96"), QStringLiteral("128"),
                             QStringLiteral("192"), QStringLiteral("256"), QStringLiteral("320")}) {
        m_audioBitrateCombo->addItem(b + QStringLiteral(" kbps"), b);
    }
    advForm->addRow(m_audioBitrateCaption, m_audioBitrateCombo);

    m_audioSamplerateCaption = new ElaText(QString(), 13, m_advContainer);
    m_audioSamplerateCombo = new ElaComboBox(m_advContainer);
    m_audioSamplerateCombo->addItem(QString(), QString());
    for (const QString &r : {QStringLiteral("44100"), QStringLiteral("48000"), QStringLiteral("96000")}) {
        m_audioSamplerateCombo->addItem(r + QStringLiteral(" Hz"), r);
    }
    advForm->addRow(m_audioSamplerateCaption, m_audioSamplerateCombo);

    m_audioChannelsCaption = new ElaText(QString(), 13, m_advContainer);
    m_audioChannelsCombo = new ElaComboBox(m_advContainer);
    m_audioChannelsCombo->addItem(QString(), QString());
    m_audioChannelsCombo->addItem(QStringLiteral("Mono (1 ch)"), QStringLiteral("1"));
    m_audioChannelsCombo->addItem(QStringLiteral("Stereo (2 ch)"), QStringLiteral("2"));
    m_audioChannelsCombo->addItem(QStringLiteral("5.1 Surround (6 ch)"), QStringLiteral("6"));
    advForm->addRow(m_audioChannelsCaption, m_audioChannelsCombo);

    m_audioNormalizeCheck = new ElaCheckBox(m_advContainer);
    advForm->addRow(new ElaText(QString(), 13, m_advContainer), m_audioNormalizeCheck);

    m_speedPresetCaption = new ElaText(QString(), 13, m_advContainer);
    m_speedPresetCombo = new ElaComboBox(m_advContainer);
    m_speedPresetCombo->addItem(QString(), QString());
    for (const QString &p : {QStringLiteral("ultrafast"), QStringLiteral("superfast"),
                             QStringLiteral("veryfast"), QStringLiteral("faster"),
                             QStringLiteral("fast"), QStringLiteral("medium"),
                             QStringLiteral("slow"), QStringLiteral("slower"),
                             QStringLiteral("veryslow")}) {
        m_speedPresetCombo->addItem(p, p);
    }
    advForm->addRow(m_speedPresetCaption, m_speedPresetCombo);

    m_resolutionCaption = new ElaText(QString(), 13, m_advContainer);
    m_resolutionCombo = new ElaComboBox(m_advContainer);
    m_resolutionCombo->addItem(QString(), QString());
    m_resolutionCombo->addItem(QStringLiteral("3840x2160 (4K UHD)"), QStringLiteral("3840:-2"));
    m_resolutionCombo->addItem(QStringLiteral("2560x1440 (2K QHD)"), QStringLiteral("2560:-2"));
    m_resolutionCombo->addItem(QStringLiteral("1920x1080 (1080p FHD)"), QStringLiteral("1920:-2"));
    m_resolutionCombo->addItem(QStringLiteral("1280x720 (720p HD)"), QStringLiteral("1280:-2"));
    m_resolutionCombo->addItem(QStringLiteral("854x480 (480p SD)"), QStringLiteral("854:-2"));
    m_resolutionCombo->addItem(QStringLiteral("640x360 (360p)"), QStringLiteral("640:-2"));
    advForm->addRow(m_resolutionCaption, m_resolutionCombo);

    m_fpsCaption = new ElaText(QString(), 13, m_advContainer);
    m_fpsCombo = new ElaComboBox(m_advContainer);
    m_fpsCombo->addItem(QString(), QString());
    m_fpsCombo->addItem(QStringLiteral("60 fps"), QStringLiteral("60"));
    m_fpsCombo->addItem(QStringLiteral("50 fps"), QStringLiteral("50"));
    m_fpsCombo->addItem(QStringLiteral("30 fps"), QStringLiteral("30"));
    m_fpsCombo->addItem(QStringLiteral("29.97 fps (NTSC)"), QStringLiteral("29.97"));
    m_fpsCombo->addItem(QStringLiteral("25 fps (PAL)"), QStringLiteral("25"));
    m_fpsCombo->addItem(QStringLiteral("24 fps (Cinema)"), QStringLiteral("24"));
    m_fpsCombo->addItem(QStringLiteral("23.976 fps"), QStringLiteral("23.976"));
    advForm->addRow(m_fpsCaption, m_fpsCombo);

    m_pixFmtCaption = new ElaText(QString(), 13, m_advContainer);
    m_pixFmtCombo = new ElaComboBox(m_advContainer);
    m_pixFmtCombo->addItem(QString(), QString());
    for (const QString &pf : {QStringLiteral("yuv420p"), QStringLiteral("yuv422p"),
                              QStringLiteral("yuv444p"), QStringLiteral("yuv420p10le")}) {
        m_pixFmtCombo->addItem(pf, pf);
    }
    advForm->addRow(m_pixFmtCaption, m_pixFmtCombo);

    m_colorSpaceCaption = new ElaText(QString(), 13, m_advContainer);
    m_colorSpaceCombo = new ElaComboBox(m_advContainer);
    m_colorSpaceCombo->addItem(QString(), QString());
    m_colorSpaceCombo->addItem(QStringLiteral("BT.709 (HD/SDR)"), QStringLiteral("bt709"));
    m_colorSpaceCombo->addItem(QStringLiteral("BT.2020 HDR (PQ)"), QStringLiteral("bt2020"));
    m_colorSpaceCombo->addItem(QStringLiteral("BT.2020 HLG"), QStringLiteral("bt2020hlg"));
    m_colorSpaceCombo->addItem(QStringLiteral("BT.601 (NTSC/PAL)"), QStringLiteral("bt601"));
    advForm->addRow(m_colorSpaceCaption, m_colorSpaceCombo);

    m_hdrTonemapCaption = new ElaText(QString(), 13, m_advContainer);
    m_hdrTonemapCombo = new ElaComboBox(m_advContainer);
    m_hdrTonemapCombo->addItem(QString(), QString());
    m_hdrTonemapCombo->addItem(QStringLiteral("Hable (Film-like)"), QStringLiteral("hable"));
    m_hdrTonemapCombo->addItem(QStringLiteral("Reinhard (Natural)"), QStringLiteral("reinhard"));
    m_hdrTonemapCombo->addItem(QStringLiteral("Mobius (Smooth)"), QStringLiteral("mobius"));
    m_hdrTonemapCombo->addItem(QStringLiteral("Linear Clip"), QStringLiteral("clip"));
    advForm->addRow(m_hdrTonemapCaption, m_hdrTonemapCombo);

    m_vfCaption = new ElaText(QString(), 13, m_advContainer);
    m_vfEdit = new ElaLineEdit(m_advContainer);
    advForm->addRow(m_vfCaption, m_vfEdit);

    m_afCaption = new ElaText(QString(), 13, m_advContainer);
    m_afEdit = new ElaLineEdit(m_advContainer);
    advForm->addRow(m_afCaption, m_afEdit);

    m_advContainer->setVisible(false);
    rootLayout->addWidget(m_advContainer);

    connect(m_advToggle, &ElaCheckBox::toggled, m_advContainer, &QWidget::setVisible);

    rebuildCodecLists();
    refreshPresetList();
    retranslateUi();
}

FFmpegCommandBuilder::ConversionOptions BatchOptionsCard::currentOptions(
    const QString &inputFile, const QString &outputPath, bool copy) const
{
    FFmpegCommandBuilder::ConversionOptions opts;
    opts.inputPath = inputFile;
    opts.outputPath = outputPath;

    if (copy) {
        opts.copyStreams = true;
        opts.preserveAllStreams = true;
        opts.stripMetadata = m_stripMetaCheck->isChecked();
        return opts;
    }

    const QString fmt = m_formatCombo->currentText();
    opts.videoCodec = m_codecCombo->currentText();
    opts.audioCodec = m_audioCodecCombo->currentText();
    opts.crf = m_qualitySpin->value();
    opts.stripMetadata = m_stripMetaCheck->isChecked();
    opts.preserveAllStreams = true;
    opts.subtitleCodec = RemuxPolicy::subtitleCodecForContainer(fmt).value_or(QString());

    opts.audioBitrateKbps = m_audioBitrateCombo->currentData().toInt();
    opts.audioSampleRate = m_audioSamplerateCombo->currentData().toInt();
    opts.audioChannels = m_audioChannelsCombo->currentData().toInt();
    opts.audioNormalize = m_audioNormalizeCheck->isChecked();

    const QString spStr = m_speedPresetCombo->currentData().toString();
    if (!spStr.isEmpty()) {
        opts.preset = spStr;
    }

    opts.pixelFormat = m_pixFmtCombo->currentData().toString();

    const QString cs = m_colorSpaceCombo->currentData().toString();
    if (cs == QLatin1String("bt709")) {
        opts.colorSpace = QStringLiteral("bt709");
        opts.colorPrimaries = QStringLiteral("bt709");
        opts.colorTrc = QStringLiteral("bt709");
    } else if (cs == QLatin1String("bt2020")) {
        opts.colorSpace = QStringLiteral("bt2020nc");
        opts.colorPrimaries = QStringLiteral("bt2020");
        opts.colorTrc = QStringLiteral("smpte2084");
    } else if (cs == QLatin1String("bt2020hlg")) {
        opts.colorSpace = QStringLiteral("bt2020nc");
        opts.colorPrimaries = QStringLiteral("bt2020");
        opts.colorTrc = QStringLiteral("arib-std-b67");
    } else if (cs == QLatin1String("bt601")) {
        opts.colorSpace = QStringLiteral("smpte170m");
        opts.colorPrimaries = QStringLiteral("smpte170m");
        opts.colorTrc = QStringLiteral("smpte170m");
    }

    opts.hdrTonemap = m_hdrTonemapCombo->currentData().toString();

    QStringList vFilters;
    const QString res = m_resolutionCombo ? m_resolutionCombo->currentData().toString() : QString();
    if (!res.isEmpty()) {
        vFilters << QStringLiteral("scale=%1").arg(res);
    }
    const QString fpsVal = m_fpsCombo ? m_fpsCombo->currentData().toString() : QString();
    if (!fpsVal.isEmpty()) {
        vFilters << QStringLiteral("fps=%1").arg(fpsVal);
    }
    if (!m_vfEdit->text().trimmed().isEmpty()) {
        vFilters << m_vfEdit->text().trimmed();
    }
    if (!vFilters.isEmpty()) {
        opts.videoFilter = vFilters.join(QLatin1Char(','));
    }
    opts.audioFilter = m_afEdit->text().trimmed();

    return opts;
}

QString BatchOptionsCard::format() const
{
    return m_formatCombo->currentText();
}

QString BatchOptionsCard::videoCodec() const
{
    return m_codecCombo->currentText();
}

QString BatchOptionsCard::audioCodec() const
{
    return m_audioCodecCombo->currentText();
}

int BatchOptionsCard::crf() const
{
    return m_qualitySpin->value();
}

QString BatchOptionsCard::audioBitrate() const
{
    return m_audioBitrateCombo->currentData().toString();
}

QString BatchOptionsCard::speedPreset() const
{
    return m_speedPresetCombo->currentData().toString();
}

bool BatchOptionsCard::isSmartCopy() const
{
    return m_smartCopyCheck->isChecked();
}

bool BatchOptionsCard::isHwAccel() const
{
    return m_hwAccelCheck->isChecked();
}

void BatchOptionsCard::setBaseSettings(const QString &format, const QString &vcodec, const QString &acodec, int crf)
{
    const auto setCombo = [](ElaComboBox *combo, const QString &val) {
        const int idx = combo->findText(val);
        if (idx >= 0) combo->setCurrentIndex(idx);
    };
    if (!format.isEmpty()) setCombo(m_formatCombo, format);
    if (!vcodec.isEmpty()) setCombo(m_codecCombo, vcodec);
    if (!acodec.isEmpty()) setCombo(m_audioCodecCombo, acodec);
    if (crf >= 0) m_qualitySpin->setValue(crf);
}

void BatchOptionsCard::rebuildCodecLists()
{
    const QString videoCodec = m_codecCombo->currentText();
    const QString audioCodec = m_audioCodecCombo->currentText();

    m_codecCombo->blockSignals(true);
    m_codecCombo->clear();
    m_codecCombo->addItems({QString(), QStringLiteral("copy")});
    if (m_caps) {
        m_codecCombo->addItems(
            m_caps->filterEncoders(FFmpegCapabilities::videoEncoderCandidates()));
    }
    const int videoIndex = m_codecCombo->findText(videoCodec);
    if (!videoCodec.isEmpty() && videoIndex >= 0) {
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
    if (m_caps) {
        m_audioCodecCombo->addItems(
            m_caps->filterEncoders(FFmpegCapabilities::audioEncoderCandidates()));
    }
    const int audioIndex = m_audioCodecCombo->findText(audioCodec);
    if (!audioCodec.isEmpty() && audioIndex >= 0) {
        m_audioCodecCombo->setCurrentIndex(audioIndex);
    } else {
        int defIdx = m_audioCodecCombo->findText(QStringLiteral("aac"));
        if (defIdx < 0 && m_audioCodecCombo->count() > 2) defIdx = 2;
        if (defIdx >= 0) m_audioCodecCombo->setCurrentIndex(defIdx);
    }
    m_audioCodecCombo->blockSignals(false);

    if (m_caps && m_caps->hasHwAcceleration()) {
        m_hwDeviceLabel->setText(QStringLiteral("(%1)").arg(m_caps->primaryHwDeviceName()));
        m_hwAccelCheck->setEnabled(true);
    } else {
        m_hwDeviceLabel->setText(QStringLiteral("(%1)").arg(trId("convert.hwAccel.none")));
        m_hwAccelCheck->setEnabled(false);
        m_hwAccelCheck->setChecked(false);
    }
}

void BatchOptionsCard::refreshPresetList()
{
    if (!m_presets) return;
    const QString current = m_presetCombo->currentText();
    m_presetCombo->blockSignals(true);
    m_presetCombo->clear();
    m_presetCombo->addItem(QString());
    m_presetCombo->addItems(m_presets->names());
    m_presetCombo->blockSignals(false);
    if (!current.isEmpty()) {
        m_presetCombo->setCurrentText(current);
    }
}

void BatchOptionsCard::loadPreset(const QString &name)
{
    if (name.isEmpty() || !m_presets) return;
    const QVariantMap values = m_presets->load(name);
    if (values.isEmpty()) return;

    const auto setCombo = [](ElaComboBox *combo, const QString &value) {
        const int index = combo->findText(value);
        if (index >= 0) combo->setCurrentIndex(index);
    };
    const auto setComboData = [](ElaComboBox *combo, const QString &data) {
        const int index = combo->findData(data);
        if (index >= 0) combo->setCurrentIndex(index);
    };

    setCombo(m_formatCombo, values.value(QStringLiteral("format")).toString());
    setCombo(m_codecCombo, values.value(QStringLiteral("videoCodec")).toString());
    setCombo(m_audioCodecCombo, values.value(QStringLiteral("audioCodec")).toString());
    m_qualitySpin->setValue(values.value(QStringLiteral("crf"), 23).toInt());
    m_smartCopyCheck->setChecked(values.value(QStringLiteral("smartCopy"), false).toBool());
    m_stripMetaCheck->setChecked(values.value(QStringLiteral("stripMetadata"), false).toBool());
    if (m_hwAccelCheck->isEnabled()) {
        m_hwAccelCheck->setChecked(values.value(QStringLiteral("hwAccel"), false).toBool());
    }
    setComboData(m_audioBitrateCombo, values.value(QStringLiteral("audioBitrate")).toString());
    setComboData(m_audioSamplerateCombo, values.value(QStringLiteral("audioSamplerate")).toString());
    setComboData(m_audioChannelsCombo, values.value(QStringLiteral("audioChannels")).toString());
    m_audioNormalizeCheck->setChecked(values.value(QStringLiteral("audioNormalize"), false).toBool());
    setComboData(m_speedPresetCombo, values.value(QStringLiteral("speedPreset")).toString());
    setComboData(m_pixFmtCombo, values.value(QStringLiteral("pixFmt")).toString());
    setComboData(m_colorSpaceCombo, values.value(QStringLiteral("colorProfile")).toString());
    setComboData(m_hdrTonemapCombo, values.value(QStringLiteral("hdrTonemap")).toString());
    m_vfEdit->setText(values.value(QStringLiteral("videoFilter")).toString());
    m_afEdit->setText(values.value(QStringLiteral("audioFilter")).toString());

    emit optionsChanged();
}

void BatchOptionsCard::onFormatChanged()
{
    emit optionsChanged();
}

void BatchOptionsCard::onHwAccelToggled(bool checked)
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

void BatchOptionsCard::retranslateUi()
{
    m_presetCaption->setText(trId("convert.preset") + QStringLiteral(":"));
    refreshPresetList();
    m_formatCaption->setText(trId("convert.outputFormat") + QStringLiteral(":"));
    m_codecCaption->setText(trId("convert.codec") + QStringLiteral(":"));
    m_audioCodecCaption->setText(trId("convert.audioCodec") + QStringLiteral(":"));
    m_qualityCaption->setText(trId("convert.quality") + QStringLiteral(":"));
    m_quickTargetCaption->setText(trId("convert.quickTarget") + QStringLiteral(":"));
    m_quickSmallBtn->setText(trId("batch.quickSmall"));
    m_quickBalancedBtn->setText(trId("batch.quickBalanced"));
    m_quickHighQualityBtn->setText(trId("batch.quickHighQuality"));
    m_quickCopyBtn->setText(trId("batch.quickCopy"));
    m_smartCopyCheck->setText(trId("common.smartCopy"));
    m_stripMetaCheck->setText(trId("metadata.remove"));
    m_hwAccelCheck->setText(trId("convert.hwAccel"));
    if (m_caps && m_caps->hasHwAcceleration()) {
        m_hwDeviceLabel->setText(QStringLiteral("(%1)").arg(m_caps->primaryHwDeviceName()));
    } else {
        m_hwDeviceLabel->setText(QStringLiteral("(%1)").arg(trId("convert.hwAccel.none")));
    }

    m_advToggle->setText(trId("convert.adv.toggle"));
    m_audioBitrateCaption->setText(trId("convert.adv.audioBitrate") + QStringLiteral(":"));
    m_audioBitrateCombo->setItemText(0, trId("common.auto"));
    m_audioSamplerateCaption->setText(trId("convert.adv.samplerate") + QStringLiteral(":"));
    m_audioSamplerateCombo->setItemText(0, trId("common.auto"));
    m_audioChannelsCaption->setText(trId("convert.adv.channels") + QStringLiteral(":"));
    m_audioChannelsCombo->setItemText(0, trId("common.auto"));
    m_audioNormalizeCheck->setText(trId("convert.adv.loudnorm"));
    m_speedPresetCaption->setText(trId("convert.adv.preset") + QStringLiteral(":"));
    m_speedPresetCombo->setItemText(0, trId("common.auto"));
    m_resolutionCaption->setText(trId("convert.adv.resolution") + QStringLiteral(":"));
    m_resolutionCombo->setItemText(0, trId("common.auto"));
    m_fpsCaption->setText(trId("convert.adv.fps") + QStringLiteral(":"));
    m_fpsCombo->setItemText(0, trId("common.auto"));
    m_pixFmtCaption->setText(trId("convert.adv.pixFmt") + QStringLiteral(":"));
    m_pixFmtCombo->setItemText(0, trId("common.auto"));
    m_colorSpaceCaption->setText(trId("convert.adv.colorSpace") + QStringLiteral(":"));
    m_colorSpaceCombo->setItemText(0, trId("common.auto"));
    m_hdrTonemapCaption->setText(trId("convert.adv.hdrTonemap") + QStringLiteral(":"));
    m_hdrTonemapCombo->setItemText(0, trId("common.disabled"));
    m_vfCaption->setText(trId("convert.adv.vf") + QStringLiteral(":"));
    m_afCaption->setText(trId("convert.adv.af") + QStringLiteral(":"));
}
