#include "MediaSalvageEngine.h"

#include "ffmpeg/FFmpegLocator.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QtMath>

MediaSalvageEngine::MediaSalvageEngine(const FFmpegLocator *locator, QObject *parent)
    : QObject(parent)
    , m_locator(locator)
    , m_process(new QProcess(this))
{
    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &MediaSalvageEngine::onReadyReadStdout);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &MediaSalvageEngine::onReadyReadStderr);
    connect(m_process, &QProcess::finished,
            this, &MediaSalvageEngine::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &MediaSalvageEngine::onProcessError);
}

MediaSalvageEngine::~MediaSalvageEngine()
{
    if (m_process) {
        disconnect(m_process, nullptr, this, nullptr);
        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
            m_process->waitForFinished(1000);
        }
    }
}

bool MediaSalvageEngine::start(const QString &inputPath,
                               const QString &outputPath,
                               SalvageMode mode,
                               double totalDuration)
{
    if (m_process->state() != QProcess::NotRunning) {
        return false;
    }

    if (!m_locator || m_locator->ffmpegPath().isEmpty()) {
        m_lastError = QStringLiteral("FFmpeg executable not found.");
        setState(State::Failed);
        emit finished(false, QString(), m_lastError);
        return false;
    }

    const QFileInfo inInfo(inputPath);
    if (!inInfo.exists() || !inInfo.isFile()) {
        m_lastError = QStringLiteral("Input file does not exist: %1").arg(inputPath);
        setState(State::Failed);
        emit finished(false, QString(), m_lastError);
        return false;
    }

    const QFileInfo outInfo(outputPath);
    QDir().mkpath(outInfo.absolutePath());

    m_inputPath = inputPath;
    m_outputPath = outputPath;
    m_mode = mode;
    m_totalDuration = totalDuration;
    m_progress = 0.0;
    m_lastError.clear();
    m_stdoutBuffer.clear();
    m_stderrBuffer.clear();

    const QStringList args = buildArguments(inputPath, outputPath, mode);

    setState(State::Running);
    emit progressChanged(0.0);
    emit statusMessage(mode == SalvageMode::FastRemux
                           ? QStringLiteral("Starting fast lossless salvage...")
                           : QStringLiteral("Starting force transcode salvage..."));

    m_process->start(m_locator->ffmpegPath(), args);
    if (!m_process->waitForStarted(5000)) {
        m_lastError = QStringLiteral("Failed to start FFmpeg process.");
        setState(State::Failed);
        emit finished(false, QString(), m_lastError);
        return false;
    }

    return true;
}

void MediaSalvageEngine::cancel()
{
    if (m_process->state() == QProcess::NotRunning) {
        return;
    }

    setState(State::Cancelled);
    emit statusMessage(QStringLiteral("Salvage operation cancelled by user."));
    m_process->kill();
}

bool MediaSalvageEngine::isRunning() const
{
    return m_state == State::Running;
}

MediaSalvageEngine::State MediaSalvageEngine::state() const
{
    return m_state;
}

MediaSalvageEngine::SalvageMode MediaSalvageEngine::mode() const
{
    return m_mode;
}

QString MediaSalvageEngine::inputPath() const
{
    return m_inputPath;
}

QString MediaSalvageEngine::outputPath() const
{
    return m_outputPath;
}

QString MediaSalvageEngine::lastError() const
{
    return m_lastError;
}

double MediaSalvageEngine::progress() const
{
    return m_progress;
}

QStringList MediaSalvageEngine::buildArguments(const QString &inputPath,
                                              const QString &outputPath,
                                              SalvageMode mode)
{
    QStringList args;
    args << QStringLiteral("-nostats")
         << QStringLiteral("-progress") << QStringLiteral("pipe:1")
         << QStringLiteral("-y")
         << QStringLiteral("-err_detect") << QStringLiteral("ignore_err")
         << QStringLiteral("-fflags") << QStringLiteral("+genpts+discardcorrupt")
         << QStringLiteral("-i") << inputPath
         << QStringLiteral("-map") << QStringLiteral("0:v?")
         << QStringLiteral("-map") << QStringLiteral("0:a?");

    if (mode == SalvageMode::FastRemux) {
        args << QStringLiteral("-c") << QStringLiteral("copy")
             << QStringLiteral("-movflags") << QStringLiteral("+faststart")
             << outputPath;
    } else {
        args << QStringLiteral("-c:v") << QStringLiteral("libx264")
             << QStringLiteral("-preset") << QStringLiteral("fast")
             << QStringLiteral("-crf") << QStringLiteral("23")
             << QStringLiteral("-c:a") << QStringLiteral("aac")
             << QStringLiteral("-b:a") << QStringLiteral("192k")
             << QStringLiteral("-movflags") << QStringLiteral("+faststart")
             << outputPath;
    }

    return args;
}

