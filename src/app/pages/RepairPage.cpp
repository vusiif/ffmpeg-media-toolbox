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
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QUrl>
#include <QVBoxLayout>

#include "ElaLineEdit.h"
#include "ElaProgressBar.h"
#include "ElaPushButton.h"
#include "ElaText.h"

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
    root->setContentsMargins(30, 20, 30, 20);
    root->setSpacing(12);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setStyleSheet(QStringLiteral(
        "QTabWidget::pane { border: 1px solid rgba(128, 128, 128, 0.2); border-radius: 8px; padding: 12px; }"
        "QTabBar::tab { padding: 8px 18px; font-weight: bold; margin-right: 4px; border-top-left-radius: 6px; border-top-right-radius: 6px; }"
        "QTabBar::tab:selected { background: rgba(0, 120, 212, 0.15); color: #0078d4; }"
    ));

    auto *scanTab = new QWidget(this);
    setupScanTab(scanTab);
    m_tabWidget->addTab(scanTab, QString());

    auto *repairTab = new QWidget(this);
    setupRepairTab(repairTab);
    m_tabWidget->addTab(repairTab, QString());

    root->addWidget(m_tabWidget);

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
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    m_scanTitleLabel = new ElaText(QString(), 13, tab);
    m_scanTitleLabel->setTextPixelSize(22);
    m_scanDescLabel = new ElaText(QString(), 12, tab);
    m_scanDescLabel->setWordWrap(true);
    layout->addWidget(m_scanTitleLabel);
    layout->addWidget(m_scanDescLabel);

    auto *inputRow = new QHBoxLayout();
    m_scanInputCaption = new ElaText(QString(), 13, tab);
    m_scanInputEdit = new ElaLineEdit(tab);
    m_scanInputEdit->setReadOnly(true);
    m_scanBrowseBtn = new ElaPushButton(tab);
    connect(m_scanBrowseBtn, &ElaPushButton::clicked, this, &RepairPage::onBrowseScanInput);

    inputRow->addWidget(m_scanInputCaption);
    inputRow->addWidget(m_scanInputEdit, 1);
    inputRow->addWidget(m_scanBrowseBtn);
    layout->addLayout(inputRow);

    auto *actionRow = new QHBoxLayout();
    m_scanStartBtn = new ElaPushButton(tab);
    m_scanCancelBtn = new ElaPushButton(tab);
    m_scanCancelBtn->setEnabled(false);
    m_scanStatusLabel = new ElaText(QString(), 12, tab);

    connect(m_scanStartBtn, &ElaPushButton::clicked, this, &RepairPage::onStartScan);
    connect(m_scanCancelBtn, &ElaPushButton::clicked, this, &RepairPage::onCancelScan);

    actionRow->addWidget(m_scanStartBtn);
    actionRow->addWidget(m_scanCancelBtn);
    actionRow->addWidget(m_scanStatusLabel);
    actionRow->addStretch();
    layout->addLayout(actionRow);

    m_scanProgressBar = new ElaProgressBar(tab);
    m_scanProgressBar->setRange(0, 100);
    m_scanProgressBar->setValue(0);
    m_scanProgressBar->setVisible(false);
    layout->addWidget(m_scanProgressBar);

    m_playerWidget = new VideoPlayerWidget(tab);
    layout->addWidget(m_playerWidget, 1);
}

