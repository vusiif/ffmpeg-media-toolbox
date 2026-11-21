#include "LosslessCutDialog.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMimeData>
#include <QProcess>
#include <QTableWidget>
#include <QVBoxLayout>

#include "ElaComboBox.h"
#include "ElaDoubleSpinBox.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include "ElaScrollArea.h"
#include "ElaScrollPageArea.h"
#include "ElaText.h"
#include "app/widgets/VideoPlayerWidget.h"
#include "cut/SmartCutEngine.h"
#include "cut/CutExportBuilder.h"
#include "ffmpeg/FFmpegLocator.h"
#include "output/OutputNaming.h"
#include "i18n/TrId.h"
#include "jobs/Job.h"
#include "jobs/JobQueue.h"
#include "media/MediaFile.h"
#include "media/MediaProbeService.h"

LosslessCutDialog::LosslessCutDialog(FFmpegLocator *locator, MediaProbeService *probeService,
                                     JobQueue *queue, QWidget *parent)
    : QDialog(parent)
    , m_locator(locator)
    , m_probeService(probeService)
    , m_queue(queue)
{
    setWindowTitle(trId("cut.dialog.title"));
    setMinimumSize(960, 750);
    resize(1020, 820);
    setAcceptDrops(true);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    auto *scroll = new ElaScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setIsGrabGesture(false);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *body = new QWidget(scroll);
    body->setStyleSheet(QStringLiteral("background-color: transparent;"));
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 10, 0);
    bodyLayout->setSpacing(12);

    // 1. Header Card (Title, Subtitle, File picker)
    auto *headerCard = new ElaScrollPageArea(body);
    auto *headerLayout = new QVBoxLayout(headerCard);
    headerLayout->setContentsMargins(14, 12, 14, 12);
    headerLayout->setSpacing(8);

    m_titleLabel = new ElaText(trId("cut.dialog.title"), 16, headerCard);
    m_titleLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #0078d4;"));
    m_subtitleLabel = new ElaText(trId("cut.dialog.subtitle"), 12, headerCard);
    m_subtitleLabel->setStyleSheet(QStringLiteral("color: #888888;"));
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addWidget(m_subtitleLabel);

    auto *fileRow = new QHBoxLayout();
    fileRow->setSpacing(8);
    m_filePathEdit = new ElaLineEdit(headerCard);
    m_filePathEdit->setReadOnly(true);
    m_filePathEdit->setPlaceholderText(trId("cut.dialog.filePlaceholder"));
    m_browseButton = new ElaPushButton(trId("cut.dialog.browse"), headerCard);
    m_browseButton->setFixedHeight(32);
    connect(m_browseButton, &ElaPushButton::clicked, this, &LosslessCutDialog::onBrowseClicked);
    fileRow->addWidget(m_filePathEdit, 1);
    fileRow->addWidget(m_browseButton);
    headerLayout->addLayout(fileRow);

    m_metaInfoLabel = new ElaText(QString(), 12, headerCard);
    m_metaInfoLabel->setStyleSheet(QStringLiteral("color: #0078d4; font-weight: bold;"));
    headerLayout->addWidget(m_metaInfoLabel);
    bodyLayout->addWidget(headerCard);

    // 2. Video Player
    m_player = new VideoPlayerWidget(body);
    m_player->setAutoSkipVisible(false);
    connect(m_player, &VideoPlayerWidget::markInRequested, this, &LosslessCutDialog::onMarkInClicked);
    connect(m_player, &VideoPlayerWidget::markOutRequested, this, &LosslessCutDialog::onMarkOutClicked);
    bodyLayout->addWidget(m_player);

    // 3. Keyframe Toolbar & Range Controls Card
    auto *toolsCard = new ElaScrollPageArea(body);
    auto *toolsLayout = new QVBoxLayout(toolsCard);
    toolsLayout->setContentsMargins(14, 12, 14, 12);
    toolsLayout->setSpacing(10);

    // Keyframe Bar
    auto *kfRow = new QHBoxLayout();
    kfRow->setSpacing(8);
    m_detectKeyframesBtn = new ElaPushButton(trId("cut.keyframe.detect"), toolsCard);
    m_detectKeyframesBtn->setFixedHeight(30);
    connect(m_detectKeyframesBtn, &ElaPushButton::clicked, this, &LosslessCutDialog::onDetectKeyframes);

    m_prevKeyframeBtn = new ElaPushButton(trId("cut.keyframe.prev"), toolsCard);
    m_prevKeyframeBtn->setFixedHeight(30);
    m_prevKeyframeBtn->setEnabled(false);
    connect(m_prevKeyframeBtn, &ElaPushButton::clicked, this, &LosslessCutDialog::onPrevKeyframeClicked);

    m_nextKeyframeBtn = new ElaPushButton(trId("cut.keyframe.next"), toolsCard);
    m_nextKeyframeBtn->setFixedHeight(30);
    m_nextKeyframeBtn->setEnabled(false);
    connect(m_nextKeyframeBtn, &ElaPushButton::clicked, this, &LosslessCutDialog::onNextKeyframeClicked);

    m_snapInBtn = new ElaPushButton(trId("cut.keyframe.snapIn"), toolsCard);
    m_snapInBtn->setFixedHeight(30);
    m_snapInBtn->setEnabled(false);
    connect(m_snapInBtn, &ElaPushButton::clicked, this, &LosslessCutDialog::onSnapInClicked);

    m_snapOutBtn = new ElaPushButton(trId("cut.keyframe.snapOut"), toolsCard);
    m_snapOutBtn->setFixedHeight(30);
    m_snapOutBtn->setEnabled(false);
    connect(m_snapOutBtn, &ElaPushButton::clicked, this, &LosslessCutDialog::onSnapOutClicked);

    m_keyframeStatusLabel = new ElaText(trId("cut.keyframe.notDetected"), 12, toolsCard);
    m_keyframeStatusLabel->setStyleSheet(QStringLiteral("color: #ff9800;"));

    kfRow->addWidget(m_detectKeyframesBtn);
    kfRow->addWidget(m_prevKeyframeBtn);
    kfRow->addWidget(m_nextKeyframeBtn);
    kfRow->addWidget(m_snapInBtn);
    kfRow->addWidget(m_snapOutBtn);
    kfRow->addSpacing(8);
    kfRow->addWidget(m_keyframeStatusLabel, 1);
    toolsLayout->addLayout(kfRow);

    // Range Marker Bar
    auto *rangeRow = new QHBoxLayout();
    rangeRow->setSpacing(8);

    m_markInBtn = new ElaPushButton(trId("cut.range.markIn"), toolsCard);
    m_markInBtn->setFixedHeight(30);
    connect(m_markInBtn, &ElaPushButton::clicked, this, &LosslessCutDialog::onMarkInClicked);

    m_startSpin = new ElaDoubleSpinBox(toolsCard);
    m_startSpin->setRange(0, 86400);
    m_startSpin->setDecimals(3);
    m_startSpin->setSingleStep(0.1);
    m_startSpin->setFixedWidth(110);
    connect(m_startSpin, &ElaDoubleSpinBox::valueChanged, this, [this](double val) {
        m_player->setMarkRange(val, m_endSpin->value());
        updateKeyframeStatus();
    });

    m_markOutBtn = new ElaPushButton(trId("cut.range.markOut"), toolsCard);
    m_markOutBtn->setFixedHeight(30);
    connect(m_markOutBtn, &ElaPushButton::clicked, this, &LosslessCutDialog::onMarkOutClicked);

    m_endSpin = new ElaDoubleSpinBox(toolsCard);
    m_endSpin->setRange(0, 86400);
    m_endSpin->setDecimals(3);
    m_endSpin->setSingleStep(0.1);
    m_endSpin->setFixedWidth(110);
    connect(m_endSpin, &ElaDoubleSpinBox::valueChanged, this, [this](double val) {
        m_player->setMarkRange(m_startSpin->value(), val);
        updateKeyframeStatus();
    });

    m_previewBtn = new ElaPushButton(trId("cut.range.preview"), toolsCard);
    m_previewBtn->setFixedHeight(30);
    connect(m_previewBtn, &ElaPushButton::clicked, this, &LosslessCutDialog::onPreviewRangeClicked);

    m_addSegmentBtn = new ElaPushButton(trId("cut.range.addSegment"), toolsCard);
    m_addSegmentBtn->setFixedHeight(30);
    m_addSegmentBtn->setStyleSheet(QStringLiteral("font-weight: bold;"));
    connect(m_addSegmentBtn, &ElaPushButton::clicked, this, &LosslessCutDialog::onAddSegmentClicked);

    rangeRow->addWidget(m_markInBtn);
    rangeRow->addWidget(m_startSpin);
    rangeRow->addWidget(m_markOutBtn);
    rangeRow->addWidget(m_endSpin);
    rangeRow->addWidget(m_previewBtn);
    rangeRow->addStretch();
    rangeRow->addWidget(m_addSegmentBtn);
    toolsLayout->addLayout(rangeRow);

    bodyLayout->addWidget(toolsCard);

    // 4. Segments Table Card
    auto *tableCard = new ElaScrollPageArea(body);
    auto *tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(14, 12, 14, 12);
    tableLayout->setSpacing(8);

    auto *tableHeaderRow = new QHBoxLayout();
    auto *tableTitle = new ElaText(trId("cut.table.title"), 14, tableCard);
    tableTitle->setStyleSheet(QStringLiteral("font-weight: bold;"));
    m_segmentsSummaryLabel = new ElaText(QString(), 12, tableCard);
    m_segmentsSummaryLabel->setStyleSheet(QStringLiteral("color: #0078d4; font-weight: bold;"));

    m_splitBtn = new ElaPushButton(trId("cut.action.split"), tableCard);
    m_splitBtn->setFixedHeight(26);
    m_splitBtn->setStyleSheet(QStringLiteral("font-weight: bold; background-color: #0078d4; color: white;"));
    connect(m_splitBtn, &ElaPushButton::clicked, this, &LosslessCutDialog::onSplitAtCurrentPosition);

    m_clearSegmentsBtn = new ElaPushButton(trId("cut.table.clear"), tableCard);
    m_clearSegmentsBtn->setFixedHeight(26);
    connect(m_clearSegmentsBtn, &ElaPushButton::clicked, this, &LosslessCutDialog::onClearSegmentsClicked);

    tableHeaderRow->addWidget(tableTitle);
    tableHeaderRow->addWidget(m_segmentsSummaryLabel);
    tableHeaderRow->addStretch();
    tableHeaderRow->addWidget(m_splitBtn);
    tableHeaderRow->addWidget(m_clearSegmentsBtn);
    tableLayout->addLayout(tableHeaderRow);

    m_segmentTable = new QTableWidget(0, 6, tableCard);
    m_segmentTable->setHorizontalHeaderLabels({
        trId("cut.table.colIndex"),
        trId("cut.table.colStart"),
        trId("cut.table.colEnd"),
        trId("cut.table.colDuration"),
        trId("cut.table.colAudioOffset"),
        trId("cut.table.colAction")
    });
    m_segmentTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_segmentTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_segmentTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_segmentTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_segmentTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_segmentTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_segmentTable->setFixedHeight(140);
    m_segmentTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_segmentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(m_segmentTable, &QTableWidget::cellClicked, this, &LosslessCutDialog::onTableItemClicked);
    tableLayout->addWidget(m_segmentTable);

    auto *syncRow = new QHBoxLayout();
    syncRow->setSpacing(8);
    m_audioOffsetLabel = new ElaText(trId("cut.segment.audioOffset"), 12, tableCard);
    m_audioOffsetLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));
    m_audioOffsetSpin = new ElaDoubleSpinBox(tableCard);
    m_audioOffsetSpin->setRange(-10.0, 10.0);
    m_audioOffsetSpin->setSingleStep(0.05);
    m_audioOffsetSpin->setDecimals(2);
    m_audioOffsetSpin->setValue(0.0);
    m_audioOffsetSpin->setSuffix(QStringLiteral(" s"));
    m_audioOffsetSpin->setFixedWidth(100);
    connect(m_audioOffsetSpin, QOverload<double>::of(&ElaDoubleSpinBox::valueChanged), this, &LosslessCutDialog::onAudioOffsetChanged);

    m_audioOffsetTip = new ElaText(trId("cut.segment.audioOffsetTip"), 11, tableCard);
    m_audioOffsetTip->setStyleSheet(QStringLiteral("color: #888888;"));

    syncRow->addWidget(m_audioOffsetLabel);
    syncRow->addWidget(m_audioOffsetSpin);
    syncRow->addWidget(m_audioOffsetTip);
    syncRow->addStretch();
    tableLayout->addLayout(syncRow);

    bodyLayout->addWidget(tableCard);

    // 5. Export Strategy & Action Card
    auto *exportCard = new ElaScrollPageArea(body);
    auto *exportLayout = new QVBoxLayout(exportCard);
    exportLayout->setContentsMargins(14, 12, 14, 12);
    exportLayout->setSpacing(10);

    auto *exportForm = new QFormLayout();
    exportForm->setSpacing(8);

    m_cutModeCombo = new ElaComboBox(exportCard);
    m_cutModeCombo->addItem(trId("cut.mode.lossless"), static_cast<int>(CutMode::KeyframeLossless));
    m_cutModeCombo->addItem(trId("cut.mode.smart"), static_cast<int>(CutMode::SmartCut));
    m_cutModeCombo->addItem(trId("cut.mode.accurate"), static_cast<int>(CutMode::AccurateReencode));

    m_exportModeCombo = new ElaComboBox(exportCard);
    m_exportModeCombo->addItem(trId("cut.export.merge"), static_cast<int>(ExportMode::MergeAll));
    m_exportModeCombo->addItem(trId("cut.export.separate"), static_cast<int>(ExportMode::SeparateFiles));

    m_formatCombo = new ElaComboBox(exportCard);
    m_formatCombo->addItems({QString(), QStringLiteral("mp4"), QStringLiteral("mkv"),
                             QStringLiteral("mov"), QStringLiteral("webm")});

    exportForm->addRow(new ElaText(trId("cut.mode.caption"), 13, exportCard), m_cutModeCombo);
    exportForm->addRow(new ElaText(trId("cut.export.caption"), 13, exportCard), m_exportModeCombo);
    exportForm->addRow(new ElaText(trId("cut.format.caption"), 13, exportCard), m_formatCombo);
    exportLayout->addLayout(exportForm);

    bodyLayout->addWidget(exportCard);

    scroll->setWidget(body);
    rootLayout->addWidget(scroll, 1);

    // Dialog bottom row
    auto *bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(10);
    bottomRow->addStretch();

    m_exportBtn = new ElaPushButton(trId("cut.action.export"), this);
    m_exportBtn->setFixedHeight(36);
    m_exportBtn->setMinimumWidth(160);
    m_exportBtn->setStyleSheet(QStringLiteral("font-weight: bold; background-color: #0078d4; color: white;"));
    connect(m_exportBtn, &ElaPushButton::clicked, this, &LosslessCutDialog::onExportClicked);

    m_closeBtn = new ElaPushButton(trId("dialog.close"), this);
    m_closeBtn->setFixedHeight(36);
    m_closeBtn->setMinimumWidth(100);
    connect(m_closeBtn, &ElaPushButton::clicked, this, &QDialog::accept);

    bottomRow->addWidget(m_exportBtn);
    bottomRow->addWidget(m_closeBtn);
    rootLayout->addLayout(bottomRow);

    m_probeProcess = new QProcess(this);
    connect(m_probeProcess, &QProcess::finished, this, &LosslessCutDialog::onKeyframeProbeFinished);

    refreshSegmentsTable();
}

