#include "UntruncRepairTab.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QVBoxLayout>

#include "ElaLineEdit.h"
#include "ElaProgressBar.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaText.h"
#include "i18n/TrId.h"
#include "repair/UntruncLocator.h"
#include "repair/UntruncProcess.h"

UntruncRepairTab::UntruncRepairTab(FFmpegLocator *ffmpegLocator,
                                   MediaProbeService *probeService,
                                   JobQueue *jobQueue,
                                   UntruncLocator *untruncLocator,
                                   QWidget *parent)
    : RepairTabBase(ffmpegLocator, probeService, jobQueue, parent)
    , m_untruncLocator(untruncLocator)
    , m_repairProcess(new UntruncProcess(this))
{
    initUi();

    connect(m_repairProcess, &UntruncProcess::logMessage,
            this, &UntruncRepairTab::onRepairLog);
    connect(m_repairProcess, &UntruncProcess::progressChanged,
            this, &UntruncRepairTab::onRepairProgress);
    connect(m_repairProcess, &UntruncProcess::finished,
            this, &UntruncRepairTab::onRepairFinished);
}

void UntruncRepairTab::initUi()
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 4, 0, 0);
    layout->setSpacing(14);

    // Left Column: Configuration & Execution Card
    m_repairControlCard = createCard(this);
    auto *leftLayout = new QVBoxLayout(m_repairControlCard);
    leftLayout->setContentsMargins(18, 16, 18, 16);
    leftLayout->setSpacing(14);

    m_repairTitleLabel = new ElaText(QString(), 15, m_repairControlCard);
    m_repairTitleLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));
    m_repairDescLabel = new ElaText(QString(), 12, m_repairControlCard);
    m_repairDescLabel->setWordWrap(true);
    leftLayout->addWidget(m_repairTitleLabel);
    leftLayout->addWidget(m_repairDescLabel);

    // Corrupted file row
    auto *corruptedCol = new QVBoxLayout();
    corruptedCol->setSpacing(6);
    m_corruptedCaption = makeCaption(QString(), m_repairControlCard);
    auto *corruptRow = new QHBoxLayout();
    m_corruptedEdit = new ElaLineEdit(m_repairControlCard);
    m_corruptedEdit->setReadOnly(true);
    m_corruptedBrowseBtn = createCardButton(QString(), 32, m_repairControlCard);
    connect(m_corruptedBrowseBtn, &ElaPushButton::clicked, this, &UntruncRepairTab::onBrowseCorrupted);
    corruptRow->addWidget(m_corruptedEdit, 1);
    corruptRow->addWidget(m_corruptedBrowseBtn);
    corruptedCol->addWidget(m_corruptedCaption);
    corruptedCol->addLayout(corruptRow);
    leftLayout->addLayout(corruptedCol);

    // Reference file row
    auto *refCol = new QVBoxLayout();
    refCol->setSpacing(6);
    m_referenceCaption = makeCaption(QString(), m_repairControlCard);
    auto *refRow = new QHBoxLayout();
    m_referenceEdit = new ElaLineEdit(m_repairControlCard);
    m_referenceEdit->setReadOnly(true);
    m_referenceBrowseBtn = createCardButton(QString(), 32, m_repairControlCard);
    connect(m_referenceBrowseBtn, &ElaPushButton::clicked, this, &UntruncRepairTab::onBrowseReference);
    refRow->addWidget(m_referenceEdit, 1);
    refRow->addWidget(m_referenceBrowseBtn);
    refCol->addWidget(m_referenceCaption);
    refCol->addLayout(refRow);
    leftLayout->addLayout(refCol);

    // Action buttons
    auto *actionRow = new QHBoxLayout();
    m_startRepairBtn = createCardButton(QString(), 36, m_repairControlCard);
    m_startRepairBtn->setMinimumWidth(130);
    connect(m_startRepairBtn, &ElaPushButton::clicked, this, &UntruncRepairTab::onStartRepair);

    m_cancelRepairBtn = createCardButton(QString(), 36, m_repairControlCard);
    m_cancelRepairBtn->setEnabled(false);
    connect(m_cancelRepairBtn, &ElaPushButton::clicked, this, &UntruncRepairTab::onCancelRepair);

    actionRow->addWidget(m_startRepairBtn);
    actionRow->addWidget(m_cancelRepairBtn);
    actionRow->addStretch();
    leftLayout->addLayout(actionRow);

    // Progress bar
    m_repairProgressBar = new ElaProgressBar(m_repairControlCard);
    m_repairProgressBar->setRange(0, 100);
    m_repairProgressBar->setValue(0);
    m_repairProgressBar->setVisible(false);
    leftLayout->addWidget(m_repairProgressBar);

    // Status label
    m_repairStatusLabel = new ElaText(QString(), 12, m_repairControlCard);
    m_repairStatusLabel->setWordWrap(true);
    m_repairStatusLabel->setVisible(false);
    leftLayout->addWidget(m_repairStatusLabel);

    // Result action bar
    m_resultBar = new QWidget(m_repairControlCard);
    auto *resultLayout = new QVBoxLayout(m_resultBar);
    resultLayout->setContentsMargins(0, 0, 0, 0);
    resultLayout->setSpacing(8);

    m_resultLabel = new ElaText(QString(), 13, m_resultBar);
    m_resultLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #107c41;"));
    auto *resultBtnRow = new QHBoxLayout();
    m_openFolderBtn = createCardButton(QString(), 32, m_resultBar);
    connect(m_openFolderBtn, &ElaPushButton::clicked, this, &UntruncRepairTab::onOpenOutputFolder);

    m_previewFixedBtn = createCardButton(QString(), 32, m_resultBar);
    connect(m_previewFixedBtn, &ElaPushButton::clicked, this, &UntruncRepairTab::onPreviewFixedVideo);

    resultBtnRow->addWidget(m_openFolderBtn);
    resultBtnRow->addWidget(m_previewFixedBtn);
    resultBtnRow->addStretch();
    resultLayout->addWidget(m_resultLabel);
    resultLayout->addLayout(resultBtnRow);
    m_resultBar->setVisible(false);
    leftLayout->addWidget(m_resultBar);

    leftLayout->addStretch();
    layout->addWidget(m_repairControlCard, 1);

    // Right Column: Log Console Card
    m_consoleCard = createCard(this);
    auto *rightLayout = new QVBoxLayout(m_consoleCard);
    rightLayout->setContentsMargins(18, 16, 18, 16);
    rightLayout->setSpacing(12);

    auto *logHeader = new QHBoxLayout();
    m_consoleCaption = new ElaText(QString(), 15, m_consoleCard);
    m_consoleCaption->setStyleSheet(QStringLiteral("font-weight: bold;"));
    m_clearConsoleBtn = createCardButton(QString(), 28, m_consoleCard);
    connect(m_clearConsoleBtn, &ElaPushButton::clicked, this, [this]() {
        m_console->clear();
    });
    logHeader->addWidget(m_consoleCaption);
    logHeader->addStretch();
    logHeader->addWidget(m_clearConsoleBtn);
    rightLayout->addLayout(logHeader);

    m_console = new QPlainTextEdit(m_consoleCard);
    m_console->setReadOnly(true);
    m_console->setFont(QFont(QStringLiteral("Consolas"), 9));
    m_console->setStyleSheet(QStringLiteral("background-color: #1e1e1e; color: #dcdcdc; border-radius: 6px; padding: 6px;"));
    rightLayout->addWidget(m_console, 1);

    layout->addWidget(m_consoleCard, 1);
}

