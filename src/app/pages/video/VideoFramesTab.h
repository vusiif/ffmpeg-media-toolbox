#pragma once

#include "VideoTabBase.h"

#include <QPointer>

class GifStudioDialog;
class ElaPushButton;
class ElaText;
class ElaLineEdit;
class ElaDoubleSpinBox;
class ElaSpinBox;
class ElaComboBox;

class VideoFramesTab : public VideoTabBase
{
    Q_OBJECT

public:
    explicit VideoFramesTab(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                            JobQueue *queue, MediaProbeService *probeService = nullptr,
                            QWidget *parent = nullptr);
    ~VideoFramesTab() override = default;

    void retranslateUi() override;

private:
    void initScreenshotCard();
    void initGifCard();
    void initSequenceCard();

    void screenshot();
    void makeGif();
    void openGifStudio();
    void sequence();

    // Screenshot Card
    ElaScrollPageArea *m_shotCard = nullptr;
    ElaText *m_shotTitleLabel = nullptr;
    ElaText *m_shotTimeCaption = nullptr;
    ElaDoubleSpinBox *m_shotTime = nullptr;
    ElaText *m_shotFormatCaption = nullptr;
    ElaComboBox *m_shotFormat = nullptr;
    ElaPushButton *m_shotButton = nullptr;

    // GIF Card
    ElaScrollPageArea *m_gifCard = nullptr;
    ElaText *m_gifTitleLabel = nullptr;
    ElaText *m_gifStartCaption = nullptr;
    ElaDoubleSpinBox *m_gifStart = nullptr;
    ElaText *m_gifEndCaption = nullptr;
    ElaDoubleSpinBox *m_gifEnd = nullptr;
    ElaText *m_gifFpsCaption = nullptr;
    ElaSpinBox *m_gifFps = nullptr;
    ElaText *m_gifWidthCaption = nullptr;
    ElaSpinBox *m_gifWidth = nullptr;
    ElaPushButton *m_gifButton = nullptr;
    ElaPushButton *m_openGifStudioBtn = nullptr;

    // Sequence Card
    ElaScrollPageArea *m_seqCard = nullptr;
    ElaText *m_seqTitleLabel = nullptr;
    ElaText *m_seqModeCaption = nullptr;
    ElaComboBox *m_seqMode = nullptr;
    ElaText *m_seqPatternCaption = nullptr;
    ElaLineEdit *m_seqPattern = nullptr;
    ElaText *m_seqFpsCaption = nullptr;
    ElaDoubleSpinBox *m_seqFps = nullptr;
    ElaPushButton *m_seqButton = nullptr;

    QPointer<GifStudioDialog> m_gifDialog;
};
