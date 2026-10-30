#include "HomePage.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "ElaPushButton.h"
#include "ElaScrollArea.h"
#include "ElaScrollPageArea.h"
#include "ElaText.h"
#include "ffmpeg/FFmpegCapabilities.h"
#include "ffmpeg/FFmpegLocator.h"
#include "i18n/TrId.h"
#include "jobs/JobListModel.h"
#include "jobs/JobQueue.h"
#include "repair/UntruncLocator.h"

HomePage::HomePage(FFmpegLocator *locator, FFmpegCapabilities *caps,
                   JobQueue *queue, UntruncLocator *untrunc, QWidget *parent)
    : QWidget(parent)
    , m_locator(locator)
    , m_caps(caps)
    , m_queue(queue)
    , m_untrunc(untrunc)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    m_scrollArea = new ElaScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setIsGrabGesture(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *scrollBody = new QWidget(m_scrollArea);
    scrollBody->setStyleSheet(QStringLiteral("background-color: transparent;"));
    auto *body = new QVBoxLayout(scrollBody);
    body->setContentsMargins(30, 26, 30, 26);
    body->setSpacing(18);

    // Header
    m_titleLabel = new ElaText(QString(), 13, scrollBody);
    m_titleLabel->setTextPixelSize(28);
    m_subtitleLabel = new ElaText(QString(), 13, scrollBody);
    m_subtitleLabel->setTextPixelSize(14);
    body->addWidget(m_titleLabel);
    body->addWidget(m_subtitleLabel);

    // 1. Environment & Hardware Acceleration Card
    m_envCard = new ElaScrollPageArea(scrollBody);
    m_envCard->setMinimumHeight(0);
    m_envCard->setMaximumHeight(QWIDGETSIZE_MAX);
    auto *envLayout = new QVBoxLayout(m_envCard);
    envLayout->setContentsMargins(18, 16, 18, 16);
    envLayout->setSpacing(12);

    auto *envTopRow = new QHBoxLayout();
    m_envTitle = new ElaText(QString(), 15, m_envCard);
    m_envTitle->setStyleSheet(QStringLiteral("font-weight: bold;"));
    m_rescanButton = new ElaPushButton(m_envCard);
    m_rescanButton->setFixedHeight(32);
    connect(m_rescanButton, &ElaPushButton::clicked, this, [=]() {
        m_locator->autoDetect();
        m_versionValue->setText(m_locator->ffmpegVersion());
        m_caps->scan();
        if (m_untrunc) {
            m_untrunc->scan();
        }
    });
    envTopRow->addWidget(m_envTitle);
    envTopRow->addStretch();
    envTopRow->addWidget(m_rescanButton);
    envLayout->addLayout(envTopRow);

    auto *envGrid = new QGridLayout();
    envGrid->setHorizontalSpacing(24);
    envGrid->setVerticalSpacing(10);

    m_versionCaption = new ElaText(QString(), 13, m_envCard);
    m_versionValue = new ElaText(QString(), 13, m_envCard);
    m_versionValue->setStyleSheet(QStringLiteral("font-weight: bold;"));
    envGrid->addWidget(m_versionCaption, 0, 0);
    envGrid->addWidget(m_versionValue, 0, 1);

    m_hwCaption = new ElaText(QString(), 13, m_envCard);
    m_hwValue = new ElaText(QString(), 13, m_envCard);
    m_hwValue->setStyleSheet(QStringLiteral("font-weight: bold; color: #0078d4;"));
    envGrid->addWidget(m_hwCaption, 0, 2);
    envGrid->addWidget(m_hwValue, 0, 3);

    m_codecsCaption = new ElaText(QString(), 13, m_envCard);
    m_codecsValue = new ElaText(QString(), 13, m_envCard);
    m_codecsValue->setStyleSheet(QStringLiteral("font-weight: bold;"));
    envGrid->addWidget(m_codecsCaption, 1, 0);
    envGrid->addWidget(m_codecsValue, 1, 1);

    m_untruncCaption = new ElaText(QString(), 13, m_envCard);
    m_untruncValue = new ElaText(QString(), 13, m_envCard);
    m_untruncValue->setStyleSheet(QStringLiteral("font-weight: bold;"));
    envGrid->addWidget(m_untruncCaption, 1, 2);
    envGrid->addWidget(m_untruncValue, 1, 3);

    envLayout->addLayout(envGrid);
    body->addWidget(m_envCard);

    // 2. Queue Status Card
    m_queueCard = new ElaScrollPageArea(scrollBody);
    m_queueCard->setMinimumHeight(0);
    m_queueCard->setMaximumHeight(QWIDGETSIZE_MAX);
    auto *queueLayout = new QVBoxLayout(m_queueCard);
    queueLayout->setContentsMargins(18, 16, 18, 16);
    queueLayout->setSpacing(12);

    auto *queueTopRow = new QHBoxLayout();
    m_queueTitle = new ElaText(QString(), 15, m_queueCard);
    m_queueTitle->setStyleSheet(QStringLiteral("font-weight: bold;"));
    m_viewQueueButton = new ElaPushButton(m_queueCard);
    m_viewQueueButton->setFixedHeight(32);
    connect(m_viewQueueButton, &ElaPushButton::clicked, this, [this]() {
        if (m_queuePage) {
            emit navigationRequested(m_queuePage);
        }
    });
    queueTopRow->addWidget(m_queueTitle);
    queueTopRow->addStretch();
    queueTopRow->addWidget(m_viewQueueButton);
    queueLayout->addLayout(queueTopRow);

    auto *queueStatusRow = new QHBoxLayout();
    queueStatusRow->setSpacing(24);
    m_queueRunningText = new ElaText(QString(), 13, m_queueCard);
    m_queueRunningText->setStyleSheet(QStringLiteral("font-weight: bold; color: #0078d4;"));
    m_queuePendingText = new ElaText(QString(), 13, m_queueCard);
    m_queueCompletedText = new ElaText(QString(), 13, m_queueCard);
    queueStatusRow->addWidget(m_queueRunningText);
    queueStatusRow->addWidget(m_queuePendingText);
    queueStatusRow->addWidget(m_queueCompletedText);
    queueStatusRow->addStretch();
    queueLayout->addLayout(queueStatusRow);

    body->addWidget(m_queueCard);

    // 3. Quick Launch Section
    m_quickLaunchTitle = new ElaText(QString(), 16, scrollBody);
    m_quickLaunchTitle->setStyleSheet(QStringLiteral("font-weight: bold;"));
    body->addWidget(m_quickLaunchTitle);

    auto buildLaunchCard = [this, scrollBody](ElaScrollPageArea *&card, ElaText *&titleLabel,
                                              ElaText *&descLabel, ElaPushButton *&btn,
                                              QWidget *&targetPageMember) {
        card = new ElaScrollPageArea(scrollBody);
        card->setMinimumHeight(130);
        card->setMaximumHeight(QWIDGETSIZE_MAX);
        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(18, 16, 18, 16);
        layout->setSpacing(8);

        titleLabel = new ElaText(QString(), 15, card);
        titleLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));
        layout->addWidget(titleLabel);

        descLabel = new ElaText(QString(), 12, card);
        descLabel->setWordWrap(true);
        layout->addWidget(descLabel, 1);

        auto *btnRow = new QHBoxLayout();
        btnRow->addStretch();
        btn = new ElaPushButton(card);
        btn->setFixedHeight(32);
        btn->setMinimumWidth(110);
        connect(btn, &ElaPushButton::clicked, this, [this, &targetPageMember]() {
            if (targetPageMember) {
                emit navigationRequested(targetPageMember);
            }
        });
        btnRow->addWidget(btn);
        layout->addLayout(btnRow);
    };

    auto *launchGrid = new QGridLayout();
    launchGrid->setHorizontalSpacing(16);
    launchGrid->setVerticalSpacing(16);

    buildLaunchCard(m_cardConvert, m_cardConvertTitle, m_cardConvertDesc, m_cardConvertBtn, m_convertPage);
    buildLaunchCard(m_cardVideo, m_cardVideoTitle, m_cardVideoDesc, m_cardVideoBtn, m_videoPage);
    buildLaunchCard(m_cardRepair, m_cardRepairTitle, m_cardRepairDesc, m_cardRepairBtn, m_repairPage);
    buildLaunchCard(m_cardBatch, m_cardBatchTitle, m_cardBatchDesc, m_cardBatchBtn, m_batchPage);

    launchGrid->addWidget(m_cardConvert, 0, 0);
    launchGrid->addWidget(m_cardVideo, 0, 1);
    launchGrid->addWidget(m_cardRepair, 1, 0);
    launchGrid->addWidget(m_cardBatch, 1, 1);
    body->addLayout(launchGrid);

    body->addStretch();
    m_scrollArea->setWidget(scrollBody);
    root->addWidget(m_scrollArea);

    connect(m_locator, &FFmpegLocator::pathsChanged, this, [this]() {
        m_versionValue->setText(m_locator->ffmpegVersion());
        m_caps->scan();
    });
    connect(m_caps, &FFmpegCapabilities::capabilitiesChanged,
            this, &HomePage::refreshHwStatus);
    if (m_untrunc) {
        connect(m_untrunc, &UntruncLocator::pathChanged, this, &HomePage::refreshUntruncStatus);
        connect(m_untrunc, &UntruncLocator::availabilityChanged, this, &HomePage::refreshUntruncStatus);
    }
    if (m_queue && m_queue->model()) {
        connect(m_queue->model(), &JobListModel::countChanged, this, &HomePage::refreshQueueStatus);
        connect(m_queue, &JobQueue::runningCountChanged, this, &HomePage::refreshQueueStatus);
    }

    retranslateUi();
}

