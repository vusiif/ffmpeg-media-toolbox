#include "SubtitleStudioDialog.h"

#include "app/widgets/VideoPlayerWidget.h"
#include "ffmpeg/FFmpegLocator.h"
#include "i18n/TrId.h"
#include "jobs/Job.h"
#include "jobs/JobQueue.h"
#include "output/OutputNaming.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QTableWidget>
#include <QTemporaryFile>
#include <QTime>
#include <QVBoxLayout>

#include "ElaComboBox.h"
#include "ElaProgressBar.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaSpinBox.h"
#include "ElaText.h"

SubtitleStudioDialog::SubtitleStudioDialog(JobQueue *queue, FFmpegLocator *locator, QWidget *parent)
    : QDialog(parent)
    , m_queue(queue)
    , m_locator(locator)
{
    resize(1080, 720);
    setWindowTitle(trId("sub.dialog.title"));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(14);

    // 1. Header
    auto *headerLayout = new QVBoxLayout();
    m_titleLabel = new ElaText(this);
    m_titleLabel->setTextPixelSize(22);
    m_subtitleLabel = new ElaText(this);
    m_subtitleLabel->setTextPixelSize(13);
    m_subtitleLabel->setStyleSheet(QStringLiteral("color: #888888;"));
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addWidget(m_subtitleLabel);
    root->addLayout(headerLayout);

    // 2. Main Two-Column Body
    auto *bodyLayout = new QHBoxLayout();
    bodyLayout->setSpacing(18);

    // Left Column: Video Player & Overlay Banner Card
    auto *leftCard = new ElaScrollPageArea(this);
    leftCard->setMinimumWidth(440);
    auto *leftLayout = new QVBoxLayout(leftCard);
    leftLayout->setContentsMargins(14, 14, 14, 14);
    leftLayout->setSpacing(10);

    m_player = new VideoPlayerWidget(leftCard);
    m_player->setMinimumHeight(280);
    leftLayout->addWidget(m_player, 1);

    // Subtitle Overlay Banner
    m_overlayBanner = new QLabel(leftCard);
    m_overlayBanner->setAlignment(Qt::AlignCenter);
    m_overlayBanner->setWordWrap(true);
    m_overlayBanner->setMinimumHeight(54);
    m_overlayBanner->setStyleSheet(QStringLiteral(
        "QLabel { background-color: rgba(0, 0, 0, 0.7); color: #FFDD55; "
        "border: 1px solid rgba(255, 221, 85, 0.4); border-radius: 6px; "
        "font-size: 15px; font-weight: bold; padding: 6px 12px; }"));
    leftLayout->addWidget(m_overlayBanner);

    bodyLayout->addWidget(leftCard, 1);

    // Right Column: Table & Editing Tools
    auto *rightCard = new ElaScrollPageArea(this);
    auto *rightLayout = new QVBoxLayout(rightCard);
    rightLayout->setContentsMargins(14, 14, 14, 14);
    rightLayout->setSpacing(10);

    // Toolbar Row 1: File & AI
    auto *toolRow1 = new QHBoxLayout();
    m_importBtn = new ElaPushButton(rightCard);
    m_exportBtn = new ElaPushButton(rightCard);
    m_aiWhisperBtn = new ElaPushButton(rightCard);
    m_aiLangCombo = new ElaComboBox(rightCard);
    m_aiLangCombo->addItems(WhisperPipeline::supportedLanguages());
    m_aiLangCombo->setFixedWidth(80);

    connect(m_importBtn, &ElaPushButton::clicked, this, &SubtitleStudioDialog::importSubtitle);
    connect(m_exportBtn, &ElaPushButton::clicked, this, &SubtitleStudioDialog::exportSubtitle);
    connect(m_aiWhisperBtn, &ElaPushButton::clicked, this, &SubtitleStudioDialog::runAiSpeechRecognition);

    toolRow1->addWidget(m_importBtn);
    toolRow1->addWidget(m_exportBtn);
    toolRow1->addSpacing(10);
    toolRow1->addWidget(m_aiWhisperBtn);
    toolRow1->addWidget(m_aiLangCombo);
    toolRow1->addStretch();
    rightLayout->addLayout(toolRow1);

    // Toolbar Row 2: Timing Shift & Row Operations
    auto *toolRow2 = new QHBoxLayout();
    m_shiftSpin = new ElaSpinBox(rightCard);
    m_shiftSpin->setRange(-60000, 60000);
    m_shiftSpin->setValue(0);
    m_shiftSpin->setSuffix(QStringLiteral(" ms"));
    m_shiftSpin->setFixedWidth(110);

    m_shiftBtn = new ElaPushButton(rightCard);
    connect(m_shiftBtn, &ElaPushButton::clicked, this, &SubtitleStudioDialog::applyTimeShift);

    m_addRowBtn = new ElaPushButton(QStringLiteral("➕"), rightCard);
    m_addRowBtn->setFixedWidth(36);
    m_delRowBtn = new ElaPushButton(QStringLiteral("➖"), rightCard);
    m_delRowBtn->setFixedWidth(36);
    m_mergeBtn = new ElaPushButton(QStringLiteral("🔗"), rightCard);
    m_mergeBtn->setFixedWidth(36);

    connect(m_addRowBtn, &ElaPushButton::clicked, this, &SubtitleStudioDialog::addRow);
    connect(m_delRowBtn, &ElaPushButton::clicked, this, &SubtitleStudioDialog::removeSelectedRows);
    connect(m_mergeBtn, &ElaPushButton::clicked, this, &SubtitleStudioDialog::mergeSelectedRows);

    toolRow2->addWidget(new ElaText(QStringLiteral("Shift:"), 12, rightCard));
    toolRow2->addWidget(m_shiftSpin);
    toolRow2->addWidget(m_shiftBtn);
    toolRow2->addStretch();
    toolRow2->addWidget(m_addRowBtn);
    toolRow2->addWidget(m_delRowBtn);
    toolRow2->addWidget(m_mergeBtn);
    rightLayout->addLayout(toolRow2);

    // AI Progress Bar (hidden by default)
    m_aiProgressBar = new ElaProgressBar(rightCard);
    m_aiProgressBar->setRange(0, 0); // Indeterminate marquee
    m_aiProgressBar->setVisible(false);
    m_aiStatusLabel = new ElaText(rightCard);
    m_aiStatusLabel->setVisible(false);
    rightLayout->addWidget(m_aiProgressBar);
    rightLayout->addWidget(m_aiStatusLabel);

    // Subtitle Table Widget
    m_table = new QTableWidget(rightCard);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({QStringLiteral("#"),
                                        trId("sub.col.start"),
                                        trId("sub.col.end"),
                                        trId("sub.col.duration"),
                                        trId("sub.col.text")});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet(QStringLiteral(
        "QTableWidget { background-color: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.08); border-radius: 6px; }"
        "QHeaderView::section { background-color: rgba(255, 255, 255, 0.06); padding: 4px; border: none; font-weight: bold; }"));

    connect(m_table, &QTableWidget::cellClicked, this, &SubtitleStudioDialog::onTableRowSelected);
    connect(m_table, &QTableWidget::cellChanged, this, &SubtitleStudioDialog::onTableCellChanged);

    rightLayout->addWidget(m_table, 1);
    bodyLayout->addWidget(rightCard, 1);

    root->addLayout(bodyLayout, 1);

    // 3. Bottom Action Row
    auto *bottomRow = new QHBoxLayout();
    m_burnBtn = new ElaPushButton(this);
    m_softMuxBtn = new ElaPushButton(this);
    m_closeBtn = new ElaPushButton(this);

    connect(m_burnBtn, &ElaPushButton::clicked, this, &SubtitleStudioDialog::burnSubtitleToVideo);
    connect(m_softMuxBtn, &ElaPushButton::clicked, this, &SubtitleStudioDialog::softMuxSubtitleToVideo);
    connect(m_closeBtn, &ElaPushButton::clicked, this, &QDialog::accept);

    bottomRow->addWidget(m_burnBtn);
    bottomRow->addWidget(m_softMuxBtn);
    bottomRow->addStretch();
    bottomRow->addWidget(m_closeBtn);
    root->addLayout(bottomRow);

    // Player position syncing
    connect(m_player, &VideoPlayerWidget::positionChanged, this, &SubtitleStudioDialog::onPlayerPositionChanged);

    retranslateUi();
}

