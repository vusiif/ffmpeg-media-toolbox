#include "AudioTagStudioDialog.h"

#include <QClipboard>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QProcess>
#include <QStandardPaths>
#include <QTableWidget>
#include <QVBoxLayout>

#include "ElaCheckBox.h"
#include "ElaComboBox.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include "ElaScrollArea.h"
#include "ElaScrollPageArea.h"
#include "ElaText.h"

#include "ffmpeg/FFmpegLocator.h"
#include "i18n/TrId.h"
#include "jobs/Job.h"
#include "jobs/JobQueue.h"
#include "media/MediaProbeService.h"
#include "output/OutputNaming.h"

AudioTagStudioDialog::AudioTagStudioDialog(JobQueue *queue, FFmpegLocator *locator,
                                           MediaProbeService *probeService, QWidget *parent)
    : QDialog(parent)
    , m_queue(queue)
    , m_locator(locator)
    , m_probeService(probeService)
{
    resize(1120, 760);
    setWindowTitle(trId("audiotag.dialog.title"));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(20, 16, 20, 16);
    root->setSpacing(12);

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
    bodyLayout->setSpacing(16);

    // Left Column: File Table & Table Toolbar
    auto *leftCol = new QVBoxLayout();
    leftCol->setSpacing(8);

    // Table Toolbar
    auto *toolbar = new QHBoxLayout();
    m_addFilesBtn = new ElaPushButton(trId("audiotag.addFiles"), this);
    m_addFolderBtn = new ElaPushButton(trId("audiotag.addFolder"), this);
    m_removeSelectedBtn = new ElaPushButton(trId("audiotag.removeSelected"), this);
    m_guessMetaBtn = new ElaPushButton(trId("audiotag.guessMeta"), this);
    m_clearAllBtn = new ElaPushButton(trId("common.clear"), this);

    connect(m_addFilesBtn, &ElaPushButton::clicked, this, &AudioTagStudioDialog::onAddFiles);
    connect(m_addFolderBtn, &ElaPushButton::clicked, this, &AudioTagStudioDialog::onAddFolder);
    connect(m_removeSelectedBtn, &ElaPushButton::clicked, this, &AudioTagStudioDialog::onRemoveSelected);
    connect(m_guessMetaBtn, &ElaPushButton::clicked, this, &AudioTagStudioDialog::onGuessFromFilenames);
    connect(m_clearAllBtn, &ElaPushButton::clicked, this, &AudioTagStudioDialog::onClearAll);

    toolbar->addWidget(m_addFilesBtn);
    toolbar->addWidget(m_addFolderBtn);
    toolbar->addWidget(m_removeSelectedBtn);
    toolbar->addWidget(m_guessMetaBtn);
    toolbar->addWidget(m_clearAllBtn);
    toolbar->addStretch();
    leftCol->addLayout(toolbar);

    // Multi-column table
    m_table = new QTableWidget(this);
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("#"),
        trId("common.file"),
        trId("audiotag.title"),
        trId("audiotag.artist"),
        trId("audiotag.album"),
        trId("common.format"),
        trId("common.duration")
    });
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setAlternatingRowColors(true);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &AudioTagStudioDialog::onTableRowSelected);
    leftCol->addWidget(m_table, 1);

    m_fileCountLabel = new QLabel(this);
    m_fileCountLabel->setStyleSheet(QStringLiteral("color: #888888; font-size: 12px;"));
    leftCol->addWidget(m_fileCountLabel);

    bodyLayout->addLayout(leftCol, 6);

    // Right Column: Controls in Scroll Area
    auto *rightScroll = new ElaScrollArea(this);
    rightScroll->setWidgetResizable(true);
    auto *rightWidget = new QWidget(rightScroll);
    auto *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(6, 6, 6, 6);
    rightLayout->setSpacing(12);

    // Card 1: Metadata & Cover Art
    auto *metaCard = new ElaScrollPageArea(rightWidget);
    auto *metaCardLayout = new QVBoxLayout(metaCard);
    metaCardLayout->setContentsMargins(12, 12, 12, 12);
    metaCardLayout->setSpacing(8);

    auto *cardTitle = new ElaText(trId("audiotag.metaCardTitle"), 15, metaCard);
    cardTitle->setStyleSheet(QStringLiteral("font-weight: bold;"));
    metaCardLayout->addWidget(cardTitle);

    // Cover art row
    auto *coverRow = new QHBoxLayout();
    m_coverPreview = new QLabel(metaCard);
    m_coverPreview->setFixedSize(100, 100);
    m_coverPreview->setStyleSheet(
        QStringLiteral("background-color: #1e2029; border: 1px dashed #555566; border-radius: 6px;"));
    m_coverPreview->setAlignment(Qt::AlignCenter);
    m_coverPreview->setText(trId("audiotag.noCover"));
    m_coverPreview->setWordWrap(true);

    auto *coverBtnCol = new QVBoxLayout();
    coverBtnCol->setSpacing(4);
    m_browseCoverBtn = new ElaPushButton(trId("audiotag.browseCover"), metaCard);
    m_extractCoverBtn = new ElaPushButton(trId("audiotag.extractCover"), metaCard);
    m_removeCoverBtn = new ElaPushButton(trId("audiotag.removeCover"), metaCard);
    connect(m_browseCoverBtn, &ElaPushButton::clicked, this, &AudioTagStudioDialog::onBrowseCover);
    connect(m_extractCoverBtn, &ElaPushButton::clicked, this, &AudioTagStudioDialog::onExtractCover);
    connect(m_removeCoverBtn, &ElaPushButton::clicked, this, &AudioTagStudioDialog::onRemoveCover);
    coverBtnCol->addWidget(m_browseCoverBtn);
    coverBtnCol->addWidget(m_extractCoverBtn);
    coverBtnCol->addWidget(m_removeCoverBtn);
    coverBtnCol->addStretch();

    coverRow->addWidget(m_coverPreview);
    coverRow->addLayout(coverBtnCol, 1);
    metaCardLayout->addLayout(coverRow);

    // Metadata Fields
    auto *metaForm = new QFormLayout();
    metaForm->setSpacing(8);

    m_titleEdit = new ElaLineEdit(metaCard);
    m_artistEdit = new ElaLineEdit(metaCard);
    m_albumEdit = new ElaLineEdit(metaCard);

    m_genreCombo = new ElaComboBox(metaCard);
    m_genreCombo->setEditable(true);
    m_genreCombo->addItems({
        QString(),
        QStringLiteral("Pop"),
        QStringLiteral("Rock"),
        QStringLiteral("Classical"),
        QStringLiteral("Jazz"),
        QStringLiteral("Electronic"),
        QStringLiteral("Hip-Hop"),
        QStringLiteral("Anime / ACG"),
        QStringLiteral("Soundtrack / OST"),
        QStringLiteral("Podcast / 播客"),
        QStringLiteral("Audiobook / 有声书")
    });

    m_yearEdit = new ElaLineEdit(metaCard);
    m_trackEdit = new ElaLineEdit(metaCard);
    m_commentEdit = new ElaLineEdit(metaCard);

    connect(m_titleEdit, &ElaLineEdit::textEdited, this, &AudioTagStudioDialog::onMetadataFieldEdited);
    connect(m_artistEdit, &ElaLineEdit::textEdited, this, &AudioTagStudioDialog::onMetadataFieldEdited);
    connect(m_albumEdit, &ElaLineEdit::textEdited, this, &AudioTagStudioDialog::onMetadataFieldEdited);
    connect(m_genreCombo, &ElaComboBox::currentTextChanged, this, &AudioTagStudioDialog::onMetadataFieldEdited);
    connect(m_yearEdit, &ElaLineEdit::textEdited, this, &AudioTagStudioDialog::onMetadataFieldEdited);
    connect(m_trackEdit, &ElaLineEdit::textEdited, this, &AudioTagStudioDialog::onMetadataFieldEdited);
    connect(m_commentEdit, &ElaLineEdit::textEdited, this, &AudioTagStudioDialog::onMetadataFieldEdited);

    metaForm->addRow(new ElaText(trId("audiotag.title") + QStringLiteral(":"), metaCard), m_titleEdit);
    metaForm->addRow(new ElaText(trId("audiotag.artist") + QStringLiteral(":"), metaCard), m_artistEdit);
    metaForm->addRow(new ElaText(trId("audiotag.album") + QStringLiteral(":"), metaCard), m_albumEdit);
    metaForm->addRow(new ElaText(trId("audiotag.genre") + QStringLiteral(":"), metaCard), m_genreCombo);
    metaForm->addRow(new ElaText(trId("audiotag.year") + QStringLiteral(":"), metaCard), m_yearEdit);
    metaForm->addRow(new ElaText(trId("audiotag.track") + QStringLiteral(":"), metaCard), m_trackEdit);
    metaForm->addRow(new ElaText(trId("audiotag.comment") + QStringLiteral(":"), metaCard), m_commentEdit);
    metaCardLayout->addLayout(metaForm);

    m_applySelectedBtn = new ElaPushButton(trId("audiotag.applySelected"), metaCard);
    connect(m_applySelectedBtn, &ElaPushButton::clicked, this, &AudioTagStudioDialog::onApplyToAllSelected);
    metaCardLayout->addWidget(m_applySelectedBtn);

    rightLayout->addWidget(metaCard);

    // Card 2: Extraction Format & Quality
    auto *fmtCard = new ElaScrollPageArea(rightWidget);
    auto *fmtLayout = new QVBoxLayout(fmtCard);
    fmtLayout->setContentsMargins(12, 12, 12, 12);
    fmtLayout->setSpacing(8);

    auto *fmtTitle = new ElaText(trId("audiotag.formatCardTitle"), 15, fmtCard);
    fmtTitle->setStyleSheet(QStringLiteral("font-weight: bold;"));
    fmtLayout->addWidget(fmtTitle);

    auto *fmtForm = new QFormLayout();
    fmtForm->setSpacing(8);

    m_presetCombo = new ElaComboBox(fmtCard);
    m_presetCombo->addItem(trId("audiotag.preset.mp3_320k"), static_cast<int>(Audio::AudioQualityPreset::HighMp3_320k));
    m_presetCombo->addItem(trId("audiotag.preset.mp3_192k"), static_cast<int>(Audio::AudioQualityPreset::BalancedMp3_192k));
    m_presetCombo->addItem(trId("audiotag.preset.flac"), static_cast<int>(Audio::AudioQualityPreset::LosslessFlac));
    m_presetCombo->addItem(trId("audiotag.preset.wav"), static_cast<int>(Audio::AudioQualityPreset::LosslessWav));
    m_presetCombo->addItem(trId("audiotag.preset.aac_256k"), static_cast<int>(Audio::AudioQualityPreset::BroadcastAac_256k));
    m_presetCombo->addItem(trId("audiotag.preset.opus_64k"), static_cast<int>(Audio::AudioQualityPreset::VoiceOpus_64k));
    connect(m_presetCombo, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this, &AudioTagStudioDialog::onPresetChanged);
    fmtForm->addRow(new ElaText(trId("audiotag.preset") + QStringLiteral(":"), fmtCard), m_presetCombo);

    m_formatCombo = new ElaComboBox(fmtCard);
    m_formatCombo->addItem(QStringLiteral("MP3 (.mp3)"), static_cast<int>(Audio::AudioFormat::MP3));
    m_formatCombo->addItem(QStringLiteral("FLAC (.flac)"), static_cast<int>(Audio::AudioFormat::FLAC));
    m_formatCombo->addItem(QStringLiteral("AAC / M4A (.m4a)"), static_cast<int>(Audio::AudioFormat::M4A));
    m_formatCombo->addItem(QStringLiteral("WAV (.wav)"), static_cast<int>(Audio::AudioFormat::WAV));
    m_formatCombo->addItem(QStringLiteral("OPUS (.opus)"), static_cast<int>(Audio::AudioFormat::OPUS));
    m_formatCombo->addItem(QStringLiteral("OGG (.ogg)"), static_cast<int>(Audio::AudioFormat::OGG));
    connect(m_formatCombo, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this, &AudioTagStudioDialog::onFormatChanged);
    fmtForm->addRow(new ElaText(trId("common.format") + QStringLiteral(":"), fmtCard), m_formatCombo);

    m_bitrateCombo = new ElaComboBox(fmtCard);
    m_bitrateCombo->addItem(QStringLiteral("128 kbps"), 128);
    m_bitrateCombo->addItem(QStringLiteral("192 kbps"), 192);
    m_bitrateCombo->addItem(QStringLiteral("256 kbps"), 256);
    m_bitrateCombo->addItem(QStringLiteral("320 kbps (最高品质)"), 320);
    m_bitrateCombo->setCurrentIndex(3); // 320k
    fmtForm->addRow(new ElaText(trId("audiotag.bitrate") + QStringLiteral(":"), fmtCard), m_bitrateCombo);

    m_sampleRateCombo = new ElaComboBox(fmtCard);
    m_sampleRateCombo->addItem(QStringLiteral("保持原样 (Auto)"), -1);
    m_sampleRateCombo->addItem(QStringLiteral("44100 Hz (CD 标准)"), 44100);
    m_sampleRateCombo->addItem(QStringLiteral("48000 Hz (影视标准)"), 48000);
    m_sampleRateCombo->addItem(QStringLiteral("96000 Hz (高解析度)"), 96000);
    fmtForm->addRow(new ElaText(trId("audiotag.samplerate") + QStringLiteral(":"), fmtCard), m_sampleRateCombo);

    m_channelsCombo = new ElaComboBox(fmtCard);
    m_channelsCombo->addItem(QStringLiteral("保持原样 (Auto)"), -1);
    m_channelsCombo->addItem(QStringLiteral("单声道 (Mono)"), 1);
    m_channelsCombo->addItem(QStringLiteral("立体声 (Stereo)"), 2);
    fmtForm->addRow(new ElaText(trId("audiotag.channels") + QStringLiteral(":"), fmtCard), m_channelsCombo);

    m_loudnormCheck = new ElaCheckBox(trId("audiotag.loudnorm"), fmtCard);
    fmtForm->addRow(QString(), m_loudnormCheck);

    fmtLayout->addLayout(fmtForm);
    rightLayout->addWidget(fmtCard);

    // Action Buttons
    auto *btnRow = new QHBoxLayout();
    m_copyCmdBtn = new ElaPushButton(trId("audiotag.copyCmd"), rightWidget);
    connect(m_copyCmdBtn, &ElaPushButton::clicked, this, &AudioTagStudioDialog::onCopyCurrentCommand);

    m_batchQueueBtn = new ElaPushButton(trId("audiotag.batchQueue"), rightWidget);
    m_batchQueueBtn->setStyleSheet(QStringLiteral("background-color: #0078d4; color: white; font-weight: bold;"));
    connect(m_batchQueueBtn, &ElaPushButton::clicked, this, &AudioTagStudioDialog::onBatchAddToQueue);

    btnRow->addWidget(m_copyCmdBtn);
    btnRow->addWidget(m_batchQueueBtn);
    rightLayout->addLayout(btnRow);

    rightScroll->setWidget(rightWidget);
    bodyLayout->addWidget(rightScroll, 5);

    root->addLayout(bodyLayout);

    retranslateUi();
}

