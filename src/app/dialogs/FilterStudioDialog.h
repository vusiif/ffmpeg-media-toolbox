#pragma once

#include <QDialog>
#include <QProcess>

#include "filters/MediaFilterStudio.h"

class FFmpegLocator;
class MediaProbeService;
class JobQueue;
class VideoPlayerWidget;
class ElaText;
class ElaPushButton;
class ElaLineEdit;
class ElaComboBox;
class ElaSlider;
class ElaDoubleSpinBox;
class ElaSpinBox;
class ElaCheckBox;
class ElaScrollArea;
class ElaScrollPageArea;
class ElaProgressBar;
class QStackedWidget;
class ElaPivot;

class FilterStudioDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilterStudioDialog(FFmpegLocator *locator, MediaProbeService *probeService,
                                JobQueue *queue, QWidget *parent = nullptr);
    ~FilterStudioDialog() override;

    void loadFile(const QString &filePath);
    void retranslateUi();

    ColorGradingOptions currentVideoOptions() const { return m_videoOpts; }
    AudioFxOptions currentAudioOptions() const { return m_audioOpts; }

signals:
    void filtersApplied(const QString &videoFilter, const QString &audioFilter);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onBrowseFile();
    void onBrowseLut();
    void onClearLut();
    void onPresetChanged(int index);
    void onStandardChanged(int index);
    void onVideoControlChanged();
    void onAudioControlChanged();
    void onResetAll();
    void onGeneratePreview();
    void onPreviewProcessFinished(int exitCode);
    void onTogglePreviewMode();
    void onApplyToConvert();
    void onExportClicked();

private:
    void syncUiFromOptions();
    void updateFilterStrings();

    FFmpegLocator *m_locator;
    MediaProbeService *m_probeService;
    JobQueue *m_queue;

    QString m_inputFile;
    QString m_previewSampleFile;
    bool m_isPreviewMode = false;
    QProcess *m_previewProcess = nullptr;

    ColorGradingOptions m_videoOpts;
    AudioFxOptions m_audioOpts;

    // UI Widgets
    ElaText *m_titleLabel = nullptr;
    ElaText *m_subtitleLabel = nullptr;
    ElaLineEdit *m_filePathEdit = nullptr;
    ElaPushButton *m_browseButton = nullptr;
    ElaText *m_metaInfoLabel = nullptr;

    // Player & Preview controls
    VideoPlayerWidget *m_player = nullptr;
    ElaPushButton *m_genPreviewBtn = nullptr;
    ElaPushButton *m_togglePreviewBtn = nullptr;
    ElaProgressBar *m_previewProgress = nullptr;
    ElaText *m_previewStatusLabel = nullptr;

    // Tabs
    ElaPivot *m_pivot = nullptr;
    QStackedWidget *m_stackedWidget = nullptr;
    QString m_tabColorTitle;
    QString m_tabEnhancementsTitle;
    QString m_tabAudioTitle;

    // Tab 1: Color & LUT
    ElaComboBox *m_presetCombo = nullptr;
    ElaLineEdit *m_lutPathEdit = nullptr;
    ElaPushButton *m_browseLutBtn = nullptr;
    ElaPushButton *m_clearLutBtn = nullptr;

    ElaDoubleSpinBox *m_brightnessSpin = nullptr;
    ElaDoubleSpinBox *m_contrastSpin = nullptr;
    ElaDoubleSpinBox *m_saturationSpin = nullptr;
    ElaDoubleSpinBox *m_gammaSpin = nullptr;
    ElaDoubleSpinBox *m_temperatureSpin = nullptr;
    ElaDoubleSpinBox *m_tintSpin = nullptr;

    // Tab 2: Video Enhancements
    ElaCheckBox *m_denoiseCheck = nullptr;
    ElaDoubleSpinBox *m_denoiseSpin = nullptr;
    ElaCheckBox *m_sharpenCheck = nullptr;
    ElaDoubleSpinBox *m_sharpenSpin = nullptr;
    ElaCheckBox *m_vignetteCheck = nullptr;
    ElaDoubleSpinBox *m_vignetteSpin = nullptr;
    ElaCheckBox *m_deinterlaceCheck = nullptr;

    // Tab 3: Audio FX
    ElaCheckBox *m_loudnormCheck = nullptr;
    ElaComboBox *m_loudnormStandardCombo = nullptr;
    ElaDoubleSpinBox *m_targetLufsSpin = nullptr;
    ElaCheckBox *m_audioDenoiseCheck = nullptr;
    ElaDoubleSpinBox *m_noiseReductionSpin = nullptr;
    ElaCheckBox *m_highpassCheck = nullptr;
    ElaSpinBox *m_highpassSpin = nullptr;
    ElaCheckBox *m_lowpassCheck = nullptr;
    ElaSpinBox *m_lowpassSpin = nullptr;
    ElaDoubleSpinBox *m_volumeDbSpin = nullptr;

    // Filter Code Displays & Bottom Actions
    ElaText *m_vfPreviewLabel = nullptr;
    ElaText *m_afPreviewLabel = nullptr;

    ElaPushButton *m_resetBtn = nullptr;
    ElaPushButton *m_applyBtn = nullptr;
    ElaPushButton *m_exportBtn = nullptr;
    ElaPushButton *m_closeBtn = nullptr;
};
