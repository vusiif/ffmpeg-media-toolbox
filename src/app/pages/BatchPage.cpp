#include "BatchPage.h"

#include "i18n/TrId.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QMimeData>
#include <QSettings>
#include <QStringListModel>
#include <QUrl>
#include <QVBoxLayout>

#include "batch/BatchExportMatrix.h"
#include "batch/FolderWatcherService.h"

#include "ElaCheckBox.h"
#include "ElaComboBox.h"
#include "ElaLineEdit.h"
#include "ElaListView.h"
#include "ElaPushButton.h"
#include "ElaScrollArea.h"
#include "ElaScrollBar.h"
#include "ElaScrollPageArea.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "app/AppSettings.h"
#include "conversion/RemuxPolicy.h"
#include "ffmpeg/FFmpegCapabilities.h"
#include "ffmpeg/FFmpegCommandBuilder.h"
#include "ffmpeg/FFmpegLocator.h"
#include "jobs/Job.h"
#include "jobs/JobQueue.h"
#include "output/OutputNaming.h"
#include "pipeline/BatchPipeline.h"
#include "pipeline/PipelineManager.h"
#include "presets/PresetManager.h"

namespace {
QString defaultOutputDirectory()
{
    QSettings s(AppSettings::organization(), AppSettings::application());
    return s.value(QStringLiteral("output/directory")).toString();
}

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

BatchPage::BatchPage(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                     FFmpegCapabilities *caps, JobQueue *queue, QWidget *parent)
    : QWidget(parent)
    , m_locator(locator)
    , m_builder(builder)
    , m_caps(caps)
    , m_queue(queue)
{
    m_presets = new PresetManager(this);
    m_pipelineManager = new PipelineManager(this);
    m_watcherService = new FolderWatcherService(this);
    connect(m_watcherService, &FolderWatcherService::fileReady,
            this, &BatchPage::onWatcherFileReady);
    setAcceptDrops(true);
    m_outputDirectory = defaultOutputDirectory();

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(30, 30, 30, 30);
    root->setSpacing(16);

    // Page title
    m_titleLabel = new ElaText(this);
    m_titleLabel->setTextPixelSize(28);
    root->addWidget(m_titleLabel);

    // Two-column layout
    auto *body = new QHBoxLayout();
    body->setSpacing(20);

    // Left Column: File List Card
    auto *leftCard = new ElaScrollPageArea(this);
    leftCard->setMinimumHeight(0);
    leftCard->setMaximumHeight(QWIDGETSIZE_MAX);
    auto *leftLayout = new QVBoxLayout(leftCard);
    leftLayout->setContentsMargins(16, 16, 16, 16);
    leftLayout->setSpacing(10);

    auto *listTopRow = new QHBoxLayout();
    m_listHeader = new ElaText(QString(), 14, leftCard);
    m_listHeader->setIsWrapAnywhere(false);
    m_countLabel = new ElaText(QString(), 12, leftCard);
    listTopRow->addWidget(m_listHeader);
    listTopRow->addStretch();
    listTopRow->addWidget(m_countLabel);
    leftLayout->addLayout(listTopRow);

    m_fileView = new ElaListView(leftCard);
    m_fileView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_fileView->setItemHeight(32);
    m_fileView->setIsTransparent(true);
    m_fileModel = new QStringListModel(this);
    m_fileView->setModel(m_fileModel);
    leftLayout->addWidget(m_fileView, 1);

    auto *fileButtons = new QHBoxLayout();
    m_addButton = new ElaPushButton(leftCard);
    m_addFolderButton = new ElaPushButton(leftCard);
    m_removeButton = new ElaPushButton(leftCard);
    m_clearButton = new ElaPushButton(leftCard);
    connect(m_addButton, &ElaPushButton::clicked, this, &BatchPage::addFiles);
    connect(m_addFolderButton, &ElaPushButton::clicked, this, &BatchPage::addFolder);
    connect(m_removeButton, &ElaPushButton::clicked, this, &BatchPage::removeSelectedFiles);
    connect(m_clearButton, &ElaPushButton::clicked, this, &BatchPage::clearFiles);

    fileButtons->addWidget(m_addButton);
    fileButtons->addWidget(m_addFolderButton);
    fileButtons->addWidget(m_removeButton);
    fileButtons->addWidget(m_clearButton);
    fileButtons->addStretch();
    leftLayout->addLayout(fileButtons);

    m_preserveTreeCheck = new ElaCheckBox(trId("batch.preserveTree"), leftCard);
    m_preserveTreeCheck->setChecked(true);
    leftLayout->addWidget(m_preserveTreeCheck);

    body->addWidget(leftCard, 1);

    // Right Column: Settings inside ElaScrollArea
    auto *scroll = new ElaScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *scrollBody = new QWidget(scroll);
    scrollBody->setStyleSheet(QStringLiteral("background-color: transparent;"));
    auto *right = new QVBoxLayout(scrollBody);
    right->setContentsMargins(0, 0, 10, 0);
    right->setSpacing(12);

    // Card 1: 目标与预设
    auto *baseCard = new ElaScrollPageArea(scrollBody);
    baseCard->setMinimumHeight(0);
    baseCard->setMaximumHeight(QWIDGETSIZE_MAX);
    auto *baseForm = new QFormLayout(baseCard);
    baseForm->setContentsMargins(16, 16, 16, 16);
    baseForm->setSpacing(10);

    m_outputDirCaption = new ElaText(QString(), 13, baseCard);
    m_outputDirButton = new ElaPushButton(baseCard);
    if (!m_outputDirectory.isEmpty()) {
        m_outputDirButton->setText(m_outputDirectory);
    }
    connect(m_outputDirButton, &ElaPushButton::clicked, this, &BatchPage::browseOutputDir);
    baseForm->addRow(m_outputDirCaption, m_outputDirButton);

    m_presetCaption = new ElaText(QString(), 13, baseCard);
    m_presetCombo = new ElaComboBox(baseCard);
    connect(m_presets, &PresetManager::presetsChanged, this, &BatchPage::refreshPresetList);
    connect(m_presetCombo, &ElaComboBox::currentTextChanged, this, &BatchPage::loadPreset);
    baseForm->addRow(m_presetCaption, m_presetCombo);

    m_formatCaption = new ElaText(QString(), 13, baseCard);
    m_formatCombo = new ElaComboBox(baseCard);
    m_formatCombo->addItems({QStringLiteral("mp4"), QStringLiteral("mkv"), QStringLiteral("mov"),
                             QStringLiteral("webm"), QStringLiteral("avi"), QStringLiteral("mp3"),
                             QStringLiteral("aac"), QStringLiteral("flac"), QStringLiteral("wav")});
    baseForm->addRow(m_formatCaption, m_formatCombo);
    right->addWidget(baseCard);

    // Card 2: 编解码器与画质
    auto *codecCard = new ElaScrollPageArea(scrollBody);
    codecCard->setMinimumHeight(0);
    codecCard->setMaximumHeight(QWIDGETSIZE_MAX);
    auto *codecForm = new QFormLayout(codecCard);
    codecForm->setContentsMargins(16, 16, 16, 16);
    codecForm->setSpacing(10);

    m_codecCaption = new ElaText(QString(), 13, codecCard);
    m_codecCombo = new ElaComboBox(codecCard);
    codecForm->addRow(m_codecCaption, m_codecCombo);

    m_audioCodecCaption = new ElaText(QString(), 13, codecCard);
    m_audioCodecCombo = new ElaComboBox(codecCard);
    codecForm->addRow(m_audioCodecCaption, m_audioCodecCombo);

    m_qualityCaption = new ElaText(QString(), 13, codecCard);
    m_qualitySpin = new ElaSpinBox(codecCard);
    m_qualitySpin->setRange(0, 51);
    m_qualitySpin->setValue(23);
    codecForm->addRow(m_qualityCaption, m_qualitySpin);

    m_smartCopyCheck = new ElaCheckBox(codecCard);
    m_stripMetaCheck = new ElaCheckBox(codecCard);

    m_hwAccelCheck = new ElaCheckBox(codecCard);
    m_hwDeviceLabel = new ElaText(QString(), 12, codecCard);
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
    });

    codecForm->addRow(new ElaText(QString(), 13, codecCard), m_smartCopyCheck);
    codecForm->addRow(new ElaText(QString(), 13, codecCard), m_stripMetaCheck);
    codecForm->addRow(new ElaText(QString(), 13, codecCard), hwRow);
    right->addWidget(codecCard);

    // Card 3: 高级选项
    auto *advCard = new ElaScrollPageArea(scrollBody);
    advCard->setMinimumHeight(0);
    advCard->setMaximumHeight(QWIDGETSIZE_MAX);
    auto *advCardLayout = new QVBoxLayout(advCard);
    advCardLayout->setContentsMargins(16, 16, 16, 16);
    advCardLayout->setSpacing(8);

    m_advToggle = new ElaCheckBox(advCard);
    connect(m_advToggle, &ElaCheckBox::toggled, this, [this](bool checked) {
        m_advContainer->setVisible(checked);
    });
    advCardLayout->addWidget(m_advToggle);

    m_advContainer = new QWidget(advCard);
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
    m_hdrTonemapCombo->addItem(QStringLiteral("Mobius"), QStringLiteral("mobius"));
    m_hdrTonemapCombo->addItem(QStringLiteral("Hable"), QStringLiteral("hable"));
    m_hdrTonemapCombo->addItem(QStringLiteral("Reinhard"), QStringLiteral("reinhard"));
    advForm->addRow(m_hdrTonemapCaption, m_hdrTonemapCombo);

    m_vfCaption = new ElaText(QString(), 13, m_advContainer);
    m_vfEdit = new ElaLineEdit(m_advContainer);
    advForm->addRow(m_vfCaption, m_vfEdit);

    m_afCaption = new ElaText(QString(), 13, m_advContainer);
    m_afEdit = new ElaLineEdit(m_advContainer);
    advForm->addRow(m_afCaption, m_afEdit);

    m_advContainer->setVisible(false);
    advCardLayout->addWidget(m_advContainer);
    right->addWidget(advCard);

    // Card 4: 自定义多步处理流水线 (Batch Pipeline)
    auto *pipelineCard = new ElaScrollPageArea(scrollBody);
    pipelineCard->setMinimumHeight(0);
    pipelineCard->setMaximumHeight(QWIDGETSIZE_MAX);
    auto *pipelineCardLayout = new QVBoxLayout(pipelineCard);
    pipelineCardLayout->setContentsMargins(16, 16, 16, 16);
    pipelineCardLayout->setSpacing(10);

    m_pipelineToggle = new ElaCheckBox(pipelineCard);
    connect(m_pipelineToggle, &ElaCheckBox::toggled, this, [this](bool checked) {
        m_pipelineContainer->setVisible(checked);
    });
    pipelineCardLayout->addWidget(m_pipelineToggle);

    m_pipelineContainer = new QWidget(pipelineCard);
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

    connect(m_pipelineTemplateCombo, &ElaComboBox::currentTextChanged, this, &BatchPage::loadPipelineTemplate);
    connect(m_pipelineSaveBtn, &ElaPushButton::clicked, this, &BatchPage::savePipelineTemplate);
    connect(m_pipelineDeleteBtn, &ElaPushButton::clicked, this, &BatchPage::deletePipelineTemplate);
    connect(m_pipelineManager, &PipelineManager::templatesChanged, this, &BatchPage::refreshPipelineTemplates);

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
    connect(m_pipeWatermarkBrowseBtn, &ElaPushButton::clicked, this, [this]() {
        const QString file = QFileDialog::getOpenFileName(this, trId("batch.pipe.selectWatermark"), QString(),
                                                          QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp *.webp)"));
        if (!file.isEmpty()) m_pipeWatermarkPathEdit->setText(file);
    });
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
    right->addWidget(pipelineCard);

    // Card 5: 监视文件夹自动转码 (Folder Watcher)
    auto *watcherCard = new ElaScrollPageArea(scrollBody);
    auto *watcherCardLayout = new QVBoxLayout(watcherCard);
    watcherCardLayout->setContentsMargins(16, 16, 16, 16);
    watcherCardLayout->setSpacing(10);

    m_watcherToggle = new ElaCheckBox(trId("batch.watcher.title"), watcherCard);
    watcherCardLayout->addWidget(m_watcherToggle);

    m_watcherContainer = new QWidget(watcherCard);
    auto *watcherForm = new QFormLayout(m_watcherContainer);
    watcherForm->setContentsMargins(0, 8, 0, 0);
    watcherForm->setSpacing(10);

    auto *watchPathRow = new QHBoxLayout();
    m_watchPathEdit = new ElaLineEdit(m_watcherContainer);
    m_watchBrowseBtn = new ElaPushButton(trId("common.browse"), m_watcherContainer);
    connect(m_watchBrowseBtn, &ElaPushButton::clicked, this, [this]() {
        const QString dir = QFileDialog::getExistingDirectory(this, trId("batch.watcher.selectDir"));
        if (!dir.isEmpty()) {
            m_watchPathEdit->setText(dir);
        }
    });
    watchPathRow->addWidget(m_watchPathEdit, 1);
    watchPathRow->addWidget(m_watchBrowseBtn);
    watcherForm->addRow(new ElaText(trId("batch.watcher.watchDir") + QStringLiteral(":"), 13, m_watcherContainer), watchPathRow);

    auto *watchActionRow = new QHBoxLayout();
    m_watchActionBtn = new ElaPushButton(trId("batch.watcher.start"), m_watcherContainer);
    connect(m_watchActionBtn, &ElaPushButton::clicked, this, &BatchPage::toggleFolderWatcher);
    m_watchStatusLabel = new ElaText(trId("batch.watcher.status.idle"), 12, m_watcherContainer);
    watchActionRow->addWidget(m_watchActionBtn);
    watchActionRow->addSpacing(12);
    watchActionRow->addWidget(m_watchStatusLabel, 1);
    watcherForm->addRow(watchActionRow);

    m_watcherContainer->setVisible(false);
    watcherCardLayout->addWidget(m_watcherContainer);
    connect(m_watcherToggle, &ElaCheckBox::toggled, m_watcherContainer, &QWidget::setVisible);
    right->addWidget(watcherCard);

    // Card 6: 一源多出 (Multi-Target Matrix Card)
    auto *matrixCard = new ElaScrollPageArea(scrollBody);
    auto *matrixCardLayout = new QVBoxLayout(matrixCard);
    matrixCardLayout->setContentsMargins(16, 16, 16, 16);
    matrixCardLayout->setSpacing(10);

    m_matrixToggle = new ElaCheckBox(trId("batch.matrix.title"), matrixCard);
    matrixCardLayout->addWidget(m_matrixToggle);

    m_matrixContainer = new QWidget(matrixCard);
    auto *matrixLayout = new QVBoxLayout(m_matrixContainer);
    matrixLayout->setContentsMargins(0, 8, 0, 0);
    matrixLayout->setSpacing(6);

    m_target1Check = new ElaCheckBox(QStringLiteral("Web MP4 (1080p H.264/AAC, CRF 23)"), m_matrixContainer);
    m_target1Check->setChecked(true);
    m_target2Check = new ElaCheckBox(QStringLiteral("Mobile MP4 (720p H.264/AAC, CRF 24)"), m_matrixContainer);
    m_target3Check = new ElaCheckBox(QStringLiteral("High Efficiency (HEVC/H.265, CRF 26)"), m_matrixContainer);
    m_target4Check = new ElaCheckBox(QStringLiteral("Audio Extraction (MP3 320kbps)"), m_matrixContainer);

    matrixLayout->addWidget(m_target1Check);
    matrixLayout->addWidget(m_target2Check);
    matrixLayout->addWidget(m_target3Check);
    matrixLayout->addWidget(m_target4Check);

    m_matrixContainer->setVisible(false);
    matrixCardLayout->addWidget(m_matrixContainer);
    connect(m_matrixToggle, &ElaCheckBox::toggled, m_matrixContainer, &QWidget::setVisible);
    right->addWidget(matrixCard);

    right->addStretch();
    scroll->setWidget(scrollBody);
    body->addWidget(scroll, 1);

    root->addLayout(body, 1);

    // Bottom Action Row
    auto *buttonRow = new QHBoxLayout();
    buttonRow->addStretch();
    m_queueButton = new ElaPushButton(this);
    m_queueButton->setFixedHeight(38);
    m_queueButton->setMinimumWidth(160);
    connect(m_queueButton, &ElaPushButton::clicked, this, &BatchPage::addAllToQueue);
    buttonRow->addWidget(m_queueButton);
    root->addLayout(buttonRow);

    connect(m_caps, &FFmpegCapabilities::capabilitiesChanged,
            this, &BatchPage::rebuildCodecLists);

    rebuildCodecLists();
    refreshPresetList();
    refreshPipelineTemplates();
    retranslateUi();
}