AudioTagStudioDialog::~AudioTagStudioDialog() = default;

void AudioTagStudioDialog::retranslateUi()
{
    setWindowTitle(trId("audiotag.dialog.title"));
    m_titleLabel->setText(trId("audiotag.dialog.title"));
    m_subtitleLabel->setText(trId("audiotag.dialog.subtitle"));

    m_addFilesBtn->setText(trId("audiotag.addFiles"));
    m_addFolderBtn->setText(trId("audiotag.addFolder"));
    m_removeSelectedBtn->setText(trId("audiotag.removeSelected"));
    m_guessMetaBtn->setText(trId("audiotag.guessMeta"));
    m_clearAllBtn->setText(trId("common.clear"));

    m_browseCoverBtn->setText(trId("audiotag.browseCover"));
    m_extractCoverBtn->setText(trId("audiotag.extractCover"));
    m_removeCoverBtn->setText(trId("audiotag.removeCover"));

    m_applySelectedBtn->setText(trId("audiotag.applySelected"));
    m_loudnormCheck->setText(trId("audiotag.loudnorm"));

    m_copyCmdBtn->setText(trId("audiotag.copyCmd"));
    m_batchQueueBtn->setText(trId("audiotag.batchQueue"));

    m_presetCombo->setItemText(0, trId("audiotag.preset.mp3_320k"));
    m_presetCombo->setItemText(1, trId("audiotag.preset.mp3_192k"));
    m_presetCombo->setItemText(2, trId("audiotag.preset.flac"));
    m_presetCombo->setItemText(3, trId("audiotag.preset.wav"));
    m_presetCombo->setItemText(4, trId("audiotag.preset.aac_256k"));
    m_presetCombo->setItemText(5, trId("audiotag.preset.opus_64k"));
}