void HomePage::setTargetPages(QWidget *convertPage, QWidget *videoPage,
                              QWidget *repairPage, QWidget *batchPage,
                              QWidget *queuePage)
{
    m_convertPage = convertPage;
    m_videoPage = videoPage;
    m_repairPage = repairPage;
    m_batchPage = batchPage;
    m_queuePage = queuePage;
}

void HomePage::retranslateUi()
{
    m_titleLabel->setText(trId("home.title"));
    m_subtitleLabel->setText(trId("home.dropHint"));

    m_envTitle->setText(trId("home.envCard"));
    m_versionCaption->setText(trId("ffmpeg.version") + QStringLiteral(":"));
    m_versionValue->setText(m_locator->ffmpegVersion());
    m_hwCaption->setText(trId("home.hwAccel") + QStringLiteral(":"));
    m_codecsCaption->setText(trId("home.codecs") + QStringLiteral(":"));
    m_untruncCaption->setText(trId("home.untrunc") + QStringLiteral(":"));
    m_rescanButton->setText(trId("ffmpeg.rescan"));

    m_queueTitle->setText(trId("home.queueCard"));
    m_viewQueueButton->setText(trId("home.queue.view"));

    m_quickLaunchTitle->setText(trId("home.quickLaunch"));
    m_cardConvertTitle->setText(trId("home.card.convert.title"));
    m_cardConvertDesc->setText(trId("home.card.convert.desc"));
    m_cardConvertBtn->setText(trId("home.launch"));

    m_cardVideoTitle->setText(trId("home.card.video.title"));
    m_cardVideoDesc->setText(trId("home.card.video.desc"));
    m_cardVideoBtn->setText(trId("home.launch"));

    m_cardRepairTitle->setText(trId("home.card.repair.title"));
    m_cardRepairDesc->setText(trId("home.card.repair.desc"));
    m_cardRepairBtn->setText(trId("home.launch"));

    m_cardBatchTitle->setText(trId("home.card.batch.title"));
    m_cardBatchDesc->setText(trId("home.card.batch.desc"));
    m_cardBatchBtn->setText(trId("home.launch"));

    refreshHwStatus();
    refreshUntruncStatus();
    refreshQueueStatus();
}