void BatchPage::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void BatchPage::dropEvent(QDropEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    bool added = false;
    for (const QUrl &url : urls) {
        if (!url.isLocalFile()) continue;
        const QString path = url.toLocalFile();
        const QFileInfo fi(path);
        if (fi.isDir()) {
            const auto scanned = BatchExportMatrix::scanDirectory(path, true);
            for (const auto &item : scanned) {
                if (!m_inputFiles.contains(item.fullPath)) {
                    m_inputFiles.append(item.fullPath);
                    m_relativeSubDirs.insert(item.fullPath, item.relativeSubDir);
                    added = true;
                }
            }
        } else if (!m_inputFiles.contains(path)) {
            m_inputFiles.append(path);
            added = true;
        }
    }
    if (added) {
        syncFileListModel();
    }
    event->acceptProposedAction();
}

void BatchPage::addFiles()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this,
        trId("batch.selectFiles"),
        QString(),
        QStringLiteral("Media Files (*.mp4 *.mkv *.mov *.avi *.webm *.flv *.ts *.mp3 *.wav *.flac *.aac *.m4a *.opus *.ogg);;All Files (*.*)"));

    if (files.isEmpty()) return;

    bool added = false;
    for (const QString &f : files) {
        if (!m_inputFiles.contains(f)) {
            m_inputFiles.append(f);
            added = true;
        }
    }
    if (added) {
        syncFileListModel();
    }
}

