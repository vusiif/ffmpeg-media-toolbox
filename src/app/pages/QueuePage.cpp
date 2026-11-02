#include "QueuePage.h"

#include "i18n/TrId.h"

#include <QApplication>
#include <QClipboard>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextCursor>
#include <QVBoxLayout>

#include "ElaComboBox.h"
#include "ElaPlainTextEdit.h"
#include "ElaProgressBar.h"
#include "ElaPushButton.h"
#include "ElaScrollArea.h"
#include "ElaScrollBar.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "jobs/Job.h"
#include "jobs/JobListModel.h"
#include "jobs/JobQueue.h"

QueuePage::QueuePage(JobQueue *queue, QWidget *parent)
    : QWidget(parent)
    , m_queue(queue)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(30, 24, 30, 24);
    root->setSpacing(14);

    // 1. Title & Global Action Bar
    auto *titleRow = new QHBoxLayout();
    m_titleLabel = new ElaText(this);
    m_titleLabel->setTextPixelSize(28);

    m_pauseButton = new ElaPushButton(this);
    connect(m_pauseButton, &ElaPushButton::clicked, this, [this]() {
        m_queue->setPaused(!m_queue->isPaused());
    });

    m_cancelAllButton = new ElaPushButton(this);
    connect(m_cancelAllButton, &ElaPushButton::clicked, this, [this]() {
        m_queue->cancelAll();
    });

    m_retryAllButton = new ElaPushButton(this);
    connect(m_retryAllButton, &ElaPushButton::clicked, this, [this]() {
        m_queue->retryAllFailed();
    });

    m_clearButton = new ElaPushButton(this);
    connect(m_clearButton, &ElaPushButton::clicked, this, [this]() {
        m_queue->clearCompleted();
    });

    titleRow->addWidget(m_titleLabel);
    titleRow->addStretch();
    titleRow->addWidget(m_pauseButton);
    titleRow->addWidget(m_cancelAllButton);
    titleRow->addWidget(m_retryAllButton);
    titleRow->addWidget(m_clearButton);
    root->addLayout(titleRow);

    // 2. Control Toolbar & Stats Bar
    auto *controlBar = new QFrame(this);
    controlBar->setStyleSheet(QStringLiteral("QFrame { background-color: rgba(255, 255, 255, 0.03); "
                                             "border: 1px solid rgba(255, 255, 255, 0.08); border-radius: 6px; }"));
    auto *controlLayout = new QHBoxLayout(controlBar);
    controlLayout->setContentsMargins(14, 10, 14, 10);
    controlLayout->setSpacing(12);

    m_concurrencyLabel = new ElaText(trId("queue.concurrency"), 13, controlBar);
    m_concurrencySpin = new ElaSpinBox(controlBar);
    m_concurrencySpin->setRange(1, 16);
    m_concurrencySpin->setValue(m_queue->maxConcurrent());
    m_concurrencySpin->setFixedWidth(70);
    connect(m_concurrencySpin, &ElaSpinBox::valueChanged, this, [this](int val) {
        m_queue->setMaxConcurrent(val);
    });

    m_postActionLabel = new ElaText(trId("queue.postAction"), 13, controlBar);
    m_postActionCombo = new ElaComboBox(controlBar);
    m_postActionCombo->addItem(trId("queue.post.none"));
    m_postActionCombo->addItem(trId("queue.post.openFolder"));
    m_postActionCombo->addItem(trId("queue.post.beep"));
    m_postActionCombo->setFixedWidth(150);
    connect(m_postActionCombo, &ElaComboBox::currentIndexChanged, this, [this](int idx) {
        m_queue->setPostQueueAction(static_cast<JobQueue::PostQueueAction>(idx));
    });

    m_statsLabel = new ElaText(controlBar);
    m_statsLabel->setTextPixelSize(13);

    controlLayout->addWidget(m_concurrencyLabel);
    controlLayout->addWidget(m_concurrencySpin);
    controlLayout->addSpacing(16);
    controlLayout->addWidget(m_postActionLabel);
    controlLayout->addWidget(m_postActionCombo);
    controlLayout->addStretch();
    controlLayout->addWidget(m_statsLabel);
    root->addWidget(controlBar);

    // 3. Scrollable Rows Container
    auto *scroll = new ElaScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_rowsContainer = new QWidget(scroll);
    m_rowsContainer->setStyleSheet(QStringLiteral("background-color: transparent;"));
    m_rowsLayout = new QVBoxLayout(m_rowsContainer);
    m_rowsLayout->setContentsMargins(0, 0, 0, 0);
    m_rowsLayout->setSpacing(8);
    m_rowsLayout->addStretch();

    m_emptyLabel = new ElaText(QString(), 13, m_rowsContainer);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_rowsLayout->insertWidget(0, m_emptyLabel);

    scroll->setWidget(m_rowsContainer);
    root->addWidget(scroll, 1);

    // 4. Model Connections
    connect(m_queue->model(), &QAbstractItemModel::rowsInserted,
            this, &QueuePage::onRowsInserted);

    connect(m_queue->model(), &QAbstractItemModel::rowsMoved,
            this, &QueuePage::onRowsMoved);

    connect(m_queue->model(), &QAbstractItemModel::rowsAboutToBeRemoved,
            this, [this](const QModelIndex &, int first, int last) {
                m_pendingRemoval.clear();
                for (int i = first; i <= last; ++i) {
                    if (Job *job = m_queue->model()->getJob(i)) {
                        m_pendingRemoval.append(job->id());
                    }
                }
            });

    connect(m_queue->model(), &QAbstractItemModel::rowsRemoved,
            this, [this](const QModelIndex &, int first, int last) {
                Q_UNUSED(first)
                Q_UNUSED(last)
                for (const QUuid &id : std::as_const(m_pendingRemoval)) {
                    destroyRow(id);
                }
                m_pendingRemoval.clear();
                m_emptyLabel->setVisible(m_rows.isEmpty());
                m_rowsLayout->invalidate();
                updateStatsBar();
            });

    connect(m_queue, &JobQueue::runningCountChanged, this, &QueuePage::updateStatsBar);
    connect(m_queue, &JobQueue::queuePausedChanged, this, [this](bool paused) {
        m_pauseButton->setText(paused ? trId("queue.resume") : trId("queue.pause"));
        updateStatsBar();
    });

    const int initialCount = m_queue->model()->count();
    if (initialCount > 0) {
        onRowsInserted(QModelIndex(), 0, initialCount - 1);
    }

    retranslateUi();
    updateStatsBar();
}

