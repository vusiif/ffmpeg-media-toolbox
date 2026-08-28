#include "FFmpegProcess.h"

#include <QCoreApplication>

FFmpegProcess::FFmpegProcess(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_killTimer(new QTimer(this))
{
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &FFmpegProcess::onReadyReadStdout);
    connect(m_process, &QProcess::readyReadStandardError, this, &FFmpegProcess::onReadyReadStderr);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &FFmpegProcess::onFinished);
    connect(m_process, &QProcess::errorOccurred, this, &FFmpegProcess::onErrorOccurred);

    m_killTimer->setSingleShot(true);
    m_killTimer->setInterval(3000);
    connect(m_killTimer, &QTimer::timeout, this, [this]() {
        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
        }
    });
}

FFmpegProcess::~FFmpegProcess()
{
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(2000);
    }
}

bool FFmpegProcess::start(const QString &program, const QStringList &arguments, double totalDuration)
{
    if (m_process->state() != QProcess::NotRunning) {
        return false;
    }

    m_totalDuration = totalDuration;
    m_progress = 0;
    m_stderrOutput.clear();
    m_progressBuffer.clear();
    m_currentProgress = FFmpegProgress();

    QStringList args = arguments;
    args.prepend(QStringLiteral("-progress"));
    args.prepend(QStringLiteral("pipe:1"));
    args.prepend(QStringLiteral("-nostats"));

    setState(State::Running);
    m_process->start(program, args);

    if (!m_process->waitForStarted(5000)) {
        setState(State::Failed);
        emit finished(false);
        return false;
    }

    return true;
}

void FFmpegProcess::cancel()
{
    if (m_process->state() == QProcess::NotRunning) {
        return;
    }

    m_process->write("q");
    m_process->closeWriteChannel();
    m_killTimer->start();
}

void FFmpegProcess::kill()
{
    if (m_process->state() == QProcess::NotRunning) {
        return;
    }

    m_process->kill();
}

bool FFmpegProcess::isRunning() const { return m_state == State::Running; }
double FFmpegProcess::progress() const { return m_progress; }
FFmpegProcess::State FFmpegProcess::state() const { return m_state; }
QString FFmpegProcess::lastError() const { return m_stderrOutput; }
QString FFmpegProcess::stderrOutput() const { return m_stderrOutput; }
int FFmpegProcess::exitCode() const { return m_process->exitCode(); }
FFmpegProgress FFmpegProcess::currentProgress() const { return m_currentProgress; }

void FFmpegProcess::onReadyReadStdout()
{
    m_progressBuffer.append(QString::fromUtf8(m_process->readAllStandardOutput()));

    int newlineIdx;
    while ((newlineIdx = m_progressBuffer.indexOf(QLatin1Char('\n'))) >= 0) {
        QString line = m_progressBuffer.left(newlineIdx).trimmed();
        m_progressBuffer = m_progressBuffer.mid(newlineIdx + 1);

        if (!line.isEmpty()) {
            parseProgressLine(line);
        }
    }
}

void FFmpegProcess::onReadyReadStderr()
{
    m_stderrOutput.append(QString::fromUtf8(m_process->readAllStandardError()));
}

void FFmpegProcess::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_killTimer->stop();

    if (m_state == State::Cancelled) {
        emit finished(false);
        return;
    }

    if (exitStatus == QProcess::CrashExit || exitCode != 0) {
        setState(State::Failed);
        emit finished(false);
    } else {
        m_progress = 1.0;
        emit progressChanged();
        setState(State::Finished);
        emit finished(true);
    }
}

void FFmpegProcess::onErrorOccurred(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart) {
        setState(State::Failed);
        emit finished(false);
    }
}

void FFmpegProcess::parseProgressLine(const QString &line)
{
    int eqIdx = line.indexOf(QLatin1Char('='));
    if (eqIdx < 0) {
        return;
    }

    QString key = line.left(eqIdx).trimmed();
    QString value = line.mid(eqIdx + 1).trimmed();

    if (key == QStringLiteral("frame")) {
        m_currentProgress.frame = value.toLongLong();
    } else if (key == QStringLiteral("fps")) {
        m_currentProgress.fps = value.toDouble();
    } else if (key == QStringLiteral("out_time_us")) {
        m_currentProgress.outTimeUs = value.toLongLong();

        if (m_totalDuration > 0) {
            double currentSec = m_currentProgress.outTimeUs / 1000000.0;
            m_progress = qBound(0.0, currentSec / m_totalDuration, 1.0);
            m_currentProgress.percent = m_progress * 100.0;
            emit progressChanged();
        }
    } else if (key == QStringLiteral("speed")) {
        QString speedStr = value;
        if (speedStr.endsWith(QLatin1Char('x'))) {
            speedStr.chop(1);
        }
        m_currentProgress.speed = speedStr.toDouble();
    } else if (key == QStringLiteral("progress")) {
        if (value == QStringLiteral("end")) {
            m_progress = 1.0;
            emit progressChanged();
        }
    }

    emit logMessage(line);
}

void FFmpegProcess::setState(State state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    emit stateChanged();
}
