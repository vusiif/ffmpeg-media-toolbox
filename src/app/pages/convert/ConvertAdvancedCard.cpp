#include "ConvertAdvancedCard.h"

#include <QFormLayout>
#include <QVBoxLayout>

#include "ElaCheckBox.h"
#include "ElaComboBox.h"
#include "ElaLineEdit.h"
#include "ElaText.h"
#include "i18n/TrId.h"

ConvertAdvancedCard::ConvertAdvancedCard(QWidget *parent)
    : ElaScrollPageArea(parent)
{
    setMinimumHeight(0);
    setMaximumHeight(QWIDGETSIZE_MAX);

    auto *advCardLayout = new QVBoxLayout(this);
    advCardLayout->setContentsMargins(18, 16, 18, 16);
    advCardLayout->setSpacing(8);

    m_advToggle = new ElaCheckBox(this);
    connect(m_advToggle, &ElaCheckBox::toggled, this, [this](bool checked) {
        m_advContainer->setVisible(checked);
    });
    advCardLayout->addWidget(m_advToggle);

    m_advContainer = new QWidget(this);
    auto *advForm = new QFormLayout(m_advContainer);
    advForm->setContentsMargins(0, 8, 0, 0);
    advForm->setSpacing(8);

    // Audio Bitrate
    m_audioBitrateCaption = new ElaText(QString(), 13, m_advContainer);
    m_audioBitrateCombo = new ElaComboBox(m_advContainer);
    m_audioBitrateCombo->addItem(QString(), QString());
    for (const QString &b : {QStringLiteral("64"), QStringLiteral("96"), QStringLiteral("128"),
                             QStringLiteral("192"), QStringLiteral("256"), QStringLiteral("320")}) {
        m_audioBitrateCombo->addItem(b + QStringLiteral(" kbps"), b);
    }
    advForm->addRow(m_audioBitrateCaption, m_audioBitrateCombo);

    // Audio Sample Rate
    m_audioSamplerateCaption = new ElaText(QString(), 13, m_advContainer);
    m_audioSamplerateCombo = new ElaComboBox(m_advContainer);
    m_audioSamplerateCombo->addItem(QString(), QString());
    for (const QString &sr : {QStringLiteral("22050"), QStringLiteral("32000"),
                              QStringLiteral("44100"), QStringLiteral("48000"),
                              QStringLiteral("96000")}) {
        m_audioSamplerateCombo->addItem(sr + QStringLiteral(" Hz"), sr);
    }
    advForm->addRow(m_audioSamplerateCaption, m_audioSamplerateCombo);

    // Audio Channels
    m_audioChannelsCaption = new ElaText(QString(), 13, m_advContainer);
    m_audioChannelsCombo = new ElaComboBox(m_advContainer);
    m_audioChannelsCombo->addItem(QString(), QString());
    m_audioChannelsCombo->addItem(QString(), QStringLiteral("1"));
    m_audioChannelsCombo->addItem(QString(), QStringLiteral("2"));
    m_audioChannelsCombo->addItem(QString(), QStringLiteral("6"));
    advForm->addRow(m_audioChannelsCaption, m_audioChannelsCombo);

    // Audio Loudness Normalization
    m_audioNormalizeCheck = new ElaCheckBox(m_advContainer);
    advForm->addRow(new ElaText(QString(), 13, m_advContainer), m_audioNormalizeCheck);

    // Encoder Speed Preset
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

    // Resolution Scaling
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

    // Frame Rate (FPS)
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

    // Pixel Format
    m_pixFmtCaption = new ElaText(QString(), 13, m_advContainer);
    m_pixFmtCombo = new ElaComboBox(m_advContainer);
    m_pixFmtCombo->addItem(QString(), QString());
    for (const QString &pf : {QStringLiteral("yuv420p"), QStringLiteral("yuv422p"),
                              QStringLiteral("yuv444p"), QStringLiteral("yuv420p10le"),
                              QStringLiteral("nv12")}) {
        m_pixFmtCombo->addItem(pf, pf);
    }
    advForm->addRow(m_pixFmtCaption, m_pixFmtCombo);

    // Color space profile
    m_colorSpaceCaption = new ElaText(QString(), 13, m_advContainer);
    m_colorSpaceCombo = new ElaComboBox(m_advContainer);
    m_colorSpaceCombo->addItem(QString(), QString());
    m_colorSpaceCombo->addItem(QStringLiteral("BT.709 (HD/SDR)"), QStringLiteral("bt709"));
    m_colorSpaceCombo->addItem(QStringLiteral("BT.2020 PQ (HDR10)"), QStringLiteral("bt2020"));
    m_colorSpaceCombo->addItem(QStringLiteral("BT.2020 HLG"), QStringLiteral("hlg"));
    m_colorSpaceCombo->addItem(QStringLiteral("BT.601 (SD)"), QStringLiteral("bt601"));
    advForm->addRow(m_colorSpaceCaption, m_colorSpaceCombo);

    // HDR to SDR Tonemapping
    m_hdrTonemapCaption = new ElaText(QString(), 13, m_advContainer);
    m_hdrTonemapCombo = new ElaComboBox(m_advContainer);
    m_hdrTonemapCombo->addItem(QString(), QString());
    m_hdrTonemapCombo->addItem(QStringLiteral("Hable (Filmic)"), QStringLiteral("hable"));
    m_hdrTonemapCombo->addItem(QStringLiteral("Mobius"), QStringLiteral("mobius"));
    m_hdrTonemapCombo->addItem(QStringLiteral("Reinhard"), QStringLiteral("reinhard"));
    advForm->addRow(m_hdrTonemapCaption, m_hdrTonemapCombo);

    // Custom Filters
    m_vfCaption = new ElaText(QString(), 13, m_advContainer);
    m_vfEdit = new ElaLineEdit(m_advContainer);
    advForm->addRow(m_vfCaption, m_vfEdit);

    m_afCaption = new ElaText(QString(), 13, m_advContainer);
    m_afEdit = new ElaLineEdit(m_advContainer);
    advForm->addRow(m_afCaption, m_afEdit);

    m_advContainer->setVisible(false);
    advCardLayout->addWidget(m_advContainer);

    connect(m_audioBitrateCombo, &ElaComboBox::currentIndexChanged, this, &ConvertAdvancedCard::optionsChanged);
    connect(m_audioSamplerateCombo, &ElaComboBox::currentIndexChanged, this, &ConvertAdvancedCard::optionsChanged);
    connect(m_audioChannelsCombo, &ElaComboBox::currentIndexChanged, this, &ConvertAdvancedCard::optionsChanged);
    connect(m_audioNormalizeCheck, &ElaCheckBox::toggled, this, &ConvertAdvancedCard::optionsChanged);
    connect(m_speedPresetCombo, &ElaComboBox::currentIndexChanged, this, &ConvertAdvancedCard::optionsChanged);
    connect(m_resolutionCombo, &ElaComboBox::currentIndexChanged, this, &ConvertAdvancedCard::optionsChanged);
    connect(m_fpsCombo, &ElaComboBox::currentIndexChanged, this, &ConvertAdvancedCard::optionsChanged);
    connect(m_pixFmtCombo, &ElaComboBox::currentIndexChanged, this, &ConvertAdvancedCard::optionsChanged);
    connect(m_colorSpaceCombo, &ElaComboBox::currentIndexChanged, this, &ConvertAdvancedCard::optionsChanged);
    connect(m_hdrTonemapCombo, &ElaComboBox::currentIndexChanged, this, &ConvertAdvancedCard::optionsChanged);
    connect(m_vfEdit, &ElaLineEdit::textChanged, this, &ConvertAdvancedCard::optionsChanged);
    connect(m_afEdit, &ElaLineEdit::textChanged, this, &ConvertAdvancedCard::optionsChanged);

    retranslateUi();
}

