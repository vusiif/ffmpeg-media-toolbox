#pragma once

#include <QMap>
#include <QStringList>

#include "ElaScrollPageArea.h"

class FolderWatcherService;
class ElaListView;
class QStringListModel;
class ElaText;
class ElaPushButton;
class ElaCheckBox;
class ElaLineEdit;

class BatchFileListCard : public ElaScrollPageArea
{
    Q_OBJECT

public:
    explicit BatchFileListCard(QWidget *parent = nullptr);
    ~BatchFileListCard() override;

    QStringList inputFiles() const { return m_inputFiles; }
    QString outputDirectory() const { return m_outputDirectory; }
    bool preserveTree() const;
    QMap<QString, QString> relativeSubDirs() const { return m_relativeSubDirs; }

    void addFiles(const QStringList &files);
    void addFolder(const QString &folder);
    void retranslateUi();

signals:
    void fileListChanged();
    void fileReadyToProcess(const QString &filePath, const QString &relativeSubDir);

private slots:
    void onAddFilesClicked();
    void onAddFolderClicked();
    void onRemoveSelectedClicked();
    void onClearFilesClicked();
    void onBrowseOutputDir();
    void onToggleFolderWatcher();
    void onWatcherFileReady(const QString &filePath, const QString &relativeSubDir);

private:
    void syncFileListModel();
    void updateCountLabel();

    QStringList m_inputFiles;
    QString m_outputDirectory;
    QMap<QString, QString> m_relativeSubDirs;

    FolderWatcherService *m_watcherService = nullptr;

    ElaText *m_listHeader = nullptr;
    ElaListView *m_fileView = nullptr;
    QStringListModel *m_fileModel = nullptr;
    ElaPushButton *m_addButton = nullptr;
    ElaPushButton *m_addFolderButton = nullptr;
    ElaPushButton *m_removeButton = nullptr;
    ElaPushButton *m_clearButton = nullptr;
    ElaCheckBox *m_preserveTreeCheck = nullptr;
    ElaText *m_countLabel = nullptr;

    ElaText *m_outputDirCaption = nullptr;
    ElaPushButton *m_outputDirButton = nullptr;

    ElaCheckBox *m_watcherToggle = nullptr;
    QWidget *m_watcherContainer = nullptr;
    ElaLineEdit *m_watchPathEdit = nullptr;
    ElaPushButton *m_watchBrowseBtn = nullptr;
    ElaPushButton *m_watchActionBtn = nullptr;
    ElaText *m_watchStatusLabel = nullptr;
};
