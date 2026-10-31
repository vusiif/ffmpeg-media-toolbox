#pragma once

#include <QFutureWatcher>
#include <QWidget>

#include "ffmpeg/FFmpegCommandBuilder.h"
#include "media/MediaFile.h"

class FFmpegLocator;
class FFmpegCommandBuilder;
class MediaProbeService;
class FFmpegCapabilities;
class RecentFiles;
class JobQueue;
class PresetManager;
class ElaText;
class ElaPushButton;
class ElaLineEdit;
class ElaComboBox;
class ElaSpinBox;
class ElaDoubleSpinBox;
class ElaCheckBox;
class ElaScrollArea;
class ElaScrollPageArea;

class ConvertPage : public QWidget
{
    Q_OBJECT

public:
    explicit ConvertPage(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                         MediaProbeService *probe, FFmpegCapabilities *caps,
                         RecentFiles *recentFiles, JobQueue *queue,
                         QWidget *parent = nullptr);

    void retranslateUi();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void browseInput();
    void startProbe(bool enqueueAfter);
    void onProbeFinished();
    void enqueueJob(const MediaFile &info);
    void addToQueue();
    void updateCommandPreview();
    void rebuildCodecLists();
    void showMetadata(const QVariantMap &tags);
    void refreshRecentList();
    void loadRecentFile(const QString &path);
    void savePreset();
    void loadPreset(const QString &name);
    void deletePreset();
    void refreshPresetList();
    FFmpegCommandBuilder::ConversionOptions currentOptions(const QString &outputPath, bool copy) const;

    FFmpegLocator *m_locator;
    FFmpegCommandBuilder *m_builder;
    MediaProbeService *m_probe;
    FFmpegCapabilities *m_caps;
    RecentFiles *m_recentFiles;
    JobQueue *m_queue;
    PresetManager *m_presets;

    QString m_inputFile;
    bool m_enqueueAfterProbe = false;
    QFutureWatcher<MediaFile> *m_probeWatcher = nullptr;
    MediaFile m_probeInfo;

    ElaText *m_titleLabel = nullptr;
    ElaScrollArea *m_scrollArea = nullptr;

    // Card 1: Media Source & Probing
    ElaScrollPageArea *m_inputCard = nullptr;
    ElaText *m_inputCardTitle = nullptr;
    ElaText *m_inputCaption = nullptr;
    ElaLineEdit *m_inputEdit = nullptr;
    ElaPushButton *m_browseButton = nullptr;
    ElaText *m_recentCaption = nullptr;
    ElaComboBox *m_recentCombo = nullptr;
    ElaText *m_infoLabel = nullptr;
    ElaText *m_metaCaption = nullptr;
    ElaText *m_metaValue = nullptr;

    // Card 2: Presets & Encoding Settings
    ElaScrollPageArea *m_codecCard = nullptr;
    ElaText *m_codecCardTitle = nullptr;
    ElaText *m_presetCaption = nullptr;
    ElaComboBox *m_presetCombo = nullptr;
    ElaPushButton *m_presetSaveButton = nullptr;
    ElaPushButton *m_presetDeleteButton = nullptr;
    ElaText *m_formatCaption = nullptr;
    ElaComboBox *m_formatCombo = nullptr;
    ElaText *m_codecCaption = nullptr;
    ElaComboBox *m_codecCombo = nullptr;
    ElaText *m_audioCodecCaption = nullptr;
    ElaComboBox *m_audioCodecCombo = nullptr;
    ElaText *m_rateControlCaption = nullptr;
    ElaComboBox *m_rateControlCombo = nullptr;
    ElaText *m_qualityCaption = nullptr;
    ElaSpinBox *m_qualitySpin = nullptr;
    ElaText *m_targetSizeCaption = nullptr;
    QWidget *m_targetSizeWidget = nullptr;
    ElaDoubleSpinBox *m_targetSizeSpin = nullptr;
    ElaPushButton *m_preset25Btn = nullptr;
    ElaPushButton *m_preset50Btn = nullptr;
    ElaPushButton *m_preset100Btn = nullptr;
    ElaCheckBox *m_smartCopyCheck = nullptr;
    ElaCheckBox *m_stripMetaCheck = nullptr;
    ElaCheckBox *m_hwAccelCheck = nullptr;
    ElaText *m_hwDeviceLabel = nullptr;

    // Card 3: Advanced Media & Filters
    ElaScrollPageArea *m_advCard = nullptr;
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

    // Card 4: Preview & Estimate
    ElaScrollPageArea *m_previewCard = nullptr;
    ElaText *m_previewCardTitle = nullptr;
    ElaText *m_estLabel = nullptr;
    ElaText *m_previewCaption = nullptr;
    ElaLineEdit *m_previewEdit = nullptr;

    // Bottom Action Row
    ElaPushButton *m_addButton = nullptr;
};
