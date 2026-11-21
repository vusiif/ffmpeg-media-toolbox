#include "ConvertPage.h"

#include <QDir>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMimeData>
#include <QPainter>
#include <QSettings>
#include <QStyleOption>
#include <QUuid>
#include <QVBoxLayout>

#include "ElaMessageBar.h"
#include "ElaPushButton.h"
#include "ElaScrollArea.h"
#include "ElaText.h"
#include "app/AppSettings.h"
#include "app/dialogs/ABCompareDialog.h"
#include "app/dialogs/FilterStudioDialog.h"
#include "app/dialogs/LosslessCutDialog.h"
#include "app/dialogs/VideoQualityDialog.h"
#include "conversion/RemuxPolicy.h"
#include "convert/ConvertAdvancedCard.h"
#include "convert/ConvertCodecCard.h"
#include "convert/ConvertPreviewCard.h"
#include "convert/ConvertSourceCard.h"
#include "ffmpeg/FFmpegCapabilities.h"
#include "ffmpeg/FFmpegCommandBuilder.h"
#include "ffmpeg/FFmpegLocator.h"
#include "i18n/TrId.h"
#include "jobs/Job.h"
#include "jobs/JobQueue.h"
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
    , m_queue(queue)
    , m_presets(new PresetManager(this))
{
    setAcceptDrops(true);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(30, 30, 30, 30);
    root->setSpacing(16);

    m_titleLabel = new ElaText(QString(), 13, this);
    m_titleLabel->setTextPixelSize(28);
    root->addWidget(m_titleLabel);

    m_scrollArea = new ElaScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setIsGrabGesture(false);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *scrollBody = new QWidget(m_scrollArea);
    scrollBody->setStyleSheet(QStringLiteral("background-color: transparent;"));
    auto *bodyLayout = new QVBoxLayout(scrollBody);
    bodyLayout->setContentsMargins(0, 0, 10, 0);
    bodyLayout->setSpacing(14);

    m_sourceCard = new ConvertSourceCard(m_probe, recentFiles, scrollBody);
    bodyLayout->addWidget(m_sourceCard);

    m_codecCard = new ConvertCodecCard(m_caps, m_presets, scrollBody);
    bodyLayout->addWidget(m_codecCard);

    m_advCard = new ConvertAdvancedCard(scrollBody);
    bodyLayout->addWidget(m_advCard);

    m_previewCard = new ConvertPreviewCard(scrollBody);
    bodyLayout->addWidget(m_previewCard);

    bodyLayout->addStretch();
    m_scrollArea->setWidget(scrollBody);
    root->addWidget(m_scrollArea, 1);

    // Bottom Action Row
    auto *buttonRow = new QHBoxLayout();
    m_sampleButton = new ElaPushButton(this);
    m_sampleButton->setFixedHeight(38);
    m_sampleButton->setMinimumWidth(120);
    connect(m_sampleButton, &ElaPushButton::clicked, this, &ConvertPage::enqueueSample);
    buttonRow->addWidget(m_sampleButton);

    m_abCompareButton = new ElaPushButton(this);
    m_abCompareButton->setFixedHeight(38);
    m_abCompareButton->setMinimumWidth(120);
    connect(m_abCompareButton, &ElaPushButton::clicked, this, &ConvertPage::openABCompare);
    buttonRow->addWidget(m_abCompareButton);

    m_qualityButton = new ElaPushButton(this);
    m_qualityButton->setFixedHeight(38);
    m_qualityButton->setMinimumWidth(120);
    connect(m_qualityButton, &ElaPushButton::clicked, this, &ConvertPage::openQualityDialog);
    buttonRow->addWidget(m_qualityButton);

    m_losslessCutButton = new ElaPushButton(this);
    m_losslessCutButton->setFixedHeight(38);
    m_losslessCutButton->setMinimumWidth(120);
    connect(m_losslessCutButton, &ElaPushButton::clicked, this, &ConvertPage::openLosslessCutDialog);
    buttonRow->addWidget(m_losslessCutButton);

    m_filterStudioButton = new ElaPushButton(this);
    m_filterStudioButton->setFixedHeight(38);
    m_filterStudioButton->setMinimumWidth(120);
    connect(m_filterStudioButton, &ElaPushButton::clicked, this, &ConvertPage::openFilterStudioDialog);
    buttonRow->addWidget(m_filterStudioButton);

    buttonRow->addStretch();
    m_addButton = new ElaPushButton(this);
    m_addButton->setFixedHeight(40);
    m_addButton->setMinimumWidth(180);
    m_addButton->setBorderRadius(6);
    m_addButton->setLightDefaultColor(QColor(0, 120, 212));
    m_addButton->setDarkDefaultColor(QColor(0, 120, 212));
    m_addButton->setLightHoverColor(QColor(16, 137, 245));
    m_addButton->setDarkHoverColor(QColor(16, 137, 245));
    m_addButton->setLightPressColor(QColor(0, 90, 165));
    m_addButton->setDarkPressColor(QColor(0, 90, 165));
    m_addButton->setLightTextColor(Qt::white);
    m_addButton->setDarkTextColor(Qt::white);
    QFont addBtnFont = m_addButton->font();
    addBtnFont.setBold(true);
    addBtnFont.setPixelSize(14);
    m_addButton->setFont(addBtnFont);
    connect(m_addButton, &ElaPushButton::clicked, this, &ConvertPage::addToQueue);
    buttonRow->addWidget(m_addButton);
    root->addLayout(buttonRow);

    // Signal/Slot Wiring
    connect(m_sourceCard, &ConvertSourceCard::inputFileChanged, this, &ConvertPage::onInputFileChanged);
    connect(m_sourceCard, &ConvertSourceCard::probeFinished, this, &ConvertPage::onProbeFinished);
    connect(m_codecCard, &ConvertCodecCard::optionsChanged, this, &ConvertPage::updateCommandPreview);
    connect(m_codecCard, &ConvertCodecCard::presetLoaded, this, &ConvertPage::onLoadPreset);
    connect(m_codecCard, &ConvertCodecCard::savePresetRequested, this, &ConvertPage::onSavePresetRequested);
    connect(m_advCard, &ConvertAdvancedCard::optionsChanged, this, &ConvertPage::updateCommandPreview);

    if (m_caps) {
        connect(m_caps, &FFmpegCapabilities::capabilitiesChanged,
                m_codecCard, &ConvertCodecCard::rebuildCodecLists);
    }

    retranslateUi();
}

