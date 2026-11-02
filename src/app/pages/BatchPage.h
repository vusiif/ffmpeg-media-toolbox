#pragma once

#include <QStringList>
#include <QWidget>

#include "ffmpeg/FFmpegCommandBuilder.h"
#include "pipeline/BatchPipeline.h"

class FFmpegLocator;
class FFmpegCapabilities;
class JobQueue;
class PresetManager;
class PipelineManager;
class FolderWatcherService;
class ElaListView;
class QStringListModel;
class ElaText;
class ElaPushButton;
class ElaComboBox;
class ElaSpinBox;
class ElaCheckBox;
class ElaLineEdit;

class BatchPage : public QWidget
{
    Q_OBJECT

public:
    explicit BatchPage(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                       FFmpegCapabilities *caps, JobQueue *queue,
                       QWidget *parent = nullptr);

    void retranslateUi();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void addFiles();
    void addFolder();
    void removeSelectedFiles();
    void clearFiles();
    void syncFileListModel();
    void browseOutputDir();
    void addAllToQueue();
    void toggleFolderWatcher();
    void onWatcherFileReady(const QString &filePath, const QString &relativeSubDir);
    void rebuildCodecLists();
    void refreshPresetList();
    void loadPreset(const QString &name);
    void updateCountLabel();

    // Custom Batch Pipeline methods
    void refreshPipelineTemplates();
    void loadPipelineTemplate(const QString &name);
    void savePipelineTemplate();
    void deletePipelineTemplate();
    PipelineStep currentPipelineStep() const;
    void applyPipelineStepToUi(const PipelineStep &step);

    FFmpegCommandBuilder::ConversionOptions currentOptions(
        const QString &inputFile, const QString &outputPath, bool copy) const;

    FFmpegLocator *m_locator;
    FFmpegCommandBuilder *m_builder;
    FFmpegCapabilities *m_caps;
    JobQueue *m_queue;
    PresetManager *m_presets = nullptr;
    PipelineManager *m_pipelineManager = nullptr;

    FolderWatcherService *m_watcherService = nullptr;
    QMap<QString, QString> m_relativeSubDirs;

    QStringList m_inputFiles;
    QString m_outputDirectory;

    ElaText *m_titleLabel = nullptr;
    ElaText *m_listHeader = nullptr;
    ElaListView *m_fileView = nullptr;
    QStringListModel *m_fileModel = nullptr;
    ElaPushButton *m_addButton = nullptr;
    ElaPushButton *m_addFolderButton = nullptr;
    ElaPushButton *m_removeButton = nullptr;
    ElaPushButton *m_clearButton = nullptr;
    ElaCheckBox *m_preserveTreeCheck = nullptr;
    ElaText *m_countLabel = nullptr;

    // Output Directory
    ElaText *m_outputDirCaption = nullptr;
    ElaPushButton *m_outputDirButton = nullptr;

    // Preset
    ElaText *m_presetCaption = nullptr;
    ElaComboBox *m_presetCombo = nullptr;

    // Basic format & codecs
    ElaText *m_formatCaption = nullptr;
    ElaComboBox *m_formatCombo = nullptr;
    ElaText *m_codecCaption = nullptr;
    ElaComboBox *m_codecCombo = nullptr;
    ElaText *m_audioCodecCaption = nullptr;
    ElaComboBox *m_audioCodecCombo = nullptr;
    ElaText *m_qualityCaption = nullptr;
    ElaSpinBox *m_qualitySpin = nullptr;
    ElaCheckBox *m_smartCopyCheck = nullptr;
    ElaCheckBox *m_stripMetaCheck = nullptr;
    ElaCheckBox *m_hwAccelCheck = nullptr;
    ElaText *m_hwDeviceLabel = nullptr;

