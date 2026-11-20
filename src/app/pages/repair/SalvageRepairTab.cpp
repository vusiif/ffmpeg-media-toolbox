#include "SalvageRepairTab.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QVBoxLayout>

#include "ElaComboBox.h"
#include "ElaLineEdit.h"
#include "ElaMessageBar.h"
#include "ElaProgressBar.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaText.h"
#include "ffmpeg/FFmpegLocator.h"
#include "i18n/TrId.h"
#include "jobs/Job.h"
#include "jobs/JobQueue.h"
#include "media/MediaProbeService.h"

SalvageRepairTab::SalvageRepairTab(FFmpegLocator *ffmpegLocator,
                                   MediaProbeService *probeService,
                                   JobQueue *jobQueue,
                                   QWidget *parent)
    : RepairTabBase(ffmpegLocator, probeService, jobQueue, parent)
    , m_salvageEngine(new MediaSalvageEngine(ffmpegLocator, this))
{
    initUi();

    connect(m_salvageEngine, &MediaSalvageEngine::progressChanged,
            this, &SalvageRepairTab::onSalvageProgress);
    connect(m_salvageEngine, &MediaSalvageEngine::logMessage,
            this, &SalvageRepairTab::onSalvageLog);
    connect(m_salvageEngine, &MediaSalvageEngine::finished,
            this, &SalvageRepairTab::onSalvageFinished);
}