LosslessCutDialog::~LosslessCutDialog()
{
    if (m_probeProcess && m_probeProcess->state() != QProcess::NotRunning) {
        m_probeProcess->kill();
        m_probeProcess->waitForFinished(500);
    }
}

void LosslessCutDialog::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void LosslessCutDialog::dropEvent(QDropEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;
    const QString path = urls.first().toLocalFile();
    if (!path.isEmpty()) {
        loadFile(path);
        event->acceptProposedAction();
    }
}

void LosslessCutDialog::onBrowseClicked()
{
    const QString file = QFileDialog::getOpenFileName(this, trId("cut.dialog.browse"), QString(),
                                                      QStringLiteral("Video Files (*.mp4 *.mkv *.mov *.avi *.webm *.ts *.flv);;All Files (*.*)"));
    if (!file.isEmpty()) {
        loadFile(file);
    }
}

void LosslessCutDialog::loadFile(const QString &filePath)
{
    if (filePath.isEmpty()) return;
    m_inputFile = filePath;
    m_media = {};
    m_filePathEdit->setText(filePath);

    m_player->loadMedia(filePath);

    // Probe metadata
    if (m_probeService) {
        const MediaFile meta = m_probeService->probe(filePath);
        m_media = meta;
        if (meta.isValid()) {
            const VideoStream v = meta.primaryVideo();
            m_metaInfoLabel->setText(QStringLiteral("%1x%2 | %3 fps | Codec: %4 | Duration: %5s")
                                         .arg(v.width)
                                         .arg(v.height)
                                         .arg(QString::number(v.frameRate, 'f', 2))
                                         .arg(v.codec)
                                         .arg(QString::number(meta.duration, 'f', 2)));
            m_startSpin->setRange(0, meta.duration);
            m_endSpin->setRange(0, meta.duration);
            m_startSpin->setValue(0.0);
            m_endSpin->setValue(meta.duration);
        }
    }

    onClearSegmentsClicked();
    onDetectKeyframes();
}

