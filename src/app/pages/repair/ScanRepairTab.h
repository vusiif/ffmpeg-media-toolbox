#pragma once

#include "RepairTabBase.h"
#include "media/CorruptedVideoScanner.h"

class ElaScrollPageArea;
class ElaText;
class ElaLineEdit;
class ElaPushButton;
class ElaCheckBox;
class ElaProgressBar;
class ElaTableView;
class VideoPlayerWidget;
class QStandardItemModel;
class QModelIndex;

class ScanRepairTab : public RepairTabBase
{
    Q_OBJECT

public:
    explicit ScanRepairTab(FFmpegLocator *ffmpegLocator,
                           MediaProbeService *probeService,
                           JobQueue *jobQueue,
                           QWidget *parent = nullptr);
    ~ScanRepairTab() override = default;

    void retranslateUi() override;
    void handleDroppedFile(const QString &filePath) override;
    void loadAndPlayMedia(const QString &filePath);

private slots:
    void onBrowseScanInput();
    void onStartScan();
    void onCancelScan();
    void onScanProgress(double fraction);
    void onScanFinished(bool success, const QVector<CorruptedSegment> &segments);
    void onQcTableClicked(const QModelIndex &index);
    void onJumpToSelectedIssue();
    void onExportReport();
    void onExciseCorruptedSegments();

private:
    void initUi();

    CorruptedVideoScanner *m_scanner = nullptr;

    QString m_scanFilePath;
    double m_scanDuration = 0.0;
    bool m_hasVideo = true;
    bool m_hasAudio = true;

    // UI elements
    ElaScrollPageArea *m_scanControlCard = nullptr;
    ElaText *m_scanTitleLabel = nullptr;
    ElaText *m_scanDescLabel = nullptr;
    ElaText *m_scanInputCaption = nullptr;
    ElaLineEdit *m_scanInputEdit = nullptr;
    ElaPushButton *m_scanBrowseBtn = nullptr;

    ElaCheckBox *m_qcDecodeCheck = nullptr;
    ElaCheckBox *m_qcSilenceCheck = nullptr;
    ElaCheckBox *m_qcBlackCheck = nullptr;
    ElaCheckBox *m_qcFreezeCheck = nullptr;

    ElaPushButton *m_scanStartBtn = nullptr;
    ElaPushButton *m_scanCancelBtn = nullptr;
    ElaPushButton *m_exportReportBtn = nullptr;
    ElaProgressBar *m_scanProgressBar = nullptr;
    ElaText *m_scanStatusLabel = nullptr;

    ElaScrollPageArea *m_playerCard = nullptr;
    VideoPlayerWidget *m_playerWidget = nullptr;

    ElaScrollPageArea *m_qcResultCard = nullptr;
    ElaText *m_qcSummaryLabel = nullptr;
    ElaTableView *m_qcTableView = nullptr;
    QStandardItemModel *m_qcModel = nullptr;
    ElaPushButton *m_jumpBtn = nullptr;
    ElaPushButton *m_exciseCleanBtn = nullptr;
};
