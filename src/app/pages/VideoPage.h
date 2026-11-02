#pragma once

#include <QProcess>
#include <QVector>
#include <QWidget>
#include "cut/SmartCutEngine.h"

class FFmpegLocator;
class FFmpegCommandBuilder;
class MediaProbeService;
class JobQueue;
class QListWidget;
class QStackedWidget;
class ElaPivot;
class ElaScrollArea;
class ElaScrollPageArea;
class ElaText;
class ElaPushButton;
class ElaLineEdit;
class ElaComboBox;
class ElaSpinBox;
class ElaDoubleSpinBox;
class ElaCheckBox;
class VideoPlayerWidget;

class VideoPage : public QWidget
{
    Q_OBJECT

public:
    explicit VideoPage(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                       JobQueue *queue, MediaProbeService *probeService = nullptr,
                       QWidget *parent = nullptr);
    ~VideoPage() override;

    void retranslateUi();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    QString browseFile(const QString &title, const QString &filters);
    void enqueue(const QString &name, const QStringList &args);

    void trim();
    void openLosslessCutStudio();
    void openFilterStudioDialog();
    void detectKeyframes();
    void onKeyframeProbeFinished(int exitCode);
    void updateKeyframeStatus();

    void extractAudio();
    void mute();
    void screenshot();
    void makeGif();
    void joinVideos();
    void sequence();
    void watermark();
    void subtitleMux();
    void openSubtitleStudio();
    void adjust();
    void detectAutoCrop();
    void pip();

    FFmpegLocator *m_locator;
    FFmpegCommandBuilder *m_builder;
    JobQueue *m_queue;
    MediaProbeService *m_probeService;

    QString m_inputFile;
    QVector<double> m_keyframes;
    QProcess *m_probeProcess = nullptr;

    ElaText *m_titleLabel = nullptr;
    ElaText *m_inputCaption = nullptr;
    ElaLineEdit *m_inputEdit = nullptr;
    ElaPushButton *m_browseButton = nullptr;

    ElaPivot *m_pivot = nullptr;
    QStackedWidget *m_stackedWidget = nullptr;
    QString m_clipTabTitle;
    QString m_audioTabTitle;
    QString m_effectsTabTitle;
    QString m_framesTabTitle;

    // Tab 1: Clip & Adjust
    ElaScrollPageArea *m_trimCard = nullptr;
    ElaText *m_trimTitleLabel = nullptr;
    VideoPlayerWidget *m_trimPlayer = nullptr;
    ElaPushButton *m_setInButton = nullptr;
    ElaPushButton *m_setOutButton = nullptr;
    ElaPushButton *m_previewTrimButton = nullptr;
    ElaPushButton *m_prevKeyframeBtn = nullptr;
    ElaPushButton *m_nextKeyframeBtn = nullptr;
    ElaPushButton *m_snapInButton = nullptr;
    ElaPushButton *m_snapOutButton = nullptr;
    ElaPushButton *m_openStudioButton = nullptr;
    ElaText *m_keyframeStatusLabel = nullptr;
    ElaText *m_trimStartCaption = nullptr;
    ElaDoubleSpinBox *m_trimStart = nullptr;
    ElaText *m_trimEndCaption = nullptr;
    ElaDoubleSpinBox *m_trimEnd = nullptr;
    ElaText *m_trimModeCaption = nullptr;
    ElaComboBox *m_trimMode = nullptr;
    ElaText *m_trimFormatCaption = nullptr;
    ElaComboBox *m_trimFormat = nullptr;
    ElaText *m_trimCopyCaption = nullptr;
    ElaCheckBox *m_trimCopy = nullptr;
    ElaPushButton *m_trimButton = nullptr;

    ElaScrollPageArea *m_adjCard = nullptr;
    ElaText *m_adjTitleLabel = nullptr;
    ElaText *m_adjCropCaption = nullptr;
    ElaSpinBox *m_adjCropX = nullptr;
    ElaSpinBox *m_adjCropY = nullptr;
    ElaSpinBox *m_adjCropW = nullptr;
    ElaSpinBox *m_adjCropH = nullptr;
    ElaPushButton *m_autoCropButton = nullptr;
    ElaText *m_autoCropStatus = nullptr;
    ElaText *m_adjResizeCaption = nullptr;
    ElaSpinBox *m_adjResizeW = nullptr;
    ElaSpinBox *m_adjResizeH = nullptr;
    ElaText *m_adjRotateCaption = nullptr;
    ElaComboBox *m_adjRotateCombo = nullptr;
    ElaText *m_adjQualityCaption = nullptr;
    ElaSpinBox *m_adjQualitySpin = nullptr;
    ElaPushButton *m_adjButton = nullptr;

    ElaScrollPageArea *m_joinCard = nullptr;
    ElaText *m_joinTitleLabel = nullptr;
    QListWidget *m_joinList = nullptr;
    ElaPushButton *m_joinAddButton = nullptr;
    ElaPushButton *m_joinRemoveButton = nullptr;
    ElaPushButton *m_joinButton = nullptr;

    // Tab 2: Audio Tools
    ElaScrollPageArea *m_audioCard = nullptr;
    ElaText *m_audioTitleLabel = nullptr;
    ElaText *m_audioFormatCaption = nullptr;
    ElaComboBox *m_audioFormat = nullptr;
    ElaText *m_audioBitrateCaption = nullptr;
    ElaSpinBox *m_audioBitrate = nullptr;
    ElaPushButton *m_audioButton = nullptr;

    ElaScrollPageArea *m_muteCard = nullptr;
    ElaText *m_muteTitleLabel = nullptr;
    ElaText *m_muteFormatCaption = nullptr;
    ElaComboBox *m_muteFormat = nullptr;
    ElaPushButton *m_muteButton = nullptr;

    // Tab 3: Overlay & Subtitles
    ElaScrollPageArea *m_filterStudioCard = nullptr;
    ElaText *m_filterStudioTitleLabel = nullptr;
    ElaPushButton *m_filterStudioButton = nullptr;
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
    QString m_pipFile;

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

    // Tab 4: Frames & Animation
    ElaScrollPageArea *m_shotCard = nullptr;
    ElaText *m_shotTitleLabel = nullptr;
    ElaText *m_shotTimeCaption = nullptr;
    ElaDoubleSpinBox *m_shotTime = nullptr;
    ElaText *m_shotFormatCaption = nullptr;
    ElaComboBox *m_shotFormat = nullptr;
    ElaPushButton *m_shotButton = nullptr;

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

    ElaScrollPageArea *m_seqCard = nullptr;
    ElaText *m_seqTitleLabel = nullptr;
    ElaText *m_seqModeCaption = nullptr;
    ElaComboBox *m_seqMode = nullptr;
    ElaText *m_seqPatternCaption = nullptr;
    ElaLineEdit *m_seqPattern = nullptr;
    ElaText *m_seqFpsCaption = nullptr;
    ElaDoubleSpinBox *m_seqFps = nullptr;
    ElaPushButton *m_seqButton = nullptr;

    QStringList m_joinFiles;
};