void HomePage::refreshHwStatus()
{
    if (m_caps->hasHwAcceleration()) {
        m_hwValue->setText(m_caps->primaryHwDeviceName());
    } else {
        m_hwValue->setText(trId("convert.hwAccel.none"));
    }

    const int encCount = m_caps->encoderCount();
    const int hwCount = m_caps->hwAccelCount();
    m_codecsValue->setText(QStringLiteral("%1 (HW: %2)").arg(encCount).arg(hwCount));
}

void HomePage::refreshUntruncStatus()
{
    if (!m_untrunc) {
        m_untruncValue->setText(trId("home.untrunc.missing"));
        return;
    }
    if (m_untrunc->isAvailable()) {
        m_untruncValue->setText(QStringLiteral("%1 (%2)").arg(trId("home.untrunc.ready"), m_untrunc->untruncPath()));
    } else {
        m_untruncValue->setText(trId("home.untrunc.missing"));
    }
}

void HomePage::refreshQueueStatus()
{
    if (!m_queue || !m_queue->model()) {
        m_queueRunningText->setText(trId("home.queue.idle"));
        m_queuePendingText->clear();
        m_queueCompletedText->clear();
        return;
    }

    const int running = m_queue->model()->runningCount();
    const int completed = m_queue->model()->completedCount();
    const int total = m_queue->model()->count();
    const int pending = qMax(0, total - running - completed);

    if (total == 0) {
        m_queueRunningText->setText(trId("home.queue.idle"));
        m_queuePendingText->clear();
        m_queueCompletedText->clear();
    } else {
        m_queueRunningText->setText(QStringLiteral("%1: %2").arg(trId("queue.running")).arg(running));
        m_queuePendingText->setText(QStringLiteral("%1: %2").arg(trId("queue.pending")).arg(pending));
        m_queueCompletedText->setText(QStringLiteral("%1: %2").arg(trId("queue.completed")).arg(completed));
    }
}