void BatchPage::addFolder()
{
    const QString dir = QFileDialog::getExistingDirectory(this, trId("batch.selectFolder"), QString());
    if (dir.isEmpty()) return;

    const auto scanned = BatchExportMatrix::scanDirectory(dir, true);
    if (scanned.isEmpty()) return;

    bool added = false;
    for (const auto &item : scanned) {
        if (!m_inputFiles.contains(item.fullPath)) {
            m_inputFiles.append(item.fullPath);
            m_relativeSubDirs.insert(item.fullPath, item.relativeSubDir);
            added = true;
        }
    }
    if (added) {
        syncFileListModel();
    }
}

void BatchPage::toggleFolderWatcher()
{
    if (m_watcherService->isWatching()) {
        m_watcherService->stopWatching();
        m_watchActionBtn->setText(trId("batch.watcher.start"));
        m_watchStatusLabel->setText(trId("batch.watcher.status.idle"));
    } else {
        const QString watchPath = m_watchPathEdit->text().trimmed();
        if (watchPath.isEmpty() || !QDir(watchPath).exists()) {
            QMessageBox::warning(this, trId("batch.watcher.title"), trId("batch.watcher.invalidDir"));
            return;
        }
        m_watcherService->startWatching(watchPath, m_outputDirectory);
        m_watchActionBtn->setText(trId("batch.watcher.stop"));
        m_watchStatusLabel->setText(trId("batch.watcher.status.active").arg(watchPath).arg(0));
    }
}