void QueuePage::onRowsInserted(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent)
    JobListModel *model = m_queue->model();
    for (int i = first; i <= last; ++i) {
        Job *job = model->getJob(i);
        if (!job || m_rows.contains(job->id())) continue;

        QFrame *row = createRow(job);
        m_rows[job->id()].frame = row;
        m_rowsLayout->insertWidget(m_rowsLayout->count() - 1, row);

        connect(job, &Job::statusChanged, this, [this, job]() {
            updateRow(job);
            updateStatsBar();
        });
        connect(job, &Job::progressChanged, this, &QueuePage::updateJobRow);
        connect(job, &Job::priorityChanged, this, &QueuePage::updateJobRow);
    }
    m_emptyLabel->setVisible(m_rows.isEmpty());
    m_rowsLayout->invalidate();
    syncRowOrder();
    updateStatsBar();
}

void QueuePage::onRowsMoved(const QModelIndex &parent, int start, int end,
                            const QModelIndex &destination, int row)
{
    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)
    Q_UNUSED(destination)
    Q_UNUSED(row)
    syncRowOrder();
}

void QueuePage::syncRowOrder()
{
    JobListModel *model = m_queue->model();
    for (int i = 0; i < model->count(); ++i) {
        Job *job = model->getJob(i);
        if (!job) continue;
        auto it = m_rows.find(job->id());
        if (it != m_rows.end() && it.value().frame) {
            m_rowsLayout->removeWidget(it.value().frame);
            m_rowsLayout->insertWidget(i, it.value().frame);
        }
    }
}