void ConvertPage::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        m_isDragHover = true;
        event->acceptProposedAction();
        update();
    }
}

void ConvertPage::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event)
    m_isDragHover = false;
    update();
}

void ConvertPage::dropEvent(QDropEvent *event)
{
    m_isDragHover = false;
    update();
    const auto urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;
    const QString local = urls.first().toLocalFile();
    if (local.isEmpty()) return;
    m_sourceCard->setInputFile(local);
}

void ConvertPage::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    if (m_isDragHover) {
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor(0, 120, 212, 220), 2, Qt::DashLine));
        p.setBrush(QColor(0, 120, 212, 20));
        p.drawRoundedRect(rect().adjusted(10, 10, -10, -10), 10, 10);
    }
}

void ConvertPage::onInputFileChanged(const QString &path)
{
    m_inputFile = path;
    updateCommandPreview();
}

void ConvertPage::onProbeFinished(const MediaFile &info, bool enqueueAfter)
{
    m_probeInfo = info;
    if (info.hasVideo() && info.primaryVideo().isHdr) {
        m_advCard->enableHdrTonemapDefault();
    }

    if (enqueueAfter) {
        enqueueJob(info);
    } else {
        updateCommandPreview();
    }
}

FFmpegCommandBuilder::ConversionOptions ConvertPage::currentOptions(const QString &outputPath, bool copy) const
{
    FFmpegCommandBuilder::ConversionOptions opts;
    opts.inputPath = m_inputFile;
    opts.outputPath = outputPath;
    m_codecCard->applyTo(opts, copy, m_probeInfo.duration);
    m_advCard->applyTo(opts);
    return opts;
}