void BatchPage::onWatcherFileReady(const QString &filePath, const QString &relativeSubDir)
{
    const QFileInfo info(filePath);
    QString outputPath;

    if (m_pipelineToggle->isChecked()) {
        const PipelineStep step = currentPipelineStep();
        BatchPipeline pipeline(QStringLiteral("BatchPipeline"), step);

        if (m_preserveTreeCheck->isChecked() && !relativeSubDir.isEmpty()) {
            outputPath = BatchExportMatrix::resolveOutputPath(m_outputDirectory, relativeSubDir,
                                                             info.completeBaseName(), step.outputFormat);
        } else {
            outputPath = OutputNaming::build(filePath, QStringLiteral("{name}.{format}"),
                                             step.outputFormat, m_outputDirectory);
        }

        const QStringList args = pipeline.buildCommand(filePath, outputPath, m_caps);
        auto *job = new Job();
        job->setName(info.fileName());
        job->setRelativeSubPath(relativeSubDir);
        job->setCommand(m_locator->ffmpegPath(), args);
        m_queue->addJob(job);
    } else {
        const QString format = m_formatCombo->currentText();
        if (m_preserveTreeCheck->isChecked() && !relativeSubDir.isEmpty()) {
            outputPath = BatchExportMatrix::resolveOutputPath(m_outputDirectory, relativeSubDir,
                                                             info.completeBaseName(), format);
        } else {
            outputPath = OutputNaming::build(filePath, QStringLiteral("{name}.{format}"),
                                             format, m_outputDirectory);
        }

        const bool copy = m_smartCopyCheck->isChecked() &&
                          RemuxPolicy::canCopyByExtension(filePath, format);
        const FFmpegCommandBuilder::ConversionOptions opts = currentOptions(filePath, outputPath, copy);
        const QStringList args = m_builder->buildConversion(opts);

        auto *job = new Job();
        job->setName(info.fileName());
        job->setRelativeSubPath(relativeSubDir);
        job->setCommand(m_locator->ffmpegPath(), args);
        m_queue->addJob(job);
    }

    m_watchStatusLabel->setText(trId("batch.watcher.status.active")
                                    .arg(m_watcherService->watchPath())
                                    .arg(m_watcherService->processedCount()));
}