void SalvageRepairTab::initUi()
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 4, 0, 0);
    layout->setSpacing(14);

    // Left Column: Configuration & Execution Card
    m_salvageControlCard = createCard(this);
    auto *leftLayout = new QVBoxLayout(m_salvageControlCard);
    leftLayout->setContentsMargins(18, 16, 18, 16);
    leftLayout->setSpacing(12);

    m_salvageTitleLabel = new ElaText(QString(), 15, m_salvageControlCard);
    m_salvageTitleLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));
    m_salvageDescLabel = new ElaText(QString(), 12, m_salvageControlCard);
    m_salvageDescLabel->setWordWrap(true);
    leftLayout->addWidget(m_salvageTitleLabel);
    leftLayout->addWidget(m_salvageDescLabel);

    // Input row
    auto *inCol = new QVBoxLayout();
    inCol->setSpacing(6);
    m_salvageInputCaption = makeCaption(QString(), m_salvageControlCard);
    auto *inRow = new QHBoxLayout();
    m_salvageInputEdit = new ElaLineEdit(m_salvageControlCard);
    m_salvageInputEdit->setReadOnly(true);
    m_salvageBrowseBtn = createCardButton(QString(), 32, m_salvageControlCard);
    connect(m_salvageBrowseBtn, &ElaPushButton::clicked, this, &SalvageRepairTab::onBrowseSalvageInput);
    inRow->addWidget(m_salvageInputEdit, 1);
    inRow->addWidget(m_salvageBrowseBtn);
    inCol->addWidget(m_salvageInputCaption);
    inCol->addLayout(inRow);
    leftLayout->addLayout(inCol);

    // Assessment label
    m_salvageAssessLabel = new ElaText(QString(), 12, m_salvageControlCard);
    m_salvageAssessLabel->setWordWrap(true);
    m_salvageAssessLabel->setStyleSheet(QStringLiteral("color: #0078d4;"));
    m_salvageAssessLabel->setVisible(false);
    leftLayout->addWidget(m_salvageAssessLabel);

    // Mode combo row
    auto *modeCol = new QVBoxLayout();
    modeCol->setSpacing(6);
    m_salvageModeCaption = makeCaption(QString(), m_salvageControlCard);
    m_salvageModeCombo = new ElaComboBox(m_salvageControlCard);
    connect(m_salvageModeCombo, &ElaComboBox::currentIndexChanged, this, &SalvageRepairTab::onSalvageModeChanged);
    modeCol->addWidget(m_salvageModeCaption);
    modeCol->addWidget(m_salvageModeCombo);
    leftLayout->addLayout(modeCol);

    // Output row
    auto *outCol = new QVBoxLayout();
    outCol->setSpacing(6);
    m_salvageOutputCaption = makeCaption(QString(), m_salvageControlCard);
    auto *outRow = new QHBoxLayout();
    m_salvageOutputEdit = new ElaLineEdit(m_salvageControlCard);
    m_salvageOutputBrowseBtn = createCardButton(QString(), 32, m_salvageControlCard);
    connect(m_salvageOutputBrowseBtn, &ElaPushButton::clicked, this, &SalvageRepairTab::onBrowseSalvageOutput);
    outRow->addWidget(m_salvageOutputEdit, 1);
    outRow->addWidget(m_salvageOutputBrowseBtn);
    outCol->addWidget(m_salvageOutputCaption);
    outCol->addLayout(outRow);
    leftLayout->addLayout(outCol);

    // Action buttons
    auto *actionRow = new QHBoxLayout();
    m_startSalvageBtn = createCardButton(QString(), 36, m_salvageControlCard);
    m_startSalvageBtn->setMinimumWidth(130);
    m_startSalvageBtn->setEnabled(false);
    connect(m_startSalvageBtn, &ElaPushButton::clicked, this, &SalvageRepairTab::onStartSalvage);

    m_queueSalvageBtn = createCardButton(QString(), 36, m_salvageControlCard);
    m_queueSalvageBtn->setMinimumWidth(120);
    m_queueSalvageBtn->setEnabled(false);
    connect(m_queueSalvageBtn, &ElaPushButton::clicked, this, &SalvageRepairTab::onQueueSalvage);

    m_cancelSalvageBtn = createCardButton(QString(), 36, m_salvageControlCard);
    m_cancelSalvageBtn->setEnabled(false);
    connect(m_cancelSalvageBtn, &ElaPushButton::clicked, this, &SalvageRepairTab::onCancelSalvage);

    actionRow->addWidget(m_startSalvageBtn);
    actionRow->addWidget(m_queueSalvageBtn);
    actionRow->addWidget(m_cancelSalvageBtn);
    actionRow->addStretch();
    leftLayout->addLayout(actionRow);

    // Progress bar
    m_salvageProgressBar = new ElaProgressBar(m_salvageControlCard);
    m_salvageProgressBar->setRange(0, 100);
    m_salvageProgressBar->setValue(0);
    m_salvageProgressBar->setVisible(false);
    leftLayout->addWidget(m_salvageProgressBar);

    // Status label
    m_salvageStatusLabel = new ElaText(QString(), 12, m_salvageControlCard);
    m_salvageStatusLabel->setWordWrap(true);
    m_salvageStatusLabel->setVisible(false);
    leftLayout->addWidget(m_salvageStatusLabel);

    // Result action bar
    m_salvageResultBar = new QWidget(m_salvageControlCard);
    auto *resultLayout = new QVBoxLayout(m_salvageResultBar);
    resultLayout->setContentsMargins(0, 0, 0, 0);
    resultLayout->setSpacing(8);

    m_salvageResultLabel = new ElaText(QString(), 13, m_salvageResultBar);
    m_salvageResultLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #107c41;"));
    auto *resultBtnRow = new QHBoxLayout();
    m_salvageOpenFolderBtn = createCardButton(QString(), 32, m_salvageResultBar);
    connect(m_salvageOpenFolderBtn, &ElaPushButton::clicked, this, &SalvageRepairTab::onOpenSalvageFolder);

    m_salvagePlayBtn = createCardButton(QString(), 32, m_salvageResultBar);
    connect(m_salvagePlayBtn, &ElaPushButton::clicked, this, &SalvageRepairTab::onPlaySalvagedVideo);

    resultBtnRow->addWidget(m_salvageOpenFolderBtn);
    resultBtnRow->addWidget(m_salvagePlayBtn);
    resultBtnRow->addStretch();
    resultLayout->addWidget(m_salvageResultLabel);
    resultLayout->addLayout(resultBtnRow);
    m_salvageResultBar->setVisible(false);
    leftLayout->addWidget(m_salvageResultBar);

    leftLayout->addStretch();
    layout->addWidget(m_salvageControlCard, 1);

    // Right Column: Log Console Card
    m_salvageConsoleCard = createCard(this);
    auto *rightLayout = new QVBoxLayout(m_salvageConsoleCard);
    rightLayout->setContentsMargins(18, 16, 18, 16);
    rightLayout->setSpacing(12);

    auto *logHeader = new QHBoxLayout();
    m_salvageConsoleCaption = new ElaText(QString(), 15, m_salvageConsoleCard);
    m_salvageConsoleCaption->setStyleSheet(QStringLiteral("font-weight: bold;"));
    m_clearSalvageConsoleBtn = createCardButton(QString(), 28, m_salvageConsoleCard);
    connect(m_clearSalvageConsoleBtn, &ElaPushButton::clicked, this, [this]() {
        m_salvageConsole->clear();
    });
    logHeader->addWidget(m_salvageConsoleCaption);
    logHeader->addStretch();
    logHeader->addWidget(m_clearSalvageConsoleBtn);
    rightLayout->addLayout(logHeader);

    m_salvageConsole = new QPlainTextEdit(m_salvageConsoleCard);
    m_salvageConsole->setReadOnly(true);
    m_salvageConsole->setFont(QFont(QStringLiteral("Consolas"), 9));
    m_salvageConsole->setStyleSheet(QStringLiteral("background-color: #1e1e1e; color: #dcdcdc; border-radius: 6px; padding: 6px;"));
    rightLayout->addWidget(m_salvageConsole, 1);

    layout->addWidget(m_salvageConsoleCard, 1);
}

