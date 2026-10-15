#include "RepairPage.h"

#include "app/widgets/VideoPlayerWidget.h"
#include "ffmpeg/FFmpegLocator.h"
#include "i18n/TrId.h"
#include "media/MediaProbeService.h"
#include "repair/UntruncLocator.h"
#include "repair/UntruncProcess.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QStackedWidget>
#include <QUrl>
#include <QVBoxLayout>

#include "ElaLineEdit.h"
#include "ElaPivot.h"
#include "ElaProgressBar.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaText.h"
#include "ElaTheme.h"

RepairPage::RepairPage(FFmpegLocator *ffmpegLocator,
                       MediaProbeService *probeService,
                       UntruncLocator *untruncLocator,
                       QWidget *parent)
    : QWidget(parent)
    , m_ffmpegLocator(ffmpegLocator)
    , m_probeService(probeService)
    , m_untruncLocator(untruncLocator)
    , m_scanner(new CorruptedVideoScanner(ffmpegLocator, this))
    , m_repairProcess(new UntruncProcess(this))
{
    setAcceptDrops(true);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(30, 24, 30, 24);
    root->setSpacing(14);

    m_pageTitleLabel = new ElaText(QString(), 13, this);
    m_pageTitleLabel->setTextPixelSize(28);
    root->addWidget(m_pageTitleLabel);

    // Fluent Pivot Segmented Switcher
    m_pivot = new ElaPivot(this);
    m_pivot->setPivotSpacing(16);
    m_pivot->setMarkWidth(90);
    root->addWidget(m_pivot);

    // Stacked Pages
    m_stackedWidget = new QStackedWidget(this);

    auto *scanTab = new QWidget(this);
    setupScanTab(scanTab);
    m_stackedWidget->addWidget(scanTab);

    auto *repairTab = new QWidget(this);
    setupRepairTab(repairTab);
    m_stackedWidget->addWidget(repairTab);

    root->addWidget(m_stackedWidget, 1);

    connect(m_pivot, &ElaPivot::pivotClicked, m_stackedWidget, &QStackedWidget::setCurrentIndex);

    // Connect Scanner signals
    connect(m_scanner, &CorruptedVideoScanner::progressChanged,
            this, &RepairPage::onScanProgress);
    connect(m_scanner, &CorruptedVideoScanner::scanFinished,
            this, &RepairPage::onScanFinished);

    // Connect Repair signals
    connect(m_repairProcess, &UntruncProcess::logMessage,
            this, &RepairPage::onRepairLog);
    connect(m_repairProcess, &UntruncProcess::finished,
            this, &RepairPage::onRepairFinished);

    retranslateUi();
}

RepairPage::~RepairPage() = default;