void ConvertAdvancedCard::applyTo(FFmpegCommandBuilder::ConversionOptions &opts) const
{
    const QString abStr = m_audioBitrateCombo->currentData().toString();
    if (!abStr.isEmpty()) {
        opts.audioBitrateKbps = abStr.toInt();
    }
    const QString arStr = m_audioSamplerateCombo->currentData().toString();
    if (!arStr.isEmpty()) {
        opts.audioSampleRate = arStr.toInt();
    }
    const QString acStr = m_audioChannelsCombo->currentData().toString();
    if (!acStr.isEmpty()) {
        opts.audioChannels = acStr.toInt();
    }
    opts.audioNormalize = m_audioNormalizeCheck->isChecked();

    const QString spStr = m_speedPresetCombo->currentData().toString();
    if (!spStr.isEmpty()) {
        opts.preset = spStr;
    }
    const QString pfStr = m_pixFmtCombo->currentData().toString();
    if (!pfStr.isEmpty()) {
        opts.pixelFormat = pfStr;
    }

    const QString cs = m_colorSpaceCombo->currentData().toString();
    if (cs == QLatin1String("bt709")) {
        opts.colorSpace = QStringLiteral("bt709");
        opts.colorPrimaries = QStringLiteral("bt709");
        opts.colorTrc = QStringLiteral("bt709");
    } else if (cs == QLatin1String("bt2020")) {
        opts.colorSpace = QStringLiteral("bt2020nc");
        opts.colorPrimaries = QStringLiteral("bt2020");
        opts.colorTrc = QStringLiteral("smpte2084");
    } else if (cs == QLatin1String("hlg")) {
        opts.colorSpace = QStringLiteral("bt2020nc");
        opts.colorPrimaries = QStringLiteral("bt2020");
        opts.colorTrc = QStringLiteral("arib-std-b67");
    } else if (cs == QLatin1String("bt601")) {
        opts.colorSpace = QStringLiteral("smpte170m");
        opts.colorPrimaries = QStringLiteral("smpte170m");
        opts.colorTrc = QStringLiteral("smpte170m");
    }

    opts.hdrTonemap = m_hdrTonemapCombo->currentData().toString();
    if (!opts.hdrTonemap.isEmpty() && opts.colorSpace.isEmpty()) {
        opts.colorSpace = QStringLiteral("bt709");
        opts.colorPrimaries = QStringLiteral("bt709");
        opts.colorTrc = QStringLiteral("bt709");
    }

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
}

