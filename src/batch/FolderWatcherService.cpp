#include "FolderWatcherService.h"
#include "BatchExportMatrix.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>

FolderWatcherService::FolderWatcherService(QObject *parent)
    : QObject(parent)
    , m_nameFilters(BatchExportMatrix::defaultMediaFilters())
    , m_fsWatcher(new QFileSystemWatcher(this))
    , m_stabilityTimer(new QTimer(this))
{
    connect(m_fsWatcher, &QFileSystemWatcher::directoryChanged,
            this, &FolderWatcherService::onDirectoryChanged);

    m_stabilityTimer->setInterval(m_checkIntervalMs);
    connect(m_stabilityTimer, &QTimer::timeout,
            this, &FolderWatcherService::onStabilityCheckTimer);
}

FolderWatcherService::~FolderWatcherService()
{
    stopWatching();
}

QString FolderWatcherService::watchPath() const { return m_watchPath; }
QString FolderWatcherService::outputPath() const { return m_outputPath; }
bool FolderWatcherService::isWatching() const { return m_isWatching; }

bool FolderWatcherService::isRecursive() const { return m_recursive; }
void FolderWatcherService::setRecursive(bool recursive) { m_recursive = recursive; }

bool FolderWatcherService::preserveSubDir() const { return m_preserveSubDir; }
void FolderWatcherService::setPreserveSubDir(bool preserve) { m_preserveSubDir = preserve; }

QStringList FolderWatcherService::nameFilters() const { return m_nameFilters; }
void FolderWatcherService::setNameFilters(const QStringList &filters)
{
    m_nameFilters = filters.isEmpty() ? BatchExportMatrix::defaultMediaFilters() : filters;
}

int FolderWatcherService::checkIntervalMs() const { return m_checkIntervalMs; }
void FolderWatcherService::setCheckIntervalMs(int ms)
{
    if (ms < 500) ms = 500;
    m_checkIntervalMs = ms;
    if (m_stabilityTimer) {
        m_stabilityTimer->setInterval(m_checkIntervalMs);
    }
}

int FolderWatcherService::processedCount() const { return m_processedFiles.size(); }
int FolderWatcherService::pendingCount() const { return m_pendingFiles.size(); }

bool FolderWatcherService::startWatching(const QString &watchPath, const QString &outputPath)
{
    stopWatching();

    QDir watchDir(watchPath);
    if (!watchDir.exists()) {
        emit statusMessage(QStringLiteral("Watch directory does not exist: %1").arg(watchPath));
        return false;
    }

    m_watchPath = QDir::cleanPath(watchPath);
    m_outputPath = QDir::cleanPath(outputPath);

    // Add root path to watcher
    m_fsWatcher->addPath(m_watchPath);

    // If recursive, also watch existing subdirectories
    if (m_recursive) {
        QDirIterator it(m_watchPath, QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            m_fsWatcher->addPath(it.next());
        }
    }

    m_isWatching = true;
    m_stabilityTimer->start();

    emit watchingChanged(true);
    emit statusMessage(QStringLiteral("Started watching folder: %1").arg(m_watchPath));

    // Initial scan to detect existing files
    scanDirectoryNow();
    return true;
}

void FolderWatcherService::stopWatching()
{
    if (!m_isWatching) {
        return;
    }

    const QStringList paths = m_fsWatcher->directories();
    if (!paths.isEmpty()) {
        m_fsWatcher->removePaths(paths);
    }

    m_stabilityTimer->stop();
    m_isWatching = false;

    emit watchingChanged(false);
    emit statusMessage(QStringLiteral("Stopped watching folder."));
}

void FolderWatcherService::resetProcessedFiles()
{
    m_processedFiles.clear();
    m_pendingFiles.clear();
    emit statusMessage(QStringLiteral("Reset processed file history."));
}

void FolderWatcherService::scanDirectoryNow()
{
    if (m_watchPath.isEmpty()) return;
    checkDirectory(m_watchPath);
}

void FolderWatcherService::onDirectoryChanged(const QString &path)
{
    checkDirectory(path);
}

void FolderWatcherService::checkDirectory(const QString &path)
{
    Q_UNUSED(path)
    if (m_watchPath.isEmpty()) return;

    const auto scanned = BatchExportMatrix::scanDirectory(m_watchPath, m_recursive, m_nameFilters);
    for (const auto &item : scanned) {
        const QString fullPath = item.fullPath;
        if (m_processedFiles.contains(fullPath)) {
            continue;
        }

        // Ignore temporary/incomplete download files
        if (fullPath.endsWith(QLatin1String(".tmp"), Qt::CaseInsensitive) ||
            fullPath.endsWith(QLatin1String(".part"), Qt::CaseInsensitive) ||
            fullPath.endsWith(QLatin1String(".crdownload"), Qt::CaseInsensitive)) {
            continue;
        }

        if (!m_pendingFiles.contains(fullPath)) {
            PendingFileState state;
            state.lastSize = -1;
            state.stableCount = 0;
            state.relativeSubDir = m_preserveSubDir ? item.relativeSubDir : QString();
            m_pendingFiles.insert(fullPath, state);

            emit fileDetected(fullPath, state.relativeSubDir);
            emit statusMessage(QStringLiteral("New media file detected: %1").arg(item.fileName));
        }
    }
}

void FolderWatcherService::onStabilityCheckTimer()
{
    if (m_pendingFiles.isEmpty()) {
        // Also perform periodic directory check
        scanDirectoryNow();
        return;
    }

    QList<QString> readyKeys;
    QList<QString> removedKeys;

    for (auto it = m_pendingFiles.begin(); it != m_pendingFiles.end(); ++it) {
        const QString &filePath = it.key();
        PendingFileState &state = it.value();

        const QFileInfo fi(filePath);
        if (!fi.exists()) {
            removedKeys.append(filePath);
            continue;
        }

        const qint64 currentSize = fi.size();
        if (currentSize > 0 && currentSize == state.lastSize) {
            state.stableCount++;
            if (state.stableCount >= 2) {
                // Verify read access
                QFile f(filePath);
                if (f.open(QIODevice::ReadOnly)) {
                    f.close();
                    readyKeys.append(filePath);
                }
            }
        } else {
            state.lastSize = currentSize;
            state.stableCount = 0;
        }
    }

    for (const QString &k : removedKeys) {
        m_pendingFiles.remove(k);
    }

    for (const QString &k : readyKeys) {
        PendingFileState state = m_pendingFiles.take(k);
        m_processedFiles.insert(k);

        emit fileReady(k, state.relativeSubDir);
        emit statusMessage(QStringLiteral("File stabilized and ready for queue: %1")
                               .arg(QFileInfo(k).fileName()));
    }
}
