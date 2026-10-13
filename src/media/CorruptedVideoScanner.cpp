#include "CorruptedVideoScanner.h"

#include "ffmpeg/FFmpegLocator.h"

#include <QRegularExpression>
#include <QtMath>

namespace {
// Regex 1: HH:MM:SS.xxx or SS.xxx after time=
static const QRegularExpression s_timeHmsRegex(
    QStringLiteral(R"(time=(\d{2}):(\d{2}):(\d{2}(?:\.\d+)?))"),
    QRegularExpression::CaseInsensitiveOption);

static const QRegularExpression s_timeSecRegex(
    QStringLiteral(R"(time=(\d+(?:\.\d+)?))"),
    QRegularExpression::CaseInsensitiveOption);

// Regex 2: pts or pts_time
static const QRegularExpression s_ptsRegex(
    QStringLiteral(R"((?:pts(?:_time)?|dts)[=:\s]+(\d+(?:\.\d+)?))"),
    QRegularExpression::CaseInsensitiveOption);

// Regex 3: Error keywords
static const QRegularExpression s_errorKeywordRegex(
    QStringLiteral(R"((?:error|corrupt|invalid|conceal|exhausted|failed|missing|mismatch|overflow))"),
    QRegularExpression::CaseInsensitiveOption);
} // namespace

CorruptedVideoScanner::CorruptedVideoScanner(const FFmpegLocator *locator, QObject *parent)
    : QObject(parent)
    , m_locator(locator)
    , m_process(new QProcess(this))
{
    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &CorruptedVideoScanner::onReadyReadStdout);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &CorruptedVideoScanner::onReadyReadStderr);
    connect(m_process, &QProcess::finished,
            this, &CorruptedVideoScanner::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &CorruptedVideoScanner::onProcessError);
}

CorruptedVideoScanner::~CorruptedVideoScanner()
{
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }
}

double CorruptedVideoScanner::extractTimestampFromLine(const QString &line)
{
    // Try HH:MM:SS.xxx
    QRegularExpressionMatch hms = s_timeHmsRegex.match(line);
    if (hms.hasMatch()) {
        const double h = hms.captured(1).toDouble();
        const double m = hms.captured(2).toDouble();
        const double s = hms.captured(3).toDouble();
        return h * 3600.0 + m * 60.0 + s;
    }

    // Try time=xxx.xx
    QRegularExpressionMatch sec = s_timeSecRegex.match(line);
    if (sec.hasMatch()) {
        return sec.captured(1).toDouble();
    }

    // Try pts / dts
    QRegularExpressionMatch pts = s_ptsRegex.match(line);
    if (pts.hasMatch()) {
        const double val = pts.captured(1).toDouble();
        // If pts is huge (> 100000) it might be in 90kHz timebase
        if (val > 100000.0) {
            return val / 90000.0;
        }
        return val;
    }

    return -1.0;
}

QVector<CorruptedSegment> CorruptedVideoScanner::mergeErrorTimestamps(
    const QVector<QPair<double, QString>> &rawErrors,
    double mergeThreshold,
    double defaultFrameDuration)
{
    QVector<CorruptedSegment> result;
    if (rawErrors.isEmpty()) return result;

    CorruptedSegment current;
    current.start = rawErrors.first().first;
    current.end = current.start + defaultFrameDuration;
    current.errorMsg = rawErrors.first().second;

    for (int i = 1; i < rawErrors.size(); ++i) {
        const double t = rawErrors[i].first;
        const QString &msg = rawErrors[i].second;

        if (t - current.end <= mergeThreshold) {
            // Merge into current segment
            current.end = qMax(current.end, t + defaultFrameDuration);
            if (!current.errorMsg.contains(msg) && current.errorMsg.length() < 120) {
                current.errorMsg += QStringLiteral("; ") + msg;
            }
        } else {
            result.append(current);
            current.start = t;
            current.end = t + defaultFrameDuration;
            current.errorMsg = msg;
        }
    }
    result.append(current);
    return result;
}

bool CorruptedVideoScanner::startScan(const QString &filePath, double totalDuration)
{
    if (m_process->state() != QProcess::NotRunning) {
        return false;
    }

    m_totalDuration = totalDuration;
    m_progress = 0.0;
    m_currentDecodeTime = 0.0;
    m_stdoutBuffer.clear();
    m_stderrBuffer.clear();
    m_segments.clear();
    m_hasActiveSegment = false;

    // Use null decode: ffmpeg -nostats -progress pipe:1 -v error -hwaccel auto -i <file> -f null -
    QStringList args = {
        QStringLiteral("-nostats"),
        QStringLiteral("-progress"),
        QStringLiteral("pipe:1"),
        QStringLiteral("-v"),
        QStringLiteral("error"),
        QStringLiteral("-hwaccel"),
        QStringLiteral("auto"),
        QStringLiteral("-i"),
        filePath,
        QStringLiteral("-f"),
        QStringLiteral("null"),
        QStringLiteral("-")
    };

    m_running = true;
    emit runningChanged(true);

    m_process->start(m_locator->ffmpegPath(), args);
    if (!m_process->waitForStarted(5000)) {
        m_running = false;
        emit runningChanged(false);
        emit scanFinished(false, {});
        return false;
    }

    return true;
}