void AudioTagStudioDialog::addFile(const QString &filePath)
{
    if (filePath.isEmpty() || !QFile::exists(filePath)) return;

    AudioFileItem item;
    item.filePath = filePath;
    item.sourceFormat = QFileInfo(filePath).suffix().toLower();

    if (m_probeService) {
        const MediaFile media = m_probeService->probe(filePath);
        item.durationSec = media.duration;
        item.metadata = Audio::AudioMetadata::parseTags(media.formatMetadata);
    }

    if (item.metadata.title.isEmpty()) {
        item.metadata = Audio::AudioMetadata::guessFromFileName(filePath);
    }

    m_items.append(item);
    rebuildTable();
}

void AudioTagStudioDialog::addFiles(const QStringList &filePaths)
{
    for (const QString &fp : filePaths) {
        if (!fp.isEmpty() && QFile::exists(fp)) {
            AudioFileItem item;
            item.filePath = fp;
            item.sourceFormat = QFileInfo(fp).suffix().toLower();
            if (m_probeService) {
                const MediaFile media = m_probeService->probe(fp);
                item.durationSec = media.duration;
                item.metadata = Audio::AudioMetadata::parseTags(media.formatMetadata);
            }
            if (item.metadata.title.isEmpty()) {
                item.metadata = Audio::AudioMetadata::guessFromFileName(fp);
            }
            m_items.append(item);
        }
    }
    rebuildTable();
}

