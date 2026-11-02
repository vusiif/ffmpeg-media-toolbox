#pragma once

#include <QDialog>
#include <QVector>
#include <QPair>

#include "cut/SmartCutEngine.h"

class FFmpegLocator;
class MediaProbeService;
class JobQueue;
class VideoPlayerWidget;
class ElaText;
class ElaPushButton;
class ElaLineEdit;
class ElaComboBox;
class ElaDoubleSpinBox;
class ElaScrollPageArea;
class QTableWidget;
class QProcess;

class LosslessCutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LosslessCutDialog(FFmpegLocator *locator, MediaProbeService *probeService,
                               JobQueue *queue, QWidget *parent = nullptr);
    ~LosslessCutDialog() override;

    void loadFile(const QString &filePath);
    void retranslateUi();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onBrowseClicked();
    void onDetectKeyframes();
    void onKeyframeProbeFinished(int exitCode);
    void onPrevKeyframeClicked();
    void onNextKeyframeClicked();
    void onSnapInClicked();
    void onSnapOutClicked();
    void onMarkInClicked();
    void onMarkOutClicked();
    void onPreviewRangeClicked();
    void onAddSegmentClicked();
    void onClearSegmentsClicked();
    void onTableItemClicked(int row, int column);
    void onExportClicked();

private:
    void updateKeyframeStatus();
    void refreshSegmentsTable();
    void updateTimelineSegments();

    FFmpegLocator *m_locator;
    MediaProbeService *m_probeService;
    JobQueue *m_queue;

    QString m_inputFile;
    QVector<double> m_keyframes;
    QVector<CutSegment> m_segments;
    QProcess *m_probeProcess = nullptr;

    // UI Widgets
    ElaText *m_titleLabel = nullptr;
    ElaText *m_subtitleLabel = nullptr;
    ElaLineEdit *m_filePathEdit = nullptr;
    ElaPushButton *m_browseButton = nullptr;
    ElaText *m_metaInfoLabel = nullptr;

    // Player
    VideoPlayerWidget *m_player = nullptr;

    // Keyframe Controls
    ElaPushButton *m_detectKeyframesBtn = nullptr;
    ElaPushButton *m_prevKeyframeBtn = nullptr;
    ElaPushButton *m_nextKeyframeBtn = nullptr;
    ElaPushButton *m_snapInBtn = nullptr;
    ElaPushButton *m_snapOutBtn = nullptr;
    ElaText *m_keyframeStatusLabel = nullptr;

    // Range Controls
    ElaPushButton *m_markInBtn = nullptr;
    ElaPushButton *m_markOutBtn = nullptr;
    ElaPushButton *m_previewBtn = nullptr;
    ElaDoubleSpinBox *m_startSpin = nullptr;
    ElaDoubleSpinBox *m_endSpin = nullptr;
    ElaPushButton *m_addSegmentBtn = nullptr;

    // Segments Table
    QTableWidget *m_segmentTable = nullptr;
    ElaPushButton *m_clearSegmentsBtn = nullptr;
    ElaText *m_segmentsSummaryLabel = nullptr;

    // Export Options
    ElaComboBox *m_cutModeCombo = nullptr;
    ElaComboBox *m_exportModeCombo = nullptr;
    ElaComboBox *m_formatCombo = nullptr;
    ElaPushButton *m_exportBtn = nullptr;
    ElaPushButton *m_closeBtn = nullptr;
};