void ConvertAdvancedCard::savePresetValues(QVariantMap &values) const
{
    values.insert(QStringLiteral("audioBitrate"), m_audioBitrateCombo->currentData().toString());
    values.insert(QStringLiteral("audioSamplerate"), m_audioSamplerateCombo->currentData().toString());
    values.insert(QStringLiteral("audioChannels"), m_audioChannelsCombo->currentData().toString());
    values.insert(QStringLiteral("audioNormalize"), m_audioNormalizeCheck->isChecked());
    values.insert(QStringLiteral("speedPreset"), m_speedPresetCombo->currentData().toString());
    values.insert(QStringLiteral("resolution"), m_resolutionCombo->currentData().toString());
    values.insert(QStringLiteral("fps"), m_fpsCombo->currentData().toString());
    values.insert(QStringLiteral("pixFmt"), m_pixFmtCombo->currentData().toString());
    values.insert(QStringLiteral("colorProfile"), m_colorSpaceCombo->currentData().toString());
    values.insert(QStringLiteral("hdrTonemap"), m_hdrTonemapCombo->currentData().toString());
    values.insert(QStringLiteral("videoFilter"), m_vfEdit->text());
    values.insert(QStringLiteral("audioFilter"), m_afEdit->text());
}

void ConvertAdvancedCard::loadPresetValues(const QVariantMap &values)
{
    const QString ab = values.value(QStringLiteral("audioBitrate")).toString();
    int abIdx = m_audioBitrateCombo->findData(ab);
    if (abIdx >= 0) m_audioBitrateCombo->setCurrentIndex(abIdx);

    const QString ar = values.value(QStringLiteral("audioSamplerate")).toString();
    int arIdx = m_audioSamplerateCombo->findData(ar);
    if (arIdx >= 0) m_audioSamplerateCombo->setCurrentIndex(arIdx);

    const QString ac = values.value(QStringLiteral("audioChannels")).toString();
    int acIdx = m_audioChannelsCombo->findData(ac);
    if (acIdx >= 0) m_audioChannelsCombo->setCurrentIndex(acIdx);

    m_audioNormalizeCheck->setChecked(values.value(QStringLiteral("audioNormalize"), false).toBool());

    const QString sp = values.value(QStringLiteral("speedPreset")).toString();
    int spIdx = m_speedPresetCombo->findData(sp);
    if (spIdx >= 0) m_speedPresetCombo->setCurrentIndex(spIdx);

    const QString res = values.value(QStringLiteral("resolution")).toString();
    int resIdx = m_resolutionCombo->findData(res);
    if (resIdx >= 0) m_resolutionCombo->setCurrentIndex(resIdx);

    const QString fps = values.value(QStringLiteral("fps")).toString();
    int fpsIdx = m_fpsCombo->findData(fps);
    if (fpsIdx >= 0) m_fpsCombo->setCurrentIndex(fpsIdx);

    const QString pf = values.value(QStringLiteral("pixFmt")).toString();
    int pfIdx = m_pixFmtCombo->findData(pf);
    if (pfIdx >= 0) m_pixFmtCombo->setCurrentIndex(pfIdx);

    const QString cp = values.value(QStringLiteral("colorProfile")).toString();
    int cpIdx = m_colorSpaceCombo->findData(cp);
    if (cpIdx >= 0) m_colorSpaceCombo->setCurrentIndex(cpIdx);

    const QString ht = values.value(QStringLiteral("hdrTonemap")).toString();
    int htIdx = m_hdrTonemapCombo->findData(ht);
    if (htIdx >= 0) m_hdrTonemapCombo->setCurrentIndex(htIdx);

    m_vfEdit->setText(values.value(QStringLiteral("videoFilter")).toString());
    m_afEdit->setText(values.value(QStringLiteral("audioFilter")).toString());
}