void AudioTagStudioDialog::onAddFiles()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this, trId("audiotag.addFiles"), QString(),
        QStringLiteral("Media files (*.mp4 *.mkv *.mov *.webm *.avi *.flv *.ts *.mp3 *.flac *.wav *.aac *.m4a *.opus *.ogg);;All files (*)"));
    if (!files.isEmpty()) {
        addFiles(files);
    }
}

void AudioTagStudioDialog::onAddFolder()
{
    const QString dir = QFileDialog::getExistingDirectory(this, trId("audiotag.addFolder"));
    if (dir.isEmpty()) return;

    QStringList found;
    QDirIterator it(dir, QStringList{QStringLiteral("*.mp4"), QStringLiteral("*.mkv"), QStringLiteral("*.mov"),
                                     QStringLiteral("*.webm"), QStringLiteral("*.flv"), QStringLiteral("*.ts"),
                                     QStringLiteral("*.mp3"), QStringLiteral("*.flac"), QStringLiteral("*.wav"),
                                     QStringLiteral("*.aac"), QStringLiteral("*.m4a"), QStringLiteral("*.opus"),
                                     QStringLiteral("*.ogg")},
                    QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        found << it.next();
    }
    if (!found.isEmpty()) {
        addFiles(found);
    }
}

void AudioTagStudioDialog::onRemoveSelected()
{
    const auto ranges = m_table->selectedRanges();
    QList<int> rowsToRemove;
    for (const auto &range : ranges) {
        for (int r = range.topRow(); r <= range.bottomRow(); ++r) {
            if (!rowsToRemove.contains(r)) rowsToRemove.append(r);
        }
    }
    std::sort(rowsToRemove.begin(), rowsToRemove.end(), std::greater<int>());

    for (int r : rowsToRemove) {
        if (r >= 0 && r < m_items.size()) {
            m_items.removeAt(r);
        }
    }
    rebuildTable();
}