void LosslessCutDialog::onDetectKeyframes()
{
    if (m_inputFile.isEmpty()) return;
    if (m_probeProcess->state() != QProcess::NotRunning) {
        m_probeProcess->kill();
        m_probeProcess->waitForFinished(500);
    }

    m_keyframes.clear();
    m_keyframeStatusLabel->setText(trId("cut.keyframe.detecting"));
    m_detectKeyframesBtn->setEnabled(false);

    const QString ffprobe = m_locator ? m_locator->ffprobePath() : QStringLiteral("ffprobe");
    const QStringList args = SmartCutEngine::buildKeyframeProbeCommand(ffprobe, m_inputFile);
    m_probeProcess->start(args.first(), args.mid(1));
}

void LosslessCutDialog::onKeyframeProbeFinished(int exitCode)
{
    m_detectKeyframesBtn->setEnabled(true);
    if (exitCode != 0) {
        m_keyframeStatusLabel->setText(trId("cut.keyframe.detectFailed"));
        return;
    }

    const QString output = QString::fromUtf8(m_probeProcess->readAllStandardOutput());
    m_keyframes = SmartCutEngine::parseKeyframeOutput(output);
    m_player->setKeyframes(m_keyframes);

    const bool hasKeyframes = !m_keyframes.isEmpty();
    m_prevKeyframeBtn->setEnabled(hasKeyframes);
    m_nextKeyframeBtn->setEnabled(hasKeyframes);
    m_snapInBtn->setEnabled(hasKeyframes);
    m_snapOutBtn->setEnabled(hasKeyframes);

    updateKeyframeStatus();
}

