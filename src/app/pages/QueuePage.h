#pragma once

#include <QMap>
#include <QUuid>
#include <QWidget>

class JobQueue;
class Job;
class QLabel;
class QFrame;
class ElaPlainTextEdit;
class QVBoxLayout;
class ElaProgressBar;
class ElaPushButton;
class ElaText;

class QueuePage : public QWidget
{
    Q_OBJECT

public:
    explicit QueuePage(JobQueue *queue, QWidget *parent = nullptr);

    void retranslateUi();

private slots:
    void onRowsInserted(const QModelIndex &parent, int first, int last);
    void updateJobRow();

private:
    struct DetailsState {
        ElaPushButton *button = nullptr;
        QWidget *container = nullptr;
        ElaPushButton *copyButton = nullptr;
        QLabel *commandCaption = nullptr;
        QLabel *commandLabel = nullptr;
        QLabel *logsCaption = nullptr;
        ElaPlainTextEdit *logView = nullptr;
        bool open = false;
    };

    QFrame* createRow(Job *job);
    void updateRow(Job *job);
    QString statusText(int status) const;
    void refreshLogView(Job *job);
    void setDetailsTexts(const DetailsState &details);
    void destroyRow(const QUuid &id);

    JobQueue *m_queue;

    ElaText *m_titleLabel = nullptr;
    ElaPushButton *m_clearButton = nullptr;
    ElaText *m_emptyLabel = nullptr;
    QWidget *m_rowsContainer = nullptr;
    QVBoxLayout *m_rowsLayout = nullptr;

    QMap<QUuid, QFrame *> m_rows;
    QMap<QUuid, QLabel *> m_statusLabels;
    QMap<QUuid, QLabel *> m_errorLabels;
    QMap<QUuid, ElaProgressBar *> m_progressBars;
    QMap<QUuid, ElaPushButton *> m_cancelButtons;
    QMap<QUuid, ElaPushButton *> m_retryButtons;
    QMap<QUuid, DetailsState> m_details;
    QList<QUuid> m_pendingRemoval;
};