    // Advanced options
    ElaCheckBox *m_advToggle = nullptr;
    QWidget *m_advContainer = nullptr;
    ElaText *m_audioBitrateCaption = nullptr;
    ElaComboBox *m_audioBitrateCombo = nullptr;
    ElaText *m_audioSamplerateCaption = nullptr;
    ElaComboBox *m_audioSamplerateCombo = nullptr;
    ElaText *m_audioChannelsCaption = nullptr;
    ElaComboBox *m_audioChannelsCombo = nullptr;
    ElaCheckBox *m_audioNormalizeCheck = nullptr;
    ElaText *m_speedPresetCaption = nullptr;
    ElaComboBox *m_speedPresetCombo = nullptr;
    ElaText *m_pixFmtCaption = nullptr;
    ElaComboBox *m_pixFmtCombo = nullptr;
    ElaText *m_colorSpaceCaption = nullptr;
    ElaComboBox *m_colorSpaceCombo = nullptr;
    ElaText *m_hdrTonemapCaption = nullptr;
    ElaComboBox *m_hdrTonemapCombo = nullptr;
    ElaText *m_vfCaption = nullptr;
    ElaLineEdit *m_vfEdit = nullptr;
    ElaText *m_afCaption = nullptr;
    ElaLineEdit *m_afEdit = nullptr;

    // Batch Processing Pipeline Card
    ElaCheckBox *m_pipelineToggle = nullptr;
    QWidget *m_pipelineContainer = nullptr;

    ElaText *m_pipelineTemplateCaption = nullptr;
    ElaComboBox *m_pipelineTemplateCombo = nullptr;
    ElaPushButton *m_pipelineSaveBtn = nullptr;
    ElaPushButton *m_pipelineDeleteBtn = nullptr;

    // Pipeline Steps UI
    ElaCheckBox *m_pipeTrimCheck = nullptr;
    ElaLineEdit *m_pipeStartEdit = nullptr;
    ElaLineEdit *m_pipeEndEdit = nullptr;

    ElaCheckBox *m_pipeCropCheck = nullptr;
    ElaComboBox *m_pipeCropPresetCombo = nullptr;
    ElaSpinBox *m_pipeCropXSpin = nullptr;
    ElaSpinBox *m_pipeCropYSpin = nullptr;
    ElaSpinBox *m_pipeCropWSpin = nullptr;
    ElaSpinBox *m_pipeCropHSpin = nullptr;

    ElaCheckBox *m_pipeScaleCheck = nullptr;
    ElaComboBox *m_pipeScaleCombo = nullptr;
    ElaSpinBox *m_pipeScaleWSpin = nullptr;
    ElaSpinBox *m_pipeScaleHSpin = nullptr;

    ElaCheckBox *m_pipeFpsCheck = nullptr;
    ElaComboBox *m_pipeFpsCombo = nullptr;

    ElaCheckBox *m_pipeWatermarkCheck = nullptr;
    ElaLineEdit *m_pipeWatermarkPathEdit = nullptr;
    ElaPushButton *m_pipeWatermarkBrowseBtn = nullptr;
    ElaComboBox *m_pipeWatermarkPosCombo = nullptr;
    ElaSpinBox *m_pipeWatermarkOpacitySpin = nullptr;

    ElaCheckBox *m_pipeAudioCheck = nullptr;
    ElaSpinBox *m_pipeAudioVolSpin = nullptr;
    ElaCheckBox *m_pipeAudioLoudnormCheck = nullptr;
    ElaCheckBox *m_pipeAudioMuteCheck = nullptr;

    // Card 5: Folder Auto-Watch Transcoding Card
    ElaCheckBox *m_watcherToggle = nullptr;
    QWidget *m_watcherContainer = nullptr;
    ElaLineEdit *m_watchPathEdit = nullptr;
    ElaPushButton *m_watchBrowseBtn = nullptr;
    ElaPushButton *m_watchActionBtn = nullptr;
    ElaText *m_watchStatusLabel = nullptr;

    // Card 6: Multi-Export Target Matrix Card
    ElaCheckBox *m_matrixToggle = nullptr;
    QWidget *m_matrixContainer = nullptr;
    ElaCheckBox *m_target1Check = nullptr;
    ElaCheckBox *m_target2Check = nullptr;
    ElaCheckBox *m_target3Check = nullptr;
    ElaCheckBox *m_target4Check = nullptr;

    ElaPushButton *m_queueButton = nullptr;
};