void UntruncRepairTab::handleDroppedFile(const QString &filePath)
{
    m_corruptedPath = filePath;
    m_corruptedEdit->setText(filePath);
}

void UntruncRepairTab::onBrowseCorrupted()
{
    const QString path = QFileDialog::getOpenFileName(
        this, trId("repair.corruptedVideo"), QString(),
        QStringLiteral("Video files (*.mp4 *.mov *.m4v);;All files (*)"));
    if (path.isEmpty()) return;
    m_corruptedPath = path;
    m_corruptedEdit->setText(path);
}

void UntruncRepairTab::onBrowseReference()
{
    const QString path = QFileDialog::getOpenFileName(
        this, trId("repair.referenceVideo"), QString(),
        QStringLiteral("Video files (*.mp4 *.mov *.m4v);;All files (*)"));
    if (path.isEmpty()) return;
    m_referencePath = path;
    m_referenceEdit->setText(path);
}

void UntruncRepairTab::onStartRepair()
{
    if (m_corruptedPath.isEmpty() || m_referencePath.isEmpty()) return;

    if (!m_untruncLocator || !m_untruncLocator->isAvailable()) {
        m_repairStatusLabel->setVisible(true);
        m_repairStatusLabel->setText(trId("repair.failed"));
        m_console->appendPlainText(QStringLiteral("[Error] Untrunc tool is not available or configured."));
        return;
    }

    m_startRepairBtn->setEnabled(false);
    m_cancelRepairBtn->setEnabled(true);
    m_repairProgressBar->setVisible(true);
    m_repairProgressBar->setValue(0);
    m_repairStatusLabel->setVisible(true);
    m_repairStatusLabel->setText(trId("repair.starting"));
    m_resultBar->setVisible(false);
    m_console->clear();

    m_repairProcess->start(m_untruncLocator->untruncPath(),
                           m_referencePath,
                           m_corruptedPath);
}