void LosslessCutDialog::updateKeyframeStatus()
{
    if (m_keyframes.isEmpty()) {
        m_keyframeStatusLabel->setText(trId("cut.keyframe.notDetected"));
        m_keyframeStatusLabel->setStyleSheet(QStringLiteral("color: #ff9800;"));
        return;
    }

    const double inVal = m_startSpin->value();
    const double outVal = m_endSpin->value();
    const bool inAligned = SmartCutEngine::isKeyframeAligned(inVal, m_keyframes);
    const bool outAligned = SmartCutEngine::isKeyframeAligned(outVal, m_keyframes);

    if (inAligned && outAligned) {
        m_keyframeStatusLabel->setText(trId("cut.keyframe.bothAligned").arg(m_keyframes.size()));
        m_keyframeStatusLabel->setStyleSheet(QStringLiteral("color: #107c41; font-weight: bold;"));
    } else if (inAligned) {
        const double nearOut = SmartCutEngine::findNearestKeyframe(outVal, m_keyframes);
        m_keyframeStatusLabel->setText(trId("cut.keyframe.inAlignedOnly")
                                           .arg(QString::number(nearOut, 'f', 3)));
        m_keyframeStatusLabel->setStyleSheet(QStringLiteral("color: #ff9800;"));
    } else if (outAligned) {
        const double nearIn = SmartCutEngine::findNearestKeyframe(inVal, m_keyframes);
        m_keyframeStatusLabel->setText(trId("cut.keyframe.outAlignedOnly")
                                           .arg(QString::number(nearIn, 'f', 3)));
        m_keyframeStatusLabel->setStyleSheet(QStringLiteral("color: #ff9800;"));
    } else {
        m_keyframeStatusLabel->setText(trId("cut.keyframe.neitherAligned").arg(m_keyframes.size()));
        m_keyframeStatusLabel->setStyleSheet(QStringLiteral("color: #d83b01;"));
    }
}

