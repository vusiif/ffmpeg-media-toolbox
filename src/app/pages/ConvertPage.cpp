#include "ConvertPage.h"

#include <QtConcurrent/QtConcurrentRun>

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMimeData>
#include <QSettings>
#include <QUrl>
#include <QVBoxLayout>

#include "ElaCheckBox.h"
#include "ElaComboBox.h"
#include "ElaDoubleSpinBox.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include "ElaScrollArea.h"
#include "ElaScrollPageArea.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "app/AppSettings.h"
#include "app/RecentFiles.h"
#include "conversion/RemuxPolicy.h"
#include "ffmpeg/FFmpegCapabilities.h"
#include "ffmpeg/FFmpegCommandBuilder.h"
#include "ffmpeg/FFmpegLocator.h"
#include "i18n/TrId.h"
#include "jobs/Job.h"
#include "jobs/JobQueue.h"
#include "media/MediaProbeService.h"
#include "output/OutputEstimate.h"
#include "output/OutputNaming.h"
#include "presets/PresetManager.h"

namespace {
QString defaultNamingTemplate()
{
    QSettings s(AppSettings::organization(), AppSettings::application());
    return s.value(QStringLiteral("output/template"), QStringLiteral("{name}.{format}")).toString();
}

QString defaultOutputDirectory()
{
    QSettings s(AppSettings::organization(), AppSettings::application());
    return s.value(QStringLiteral("output/directory")).toString();
}
} // namespace

