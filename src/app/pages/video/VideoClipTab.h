#pragma once

#include "VideoTabBase.h"

#include <QFutureWatcher>
#include <QPointer>
#include <QProcess>
#include <QVector>

class VideoPlayerWidget;
class LosslessCutDialog;
class ElaPushButton;
class ElaText;
class ElaDoubleSpinBox;
class ElaSpinBox;
class ElaComboBox;
class ElaCheckBox;
class QListWidget;

class VideoClipTab : public VideoTabBase
{
    Q_OBJECT

public:
    explicit VideoClipTab(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                          JobQueue *queue, MediaProbeService *probeService = nullptr,
                          QWidget *parent = nullptr);
    ~VideoClipTab() override;

    void setInputFile(const QString &path) override;
    void retranslateUi() override;

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void initTrimCard();
    void initAdjustCard();
    void initJoinCard();

    void trim();
    void openLosslessCutStudio();
    void detectKeyframes();
    void onKeyframeProbeFinished(int exitCode);
    void updateKeyframeStatus();

    void adjust();
    void detectAutoCrop();
    void joinVideos();

    QVector<double> m_keyframes;
    QProcess *m_probeProcess = nullptr;

    // Trim Card
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

    // Adjust Card
    ElaScrollPageArea *m_adjCard = nullptr;
    ElaText *m_adjTitleLabel = nullptr;
    ElaText *m_adjCropCaption = nullptr;
    ElaSpinBox *m_adjCropX = nullptr;
    ElaSpinBox *m_adjCropY = nullptr;
    ElaSpinBox *m_adjCropW = nullptr;
    ElaSpinBox *m_adjCropH = nullptr;
    ElaPushButton *m_autoCropButton = nullptr;
    ElaText *m_autoCropStatus = nullptr;
    QFutureWatcher<QString> *m_autoCropWatcher = nullptr;
    ElaText *m_adjResizeCaption = nullptr;
    ElaSpinBox *m_adjResizeW = nullptr;
    ElaSpinBox *m_adjResizeH = nullptr;
    ElaText *m_adjRotateCaption = nullptr;
    ElaComboBox *m_adjRotateCombo = nullptr;
    ElaText *m_adjQualityCaption = nullptr;
    ElaSpinBox *m_adjQualitySpin = nullptr;
    ElaPushButton *m_adjButton = nullptr;

    // Join Card
    ElaScrollPageArea *m_joinCard = nullptr;
    ElaText *m_joinTitleLabel = nullptr;
    QListWidget *m_joinList = nullptr;
    ElaPushButton *m_joinAddButton = nullptr;
    ElaPushButton *m_joinRemoveButton = nullptr;
    ElaPushButton *m_joinButton = nullptr;
    QStringList m_joinFiles;

    QPointer<LosslessCutDialog> m_cutDialog;
};