void AudioTagStudioDialog::onClearAll()
{
    m_items.clear();
    rebuildTable();
}

void AudioTagStudioDialog::onGuessFromFilenames()
{
    for (auto &item : m_items) {
        const auto guessed = Audio::AudioMetadata::guessFromFileName(item.filePath);
        if (!guessed.title.isEmpty()) item.metadata.title = guessed.title;
        if (!guessed.artist.isEmpty()) item.metadata.artist = guessed.artist;
    }
    rebuildTable();
}

void AudioTagStudioDialog::rebuildTable()
{
    m_table->blockSignals(true);
    m_table->setRowCount(m_items.size());

    for (int i = 0; i < m_items.size(); ++i) {
        const auto &it = m_items.at(i);
        const QFileInfo fi(it.filePath);

        auto *itemNum = new QTableWidgetItem(QString::number(i + 1));
        itemNum->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 0, itemNum);

        m_table->setItem(i, 1, new QTableWidgetItem(fi.fileName()));
        m_table->setItem(i, 2, new QTableWidgetItem(it.metadata.title));
        m_table->setItem(i, 3, new QTableWidgetItem(it.metadata.artist));
        m_table->setItem(i, 4, new QTableWidgetItem(it.metadata.album));
        m_table->setItem(i, 5, new QTableWidgetItem(it.sourceFormat.toUpper()));

        const int s = static_cast<int>(it.durationSec);
        const QString durStr = (s > 0) ? QStringLiteral("%1:%2")
                                             .arg(s / 60, 2, 10, QLatin1Char('0'))
                                             .arg(s % 60, 2, 10, QLatin1Char('0'))
                                       : QStringLiteral("--:--");
        auto *itemDur = new QTableWidgetItem(durStr);
        itemDur->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 6, itemDur);
    }

    m_table->blockSignals(false);
    m_fileCountLabel->setText(trId("audiotag.fileCount").arg(m_items.size()));

    if (!m_items.isEmpty() && m_table->currentRow() < 0) {
        m_table->selectRow(0);
    }
}