void LosslessCutDialog::onPrevKeyframeClicked()
{
    if (m_keyframes.isEmpty()) return;
    const double cur = m_player->currentPositionSec();
    const double target = SmartCutEngine::findPrevKeyframe(cur - 0.05, m_keyframes);
    m_player->seekToSec(target);
}

void LosslessCutDialog::onNextKeyframeClicked()
{
    if (m_keyframes.isEmpty()) return;
    const double cur = m_player->currentPositionSec();
    const double target = SmartCutEngine::findNextKeyframe(cur + 0.05, m_keyframes);
    m_player->seekToSec(target);
}

void LosslessCutDialog::onSnapInClicked()
{
    if (m_keyframes.isEmpty()) return;
    const double snapped = SmartCutEngine::findNearestKeyframe(m_startSpin->value(), m_keyframes);
    m_startSpin->setValue(snapped);
    m_player->seekToSec(snapped);
}

void LosslessCutDialog::onSnapOutClicked()
{
    if (m_keyframes.isEmpty()) return;
    const double snapped = SmartCutEngine::findNearestKeyframe(m_endSpin->value(), m_keyframes);
    m_endSpin->setValue(snapped);
}

void LosslessCutDialog::onMarkInClicked()
{
    const double pos = m_player->currentPositionSec();
    m_startSpin->setValue(pos);
    if (m_endSpin->value() <= pos && m_player->durationSec() > pos) {
        m_endSpin->setValue(m_player->durationSec());
    }
    m_player->setMarkRange(m_startSpin->value(), m_endSpin->value());
}

