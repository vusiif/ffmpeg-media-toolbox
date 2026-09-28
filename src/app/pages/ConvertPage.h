#pragma once

#include <QFutureWatcher>
#include <QWidget>

#include "media/MediaFile.h"

class FFmpegLocator;
class FFmpegCommandBuilder;
class MediaProbeService;
class FFmpegCapabilities;
class JobQueue;
class PresetManager;
class ElaText;
class ElaPushButton;
class ElaLineEdit;
class ElaComboBox;
class ElaSpinBox;
class ElaCheckBox;

class ConvertPage : public QWidget
{
    Q_OBJECT

public:
    explicit ConvertPage(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                         MediaProbeService *probe, FFmpegCapabilities *caps,
                         JobQueue *queue, QWidget *parent = nullptr);

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
    void savePreset();
    void loadPreset(const QString &name);
    void deletePreset();
    void refreshPresetList();

    FFmpegLocator *m_locator;
    FFmpegCommandBuilder *m_builder;
    MediaProbeService *m_probe;
    FFmpegCapabilities *m_caps;
    JobQueue *m_queue;
    PresetManager *m_presets;

    QString m_inputFile;
    bool m_enqueueAfterProbe = false;
    QFutureWatcher<MediaFile> *m_probeWatcher = nullptr;
    MediaFile m_probeInfo;

    ElaText *m_titleLabel = nullptr;
    ElaText *m_inputCaption = nullptr;
    ElaLineEdit *m_inputEdit = nullptr;
    ElaPushButton *m_browseButton = nullptr;
    ElaText *m_infoLabel = nullptr;
    ElaText *m_metaCaption = nullptr;
    ElaText *m_metaValue = nullptr;
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
    ElaText *m_qualityCaption = nullptr;
    ElaSpinBox *m_qualitySpin = nullptr;
    ElaCheckBox *m_smartCopyCheck = nullptr;
    ElaCheckBox *m_stripMetaCheck = nullptr;
    ElaText *m_previewCaption = nullptr;
    ElaLineEdit *m_previewEdit = nullptr;
    ElaPushButton *m_addButton = nullptr;
};