QFrame* QueuePage::createRow(Job *job)
{
    auto *row = new QFrame(m_rowsContainer);
    row->setFrameShape(QFrame::StyledPanel);
    row->setStyleSheet(QStringLiteral("QFrame { background-color: rgba(255, 255, 255, 0.04); "
                                      "border: 1px solid rgba(255, 255, 255, 0.08); border-radius: 6px; }"));

    auto *layout = new QVBoxLayout(row);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(8);

    // Top name + subpath + priority + status
    auto *nameRow = new QHBoxLayout();
    QString displayName = job->name();
    if (!job->relativeSubPath().isEmpty()) {
        displayName = QStringLiteral("[%1] %2").arg(job->relativeSubPath(), job->name());
    }
    auto *nameLabel = new QLabel(displayName, row);
    nameLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));
    nameLabel->setWordWrap(true);

    auto *priorityCombo = new ElaComboBox(row);
    priorityCombo->addItem(trId("queue.priority.low"));
    priorityCombo->addItem(trId("queue.priority.normal"));
    priorityCombo->addItem(trId("queue.priority.high"));
    priorityCombo->addItem(trId("queue.priority.urgent"));
    priorityCombo->setCurrentIndex(static_cast<int>(job->priority()));
    priorityCombo->setFixedWidth(110);
    connect(priorityCombo, &ElaComboBox::currentIndexChanged, this, [job](int idx) {
        job->setPriority(static_cast<JobPriority>(idx));
    });

    auto *statusLabel = new QLabel(row);
    nameRow->addWidget(nameLabel, 1);
    nameRow->addWidget(priorityCombo);
    nameRow->addWidget(statusLabel);
    layout->addLayout(nameRow);

    auto *progressBar = new ElaProgressBar(row);
    progressBar->setRange(0, 1000);
    layout->addWidget(progressBar);

    auto *errorLabel = new QLabel(row);
    errorLabel->setWordWrap(true);
    errorLabel->setVisible(false);
    layout->addWidget(errorLabel);

    // Collapsible details
    auto *detailsContainer = new QWidget(row);
    detailsContainer->setVisible(false);
    auto *detailsLayout = new QVBoxLayout(detailsContainer);
    detailsLayout->setContentsMargins(0, 4, 0, 0);
    detailsLayout->setSpacing(4);

    auto *commandCaption = new QLabel(detailsContainer);
    auto *commandLabel = new QLabel(job->commandDisplayString(), detailsContainer);
    commandLabel->setWordWrap(true);
    commandLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    auto *copyButton = new ElaPushButton(detailsContainer);
    connect(copyButton, &ElaPushButton::clicked, this, [job]() {
        QApplication::clipboard()->setText(job->commandDisplayString());
    });
    detailsLayout->addWidget(commandCaption);
    detailsLayout->addWidget(commandLabel);
    detailsLayout->addWidget(copyButton, 0, Qt::AlignRight);

    auto *logsCaption = new QLabel(detailsContainer);
    auto *logView = new ElaPlainTextEdit(detailsContainer);
    logView->setReadOnly(true);
    logView->setMaximumHeight(140);
    logView->setPlaceholderText(trId("queue.noLogs"));
    detailsLayout->addWidget(logsCaption);
    detailsLayout->addWidget(logView);
    layout->addWidget(detailsContainer);

    // Bottom Action Row (Reorder + Operations)
    auto *buttonRow = new QHBoxLayout();

    auto *topButton = new ElaPushButton(QStringLiteral("⏫"), row);
    topButton->setToolTip(trId("queue.moveToTop"));
    topButton->setFixedWidth(36);
    connect(topButton, &ElaPushButton::clicked, this, [this, job]() {
        m_queue->moveJobToTop(job->id().toString());
    });

    auto *upButton = new ElaPushButton(QStringLiteral("⬆"), row);
    upButton->setToolTip(trId("queue.moveUp"));
    upButton->setFixedWidth(36);
    connect(upButton, &ElaPushButton::clicked, this, [this, job]() {
        m_queue->moveJobUp(job->id().toString());
    });

    auto *downButton = new ElaPushButton(QStringLiteral("⬇"), row);
    downButton->setToolTip(trId("queue.moveDown"));
    downButton->setFixedWidth(36);
    connect(downButton, &ElaPushButton::clicked, this, [this, job]() {
        m_queue->moveJobDown(job->id().toString());
    });

    buttonRow->addWidget(topButton);
    buttonRow->addWidget(upButton);
    buttonRow->addWidget(downButton);
    buttonRow->addStretch();

    auto *detailsButton = new ElaPushButton(row);
    auto *cancelButton = new ElaPushButton(row);
    auto *retryButton = new ElaPushButton(row);
    auto *removeButton = new ElaPushButton(QStringLiteral("❌"), row);
    removeButton->setToolTip(trId("queue.remove"));
    removeButton->setFixedWidth(36);

    connect(detailsButton, &ElaPushButton::clicked, this, [this, job]() {
        auto it = m_rows.find(job->id());
        if (it == m_rows.end()) return;
        it->details.open = !it->details.open;
        it->details.container->setVisible(it->details.open);
        it->details.button->setText(it->details.open ? trId("queue.hideDetails") : trId("queue.details"));
        if (it->details.open) refreshLogView(job);
    });

    connect(cancelButton, &ElaPushButton::clicked, this, [this, job]() {
        m_queue->cancelJob(job->id().toString());
    });

    connect(retryButton, &ElaPushButton::clicked, this, [this, job]() {
        m_queue->retryJob(job->id().toString());
    });

    connect(removeButton, &ElaPushButton::clicked, this, [this, job]() {
        m_queue->model()->removeJobById(job->id().toString());
    });

    buttonRow->addWidget(detailsButton);
    buttonRow->addWidget(cancelButton);
    buttonRow->addWidget(retryButton);
    buttonRow->addWidget(removeButton);
    layout->addLayout(buttonRow);

    DetailsState details;
    details.button = detailsButton;
    details.container = detailsContainer;
    details.copyButton = copyButton;
    details.commandCaption = commandCaption;
    details.logsCaption = logsCaption;
    details.logView = logView;

    RowWidgets rw;
    rw.frame = row;
    rw.nameLabel = nameLabel;
    rw.statusLabel = statusLabel;
    rw.errorLabel = errorLabel;
    rw.progressBar = progressBar;
    rw.priorityCombo = priorityCombo;
    rw.topButton = topButton;
    rw.upButton = upButton;
    rw.downButton = downButton;
    rw.cancelButton = cancelButton;
    rw.retryButton = retryButton;
    rw.removeButton = removeButton;
    rw.details = details;

    m_rows.insert(job->id(), rw);
    setDetailsTexts(rw.details);
    updateRow(job);
    return row;
}