void LosslessCutDialog::onMarkOutClicked()
{
    const double pos = m_player->currentPositionSec();
    m_endSpin->setValue(pos);
    m_player->setMarkRange(m_startSpin->value(), m_endSpin->value());
}

void LosslessCutDialog::onPreviewRangeClicked()
{
    m_player->seekToSec(m_startSpin->value());
    m_player->play();
}

void LosslessCutDialog::onAddSegmentClicked()
{
    const double inVal = m_startSpin->value();
    const double outVal = m_endSpin->value();
    if (outVal <= inVal) {
        QMessageBox::warning(this, trId("cut.dialog.title"), trId("cut.error.invalidRange"));
        return;
    }

    CutSegment seg;
    seg.startSec = inVal;
    seg.endSec = outVal;
    seg.label = QStringLiteral("Part %1").arg(m_segments.size() + 1);
    m_segments.append(seg);

    refreshSegmentsTable();
    updateTimelineSegments();
}

void LosslessCutDialog::onClearSegmentsClicked()
{
    m_segments.clear();
    refreshSegmentsTable();
    updateTimelineSegments();
}

void LosslessCutDialog::updateTimelineSegments()
{
    QVector<QPair<double, double>> pairs;
    for (const auto &seg : m_segments) {
        pairs.append(qMakePair(seg.startSec, seg.endSec));
    }
    m_player->setCutSegments(pairs);
}

void LosslessCutDialog::refreshSegmentsTable()
{
    m_segmentTable->setRowCount(m_segments.size());
    double totalSec = 0.0;

    for (int i = 0; i < m_segments.size(); ++i) {
        const auto &seg = m_segments[i];
        totalSec += seg.duration();

        auto *itemIndex = new QTableWidgetItem(QString::number(i + 1));
        itemIndex->setTextAlignment(Qt::AlignCenter);

        auto *itemStart = new QTableWidgetItem(SmartCutEngine::formatTime(seg.startSec));
        itemStart->setTextAlignment(Qt::AlignCenter);

        auto *itemEnd = new QTableWidgetItem(SmartCutEngine::formatTime(seg.endSec));
        itemEnd->setTextAlignment(Qt::AlignCenter);

        auto *itemDur = new QTableWidgetItem(SmartCutEngine::formatDuration(seg.duration()));
        itemDur->setTextAlignment(Qt::AlignCenter);

        QString offsetText = QStringLiteral("0.00s");
        if (std::abs(seg.audioOffsetSec) > 0.001) {
            offsetText = QStringLiteral("%1%2s")
                             .arg(seg.audioOffsetSec > 0 ? QStringLiteral("+") : QString())
                             .arg(seg.audioOffsetSec, 0, 'f', 2);
        }
        auto *itemOffset = new QTableWidgetItem(offsetText);
        itemOffset->setTextAlignment(Qt::AlignCenter);
        if (std::abs(seg.audioOffsetSec) > 0.001) {
            itemOffset->setForeground(QBrush(QColor(0, 120, 212)));
        }

        auto *delBtn = new ElaPushButton(QStringLiteral("✕"), m_segmentTable);
        delBtn->setFixedSize(28, 22);
        delBtn->setProperty("segIndex", i);
        connect(delBtn, &ElaPushButton::clicked, this, [this, i]() {
            if (i >= 0 && i < m_segments.size()) {
                m_segments.removeAt(i);
                refreshSegmentsTable();
                updateTimelineSegments();
            }
        });

        auto *btnContainer = new QWidget(m_segmentTable);
        auto *btnLayout = new QHBoxLayout(btnContainer);
        btnLayout->setContentsMargins(0, 0, 0, 0);
        btnLayout->setAlignment(Qt::AlignCenter);
        btnLayout->addWidget(delBtn);

        m_segmentTable->setItem(i, 0, itemIndex);
        m_segmentTable->setItem(i, 1, itemStart);
        m_segmentTable->setItem(i, 2, itemEnd);
        m_segmentTable->setItem(i, 3, itemDur);
        m_segmentTable->setItem(i, 4, itemOffset);
        m_segmentTable->setCellWidget(i, 5, btnContainer);
    }

    m_segmentsSummaryLabel->setText(trId("cut.table.summary")
                                        .arg(m_segments.size())
                                        .arg(SmartCutEngine::formatDuration(totalSec)));
}

