#include "RecentFiles.h"

#include <QSettings>

namespace {
const QString kOrg = QStringLiteral("FFMedia");
const QString kApp = QStringLiteral("FFmpeg Media Toolbox");
const QString kKeyFiles = QStringLiteral("recent/files");
} // namespace

RecentFiles::RecentFiles(QObject *parent)
    : QObject(parent)
{
    QSettings settings(kOrg, kApp);
    m_files = settings.value(kKeyFiles).toStringList();
}

QStringList RecentFiles::files() const
{
    return m_files;
}

void RecentFiles::add(const QString &path)
{
    const QStringList updated = pushFront(m_files, path, kMaxCount);
    if (updated == m_files) {
        return;
    }
    m_files = updated;
    save();
    emit changed();
}

void RecentFiles::clear()
{
    if (m_files.isEmpty()) {
        return;
    }
    m_files.clear();
    save();
    emit changed();
}

QStringList RecentFiles::pushFront(const QStringList &current, const QString &path,
                                   int maxCount)
{
    if (path.isEmpty() || maxCount <= 0) {
        return current;
    }

    QStringList result = current;
    result.removeAll(path);
    result.prepend(path);
    while (result.size() > maxCount) {
        result.removeLast();
    }
    return result;
}

void RecentFiles::save()
{
    QSettings settings(kOrg, kApp);
    settings.setValue(kKeyFiles, m_files);
}