void AudioTagStudioDialog::onTableRowSelected()
{
    const int r = m_table->currentRow();
    if (r >= 0 && r < m_items.size()) {
        loadItemMetadataToEditor(r);
    }
}

void AudioTagStudioDialog::loadItemMetadataToEditor(int row)
{
    if (row < 0 || row >= m_items.size()) return;
    const auto &meta = m_items.at(row).metadata;

    m_isUpdatingEditor = true;
    m_titleEdit->setText(meta.title);
    m_artistEdit->setText(meta.artist);
    m_albumEdit->setText(meta.album);
    m_genreCombo->setEditText(meta.genre);
    m_yearEdit->setText(meta.year);
    m_trackEdit->setText(meta.track);
    m_commentEdit->setText(meta.comment);
    updateCoverThumbnail(meta.coverImagePath);
    m_isUpdatingEditor = false;
}

void AudioTagStudioDialog::syncEditorToItem(int row)
{
    if (row < 0 || row >= m_items.size() || m_isUpdatingEditor) return;

    auto &meta = m_items[row].metadata;
    meta.title = m_titleEdit->text().trimmed();
    meta.artist = m_artistEdit->text().trimmed();
    meta.album = m_albumEdit->text().trimmed();
    meta.genre = m_genreCombo->currentText().trimmed();
    meta.year = m_yearEdit->text().trimmed();
    meta.track = m_trackEdit->text().trimmed();
    meta.comment = m_commentEdit->text().trimmed();

    m_table->blockSignals(true);
    if (auto *itemTitle = m_table->item(row, 2)) itemTitle->setText(meta.title);
    if (auto *itemArtist = m_table->item(row, 3)) itemArtist->setText(meta.artist);
    if (auto *itemAlbum = m_table->item(row, 4)) itemAlbum->setText(meta.album);
    m_table->blockSignals(false);
}