void RepairPage::setupScanTab(QWidget *tab)
{
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(0, 4, 0, 0);
    layout->setSpacing(14);

    // Card 1: Scan Controls
    m_scanControlCard = new ElaScrollPageArea(tab);
    m_scanControlCard->setMinimumHeight(0);
    m_scanControlCard->setMaximumHeight(QWIDGETSIZE_MAX);
    auto *controlLayout = new QVBoxLayout(m_scanControlCard);
    controlLayout->setContentsMargins(18, 16, 18, 16);
    controlLayout->setSpacing(12);

    m_scanTitleLabel = new ElaText(QString(), 15, m_scanControlCard);
    m_scanTitleLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));
    m_scanDescLabel = new ElaText(QString(), 12, m_scanControlCard);
    m_scanDescLabel->setWordWrap(true);
    controlLayout->addWidget(m_scanTitleLabel);
    controlLayout->addWidget(m_scanDescLabel);

    auto *inputRow = new QHBoxLayout();
    m_scanInputCaption = new ElaText(QString(), 13, m_scanControlCard);
    m_scanInputEdit = new ElaLineEdit(m_scanControlCard);
    m_scanInputEdit->setReadOnly(true);
    m_scanBrowseBtn = new ElaPushButton(m_scanControlCard);
    m_scanBrowseBtn->setFixedHeight(32);
    connect(m_scanBrowseBtn, &ElaPushButton::clicked, this, &RepairPage::onBrowseScanInput);

    inputRow->addWidget(m_scanInputCaption);
    inputRow->addWidget(m_scanInputEdit, 1);
    inputRow->addWidget(m_scanBrowseBtn);
    controlLayout->addLayout(inputRow);

    auto *actionRow = new QHBoxLayout();
    m_scanStartBtn = new ElaPushButton(m_scanControlCard);
    m_scanStartBtn->setFixedHeight(34);
    m_scanStartBtn->setMinimumWidth(120);
    m_scanStartBtn->setEnabled(false);

    m_scanCancelBtn = new ElaPushButton(m_scanControlCard);
    m_scanCancelBtn->setFixedHeight(34);
    m_scanCancelBtn->setEnabled(false);

    m_scanProgressBar = new ElaProgressBar(m_scanControlCard);
    m_scanProgressBar->setRange(0, 100);
    m_scanProgressBar->setValue(0);
    m_scanProgressBar->setVisible(false);

    m_scanStatusLabel = new ElaText(QString(), 12, m_scanControlCard);
    m_scanStatusLabel->setWordWrap(true);

    connect(m_scanStartBtn, &ElaPushButton::clicked, this, &RepairPage::onStartScan);
    connect(m_scanCancelBtn, &ElaPushButton::clicked, this, &RepairPage::onCancelScan);

    actionRow->addWidget(m_scanStartBtn);
    actionRow->addWidget(m_scanCancelBtn);
    actionRow->addWidget(m_scanProgressBar, 1);
    actionRow->addWidget(m_scanStatusLabel);
    controlLayout->addLayout(actionRow);

    layout->addWidget(m_scanControlCard);

    // Card 2: Video Player & Corruption Timeline
    m_playerCard = new ElaScrollPageArea(tab);
    m_playerCard->setMinimumHeight(0);
    m_playerCard->setMaximumHeight(QWIDGETSIZE_MAX);
    auto *playerLayout = new QVBoxLayout(m_playerCard);
    playerLayout->setContentsMargins(12, 12, 12, 12);

    m_playerWidget = new VideoPlayerWidget(m_playerCard);
    playerLayout->addWidget(m_playerWidget);

    layout->addWidget(m_playerCard, 1);
}

