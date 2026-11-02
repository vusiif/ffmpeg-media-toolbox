#pragma once

#include <QDialog>
#include <QString>
#include <QWidget>

#include "compositor/MultiTrackCompositor.h"

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

class PipPreviewCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit PipPreviewCanvas(QWidget *parent = nullptr);

    void setConfig(const Compositor::CompositorConfig &config);
    const Compositor::CompositorConfig &config() const { return m_config; }

signals:
    void configChanged(const Compositor::CompositorConfig &newConfig);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QRect canvasRect() const;
    QRect pipRect(const QRect &canvas) const;

    Compositor::CompositorConfig m_config;
    bool m_dragging = false;
    QPoint m_dragOffset;
};

class PipCompositorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PipCompositorDialog(JobQueue *queue, FFmpegLocator *locator,
                                 QWidget *parent = nullptr);
    ~PipCompositorDialog() override;

    void setMainVideo(const QString &mainPath);
    void setPipVideo(const QString &pipPath);
    void retranslateUi();

private slots:
    void onBrowseMain();
    void onBrowsePip();
    void onLayoutModeChanged(int index);
    void onPositionPresetChanged(int index);
    void onScaleSliderChanged(int value);
    void onMarginChanged(int value);
    void onBorderToggled(bool checked);
    void onBorderWidthChanged(int value);
    void onBorderColorChanged(int index);
    void onTimeRangeToggled(bool checked);
    void onStartTimeChanged(double value);
    void onDurationChanged(double value);
    void onResolutionChanged(int index);
    void onSplitRatioSliderChanged(int value);
    void onAudioPolicyChanged(int index);
    void onMainVolumeSliderChanged(int value);
    void onPipVolumeSliderChanged(int value);
    void onCanvasConfigChanged(const Compositor::CompositorConfig &newConfig);

    void onCopyCommand();
    void onAddToQueue();

private:
    void updateUiFromConfig();
    void syncConfigFromUi();
    void refreshPreview();

    JobQueue *m_queue = nullptr;
    FFmpegLocator *m_locator = nullptr;

    Compositor::CompositorConfig m_config;
    QString m_mainPath;
    QString m_pipPath;

    // Header
    ElaText *m_titleLabel = nullptr;
    ElaText *m_subtitleLabel = nullptr;

    // Left Column
    ElaLineEdit *m_mainPathEdit = nullptr;
    ElaPushButton *m_mainBrowseBtn = nullptr;
    ElaLineEdit *m_pipPathEdit = nullptr;
    ElaPushButton *m_pipBrowseBtn = nullptr;
    PipPreviewCanvas *m_canvas = nullptr;
    VideoPlayerWidget *m_player = nullptr;

    // Right Column: Layout Card
    ElaComboBox *m_layoutCombo = nullptr;
    QWidget *m_pipParamWidget = nullptr;
    ElaComboBox *m_positionCombo = nullptr;
    ElaSlider *m_scaleSlider = nullptr;
    QLabel *m_scaleLabel = nullptr;
    ElaSpinBox *m_marginSpin = nullptr;
    ElaCheckBox *m_borderCheck = nullptr;
    ElaSpinBox *m_borderWidthSpin = nullptr;
    ElaComboBox *m_borderColorCombo = nullptr;
    ElaCheckBox *m_timeRangeCheck = nullptr;
    ElaDoubleSpinBox *m_startTimeSpin = nullptr;
    ElaDoubleSpinBox *m_durationSpin = nullptr;

    QWidget *m_splitParamWidget = nullptr;
    ElaComboBox *m_resolutionCombo = nullptr;
    ElaSlider *m_splitRatioSlider = nullptr;
    QLabel *m_splitRatioLabel = nullptr;

    // Right Column: Audio Card
    ElaComboBox *m_audioPolicyCombo = nullptr;
    ElaSlider *m_mainVolumeSlider = nullptr;
    QLabel *m_mainVolumeLabel = nullptr;
    ElaSlider *m_pipVolumeSlider = nullptr;
    QLabel *m_pipVolumeLabel = nullptr;

    // Right Column: Output Card
    ElaComboBox *m_videoCodecCombo = nullptr;
    ElaSpinBox *m_crfSpin = nullptr;
    ElaCheckBox *m_shortestCheck = nullptr;

    ElaPushButton *m_copyCmdBtn = nullptr;
    ElaPushButton *m_addToQueueBtn = nullptr;
};
