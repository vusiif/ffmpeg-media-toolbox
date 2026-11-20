#pragma once

#include "RepairTabBase.h"
#include "repair/MediaSalvageEngine.h"

class ElaScrollPageArea;
class ElaText;
class ElaLineEdit;
class ElaPushButton;
class ElaComboBox;
class ElaProgressBar;
class QPlainTextEdit;

class SalvageRepairTab : public RepairTabBase
{
    Q_OBJECT

public:
    explicit SalvageRepairTab(FFmpegLocator *ffmpegLocator,
                              MediaProbeService *probeService,
                              JobQueue *jobQueue,
                              QWidget *parent = nullptr);
    ~SalvageRepairTab() override = default;

    void retranslateUi() override;
    void handleDroppedFile(const QString &filePath) override;

private slots:
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

private:
    void initUi();

    MediaSalvageEngine *m_salvageEngine = nullptr;

    QString m_salvageInputPath;
    QString m_salvageOutputPath;
    double m_salvageDuration = 0.0;
    QString m_lastSalvagedPath;

    // UI elements
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
};