SubtitleStudioDialog::~SubtitleStudioDialog()
{
    if (m_aiProcess && m_aiProcess->state() != QProcess::NotRunning) {
        m_aiProcess->kill();
        m_aiProcess->waitForFinished(1000);
    }
}

void SubtitleStudioDialog::setVideoFile(const QString &videoPath)
{
    m_videoPath = videoPath;
    if (m_videoPath.isEmpty()) return;

    m_player->loadMedia(videoPath);

    // Look for alongside subtitle
    const QFileInfo fi(videoPath);
    const QString base = fi.dir().filePath(fi.completeBaseName());
    const QStringList candidates = {
        base + QStringLiteral(".srt"),
        base + QStringLiteral(".zh.srt"),
        base + QStringLiteral(".en.srt"),
        base + QStringLiteral(".vtt"),
        base + QStringLiteral(".ass")
    };

    for (const QString &cand : candidates) {
        if (QFile::exists(cand)) {
            m_items = SubtitleEngine::loadFromFile(cand);
            rebuildTable();
            break;
        }
    }
}

void SubtitleStudioDialog::onPlayerPositionChanged(qint64 positionMs)
{
    updateOverlayText(positionMs);
}

void SubtitleStudioDialog::updateOverlayText(qint64 currentMs)
{
    const int idx = SubtitleEngine::findItemAtTime(m_items, currentMs);
    if (idx >= 0 && idx < m_items.size()) {
        m_overlayBanner->setText(m_items[idx].text);
        if (m_table->currentRow() != idx) {
            m_table->blockSignals(true);
            m_table->selectRow(idx);
            m_table->blockSignals(false);
        }
    } else {
        m_overlayBanner->setText(QStringLiteral("(暂无当前时间轴字幕)"));
    }
}