void BatchPage::removeSelectedFiles()
{
    const QModelIndexList selected = m_fileView->selectionModel()->selectedIndexes();
    if (selected.isEmpty()) return;

    QList<int> rows;
    for (const QModelIndex &idx : selected) {
        rows.append(idx.row());
    }
    std::sort(rows.begin(), rows.end(), std::greater<int>());

    for (int row : rows) {
        if (row >= 0 && row < m_inputFiles.size()) {
            m_inputFiles.removeAt(row);
        }
    }
    syncFileListModel();
}

void BatchPage::clearFiles()
{
    if (m_inputFiles.isEmpty()) return;
    m_inputFiles.clear();
    syncFileListModel();
}

void BatchPage::syncFileListModel()
{
    m_fileModel->setStringList(m_inputFiles);
    updateCountLabel();
}

void BatchPage::browseOutputDir()
{
    const QString dir = QFileDialog::getExistingDirectory(this, trId("batch.selectOutputDir"), m_outputDirectory);
    if (!dir.isEmpty()) {
        m_outputDirectory = dir;
        m_outputDirButton->setText(dir);
    }
}

PipelineStep BatchPage::currentPipelineStep() const
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
    step.outputFormat = m_formatCombo->currentText();
    step.videoCodec = m_codecCombo->currentText();
    step.audioCodec = m_audioCodecCombo->currentText();
    step.crf = m_qualitySpin->value();
    step.audioBitrate = m_audioBitrateCombo->currentData().toString();
    step.speedPreset = m_speedPresetCombo->currentData().toString();
    step.hwAccel = m_hwAccelCheck->isChecked();
    step.preserveAllStreams = true;

    return step;
}

