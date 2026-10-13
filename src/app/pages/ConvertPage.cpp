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
#include "ElaLineEdit.h"
#include "ElaPushButton.h"
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

    auto *inputRow = new QHBoxLayout();
    m_inputCaption = new ElaText(QString(), 13, this);
    m_inputEdit = new ElaLineEdit(this);
    m_inputEdit->setReadOnly(true);
    m_browseButton = new ElaPushButton(this);
    connect(m_browseButton, &ElaPushButton::clicked, this, &ConvertPage::browseInput);
    inputRow->addWidget(m_inputCaption);
    inputRow->addWidget(m_inputEdit, 1);
    inputRow->addWidget(m_browseButton);
    root->addLayout(inputRow);

    auto *recentRow = new QHBoxLayout();
    m_recentCaption = new ElaText(QString(), 13, this);
    m_recentCombo = new ElaComboBox(this);
    m_recentCaption->setVisible(false);
    m_recentCombo->setVisible(false);
    recentRow->addWidget(m_recentCaption);
    recentRow->addWidget(m_recentCombo, 1);
    root->addLayout(recentRow);

    m_infoLabel = new ElaText(QString(), 13, this);
    m_infoLabel->setTextPixelSize(12);
    root->addWidget(m_infoLabel);

    m_metaCaption = new ElaText(QString(), 13, this);
    m_metaValue = new ElaText(QString(), 13, this);
    m_metaValue->setWordWrap(true);
    m_metaCaption->setVisible(false);
    m_metaValue->setVisible(false);
    root->addWidget(m_metaCaption);
    root->addWidget(m_metaValue);

    auto *presetRow = new QHBoxLayout();
    m_presetCaption = new ElaText(QString(), 13, this);
    m_presetCombo = new ElaComboBox(this);
    m_presetSaveButton = new ElaPushButton(this);
    m_presetDeleteButton = new ElaPushButton(this);
    connect(m_presetCombo, &ElaComboBox::currentTextChanged,
            this, &ConvertPage::loadPreset);
    connect(m_presetSaveButton, &ElaPushButton::clicked, this, &ConvertPage::savePreset);
    connect(m_presetDeleteButton, &ElaPushButton::clicked, this, &ConvertPage::deletePreset);
    presetRow->addWidget(m_presetCaption);
    presetRow->addWidget(m_presetCombo, 1);
    presetRow->addWidget(m_presetSaveButton);
    presetRow->addWidget(m_presetDeleteButton);
    root->addLayout(presetRow);

    auto *form = new QFormLayout();
    form->setSpacing(10);

    m_formatCaption = new ElaText(QString(), 13, this);
    m_formatCombo = new ElaComboBox(this);
    m_formatCombo->addItems({QStringLiteral("mp4"), QStringLiteral("mkv"), QStringLiteral("mov"),
                             QStringLiteral("webm"), QStringLiteral("avi"), QStringLiteral("mp3"),
                             QStringLiteral("aac"), QStringLiteral("flac"), QStringLiteral("wav")});

    m_codecCaption = new ElaText(QString(), 13, this);
    m_codecCombo = new ElaComboBox(this);

    m_audioCodecCaption = new ElaText(QString(), 13, this);
    m_audioCodecCombo = new ElaComboBox(this);

    m_qualityCaption = new ElaText(QString(), 13, this);
    m_qualitySpin = new ElaSpinBox(this);
    m_qualitySpin->setRange(0, 51);
    m_qualitySpin->setValue(23);

    m_smartCopyCheck = new ElaCheckBox(this);

    m_stripMetaCheck = new ElaCheckBox(this);

    form->addRow(m_formatCaption, m_formatCombo);
    form->addRow(m_codecCaption, m_codecCombo);
    form->addRow(m_audioCodecCaption, m_audioCodecCombo);
    form->addRow(m_qualityCaption, m_qualitySpin);
    form->addRow(new ElaText(QString(), 13, this), m_smartCopyCheck);
    form->addRow(new ElaText(QString(), 13, this), m_stripMetaCheck);

    m_advToggle = new ElaCheckBox(this);
    form->addRow(new ElaText(QString(), 13, this), m_advToggle);
    root->addLayout(form);

    m_advContainer = new QWidget(this);
    auto *advForm = new QFormLayout(m_advContainer);
    advForm->setContentsMargins(0, 0, 0, 0);
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
    connect(m_advToggle, &ElaCheckBox::toggled, m_advContainer, &QWidget::setVisible);
    root->addWidget(m_advContainer);

    m_estLabel = new ElaText(QString(), 13, this);
    m_estLabel->setVisible(false);
    root->addWidget(m_estLabel);

    m_previewCaption = new ElaText(QString(), 13, this);
    m_previewEdit = new ElaLineEdit(this);
    m_previewEdit->setReadOnly(true);
    root->addWidget(m_previewCaption);
    root->addWidget(m_previewEdit);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->addStretch();
    m_addButton = new ElaPushButton(this);
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
    connect(m_smartCopyCheck, &ElaCheckBox::toggled, this, &ConvertPage::updateCommandPreview);
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
    }
    if (info.hasAudio()) {
        parts << info.primaryAudio().codec;
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
    opts.crf = m_qualitySpin->value();
    opts.copyStreams = copy;
    opts.stripMetadata = m_stripMetaCheck->isChecked();

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
    m_audioCodecCombo->blockSignals(false);

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
    values.insert(QStringLiteral("crf"), m_qualitySpin->value());
    values.insert(QStringLiteral("smartCopy"), m_smartCopyCheck->isChecked());
    values.insert(QStringLiteral("stripMetadata"), m_stripMetaCheck->isChecked());
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
    m_qualitySpin->setValue(values.value(QStringLiteral("crf"), 23).toInt());
    m_smartCopyCheck->setChecked(values.value(QStringLiteral("smartCopy"), false).toBool());
    m_stripMetaCheck->setChecked(values.value(QStringLiteral("stripMetadata"), false).toBool());
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
    m_inputCaption->setText(trId("convert.input") + QStringLiteral(":"));
    m_recentCaption->setText(trId("common.recent") + QStringLiteral(":"));
    m_browseButton->setText(trId("common.browse"));
    m_presetCaption->setText(trId("convert.preset") + QStringLiteral(":"));
    m_presetSaveButton->setText(trId("common.save"));
    m_presetDeleteButton->setText(trId("common.delete"));
    m_formatCaption->setText(trId("convert.outputFormat") + QStringLiteral(":"));
    m_codecCaption->setText(trId("convert.codec") + QStringLiteral(":"));
    m_audioCodecCaption->setText(trId("convert.audioCodec") + QStringLiteral(":"));
    m_qualityCaption->setText(trId("convert.quality") + QStringLiteral(":"));
    m_smartCopyCheck->setText(trId("common.smartCopy"));
    m_stripMetaCheck->setText(trId("metadata.remove"));
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