void LosslessCutDialog::onSplitAtCurrentPosition()
{
    if (!m_player) return;
    const double pos = m_player->currentPositionSec();
    if (pos <= 0.05) return;

    // Case 1: If m_segments is empty, take the full media and split it into [0, pos] and [pos, duration]
    if (m_segments.isEmpty()) {
        const double total = m_media.duration > 0 ? m_media.duration : m_player->durationSec();
        if (total > pos + 0.05) {
            CutSegment seg1;
            seg1.startSec = 0.0;
            seg1.endSec = pos;
            CutSegment seg2;
            seg2.startSec = pos;
            seg2.endSec = total;
            m_segments.append(seg1);
            m_segments.append(seg2);
            refreshSegmentsTable();
            updateTimelineSegments();
            m_segmentTable->selectRow(1);
            onTableItemClicked(1, 0);
            return;
        }
    }

    // Case 2: Split the segment containing pos
    for (int i = 0; i < m_segments.size(); ++i) {
        auto &seg = m_segments[i];
        if (pos > seg.startSec + 0.05 && pos < seg.endSec - 0.05) {
            const double originalEnd = seg.endSec;
            seg.endSec = pos;
            CutSegment nextSeg;
            nextSeg.startSec = pos;
            nextSeg.endSec = originalEnd;
            nextSeg.audioOffsetSec = 0.0;
            m_segments.insert(i + 1, nextSeg);
            refreshSegmentsTable();
            updateTimelineSegments();
            m_segmentTable->selectRow(i + 1);
            onTableItemClicked(i + 1, 0);
            return;
        }
    }
}

void LosslessCutDialog::onAudioOffsetChanged(double val)
{
    const int row = m_segmentTable->currentRow();
    if (row >= 0 && row < m_segments.size()) {
        m_segments[row].audioOffsetSec = val;
        QString offsetText = QStringLiteral("0.00s");
        if (std::abs(val) > 0.001) {
            offsetText = QStringLiteral("%1%2s")
                             .arg(val > 0 ? QStringLiteral("+") : QString())
                             .arg(val, 0, 'f', 2);
        }
        if (auto *item = m_segmentTable->item(row, 4)) {
            item->setText(offsetText);
            item->setForeground(std::abs(val) > 0.001 ? QBrush(QColor(0, 120, 212)) : QBrush(QColor(136, 136, 136)));
        }
    }
}

void LosslessCutDialog::onTableItemClicked(int row, int /*column*/)
{
    if (row >= 0 && row < m_segments.size()) {
        const auto &seg = m_segments[row];
        m_startSpin->setValue(seg.startSec);
        m_endSpin->setValue(seg.endSec);
        if (m_audioOffsetSpin) {
            m_audioOffsetSpin->blockSignals(true);
            m_audioOffsetSpin->setValue(seg.audioOffsetSec);
            m_audioOffsetSpin->blockSignals(false);
        }
        m_player->setMarkRange(seg.startSec, seg.endSec);
        m_player->seekToSec(seg.startSec);
    }
}