void RepairPage::setupRepairTab(QWidget *tab)
{
    auto *layout = new QHBoxLayout(tab);
    layout->setContentsMargins(0, 4, 0, 0);
    layout->setSpacing(14);

    // Left Column: Configuration & Execution Card
    m_repairControlCard = new ElaScrollPageArea(tab);
    m_repairControlCard->setMinimumHeight(0);
    m_repairControlCard->setMaximumHeight(QWIDGETSIZE_MAX);
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
    m_corruptedCaption = new ElaText(QString(), 13, m_repairControlCard);
    auto *corruptRow = new QHBoxLayout();
    m_corruptedEdit = new ElaLineEdit(m_repairControlCard);
    m_corruptedEdit->setReadOnly(true);
    m_corruptedBrowseBtn = new ElaPushButton(m_repairControlCard);
    m_corruptedBrowseBtn->setFixedHeight(32);
    connect(m_corruptedBrowseBtn, &ElaPushButton::clicked, this, &RepairPage::onBrowseCorrupted);
    corruptRow->addWidget(m_corruptedEdit, 1);
    corruptRow->addWidget(m_corruptedBrowseBtn);
    corruptedCol->addWidget(m_corruptedCaption);
    corruptedCol->addLayout(corruptRow);
    leftLayout->addLayout(corruptedCol);

    // Reference file row
    auto *refCol = new QVBoxLayout();
    refCol->setSpacing(6);
    m_referenceCaption = new ElaText(QString(), 13, m_repairControlCard);
    auto *refRow = new QHBoxLayout();
    m_referenceEdit = new ElaLineEdit(m_repairControlCard);
    m_referenceEdit->setReadOnly(true);
    m_referenceBrowseBtn = new ElaPushButton(m_repairControlCard);
    m_referenceBrowseBtn->setFixedHeight(32);
    connect(m_referenceBrowseBtn, &ElaPushButton::clicked, this, &RepairPage::onBrowseReference);
    refRow->addWidget(m_referenceEdit, 1);
    refRow->addWidget(m_referenceBrowseBtn);
    refCol->addWidget(m_referenceCaption);
    refCol->addLayout(refRow);
    leftLayout->addLayout(refCol);

    // Action buttons
    auto *actionRow = new QHBoxLayout();
    m_startRepairBtn = new ElaPushButton(m_repairControlCard);
    m_startRepairBtn->setFixedHeight(36);
    m_startRepairBtn->setMinimumWidth(130);
    connect(m_startRepairBtn, &ElaPushButton::clicked, this, &RepairPage::onStartRepair);

    m_cancelRepairBtn = new ElaPushButton(m_repairControlCard);
    m_cancelRepairBtn->setFixedHeight(36);
    m_cancelRepairBtn->setEnabled(false);
    connect(m_cancelRepairBtn, &ElaPushButton::clicked, this, &RepairPage::onCancelRepair);

    actionRow->addWidget(m_startRepairBtn);
    actionRow->addWidget(m_cancelRepairBtn);
    actionRow->addStretch();
    leftLayout->addLayout(actionRow);

    // Progress bar
    m_repairProgressBar = new ElaProgressBar(m_repairControlCard);
    m_repairProgressBar->setRange(0, 0); // indeterminate
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
    m_openFolderBtn = new ElaPushButton(m_resultBar);
    m_openFolderBtn->setFixedHeight(32);
    connect(m_openFolderBtn, &ElaPushButton::clicked, this, &RepairPage::onOpenOutputFolder);

    m_previewFixedBtn = new ElaPushButton(m_resultBar);
    m_previewFixedBtn->setFixedHeight(32);
    connect(m_previewFixedBtn, &ElaPushButton::clicked, this, &RepairPage::onPreviewFixedVideo);

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
    m_consoleCard = new ElaScrollPageArea(tab);
    m_consoleCard->setMinimumHeight(0);
    m_consoleCard->setMaximumHeight(QWIDGETSIZE_MAX);
    auto *rightLayout = new QVBoxLayout(m_consoleCard);
    rightLayout->setContentsMargins(18, 16, 18, 16);
    rightLayout->setSpacing(12);

    auto *logHeader = new QHBoxLayout();
    m_consoleCaption = new ElaText(QString(), 15, m_consoleCard);
    m_consoleCaption->setStyleSheet(QStringLiteral("font-weight: bold;"));
    m_clearConsoleBtn = new ElaPushButton(m_consoleCard);
    m_clearConsoleBtn->setFixedHeight(28);
    connect(m_clearConsoleBtn, &ElaPushButton::clicked, this, [this]() {
        m_console->clear();
    });
    logHeader->addWidget(m_consoleCaption);
    logHeader->addStretch();
    logHeader->addWidget(m_clearConsoleBtn);
    rightLayout->addLayout(logHeader);

    m_console = new QPlainTextEdit(m_consoleCard);
    m_console->setReadOnly(true);

    const auto updateConsoleTheme = [this](ElaThemeType::ThemeMode mode) {
        if (mode == ElaThemeType::Dark) {
            m_console->setStyleSheet(QStringLiteral(
                "QPlainTextEdit { background-color: #1a1a1a; color: #cccccc; "
                "border: 1px solid rgba(255, 255, 255, 0.08); border-radius: 6px; "
                "font-family: 'Consolas', 'Courier New', monospace; font-size: 12px; padding: 8px; }"
            ));
        } else {
            m_console->setStyleSheet(QStringLiteral(
                "QPlainTextEdit { background-color: #f6f6f6; color: #222222; "
                "border: 1px solid rgba(0, 0, 0, 0.08); border-radius: 6px; "
                "font-family: 'Consolas', 'Courier New', monospace; font-size: 12px; padding: 8px; }"
            ));
        }
    };
    connect(eTheme, &ElaTheme::themeModeChanged, this, updateConsoleTheme);
    updateConsoleTheme(eTheme->getThemeMode());

    rightLayout->addWidget(m_console, 1);
    layout->addWidget(m_consoleCard, 1);
}

