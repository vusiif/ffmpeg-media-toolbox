#pragma once

#include <QStringList>
#include <QWidget>

class FFmpegLocator;
class FFmpegCommandBuilder;
class JobQueue;
class ElaListView;
class QStringListModel;
class ElaText;
class ElaPushButton;
class ElaComboBox;
class ElaSpinBox;

class JoinPage : public QWidget
{
    Q_OBJECT

public:
    explicit JoinPage(FFmpegLocator *locator, FFmpegCommandBuilder *builder,
                       JobQueue *queue, QWidget *parent = nullptr);

    void retranslateUi();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void addFiles();
    void removeSelected();
    void syncFileListModel();
    void addToQueue();
    void updateModeVisibility();

    FFmpegLocator *m_locator;
    FFmpegCommandBuilder *m_builder;
    JobQueue *m_queue;

    QStringList m_inputFiles;

    ElaText *m_titleLabel = nullptr;
    ElaText *m_listHeader = nullptr;
    ElaListView *m_fileView = nullptr;
    QStringListModel *m_fileModel = nullptr;
    ElaPushButton *m_addButton = nullptr;
    ElaPushButton *m_removeButton = nullptr;
    ElaText *m_modeCaption = nullptr;
    ElaComboBox *m_modeCombo = nullptr;
    ElaText *m_colsCaption = nullptr;
    ElaSpinBox *m_colsSpin = nullptr;
    ElaText *m_spacingCaption = nullptr;
    ElaSpinBox *m_spacingSpin = nullptr;
    ElaText *m_paddingCaption = nullptr;
    ElaSpinBox *m_paddingSpin = nullptr;
    ElaText *m_backgroundCaption = nullptr;
    ElaComboBox *m_backgroundCombo = nullptr;
    ElaPushButton *m_queueButton = nullptr;
};
