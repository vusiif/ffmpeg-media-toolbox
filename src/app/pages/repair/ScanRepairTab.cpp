#include "ScanRepairTab.h"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QTextStream>
#include <QVBoxLayout>

#include "ElaCheckBox.h"
#include "ElaLineEdit.h"
#include "ElaMessageBar.h"
#include "ElaProgressBar.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaTableView.h"
#include "ElaText.h"
#include "app/widgets/VideoPlayerWidget.h"
#include "cut/CutExportBuilder.h"
#include "ffmpeg/FFmpegLocator.h"
#include "i18n/TrId.h"
#include "jobs/Job.h"
#include "jobs/JobQueue.h"
#include "media/MediaProbeService.h"

ScanRepairTab::ScanRepairTab(FFmpegLocator *ffmpegLocator,
                             MediaProbeService *probeService,
                             JobQueue *jobQueue,
                             QWidget *parent)
    : RepairTabBase(ffmpegLocator, probeService, jobQueue, parent)
    , m_scanner(new CorruptedVideoScanner(ffmpegLocator, this))
{
    initUi();

    connect(m_scanner, &CorruptedVideoScanner::progressChanged,
            this, &ScanRepairTab::onScanProgress);
    connect(m_scanner, &CorruptedVideoScanner::scanFinished,
            this, &ScanRepairTab::onScanFinished);
}

void ScanRepairTab::initUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 4, 0, 0);
    layout->setSpacing(14);

    // Card 1: Scan Controls
    m_scanControlCard = createCard(this);
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
    m_scanInputCaption = makeCaption(QString(), m_scanControlCard);
    m_scanInputEdit = new ElaLineEdit(m_scanControlCard);
    m_scanInputEdit->setReadOnly(true);
    m_scanBrowseBtn = createCardButton(QString(), 32, m_scanControlCard);
    connect(m_scanBrowseBtn, &ElaPushButton::clicked, this, &ScanRepairTab::onBrowseScanInput);

    inputRow->addWidget(m_scanInputCaption);
    inputRow->addWidget(m_scanInputEdit, 1);
    inputRow->addWidget(m_scanBrowseBtn);
    controlLayout->addLayout(inputRow);

    // QC Detection Options row
    auto *qcOptionsRow = new QHBoxLayout();
    qcOptionsRow->setSpacing(16);
    m_qcDecodeCheck = new ElaCheckBox(m_scanControlCard);
    m_qcDecodeCheck->setChecked(true);
    m_qcSilenceCheck = new ElaCheckBox(m_scanControlCard);
    m_qcSilenceCheck->setChecked(true);
    m_qcBlackCheck = new ElaCheckBox(m_scanControlCard);
    m_qcBlackCheck->setChecked(true);
    m_qcFreezeCheck = new ElaCheckBox(m_scanControlCard);
    m_qcFreezeCheck->setChecked(true);

    qcOptionsRow->addWidget(m_qcDecodeCheck);
    qcOptionsRow->addWidget(m_qcSilenceCheck);
    qcOptionsRow->addWidget(m_qcBlackCheck);
    qcOptionsRow->addWidget(m_qcFreezeCheck);
    qcOptionsRow->addStretch();
    controlLayout->addLayout(qcOptionsRow);

    auto *actionRow = new QHBoxLayout();
    m_scanStartBtn = createCardButton(QString(), 34, m_scanControlCard);
    m_scanStartBtn->setMinimumWidth(120);
    m_scanStartBtn->setEnabled(false);

    m_scanCancelBtn = createCardButton(QString(), 34, m_scanControlCard);
    m_scanCancelBtn->setEnabled(false);

    m_exportReportBtn = createCardButton(QString(), 34, m_scanControlCard);
    m_exportReportBtn->setEnabled(false);
    connect(m_exportReportBtn, &ElaPushButton::clicked, this, &ScanRepairTab::onExportReport);

    m_scanProgressBar = new ElaProgressBar(m_scanControlCard);
    m_scanProgressBar->setRange(0, 100);
    m_scanProgressBar->setValue(0);
    m_scanProgressBar->setVisible(false);

    m_scanStatusLabel = new ElaText(QString(), 12, m_scanControlCard);
    m_scanStatusLabel->setWordWrap(true);

    connect(m_scanStartBtn, &ElaPushButton::clicked, this, &ScanRepairTab::onStartScan);
    connect(m_scanCancelBtn, &ElaPushButton::clicked, this, &ScanRepairTab::onCancelScan);

    actionRow->addWidget(m_scanStartBtn);
    actionRow->addWidget(m_scanCancelBtn);
    actionRow->addWidget(m_exportReportBtn);
    actionRow->addWidget(m_scanProgressBar, 1);
    actionRow->addWidget(m_scanStatusLabel);
    controlLayout->addLayout(actionRow);

    layout->addWidget(m_scanControlCard);

    // Body: Player Card (Left) and QC Results Table Card (Right)
    auto *scanBodyLayout = new QHBoxLayout();
    scanBodyLayout->setSpacing(14);

    // Left Column: Video Player & Timeline
    m_playerCard = createCard(this);
    auto *playerLayout = new QVBoxLayout(m_playerCard);
    playerLayout->setContentsMargins(12, 12, 12, 12);

    m_playerWidget = new VideoPlayerWidget(m_playerCard);
    playerLayout->addWidget(m_playerWidget);
    scanBodyLayout->addWidget(m_playerCard, 3);

    // Right Column: QC Issues List Card
    m_qcResultCard = createCard(this);
    auto *qcLayout = new QVBoxLayout(m_qcResultCard);
    qcLayout->setContentsMargins(14, 14, 14, 14);
    qcLayout->setSpacing(10);

    auto *qcHeader = new QHBoxLayout();
    m_qcSummaryLabel = new ElaText(QString(), 14, m_qcResultCard);
    m_qcSummaryLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));
    m_jumpBtn = createCardButton(QString(), 30, m_qcResultCard);
    m_jumpBtn->setEnabled(false);
    connect(m_jumpBtn, &ElaPushButton::clicked, this, &ScanRepairTab::onJumpToSelectedIssue);

    qcHeader->addWidget(m_qcSummaryLabel);
    qcHeader->addStretch();
    qcHeader->addWidget(m_jumpBtn);
    qcLayout->addLayout(qcHeader);

    m_qcTableView = new ElaTableView(m_qcResultCard);
    m_qcModel = new QStandardItemModel(0, 5, this);
    m_qcTableView->setModel(m_qcModel);
    m_qcTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_qcTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_qcTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_qcTableView->verticalHeader()->setVisible(false);
    m_qcTableView->horizontalHeader()->setStretchLastSection(true);
    connect(m_qcTableView, &ElaTableView::clicked, this, &ScanRepairTab::onQcTableClicked);
    connect(m_qcTableView, &ElaTableView::doubleClicked, this, &ScanRepairTab::onJumpToSelectedIssue);

    qcLayout->addWidget(m_qcTableView, 1);

    // Excise corrupted segments action button
    m_exciseCleanBtn = createCardButton(QString(), 36, m_qcResultCard);
    m_exciseCleanBtn->setEnabled(false);
    connect(m_exciseCleanBtn, &ElaPushButton::clicked, this, &ScanRepairTab::onExciseCorruptedSegments);
    qcLayout->addWidget(m_exciseCleanBtn);

    scanBodyLayout->addWidget(m_qcResultCard, 2);

    layout->addLayout(scanBodyLayout, 1);
}