void BatchPage::applyPipelineStepToUi(const PipelineStep &step)
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

    // Base settings
    const auto setCombo = [](ElaComboBox *combo, const QString &val) {
        const int idx = combo->findText(val);
        if (idx >= 0) combo->setCurrentIndex(idx);
    };
    if (!step.outputFormat.isEmpty()) setCombo(m_formatCombo, step.outputFormat);
    if (!step.videoCodec.isEmpty()) setCombo(m_codecCombo, step.videoCodec);
    if (!step.audioCodec.isEmpty()) setCombo(m_audioCodecCombo, step.audioCodec);
    if (step.crf >= 0) m_qualitySpin->setValue(step.crf);
}

void BatchPage::refreshPipelineTemplates()
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

void BatchPage::loadPipelineTemplate(const QString &name)
{
    const bool isBuiltIn = m_pipelineManager->isBuiltIn(name);
    m_pipelineDeleteBtn->setEnabled(!isBuiltIn && !name.isEmpty());

    if (name.isEmpty()) return;
    const BatchPipeline pipeline = m_pipelineManager->load(name);
    applyPipelineStepToUi(pipeline.settings());
}

void BatchPage::savePipelineTemplate()
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

void BatchPage::deletePipelineTemplate()
{
    const QString name = m_pipelineTemplateCombo->currentText();
    if (name.isEmpty()) return;
    m_pipelineManager->remove(name);
    refreshPipelineTemplates();
}

FFmpegCommandBuilder::ConversionOptions BatchPage::currentOptions(
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

    const QString format = m_formatCombo->currentText();
    opts.videoCodec = m_codecCombo->currentText();
    opts.audioCodec = m_audioCodecCombo->currentText();
    opts.crf = m_qualitySpin->value();
    opts.stripMetadata = m_stripMetaCheck->isChecked();
    opts.preserveAllStreams = true;
    opts.subtitleCodec = RemuxPolicy::subtitleCodecForContainer(format).value_or(QString());

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
    opts.videoFilter = m_vfEdit->text().trimmed();
    opts.audioFilter = m_afEdit->text().trimmed();

    return opts;
}

