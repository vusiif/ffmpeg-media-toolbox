#include "UntruncProcess.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

UntruncProcess::UntruncProcess(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
{
    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &UntruncProcess::onReadyReadStdout);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &UntruncProcess::onReadyReadStderr);
    connect(m_process, &QProcess::finished,
            this, &UntruncProcess::onFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &UntruncProcess::onErrorOccurred);
}

UntruncProcess::~UntruncProcess()
{
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }
}

bool UntruncProcess::start(const QString &program, const QString &referenceFile, const QString &corruptedFile)
{
    if (m_process->state() != QProcess::NotRunning) {
        return false;
    }

    m_referenceFile = referenceFile;
    m_corruptedFile = corruptedFile;
    m_fixedFilePath.clear();
    m_lastError.clear();
    m_stdoutBuffer.clear();
    m_stderrBuffer.clear();

    const QFileInfo corruptInfo(corruptedFile);
    m_process->setWorkingDirectory(corruptInfo.absolutePath());

    const QStringList args = {
        QStringLiteral("-s"),
        QDir::toNativeSeparators(referenceFile),
        QDir::toNativeSeparators(corruptedFile)
    };

    setState(State::Running);
    m_process->start(program, args);

    if (!m_process->waitForStarted(5000)) {
        setState(State::Failed);
        emit finished(false, QString());
        return false;
    }

    return true;
}

void UntruncProcess::cancel()
{
    if (m_process->state() == QProcess::NotRunning) {
        return;
    }

    setState(State::Cancelled);
    m_process->kill();
}

bool UntruncProcess::isRunning() const
{
    return m_state == State::Running;
}

UntruncProcess::State UntruncProcess::state() const
{
    return m_state;
}

QString UntruncProcess::fixedFilePath() const
{
    return m_fixedFilePath;
}

QString UntruncProcess::lastError() const
{
    return m_lastError;
}

QString UntruncProcess::predictFixedPath(const QString &referenceFile, const QString &corruptedFile)
{
    const QFileInfo refInfo(referenceFile);
    QString refExt = refInfo.suffix().toLower();
    if (refExt.isEmpty()) refExt = QStringLiteral("mp4");

    // Primary: untrunc native convention `corrupted + "_fixed." + ext`
    const QString p1 = corruptedFile + QStringLiteral("_fixed.") + refExt;
    if (QFileInfo::exists(p1)) return p1;

    // Secondary: `baseName + "_fixed." + ext`
    const QFileInfo corruptInfo(corruptedFile);
    const QString p2 = corruptInfo.absolutePath() + QStringLiteral("/")
                       + corruptInfo.completeBaseName() + QStringLiteral("_fixed.") + refExt;
    if (QFileInfo::exists(p2)) return p2;

    // Check directory for any newly formed fixed files matching the base
    const QDir dir(corruptInfo.absolutePath());
    const QStringList matches = dir.entryList({corruptInfo.completeBaseName() + QStringLiteral("*_fixed*")},
                                              QDir::Files, QDir::Time);
    if (!matches.isEmpty()) {
        return dir.absoluteFilePath(matches.first());
    }

    return p1;
}

void UntruncProcess::setState(State state)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged(m_state);
    }
}

void UntruncProcess::processOutputBuffer(QString &buffer)
{
    static const QRegularExpression delimiterRegex(QStringLiteral("[\r\n]"));
    static const QRegularExpression pctRegex(QStringLiteral(R"((\d+(?:\.\d+)?)\s*%)"));

    int idx;
    while ((idx = buffer.indexOf(delimiterRegex)) >= 0) {
        QString line = buffer.left(idx).trimmed();
        int skip = 1;
        if (idx + 1 < buffer.size()) {
            const QChar c1 = buffer.at(idx);
            const QChar c2 = buffer.at(idx + 1);
            if ((c1 == QLatin1Char('\r') && c2 == QLatin1Char('\n')) ||
                (c1 == QLatin1Char('\n') && c2 == QLatin1Char('\r'))) {
                skip = 2;
            }
        }
        buffer = buffer.mid(idx + skip);

        if (!line.isEmpty()) {
            const auto match = pctRegex.match(line);
            if (match.hasMatch()) {
                const double pct = match.captured(1).toDouble();
                emit progressChanged(qBound(0.0, pct / 100.0, 1.0));
            }
            emit logMessage(line);
        }
    }
}

void UntruncProcess::onReadyReadStdout()
{
    m_stdoutBuffer.append(QString::fromUtf8(m_process->readAllStandardOutput()));
    processOutputBuffer(m_stdoutBuffer);
}

void UntruncProcess::onReadyReadStderr()
{
    const QString err = QString::fromUtf8(m_process->readAllStandardError());
    m_lastError.append(err);
    m_stderrBuffer.append(err);
    processOutputBuffer(m_stderrBuffer);
}

void UntruncProcess::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    // Flush any remaining buffers
    if (!m_stdoutBuffer.trimmed().isEmpty()) {
        emit logMessage(m_stdoutBuffer.trimmed());
        m_stdoutBuffer.clear();
    }
    if (!m_stderrBuffer.trimmed().isEmpty()) {
        emit logMessage(m_stderrBuffer.trimmed());
        m_stderrBuffer.clear();
    }

    if (m_state == State::Cancelled) {
        emit finished(false, QString());
        return;
    }

    const QString fixed = predictFixedPath(m_referenceFile, m_corruptedFile);
    const bool fixedExists = QFileInfo::exists(fixed);

    // Untrunc returns 0 on success, or even if non-zero exit code, check if fixed file was created and is non-empty
    const bool success = (exitStatus == QProcess::NormalExit) &&
                         (exitCode == 0 || (fixedExists && QFileInfo(fixed).size() > 0));

    if (success && fixedExists) {
        emit progressChanged(1.0);
        m_fixedFilePath = fixed;
        setState(State::Finished);
        emit finished(true, m_fixedFilePath);
    } else {
        setState(State::Failed);
        emit finished(false, QString());
    }
}

void UntruncProcess::onErrorOccurred(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart) {
        m_lastError = QStringLiteral("Failed to start untrunc process");
        setState(State::Failed);
        emit finished(false, QString());
    }
}