void CorruptedVideoScanner::cancel()
{
    if (m_process->state() == QProcess::NotRunning) {
        return;
    }
    m_process->kill();
}

bool CorruptedVideoScanner::isRunning() const
{
    return m_running;
}

double CorruptedVideoScanner::progress() const
{
    return m_progress;
}

QVector<CorruptedSegment> CorruptedVideoScanner::segments() const
{
    return m_segments;
}

double CorruptedVideoScanner::totalDuration() const
{
    return m_totalDuration;
}

void CorruptedVideoScanner::recordError(double timestamp, const QString &errorMsg)
{
    if (timestamp < 0.0) return;

    if (!m_hasActiveSegment) {
        m_activeSegment.start = timestamp;
        m_activeSegment.end = timestamp + 0.08;
        m_activeSegment.errorMsg = errorMsg;
        m_hasActiveSegment = true;
        return;
    }

    if (timestamp - m_activeSegment.end <= 0.5) {
        m_activeSegment.end = qMax(m_activeSegment.end, timestamp + 0.08);
        if (!m_activeSegment.errorMsg.contains(errorMsg) && m_activeSegment.errorMsg.length() < 120) {
            m_activeSegment.errorMsg += QStringLiteral("; ") + errorMsg;
        }
    } else {
        finalizeCurrentSegment();
        m_activeSegment.start = timestamp;
        m_activeSegment.end = timestamp + 0.08;
        m_activeSegment.errorMsg = errorMsg;
        m_hasActiveSegment = true;
    }
}

void CorruptedVideoScanner::finalizeCurrentSegment()
{
    if (!m_hasActiveSegment) return;

    if (m_totalDuration > 0.0) {
        m_activeSegment.start = qBound(0.0, m_activeSegment.start, m_totalDuration);
        m_activeSegment.end = qBound(0.0, m_activeSegment.end, m_totalDuration);
    }
    m_segments.append(m_activeSegment);
    emit segmentDiscovered(m_activeSegment);
    m_hasActiveSegment = false;
}

void CorruptedVideoScanner::onReadyReadStdout()
{
    m_stdoutBuffer.append(QString::fromUtf8(m_process->readAllStandardOutput()));

    int newlineIdx;
    while ((newlineIdx = m_stdoutBuffer.indexOf(QLatin1Char('\n'))) >= 0) {
        const QString line = m_stdoutBuffer.left(newlineIdx).trimmed();
        m_stdoutBuffer = m_stdoutBuffer.mid(newlineIdx + 1);

        if (line.startsWith(QLatin1String("out_time_us="))) {
            const qint64 us = line.mid(12).toLongLong();
            m_currentDecodeTime = us / 1000000.0;
            if (m_totalDuration > 0.0) {
                m_progress = qBound(0.0, m_currentDecodeTime / m_totalDuration, 1.0);
                emit progressChanged(m_progress);
            }
        } else if (line == QLatin1String("progress=end")) {
            m_progress = 1.0;
            emit progressChanged(1.0);
        }
    }
}

void CorruptedVideoScanner::onReadyReadStderr()
{
    m_stderrBuffer.append(QString::fromUtf8(m_process->readAllStandardError()));

    int newlineIdx;
    while ((newlineIdx = m_stderrBuffer.indexOf(QLatin1Char('\n'))) >= 0) {
        const QString line = m_stderrBuffer.left(newlineIdx).trimmed();
        m_stderrBuffer = m_stderrBuffer.mid(newlineIdx + 1);

        if (line.isEmpty()) continue;
        emit logMessage(line);

        // Check if line indicates an error
        if (s_errorKeywordRegex.match(line).hasMatch()) {
            double t = extractTimestampFromLine(line);
            if (t < 0.0) {
                // Fallback to current decoded position
                t = m_currentDecodeTime;
            }
            recordError(t, line);
        }
    }
}

void CorruptedVideoScanner::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    finalizeCurrentSegment();

    m_running = false;
    emit runningChanged(false);

    const bool success = (exitStatus == QProcess::NormalExit);
    m_progress = 1.0;
    emit progressChanged(1.0);
    emit scanFinished(success, m_segments);
}

void CorruptedVideoScanner::onProcessError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart) {
        m_running = false;
        emit runningChanged(false);
        emit scanFinished(false, {});
    }
}
