#pragma once

#include "VideoTabBase.h"

class ElaPushButton;
class ElaText;
class ElaLineEdit;
class ElaSpinBox;
class ElaDoubleSpinBox;
class ElaComboBox;

class VideoAudioTab : public VideoTabBase
{
    Q_OBJECT

public:
    explicit VideoAudioTab(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                           JobQueue *queue, MediaProbeService *probeService = nullptr,
                           QWidget *parent = nullptr);
    ~VideoAudioTab() override = default;

    void retranslateUi() override;

private:
    void initAudioCard();
    void initMuteCard();
    void initBgmCard();
    void initSyncCard();

    void extractAudio();
    void mute();
    void applyBgm();
    void applyAudioSync();

    // Audio Extract Card
    ElaScrollPageArea *m_audioCard = nullptr;
    ElaText *m_audioTitleLabel = nullptr;
    ElaText *m_audioFormatCaption = nullptr;
    ElaComboBox *m_audioFormat = nullptr;
    ElaText *m_audioBitrateCaption = nullptr;
    ElaSpinBox *m_audioBitrate = nullptr;
    ElaPushButton *m_audioButton = nullptr;

    // Mute Card
    ElaScrollPageArea *m_muteCard = nullptr;
    ElaText *m_muteTitleLabel = nullptr;
    ElaText *m_muteFormatCaption = nullptr;
    ElaComboBox *m_muteFormat = nullptr;
    ElaPushButton *m_muteButton = nullptr;

    // BGM Card
    ElaScrollPageArea *m_bgmCard = nullptr;
    ElaText *m_bgmTitleLabel = nullptr;
    ElaText *m_bgmPathCaption = nullptr;
    ElaLineEdit *m_bgmPath = nullptr;
    ElaPushButton *m_bgmBrowseButton = nullptr;
    ElaText *m_bgmModeCaption = nullptr;
    ElaComboBox *m_bgmMode = nullptr;
    ElaText *m_bgmOrigVolCaption = nullptr;
    ElaSpinBox *m_bgmOrigVol = nullptr;
    ElaText *m_bgmMusicVolCaption = nullptr;
    ElaSpinBox *m_bgmMusicVol = nullptr;
    ElaPushButton *m_bgmButton = nullptr;

    // Audio Sync & Volume Card
    ElaScrollPageArea *m_syncCard = nullptr;
    ElaText *m_syncTitleLabel = nullptr;
    ElaText *m_syncOffsetCaption = nullptr;
    ElaDoubleSpinBox *m_syncOffset = nullptr;
    ElaText *m_syncOffsetTip = nullptr;
    ElaText *m_syncVolumeCaption = nullptr;
    ElaSpinBox *m_syncVolume = nullptr;
    ElaPushButton *m_syncButton = nullptr;
};