void SalvageRepairTab::handleDroppedFile(const QString &filePath)
{
    m_salvageInputPath = filePath;
    m_salvageInputEdit->setText(filePath);
    m_salvageDuration = 0.0;
    bool probeOk = false;
    if (m_probeService) {
        MediaFile info = m_probeService->probe(filePath);
        if (info.duration > 0.0) {
            m_salvageDuration = info.duration;
            probeOk = true;
        }
    }
    const auto mode = (m_salvageModeCombo->currentIndex() == 1)
                          ? MediaSalvageEngine::SalvageMode::ForceTranscode
                          : MediaSalvageEngine::SalvageMode::FastRemux;
    m_salvageOutputPath = MediaSalvageEngine::suggestOutputPath(filePath, mode);
    m_salvageOutputEdit->setText(m_salvageOutputPath);
    m_salvageAssessLabel->setVisible(true);
    if (!probeOk) {
        m_salvageAssessLabel->setText(trId("repair.salvage.assess.unprobed"));
        m_salvageAssessLabel->setStyleSheet(QStringLiteral("color: #d83b01; font-weight: bold;"));
        m_salvageModeCombo->setCurrentIndex(1);
    } else {
        m_salvageAssessLabel->setText(trId("repair.salvage.assess.probed"));
        m_salvageAssessLabel->setStyleSheet(QStringLiteral("color: #0078d4;"));
    }
    m_startSalvageBtn->setEnabled(true);
    m_queueSalvageBtn->setEnabled(true);
    m_salvageResultBar->setVisible(false);
}

void SalvageRepairTab::onBrowseSalvageInput()
{
    const QString path = QFileDialog::getOpenFileName(
        this, trId("repair.salvage.selectInput"), QString(),
        QStringLiteral("Media Files (*.mp4 *.mkv *.mov *.avi *.ts *.webm *.m4v *.flv);;All Files (*)"));
    if (path.isEmpty()) return;
    handleDroppedFile(path);
}

void SalvageRepairTab::onBrowseSalvageOutput()
{
    const QString filter = (m_salvageModeCombo->currentIndex() == 1)
                               ? QStringLiteral("MP4 Video (*.mp4)")
                               : QStringLiteral("Same Format (*.*);;MP4 Video (*.mp4)");
    const QString path = QFileDialog::getSaveFileName(
        this, trId("repair.salvage.output"), m_salvageOutputPath, filter);
    if (!path.isEmpty()) {
        m_salvageOutputPath = path;
        m_salvageOutputEdit->setText(path);
    }
}

void SalvageRepairTab::onSalvageModeChanged(int index)
{
    if (m_salvageInputPath.isEmpty()) return;
    const auto mode = (index == 1)
                          ? MediaSalvageEngine::SalvageMode::ForceTranscode
                          : MediaSalvageEngine::SalvageMode::FastRemux;
    m_salvageOutputPath = MediaSalvageEngine::suggestOutputPath(m_salvageInputPath, mode);
    m_salvageOutputEdit->setText(m_salvageOutputPath);
}

void SalvageRepairTab::onStartSalvage()
{
    if (m_salvageInputPath.isEmpty() || m_salvageOutputPath.isEmpty()) return;

    m_startSalvageBtn->setEnabled(false);
    m_queueSalvageBtn->setEnabled(false);
    m_cancelSalvageBtn->setEnabled(true);
    m_salvageProgressBar->setVisible(true);
    m_salvageProgressBar->setValue(0);
    m_salvageStatusLabel->setVisible(true);
    m_salvageStatusLabel->setText(trId("repair.salvage.starting"));
    m_salvageResultBar->setVisible(false);
    m_salvageConsole->clear();

    const auto mode = (m_salvageModeCombo->currentIndex() == 1)
                          ? MediaSalvageEngine::SalvageMode::ForceTranscode
                          : MediaSalvageEngine::SalvageMode::FastRemux;

    m_salvageEngine->start(m_salvageInputPath, m_salvageOutputPath, mode, m_salvageDuration);
}

