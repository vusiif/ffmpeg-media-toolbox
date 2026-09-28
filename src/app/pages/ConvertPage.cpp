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
#include <QRegularExpression>
#include <QSettings>
#include <QUrl>
#include <QVBoxLayout>

#include "ElaCheckBox.h"
#include "ElaComboBox.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "conversion/RemuxPolicy.h"
#include "ffmpeg/FFmpegCommandBuilder.h"
#include "ffmpeg/FFmpegLocator.h"
#include "i18n/TrId.h"
#include "jobs/Job.h"
#include "jobs/JobQueue.h"
#include "media/MediaProbeService.h"
#include "output/OutputNaming.h"
#include "presets/PresetManager.h"

namespace {
QStringList buildArgs(FFmpegCommandBuilder *builder, const QString &input, const QString &output,
                      const QString &format, const QString &vCodec, const QString &aCodec,
                      int crf, bool copy)
{
    return builder->buildConversion(input, output, format, vCodec, aCodec, crf, copy);
}

QString defaultNamingTemplate()
{
    QSettings s(QStringLiteral("FFMedia"), QStringLiteral("FFmpeg Media Toolbox"));
    return s.value(QStringLiteral("output/template"), QStringLiteral("{name}.{format}")).toString();
}

QString defaultOutputDirectory()
{
    QSettings s(QStringLiteral("FFMedia"), QStringLiteral("FFmpeg Media Toolbox"));
    return s.value(QStringLiteral("output/directory")).toString();
}
} // namespace

ConvertPage::ConvertPage(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                         MediaProbeService *probe, JobQueue *queue,
                         QWidget *parent)
    : QWidget(parent)
    , m_locator(locator)
    , m_builder(builder)
    , m_probe(probe)
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

    m_infoLabel = new ElaText(QString(), 13, this);
    m_infoLabel->setTextPixelSize(12);
    root->addWidget(m_infoLabel);

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
    m_codecCombo->addItems({QStringLiteral(""), QStringLiteral("copy"), QStringLiteral("libx264"),
                            QStringLiteral("libx265"), QStringLiteral("libvpx-vp9"),
                            QStringLiteral("h264_nvenc"), QStringLiteral("hevc_nvenc"),
                            QStringLiteral("h264_qsv"), QStringLiteral("hevc_qsv"),
                            QStringLiteral("h264_amf"), QStringLiteral("hevc_amf")});

    m_audioCodecCaption = new ElaText(QString(), 13, this);
    m_audioCodecCombo = new ElaComboBox(this);
    m_audioCodecCombo->addItems({QStringLiteral(""), QStringLiteral("copy"), QStringLiteral("aac"),
                                 QStringLiteral("libmp3lame"), QStringLiteral("flac"),
                                 QStringLiteral("libopus"), QStringLiteral("pcm_s16le")});

    m_qualityCaption = new ElaText(QString(), 13, this);
    m_qualitySpin = new ElaSpinBox(this);
    m_qualitySpin->setRange(0, 51);
    m_qualitySpin->setValue(23);

    m_smartCopyCheck = new ElaCheckBox(this);

    form->addRow(m_formatCaption, m_formatCombo);
    form->addRow(m_codecCaption, m_codecCombo);
    form->addRow(m_audioCodecCaption, m_audioCodecCombo);
    form->addRow(m_qualityCaption, m_qualitySpin);
    form->addRow(new ElaText(QString(), 13, this), m_smartCopyCheck);
    root->addLayout(form);

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
    startProbe(false);
}

void ConvertPage::startProbe(bool enqueueAfter)
{
    if (m_inputFile.isEmpty()) return;

    m_enqueueAfterProbe = enqueueAfter;
    m_probeInfo = MediaFile();
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
    updateCommandPreview();
}

void ConvertPage::enqueueJob(const MediaFile &info)
{
    if (!info.isValid()) return;

    const QString format = m_formatCombo->currentText();
    const QString outputPath = OutputNaming::build(m_inputFile, defaultNamingTemplate(),
                                                   format, defaultOutputDirectory());

    const bool copy = m_smartCopyCheck->isChecked() &&
                      RemuxPolicy::canCopyAllStreams(info, format);
    const QStringList args = buildArgs(m_builder, m_inputFile, outputPath,
                                       format, m_codecCombo->currentText(),
                                       m_audioCodecCombo->currentText(),
                                       m_qualitySpin->value(), copy);

    auto *job = new Job();
    job->setName(QFileInfo(m_inputFile).fileName());
    job->setCommand(m_locator->ffmpegPath(), args);
    job->setTotalDuration(info.duration);
    m_queue->addJob(job);
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
        return;
    }

    const QString format = m_formatCombo->currentText();
    const QString outputPath = OutputNaming::build(m_inputFile, defaultNamingTemplate(),
                                                   format, defaultOutputDirectory());
    const bool copy = m_smartCopyCheck->isChecked() &&
                      RemuxPolicy::canCopyAllStreams(m_probeInfo, format);
    const QStringList args = buildArgs(m_builder, m_inputFile, outputPath,
                                       format, m_codecCombo->currentText(),
                                       m_audioCodecCombo->currentText(),
                                       m_qualitySpin->value(), copy);
    m_previewEdit->setText(m_locator->ffmpegPath() + QStringLiteral(" ") + args.join(QLatin1Char(' ')));
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
    setCombo(m_formatCombo, values.value(QStringLiteral("format")).toString());
    setCombo(m_codecCombo, values.value(QStringLiteral("videoCodec")).toString());
    setCombo(m_audioCodecCombo, values.value(QStringLiteral("audioCodec")).toString());
    m_qualitySpin->setValue(values.value(QStringLiteral("crf"), 23).toInt());
    m_smartCopyCheck->setChecked(values.value(QStringLiteral("smartCopy"), false).toBool());
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
    m_browseButton->setText(trId("common.browse"));
    m_presetCaption->setText(trId("convert.preset") + QStringLiteral(":"));
    m_presetSaveButton->setText(trId("common.save"));
    m_presetDeleteButton->setText(trId("common.delete"));
    m_formatCaption->setText(trId("convert.outputFormat") + QStringLiteral(":"));
    m_codecCaption->setText(trId("convert.codec") + QStringLiteral(":"));
    m_audioCodecCaption->setText(trId("convert.audioCodec") + QStringLiteral(":"));
    m_qualityCaption->setText(trId("convert.quality") + QStringLiteral(":"));
    m_smartCopyCheck->setText(trId("common.smartCopy"));
    m_previewCaption->setText(trId("convert.preview") + QStringLiteral(":"));
    m_addButton->setText(trId("convert.addToQueue"));
    updateCommandPreview();
}