void UntruncRepairTab::onCancelRepair()
{
    m_repairProcess->cancel();
    m_startRepairBtn->setEnabled(true);
    m_cancelRepairBtn->setEnabled(false);
    m_repairProgressBar->setValue(0);
    m_repairProgressBar->setVisible(false);
    m_repairStatusLabel->setText(trId("repair.repairCancelled"));
}

void UntruncRepairTab::onRepairLog(const QString &line)
{
    m_console->appendPlainText(line);
}

void UntruncRepairTab::onRepairProgress(double fraction)
{
    const int pct = qBound(0, static_cast<int>(fraction * 100.0), 100);
    m_repairProgressBar->setValue(pct);
    m_repairStatusLabel->setText(trId("repair.repairingPct").arg(pct));
}

void UntruncRepairTab::onRepairFinished(bool success, const QString &fixedPath)
{
    m_startRepairBtn->setEnabled(true);
    m_cancelRepairBtn->setEnabled(false);
    m_repairProgressBar->setVisible(false);

    if (success && !fixedPath.isEmpty()) {
        m_repairProgressBar->setValue(100);
        m_lastFixedPath = fixedPath;
        m_repairStatusLabel->setText(trId("repair.success"));
        m_resultLabel->setText(QFileInfo(fixedPath).fileName());
        m_resultBar->setVisible(true);
    } else {
        m_repairStatusLabel->setText(trId("repair.failed"));
    }
}

void UntruncRepairTab::onOpenOutputFolder()
{
    openFolderAndSelectFile(m_lastFixedPath);
}

void UntruncRepairTab::onPreviewFixedVideo()
{
    if (m_lastFixedPath.isEmpty()) return;
    emit previewRequested(m_lastFixedPath);
}

void UntruncRepairTab::retranslateUi()
{
    m_repairTitleLabel->setText(trId("repair.untrunc.title"));
    m_repairDescLabel->setText(trId("repair.untrunc.desc"));
    m_corruptedCaption->setText(trId("repair.corruptedVideo") + QStringLiteral(":"));
    m_corruptedBrowseBtn->setText(trId("common.browse"));
    m_referenceCaption->setText(trId("repair.referenceVideo") + QStringLiteral(":"));
    m_referenceBrowseBtn->setText(trId("common.browse"));
    m_startRepairBtn->setText(trId("repair.untrunc.start"));
    m_cancelRepairBtn->setText(trId("common.cancel"));
    m_openFolderBtn->setText(trId("repair.openFolder"));
    m_previewFixedBtn->setText(trId("repair.previewFixed"));
    m_consoleCaption->setText(trId("repair.console"));
    m_clearConsoleBtn->setText(trId("repair.clearConsole"));
}
