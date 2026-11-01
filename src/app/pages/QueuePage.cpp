#include "QueuePage.h"

#include "i18n/TrId.h"

#include <QApplication>
#include <QClipboard>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextCursor>
#include <QVBoxLayout>

#include "ElaPlainTextEdit.h"
#include "ElaProgressBar.h"
#include "ElaPushButton.h"
#include "ElaScrollArea.h"
#include "ElaScrollBar.h"
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
    root->setSpacing(16);

    auto *titleRow = new QHBoxLayout();
    m_titleLabel = new ElaText(this);
    m_titleLabel->setTextPixelSize(28);
    m_clearButton = new ElaPushButton(this);
    connect(m_clearButton, &ElaPushButton::clicked, this, [=]() {
        m_queue->clearCompleted();
    });
    titleRow->addWidget(m_titleLabel);
    titleRow->addStretch();
    titleRow->addWidget(m_clearButton);
    root->addLayout(titleRow);

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

    connect(m_queue->model(), &QAbstractItemModel::rowsInserted,
            this, &QueuePage::onRowsInserted);

    // Resolve ids while the rows still exist, then drop the widgets once the
    // model has actually removed them - otherwise "Clear" leaves stale frames.
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
            });

    const int initialCount = m_queue->model()->count();
    if (initialCount > 0) {
        onRowsInserted(QModelIndex(), 0, initialCount - 1);
    }

    retranslateUi();
}

void QueuePage::onRowsInserted(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent)
    JobListModel *model = m_queue->model();
    for (int i = first; i <= last; ++i) {
        Job *job = model->getJob(i);
        if (!job || m_rows.contains(job->id())) continue;

        QFrame *row = createRow(job);
        m_rows.insert(job->id(), row);
        m_rowsLayout->insertWidget(m_rowsLayout->count() - 1, row);

        connect(job, &Job::statusChanged, this, &QueuePage::updateJobRow);
        connect(job, &Job::progressChanged, this, &QueuePage::updateJobRow);
    }
    m_emptyLabel->setVisible(m_rows.isEmpty());
    m_rowsLayout->invalidate();
}

QFrame* QueuePage::createRow(Job *job)
{
    auto *row = new QFrame(m_rowsContainer);
    row->setFrameShape(QFrame::StyledPanel);
    row->setStyleSheet(QStringLiteral("QFrame { background-color: rgba(255, 255, 255, 0.04); border: 1px solid rgba(255, 255, 255, 0.08); border-radius: 6px; }"));

    auto *layout = new QVBoxLayout(row);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(8);

    auto *nameRow = new QHBoxLayout();
    auto *nameLabel = new QLabel(job->name(), row);
    nameLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));
    nameLabel->setWordWrap(true);
    auto *statusLabel = new QLabel(row);
    nameRow->addWidget(nameLabel, 1);
    nameRow->addWidget(statusLabel);
    layout->addLayout(nameRow);

    auto *progressBar = new ElaProgressBar(row);
    progressBar->setRange(0, 1000);
    layout->addWidget(progressBar);

    auto *errorLabel = new QLabel(row);
    errorLabel->setWordWrap(true);
    errorLabel->setVisible(false);
    layout->addWidget(errorLabel);

    // Collapsible details: the exact command that will run plus the job log.
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

    auto *buttonRow = new QHBoxLayout();
    buttonRow->addStretch();
    auto *detailsButton = new ElaPushButton(row);
    auto *cancelButton = new ElaPushButton(row);
    auto *retryButton = new ElaPushButton(row);
    connect(detailsButton, &ElaPushButton::clicked, this, [this, job]() {
        auto it = m_details.find(job->id());
        if (it == m_details.end()) return;
        it->open = !it->open;
        it->container->setVisible(it->open);
        it->button->setText(it->open ? trId("queue.hideDetails") : trId("queue.details"));
        if (it->open) refreshLogView(job);
    });
    connect(cancelButton, &ElaPushButton::clicked, this, [=]() {
        m_queue->cancelJob(job->id().toString());
    });
    connect(retryButton, &ElaPushButton::clicked, this, [=]() {
        m_queue->retryJob(job->id().toString());
    });
    buttonRow->addWidget(detailsButton);
    buttonRow->addWidget(cancelButton);
    buttonRow->addWidget(retryButton);
    layout->addLayout(buttonRow);

    DetailsState details;
    details.button = detailsButton;
    details.container = detailsContainer;
    details.copyButton = copyButton;
    details.commandCaption = commandCaption;
    details.logsCaption = logsCaption;
    details.logView = logView;
    m_details.insert(job->id(), details);
    setDetailsTexts(m_details[job->id()]);

    m_statusLabels.insert(job->id(), statusLabel);
    m_errorLabels.insert(job->id(), errorLabel);
    m_progressBars.insert(job->id(), progressBar);
    m_cancelButtons.insert(job->id(), cancelButton);
    m_retryButtons.insert(job->id(), retryButton);

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

    const int status = static_cast<int>(job->status());
    m_statusLabels[id]->setText(statusText(status));
    m_progressBars[id]->setValue(static_cast<int>(job->progress() * 1000));
    m_progressBars[id]->setVisible(status == static_cast<int>(JobStatus::Running));

    const bool failed = status == static_cast<int>(JobStatus::Failed);
    m_errorLabels[id]->setText(job->errorMessage());
    m_errorLabels[id]->setVisible(failed && !job->errorMessage().isEmpty());

    const auto detailsIt = m_details.constFind(id);
    if (detailsIt != m_details.constEnd() && detailsIt->open) {
        refreshLogView(job);
    }

    m_cancelButtons[id]->setVisible(status == static_cast<int>(JobStatus::Pending)
                                    || status == static_cast<int>(JobStatus::Running));
    m_retryButtons[id]->setVisible(status == static_cast<int>(JobStatus::Failed)
                                   || status == static_cast<int>(JobStatus::Cancelled));
    m_cancelButtons[id]->setText(trId("queue.cancel"));
    m_retryButtons[id]->setText(trId("queue.retry"));
}

void QueuePage::refreshLogView(Job *job)
{
    const auto it = m_details.constFind(job->id());
    if (it == m_details.constEnd()) return;

    QStringList lines;
    const QList<JobLog> logs = job->logs();
    lines.reserve(logs.size());
    for (const JobLog &log : logs) {
        lines << log.timestamp.toString(QStringLiteral("HH:mm:ss"))
                     + QStringLiteral("  ") + log.message;
    }
    it->logView->setPlainText(lines.join(QLatin1Char('\n')));
    it->logView->moveCursor(QTextCursor::End);
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
    if (QFrame *row = m_rows.take(id)) {
        delete row;
    }
    m_statusLabels.remove(id);
    m_errorLabels.remove(id);
    m_progressBars.remove(id);
    m_cancelButtons.remove(id);
    m_retryButtons.remove(id);
    m_details.remove(id);
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

void QueuePage::retranslateUi()
{
    m_titleLabel->setText(trId("queue.title"));
    m_clearButton->setText(trId("common.clear"));
    m_emptyLabel->setText(trId("queue.pending"));

    for (auto it = m_details.constBegin(); it != m_details.constEnd(); ++it) {
        setDetailsTexts(it.value());
    }

    for (auto it = m_rows.constBegin(); it != m_rows.constEnd(); ++it) {
        Job *job = m_queue->model()->findJobById(it.key().toString());
        if (job) updateRow(job);
    }
}
