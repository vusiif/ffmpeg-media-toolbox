#include "RepairPage.h"

#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>
#include <QStackedWidget>
#include <QUrl>
#include <QVBoxLayout>

#include "ElaPivot.h"
#include "ElaText.h"
#include "i18n/TrId.h"
#include "repair/SalvageRepairTab.h"
#include "repair/ScanRepairTab.h"
#include "repair/UntruncRepairTab.h"

RepairPage::RepairPage(FFmpegLocator *ffmpegLocator,
                       MediaProbeService *probeService,
                       UntruncLocator *untruncLocator,
                       JobQueue *jobQueue,
                       QWidget *parent)
    : QWidget(parent)
{
    setAcceptDrops(true);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(30, 24, 30, 24);
    root->setSpacing(14);

    m_pageTitleLabel = new ElaText(QString(), 13, this);
    m_pageTitleLabel->setTextPixelSize(28);
    root->addWidget(m_pageTitleLabel);

    // Fluent Pivot Segmented Switcher
    m_pivot = new ElaPivot(this);
    m_pivot->setPivotSpacing(16);
    m_pivot->setMarkWidth(110);
    root->addWidget(m_pivot);

    // Stacked Pages
    m_stackedWidget = new QStackedWidget(this);

    m_salvageTab = new SalvageRepairTab(ffmpegLocator, probeService, jobQueue, this);
    m_stackedWidget->addWidget(m_salvageTab);

    m_scanTab = new ScanRepairTab(ffmpegLocator, probeService, jobQueue, this);
    m_stackedWidget->addWidget(m_scanTab);

    m_untruncTab = new UntruncRepairTab(ffmpegLocator, probeService, jobQueue, untruncLocator, this);
    m_stackedWidget->addWidget(m_untruncTab);

    root->addWidget(m_stackedWidget, 1);

    connect(m_pivot, &ElaPivot::pivotClicked, m_stackedWidget, &QStackedWidget::setCurrentIndex);

    // Preview fixed video cross-tab interaction
    connect(m_untruncTab, &UntruncRepairTab::previewRequested, this, [this](const QString &path) {
        m_scanTab->loadAndPlayMedia(path);
        m_pivot->setCurrentIndex(1);
        m_stackedWidget->setCurrentIndex(1);
    });

    retranslateUi();
}

RepairPage::~RepairPage() = default;

void RepairPage::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        m_isDragHover = true;
        update();
        event->acceptProposedAction();
    }
}

void RepairPage::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);
    if (m_isDragHover) {
        m_isDragHover = false;
        update();
    }
}

void RepairPage::dropEvent(QDropEvent *event)
{
    if (m_isDragHover) {
        m_isDragHover = false;
        update();
    }
    const auto urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;
    const QString local = urls.first().toLocalFile();
    if (local.isEmpty()) return;

    const int idx = m_pivot->getCurrentIndex();
    if (idx == 0 && m_salvageTab) {
        m_salvageTab->handleDroppedFile(local);
    } else if (idx == 1 && m_scanTab) {
        m_scanTab->handleDroppedFile(local);
    } else if (idx == 2 && m_untruncTab) {
        m_untruncTab->handleDroppedFile(local);
    }
}

void RepairPage::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    if (m_isDragHover) {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QRectF r = rect().adjusted(6, 6, -6, -6);
        p.setPen(QPen(QColor(0, 120, 212, 220), 2.0, Qt::DashLine));
        p.setBrush(QColor(0, 120, 212, 20));
        p.drawRoundedRect(r, 8.0, 8.0);
    }
}

void RepairPage::retranslateUi()
{
    const int currentIdx = m_pivot->getCurrentIndex();
    if (!m_salvageTabTitle.isEmpty()) {
        m_pivot->removePivot(m_salvageTabTitle);
        m_pivot->removePivot(m_scanTabTitle);
        m_pivot->removePivot(m_repairTabTitle);
    }
    m_salvageTabTitle = trId("repair.tab.salvage");
    m_scanTabTitle = trId("repair.tab.scan");
    m_repairTabTitle = trId("repair.tab.untrunc");
    m_pivot->appendPivot(m_salvageTabTitle);
    m_pivot->appendPivot(m_scanTabTitle);
    m_pivot->appendPivot(m_repairTabTitle);
    m_pivot->setCurrentIndex(currentIdx >= 0 ? currentIdx : 0);

    m_pageTitleLabel->setText(trId("nav.repair"));

    if (m_salvageTab) m_salvageTab->retranslateUi();
    if (m_scanTab) m_scanTab->retranslateUi();
    if (m_untruncTab) m_untruncTab->retranslateUi();
}
