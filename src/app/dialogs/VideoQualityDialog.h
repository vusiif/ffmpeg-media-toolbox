#pragma once

#include "ElaDialog.h"
#include "quality/VideoQualityEvaluator.h"

class FFmpegLocator;
class MediaProbeService;
class ElaText;
class ElaLineEdit;
class ElaPushButton;
class ElaProgressBar;
class ElaCheckBox;
class ElaComboBox;
class ElaScrollPageArea;

class VideoQualityDialog : public ElaDialog
{
    Q_OBJECT

public:
    explicit VideoQualityDialog(const QString &referencePath,
                                const QString &distortedPath,
                                const FFmpegLocator *locator,
                                MediaProbeService *probeService,
                                QWidget *parent = nullptr);
    ~VideoQualityDialog() override;

    void retranslateUi();

private slots:
    void onBrowseReference();
    void onBrowseDistorted();
    void onStartEvaluation();
    void onCancelEvaluation();
    void onEvaluationProgress(double fraction);
    void onEvaluationFinished(const VideoQualityResult &result);
    void onExportReport();

private:
    void setupUi();
    void updateScoreDisplay(const VideoQualityResult &res);

    QString m_referencePath;
    QString m_distortedPath;
    const FFmpegLocator *m_locator = nullptr;
    MediaProbeService *m_probeService = nullptr;
    VideoQualityEvaluator *m_evaluator = nullptr;
    VideoQualityResult m_lastResult;

    double m_referenceDuration = 0.0;
    int m_refWidth = 0;
    int m_refHeight = 0;

    ElaText *m_titleLabel = nullptr;

    // File Inputs
    ElaText *m_refCaption = nullptr;
    ElaLineEdit *m_refEdit = nullptr;
    ElaPushButton *m_refBrowseBtn = nullptr;

    ElaText *m_distCaption = nullptr;
    ElaLineEdit *m_distEdit = nullptr;
    ElaPushButton *m_distBrowseBtn = nullptr;

    // Options
    ElaText *m_modeCaption = nullptr;
    ElaComboBox *m_modeCombo = nullptr;
    ElaCheckBox *m_vmafCheck = nullptr;
    ElaCheckBox *m_ssimCheck = nullptr;
    ElaCheckBox *m_psnrCheck = nullptr;
    ElaCheckBox *m_fastSubsampleCheck = nullptr;

    // Actions
    ElaPushButton *m_startBtn = nullptr;
    ElaPushButton *m_cancelBtn = nullptr;
    ElaPushButton *m_exportBtn = nullptr;
    ElaPushButton *m_closeBtn = nullptr;
    ElaProgressBar *m_progressBar = nullptr;
    ElaText *m_statusLabel = nullptr;

    // Results Card
    ElaScrollPageArea *m_resultCard = nullptr;
    ElaText *m_vmafValueLabel = nullptr;
    ElaText *m_vmafRatingBadge = nullptr;
    ElaText *m_ssimValueLabel = nullptr;
    ElaText *m_psnrValueLabel = nullptr;
    ElaText *m_guideText = nullptr;
};
