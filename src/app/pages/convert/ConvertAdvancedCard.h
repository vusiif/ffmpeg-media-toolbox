#pragma once

#include <QString>
#include <QVariantMap>

#include "ElaScrollPageArea.h"
#include "ffmpeg/FFmpegCommandBuilder.h"

class ElaCheckBox;
class ElaText;
class ElaComboBox;
class ElaLineEdit;

class ConvertAdvancedCard : public ElaScrollPageArea
{
    Q_OBJECT

public:
    explicit ConvertAdvancedCard(QWidget *parent = nullptr);
    ~ConvertAdvancedCard() override = default;

    void applyTo(FFmpegCommandBuilder::ConversionOptions &opts) const;
    void savePresetValues(QVariantMap &values) const;
    void loadPresetValues(const QVariantMap &values);

    void enableHdrTonemapDefault();
    void setCustomFilters(const QString &vf, const QString &af);
    void retranslateUi();

signals:
    void optionsChanged();

private:
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
