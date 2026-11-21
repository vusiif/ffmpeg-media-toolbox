#pragma once

#include <QString>
#include <QVariantMap>

#include "ElaScrollPageArea.h"
#include "ffmpeg/FFmpegCommandBuilder.h"

class FFmpegCapabilities;
class PresetManager;
class ElaText;
class ElaPushButton;
class ElaComboBox;
class ElaSpinBox;
class ElaDoubleSpinBox;
class ElaCheckBox;

class ConvertCodecCard : public ElaScrollPageArea
{
    Q_OBJECT

public:
    explicit ConvertCodecCard(FFmpegCapabilities *caps, PresetManager *presets, QWidget *parent = nullptr);
    ~ConvertCodecCard() override = default;

    void applyTo(FFmpegCommandBuilder::ConversionOptions &opts, bool copy, double durationSeconds) const;
    void savePresetValues(QVariantMap &values) const;
    void loadPresetValues(const QVariantMap &values);

    QString format() const;
    QString videoCodec() const;
    QString audioCodec() const;
    int crf() const;
    int rateControlIndex() const;
    double targetSizeMB() const;
    bool isSmartCopy() const;

    void rebuildCodecLists();
    void refreshPresetList();
    void retranslateUi();

signals:
    void optionsChanged();
    void presetLoaded(const QString &name);
    void savePresetRequested();

private slots:
    void onFormatChanged();
    void onRateControlChanged(int idx);
    void onHwAccelToggled(bool checked);
    void onPresetComboChanged(const QString &name);
    void onSavePresetClicked();
    void onDeletePresetClicked();

private:
    FFmpegCapabilities *m_caps = nullptr;
    PresetManager *m_presets = nullptr;

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

    ElaText *m_quickTargetCaption = nullptr;
    QWidget *m_quickTargetWidget = nullptr;
    ElaPushButton *m_quick25MBBtn = nullptr;
    ElaPushButton *m_quick50MBBtn = nullptr;
    ElaPushButton *m_quick100MBBtn = nullptr;
    ElaPushButton *m_quickBalancedBtn = nullptr;
    ElaPushButton *m_quickLosslessBtn = nullptr;

    ElaText *m_rateControlCaption = nullptr;
    ElaComboBox *m_rateControlCombo = nullptr;
    ElaText *m_qualityCaption = nullptr;
    ElaSpinBox *m_qualitySpin = nullptr;

    ElaText *m_videoBitrateCaption = nullptr;
    QWidget *m_videoBitrateWidget = nullptr;
    ElaSpinBox *m_videoBitrateSpin = nullptr;
    ElaPushButton *m_preset1500kBtn = nullptr;
    ElaPushButton *m_preset3000kBtn = nullptr;
    ElaPushButton *m_preset6000kBtn = nullptr;
    ElaPushButton *m_preset10000kBtn = nullptr;
    ElaCheckBox *m_bitrateTwoPassCheck = nullptr;

    ElaText *m_targetSizeCaption = nullptr;
    QWidget *m_targetSizeWidget = nullptr;
    ElaDoubleSpinBox *m_targetSizeSpin = nullptr;
    ElaPushButton *m_preset25Btn = nullptr;
    ElaPushButton *m_preset50Btn = nullptr;
    ElaPushButton *m_preset100Btn = nullptr;
    ElaCheckBox *m_twoPassCheck = nullptr;

    ElaCheckBox *m_smartCopyCheck = nullptr;
    ElaCheckBox *m_preserveAllStreamsCheck = nullptr;
    ElaCheckBox *m_stripMetaCheck = nullptr;
    ElaCheckBox *m_hwAccelCheck = nullptr;
    ElaText *m_hwDeviceLabel = nullptr;
};