void ScanRepairTab::handleDroppedFile(const QString &filePath)
{
    m_scanFilePath = filePath;
    m_scanInputEdit->setText(filePath);
    m_playerWidget->loadMedia(filePath);
    m_playerWidget->clearSegments();
    m_qcModel->removeRows(0, m_qcModel->rowCount());
    m_scanStartBtn->setEnabled(true);
    m_exportReportBtn->setEnabled(false);
    m_exciseCleanBtn->setEnabled(false);
    m_jumpBtn->setEnabled(false);

    if (m_probeService) {
        MediaFile info = m_probeService->probe(filePath);
        m_scanDuration = info.duration;
        m_hasVideo = !info.videoStreams.isEmpty();
        m_hasAudio = !info.audioStreams.isEmpty();
    }
}

void ScanRepairTab::loadAndPlayMedia(const QString &filePath)
{
    handleDroppedFile(filePath);
    m_playerWidget->play();
}

void ScanRepairTab::onBrowseScanInput()
{
    const QString path = QFileDialog::getOpenFileName(
        this, trId("repair.scan.input"), QString(),
        QStringLiteral("Video files (*.mp4 *.mov *.mkv *.avi *.ts *.webm *.m4v);;All files (*)"));
    if (path.isEmpty()) return;
    handleDroppedFile(path);
}

