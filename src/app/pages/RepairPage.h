#pragma once

#include <QWidget>

class FFmpegLocator;
class MediaProbeService;
class UntruncLocator;
class JobQueue;
class ElaPivot;
class ElaText;
class QStackedWidget;
class SalvageRepairTab;
class ScanRepairTab;
class UntruncRepairTab;

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
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    ElaText *m_pageTitleLabel = nullptr;
    ElaPivot *m_pivot = nullptr;
    QStackedWidget *m_stackedWidget = nullptr;

    SalvageRepairTab *m_salvageTab = nullptr;
    ScanRepairTab *m_scanTab = nullptr;
    UntruncRepairTab *m_untruncTab = nullptr;

    QString m_salvageTabTitle;
    QString m_scanTabTitle;
    QString m_repairTabTitle;
    bool m_isDragHover = false;
};
