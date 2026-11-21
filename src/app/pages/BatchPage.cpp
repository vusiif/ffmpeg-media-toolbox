#include "BatchPage.h"

#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QVBoxLayout>

#include "batch/BatchExportMatrix.h"
#include "batch/BatchFileListCard.h"
#include "batch/BatchMatrixCard.h"
#include "batch/BatchOptionsCard.h"
#include "batch/BatchPipelineCard.h"
#include "ElaPushButton.h"
#include "ElaScrollArea.h"
#include "ElaText.h"
#include "ffmpeg/FFmpegCapabilities.h"
#include "ffmpeg/FFmpegCommandBuilder.h"
#include "ffmpeg/FFmpegLocator.h"
#include "conversion/RemuxPolicy.h"
#include "i18n/TrId.h"
#include "jobs/Job.h"
#include "jobs/JobQueue.h"
#include "output/OutputNaming.h"
#include "pipeline/BatchPipeline.h"
#include "presets/PresetManager.h"

BatchPage::BatchPage(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                     FFmpegCapabilities *caps, JobQueue *queue,
                     QWidget *parent)
    : QWidget(parent)
    , m_locator(locator)
    , m_builder(builder)
    , m_caps(caps)
    , m_queue(queue)
    , m_presets(new PresetManager(this))
{
    setAcceptDrops(true);

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

    // Left Column: File List Card (handles list, watcher, output dir)
    m_fileCard = new BatchFileListCard(this);
    body->addWidget(m_fileCard, 1);

    // Right Column: Options, Pipeline, and Matrix inside ElaScrollArea
    auto *scroll = new ElaScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setIsGrabGesture(false);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *scrollBody = new QWidget(scroll);
    scrollBody->setAttribute(Qt::WA_TranslucentBackground);
    auto *right = new QVBoxLayout(scrollBody);
    right->setContentsMargins(0, 0, 10, 0);
    right->setSpacing(12);

    m_optionsCard = new BatchOptionsCard(m_caps, m_presets, scrollBody);
    right->addWidget(m_optionsCard);

    m_pipelineCard = new BatchPipelineCard(scrollBody);
    right->addWidget(m_pipelineCard);

    m_matrixCard = new BatchMatrixCard(scrollBody);
    right->addWidget(m_matrixCard);

    right->addStretch();
    scroll->setWidget(scrollBody);
    body->addWidget(scroll, 1);

    root->addLayout(body, 1);

    // Bottom Action: Add all to queue
    m_queueButton = new ElaPushButton(this);
    m_queueButton->setFixedHeight(42);
    m_queueButton->setBorderRadius(6);
    m_queueButton->setLightDefaultColor(QColor(0, 120, 212));
    m_queueButton->setDarkDefaultColor(QColor(0, 120, 212));
    m_queueButton->setLightHoverColor(QColor(16, 137, 245));
    m_queueButton->setDarkHoverColor(QColor(16, 137, 245));
    m_queueButton->setLightPressColor(QColor(0, 90, 165));
    m_queueButton->setDarkPressColor(QColor(0, 90, 165));
    m_queueButton->setLightTextColor(Qt::white);
    m_queueButton->setDarkTextColor(Qt::white);
    QFont queueFont = m_queueButton->font();
    queueFont.setBold(true);
    queueFont.setPixelSize(14);
    m_queueButton->setFont(queueFont);
    root->addWidget(m_queueButton);

    connect(m_fileCard, &BatchFileListCard::fileReadyToProcess,
            this, &BatchPage::onWatcherFileReady);
    connect(m_fileCard, &BatchFileListCard::fileListChanged,
            this, &BatchPage::updateQueueButtonText);
    connect(m_pipelineCard, &BatchPipelineCard::baseSettingsChanged,
            m_optionsCard, &BatchOptionsCard::setBaseSettings);
    connect(m_queueButton, &ElaPushButton::clicked,
            this, &BatchPage::addAllToQueue);

    retranslateUi();
}

void BatchPage::addFiles(const QStringList &files)
{
    m_fileCard->addFiles(files);
}

void BatchPage::addFolder(const QString &folder)
{
    m_fileCard->addFolder(folder);
}

void BatchPage::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        m_isDragHover = true;
        event->acceptProposedAction();
        update();
    }
}

