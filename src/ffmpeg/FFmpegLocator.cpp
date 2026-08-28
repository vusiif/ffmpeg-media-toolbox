#include "FFmpegLocator.h"

#include <QProcess>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>

static constexpr auto kSettingsGroup = "FFmpegLocator";
static constexpr auto kKeyFfmpeg = "ffmpegPath";
static constexpr auto kKeyFfprobe = "ffprobePath";
static constexpr auto kKeyCustom = "customPaths";

FFmpegLocator::FFmpegLocator(QObject *parent)
    : QObject(parent)
{
    loadSavedPaths();
    if (m_ffmpegPath.isEmpty() || m_ffprobePath.isEmpty()) {
        autoDetect();
    }
}

QString FFmpegLocator::ffmpegPath() const { return m_ffmpegPath; }
QString FFmpegLocator::ffprobePath() const { return m_ffprobePath; }
QString FFmpegLocator::ffmpegVersion() const { return m_ffmpegVersion; }
bool FFmpegLocator::isValid() const { return !m_ffmpegPath.isEmpty() && !m_ffprobePath.isEmpty(); }

bool FFmpegLocator::autoDetect()
{
    QString oldFfmpeg = m_ffmpegPath;
    QString oldFfprobe = m_ffprobePath;
    QString oldVersion = m_ffmpegVersion;

    m_ffmpegPath.clear();
    m_ffprobePath.clear();
    m_ffmpegVersion.clear();
    m_customPaths = false;

    auto ffmpegResult = probe(findInPath(QStringLiteral("ffmpeg")));
    if (ffmpegResult.path.isEmpty()) {
        ffmpegResult = probe(findCommonLocations(QStringLiteral("ffmpeg")));
    }

    auto ffprobeResult = probe(findInPath(QStringLiteral("ffprobe")));
    if (ffprobeResult.path.isEmpty()) {
        ffprobeResult = probe(findCommonLocations(QStringLiteral("ffprobe")));
    }

    if (!ffmpegResult.path.isEmpty() && !ffprobeResult.path.isEmpty()) {
        m_ffmpegPath = ffmpegResult.path;
        m_ffprobePath = ffprobeResult.path;
        m_ffmpegVersion = ffmpegResult.version;
        savePaths();
    }

    if (m_ffmpegPath != oldFfmpeg || m_ffprobePath != oldFfprobe || m_ffmpegVersion != oldVersion) {
        emit pathsChanged();
    }

    return isValid();
}

bool FFmpegLocator::setCustomPaths(const QString &ffmpeg, const QString &ffprobe)
{
    auto ffmpegResult = probe(ffmpeg);
    auto ffprobeResult = probe(ffprobe);

    if (ffmpegResult.path.isEmpty() || ffprobeResult.path.isEmpty()) {
        return false;
    }

    m_ffmpegPath = ffmpegResult.path;
    m_ffprobePath = ffprobeResult.path;
    m_ffmpegVersion = ffmpegResult.version;
    m_customPaths = true;
    savePaths();
    emit pathsChanged();
    return true;
}

void FFmpegLocator::resetToAutoDetect()
{
    m_customPaths = false;
    autoDetect();
}

FFmpegLocator::ProbeResult FFmpegLocator::probe(const QString &candidate) const
{
    if (candidate.isEmpty()) {
        return {};
    }

    QFileInfo fi(candidate);
    if (!fi.exists() || !fi.isExecutable()) {
        return {};
    }

    QProcess proc;
    proc.start(candidate, {QStringLiteral("-version")});
    if (!proc.waitForFinished(5000)) {
        proc.kill();
        return {};
    }

    if (proc.exitCode() != 0) {
        return {};
    }

    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    QString version;
    auto lines = output.split(QLatin1Char('\n'));
    if (!lines.isEmpty()) {
        version = lines.first().trimmed();
    }

    return {fi.absoluteFilePath(), version};
}