void SalvageRepairTab::onQueueSalvage()
{
    if (m_salvageInputPath.isEmpty() || m_salvageOutputPath.isEmpty() || !m_jobQueue) return;

    const auto mode = (m_salvageModeCombo->currentIndex() == 1)
                          ? MediaSalvageEngine::SalvageMode::ForceTranscode
                          : MediaSalvageEngine::SalvageMode::FastRemux;
    const QStringList args = MediaSalvageEngine::buildArguments(
        m_salvageInputPath, m_salvageOutputPath, mode);

    auto *job = new Job();
    const QString modeTag = (mode == MediaSalvageEngine::SalvageMode::FastRemux)
                                ? QStringLiteral("[Fast Salvage]")
                                : QStringLiteral("[Transcode Salvage]");
    job->setName(modeTag + QStringLiteral(" ") + QFileInfo(m_salvageInputPath).fileName());
    job->setCommand(m_ffmpegLocator->ffmpegPath(), args);
    job->setTotalDuration(m_salvageDuration);
    m_jobQueue->addJob(job);

    m_salvageStatusLabel->setVisible(true);
    m_salvageStatusLabel->setText(trId("repair.salvage.enqueued"));
}

void SalvageRepairTab::onCancelSalvage()
{
    m_salvageEngine->cancel();
    m_startSalvageBtn->setEnabled(true);
    m_queueSalvageBtn->setEnabled(true);
    m_cancelSalvageBtn->setEnabled(false);
    m_salvageProgressBar->setValue(0);
    m_salvageProgressBar->setVisible(false);
    m_salvageStatusLabel->setText(trId("repair.salvage.cancelled"));
}

void SalvageRepairTab::onSalvageProgress(double fraction)
{
    const int pct = qBound(0, static_cast<int>(fraction * 100.0), 100);
    m_salvageProgressBar->setValue(pct);
    m_salvageStatusLabel->setText(trId("repair.repairingPct").arg(pct));
}

void SalvageRepairTab::onSalvageLog(const QString &line)
{
    m_salvageConsole->appendPlainText(line);
}

void SalvageRepairTab::onSalvageFinished(bool success, const QString &outputPath, const QString &errorMsg)
{
    m_startSalvageBtn->setEnabled(true);
    m_queueSalvageBtn->setEnabled(true);
    m_cancelSalvageBtn->setEnabled(false);
    m_salvageProgressBar->setVisible(false);

    if (success && !outputPath.isEmpty()) {
        m_salvageProgressBar->setValue(100);
        m_lastSalvagedPath = outputPath;
        m_salvageStatusLabel->setText(trId("repair.salvage.success"));
        m_salvageResultLabel->setText(trId("repair.salvage.resultText").arg(QFileInfo(outputPath).fileName()));
        m_salvageResultBar->setVisible(true);
        ElaMessageBar::success(ElaMessageBarType::TopRight, trId("common.success"),
                               trId("repair.salvage.success"), 4000, this);
    } else {
        m_salvageStatusLabel->setText(errorMsg.isEmpty() ? trId("repair.salvage.failed") : errorMsg);
        ElaMessageBar::error(ElaMessageBarType::TopRight, trId("nav.repair"),
                             errorMsg.isEmpty() ? trId("repair.salvage.failed") : errorMsg, 4000, this);
    }
}

void SalvageRepairTab::onOpenSalvageFolder()
{
    openFolderAndSelectFile(m_lastSalvagedPath);
}

void SalvageRepairTab::onPlaySalvagedVideo()
{
    playMediaFile(m_lastSalvagedPath);
}

void SalvageRepairTab::retranslateUi()
{
    m_salvageTitleLabel->setText(trId("repair.salvage.title"));
    m_salvageDescLabel->setText(trId("repair.salvage.desc"));
    m_salvageInputCaption->setText(trId("repair.salvage.input") + QStringLiteral(":"));
    m_salvageBrowseBtn->setText(trId("common.browse"));
    m_salvageModeCaption->setText(trId("repair.salvage.mode") + QStringLiteral(":"));

    const int modeIdx = m_salvageModeCombo->currentIndex();
    m_salvageModeCombo->blockSignals(true);
    m_salvageModeCombo->clear();
    m_salvageModeCombo->addItem(trId("repair.salvage.mode.remux"));
    m_salvageModeCombo->addItem(trId("repair.salvage.mode.transcode"));
    m_salvageModeCombo->setCurrentIndex(modeIdx >= 0 ? modeIdx : 0);
    m_salvageModeCombo->blockSignals(false);

    m_salvageOutputCaption->setText(trId("repair.salvage.output") + QStringLiteral(":"));
    m_salvageOutputBrowseBtn->setText(trId("common.browse"));
    m_startSalvageBtn->setText(trId("repair.salvage.start"));
    m_queueSalvageBtn->setText(trId("repair.salvage.queue"));
    m_cancelSalvageBtn->setText(trId("common.cancel"));
    m_salvageOpenFolderBtn->setText(trId("repair.openFolder"));
    m_salvagePlayBtn->setText(trId("queue.play"));
    m_salvageConsoleCaption->setText(trId("repair.console"));
    m_clearSalvageConsoleBtn->setText(trId("repair.clearConsole"));
}