void BatchPage::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event)
    m_isDragHover = false;
    update();
}

void BatchPage::dropEvent(QDropEvent *event)
{
    m_isDragHover = false;
    update();

    QStringList files;
    for (const QUrl &url : event->mimeData()->urls()) {
        if (!url.isLocalFile()) continue;
        const QString path = url.toLocalFile();
        const QFileInfo fi(path);
        if (fi.isDir()) {
            m_fileCard->addFolder(path);
        } else {
            files.append(path);
        }
    }
    if (!files.isEmpty()) {
        m_fileCard->addFiles(files);
    }
}

void BatchPage::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    if (m_isDragHover) {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor(0, 120, 212, 220), 2, Qt::DashLine));
        p.setBrush(QColor(0, 120, 212, 20));
        p.drawRoundedRect(rect().adjusted(10, 10, -10, -10), 10, 10);
    }
}

void BatchPage::onWatcherFileReady(const QString &filePath, const QString &relativeSubDir)
{
    const QFileInfo info(filePath);
    QString outputPath;
    const QString outputDirectory = m_fileCard->outputDirectory();
    const bool preserveTree = m_fileCard->preserveTree();

    if (m_pipelineCard->isPipelineEnabled()) {
        const PipelineStep step = m_pipelineCard->currentPipelineStep(
            m_optionsCard->format(), m_optionsCard->videoCodec(),
            m_optionsCard->audioCodec(), m_optionsCard->crf(),
            m_optionsCard->audioBitrate(), m_optionsCard->speedPreset(),
            m_optionsCard->isHwAccel());
        BatchPipeline pipeline(QStringLiteral("BatchPipeline"), step);

        if (preserveTree && !relativeSubDir.isEmpty()) {
            outputPath = BatchExportMatrix::resolveOutputPath(outputDirectory, relativeSubDir,
                                                             info.completeBaseName(), step.outputFormat);
        } else {
            outputPath = OutputNaming::build(filePath, QStringLiteral("{name}.{format}"),
                                             step.outputFormat, outputDirectory);
        }

        const QStringList args = pipeline.buildCommand(filePath, outputPath, m_caps);
        auto *job = new Job();
        job->setName(info.fileName());
        job->setRelativeSubPath(relativeSubDir);
        job->setCommand(m_locator->ffmpegPath(), args);
        m_queue->addJob(job);
    } else {
        const QString format = m_optionsCard->format();
        if (preserveTree && !relativeSubDir.isEmpty()) {
            outputPath = BatchExportMatrix::resolveOutputPath(outputDirectory, relativeSubDir,
                                                             info.completeBaseName(), format);
        } else {
            outputPath = OutputNaming::build(filePath, QStringLiteral("{name}.{format}"),
                                             format, outputDirectory);
        }

        const bool copy = m_optionsCard->isSmartCopy() &&
                          RemuxPolicy::canCopyByExtension(filePath, format);
        const FFmpegCommandBuilder::ConversionOptions opts = m_optionsCard->currentOptions(filePath, outputPath, copy);
        const QStringList args = m_builder->buildConversion(opts);

        auto *job = new Job();
        job->setName(info.fileName());
        job->setRelativeSubPath(relativeSubDir);
        job->setCommand(m_locator->ffmpegPath(), args);
        m_queue->addJob(job);
    }
}

