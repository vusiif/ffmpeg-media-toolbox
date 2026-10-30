#include "UntruncLocator.h"

#include "app/AppSettings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QtConcurrent>

namespace {
bool isValidUntrunc(const QString &path)
{
    if (path.isEmpty()) return false;
    const QFileInfo info(path);
    return info.exists() && info.isFile() && info.isExecutable();
}
} // namespace

UntruncLocator::UntruncLocator(QObject *parent)
    : QObject(parent)
{
    scanAsync();
}

void UntruncLocator::scanAsync()
{
    QPointer<UntruncLocator> self = this;
    (void)QtConcurrent::run([self]() {
        if (!self) return;
        const QString found = findUntruncExecutable();
        QMetaObject::invokeMethod(self, [self, found]() {
            if (!self) return;
            const bool newAvailable = !found.isEmpty();
            const bool changed = (newAvailable != self->m_available) || (found != self->m_untruncPath);

            self->m_untruncPath = found;
            self->m_available = newAvailable;

            if (changed) {
                emit self->availabilityChanged(self->m_available);
                emit self->pathChanged(self->m_untruncPath);
            }
        });
    });
}

bool UntruncLocator::waitForScan(int timeoutMs)
{
    if (m_available) {
        return true;
    }

    QEventLoop loop;
    connect(this, &UntruncLocator::availabilityChanged, &loop, &QEventLoop::quit);

    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);

    loop.exec();
    return m_available;
}

QString UntruncLocator::findUntruncExecutable()
{
    QString found;

    // 1. User specified in settings
    QSettings settings(AppSettings::organization(), AppSettings::application());
    const QString custom = settings.value(QStringLiteral("tools/untruncPath")).toString();
    if (isValidUntrunc(custom)) {
        found = QDir::toNativeSeparators(custom);
    }

    const QString appDir = QCoreApplication::applicationDirPath();

    // 2. Application runtime directory subfolders
    if (found.isEmpty()) {
        const QStringList candidates = {
            appDir + QStringLiteral("/untrunc_x64/untrunc.exe"),
            appDir + QStringLiteral("/untrunc.exe"),
            appDir + QStringLiteral("/untrunc/untrunc.exe"),
            // Dev tree relative to build directory
            appDir + QStringLiteral("/../Others/untrunc/untrunc_x64/untrunc.exe"),
            QDir::currentPath() + QStringLiteral("/Others/untrunc/untrunc_x64/untrunc.exe")
        };
        for (const QString &c : candidates) {
            if (isValidUntrunc(c)) {
                found = QDir::toNativeSeparators(QFileInfo(c).canonicalFilePath());
                break;
            }
        }
    }

    // 3. System PATH
    if (found.isEmpty()) {
        const QString pathExe = QStandardPaths::findExecutable(QStringLiteral("untrunc"));
        if (isValidUntrunc(pathExe)) {
            found = QDir::toNativeSeparators(pathExe);
        }
    }

    return found;
}

bool UntruncLocator::scan()
{
    const QString found = findUntruncExecutable();
    const bool newAvailable = !found.isEmpty();
    const bool changed = (newAvailable != m_available) || (found != m_untruncPath);

    m_untruncPath = found;
    m_available = newAvailable;

    if (changed) {
        emit availabilityChanged(m_available);
        emit pathChanged(m_untruncPath);
    }

    return m_available;
}

bool UntruncLocator::isAvailable() const
{
    return m_available;
}

QString UntruncLocator::untruncPath() const
{
    return m_untruncPath;
}

void UntruncLocator::setCustomPath(const QString &path)
{
    QSettings settings(AppSettings::organization(), AppSettings::application());
    settings.setValue(QStringLiteral("tools/untruncPath"), path);
    scan();
}