ConvertPage::ConvertPage(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                         MediaProbeService *probe, FFmpegCapabilities *caps,
                         RecentFiles *recentFiles, JobQueue *queue,
                         QWidget *parent)
    : QWidget(parent)
    , m_locator(locator)
    , m_builder(builder)
    , m_probe(probe)
    , m_caps(caps)
    , m_recentFiles(recentFiles)
    , m_queue(queue)
{
    m_presets = new PresetManager(this);
    setAcceptDrops(true);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(30, 30, 30, 30);
    root->setSpacing(16);

    m_titleLabel = new ElaText(QString(), 13, this);
    m_titleLabel->setTextPixelSize(28);
    root->addWidget(m_titleLabel);

    m_scrollArea = new ElaScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setIsGrabGesture(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *scrollBody = new QWidget(m_scrollArea);
    scrollBody->setStyleSheet(QStringLiteral("background-color: transparent;"));
    auto *bodyLayout = new QVBoxLayout(scrollBody);
    bodyLayout->setContentsMargins(0, 0, 10, 0);
    bodyLayout->setSpacing(14);

    // Card 1: Media Source & Probing
    m_inputCard = new ElaScrollPageArea(scrollBody);
    m_inputCard->setMinimumHeight(0);
    m_inputCard->setMaximumHeight(QWIDGETSIZE_MAX);
    auto *inputLayout = new QVBoxLayout(m_inputCard);
    inputLayout->setContentsMargins(18, 16, 18, 16);
    inputLayout->setSpacing(10);

    m_inputCardTitle = new ElaText(QString(), 15, m_inputCard);
    m_inputCardTitle->setStyleSheet(QStringLiteral("font-weight: bold;"));
    inputLayout->addWidget(m_inputCardTitle);

    auto *inputRow = new QHBoxLayout();
    m_inputCaption = new ElaText(QString(), 13, m_inputCard);
    m_inputEdit = new ElaLineEdit(m_inputCard);
    m_inputEdit->setReadOnly(true);
    m_browseButton = new ElaPushButton(m_inputCard);
    connect(m_browseButton, &ElaPushButton::clicked, this, &ConvertPage::browseInput);
    inputRow->addWidget(m_inputCaption);
    inputRow->addWidget(m_inputEdit, 1);
    inputRow->addWidget(m_browseButton);
    inputLayout->addLayout(inputRow);

    auto *recentRow = new QHBoxLayout();
    m_recentCaption = new ElaText(QString(), 13, m_inputCard);
    m_recentCombo = new ElaComboBox(m_inputCard);
    m_recentCaption->setVisible(false);
    m_recentCombo->setVisible(false);
    recentRow->addWidget(m_recentCaption);
    recentRow->addWidget(m_recentCombo, 1);
    inputLayout->addLayout(recentRow);

    m_infoLabel = new ElaText(QString(), 13, m_inputCard);
    m_infoLabel->setTextPixelSize(12);
    inputLayout->addWidget(m_infoLabel);

    m_metaCaption = new ElaText(QString(), 13, m_inputCard);
    m_metaCaption->setStyleSheet(QStringLiteral("font-weight: bold;"));
    m_metaValue = new ElaText(QString(), 13, m_inputCard);
    m_metaValue->setWordWrap(true);
    m_metaCaption->setVisible(false);
    m_metaValue->setVisible(false);
    inputLayout->addWidget(m_metaCaption);
    inputLayout->addWidget(m_metaValue);

    bodyLayout->addWidget(m_inputCard);

    // Card 2: Presets & Encoding Settings
    m_codecCard = new ElaScrollPageArea(scrollBody);
    m_codecCard->setMinimumHeight(0);
    m_codecCard->setMaximumHeight(QWIDGETSIZE_MAX);
    auto *codecLayout = new QVBoxLayout(m_codecCard);
    codecLayout->setContentsMargins(18, 16, 18, 16);
    codecLayout->setSpacing(10);

    m_codecCardTitle = new ElaText(QString(), 15, m_codecCard);
    m_codecCardTitle->setStyleSheet(QStringLiteral("font-weight: bold;"));
    codecLayout->addWidget(m_codecCardTitle);

    auto *presetRow = new QHBoxLayout();
    m_presetCaption = new ElaText(QString(), 13, m_codecCard);
    m_presetCombo = new ElaComboBox(m_codecCard);
    m_presetSaveButton = new ElaPushButton(m_codecCard);
    m_presetDeleteButton = new ElaPushButton(m_codecCard);
    connect(m_presetCombo, &ElaComboBox::currentTextChanged,
            this, &ConvertPage::loadPreset);
    connect(m_presetSaveButton, &ElaPushButton::clicked, this, &ConvertPage::savePreset);
    connect(m_presetDeleteButton, &ElaPushButton::clicked, this, &ConvertPage::deletePreset);
    presetRow->addWidget(m_presetCaption);
    presetRow->addWidget(m_presetCombo, 1);
    presetRow->addWidget(m_presetSaveButton);
    presetRow->addWidget(m_presetDeleteButton);
    codecLayout->addLayout(presetRow);

    auto *form = new QFormLayout();
    form->setSpacing(10);

    m_formatCaption = new ElaText(QString(), 13, m_codecCard);
    m_formatCombo = new ElaComboBox(m_codecCard);
    m_formatCombo->addItems({QStringLiteral("mp4"), QStringLiteral("mkv"), QStringLiteral("mov"),
                             QStringLiteral("webm"), QStringLiteral("avi"), QStringLiteral("mp3"),
                             QStringLiteral("aac"), QStringLiteral("flac"), QStringLiteral("wav")});

    m_codecCaption = new ElaText(QString(), 13, m_codecCard);
    m_codecCombo = new ElaComboBox(m_codecCard);

    m_audioCodecCaption = new ElaText(QString(), 13, m_codecCard);
    m_audioCodecCombo = new ElaComboBox(m_codecCard);

    m_rateControlCaption = new ElaText(QString(), 13, m_codecCard);
    m_rateControlCombo = new ElaComboBox(m_codecCard);
    m_rateControlCombo->addItem(trId("convert.rateControl.crf"));
    m_rateControlCombo->addItem(trId("convert.rateControl.targetSize"));

    m_qualityCaption = new ElaText(QString(), 13, m_codecCard);
    m_qualitySpin = new ElaSpinBox(m_codecCard);
    m_qualitySpin->setRange(0, 51);
    m_qualitySpin->setValue(23);

    m_targetSizeCaption = new ElaText(QString(), 13, m_codecCard);
    m_targetSizeWidget = new QWidget(m_codecCard);
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

    m_targetSizeCaption->setVisible(false);
    m_targetSizeWidget->setVisible(false);

    connect(m_rateControlCombo, QOverload<int>::of(&ElaComboBox::currentIndexChanged), this, [this](int idx) {
        const bool isTarget = (idx == 1);
        m_qualityCaption->setVisible(!isTarget);
        m_qualitySpin->setVisible(!isTarget);
        m_targetSizeCaption->setVisible(isTarget);
        m_targetSizeWidget->setVisible(isTarget);
        updateCommandPreview();
    });

    m_smartCopyCheck = new ElaCheckBox(m_codecCard);
    m_preserveAllStreamsCheck = new ElaCheckBox(m_codecCard);
    m_preserveAllStreamsCheck->setChecked(true);
    m_stripMetaCheck = new ElaCheckBox(m_codecCard);

    m_hwAccelCheck = new ElaCheckBox(m_codecCard);
    m_hwDeviceLabel = new ElaText(QString(), 12, m_codecCard);
    m_hwDeviceLabel->setStyleSheet(QStringLiteral("color: #0078d4; font-weight: bold;"));
    auto *hwRow = new QHBoxLayout();
    hwRow->addWidget(m_hwAccelCheck);
    hwRow->addWidget(m_hwDeviceLabel);
    hwRow->addStretch();

    connect(m_hwAccelCheck, &ElaCheckBox::toggled, this, [this](bool checked) {
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
        updateCommandPreview();
    });

    form->addRow(m_formatCaption, m_formatCombo);
    form->addRow(m_codecCaption, m_codecCombo);
    form->addRow(m_audioCodecCaption, m_audioCodecCombo);
    form->addRow(m_rateControlCaption, m_rateControlCombo);
    form->addRow(m_qualityCaption, m_qualitySpin);
    form->addRow(m_targetSizeCaption, m_targetSizeWidget);
    form->addRow(new ElaText(QString(), 13, m_codecCard), m_smartCopyCheck);
    form->addRow(new ElaText(QString(), 13, m_codecCard), m_preserveAllStreamsCheck);
    form->addRow(new ElaText(QString(), 13, m_codecCard), m_stripMetaCheck);
    form->addRow(new ElaText(QString(), 13, m_codecCard), hwRow);
    codecLayout->addLayout(form);

    bodyLayout->addWidget(m_codecCard);

    // Card 3: Advanced Options (Collapsible)
    m_advCard = new ElaScrollPageArea(scrollBody);
    m_advCard->setMinimumHeight(0);
    m_advCard->setMaximumHeight(QWIDGETSIZE_MAX);
    auto *advCardLayout = new QVBoxLayout(m_advCard);
    advCardLayout->setContentsMargins(18, 16, 18, 16);
    advCardLayout->setSpacing(8);

    m_advToggle = new ElaCheckBox(m_advCard);
    connect(m_advToggle, &ElaCheckBox::toggled, this, [this](bool checked) {
        m_advContainer->setVisible(checked);
    });
    advCardLayout->addWidget(m_advToggle);

    m_advContainer = new QWidget(m_advCard);
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

    bodyLayout->addWidget(m_advCard);

    // Card 4: Output Preview & Size Estimation
    m_previewCard = new ElaScrollPageArea(scrollBody);
    m_previewCard->setMinimumHeight(0);
    m_previewCard->setMaximumHeight(QWIDGETSIZE_MAX);
    auto *previewLayout = new QVBoxLayout(m_previewCard);
    previewLayout->setContentsMargins(18, 16, 18, 16);
    previewLayout->setSpacing(10);

    auto *previewTopRow = new QHBoxLayout();
    m_previewCardTitle = new ElaText(QString(), 15, m_previewCard);
    m_previewCardTitle->setStyleSheet(QStringLiteral("font-weight: bold;"));
    m_estLabel = new ElaText(QString(), 13, m_previewCard);
    m_estLabel->setStyleSheet(QStringLiteral("color: #0078d4; font-weight: bold;"));
    m_estLabel->setVisible(false);
    previewTopRow->addWidget(m_previewCardTitle);
    previewTopRow->addStretch();
    previewTopRow->addWidget(m_estLabel);
    previewLayout->addLayout(previewTopRow);

    m_previewCaption = new ElaText(QString(), 13, m_previewCard);
    m_previewEdit = new ElaLineEdit(m_previewCard);
    m_previewEdit->setReadOnly(true);
    previewLayout->addWidget(m_previewCaption);
    previewLayout->addWidget(m_previewEdit);

    bodyLayout->addWidget(m_previewCard);

    bodyLayout->addStretch();
    m_scrollArea->setWidget(scrollBody);
    root->addWidget(m_scrollArea, 1);

    // Pinned Bottom Action Row
    auto *buttonRow = new QHBoxLayout();
    buttonRow->addStretch();
    m_addButton = new ElaPushButton(this);
    m_addButton->setFixedHeight(38);
    m_addButton->setMinimumWidth(160);
    connect(m_addButton, &ElaPushButton::clicked, this, &ConvertPage::addToQueue);
    buttonRow->addWidget(m_addButton);
    root->addLayout(buttonRow);

    root->addStretch();

    m_probeWatcher = new QFutureWatcher<MediaFile>(this);
    connect(m_probeWatcher, &QFutureWatcher<MediaFile>::finished,
            this, &ConvertPage::onProbeFinished);

    connect(m_formatCombo, &ElaComboBox::currentTextChanged, this, &ConvertPage::updateCommandPreview);
    connect(m_codecCombo, &ElaComboBox::currentTextChanged, this, &ConvertPage::updateCommandPreview);
    connect(m_audioCodecCombo, &ElaComboBox::currentTextChanged, this, &ConvertPage::updateCommandPreview);
    connect(m_qualitySpin, &ElaSpinBox::valueChanged, this, &ConvertPage::updateCommandPreview);
    connect(m_targetSizeSpin, &ElaDoubleSpinBox::valueChanged, this, &ConvertPage::updateCommandPreview);
    connect(m_smartCopyCheck, &ElaCheckBox::toggled, this, &ConvertPage::updateCommandPreview);
    connect(m_preserveAllStreamsCheck, &ElaCheckBox::toggled, this, &ConvertPage::updateCommandPreview);
    connect(m_stripMetaCheck, &ElaCheckBox::toggled, this, &ConvertPage::updateCommandPreview);
    connect(m_audioBitrateCombo, &ElaComboBox::currentIndexChanged, this, &ConvertPage::updateCommandPreview);
    connect(m_audioSamplerateCombo, &ElaComboBox::currentIndexChanged, this, &ConvertPage::updateCommandPreview);
    connect(m_audioChannelsCombo, &ElaComboBox::currentIndexChanged, this, &ConvertPage::updateCommandPreview);
    connect(m_audioNormalizeCheck, &ElaCheckBox::toggled, this, &ConvertPage::updateCommandPreview);
    connect(m_speedPresetCombo, &ElaComboBox::currentIndexChanged, this, &ConvertPage::updateCommandPreview);
    connect(m_pixFmtCombo, &ElaComboBox::currentIndexChanged, this, &ConvertPage::updateCommandPreview);
    connect(m_colorSpaceCombo, &ElaComboBox::currentIndexChanged, this, &ConvertPage::updateCommandPreview);
    connect(m_hdrTonemapCombo, &ElaComboBox::currentIndexChanged, this, &ConvertPage::updateCommandPreview);
    connect(m_vfEdit, &ElaLineEdit::textChanged, this, &ConvertPage::updateCommandPreview);
    connect(m_afEdit, &ElaLineEdit::textChanged, this, &ConvertPage::updateCommandPreview);
    connect(m_caps, &FFmpegCapabilities::capabilitiesChanged,
            this, &ConvertPage::rebuildCodecLists);
    connect(m_recentFiles, &RecentFiles::changed, this, &ConvertPage::refreshRecentList);
    connect(m_recentCombo, &ElaComboBox::currentIndexChanged, this, [this](int index) {
        if (index < 0) {
            return;
        }
        const QString path = m_recentCombo->itemText(index);
        if (path.isEmpty() || path == m_inputFile) {
            return;
        }
        loadRecentFile(path);
        m_recentFiles->add(path);
    });

    rebuildCodecLists();
    refreshRecentList();
    refreshPresetList();
    retranslateUi();
}

void ConvertPage::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void ConvertPage::dropEvent(QDropEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;

    m_inputFile = urls.first().toLocalFile();
    m_inputEdit->setText(m_inputFile);
    m_infoLabel->clear();
    m_recentFiles->add(m_inputFile);
    startProbe(false);
    event->acceptProposedAction();
}

void ConvertPage::browseInput()
{
    const QString path = QFileDialog::getOpenFileName(
        this, trId("convert.input"), QString(),
        QStringLiteral("Media files (*.mp4 *.mkv *.mov *.webm *.avi *.flv *.wmv *.mp3 *.aac *.flac *.wav *.m4a);;All files (*)"));
    if (path.isEmpty()) return;

    m_inputFile = path;
    m_inputEdit->setText(path);
    m_infoLabel->clear();
    m_recentFiles->add(path);
    startProbe(false);
}

void ConvertPage::startProbe(bool enqueueAfter)
{
    if (m_inputFile.isEmpty()) return;

    m_enqueueAfterProbe = enqueueAfter;
    m_probeInfo = MediaFile();
    showMetadata({});
    MediaProbeService *probe = m_probe;
    const QString path = m_inputFile;
    m_probeWatcher->setFuture(QtConcurrent::run([probe, path]() {
        return probe->probe(path);
    }));
}

void ConvertPage::onProbeFinished()
{
    const MediaFile info = m_probeWatcher->result();
    m_probeInfo = info;
    if (m_enqueueAfterProbe) {
        enqueueJob(info);
        return;
    }

    if (!info.isValid()) {
        m_infoLabel->setText(trId("convert.invalidFile"));
        showMetadata({});
        return;
    }

    QStringList parts;
    parts << info.formatName;
    parts << QStringLiteral("%1s").arg(info.duration, 0, 'f', 1);
    if (info.hasVideo()) {
        const VideoStream &video = info.primaryVideo();
        parts << QStringLiteral("%1x%2").arg(video.width).arg(video.height);
        parts << video.codec;
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
    updateCommandPreview();
}

FFmpegCommandBuilder::ConversionOptions ConvertPage::currentOptions(const QString &outputPath, bool copy) const
{
    FFmpegCommandBuilder::ConversionOptions opts;
    opts.inputPath = m_inputFile;
    opts.outputPath = outputPath;
    opts.videoCodec = m_codecCombo->currentText();
    opts.audioCodec = m_audioCodecCombo->currentText();
    if (m_rateControlCombo && m_rateControlCombo->currentIndex() == 1) {
        opts.targetSizeMB = m_targetSizeSpin->value();
        opts.durationSeconds = m_probeInfo.duration;
        opts.crf = -1;
    } else {
        opts.crf = m_qualitySpin->value();
        opts.targetSizeMB = -1.0;
    }
    opts.copyStreams = copy;
    opts.preserveAllStreams = copy || m_preserveAllStreamsCheck->isChecked();
    opts.stripMetadata = m_stripMetaCheck->isChecked();
    const auto subCodec = RemuxPolicy::subtitleCodecForContainer(m_formatCombo->currentText());
    if (subCodec.has_value() && !subCodec->isEmpty()) {
        opts.subtitleCodec = *subCodec;
    }
    if (m_hwAccelCheck->isChecked()) {
        opts.hwAccel = m_caps->recommendedHwAccelForEncoder(opts.videoCodec);
    }

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

    opts.videoFilter = m_vfEdit->text().trimmed();
    opts.audioFilter = m_afEdit->text().trimmed();

    return opts;
}

void ConvertPage::enqueueJob(const MediaFile &info)
{
    if (!info.isValid()) return;

    const QString format = m_formatCombo->currentText();
    const QString outputPath = OutputNaming::build(m_inputFile, defaultNamingTemplate(),
                                                   format, defaultOutputDirectory());

    const bool copy = m_smartCopyCheck->isChecked() &&
                      RemuxPolicy::canCopyAllStreams(info, format);
    const QStringList args = m_builder->buildConversion(currentOptions(outputPath, copy));

    auto *job = new Job();
    job->setName(QFileInfo(m_inputFile).fileName());
    job->setCommand(m_locator->ffmpegPath(), args);
    job->setTotalDuration(info.duration);
    m_queue->addJob(job);

    // The stamp is generated per call, so sync the preview to the command
    // that was actually queued instead of a stale one.
    m_previewEdit->setText(m_locator->ffmpegPath() + QStringLiteral(" ")
                           + args.join(QLatin1Char(' ')));
}

void ConvertPage::addToQueue()
{
    if (m_inputFile.isEmpty()) return;
    startProbe(true);
}

void ConvertPage::updateCommandPreview()
{
    if (m_inputFile.isEmpty()) {
        m_previewEdit->clear();
        m_estLabel->clear();
        m_estLabel->setVisible(false);
        return;
    }

    const QString format = m_formatCombo->currentText();
    const QString outputPath = OutputNaming::build(m_inputFile, defaultNamingTemplate(),
                                                   format, defaultOutputDirectory());
    const bool copy = m_smartCopyCheck->isChecked() &&
                      RemuxPolicy::canCopyAllStreams(m_probeInfo, format);
    const QStringList args = m_builder->buildConversion(currentOptions(outputPath, copy));
    m_previewEdit->setText(m_locator->ffmpegPath() + QStringLiteral(" ") + args.join(QLatin1Char(' ')));

    if (m_rateControlCombo && m_rateControlCombo->currentIndex() == 1) {
        const qint64 estimate = static_cast<qint64>(m_targetSizeSpin->value() * 1024.0 * 1024.0);
        m_estLabel->setText(trId("convert.estimate") + QStringLiteral(": ≈")
                            + OutputEstimate::humanizeBytes(estimate));
        m_estLabel->setVisible(true);
    } else {
        const qint64 estimate = OutputEstimate::estimateBytes(
            m_probeInfo, m_codecCombo->currentText(), m_audioCodecCombo->currentText(),
            m_qualitySpin->value(), copy);
        if (estimate > 0) {
            m_estLabel->setText(trId("convert.estimate") + QStringLiteral(": ≈")
                                + OutputEstimate::humanizeBytes(estimate));
            m_estLabel->setVisible(true);
        } else {
            m_estLabel->clear();
            m_estLabel->setVisible(false);
        }
    }
}

void ConvertPage::refreshRecentList()
{
    const QStringList files = m_recentFiles->files();

    m_recentCombo->blockSignals(true);
    m_recentCombo->clear();
    m_recentCombo->addItems(files);
    m_recentCombo->blockSignals(false);

    const bool visible = !files.isEmpty();
    m_recentCaption->setVisible(visible);
    m_recentCombo->setVisible(visible);
}

void ConvertPage::loadRecentFile(const QString &path)
{
    m_inputFile = path;
    m_inputEdit->setText(path);
    m_infoLabel->clear();
    startProbe(false);
}

void ConvertPage::showMetadata(const QVariantMap &tags)
{
    QStringList parts;
    for (auto it = tags.constBegin(); it != tags.constEnd(); ++it) {
        parts << QStringLiteral("%1: %2").arg(it.key(), it.value().toString());
    }

    m_metaValue->setText(parts.join(QStringLiteral(" | ")));
    const bool visible = !parts.isEmpty();
    m_metaCaption->setVisible(visible);
    m_metaValue->setVisible(visible);
}

void ConvertPage::rebuildCodecLists()
{
    const QString videoCodec = m_codecCombo->currentText();
    const QString audioCodec = m_audioCodecCombo->currentText();

    m_codecCombo->blockSignals(true);
    m_codecCombo->clear();
    m_codecCombo->addItems({QString(), QStringLiteral("copy")});
    m_codecCombo->addItems(
        m_caps->filterEncoders(FFmpegCapabilities::videoEncoderCandidates()));
    const int videoIndex = m_codecCombo->findText(videoCodec);
    if (videoIndex >= 0) {
        m_codecCombo->setCurrentIndex(videoIndex);
    }
    m_codecCombo->blockSignals(false);

    m_audioCodecCombo->blockSignals(true);
    m_audioCodecCombo->clear();
    m_audioCodecCombo->addItems({QString(), QStringLiteral("copy")});
    m_audioCodecCombo->addItems(
        m_caps->filterEncoders(FFmpegCapabilities::audioEncoderCandidates()));
    const int audioIndex = m_audioCodecCombo->findText(audioCodec);
    if (audioIndex >= 0) {
        m_audioCodecCombo->setCurrentIndex(audioIndex);
    }
    if (m_caps->hasHwAcceleration()) {
        m_hwDeviceLabel->setText(QStringLiteral("(%1)").arg(m_caps->primaryHwDeviceName()));
        m_hwAccelCheck->setEnabled(true);
    } else {
        m_hwDeviceLabel->setText(QStringLiteral("(%1)").arg(trId("convert.hwAccel.none")));
        m_hwAccelCheck->setEnabled(false);
        m_hwAccelCheck->setChecked(false);
    }

    updateCommandPreview();
}

void ConvertPage::savePreset()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, trId("convert.preset"),
                                               trId("convert.presetName") + QStringLiteral(":"),
                                               QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    QVariantMap values;
    values.insert(QStringLiteral("format"), m_formatCombo->currentText());
    values.insert(QStringLiteral("videoCodec"), m_codecCombo->currentText());
    values.insert(QStringLiteral("audioCodec"), m_audioCodecCombo->currentText());
    values.insert(QStringLiteral("rateControl"), m_rateControlCombo->currentIndex());
    values.insert(QStringLiteral("targetSizeMB"), m_targetSizeSpin->value());
    values.insert(QStringLiteral("crf"), m_qualitySpin->value());
    values.insert(QStringLiteral("smartCopy"), m_smartCopyCheck->isChecked());
    values.insert(QStringLiteral("preserveAllStreams"), m_preserveAllStreamsCheck->isChecked());
    values.insert(QStringLiteral("stripMetadata"), m_stripMetaCheck->isChecked());
    values.insert(QStringLiteral("hwAccel"), m_hwAccelCheck->isChecked());
    values.insert(QStringLiteral("audioBitrate"), m_audioBitrateCombo->currentData().toString());
    values.insert(QStringLiteral("audioSamplerate"), m_audioSamplerateCombo->currentData().toString());
    values.insert(QStringLiteral("audioChannels"), m_audioChannelsCombo->currentData().toString());
    values.insert(QStringLiteral("audioNormalize"), m_audioNormalizeCheck->isChecked());
    values.insert(QStringLiteral("speedPreset"), m_speedPresetCombo->currentData().toString());
    values.insert(QStringLiteral("pixFmt"), m_pixFmtCombo->currentData().toString());
    values.insert(QStringLiteral("colorProfile"), m_colorSpaceCombo->currentData().toString());
    values.insert(QStringLiteral("hdrTonemap"), m_hdrTonemapCombo->currentData().toString());
    values.insert(QStringLiteral("videoFilter"), m_vfEdit->text());
    values.insert(QStringLiteral("audioFilter"), m_afEdit->text());
    m_presets->save(name, values);
    refreshPresetList();
    m_presetCombo->setCurrentText(name.trimmed());
}

void ConvertPage::loadPreset(const QString &name)
{
    if (name.isEmpty()) return;
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
    const int rcIdx = values.value(QStringLiteral("rateControl"), 0).toInt();
    m_rateControlCombo->setCurrentIndex(rcIdx);
    m_targetSizeSpin->setValue(values.value(QStringLiteral("targetSizeMB"), 25.0).toDouble());
    m_qualitySpin->setValue(values.value(QStringLiteral("crf"), 23).toInt());
    m_smartCopyCheck->setChecked(values.value(QStringLiteral("smartCopy"), false).toBool());
    m_preserveAllStreamsCheck->setChecked(values.value(QStringLiteral("preserveAllStreams"), true).toBool());
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
}

void ConvertPage::deletePreset()
{
    const QString name = m_presetCombo->currentText();
    if (name.isEmpty()) return;
    m_presets->remove(name);
    refreshPresetList();
}

void ConvertPage::refreshPresetList()
{
    const QString current = m_presetCombo->currentText();
    m_presetCombo->blockSignals(true);
    m_presetCombo->clear();
    m_presetCombo->addItems(m_presets->names());
    m_presetCombo->blockSignals(false);
    if (!current.isEmpty()) {
        m_presetCombo->setCurrentText(current);
    }
}

void ConvertPage::retranslateUi()
{
    m_titleLabel->setText(trId("convert.title"));
    if (m_inputCardTitle) {
        m_inputCardTitle->setText(trId("convert.card.input"));
    }
    if (m_codecCardTitle) {
        m_codecCardTitle->setText(trId("convert.card.encoding"));
    }
    if (m_previewCardTitle) {
        m_previewCardTitle->setText(trId("convert.card.preview"));
    }
    m_inputCaption->setText(trId("convert.input") + QStringLiteral(":"));
    m_recentCaption->setText(trId("common.recent") + QStringLiteral(":"));
    m_browseButton->setText(trId("common.browse"));
    m_presetCaption->setText(trId("convert.preset") + QStringLiteral(":"));
    m_presetSaveButton->setText(trId("common.save"));
    m_presetDeleteButton->setText(trId("common.delete"));
    m_formatCaption->setText(trId("convert.outputFormat") + QStringLiteral(":"));
    m_codecCaption->setText(trId("convert.codec") + QStringLiteral(":"));
    m_audioCodecCaption->setText(trId("convert.audioCodec") + QStringLiteral(":"));
    m_rateControlCaption->setText(trId("convert.rateControl") + QStringLiteral(":"));
    m_rateControlCombo->setItemText(0, trId("convert.rateControl.crf"));
    m_rateControlCombo->setItemText(1, trId("convert.rateControl.targetSize"));
    m_qualityCaption->setText(trId("convert.quality") + QStringLiteral(":"));
    m_targetSizeCaption->setText(trId("convert.targetSize") + QStringLiteral(":"));
    m_smartCopyCheck->setText(trId("common.smartCopy"));
    m_preserveAllStreamsCheck->setText(trId("convert.preserveAllStreams"));
    m_stripMetaCheck->setText(trId("metadata.remove"));
    m_hwAccelCheck->setText(trId("convert.hwAccel"));
    if (m_caps->hasHwAcceleration()) {
        m_hwDeviceLabel->setText(QStringLiteral("(%1)").arg(m_caps->primaryHwDeviceName()));
    } else {
        m_hwDeviceLabel->setText(QStringLiteral("(%1)").arg(trId("convert.hwAccel.none")));
    }
    m_metaCaption->setText(trId("metadata.title"));
    m_metaCaption->setVisible(!m_metaValue->text().isEmpty());

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
    m_pixFmtCaption->setText(trId("convert.adv.pixFmt") + QStringLiteral(":"));
    m_pixFmtCombo->setItemText(0, trId("common.auto"));
    m_colorSpaceCaption->setText(trId("convert.adv.colorSpace") + QStringLiteral(":"));
    m_colorSpaceCombo->setItemText(0, trId("common.auto"));
    m_hdrTonemapCaption->setText(trId("convert.adv.hdrTonemap") + QStringLiteral(":"));
    m_hdrTonemapCombo->setItemText(0, trId("common.disabled"));
    m_vfCaption->setText(trId("convert.adv.vf") + QStringLiteral(":"));
    m_afCaption->setText(trId("convert.adv.af") + QStringLiteral(":"));

    m_previewCaption->setText(trId("convert.preview") + QStringLiteral(":"));
    m_addButton->setText(trId("convert.addToQueue"));
    updateCommandPreview();
}
