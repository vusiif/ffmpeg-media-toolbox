#pragma once

#include <QString>

#include "ElaScrollPageArea.h"
#include "pipeline/BatchPipeline.h"

class PipelineManager;
class ElaCheckBox;
class ElaText;
class ElaComboBox;
class ElaPushButton;
class ElaLineEdit;
class ElaSpinBox;

class BatchPipelineCard : public ElaScrollPageArea
{
    Q_OBJECT

public:
    explicit BatchPipelineCard(QWidget *parent = nullptr);
    ~BatchPipelineCard() override = default;

    bool isPipelineEnabled() const;
    PipelineStep currentPipelineStep(const QString &format = QString(),
                                     const QString &vcodec = QString(),
                                     const QString &acodec = QString(),
                                     int crf = -1,
                                     const QString &audioBitrate = QString(),
                                     const QString &speedPreset = QString(),
                                     bool hwAccel = false) const;
    void applyPipelineStepToUi(const PipelineStep &step);

    void refreshPipelineTemplates();
    void retranslateUi();

signals:
    void baseSettingsChanged(const QString &format, const QString &vcodec, const QString &acodec, int crf);
    void pipelineChanged();

private slots:
    void onLoadTemplateClicked(const QString &name);
    void onSaveTemplateClicked();
    void onDeleteTemplateClicked();
    void onBrowseWatermark();

private:
    PipelineManager *m_pipelineManager = nullptr;

    ElaCheckBox *m_pipelineToggle = nullptr;
    QWidget *m_pipelineContainer = nullptr;

    ElaText *m_pipelineTemplateCaption = nullptr;
    ElaComboBox *m_pipelineTemplateCombo = nullptr;
    ElaPushButton *m_pipelineSaveBtn = nullptr;
    ElaPushButton *m_pipelineDeleteBtn = nullptr;

    // Step 1: Trim
    ElaCheckBox *m_pipeTrimCheck = nullptr;
    ElaLineEdit *m_pipeStartEdit = nullptr;
    ElaLineEdit *m_pipeEndEdit = nullptr;

    // Step 2: Crop
    ElaCheckBox *m_pipeCropCheck = nullptr;
    ElaComboBox *m_pipeCropPresetCombo = nullptr;
    ElaSpinBox *m_pipeCropXSpin = nullptr;
    ElaSpinBox *m_pipeCropYSpin = nullptr;
    ElaSpinBox *m_pipeCropWSpin = nullptr;
    ElaSpinBox *m_pipeCropHSpin = nullptr;

    // Step 3: Scale
    ElaCheckBox *m_pipeScaleCheck = nullptr;
    ElaComboBox *m_pipeScaleCombo = nullptr;
    ElaSpinBox *m_pipeScaleWSpin = nullptr;
    ElaSpinBox *m_pipeScaleHSpin = nullptr;

    // Step 4: FPS
    ElaCheckBox *m_pipeFpsCheck = nullptr;
    ElaComboBox *m_pipeFpsCombo = nullptr;

    // Step 5: Watermark
    ElaCheckBox *m_pipeWatermarkCheck = nullptr;
    ElaLineEdit *m_pipeWatermarkPathEdit = nullptr;
    ElaPushButton *m_pipeWatermarkBrowseBtn = nullptr;
    ElaComboBox *m_pipeWatermarkPosCombo = nullptr;
    ElaSpinBox *m_pipeWatermarkOpacitySpin = nullptr;

    // Step 6: Audio
    ElaCheckBox *m_pipeAudioCheck = nullptr;
    ElaSpinBox *m_pipeAudioVolSpin = nullptr;
    ElaCheckBox *m_pipeAudioLoudnormCheck = nullptr;
    ElaCheckBox *m_pipeAudioMuteCheck = nullptr;
};
