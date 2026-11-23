#pragma once

#include <QString>

#include "ElaScrollPageArea.h"
#include "ffmpeg/FFmpegCommandBuilder.h"

class FFmpegCapabilities;
class PresetManager;
class ElaText;
class ElaComboBox;
class ElaSpinBox;
class ElaPushButton;
class ElaCheckBox;
class ElaLineEdit;

class BatchOptionsCard : public ElaScrollPageArea
{
    Q_OBJECT

public:
    explicit BatchOptionsCard(FFmpegCapabilities *caps, PresetManager *presets, QWidget *parent = nullptr);
    ~BatchOptionsCard() override = default;

    FFmpegCommandBuilder::ConversionOptions currentOptions(
        const QString &inputFile, const QString &outputPath, bool copy) const;

    QString format() const;
    QString videoCodec() const;
    QString audioCodec() const;
    int crf() const;
    QString audioBitrate() const;
    QString speedPreset() const;
    bool isSmartCopy() const;
    bool isHwAccel() const;

    void setBaseSettings(const QString &format, const QString &vcodec, const QString &acodec, int crf);
    void rebuildCodecLists();
    void refreshPresetList();
    void loadPreset(const QString &name);
    void retranslateUi();

signals:
    void optionsChanged();

private slots:
    void onFormatChanged();
    void onHwAccelToggled(bool checked);

private:
    FFmpegCapabilities *m_caps = nullptr;
    PresetManager *m_presets = nullptr;

    // Presets & Base format
    ElaText *m_optionsCardTitle = nullptr;
    ElaText *m_presetCaption = nullptr;
    ElaComboBox *m_presetCombo = nullptr;
    ElaText *m_formatCaption = nullptr;
    ElaComboBox *m_formatCombo = nullptr;

    // Codecs & Quality
    ElaText *m_codecCaption = nullptr;
    ElaComboBox *m_codecCombo = nullptr;
    ElaText *m_audioCodecCaption = nullptr;
    ElaComboBox *m_audioCodecCombo = nullptr;
    ElaText *m_qualityCaption = nullptr;
    ElaSpinBox *m_qualitySpin = nullptr;
    ElaText *m_quickTargetCaption = nullptr;
    QWidget *m_qualityWidget = nullptr;
    ElaPushButton *m_quickSmallBtn = nullptr;
    ElaPushButton *m_quickBalancedBtn = nullptr;
    ElaPushButton *m_quickHighQualityBtn = nullptr;
    ElaPushButton *m_quickCopyBtn = nullptr;
    ElaCheckBox *m_smartCopyCheck = nullptr;
    ElaCheckBox *m_stripMetaCheck = nullptr;
    ElaCheckBox *m_hwAccelCheck = nullptr;
    ElaText *m_hwDeviceLabel = nullptr;

    // Collapsible Advanced options
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
    ElaText *m_resolutionCaption = nullptr;
    ElaComboBox *m_resolutionCombo = nullptr;
    ElaText *m_fpsCaption = nullptr;
    ElaComboBox *m_fpsCombo = nullptr;
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
};