void RepairPage::setupRepairTab(QWidget *tab)
{
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    m_repairTitleLabel = new ElaText(QString(), 13, tab);
    m_repairTitleLabel->setTextPixelSize(22);
    m_repairDescLabel = new ElaText(QString(), 12, tab);
    m_repairDescLabel->setWordWrap(true);
    layout->addWidget(m_repairTitleLabel);
    layout->addWidget(m_repairDescLabel);

    auto *form = new QFormLayout();
    form->setSpacing(10);

    // Corrupted file row
    m_corruptedCaption = new ElaText(QString(), 13, tab);
    auto *corruptRow = new QHBoxLayout();
    m_corruptedEdit = new ElaLineEdit(tab);
    m_corruptedEdit->setReadOnly(true);
    m_corruptedBrowseBtn = new ElaPushButton(tab);
    connect(m_corruptedBrowseBtn, &ElaPushButton::clicked, this, &RepairPage::onBrowseCorrupted);
    corruptRow->addWidget(m_corruptedEdit, 1);
    corruptRow->addWidget(m_corruptedBrowseBtn);
    form->addRow(m_corruptedCaption, corruptRow);

    // Reference file row
    m_referenceCaption = new ElaText(QString(), 13, tab);
    auto *refRow = new QHBoxLayout();
    m_referenceEdit = new ElaLineEdit(tab);
    m_referenceEdit->setReadOnly(true);
    m_referenceBrowseBtn = new ElaPushButton(tab);
    connect(m_referenceBrowseBtn, &ElaPushButton::clicked, this, &RepairPage::onBrowseReference);
    refRow->addWidget(m_referenceEdit, 1);
    refRow->addWidget(m_referenceBrowseBtn);
    form->addRow(m_referenceCaption, refRow);

    layout->addLayout(form);

    auto *actionRow = new QHBoxLayout();
    m_startRepairBtn = new ElaPushButton(tab);
    m_cancelRepairBtn = new ElaPushButton(tab);
    m_cancelRepairBtn->setEnabled(false);
    m_repairStatusLabel = new ElaText(QString(), 12, tab);

    connect(m_startRepairBtn, &ElaPushButton::clicked, this, &RepairPage::onStartRepair);
    connect(m_cancelRepairBtn, &ElaPushButton::clicked, this, &RepairPage::onCancelRepair);

    actionRow->addWidget(m_startRepairBtn);
    actionRow->addWidget(m_cancelRepairBtn);
    actionRow->addWidget(m_repairStatusLabel);
    actionRow->addStretch();
    layout->addLayout(actionRow);

    m_repairProgressBar = new ElaProgressBar(tab);
    m_repairProgressBar->setRange(0, 0); // marquee / indeterminate
    m_repairProgressBar->setVisible(false);
    layout->addWidget(m_repairProgressBar);

    // Result action bar
    m_resultBar = new QWidget(tab);
    auto *resLayout = new QHBoxLayout(m_resultBar);
    resLayout->setContentsMargins(0, 0, 0, 0);
    m_resultLabel = new ElaText(QString(), 13, m_resultBar);
    m_openFolderBtn = new ElaPushButton(m_resultBar);
    m_previewFixedBtn = new ElaPushButton(m_resultBar);

    connect(m_openFolderBtn, &ElaPushButton::clicked, this, &RepairPage::onOpenOutputFolder);
    connect(m_previewFixedBtn, &ElaPushButton::clicked, this, &RepairPage::onPreviewFixedVideo);

    resLayout->addWidget(m_resultLabel);
    resLayout->addWidget(m_openFolderBtn);
    resLayout->addWidget(m_previewFixedBtn);
    resLayout->addStretch();
    m_resultBar->setVisible(false);
    layout->addWidget(m_resultBar);

    auto *logHeader = new QHBoxLayout();
    m_consoleCaption = new ElaText(QString(), 13, tab);
    m_clearConsoleBtn = new ElaPushButton(tab);
    connect(m_clearConsoleBtn, &ElaPushButton::clicked, this, [this]() {
        m_console->clear();
    });
    logHeader->addWidget(m_consoleCaption);
    logHeader->addStretch();
    logHeader->addWidget(m_clearConsoleBtn);
    layout->addLayout(logHeader);

    m_console = new QPlainTextEdit(tab);
    m_console->setReadOnly(true);
    m_console->setStyleSheet(QStringLiteral(
        "QPlainTextEdit { background-color: #1a1a1a; color: #00ff66; "
        "font-family: Consolas, 'Courier New', monospace; font-size: 12px; border-radius: 6px; padding: 6px; }"));
    layout->addWidget(m_console, 1);
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

    if (m_tabWidget->currentIndex() == 0) {
        m_scanFilePath = path;
        m_scanInputEdit->setText(path);
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
        m_repairStatusLabel->setText(trId("repair.errorSelectBoth"));
        return;
    }

    if (!m_untruncLocator->isAvailable()) {
        m_repairStatusLabel->setText(trId("repair.untruncNotFound"));
        return;
    }

    m_startRepairBtn->setEnabled(false);
    m_cancelRepairBtn->setEnabled(true);
    m_repairProgressBar->setVisible(true);
    m_resultBar->setVisible(false);
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

    m_tabWidget->setCurrentIndex(0);
    m_playerWidget->play();
}

void RepairPage::retranslateUi()
{
    m_tabWidget->setTabText(0, trId("repair.tab.scan"));
    m_tabWidget->setTabText(1, trId("repair.tab.untrunc"));

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
    m_clearConsoleBtn->setText(trId("common.clear"));

    m_playerWidget->retranslateUi();
}