void ConvertPage::updateCommandPreview()
{
    if (m_inputFile.isEmpty()) {
        m_previewCard->setCommand(QString());
        m_previewCard->setEstimate(QString(), false);
        return;
    }

    const QString format = m_codecCard->format();
    const QString outputPath = OutputNaming::build(m_inputFile, defaultNamingTemplate(),
                                                   format, defaultOutputDirectory());
    const bool copy = m_codecCard->isSmartCopy() &&
                      RemuxPolicy::canCopyAllStreams(m_probeInfo, format);
    const QStringList args = m_builder->buildConversion(currentOptions(outputPath, copy));
    m_previewCard->setCommand(m_locator->ffmpegPath() + QStringLiteral(" ") + args.join(QLatin1Char(' ')));

    if (m_codecCard->rateControlIndex() == 2) {
        const qint64 estimate = static_cast<qint64>(m_codecCard->targetSizeMB() * 1024.0 * 1024.0);
        m_previewCard->setEstimate(trId("convert.estimate") + QStringLiteral(": ≈")
                                   + OutputEstimate::humanizeBytes(estimate), true);
    } else {
        const qint64 estimate = OutputEstimate::estimateBytes(
            m_probeInfo, m_codecCard->videoCodec(), m_codecCard->audioCodec(),
            m_codecCard->crf(), copy);
        if (estimate > 0) {
            m_previewCard->setEstimate(trId("convert.estimate") + QStringLiteral(": ≈")
                                       + OutputEstimate::humanizeBytes(estimate), true);
        } else {
            m_previewCard->setEstimate(QString(), false);
        }
    }
}

void ConvertPage::onSavePresetRequested()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, trId("convert.preset"),
                                               trId("convert.presetName") + QStringLiteral(":"),
                                               QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    QVariantMap values;
    m_codecCard->savePresetValues(values);
    m_advCard->savePresetValues(values);
    m_presets->save(name, values);
    m_codecCard->refreshPresetList();
}

void ConvertPage::onLoadPreset(const QString &name)
{
    if (name.isEmpty() || !m_presets) return;
    const QVariantMap values = m_presets->load(name);
    m_codecCard->loadPresetValues(values);
    m_advCard->loadPresetValues(values);
    updateCommandPreview();
}

void ConvertPage::enqueueJob(const MediaFile &info)
{
    if (!info.isValid() || !m_queue) return;

    const QString format = m_codecCard->format();
    const QString outputPath = OutputNaming::build(m_inputFile, defaultNamingTemplate(),
                                                   format, defaultOutputDirectory());
    const bool copy = m_codecCard->isSmartCopy() &&
                      RemuxPolicy::canCopyAllStreams(info, format);
    const auto opts = currentOptions(outputPath, copy);

    if (opts.twoPass) {
        const QString passLog = QDir::temp().filePath(QStringLiteral("ff_pass_%1").arg(QUuid::createUuid().toString(QUuid::Id128).left(8)));
        const auto passPair = m_builder->buildConversionTwoPass(opts, passLog);

        auto *job1 = new Job();
        job1->setName(QFileInfo(m_inputFile).fileName() + QStringLiteral(" [Pass 1/2]"));
        job1->setCommand(m_locator->ffmpegPath(), passPair.first);
        job1->setTotalDuration(info.duration);
        m_queue->addJob(job1);

        auto *job2 = new Job();
        job2->setName(QFileInfo(m_inputFile).fileName() + QStringLiteral(" [Pass 2/2]"));
        job2->setCommand(m_locator->ffmpegPath(), passPair.second);
        job2->setTotalDuration(info.duration);
        m_queue->addJob(job2);

        m_previewCard->setCommand(m_locator->ffmpegPath() + QStringLiteral(" ")
                                  + passPair.second.join(QLatin1Char(' ')));
    } else {
        const QStringList args = m_builder->buildConversion(opts);

        auto *job = new Job();
        job->setName(QFileInfo(m_inputFile).fileName());
        job->setCommand(m_locator->ffmpegPath(), args);
        job->setTotalDuration(info.duration);
        m_queue->addJob(job);

        m_previewCard->setCommand(m_locator->ffmpegPath() + QStringLiteral(" ")
                                  + args.join(QLatin1Char(' ')));
    }

    ElaMessageBar::success(ElaMessageBarType::TopRight,
                           trId("convert.dialog.title"),
                           trId("common.taskAdded") + QStringLiteral(": ") + QFileInfo(m_inputFile).fileName(),
                           2500, this);
}