void LosslessCutDialog::onExportClicked()
{
    if (m_inputFile.isEmpty()) {
        QMessageBox::warning(this, trId("cut.dialog.title"), trId("cut.error.noFile"));
        return;
    }

    // If segments table is empty, use current range as a single segment
    QVector<CutSegment> segmentsToExport = m_segments;
    if (segmentsToExport.isEmpty()) {
        if (m_endSpin->value() <= m_startSpin->value()) {
            QMessageBox::warning(this, trId("cut.dialog.title"), trId("cut.error.invalidRange"));
            return;
        }
        CutSegment single;
        single.startSec = m_startSpin->value();
        single.endSec = m_endSpin->value();
        single.label = QStringLiteral("Part 1");
        segmentsToExport.append(single);
    }

    const auto cutMode = static_cast<CutMode>(m_cutModeCombo->currentData().toInt());
    const auto exportMode = static_cast<ExportMode>(m_exportModeCombo->currentData().toInt());
    const QString formatExt = m_formatCombo->currentText().isEmpty()
                                  ? QFileInfo(m_inputFile).suffix()
                                  : m_formatCombo->currentText();
    if (!m_queue || !m_locator || !m_locator->isValid()) {
        QMessageBox::warning(this, trId("cut.dialog.title"), trId("cut.export.invalidInput"));
        return;
    }

    QList<Job *> jobs;
    const bool separate = exportMode == ExportMode::SeparateFiles;
    const int count = separate ? segmentsToExport.size() : 1;
    for (int i = 0; i < count; ++i) {
        const QString nameTemplate = separate
            ? QStringLiteral("{name}_cut_%1.{format}").arg(i + 1, 2, 10, QLatin1Char('0'))
            : QStringLiteral("{name}_merged.{format}");
        const QString output = OutputNaming::build(m_inputFile, nameTemplate, formatExt);
        const QVector<CutSegment> segments = separate
            ? QVector<CutSegment>{segmentsToExport[i]} : segmentsToExport;
        QString error;
        Job *job = CutExportBuilder::createJob(m_locator->ffmpegPath(), m_media,
                                               output, segments, cutMode, m_keyframes, &error);
        if (!job) {
            qDeleteAll(jobs);
            QMessageBox::warning(this, trId("cut.dialog.title"), error);
            return;
        }
        const QString strategy = job->name();
        const QString name = separate
            ? trId("cut.export.job").arg(i + 1).arg(QFileInfo(m_inputFile).fileName())
            : trId("cut.export.mergedJob").arg(QFileInfo(m_inputFile).fileName()).arg(segments.size());
        job->setName(strategy.isEmpty() ? name : name + QStringLiteral(" — ") + strategy);
        jobs << job;
    }
    m_queue->addJobs(jobs);

    QMessageBox::information(this, trId("cut.dialog.title"),
                            trId("cut.success.enqueued").arg(jobs.size()));
    accept();
}

void LosslessCutDialog::retranslateUi()
{
    setWindowTitle(trId("cut.dialog.title"));
    m_titleLabel->setText(trId("cut.dialog.title"));
    m_subtitleLabel->setText(trId("cut.dialog.subtitle"));
    m_browseButton->setText(trId("cut.dialog.browse"));
    m_detectKeyframesBtn->setText(trId("cut.keyframe.detect"));
    m_prevKeyframeBtn->setText(trId("cut.keyframe.prev"));
    m_nextKeyframeBtn->setText(trId("cut.keyframe.next"));
    m_snapInBtn->setText(trId("cut.keyframe.snapIn"));
    m_snapOutBtn->setText(trId("cut.keyframe.snapOut"));
    m_markInBtn->setText(trId("cut.range.markIn") + QStringLiteral(" (I)"));
    m_markInBtn->setToolTip(QStringLiteral("快捷键: I 或 ["));
    m_markOutBtn->setText(trId("cut.range.markOut") + QStringLiteral(" (O)"));
    m_markOutBtn->setToolTip(QStringLiteral("快捷键: O 或 ]"));
    m_previewBtn->setText(trId("cut.range.preview"));
    m_addSegmentBtn->setText(trId("cut.range.addSegment"));
    if (m_splitBtn) m_splitBtn->setText(trId("cut.action.split"));
    if (m_audioOffsetLabel) m_audioOffsetLabel->setText(trId("cut.segment.audioOffset"));
    if (m_audioOffsetTip) m_audioOffsetTip->setText(trId("cut.segment.audioOffsetTip"));
    m_clearSegmentsBtn->setText(trId("cut.table.clear"));
    m_exportBtn->setText(trId("cut.action.export"));
    m_closeBtn->setText(trId("dialog.close"));
    m_segmentTable->setHorizontalHeaderLabels({
        trId("cut.table.colIndex"),
        trId("cut.table.colStart"),
        trId("cut.table.colEnd"),
        trId("cut.table.colDuration"),
        trId("cut.table.colAudioOffset"),
        trId("cut.table.colAction")
    });
    updateKeyframeStatus();
    refreshSegmentsTable();
}

void LosslessCutDialog::keyPressEvent(QKeyEvent *event)
{
    QWidget *fw = focusWidget();
    if (fw && (qobject_cast<QLineEdit *>(fw) || qobject_cast<QAbstractSpinBox *>(fw))) {
        QDialog::keyPressEvent(event);
        return;
    }

    if (m_player) {
        if (event->key() == Qt::Key_I || event->key() == Qt::Key_BracketLeft) {
            onMarkInClicked();
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_O || event->key() == Qt::Key_BracketRight) {
            onMarkOutClicked();
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Space) {
            m_player->togglePlayPause();
            event->accept();
            return;
        }
    }
    QDialog::keyPressEvent(event);
}