void AudioTagStudioDialog::onMetadataFieldEdited()
{
    const int r = m_table->currentRow();
    if (r >= 0) {
        syncEditorToItem(r);
    }
}

void AudioTagStudioDialog::onApplyToAllSelected()
{
    const QString artist = m_artistEdit->text().trimmed();
    const QString album = m_albumEdit->text().trimmed();
    const QString genre = m_genreCombo->currentText().trimmed();
    const QString year = m_yearEdit->text().trimmed();
    const QString comment = m_commentEdit->text().trimmed();

    const auto ranges = m_table->selectedRanges();
    for (const auto &range : ranges) {
        for (int r = range.topRow(); r <= range.bottomRow(); ++r) {
            if (r >= 0 && r < m_items.size()) {
                auto &meta = m_items[r].metadata;
                if (!artist.isEmpty()) meta.artist = artist;
                if (!album.isEmpty()) meta.album = album;
                if (!genre.isEmpty()) meta.genre = genre;
                if (!year.isEmpty()) meta.year = year;
                if (!comment.isEmpty()) meta.comment = comment;
            }
        }
    }
    rebuildTable();
    QMessageBox::information(this, trId("common.success"), trId("audiotag.appliedNotice"));
}

void AudioTagStudioDialog::updateCoverThumbnail(const QString &imagePath)
{
    if (!imagePath.isEmpty() && QFile::exists(imagePath)) {
        QPixmap pix(imagePath);
        if (!pix.isNull()) {
            m_coverPreview->setPixmap(pix.scaled(m_coverPreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            return;
        }
    }
    m_coverPreview->setPixmap(QPixmap());
    m_coverPreview->setText(trId("audiotag.noCover"));
}

void AudioTagStudioDialog::onBrowseCover()
{
    const QString path = QFileDialog::getOpenFileName(
        this, trId("audiotag.browseCover"), QString(),
        QStringLiteral("Image files (*.jpg *.jpeg *.png *.bmp);;All files (*)"));
    if (path.isEmpty()) return;

    const int r = m_table->currentRow();
    if (r >= 0 && r < m_items.size()) {
        m_items[r].metadata.coverImagePath = path;
    }

    const auto ranges = m_table->selectedRanges();
    for (const auto &range : ranges) {
        for (int row = range.topRow(); row <= range.bottomRow(); ++row) {
            if (row >= 0 && row < m_items.size()) {
                m_items[row].metadata.coverImagePath = path;
            }
        }
    }

    updateCoverThumbnail(path);
}

void AudioTagStudioDialog::onExtractCover()
{
    const int r = m_table->currentRow();
    if (r < 0 || r >= m_items.size()) {
        QMessageBox::warning(this, trId("common.warning"), trId("audiotag.noFileSelected"));
        return;
    }

    const QString input = m_items[r].filePath;
    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString outputCover = QDir(tempDir).filePath(QStringLiteral("cover_%1.jpg").arg(QFileInfo(input).baseName()));

    const QStringList args = Audio::AudioTagEngine::buildCoverExtractCommand(input, outputCover);
    const QString ffmpegPath = m_locator ? m_locator->ffmpegPath() : QStringLiteral("ffmpeg");

    QProcess proc;
    proc.start(ffmpegPath, args);
    proc.waitForFinished(10000);

    if (proc.exitCode() == 0 && QFile::exists(outputCover)) {
        m_items[r].metadata.coverImagePath = outputCover;
        updateCoverThumbnail(outputCover);
        QMessageBox::information(this, trId("common.success"), trId("audiotag.coverExtractedNotice"));
    } else {
        QMessageBox::warning(this, trId("common.warning"), trId("audiotag.coverExtractFailed"));
    }
}

void AudioTagStudioDialog::onRemoveCover()
{
    const int r = m_table->currentRow();
    if (r >= 0 && r < m_items.size()) {
        m_items[r].metadata.coverImagePath.clear();
    }
    updateCoverThumbnail(QString());
}

void AudioTagStudioDialog::onPresetChanged(int index)
{
    m_options.applyPreset(static_cast<Audio::AudioQualityPreset>(index));
    m_formatCombo->blockSignals(true);
    m_formatCombo->setCurrentIndex(static_cast<int>(m_options.format));
    m_formatCombo->blockSignals(false);

    if (m_options.bitrateKbps > 0) {
        const int idx = m_bitrateCombo->findData(m_options.bitrateKbps);
        if (idx >= 0) m_bitrateCombo->setCurrentIndex(idx);
    }
}

void AudioTagStudioDialog::onFormatChanged(int index)
{
    m_options.format = static_cast<Audio::AudioFormat>(index);
    const bool isLossless = (m_options.format == Audio::AudioFormat::FLAC || m_options.format == Audio::AudioFormat::WAV);
    m_bitrateCombo->setEnabled(!isLossless);
}

void AudioTagStudioDialog::onCopyCurrentCommand()
{
    const int r = m_table->currentRow();
    if (r < 0 || r >= m_items.size()) {
        QMessageBox::warning(this, trId("common.warning"), trId("audiotag.noFileSelected"));
        return;
    }

    const auto &item = m_items.at(r);
    const QString ext = Audio::AudioTagEngine::formatToString(m_options.format);
    const QString output = OutputNaming::build(item.filePath, QStringLiteral("{name}.{format}"), ext);

    Audio::AudioExtractOptions opts = m_options;
    opts.bitrateKbps = m_bitrateCombo->currentData().toInt();
    opts.sampleRate = m_sampleRateCombo->currentData().toInt();
    opts.channels = m_channelsCombo->currentData().toInt();
    opts.normalizeLoudness = m_loudnormCheck->isChecked();
    opts.metadata = item.metadata;

    const QStringList args = Audio::AudioTagEngine::buildExtractCommand(item.filePath, output, opts);
    const QString ffmpegPath = m_locator ? m_locator->ffmpegPath() : QStringLiteral("ffmpeg");
    const QString fullCmd = ffmpegPath + QStringLiteral(" ") + args.join(QStringLiteral(" "));

    QGuiApplication::clipboard()->setText(fullCmd);
    QMessageBox::information(this, trId("common.success"), trId("audiotag.cmdCopiedNotice"));
}

void AudioTagStudioDialog::onBatchAddToQueue()
{
    if (m_items.isEmpty()) {
        QMessageBox::warning(this, trId("common.warning"), trId("audiotag.noFileSelected"));
        return;
    }

    if (!m_queue) return;

    Audio::AudioExtractOptions baseOpts = m_options;
    baseOpts.bitrateKbps = m_bitrateCombo->currentData().toInt();
    baseOpts.sampleRate = m_sampleRateCombo->currentData().toInt();
    baseOpts.channels = m_channelsCombo->currentData().toInt();
    baseOpts.normalizeLoudness = m_loudnormCheck->isChecked();

    const QString ext = Audio::AudioTagEngine::formatToString(baseOpts.format);
    const QString ffmpegPath = m_locator ? m_locator->ffmpegPath() : QStringLiteral("ffmpeg");

    for (const auto &item : m_items) {
        const QString output = OutputNaming::build(item.filePath, QStringLiteral("{name}.{format}"), ext);
        Audio::AudioExtractOptions itemOpts = baseOpts;
        itemOpts.metadata = item.metadata;

        const QStringList args = Audio::AudioTagEngine::buildExtractCommand(item.filePath, output, itemOpts);

        auto *job = new Job();
        job->setName(trId("audiotag.job.extract").arg(QFileInfo(item.filePath).fileName(), ext.toUpper()));
        job->setCommand(ffmpegPath, args);
        m_queue->addJob(job);
    }

    QMessageBox::information(this, trId("common.success"), trId("audiotag.batchEnqueuedNotice").arg(m_items.size()));
}