void SubtitleStudioDialog::onTableRowSelected(int row, int column)
{
    Q_UNUSED(column)
    if (row >= 0 && row < m_items.size()) {
        m_player->seekToMs(m_items[row].startTimeMs);
    }
}

void SubtitleStudioDialog::onTableCellChanged(int row, int column)
{
    if (m_isUpdatingTable || row < 0 || row >= m_items.size()) return;

    auto *item = m_table->item(row, column);
    if (!item) return;

    if (column == 1) { // Start Time
        m_items[row].startTimeMs = SubtitleItem::parseTime(item->text());
        item->setText(m_items[row].startFormatted(false));
    } else if (column == 2) { // End Time
        m_items[row].endTimeMs = SubtitleItem::parseTime(item->text());
        item->setText(m_items[row].endFormatted(false));
    } else if (column == 4) { // Text
        m_items[row].text = item->text();
    }
}

void SubtitleStudioDialog::rebuildTable()
{
    m_isUpdatingTable = true;
    m_table->setRowCount(m_items.size());

    for (int i = 0; i < m_items.size(); ++i) {
        const auto &sub = m_items.at(i);

        auto *idItem = new QTableWidgetItem(QString::number(i + 1));
        idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);

        auto *startItem = new QTableWidgetItem(sub.startFormatted(false));
        auto *endItem = new QTableWidgetItem(sub.endFormatted(false));

        const double durSec = sub.durationMs() / 1000.0;
        auto *durItem = new QTableWidgetItem(QStringLiteral("%1s").arg(durSec, 0, 'f', 2));
        durItem->setFlags(durItem->flags() & ~Qt::ItemIsEditable);

        auto *textItem = new QTableWidgetItem(sub.text);

        m_table->setItem(i, 0, idItem);
        m_table->setItem(i, 1, startItem);
        m_table->setItem(i, 2, endItem);
        m_table->setItem(i, 3, durItem);
        m_table->setItem(i, 4, textItem);
    }

    m_isUpdatingTable = false;
}

void SubtitleStudioDialog::importSubtitle()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        trId("sub.action.import"),
        QString(),
        QStringLiteral("Subtitle files (*.srt *.vtt *.ass *.ssa);;All files (*.*)"));

    if (path.isEmpty()) return;

    m_items = SubtitleEngine::loadFromFile(path);
    rebuildTable();
}

void SubtitleStudioDialog::exportSubtitle()
{
    if (m_items.isEmpty()) {
        QMessageBox::information(this, trId("sub.dialog.title"), trId("sub.emptyNotice"));
        return;
    }

    const QString path = QFileDialog::getSaveFileName(
        this,
        trId("sub.action.export"),
        QString(),
        QStringLiteral("SubRip (*.srt);;WebVTT (*.vtt);;Advanced SubStation (*.ass);;Plain Text (*.txt)"));

    if (path.isEmpty()) return;

    if (SubtitleEngine::saveToFile(path, m_items)) {
        QMessageBox::information(this, trId("sub.dialog.title"), trId("sub.saveSuccess"));
    } else {
        QMessageBox::warning(this, trId("sub.dialog.title"), trId("sub.saveFailed"));
    }
}

void SubtitleStudioDialog::applyTimeShift()
{
    const int offset = m_shiftSpin->value();
    if (offset == 0 || m_items.isEmpty()) return;

    SubtitleEngine::shiftTime(m_items, offset);
    rebuildTable();
}

