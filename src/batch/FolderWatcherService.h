#pragma once

#include <QFileSystemWatcher>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimer>

class FolderWatcherService : public QObject
{
    Q_OBJECT

public:
    explicit FolderWatcherService(QObject *parent = nullptr);
    ~FolderWatcherService() override;

    QString watchPath() const;
    QString outputPath() const;
    bool isWatching() const;
    bool isRecursive() const;
    void setRecursive(bool recursive);
    bool preserveSubDir() const;
    void setPreserveSubDir(bool preserve);

    QStringList nameFilters() const;
    void setNameFilters(const QStringList &filters);

    int checkIntervalMs() const;
    void setCheckIntervalMs(int ms);

    int processedCount() const;
    int pendingCount() const;

    bool startWatching(const QString &watchPath, const QString &outputPath);
    void stopWatching();
    void scanDirectoryNow();
    void resetProcessedFiles();

signals:
    void watchingChanged(bool active);
    void fileDetected(const QString &filePath, const QString &relativeSubDir);
    void fileReady(const QString &filePath, const QString &relativeSubDir);
    void statusMessage(const QString &message);

private slots:
    void onDirectoryChanged(const QString &path);
    void onStabilityCheckTimer();

private:
    void checkDirectory(const QString &path);

    QString m_watchPath;
    QString m_outputPath;
    bool m_isWatching = false;
    bool m_recursive = true;
    bool m_preserveSubDir = true;
    int m_checkIntervalMs = 2000;
    QStringList m_nameFilters;

    QFileSystemWatcher *m_fsWatcher = nullptr;
    QTimer *m_stabilityTimer = nullptr;

    struct PendingFileState {
        qint64 lastSize = -1;
        int stableCount = 0;
        QString relativeSubDir;
    };

    QMap<QString, PendingFileState> m_pendingFiles;
    QSet<QString> m_processedFiles;
};