void BatchPage::addAllToQueue()
{
    if (m_inputFiles.isEmpty()) return;

    // 1. Multi-Target Matrix Mode
    if (m_matrixToggle->isChecked()) {
        struct TargetConfig {
            QString name;
            QString format;
            QString vcodec;
            QString acodec;
            int crf;
            QString scale;
            QString suffix;
            bool isAudio;
        };

        QList<TargetConfig> activeTargets;
        if (m_target1Check->isChecked()) {
            activeTargets.append({QStringLiteral("1080p MP4"), QStringLiteral("mp4"), QStringLiteral("libx264"),
                                  QStringLiteral("aac"), 23, QStringLiteral("1920:-2"), QStringLiteral("_1080p"), false});
        }
        if (m_target2Check->isChecked()) {
            activeTargets.append({QStringLiteral("720p MP4"), QStringLiteral("mp4"), QStringLiteral("libx264"),
                                  QStringLiteral("aac"), 24, QStringLiteral("1280:-2"), QStringLiteral("_720p"), false});
        }
        if (m_target3Check->isChecked()) {
            activeTargets.append({QStringLiteral("HEVC"), QStringLiteral("mp4"), QStringLiteral("libx265"),
                                  QStringLiteral("aac"), 26, QString(), QStringLiteral("_hevc"), false});
        }
        if (m_target4Check->isChecked()) {
            activeTargets.append({QStringLiteral("MP3 Audio"), QStringLiteral("mp3"), QString(),
                                  QStringLiteral("libmp3lame"), 0, QString(), QStringLiteral("_audio"), true});
        }

        if (activeTargets.isEmpty()) {
            QMessageBox::warning(this, trId("batch.matrix.title"), trId("batch.matrix.noTargetSelected"));
            return;
        }

        for (const QString &inputFile : m_inputFiles) {
            const QFileInfo info(inputFile);
            const QString subDir = m_preserveTreeCheck->isChecked() ? m_relativeSubDirs.value(inputFile) : QString();

            for (const auto &tgt : activeTargets) {
                const QString outputPath = BatchExportMatrix::resolveOutputPath(m_outputDirectory, subDir,
                                                                                info.completeBaseName(),
                                                                                tgt.format, tgt.suffix);

                QStringList args;
                args << QStringLiteral("-hide_banner") << QStringLiteral("-y")
                     << QStringLiteral("-i") << inputFile;

                if (tgt.isAudio) {
                    args << QStringLiteral("-vn")
                         << QStringLiteral("-c:a") << tgt.acodec
                         << QStringLiteral("-b:a") << QStringLiteral("320k");
                } else {
                    args << QStringLiteral("-c:v") << tgt.vcodec
                         << QStringLiteral("-crf") << QString::number(tgt.crf);
                    if (!tgt.scale.isEmpty()) {
                        args << QStringLiteral("-vf") << QStringLiteral("scale=%1").arg(tgt.scale);
                    }
                    args << QStringLiteral("-c:a") << tgt.acodec;
                }
                args << outputPath;

                auto *job = new Job();
                job->setName(QStringLiteral("%1 (%2)").arg(info.fileName(), tgt.name));
                job->setRelativeSubPath(subDir);
                job->setCommand(m_locator->ffmpegPath(), args);
                m_queue->addJob(job);
            }
        }
        return;
    }

    // 2. Custom Pipeline Mode
    if (m_pipelineToggle->isChecked()) {
        const PipelineStep step = currentPipelineStep();
        BatchPipeline pipeline(QStringLiteral("BatchPipeline"), step);

        for (const QString &inputFile : m_inputFiles) {
            const QFileInfo info(inputFile);
            const QString subDir = m_preserveTreeCheck->isChecked() ? m_relativeSubDirs.value(inputFile) : QString();

            QString outputPath;
            if (!subDir.isEmpty()) {
                outputPath = BatchExportMatrix::resolveOutputPath(m_outputDirectory, subDir,
                                                                 info.completeBaseName(), step.outputFormat);
            } else {
                outputPath = OutputNaming::build(inputFile, QStringLiteral("{name}.{format}"),
                                                 step.outputFormat, m_outputDirectory);
            }

            const QStringList args = pipeline.buildCommand(inputFile, outputPath, m_caps);

            auto *job = new Job();
            job->setName(info.fileName());
            job->setRelativeSubPath(subDir);
            job->setCommand(m_locator->ffmpegPath(), args);
            m_queue->addJob(job);
        }
        return;
    }

    // 3. Standard Conversion Mode
    const QString format = m_formatCombo->currentText();
    for (const QString &inputFile : m_inputFiles) {
        const QFileInfo info(inputFile);
        const QString subDir = m_preserveTreeCheck->isChecked() ? m_relativeSubDirs.value(inputFile) : QString();

        QString outputPath;
        if (!subDir.isEmpty()) {
            outputPath = BatchExportMatrix::resolveOutputPath(m_outputDirectory, subDir,
                                                             info.completeBaseName(), format);
        } else {
            outputPath = OutputNaming::build(inputFile, QStringLiteral("{name}.{format}"),
                                             format, m_outputDirectory);
        }

        const bool copy = m_smartCopyCheck->isChecked() &&
                          RemuxPolicy::canCopyByExtension(inputFile, format);

        const FFmpegCommandBuilder::ConversionOptions opts = currentOptions(inputFile, outputPath, copy);
        const QStringList args = m_builder->buildConversion(opts);

        auto *job = new Job();
        job->setName(info.fileName());
        job->setRelativeSubPath(subDir);
        job->setCommand(m_locator->ffmpegPath(), args);
        m_queue->addJob(job);
    }
}

