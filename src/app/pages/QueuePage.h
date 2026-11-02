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
class ElaSpinBox;
class ElaComboBox;

class QueuePage : public QWidget
{
    Q_OBJECT

public:
    explicit QueuePage(JobQueue *queue, QWidget *parent = nullptr);

    void retranslateUi();

private slots:
    void onRowsInserted(const QModelIndex &parent, int first, int last);
    void onRowsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row);
    void updateJobRow();
    void updateStatsBar();

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

    struct RowWidgets {
        QFrame *frame = nullptr;
        QLabel *nameLabel = nullptr;
        QLabel *statusLabel = nullptr;
        QLabel *errorLabel = nullptr;
        ElaProgressBar *progressBar = nullptr;
        ElaComboBox *priorityCombo = nullptr;
        ElaPushButton *topButton = nullptr;
        ElaPushButton *upButton = nullptr;
        ElaPushButton *downButton = nullptr;
        ElaPushButton *cancelButton = nullptr;
        ElaPushButton *retryButton = nullptr;
        ElaPushButton *removeButton = nullptr;
        DetailsState details;
    };

    QFrame* createRow(Job *job);
    void updateRow(Job *job);
    QString statusText(int status) const;
    void refreshLogView(Job *job);
    void setDetailsTexts(const DetailsState &details);
    void destroyRow(const QUuid &id);
    void syncRowOrder();

    JobQueue *m_queue;

    ElaText *m_titleLabel = nullptr;
    ElaText *m_statsLabel = nullptr;

    // Global action bar
    ElaPushButton *m_pauseButton = nullptr;
    ElaPushButton *m_cancelAllButton = nullptr;
    ElaPushButton *m_retryAllButton = nullptr;
    ElaPushButton *m_clearButton = nullptr;

    ElaText *m_concurrencyLabel = nullptr;
    ElaSpinBox *m_concurrencySpin = nullptr;
    ElaText *m_postActionLabel = nullptr;
    ElaComboBox *m_postActionCombo = nullptr;

    ElaText *m_emptyLabel = nullptr;
    QWidget *m_rowsContainer = nullptr;
    QVBoxLayout *m_rowsLayout = nullptr;

    QMap<QUuid, RowWidgets> m_rows;
    QList<QUuid> m_pendingRemoval;
};