void RepairPage::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void RepairPage::dropEvent(QDropEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;

    const QString path = urls.first().toLocalFile();
    if (path.isEmpty()) return;

    if (m_stackedWidget->currentIndex() == 0) {
        m_scanFilePath = path;
        m_scanInputEdit->setText(path);
        m_scanStartBtn->setEnabled(true);
        m_playerWidget->loadMedia(path);
        m_playerWidget->clearSegments();
    } else {
        if (m_corruptedPath.isEmpty()) {
            m_corruptedPath = path;
            m_corruptedEdit->setText(path);
        } else {
            m_referencePath = path;
            m_referenceEdit->setText(path);
        }
    }
}

void RepairPage::onBrowseScanInput()
{
    const QString path = QFileDialog::getOpenFileName(
        this, trId("repair.browse"), QString(),
        QStringLiteral("Video files (*.mp4 *.mkv *.mov *.webm *.avi *.flv *.wmv);;All files (*)"));
    if (path.isEmpty()) return;

    m_scanFilePath = path;
    m_scanInputEdit->setText(path);
    m_scanStatusLabel->clear();
    m_scanStartBtn->setEnabled(true);
    m_playerWidget->loadMedia(path);
    m_playerWidget->clearSegments();

    if (m_probeService) {
        MediaFile info = m_probeService->probe(path);
        m_scanDuration = info.duration;
    }
}

void RepairPage::onStartScan()
{
    if (m_scanFilePath.isEmpty()) return;

    m_scanStartBtn->setEnabled(false);
    m_scanCancelBtn->setEnabled(true);
    m_scanProgressBar->setVisible(true);
    m_scanProgressBar->setValue(0);
    m_scanStatusLabel->setText(trId("repair.scanning"));

    m_playerWidget->clearSegments();
    m_scanner->startScan(m_scanFilePath, m_scanDuration);
}

void RepairPage::onCancelScan()
{
    m_scanner->cancel();
    m_scanStartBtn->setEnabled(true);
    m_scanCancelBtn->setEnabled(false);
    m_scanProgressBar->setVisible(false);
    m_scanStatusLabel->setText(trId("repair.scanCancelled"));
}

void RepairPage::onScanProgress(double fraction)
{
    m_scanProgressBar->setValue(static_cast<int>(fraction * 100.0));
}

void RepairPage::onScanFinished(bool success, const QVector<CorruptedSegment> &segments)
{
    m_scanStartBtn->setEnabled(true);
    m_scanCancelBtn->setEnabled(false);
    m_scanProgressBar->setVisible(false);

    if (success) {
        m_playerWidget->setCorruptedSegments(segments);
        if (segments.isEmpty()) {
            m_scanStatusLabel->setText(trId("repair.scanNoDamage"));
        } else {
            m_scanStatusLabel->setText(trId("repair.scanFoundDamage").arg(segments.size()));
        }
    } else {
        m_scanStatusLabel->setText(trId("repair.scanFailed"));
    }
}

void RepairPage::onBrowseCorrupted()
{
    const QString path = QFileDialog::getOpenFileName(
        this, trId("repair.corruptedVideo"), QString(),
        QStringLiteral("Video files (*.mp4 *.mov *.m4v *.rsv);;All files (*)"));
    if (path.isEmpty()) return;
    m_corruptedPath = path;
    m_corruptedEdit->setText(path);
}

void RepairPage::onBrowseReference()
{
    const QString path = QFileDialog::getOpenFileName(
        this, trId("repair.referenceVideo"), QString(),
        QStringLiteral("Video files (*.mp4 *.mov *.m4v);;All files (*)"));
    if (path.isEmpty()) return;
    m_referencePath = path;
    m_referenceEdit->setText(path);
}

