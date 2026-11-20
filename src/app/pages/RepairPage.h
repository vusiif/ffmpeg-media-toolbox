#pragma once

#include "media/CorruptedVideoScanner.h"
#include "repair/MediaSalvageEngine.h"

#include <QWidget>

class FFmpegLocator;
class MediaProbeService;
class UntruncLocator;
class UntruncProcess;
class JobQueue;
class VideoPlayerWidget;
class ElaText;
class ElaLineEdit;
class ElaPushButton;
class ElaProgressBar;
class ElaPivot;
class ElaScrollPageArea;
class ElaCheckBox;
class ElaComboBox;
class ElaTableView;
class QStackedWidget;
class QPlainTextEdit;
class QStandardItemModel;
class QModelIndex;

class RepairPage : public QWidget
{
    Q_OBJECT

public:
    explicit RepairPage(FFmpegLocator *ffmpegLocator,
                        MediaProbeService *probeService,
                        UntruncLocator *untruncLocator,
                        JobQueue *jobQueue = nullptr,
                        QWidget *parent = nullptr);
    ~RepairPage() override;

    void retranslateUi();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    // Tab 0: Smart Salvage (Reference-free)
    void onBrowseSalvageInput();
    void onBrowseSalvageOutput();
    void onSalvageModeChanged(int index);
    void onStartSalvage();
    void onQueueSalvage();
    void onCancelSalvage();
    void onSalvageProgress(double fraction);
    void onSalvageLog(const QString &line);
    void onSalvageFinished(bool success, const QString &outputPath, const QString &errorMsg);
    void onOpenSalvageFolder();
    void onPlaySalvagedVideo();

    // Tab 1: Scan & Preview (QC)
    void onBrowseScanInput();
    void onStartScan();
    void onCancelScan();
    void onScanProgress(double fraction);
    void onScanFinished(bool success, const QVector<CorruptedSegment> &segments);
    void onQcTableClicked(const QModelIndex &index);
    void onJumpToSelectedIssue();
    void onExportReport();
    void onExciseCorruptedSegments();

    // Tab 2: Deep Repair (Untrunc)
    void onBrowseCorrupted();
    void onBrowseReference();
    void onStartRepair();
    void onCancelRepair();
    void onRepairLog(const QString &line);
    void onRepairProgress(double fraction);
    void onRepairFinished(bool success, const QString &fixedPath);
    void onOpenOutputFolder();
    void onPreviewFixedVideo();

private:
    void setupSalvageTab(QWidget *tab);
    void setupScanTab(QWidget *tab);
    void setupRepairTab(QWidget *tab);

    FFmpegLocator *m_ffmpegLocator = nullptr;
    MediaProbeService *m_probeService = nullptr;
    UntruncLocator *m_untruncLocator = nullptr;
    JobQueue *m_jobQueue = nullptr;
    CorruptedVideoScanner *m_scanner = nullptr;
    UntruncProcess *m_repairProcess = nullptr;
    MediaSalvageEngine *m_salvageEngine = nullptr;

    // Smart Salvage State
    QString m_salvageInputPath;
    QString m_salvageOutputPath;
    double m_salvageDuration = 0.0;
    QString m_lastSalvagedPath;

    // Scan State
    QString m_scanFilePath;
    double m_scanDuration = 0.0;
    bool m_hasVideo = true;
    bool m_hasAudio = true;

    // Untrunc State
    QString m_corruptedPath;
    QString m_referencePath;
    QString m_lastFixedPath;

    ElaText *m_pageTitleLabel = nullptr;
    ElaPivot *m_pivot = nullptr;
    QStackedWidget *m_stackedWidget = nullptr;
    QString m_salvageTabTitle;
    QString m_scanTabTitle;
    QString m_repairTabTitle;

    // Tab 0: Smart Salvage UI
    ElaScrollPageArea *m_salvageControlCard = nullptr;
    ElaText *m_salvageTitleLabel = nullptr;
    ElaText *m_salvageDescLabel = nullptr;
    ElaText *m_salvageInputCaption = nullptr;
    ElaLineEdit *m_salvageInputEdit = nullptr;
    ElaPushButton *m_salvageBrowseBtn = nullptr;
    ElaText *m_salvageAssessLabel = nullptr;
    ElaText *m_salvageModeCaption = nullptr;
    ElaComboBox *m_salvageModeCombo = nullptr;
    ElaText *m_salvageOutputCaption = nullptr;
    ElaLineEdit *m_salvageOutputEdit = nullptr;
    ElaPushButton *m_salvageOutputBrowseBtn = nullptr;
    ElaPushButton *m_startSalvageBtn = nullptr;
    ElaPushButton *m_queueSalvageBtn = nullptr;
    ElaPushButton *m_cancelSalvageBtn = nullptr;
    ElaProgressBar *m_salvageProgressBar = nullptr;
    ElaText *m_salvageStatusLabel = nullptr;
    QWidget *m_salvageResultBar = nullptr;
    ElaText *m_salvageResultLabel = nullptr;
    ElaPushButton *m_salvageOpenFolderBtn = nullptr;
    ElaPushButton *m_salvagePlayBtn = nullptr;

    ElaScrollPageArea *m_salvageConsoleCard = nullptr;
    ElaText *m_salvageConsoleCaption = nullptr;
    ElaPushButton *m_clearSalvageConsoleBtn = nullptr;
    QPlainTextEdit *m_salvageConsole = nullptr;

    // Tab 1: Scan Tab UI
    ElaScrollPageArea *m_scanControlCard = nullptr;
    ElaText *m_scanTitleLabel = nullptr;
    ElaText *m_scanDescLabel = nullptr;
    ElaText *m_scanInputCaption = nullptr;
    ElaLineEdit *m_scanInputEdit = nullptr;
    ElaPushButton *m_scanBrowseBtn = nullptr;

    // QC Options
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

    // QC Issues Table Card
    ElaScrollPageArea *m_qcResultCard = nullptr;
    ElaText *m_qcSummaryLabel = nullptr;
    ElaTableView *m_qcTableView = nullptr;
    QStandardItemModel *m_qcModel = nullptr;
    ElaPushButton *m_jumpBtn = nullptr;
    ElaPushButton *m_exciseCleanBtn = nullptr;

    // Tab 2: Repair Tab UI
    ElaScrollPageArea *m_repairControlCard = nullptr;
    ElaText *m_repairTitleLabel = nullptr;
    ElaText *m_repairDescLabel = nullptr;
    ElaText *m_corruptedCaption = nullptr;
    ElaLineEdit *m_corruptedEdit = nullptr;
    ElaPushButton *m_corruptedBrowseBtn = nullptr;
    ElaText *m_referenceCaption = nullptr;
    ElaLineEdit *m_referenceEdit = nullptr;
    ElaPushButton *m_referenceBrowseBtn = nullptr;
    ElaPushButton *m_startRepairBtn = nullptr;
    ElaPushButton *m_cancelRepairBtn = nullptr;
    ElaProgressBar *m_repairProgressBar = nullptr;
    ElaText *m_repairStatusLabel = nullptr;
    QWidget *m_resultBar = nullptr;
    ElaText *m_resultLabel = nullptr;
    ElaPushButton *m_openFolderBtn = nullptr;
    ElaPushButton *m_previewFixedBtn = nullptr;

    ElaScrollPageArea *m_consoleCard = nullptr;
    ElaText *m_consoleCaption = nullptr;
    ElaPushButton *m_clearConsoleBtn = nullptr;
    QPlainTextEdit *m_console = nullptr;
};