void ConvertPage::enqueueSample()
{
    if (m_inputFile.isEmpty() || !m_probeInfo.isValid() || !m_queue) return;

    const QString format = m_codecCard->format();
    const QString outputPath = OutputNaming::build(m_inputFile, defaultNamingTemplate(),
                                                   format, defaultOutputDirectory());
    const bool copy = m_codecCard->isSmartCopy() &&
                      RemuxPolicy::canCopyAllStreams(m_probeInfo, format);
    auto opts = currentOptions(outputPath, copy);

    const double startTime = m_probeInfo.duration > 15.0 ? qMin(30.0, m_probeInfo.duration * 0.25) : 0.0;
    const QStringList args = m_builder->buildConversionSample(opts, startTime, 5.0);

    const QFileInfo fi(outputPath);
    m_lastSamplePath = fi.dir().filePath(QStringLiteral("%1_sample.%2").arg(fi.completeBaseName(), fi.suffix()));

    auto *job = new Job();
    job->setName(QFileInfo(m_inputFile).fileName() + QStringLiteral(" [5s Sample]"));
    job->setCommand(m_locator->ffmpegPath(), args);
    job->setTotalDuration(5.0);
    m_queue->addJob(job);
}

void ConvertPage::openABCompare()
{
    if (m_inputFile.isEmpty()) return;
    ABCompareDialog dlg(m_inputFile, m_lastSamplePath, m_probeInfo.duration, m_locator, this);
    dlg.exec();
}

void ConvertPage::openQualityDialog()
{
    if (m_inputFile.isEmpty()) return;
    VideoQualityDialog dlg(m_inputFile, m_lastSamplePath, m_locator, m_probe, this);
    dlg.exec();
}

void ConvertPage::openLosslessCutDialog()
{
    if (m_cutDialog) {
        if (!m_inputFile.isEmpty()) m_cutDialog->loadFile(m_inputFile);
        m_cutDialog->raise();
        m_cutDialog->activateWindow();
        return;
    }
    m_cutDialog = new LosslessCutDialog(m_locator, m_probe, m_queue, this);
    m_cutDialog->setAttribute(Qt::WA_DeleteOnClose);
    if (!m_inputFile.isEmpty()) {
        m_cutDialog->loadFile(m_inputFile);
    }
    m_cutDialog->show();
}

void ConvertPage::openFilterStudioDialog()
{
    if (m_filterDialog) {
        if (!m_inputFile.isEmpty()) m_filterDialog->loadFile(m_inputFile);
        m_filterDialog->raise();
        m_filterDialog->activateWindow();
        return;
    }
    m_filterDialog = new FilterStudioDialog(m_locator, m_probe, m_queue, this);
    m_filterDialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(m_filterDialog, &FilterStudioDialog::filtersApplied, this, [this](const QString &vf, const QString &af) {
        m_advCard->setCustomFilters(vf, af);
        updateCommandPreview();
    });
    if (!m_inputFile.isEmpty()) {
        m_filterDialog->loadFile(m_inputFile);
    }
    m_filterDialog->show();
}

void ConvertPage::addToQueue()
{
    if (m_inputFile.isEmpty()) return;
    m_sourceCard->startProbe(true);
}

void ConvertPage::retranslateUi()
{
    m_titleLabel->setText(trId("convert.title"));
    if (m_sourceCard) m_sourceCard->retranslateUi();
    if (m_codecCard) m_codecCard->retranslateUi();
    if (m_advCard) m_advCard->retranslateUi();
    if (m_previewCard) m_previewCard->retranslateUi();

    m_sampleButton->setText(trId("convert.sampleClip"));
    m_abCompareButton->setText(trId("convert.abCompare"));
    m_qualityButton->setText(trId("convert.qualityEval"));
    m_losslessCutButton->setText(trId("convert.losslessCut"));
    m_filterStudioButton->setText(trId("convert.filterStudio"));
    m_addButton->setText(trId("convert.addToQueue"));
    updateCommandPreview();
}