void ConvertAdvancedCard::enableHdrTonemapDefault()
{
    if (m_hdrTonemapCombo->currentIndex() == 0) {
        m_hdrTonemapCombo->setCurrentIndex(1);
    }
}

void ConvertAdvancedCard::setCustomFilters(const QString &vf, const QString &af)
{
    if (!vf.isEmpty()) m_vfEdit->setText(vf);
    if (!af.isEmpty()) m_afEdit->setText(af);
    m_advToggle->setChecked(true);
    emit optionsChanged();
}

void ConvertAdvancedCard::retranslateUi()
{
    m_advToggle->setText(trId("convert.adv.toggle"));
    m_audioBitrateCaption->setText(trId("convert.adv.audioBitrate") + QStringLiteral(":"));
    m_audioBitrateCombo->setItemText(0, trId("common.auto"));
    m_audioSamplerateCaption->setText(trId("convert.adv.samplerate") + QStringLiteral(":"));
    m_audioSamplerateCombo->setItemText(0, trId("common.auto"));
    m_audioChannelsCaption->setText(trId("convert.adv.channels") + QStringLiteral(":"));
    m_audioChannelsCombo->setItemText(0, trId("common.auto"));
    m_audioChannelsCombo->setItemText(1, trId("convert.adv.mono"));
    m_audioChannelsCombo->setItemText(2, trId("convert.adv.stereo"));
    m_audioChannelsCombo->setItemText(3, trId("convert.adv.surround51"));
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