void BatchPage::addAllToQueue()
{
    const QStringList files = m_fileCard->inputFiles();
    if (files.isEmpty()) return;

    const QString outputDirectory = m_fileCard->outputDirectory();
    const bool preserveTree = m_fileCard->preserveTree();
    const QMap<QString, QString> relativeSubDirs = m_fileCard->relativeSubDirs();

    // 1. Multi-Target Matrix Mode
    if (m_matrixCard->isMatrixEnabled()) {
        const auto activeTargets = m_matrixCard->activeTargets();
        if (activeTargets.isEmpty()) {
            QMessageBox::warning(this, trId("batch.matrix.title"), trId("batch.matrix.noTargetSelected"));
            return;
        }

        QList<Job*> matrixJobs;
        for (const QString &inputFile : files) {
            const QFileInfo info(inputFile);
            const QString subDir = preserveTree ? relativeSubDirs.value(inputFile) : QString();

            for (const auto &tgt : activeTargets) {
                const QString outputPath = BatchExportMatrix::resolveOutputPath(outputDirectory, subDir,
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
                    if (!tgt.scale.isEmpty()) {
                        args << QStringLiteral("-vf") << QStringLiteral("scale=%1").arg(tgt.scale);
                    }
                    args << QStringLiteral("-c:v") << tgt.vcodec
                         << QStringLiteral("-crf") << QString::number(tgt.crf)
                         << QStringLiteral("-c:a") << tgt.acodec
                         << QStringLiteral("-b:a") << QStringLiteral("192k");
                }
                args << outputPath;

                auto *job = new Job();
                job->setName(QStringLiteral("[%1] %2").arg(tgt.name, info.fileName()));
                job->setRelativeSubPath(subDir);
                job->setCommand(m_locator->ffmpegPath(), args);
                matrixJobs.append(job);
            }
        }
        m_queue->addJobs(matrixJobs);
        return;
    }

    // 2. Custom Pipeline Mode
    if (m_pipelineCard->isPipelineEnabled()) {
        const PipelineStep step = m_pipelineCard->currentPipelineStep(
            m_optionsCard->format(), m_optionsCard->videoCodec(),
            m_optionsCard->audioCodec(), m_optionsCard->crf(),
            m_optionsCard->audioBitrate(), m_optionsCard->speedPreset(),
            m_optionsCard->isHwAccel());
        BatchPipeline pipeline(QStringLiteral("BatchPipeline"), step);
        QList<Job*> pipeJobs;

        for (const QString &inputFile : files) {
            const QFileInfo info(inputFile);
            const QString subDir = preserveTree ? relativeSubDirs.value(inputFile) : QString();

            QString outputPath;
            if (!subDir.isEmpty()) {
                outputPath = BatchExportMatrix::resolveOutputPath(outputDirectory, subDir,
                                                                 info.completeBaseName(), step.outputFormat);
            } else {
                outputPath = OutputNaming::build(inputFile, QStringLiteral("{name}.{format}"),
                                                 step.outputFormat, outputDirectory);
            }

            const QStringList args = pipeline.buildCommand(inputFile, outputPath, m_caps);
            auto *job = new Job();
            job->setName(info.fileName());
            job->setRelativeSubPath(subDir);
            job->setCommand(m_locator->ffmpegPath(), args);
            pipeJobs.append(job);
        }
        m_queue->addJobs(pipeJobs);
        return;
    }

    // 3. Standard Conversion Mode
    const QString format = m_optionsCard->format();
    QList<Job*> stdJobs;
    for (const QString &inputFile : files) {
        const QFileInfo info(inputFile);
        const QString subDir = preserveTree ? relativeSubDirs.value(inputFile) : QString();

        QString outputPath;
        if (!subDir.isEmpty()) {
            outputPath = BatchExportMatrix::resolveOutputPath(outputDirectory, subDir,
                                                             info.completeBaseName(), format);
        } else {
            outputPath = OutputNaming::build(inputFile, QStringLiteral("{name}.{format}"),
                                             format, outputDirectory);
        }

        const bool copy = m_optionsCard->isSmartCopy() &&
                          RemuxPolicy::canCopyByExtension(inputFile, format);

        const FFmpegCommandBuilder::ConversionOptions opts = m_optionsCard->currentOptions(inputFile, outputPath, copy);
        const QStringList args = m_builder->buildConversion(opts);

        auto *job = new Job();
        job->setName(info.fileName());
        job->setRelativeSubPath(subDir);
        job->setCommand(m_locator->ffmpegPath(), args);
        stdJobs.append(job);
    }
    m_queue->addJobs(stdJobs);
}

void BatchPage::updateQueueButtonText()
{
    const int count = m_fileCard ? m_fileCard->inputFiles().count() : 0;
    if (count > 0) {
        m_queueButton->setText(QStringLiteral("%1 (%2)").arg(trId("convert.addToQueue"), QString::number(count)));
    } else {
        m_queueButton->setText(trId("convert.addToQueue"));
    }
}

void BatchPage::retranslateUi()
{
    m_titleLabel->setText(trId("batch.title"));
    m_fileCard->retranslateUi();
    m_optionsCard->retranslateUi();
    m_pipelineCard->retranslateUi();
    m_matrixCard->retranslateUi();
    updateQueueButtonText();
}