void QueuePage::updateJobRow()
{
    auto *job = qobject_cast<Job *>(sender());
    if (job) updateRow(job);
}

void QueuePage::updateRow(Job *job)
{
    const QUuid id = job->id();
    if (!m_rows.contains(id)) return;

    RowWidgets &rw = m_rows[id];
    const int status = static_cast<int>(job->status());

    rw.statusLabel->setText(statusText(status));
    rw.progressBar->setValue(static_cast<int>(job->progress() * 1000));
    rw.progressBar->setVisible(status == static_cast<int>(JobStatus::Running));

    const bool failed = status == static_cast<int>(JobStatus::Failed);
    rw.errorLabel->setText(job->errorMessage());
    rw.errorLabel->setVisible(failed && !job->errorMessage().isEmpty());

    rw.priorityCombo->setCurrentIndex(static_cast<int>(job->priority()));

    if (rw.details.open) {
        refreshLogView(job);
    }

    rw.cancelButton->setVisible(status == static_cast<int>(JobStatus::Pending)
                                || status == static_cast<int>(JobStatus::Running));
    rw.retryButton->setVisible(status == static_cast<int>(JobStatus::Failed)
                               || status == static_cast<int>(JobStatus::Cancelled));
    rw.cancelButton->setText(trId("queue.cancel"));
    rw.retryButton->setText(trId("queue.retry"));
}