QString FFmpegLocator::findInPath(const QString &executable) const
{
    QString found = QStandardPaths::findExecutable(executable);
    return found;
}

QString FFmpegLocator::findCommonLocations(const QString &executable) const
{
    QString exeName = executable;
#ifdef Q_OS_WIN
    exeName += QStringLiteral(".exe");
#endif

    QStringList candidates;

    QString appDir = QCoreApplication::applicationDirPath();
    candidates << QDir(appDir).absoluteFilePath(exeName);
    candidates << QDir(appDir).absoluteFilePath(QStringLiteral("ffmpeg/") + exeName);
    candidates << QDir(appDir).absoluteFilePath(QStringLiteral("tools/") + exeName);

#ifdef Q_OS_WIN
    QStringList envPaths = qEnvironmentVariable("PATH").split(QLatin1Char(';'));
    for (const QString &path : envPaths) {
        QString trimmed = path.trimmed();
        if (!trimmed.isEmpty()) {
            candidates << QDir(trimmed).absoluteFilePath(exeName);
        }
    }

    QString programFiles = qEnvironmentVariable("ProgramFiles");
    if (!programFiles.isEmpty()) {
        candidates << QDir(programFiles).absoluteFilePath(QStringLiteral("ffmpeg/bin/") + exeName);
        candidates << QDir(programFiles).absoluteFilePath(QStringLiteral("ffmpeg/") + exeName);
    }

    QString localAppData = qEnvironmentVariable("LOCALAPPDATA");
    if (!localAppData.isEmpty()) {
        candidates << QDir(localAppData).absoluteFilePath(QStringLiteral("ffmpeg/") + exeName);
        candidates << QDir(localAppData).absoluteFilePath(QStringLiteral("Microsoft/WinGet/Packages/ffmpeg/") + exeName);
    }

    QString scoop = qEnvironmentVariable("USERPROFILE");
    if (!scoop.isEmpty()) {
        candidates << QDir(scoop).absoluteFilePath(QStringLiteral("scoop/shims/") + exeName);
        candidates << QDir(scoop).absoluteFilePath(QStringLiteral("scoop/apps/ffmpeg/current/bin/") + exeName);
    }
#else
    candidates << QStringLiteral("/usr/bin/") + exeName;
    candidates << QStringLiteral("/usr/local/bin/") + exeName;
    candidates << QDir::homePath() + QStringLiteral("/bin/") + exeName;

    QString homebrew = QStringLiteral("/opt/homebrew/bin/") + exeName;
    candidates << homebrew;
#endif

    for (const QString &candidate : std::as_const(candidates)) {
        QFileInfo fi(candidate);
        if (fi.exists() && fi.isExecutable()) {
            return fi.absoluteFilePath();
        }
    }

    return {};
}

void FFmpegLocator::loadSavedPaths()
{
    QSettings settings;
    settings.beginGroup(kSettingsGroup);

    m_customPaths = settings.value(kKeyCustom, false).toBool();

    if (m_customPaths) {
        QString ffmpeg = settings.value(kKeyFfmpeg).toString();
        QString ffprobe = settings.value(kKeyFfprobe).toString();
        if (!ffmpeg.isEmpty() && !ffprobe.isEmpty()) {
            auto ffmpegResult = probe(ffmpeg);
            auto ffprobeResult = probe(ffprobe);
            if (!ffmpegResult.path.isEmpty() && !ffprobeResult.path.isEmpty()) {
                m_ffmpegPath = ffmpegResult.path;
                m_ffprobePath = ffprobeResult.path;
                m_ffmpegVersion = ffmpegResult.version;
            }
        }
    }

    settings.endGroup();
}

void FFmpegLocator::savePaths() const
{
    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    settings.setValue(kKeyCustom, m_customPaths);
    settings.setValue(kKeyFfmpeg, m_ffmpegPath);
    settings.setValue(kKeyFfprobe, m_ffprobePath);
    settings.endGroup();
}
