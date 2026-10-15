#pragma once

#include "media/CorruptedVideoScanner.h"

#include <QWidget>

class FFmpegLocator;
class MediaProbeService;
class UntruncLocator;
class UntruncProcess;
class VideoPlayerWidget;
class ElaText;
class ElaLineEdit;
class ElaPushButton;
class ElaProgressBar;
class ElaPivot;
class ElaScrollPageArea;
class QStackedWidget;
class QPlainTextEdit;

class RepairPage : public QWidget
{
    Q_OBJECT

public:
    explicit RepairPage(FFmpegLocator *ffmpegLocator,
                        MediaProbeService *probeService,
                        UntruncLocator *untruncLocator,
                        QWidget *parent = nullptr);
    ~RepairPage() override;

    void retranslateUi();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    // Tab 1: Scan & Preview
    void onBrowseScanInput();
    void onStartScan();
    void onCancelScan();
    void onScanProgress(double fraction);
    void onScanFinished(bool success, const QVector<CorruptedSegment> &segments);

    // Tab 2: Deep Repair (Untrunc)
    void onBrowseCorrupted();
    void onBrowseReference();
    void onStartRepair();
    void onCancelRepair();
    void onRepairLog(const QString &line);
    void onRepairFinished(bool success, const QString &fixedPath);
    void onOpenOutputFolder();
    void onPreviewFixedVideo();

private:
    void setupScanTab(QWidget *tab);
    void setupRepairTab(QWidget *tab);

    FFmpegLocator *m_ffmpegLocator = nullptr;
    MediaProbeService *m_probeService = nullptr;
    UntruncLocator *m_untruncLocator = nullptr;
    CorruptedVideoScanner *m_scanner = nullptr;
    UntruncProcess *m_repairProcess = nullptr;

    QString m_scanFilePath;
    double m_scanDuration = 0.0;

    QString m_corruptedPath;
    QString m_referencePath;
    QString m_lastFixedPath;

    ElaText *m_pageTitleLabel = nullptr;
    ElaPivot *m_pivot = nullptr;
    QStackedWidget *m_stackedWidget = nullptr;
    QString m_scanTabTitle;
    QString m_repairTabTitle;

    // Scan Tab UI
    ElaScrollPageArea *m_scanControlCard = nullptr;
    ElaText *m_scanTitleLabel = nullptr;
    ElaText *m_scanDescLabel = nullptr;
    ElaText *m_scanInputCaption = nullptr;
    ElaLineEdit *m_scanInputEdit = nullptr;
    ElaPushButton *m_scanBrowseBtn = nullptr;
    ElaPushButton *m_scanStartBtn = nullptr;
    ElaPushButton *m_scanCancelBtn = nullptr;
    ElaProgressBar *m_scanProgressBar = nullptr;
    ElaText *m_scanStatusLabel = nullptr;
    ElaScrollPageArea *m_playerCard = nullptr;
    VideoPlayerWidget *m_playerWidget = nullptr;

    // Repair Tab UI
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