void RepairPage::onStartRepair()
{
    if (m_corruptedPath.isEmpty() || m_referencePath.isEmpty()) {
        m_repairStatusLabel->setVisible(true);
        m_repairStatusLabel->setText(trId("repair.errorSelectBoth"));
        return;
    }

    if (!m_untruncLocator->isAvailable()) {
        m_repairStatusLabel->setVisible(true);
        m_repairStatusLabel->setText(trId("repair.untruncNotFound"));
        return;
    }

    m_startRepairBtn->setEnabled(false);
    m_cancelRepairBtn->setEnabled(true);
    m_repairProgressBar->setVisible(true);
    m_resultBar->setVisible(false);
    m_repairStatusLabel->setVisible(true);
    m_repairStatusLabel->setText(trId("repair.repairing"));
    m_console->clear();

    m_repairProcess->start(m_untruncLocator->untruncPath(), m_referencePath, m_corruptedPath);
}

void RepairPage::onCancelRepair()
{
    m_repairProcess->cancel();
    m_startRepairBtn->setEnabled(true);
    m_cancelRepairBtn->setEnabled(false);
    m_repairProgressBar->setVisible(false);
    m_repairStatusLabel->setText(trId("repair.repairCancelled"));
}

void RepairPage::onRepairLog(const QString &line)
{
    m_console->appendPlainText(line);
}

void RepairPage::onRepairFinished(bool success, const QString &fixedPath)
{
    m_startRepairBtn->setEnabled(true);
    m_cancelRepairBtn->setEnabled(false);
    m_repairProgressBar->setVisible(false);

    if (success && !fixedPath.isEmpty()) {
        m_lastFixedPath = fixedPath;
        m_repairStatusLabel->setText(trId("repair.success"));
        m_resultLabel->setText(QFileInfo(fixedPath).fileName());
        m_resultBar->setVisible(true);
    } else {
        m_repairStatusLabel->setText(trId("repair.failed"));
    }
}

void RepairPage::onOpenOutputFolder()
{
    if (m_lastFixedPath.isEmpty()) return;
    const QString dir = QFileInfo(m_lastFixedPath).absolutePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}

void RepairPage::onPreviewFixedVideo()
{
    if (m_lastFixedPath.isEmpty()) return;

    m_scanFilePath = m_lastFixedPath;
    m_scanInputEdit->setText(m_lastFixedPath);
    m_playerWidget->loadMedia(m_lastFixedPath);
    m_playerWidget->clearSegments();

    if (m_probeService) {
        MediaFile info = m_probeService->probe(m_lastFixedPath);
        m_scanDuration = info.duration;
    }

    m_pivot->setCurrentIndex(0);
    m_stackedWidget->setCurrentIndex(0);
    m_playerWidget->play();
}

void RepairPage::retranslateUi()
{
    const int currentIdx = m_pivot->getCurrentIndex();
    if (!m_scanTabTitle.isEmpty()) {
        m_pivot->removePivot(m_scanTabTitle);
        m_pivot->removePivot(m_repairTabTitle);
    }
    m_scanTabTitle = trId("repair.tab.scan");
    m_repairTabTitle = trId("repair.tab.untrunc");
    m_pivot->appendPivot(m_scanTabTitle);
    m_pivot->appendPivot(m_repairTabTitle);
    m_pivot->setCurrentIndex(currentIdx >= 0 ? currentIdx : 0);

    m_pageTitleLabel->setText(trId("nav.repair"));

    // Tab 1
    m_scanTitleLabel->setText(trId("repair.scan.title"));
    m_scanDescLabel->setText(trId("repair.scan.desc"));
    m_scanInputCaption->setText(trId("repair.scan.input") + QStringLiteral(":"));
    m_scanBrowseBtn->setText(trId("common.browse"));
    m_scanStartBtn->setText(trId("repair.scan.start"));
    m_scanCancelBtn->setText(trId("common.cancel"));

    // Tab 2
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

    m_playerWidget->retranslateUi();
}