void ScanRepairTab::onStartScan()
{
    if (m_scanFilePath.isEmpty()) return;

    m_scanStartBtn->setEnabled(false);
    m_scanCancelBtn->setEnabled(true);
    m_exportReportBtn->setEnabled(false);
    m_exciseCleanBtn->setEnabled(false);
    m_jumpBtn->setEnabled(false);
    m_scanProgressBar->setVisible(true);
    m_scanProgressBar->setValue(0);
    m_scanStatusLabel->setText(trId("repair.scanning"));

    QCOptions opts;
    opts.checkDecodeErrors = m_qcDecodeCheck->isChecked();
    opts.checkSilence = m_qcSilenceCheck->isChecked();
    opts.checkBlackFrames = m_qcBlackCheck->isChecked();
    opts.checkFreezeFrames = m_qcFreezeCheck->isChecked();
    opts.hasVideo = m_hasVideo;
    opts.hasAudio = m_hasAudio;

    m_scanner->startScan(m_scanFilePath, m_scanDuration, opts);
}

void ScanRepairTab::onCancelScan()
{
    m_scanner->cancel();
    m_scanStartBtn->setEnabled(true);
    m_scanCancelBtn->setEnabled(false);
    m_scanProgressBar->setValue(0);
    m_scanProgressBar->setVisible(false);
    m_scanStatusLabel->setText(trId("repair.scanCancelled"));
}

void ScanRepairTab::onScanProgress(double fraction)
{
    const int pct = qBound(0, static_cast<int>(fraction * 100.0), 100);
    m_scanProgressBar->setValue(pct);
    m_scanStatusLabel->setText(trId("repair.repairingPct").arg(pct));
}

void ScanRepairTab::onScanFinished(bool success, const QVector<CorruptedSegment> &segments)
{
    m_scanStartBtn->setEnabled(true);
    m_scanCancelBtn->setEnabled(false);
    m_scanProgressBar->setVisible(false);

    if (success) {
        m_playerWidget->setCorruptedSegments(segments);
        m_exportReportBtn->setEnabled(!segments.isEmpty());
        m_exciseCleanBtn->setEnabled(!segments.isEmpty());
        m_jumpBtn->setEnabled(false);

        // Populate QC Table
        m_qcModel->removeRows(0, m_qcModel->rowCount());

        for (int i = 0; i < segments.size(); ++i) {
            const auto &seg = segments[i];
            QList<QStandardItem *> rowItems;

            auto *typeItem = new QStandardItem(seg.typeName());
            typeItem->setForeground(seg.color().darker(110));
            rowItems << typeItem;

            rowItems << new QStandardItem(QString::number(seg.start, 'f', 2) + QStringLiteral("s"));
            rowItems << new QStandardItem(QString::number(seg.end, 'f', 2) + QStringLiteral("s"));
            rowItems << new QStandardItem(QString::number(seg.end - seg.start, 'f', 2) + QStringLiteral("s"));
            rowItems << new QStandardItem(seg.errorMsg);

            m_qcModel->appendRow(rowItems);
        }

        if (segments.isEmpty()) {
            m_scanStatusLabel->setText(trId("repair.scanNoDamage"));
            m_qcSummaryLabel->setText(trId("repair.qc.summary").arg(0));
        } else {
            m_scanStatusLabel->setText(trId("repair.scanFoundDamage").arg(segments.size()));
            m_qcSummaryLabel->setText(trId("repair.qc.summary").arg(segments.size()));
        }
    } else {
        m_scanStatusLabel->setText(trId("repair.scanFailed"));
        m_exportReportBtn->setEnabled(false);
        m_exciseCleanBtn->setEnabled(false);
    }
}

void ScanRepairTab::onQcTableClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    m_jumpBtn->setEnabled(true);
    const int row = index.row();
    const auto segs = m_scanner->segments();
    if (row >= 0 && row < segs.size()) {
        m_playerWidget->seekToSec(segs.at(row).start);
    }
}

void ScanRepairTab::onJumpToSelectedIssue()
{
    const QModelIndex idx = m_qcTableView->currentIndex();
    if (!idx.isValid()) return;
    const int row = idx.row();
    const auto segs = m_scanner->segments();
    if (row >= 0 && row < segs.size()) {
        m_playerWidget->seekToSec(segs.at(row).start);
    }
}

void ScanRepairTab::onExportReport()
{
    if (m_scanFilePath.isEmpty()) return;

    const QString defaultPath = QFileInfo(m_scanFilePath).absolutePath() + QStringLiteral("/qc_report.txt");
    const QString savePath = QFileDialog::getSaveFileName(
        this, trId("repair.qc.export"), defaultPath, QStringLiteral("Text Files (*.txt);;All Files (*)"));
    if (savePath.isEmpty()) return;

    const QString report = CorruptedVideoScanner::generateReport(
        m_scanFilePath, m_scanDuration, m_scanner->segments());

    QFile file(savePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << report;
        file.close();
    }
}