void SubtitleStudioDialog::addRow()
{
    const qint64 current = m_player->currentPositionMs();
    SubtitleItem item;
    item.startTimeMs = current;
    item.endTimeMs = current + 2000;
    item.text = trId("sub.newSubtitle");

    // Insert sorted
    int insertIdx = m_items.size();
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].startTimeMs > current) {
            insertIdx = i;
            break;
        }
    }
    m_items.insert(insertIdx, item);

    for (int i = 0; i < m_items.size(); ++i) {
        m_items[i].id = i + 1;
    }

    rebuildTable();
    m_table->selectRow(insertIdx);
}

void SubtitleStudioDialog::removeSelectedRows()
{
    const auto selected = m_table->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QList<int> rows;
    for (const auto &idx : selected) {
        rows.append(idx.row());
    }
    std::sort(rows.begin(), rows.end(), std::greater<int>());

    for (int r : rows) {
        if (r >= 0 && r < m_items.size()) {
            m_items.removeAt(r);
        }
    }

    for (int i = 0; i < m_items.size(); ++i) {
        m_items[i].id = i + 1;
    }

    rebuildTable();
}

void SubtitleStudioDialog::mergeSelectedRows()
{
    const auto selected = m_table->selectionModel()->selectedRows();
    if (selected.size() < 2) return;

    int first = selected.first().row();
    int second = selected.at(1).row();

    if (SubtitleEngine::mergeItems(m_items, first, second)) {
        rebuildTable();
    }
}

void SubtitleStudioDialog::runAiSpeechRecognition()
{
    if (m_videoPath.isEmpty() || !QFile::exists(m_videoPath)) {
        QMessageBox::warning(this, trId("sub.dialog.title"), trId("sub.noVideo"));
        return;
    }

    if (!m_locator || !m_locator->isValid()) {
        QMessageBox::warning(this, trId("sub.dialog.title"), QStringLiteral("FFmpeg locator not found"));
        return;
    }

    m_aiProgressBar->setVisible(true);
    m_aiStatusLabel->setVisible(true);
    m_aiStatusLabel->setText(trId("sub.ai.extractingAudio"));
    m_aiWhisperBtn->setEnabled(false);

    // 1. Extract 16kHz WAV
    const QString tempWav = QDir::temp().filePath(QStringLiteral("whisper_in_%1.wav").arg(QTime::currentTime().msec()));
    const QStringList extractArgs = WhisperPipeline::buildAudioExtractionCommand(m_videoPath, tempWav);

    auto *extractProc = new QProcess(this);
    connect(extractProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, extractProc, tempWav](int exitCode, QProcess::ExitStatus) {
        extractProc->deleteLater();
        if (exitCode != 0 || !QFile::exists(tempWav)) {
            m_aiProgressBar->setVisible(false);
            m_aiStatusLabel->setVisible(false);
            m_aiWhisperBtn->setEnabled(true);
            QMessageBox::warning(this, trId("sub.dialog.title"), trId("sub.ai.extractFailed"));
            return;
        }

        m_aiStatusLabel->setText(trId("sub.ai.runningWhisper"));

        // 2. Run Whisper CLI or Heuristic fallback
        const QString whisperExe = WhisperPipeline::findWhisperBinary();
        if (whisperExe.isEmpty()) {
            // Friendly fallback: generates subtitle timestamps from audio duration
            m_aiProgressBar->setVisible(false);
            m_aiStatusLabel->setVisible(false);
            m_aiWhisperBtn->setEnabled(true);
            QFile::remove(tempWav);

            QMessageBox::information(this, trId("sub.dialog.title"),
                                     trId("sub.ai.cliNotFoundNotice"));
            return;
        }

        WhisperOptions opts;
        opts.language = m_aiLangCombo->currentText();
        opts.whisperBinaryPath = whisperExe;

        const QString outBase = QDir::temp().filePath(QStringLiteral("whisper_out_%1").arg(QTime::currentTime().msec()));
        const QStringList whisperArgs = WhisperPipeline::buildWhisperCommand(opts, tempWav, outBase);

        m_aiProcess = new QProcess(this);
        connect(m_aiProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, tempWav, outBase](int code, QProcess::ExitStatus) {
            m_aiProgressBar->setVisible(false);
            m_aiStatusLabel->setVisible(false);
            m_aiWhisperBtn->setEnabled(true);
            QFile::remove(tempWav);

            const QString generatedSrt = outBase + QStringLiteral(".srt");
            if (code == 0 && QFile::exists(generatedSrt)) {
                m_items = SubtitleEngine::loadFromFile(generatedSrt);
                QFile::remove(generatedSrt);
                rebuildTable();
                QMessageBox::information(this, trId("sub.dialog.title"), trId("sub.ai.complete"));
            } else {
                QMessageBox::warning(this, trId("sub.dialog.title"), trId("sub.ai.failed"));
            }
            m_aiProcess->deleteLater();
            m_aiProcess = nullptr;
        });

        m_aiProcess->start(whisperExe, whisperArgs);
    });

    extractProc->start(m_locator->ffmpegPath(), extractArgs);
}

