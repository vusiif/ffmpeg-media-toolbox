#include "QueuePage.h"

#include "i18n/TrId.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

#include "ElaProgressBar.h"
#include "ElaPushButton.h"
#include "ElaText.h"
#include "jobs/Job.h"
#include "jobs/JobListModel.h"
#include "jobs/JobQueue.h"

QueuePage::QueuePage(JobQueue *queue, QWidget *parent)
    : QWidget(parent)
    , m_queue(queue)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(30, 30, 30, 30);
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

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    m_rowsContainer = new QWidget(scroll);
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

    auto *layout = new QVBoxLayout(row);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(6);

    auto *nameRow = new QHBoxLayout();
    auto *nameLabel = new QLabel(job->name(), row);
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

    auto *buttonRow = new QHBoxLayout();
    buttonRow->addStretch();
    auto *cancelButton = new ElaPushButton(row);
    auto *retryButton = new ElaPushButton(row);
    connect(cancelButton, &ElaPushButton::clicked, this, [=]() {
        m_queue->cancelJob(job->id().toString());
    });
    connect(retryButton, &ElaPushButton::clicked, this, [=]() {
        m_queue->retryJob(job->id().toString());
    });
    buttonRow->addWidget(cancelButton);
    buttonRow->addWidget(retryButton);
    layout->addLayout(buttonRow);

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

    m_cancelButtons[id]->setVisible(status == static_cast<int>(JobStatus::Pending)
                                    || status == static_cast<int>(JobStatus::Running));
    m_retryButtons[id]->setVisible(status == static_cast<int>(JobStatus::Failed)
                                   || status == static_cast<int>(JobStatus::Cancelled));
    m_cancelButtons[id]->setText(trId("queue.cancel"));
    m_retryButtons[id]->setText(trId("queue.retry"));
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

    for (auto it = m_rows.constBegin(); it != m_rows.constEnd(); ++it) {
        Job *job = m_queue->model()->findJobById(it.key().toString());
        if (job) updateRow(job);
    }
}