void ScanRepairTab::onExciseCorruptedSegments()
{
    const auto segments = m_scanner->segments();
    if (segments.isEmpty() || m_scanFilePath.isEmpty()) return;

    double duration = m_scanDuration;
    if (duration <= 0.0 && m_probeService) {
        MediaFile info = m_probeService->probe(m_scanFilePath);
        duration = info.duration;
    }
    if (duration <= 0.0) {
        duration = segments.last().end + 1.0;
    }

    QVector<CutSegment> cleanSegments;
    double currentPos = 0.0;
    for (int i = 0; i < segments.size(); ++i) {
        const auto &bad = segments[i];
        if (bad.start > currentPos + 0.1) {
            CutSegment clean;
            clean.startSec = currentPos;
            clean.endSec = bad.start;
            clean.label = QStringLiteral("Clean Part %1").arg(cleanSegments.size() + 1);
            cleanSegments.append(clean);
        }
        currentPos = qMax(currentPos, bad.end);
    }
    if (duration > currentPos + 0.1) {
        CutSegment clean;
        clean.startSec = currentPos;
        clean.endSec = duration;
        clean.label = QStringLiteral("Clean Part %1").arg(cleanSegments.size() + 1);
        cleanSegments.append(clean);
    }

    if (cleanSegments.isEmpty()) {
        ElaMessageBar::warning(ElaMessageBarType::TopRight, trId("nav.repair"),
                               trId("repair.qc.noCleanContent"), 4000, this);
        return;
    }

    const QFileInfo fi(m_scanFilePath);
    const QString defaultOut = fi.absolutePath() + QStringLiteral("/") + fi.completeBaseName() + QStringLiteral("_cleaned.mp4");
    const QString outPath = QFileDialog::getSaveFileName(this, trId("repair.qc.excise"), defaultOut,
                                                         QStringLiteral("Video Files (*.mp4 *.mkv *.mov);;All Files (*)"));
    if (outPath.isEmpty()) return;

    if (m_jobQueue && m_probeService) {
        MediaFile media = m_probeService->probe(m_scanFilePath);
        QString error;
        QVector<double> keyframes;
        Job *job = CutExportBuilder::createJob(m_ffmpegLocator->ffmpegPath(), media,
                                               outPath, cleanSegments, CutMode::AccurateReencode,
                                               keyframes, &error, m_jobQueue);
        if (job) {
            job->setName(trId("repair.qc.exciseJobName").arg(fi.fileName()));
            m_jobQueue->addJob(job);
            ElaMessageBar::success(ElaMessageBarType::TopRight, trId("common.success"),
                                   trId("repair.qc.exciseEnqueued"), 4000, this);
        } else {
            ElaMessageBar::error(ElaMessageBarType::TopRight, trId("nav.repair"),
                                 error.isEmpty() ? trId("repair.failed") : error, 4000, this);
        }
    }
}

void ScanRepairTab::retranslateUi()
{
    m_scanTitleLabel->setText(trId("repair.scan.title"));
    m_scanDescLabel->setText(trId("repair.scan.desc"));
    m_scanInputCaption->setText(trId("repair.scan.input") + QStringLiteral(":"));
    m_scanBrowseBtn->setText(trId("common.browse"));

    m_qcDecodeCheck->setText(trId("repair.qc.decode"));
    m_qcSilenceCheck->setText(trId("repair.qc.silence"));
    m_qcBlackCheck->setText(trId("repair.qc.black"));
    m_qcFreezeCheck->setText(trId("repair.qc.freeze"));

    m_scanStartBtn->setText(trId("repair.scan.start"));
    m_scanCancelBtn->setText(trId("common.cancel"));
    m_exportReportBtn->setText(trId("repair.qc.export"));
    m_exciseCleanBtn->setText(trId("repair.qc.excise"));

    m_qcSummaryLabel->setText(trId("repair.qc.summary").arg(m_qcModel->rowCount()));
    m_jumpBtn->setText(trId("repair.qc.jump"));

    // Table headers
    m_qcModel->setHeaderData(0, Qt::Horizontal, trId("repair.qc.col.type"));
    m_qcModel->setHeaderData(1, Qt::Horizontal, trId("repair.qc.col.start"));
    m_qcModel->setHeaderData(2, Qt::Horizontal, trId("repair.qc.col.end"));
    m_qcModel->setHeaderData(3, Qt::Horizontal, trId("repair.qc.col.duration"));
    m_qcModel->setHeaderData(4, Qt::Horizontal, trId("repair.qc.col.desc"));

    m_playerWidget->retranslateUi();
}
