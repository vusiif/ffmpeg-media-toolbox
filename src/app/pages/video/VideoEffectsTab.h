#pragma once

#include "VideoTabBase.h"

#include <QPointer>

class FilterStudioDialog;
class SubtitleStudioDialog;
class PipCompositorDialog;
class ElaPushButton;
class ElaText;
class ElaLineEdit;
class ElaComboBox;
class ElaSpinBox;
class ElaCheckBox;

class VideoEffectsTab : public VideoTabBase
{
    Q_OBJECT

public:
    explicit VideoEffectsTab(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                             JobQueue *queue, MediaProbeService *probeService = nullptr,
                             QWidget *parent = nullptr);
    ~VideoEffectsTab() override = default;

    void setMediaInfo(const MediaFile &info) override;
    void retranslateUi() override;

private:
    void initFilterStudioCard();
    void initWatermarkCard();
    void initPipCard();
    void initSubtitleCard();

    void watermark();
    void pip();
    void openFilterStudioDialog();
    void openPipCompositorStudio();

    void subtitleMux();
    void extractSubtitle();
    void extractSubtitleToStudio();
    void openSubtitleStudio();
    void updateSubtitleStreamsUI();

    // Filter Studio Card
    ElaScrollPageArea *m_filterStudioCard = nullptr;
    ElaText *m_filterStudioTitleLabel = nullptr;
    ElaPushButton *m_filterStudioButton = nullptr;

    // Watermark Card
    ElaScrollPageArea *m_wmCard = nullptr;
    ElaText *m_wmTitleLabel = nullptr;
    ElaText *m_wmImageCaption = nullptr;
    ElaLineEdit *m_wmPath = nullptr;
    ElaPushButton *m_wmBrowseButton = nullptr;
    ElaText *m_wmPositionCaption = nullptr;
    ElaComboBox *m_wmPosition = nullptr;
    ElaText *m_wmOpacityCaption = nullptr;
    ElaSpinBox *m_wmOpacity = nullptr;
    ElaPushButton *m_wmButton = nullptr;

    // PIP Card
    ElaScrollPageArea *m_pipCard = nullptr;
    ElaText *m_pipTitleLabel = nullptr;
    ElaText *m_pipVideoCaption = nullptr;
    ElaLineEdit *m_pipPath = nullptr;
    ElaPushButton *m_pipBrowseButton = nullptr;
    ElaText *m_pipSizeCaption = nullptr;
    ElaSpinBox *m_pipWidth = nullptr;
    ElaSpinBox *m_pipHeight = nullptr;
    ElaText *m_pipPositionCaption = nullptr;
    ElaComboBox *m_pipPosition = nullptr;
    ElaText *m_pipMarginCaption = nullptr;
    ElaSpinBox *m_pipMargin = nullptr;
    ElaText *m_pipQualityCaption = nullptr;
    ElaSpinBox *m_pipQualitySpin = nullptr;
    ElaPushButton *m_pipButton = nullptr;
    ElaPushButton *m_openPipStudioBtn = nullptr;
    QString m_pipFile;

    // Subtitle Card
    ElaScrollPageArea *m_subCard = nullptr;
    ElaText *m_subTitleLabel = nullptr;
    ElaText *m_subCaption = nullptr;
    ElaLineEdit *m_subPath = nullptr;
    ElaPushButton *m_subPathButton = nullptr;
    ElaCheckBox *m_subBurnCheck = nullptr;
    ElaText *m_subQualityCaption = nullptr;
    ElaSpinBox *m_subQualitySpin = nullptr;
    ElaPushButton *m_subButton = nullptr;
    ElaPushButton *m_openSubStudioBtn = nullptr;
    QString m_subtitleFile;

    // Subtitle Extraction UI
    ElaText *m_subExtractSectionCaption = nullptr;
    ElaText *m_subStreamCaption = nullptr;
    ElaComboBox *m_subStreamCombo = nullptr;
    ElaPushButton *m_subExtractButton = nullptr;
    ElaPushButton *m_subExtractToStudioButton = nullptr;

    QPointer<FilterStudioDialog> m_filterDialog;
    QPointer<SubtitleStudioDialog> m_subDialog;
    QPointer<PipCompositorDialog> m_pipDialog;
};
