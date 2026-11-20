#pragma once

#include "RepairTabBase.h"

class UntruncLocator;
class UntruncProcess;
class ElaScrollPageArea;
class ElaText;
class ElaLineEdit;
class ElaPushButton;
class ElaProgressBar;
class QPlainTextEdit;

class UntruncRepairTab : public RepairTabBase
{
    Q_OBJECT

public:
    explicit UntruncRepairTab(FFmpegLocator *ffmpegLocator,
                             MediaProbeService *probeService,
                             JobQueue *jobQueue,
                             UntruncLocator *untruncLocator,
                             QWidget *parent = nullptr);
    ~UntruncRepairTab() override = default;

    void retranslateUi() override;
    void handleDroppedFile(const QString &filePath) override;

signals:
    void previewRequested(const QString &filePath);

private slots:
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
    void initUi();

    UntruncLocator *m_untruncLocator = nullptr;
    UntruncProcess *m_repairProcess = nullptr;

    QString m_corruptedPath;
    QString m_referencePath;
    QString m_lastFixedPath;

    // UI elements
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