QString MediaSalvageEngine::suggestOutputPath(const QString &inputPath, SalvageMode mode)
{
    if (inputPath.isEmpty()) return QString();
    const QFileInfo fi(inputPath);
    const QString dir = fi.absolutePath();
    const QString base = fi.completeBaseName();
    const QString suffix = (mode == SalvageMode::ForceTranscode) ? QStringLiteral("mp4") : fi.suffix();
    const QString tag = (mode == SalvageMode::ForceTranscode)
                            ? QStringLiteral("_salvaged.")
                            : QStringLiteral("_repaired.");
    return dir + QStringLiteral("/") + base + tag + suffix;
}

void MediaSalvageEngine::setState(State newState)
{
    if (m_state != newState) {
        m_state = newState;
        emit stateChanged(newState);
    }
}

void MediaSalvageEngine::onReadyReadStdout()
{
    m_stdoutBuffer.append(QString::fromUtf8(m_process->readAllStandardOutput()));
    int newlineIdx = -1;
    while ((newlineIdx = m_stdoutBuffer.indexOf(QLatin1Char('\n'))) != -1) {
        const QString line = m_stdoutBuffer.left(newlineIdx).trimmed();
        m_stdoutBuffer.remove(0, newlineIdx + 1);
        if (!line.isEmpty()) {
            parseProgressLine(line);
        }
    }
}

void MediaSalvageEngine::onReadyReadStderr()
{
    m_stderrBuffer.append(QString::fromUtf8(m_process->readAllStandardError()));
    int newlineIdx = -1;
    while ((newlineIdx = m_stderrBuffer.indexOf(QLatin1Char('\n'))) != -1) {
        const QString line = m_stderrBuffer.left(newlineIdx).trimmed();
        m_stderrBuffer.remove(0, newlineIdx + 1);
        if (!line.isEmpty()) {
            emit logMessage(line);
            if (line.contains(QStringLiteral("error"), Qt::CaseInsensitive)
                || line.contains(QStringLiteral("invalid"), Qt::CaseInsensitive)
                || line.contains(QStringLiteral("fatal"), Qt::CaseInsensitive)) {
                m_lastError = line;
            }
        }
    }
}

void MediaSalvageEngine::parseProgressLine(const QString &line)
{
    if (line.startsWith(QStringLiteral("out_time_us="))) {
        const qint64 us = line.mid(12).trimmed().toLongLong();
        const double sec = static_cast<double>(us) / 1000000.0;
        if (m_totalDuration > 0.0) {
            m_progress = qBound(0.0, sec / m_totalDuration, 0.99);
            emit progressChanged(m_progress);
        }
    } else if (line.startsWith(QStringLiteral("progress=end"))) {
        m_progress = 1.0;
        emit progressChanged(1.0);
    }
}

void MediaSalvageEngine::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);
    const QFileInfo outInfo(m_outputPath);
    const bool outOk = outInfo.exists() && outInfo.size() > 0;

    if (m_state == State::Cancelled) {
        emit finished(false, m_outputPath, QStringLiteral("Operation cancelled."));
        return;
    }

    if (exitCode == 0 && outOk) {
        m_progress = 1.0;
        emit progressChanged(1.0);
        setState(State::Success);
        emit statusMessage(QStringLiteral("Salvage finished successfully."));
        emit finished(true, m_outputPath, QString());
    } else if (outOk) {
        // In salvage scenarios, FFmpeg might return non-zero due to end-of-file truncation or dropped bad packets,
        // but still successfully salvage valid video frames to output!
        m_progress = 1.0;
        emit progressChanged(1.0);
        setState(State::Success);
        emit statusMessage(QStringLiteral("Salvage completed with warnings (playable file generated)."));
        emit finished(true, m_outputPath, QString());
    } else {
        setState(State::Failed);
        if (m_lastError.isEmpty()) {
            m_lastError = QStringLiteral("FFmpeg exited with code %1 without generating output.").arg(exitCode);
        }
        emit statusMessage(QStringLiteral("Salvage failed: %1").arg(m_lastError));
        emit finished(false, m_outputPath, m_lastError);
    }
}

void MediaSalvageEngine::onProcessError(QProcess::ProcessError error)
{
    if (m_state == State::Cancelled) return;
    if (error == QProcess::FailedToStart) {
        m_lastError = QStringLiteral("FFmpeg process failed to start.");
        setState(State::Failed);
        emit finished(false, m_outputPath, m_lastError);
    }
}