void BatchPage::rebuildCodecLists()
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
    m_audioCodecCombo->blockSignals(false);

    if (m_caps->hasHwAcceleration()) {
        m_hwDeviceLabel->setText(QStringLiteral("(%1)").arg(m_caps->primaryHwDeviceName()));
        m_hwAccelCheck->setEnabled(true);
    } else {
        m_hwDeviceLabel->setText(QStringLiteral("(%1)").arg(trId("convert.hwAccel.none")));
        m_hwAccelCheck->setEnabled(false);
        m_hwAccelCheck->setChecked(false);
    }
}

void BatchPage::refreshPresetList()
{
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

void BatchPage::loadPreset(const QString &name)
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
}

void BatchPage::updateCountLabel()
{
    m_countLabel->setText(trId("batch.count").arg(m_inputFiles.size()));
}

void BatchPage::retranslateUi()
{
    m_titleLabel->setText(trId("batch.title"));
    m_listHeader->setText(trId("batch.fileList"));
    m_addButton->setText(trId("batch.addFiles"));
    m_addFolderButton->setText(trId("batch.addFolder"));
    m_removeButton->setText(trId("batch.removeSelected"));
    m_clearButton->setText(trId("common.clear"));
    m_preserveTreeCheck->setText(trId("batch.preserveTree"));
    updateCountLabel();

    m_outputDirCaption->setText(trId("common.save") + QStringLiteral(":"));
    if (m_outputDirectory.isEmpty()) {
        m_outputDirButton->setText(trId("common.browse"));
    }
    m_presetCaption->setText(trId("convert.preset") + QStringLiteral(":"));
    refreshPresetList();
    m_formatCaption->setText(trId("convert.outputFormat") + QStringLiteral(":"));
    m_codecCaption->setText(trId("convert.codec") + QStringLiteral(":"));
    m_audioCodecCaption->setText(trId("convert.audioCodec") + QStringLiteral(":"));
    m_qualityCaption->setText(trId("convert.quality") + QStringLiteral(":"));
    m_smartCopyCheck->setText(trId("common.smartCopy"));
    m_stripMetaCheck->setText(trId("metadata.remove"));
    m_hwAccelCheck->setText(trId("convert.hwAccel"));
    if (m_caps->hasHwAcceleration()) {
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
    m_pixFmtCaption->setText(trId("convert.adv.pixFmt") + QStringLiteral(":"));
    m_pixFmtCombo->setItemText(0, trId("common.auto"));
    m_colorSpaceCaption->setText(trId("convert.adv.colorSpace") + QStringLiteral(":"));
    m_colorSpaceCombo->setItemText(0, trId("common.auto"));
    m_hdrTonemapCaption->setText(trId("convert.adv.hdrTonemap") + QStringLiteral(":"));
    m_hdrTonemapCombo->setItemText(0, trId("common.disabled"));
    m_vfCaption->setText(trId("convert.adv.vf") + QStringLiteral(":"));
    m_afCaption->setText(trId("convert.adv.af") + QStringLiteral(":"));

    // Pipeline UI retranslate
    m_pipelineToggle->setText(trId("batch.pipeline.toggle"));
    m_pipelineTemplateCaption->setText(trId("batch.pipeline.template") + QStringLiteral(":"));
    m_pipelineSaveBtn->setText(trId("common.save"));
    m_pipelineDeleteBtn->setText(trId("common.delete"));
    refreshPipelineTemplates();

    m_pipeTrimCheck->setText(trId("batch.pipeline.trim"));
    m_pipeCropCheck->setText(trId("batch.pipeline.crop"));
    m_pipeScaleCheck->setText(trId("batch.pipeline.scale"));
    m_pipeFpsCheck->setText(trId("batch.pipeline.fps"));
    m_pipeWatermarkCheck->setText(trId("batch.pipeline.watermark"));
    m_pipeWatermarkBrowseBtn->setText(trId("common.browse"));
    m_pipeAudioCheck->setText(trId("batch.pipeline.audio"));
    m_pipeAudioLoudnormCheck->setText(trId("batch.pipeline.loudnorm"));
    m_pipeAudioMuteCheck->setText(trId("batch.pipeline.mute"));

    // Watcher & Matrix retranslate
    m_watcherToggle->setText(trId("batch.watcher.title"));
    m_watchBrowseBtn->setText(trId("common.browse"));
    m_watchActionBtn->setText(m_watcherService->isWatching() ? trId("batch.watcher.stop")
                                                            : trId("batch.watcher.start"));
    m_matrixToggle->setText(trId("batch.matrix.title"));

    m_queueButton->setText(trId("convert.addToQueue"));
}