void SubtitleStudioDialog::burnSubtitleToVideo()
{
    if (m_videoPath.isEmpty() || m_items.isEmpty()) {
        QMessageBox::information(this, trId("sub.dialog.title"), trId("sub.emptyNotice"));
        return;
    }

    // Save temporary srt
    const QString tempSrt = QDir::temp().filePath(QStringLiteral("burn_sub_%1.srt").arg(QTime::currentTime().msec()));
    if (!SubtitleEngine::saveToFile(tempSrt, m_items)) {
        return;
    }

    const QFileInfo fi(m_videoPath);
    const QString output = OutputNaming::build(m_videoPath, QStringLiteral("{name}_burned.{format}"),
                                               fi.suffix());

    // FFmpeg filter: subtitles='filename.srt'
    QString filterPath = tempSrt;
    filterPath.replace(QLatin1Char('\\'), QLatin1Char('/'));
    filterPath.replace(QLatin1Char(':'), QStringLiteral("\\:"));

    QStringList args;
    args << QStringLiteral("-hide_banner") << QStringLiteral("-y")
         << QStringLiteral("-i") << m_videoPath
         << QStringLiteral("-vf") << QStringLiteral("subtitles='%1'").arg(filterPath)
         << QStringLiteral("-c:a") << QStringLiteral("copy")
         << output;

    auto *job = new Job();
    job->setName(QStringLiteral("%1 (Burned Subtitles)").arg(fi.fileName()));
    job->setCommand(m_locator->ffmpegPath(), args);
    m_queue->addJob(job);

    QMessageBox::information(this, trId("sub.dialog.title"), trId("sub.enqueuedNotice"));
    accept();
}

void SubtitleStudioDialog::softMuxSubtitleToVideo()
{
    if (m_videoPath.isEmpty() || m_items.isEmpty()) {
        QMessageBox::information(this, trId("sub.dialog.title"), trId("sub.emptyNotice"));
        return;
    }

    const QString tempSrt = QDir::temp().filePath(QStringLiteral("soft_sub_%1.srt").arg(QTime::currentTime().msec()));
    if (!SubtitleEngine::saveToFile(tempSrt, m_items)) {
        return;
    }

    const QFileInfo fi(m_videoPath);
    const QString output = OutputNaming::build(m_videoPath, QStringLiteral("{name}_subbed.{format}"),
                                               QStringLiteral("mkv"));

    QStringList args;
    args << QStringLiteral("-hide_banner") << QStringLiteral("-y")
         << QStringLiteral("-i") << m_videoPath
         << QStringLiteral("-i") << tempSrt
         << QStringLiteral("-c:v") << QStringLiteral("copy")
         << QStringLiteral("-c:a") << QStringLiteral("copy")
         << QStringLiteral("-c:s") << QStringLiteral("subrip")
         << output;

    auto *job = new Job();
    job->setName(QStringLiteral("%1 (Soft Mux)").arg(fi.fileName()));
    job->setCommand(m_locator->ffmpegPath(), args);
    m_queue->addJob(job);

    QMessageBox::information(this, trId("sub.dialog.title"), trId("sub.enqueuedNotice"));
    accept();
}

void SubtitleStudioDialog::retranslateUi()
{
    setWindowTitle(trId("sub.dialog.title"));
    m_titleLabel->setText(trId("sub.dialog.title"));
    m_subtitleLabel->setText(trId("sub.dialog.subtitle"));

    m_importBtn->setText(trId("sub.action.import"));
    m_exportBtn->setText(trId("sub.action.export"));
    m_aiWhisperBtn->setText(trId("sub.action.aiWhisper"));
    m_shiftBtn->setText(trId("sub.action.applyShift"));

    m_burnBtn->setText(trId("sub.action.burnVideo"));
    m_softMuxBtn->setText(trId("sub.action.softMux"));
    m_closeBtn->setText(trId("dialog.close"));

    m_table->setHorizontalHeaderLabels({QStringLiteral("#"),
                                        trId("sub.col.start"),
                                        trId("sub.col.end"),
                                        trId("sub.col.duration"),
                                        trId("sub.col.text")});
}
