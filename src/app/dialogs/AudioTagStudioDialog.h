#pragma once

#include <QDialog>
#include <QFutureWatcher>
#include <QList>
#include <QString>
#include <QWidget>

#include "audio/AudioTagEngine.h"

class JobQueue;
class FFmpegLocator;
class MediaProbeService;
class ElaText;
class ElaPushButton;
class ElaLineEdit;
class ElaComboBox;
class ElaCheckBox;
class ElaSpinBox;
class QTableWidget;
class QLabel;

struct AudioFileItem {
    QString filePath;
    Audio::AudioMetadata metadata;
    double durationSec = 0.0;
    QString sourceFormat;
    bool isProbed = false;
    bool userModified = false;
};

class AudioTagStudioDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AudioTagStudioDialog(JobQueue *queue, FFmpegLocator *locator,
                                  MediaProbeService *probeService = nullptr,
                                  QWidget *parent = nullptr);
    ~AudioTagStudioDialog() override;

    void addFile(const QString &filePath);
    void addFiles(const QStringList &filePaths);
    void retranslateUi();

private slots:
    void onAddFiles();
    void onAddFolder();
    void onRemoveSelected();
    void onClearAll();
    void onGuessFromFilenames();

    void onTableRowSelected();
    void onMetadataFieldEdited();
    void onApplyToAllSelected();

    void onBrowseCover();
    void onExtractCover();
    void onRemoveCover();

    void onPresetChanged(int index);
    void onFormatChanged(int index);
    void onProbeBatchFinished();

    void onCopyCurrentCommand();
    void onBatchAddToQueue();

private:
    void rebuildTable();
    void loadItemMetadataToEditor(int row);
    void syncEditorToItem(int row);
    void updateCoverThumbnail(const QString &imagePath);

    JobQueue *m_queue = nullptr;
    FFmpegLocator *m_locator = nullptr;
    MediaProbeService *m_probeService = nullptr;

    struct ProbeResult {
        QString filePath;
        double durationSec = 0.0;
        Audio::AudioMetadata metadata;
    };

    QFutureWatcher<QList<ProbeResult>> *m_probeWatcher = nullptr;
    QList<AudioFileItem> m_items;
    Audio::AudioExtractOptions m_options;

    // Header
    ElaText *m_titleLabel = nullptr;
    ElaText *m_subtitleLabel = nullptr;

    // Left Column: Table & Table Tools
    QTableWidget *m_table = nullptr;
    ElaPushButton *m_addFilesBtn = nullptr;
    ElaPushButton *m_addFolderBtn = nullptr;
    ElaPushButton *m_removeSelectedBtn = nullptr;
    ElaPushButton *m_guessMetaBtn = nullptr;
    ElaPushButton *m_clearAllBtn = nullptr;
    QLabel *m_fileCountLabel = nullptr;

    // Right Column: Metadata Card
    QLabel *m_coverPreview = nullptr;
    ElaPushButton *m_browseCoverBtn = nullptr;
    ElaPushButton *m_extractCoverBtn = nullptr;
    ElaPushButton *m_removeCoverBtn = nullptr;

    ElaLineEdit *m_titleEdit = nullptr;
    ElaLineEdit *m_artistEdit = nullptr;
    ElaLineEdit *m_albumEdit = nullptr;
    ElaComboBox *m_genreCombo = nullptr;
    ElaLineEdit *m_yearEdit = nullptr;
    ElaLineEdit *m_trackEdit = nullptr;
    ElaLineEdit *m_commentEdit = nullptr;
    ElaPushButton *m_applySelectedBtn = nullptr;

    // Right Column: Format & Extraction Card
    ElaComboBox *m_presetCombo = nullptr;
    ElaComboBox *m_formatCombo = nullptr;
    ElaComboBox *m_bitrateCombo = nullptr;
    ElaComboBox *m_sampleRateCombo = nullptr;
    ElaComboBox *m_channelsCombo = nullptr;
    ElaCheckBox *m_loudnormCheck = nullptr;

    // Action Buttons
    ElaPushButton *m_copyCmdBtn = nullptr;
    ElaPushButton *m_batchQueueBtn = nullptr;

    bool m_isUpdatingEditor = false;
};
