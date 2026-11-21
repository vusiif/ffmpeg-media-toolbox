#pragma once

#include <QPointer>
#include <QWidget>

#include "ffmpeg/FFmpegCommandBuilder.h"
#include "media/MediaFile.h"

class LosslessCutDialog;
class FilterStudioDialog;
class FFmpegLocator;
class MediaProbeService;
class FFmpegCapabilities;
class RecentFiles;
class JobQueue;
class PresetManager;
class ElaText;
class ElaPushButton;
class ElaScrollArea;
class ConvertSourceCard;
class ConvertCodecCard;
class ConvertAdvancedCard;
class ConvertPreviewCard;

class ConvertPage : public QWidget
{
    Q_OBJECT

public:
    explicit ConvertPage(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                         MediaProbeService *probe, FFmpegCapabilities *caps,
                         RecentFiles *recentFiles, JobQueue *queue,
                         QWidget *parent = nullptr);
    ~ConvertPage() override = default;

    void retranslateUi();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onInputFileChanged(const QString &path);
    void onProbeFinished(const MediaFile &info, bool enqueueAfter);
    void updateCommandPreview();
    void onSavePresetRequested();
    void onLoadPreset(const QString &name);
    void enqueueJob(const MediaFile &info);
    void enqueueSample();
    void openABCompare();
    void openQualityDialog();
    void openLosslessCutDialog();
    void openFilterStudioDialog();
    void addToQueue();

private:
    FFmpegCommandBuilder::ConversionOptions currentOptions(const QString &outputPath, bool copy) const;

    FFmpegLocator *m_locator = nullptr;
    FFmpegCommandBuilder *m_builder = nullptr;
    MediaProbeService *m_probe = nullptr;
    FFmpegCapabilities *m_caps = nullptr;
    JobQueue *m_queue = nullptr;
    PresetManager *m_presets = nullptr;

    QPointer<LosslessCutDialog> m_cutDialog;
    QPointer<FilterStudioDialog> m_filterDialog;

    QString m_inputFile;
    MediaFile m_probeInfo;
    QString m_lastSamplePath;

    ElaText *m_titleLabel = nullptr;
    ElaScrollArea *m_scrollArea = nullptr;

    ConvertSourceCard *m_sourceCard = nullptr;
    ConvertCodecCard *m_codecCard = nullptr;
    ConvertAdvancedCard *m_advCard = nullptr;
    ConvertPreviewCard *m_previewCard = nullptr;

    ElaPushButton *m_sampleButton = nullptr;
    ElaPushButton *m_abCompareButton = nullptr;
    ElaPushButton *m_qualityButton = nullptr;
    ElaPushButton *m_losslessCutButton = nullptr;
    ElaPushButton *m_filterStudioButton = nullptr;
    ElaPushButton *m_addButton = nullptr;
    bool m_isDragHover = false;
};
