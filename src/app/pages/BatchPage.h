#pragma once

#include <QStringList>
#include <QWidget>

#include "ffmpeg/FFmpegCommandBuilder.h"

class FFmpegLocator;
class FFmpegCapabilities;
class JobQueue;
class PresetManager;
class QListWidget;
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
    void removeSelectedFiles();
    void clearFiles();
    void browseOutputDir();
    void addAllToQueue();
    void rebuildCodecLists();
    void refreshPresetList();
    void loadPreset(const QString &name);
    void updateCountLabel();
    FFmpegCommandBuilder::ConversionOptions currentOptions(
        const QString &inputFile, const QString &outputPath, bool copy) const;

    FFmpegLocator *m_locator;
    FFmpegCommandBuilder *m_builder;
    FFmpegCapabilities *m_caps;
    JobQueue *m_queue;
    PresetManager *m_presets = nullptr;

    QStringList m_inputFiles;
    QString m_outputDirectory;

    ElaText *m_titleLabel = nullptr;
    QListWidget *m_fileList = nullptr;
    ElaPushButton *m_addButton = nullptr;
    ElaPushButton *m_removeButton = nullptr;
    ElaPushButton *m_clearButton = nullptr;
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

    ElaPushButton *m_queueButton = nullptr;
};