void QueuePage::refreshLogView(Job *job)
{
    const auto it = m_rows.constFind(job->id());
    if (it == m_rows.constEnd()) return;

    QStringList lines;
    const QList<JobLog> logs = job->logs();
    lines.reserve(logs.size());
    for (const JobLog &log : logs) {
        lines << log.timestamp.toString(QStringLiteral("HH:mm:ss"))
                     + QStringLiteral("  ") + log.message;
    }
    it->details.logView->setPlainText(lines.join(QLatin1Char('\n')));
    it->details.logView->moveCursor(QTextCursor::End);
}

void QueuePage::setDetailsTexts(const DetailsState &details)
{
    details.button->setText(details.open ? trId("queue.hideDetails")
                                         : trId("queue.details"));
    details.commandCaption->setText(trId("queue.command") + QStringLiteral(":"));
    details.logsCaption->setText(trId("queue.logs") + QStringLiteral(":"));
    details.copyButton->setText(trId("common.copy"));
    details.logView->setPlaceholderText(trId("queue.noLogs"));
}

void QueuePage::destroyRow(const QUuid &id)
{
    if (m_rows.contains(id)) {
        RowWidgets rw = m_rows.take(id);
        if (rw.frame) {
            delete rw.frame;
        }
    }
}

QString QueuePage::statusText(int status) const
{
    switch (status) {
    case static_cast<int>(JobStatus::Pending): return trId("queue.pending");
    case static_cast<int>(JobStatus::Preparing): return trId("queue.pending");
    case static_cast<int>(JobStatus::Running): return trId("queue.running");
    case static_cast<int>(JobStatus::Completed): return trId("queue.completed");
    case static_cast<int>(JobStatus::Failed): return trId("queue.failed");
    case static_cast<int>(JobStatus::Cancelled): return trId("queue.cancel");
    default: return QString();
    }
}

void QueuePage::updateStatsBar()
{
    JobListModel *model = m_queue->model();
    int total = model->count();
    int running = 0;
    int pending = 0;
    int completed = 0;
    int failed = 0;

    for (int i = 0; i < total; ++i) {
        if (Job *job = model->getJob(i)) {
            switch (job->status()) {
            case JobStatus::Running: running++; break;
            case JobStatus::Pending:
            case JobStatus::Preparing: pending++; break;
            case JobStatus::Completed: completed++; break;
            case JobStatus::Failed:
            case JobStatus::Cancelled: failed++; break;
            }
        }
    }

    const QString statsStr = trId("queue.stats")
                                 .arg(total)
                                 .arg(running)
                                 .arg(pending)
                                 .arg(completed)
                                 .arg(failed);
    m_statsLabel->setText(statsStr);
}

void QueuePage::retranslateUi()
{
    m_titleLabel->setText(trId("queue.title"));
    m_pauseButton->setText(m_queue->isPaused() ? trId("queue.resume") : trId("queue.pause"));
    m_cancelAllButton->setText(trId("queue.cancelAll"));
    m_retryAllButton->setText(trId("queue.retryFailed"));
    m_clearButton->setText(trId("common.clear"));

    m_concurrencyLabel->setText(trId("queue.concurrency"));
    m_postActionLabel->setText(trId("queue.postAction"));

    m_emptyLabel->setText(trId("queue.pending"));

    for (auto it = m_rows.constBegin(); it != m_rows.constEnd(); ++it) {
        setDetailsTexts(it.value().details);
        it.value().topButton->setToolTip(trId("queue.moveToTop"));
        it.value().upButton->setToolTip(trId("queue.moveUp"));
        it.value().downButton->setToolTip(trId("queue.moveDown"));
        it.value().removeButton->setToolTip(trId("queue.remove"));
    }

    for (auto it = m_rows.constBegin(); it != m_rows.constEnd(); ++it) {
        Job *job = m_queue->model()->findJobById(it.key().toString());
        if (job) updateRow(job);
    }

    updateStatsBar();
}
