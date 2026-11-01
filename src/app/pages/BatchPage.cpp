#include "BatchPage.h"

#include "i18n/TrId.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QMimeData>
#include <QSettings>
#include <QStringListModel>
#include <QUrl>
#include <QVBoxLayout>

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
#include "presets/PresetManager.h"

namespace {
QString defaultOutputDirectory()
{
    QSettings s(AppSettings::organization(), AppSettings::application());
    return s.value(QStringLiteral("output/directory")).toString();
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
    setAcceptDrops(true);
    m_outputDirectory = defaultOutputDirectory();

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(30, 30, 30, 30);
    root->setSpacing(16);

    m_titleLabel = new ElaText(QString(), 13, this);
    m_titleLabel->setTextPixelSize(28);
    root->addWidget(m_titleLabel);

    auto *body = new QHBoxLayout();
    body->setSpacing(16);

    // Left Column: File List Card
    auto *leftCard = new ElaScrollPageArea(this);
    leftCard->setMinimumHeight(0);
    leftCard->setMaximumHeight(QWIDGETSIZE_MAX);
    auto *leftLayout = new QVBoxLayout(leftCard);
    leftLayout->setContentsMargins(16, 16, 16, 16);
    leftLayout->setSpacing(10);

    auto *listTopRow = new QHBoxLayout();
    m_listHeader = new ElaText(QString(), 15, leftCard);
    m_listHeader->setStyleSheet(QStringLiteral("font-weight: bold;"));
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
    m_removeButton = new ElaPushButton(leftCard);
    m_clearButton = new ElaPushButton(leftCard);
    connect(m_addButton, &ElaPushButton::clicked, this, &BatchPage::addFiles);
    connect(m_removeButton, &ElaPushButton::clicked, this, &BatchPage::removeSelectedFiles);
    connect(m_clearButton, &ElaPushButton::clicked, this, &BatchPage::clearFiles);

    fileButtons->addWidget(m_addButton);
    fileButtons->addWidget(m_removeButton);
    fileButtons->addWidget(m_clearButton);
    fileButtons->addStretch();
    leftLayout->addLayout(fileButtons);

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

    right->addStretch();
    scroll->setWidget(scrollBody);
    body->addWidget(scroll, 1);

    root->addLayout(body, 1);

    // Bottom Action Row
    auto *buttonRow = new QHBoxLayout();
    buttonRow->addStretch();
    m_queueButton = new ElaPushButton(this);
    m_queueButton->setFixedHeight(38);
    m_queueButton->setMinimumWidth(140);
    connect(m_queueButton, &ElaPushButton::clicked, this, &BatchPage::addAllToQueue);
    buttonRow->addWidget(m_queueButton);
    root->addLayout(buttonRow);

    connect(m_caps, &FFmpegCapabilities::capabilitiesChanged,
            this, &BatchPage::rebuildCodecLists);
    rebuildCodecLists();
    refreshPresetList();

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
    for (const QUrl &url : urls) {
        const QString path = url.toLocalFile();
        if (!path.isEmpty() && !m_inputFiles.contains(path)) {
            m_inputFiles.append(path);
        }
    }
    syncFileListModel();
}

void BatchPage::syncFileListModel()
{
    QStringList displayNames;
    displayNames.reserve(m_inputFiles.size());
    for (const QString &path : m_inputFiles) {
        displayNames << QFileInfo(path).fileName();
    }
    m_fileModel->setStringList(displayNames);
    updateCountLabel();
}

void BatchPage::addFiles()
{
    const QStringList paths = QFileDialog::getOpenFileNames(
        this, trId("batch.title"), QString(), QStringLiteral("All files (*)"));
    for (const QString &path : paths) {
        if (!m_inputFiles.contains(path)) {
            m_inputFiles.append(path);
        }
    }
    syncFileListModel();
}

void BatchPage::removeSelectedFiles()
{
    const auto selected = m_fileView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QList<int> rows;
    for (const auto &index : selected) {
        rows.append(index.row());
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
    m_inputFiles.clear();
    syncFileListModel();
}

void BatchPage::browseOutputDir()
{
    const QString dir = QFileDialog::getExistingDirectory(this, trId("common.save"));
    if (dir.isEmpty()) return;
    m_outputDirectory = dir;
    m_outputDirButton->setText(dir);
}

FFmpegCommandBuilder::ConversionOptions BatchPage::currentOptions(
    const QString &inputFile, const QString &outputPath, bool copy) const
{
    FFmpegCommandBuilder::ConversionOptions opts;
    opts.inputPath = inputFile;
    opts.outputPath = outputPath;
    opts.videoCodec = m_codecCombo->currentText();
    opts.audioCodec = m_audioCodecCombo->currentText();
    opts.crf = m_qualitySpin->value();
    opts.copyStreams = copy;
    if (copy) {
        opts.preserveAllStreams = true;
    }
    opts.stripMetadata = m_stripMetaCheck->isChecked();
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

    const QString format = m_formatCombo->currentText();
    for (const QString &inputFile : m_inputFiles) {
        const QFileInfo info(inputFile);
        const QString outputPath = OutputNaming::build(inputFile,
                                                       QStringLiteral("{name}.{format}"),
                                                       format, m_outputDirectory);

        const bool copy = m_smartCopyCheck->isChecked() &&
                          RemuxPolicy::canCopyByExtension(inputFile, format);

        const FFmpegCommandBuilder::ConversionOptions opts = currentOptions(inputFile, outputPath, copy);
        const QStringList args = m_builder->buildConversion(opts);

        auto *job = new Job();
        job->setName(info.fileName());
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
    m_removeButton->setText(trId("batch.removeSelected"));
    m_clearButton->setText(trId("common.clear"));
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

    m_queueButton->setText(trId("convert.addToQueue"));
}
