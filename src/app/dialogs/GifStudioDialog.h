#pragma once

#include <QDialog>
#include <QString>
#include <QWidget>

#include "animation/AnimatedImageEngine.h"

class JobQueue;
class FFmpegLocator;
class VideoPlayerWidget;
class ElaText;
class ElaPushButton;
class ElaLineEdit;
class ElaSpinBox;
class ElaDoubleSpinBox;
class ElaComboBox;
class ElaCheckBox;
class ElaSlider;
class QLabel;

class GifStudioDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GifStudioDialog(JobQueue *queue, FFmpegLocator *locator,
                             QWidget *parent = nullptr);
    ~GifStudioDialog() override;

    void setVideoFile(const QString &filePath);
    void retranslateUi();

private slots:
    void onBrowseFile();
    void onSetInPoint();
    void onSetOutPoint();
    void onPreviewRange();
    void onStartSpinChanged(double val);
    void onDurationSpinChanged(double val);
    void onEndSpinChanged(double val);

    void onFormatChanged(int index);
    void onPresetChanged(int index);
    void onResolutionPresetChanged(int index);
    void onWidthChanged(int val);
    void onFpsChanged(int index);
    void onSpeedChanged(int index);
    void onDirectionChanged(int index);

    void onMaxColorsChanged(int val);
    void onDitherModeChanged(int index);
    void onBayerScaleChanged(int val);
    void onStatsModeChanged(int index);
    void onLoopCountChanged(int val);

    void onWebpQualityChanged(int val);
    void onWebpLosslessToggled(bool checked);

    void onCopyCommand();
    void onAddToQueue();

private:
    void updateUiFromConfig();
    void syncConfigFromUi();
    void updateEstimateBanner();

    JobQueue *m_queue = nullptr;
    FFmpegLocator *m_locator = nullptr;

    Animation::AnimatedImageConfig m_config;
    QString m_videoPath;

    // Header
    ElaText *m_titleLabel = nullptr;
    ElaText *m_subtitleLabel = nullptr;

    // Left Column: Player & Range Selection
    ElaLineEdit *m_pathEdit = nullptr;
    ElaPushButton *m_browseBtn = nullptr;
    VideoPlayerWidget *m_player = nullptr;

    ElaPushButton *m_setInBtn = nullptr;
    ElaPushButton *m_setOutBtn = nullptr;
    ElaPushButton *m_previewRangeBtn = nullptr;
    ElaDoubleSpinBox *m_startSpin = nullptr;
    ElaDoubleSpinBox *m_durationSpin = nullptr;
    ElaDoubleSpinBox *m_endSpin = nullptr;

    // Live Stats Banner
    QLabel *m_estimateBanner = nullptr;

    // Right Column: Controls
    ElaComboBox *m_formatCombo = nullptr;
    ElaComboBox *m_presetCombo = nullptr;

    // Geometry & Timing
    ElaComboBox *m_resPresetCombo = nullptr;
    ElaSpinBox *m_widthSpin = nullptr;
    ElaComboBox *m_fpsCombo = nullptr;
    ElaComboBox *m_speedCombo = nullptr;
    ElaComboBox *m_directionCombo = nullptr;

    // GIF Palette Options
    QWidget *m_gifPaletteWidget = nullptr;
    ElaSpinBox *m_colorsSpin = nullptr;
    ElaComboBox *m_ditherCombo = nullptr;
    ElaSlider *m_bayerSlider = nullptr;
    QLabel *m_bayerLabel = nullptr;
    ElaComboBox *m_statsModeCombo = nullptr;
    ElaSpinBox *m_loopSpin = nullptr;

    // WebP Options
    QWidget *m_webpWidget = nullptr;
    ElaSlider *m_webpQualitySlider = nullptr;
    QLabel *m_webpQualityLabel = nullptr;
    ElaCheckBox *m_webpLosslessCheck = nullptr;

    // Action Buttons
    ElaPushButton *m_copyCmdBtn = nullptr;
    ElaPushButton *m_addToQueueBtn = nullptr;
};
