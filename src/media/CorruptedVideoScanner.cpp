#include "CorruptedVideoScanner.h"

#include "ffmpeg/FFmpegLocator.h"

#include <QRegularExpression>
#include <QTextStream>
#include <QtMath>
#include <algorithm>

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

// Regex for silence detection
static const QRegularExpression s_silenceStartRegex(
    QStringLiteral(R"(silence_start:\s*(-?\d+(?:\.\d+)?))"),
    QRegularExpression::CaseInsensitiveOption);

static const QRegularExpression s_silenceEndRegex(
    QStringLiteral(R"(silence_end:\s*(-?\d+(?:\.\d+)?)(?:\s*\|\s*silence_duration:\s*(-?\d+(?:\.\d+)?))?)"),
    QRegularExpression::CaseInsensitiveOption);

// Regex for black frame detection
static const QRegularExpression s_blackRegex(
    QStringLiteral(R"(black_start:\s*(-?\d+(?:\.\d+)?)\s+black_end:\s*(-?\d+(?:\.\d+)?)(?:\s+black_duration:\s*(-?\d+(?:\.\d+)?))?)"),
    QRegularExpression::CaseInsensitiveOption);

// Regex for freeze frame detection
static const QRegularExpression s_freezeStartRegex(
    QStringLiteral(R"((?:lavfi\.freezedetect\.)?freeze_start:\s*(-?\d+(?:\.\d+)?))"),
    QRegularExpression::CaseInsensitiveOption);

static const QRegularExpression s_freezeEndRegex(
    QStringLiteral(R"((?:lavfi\.freezedetect\.)?freeze_end:\s*(-?\d+(?:\.\d+)?))"),
    QRegularExpression::CaseInsensitiveOption);

static const QRegularExpression s_freezeDurationRegex(
    QStringLiteral(R"((?:lavfi\.freezedetect\.)?freeze_duration:\s*(-?\d+(?:\.\d+)?))"),
    QRegularExpression::CaseInsensitiveOption);
} // namespace

QString CorruptedSegment::typeName() const
{
    switch (type) {
    case QCIssueType::Silence:
        return QStringLiteral("Silence");
    case QCIssueType::BlackFrame:
        return QStringLiteral("Black Frame");
    case QCIssueType::FreezeFrame:
        return QStringLiteral("Freeze Frame");
    case QCIssueType::DecodeError:
    default:
        return QStringLiteral("Decode Error");
    }
}

QColor CorruptedSegment::color() const
{
    switch (type) {
    case QCIssueType::Silence:
        return QColor(50, 150, 250, 210);    // Blue
    case QCIssueType::BlackFrame:
        return QColor(100, 100, 110, 210);   // Dark Gray
    case QCIssueType::FreezeFrame:
        return QColor(245, 160, 30, 210);    // Amber / Orange
    case QCIssueType::DecodeError:
    default:
        return QColor(235, 60, 60, 210);     // Red
    }
}

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
    current.type = QCIssueType::DecodeError;
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
            current.type = QCIssueType::DecodeError;
            current.start = t;
            current.end = t + defaultFrameDuration;
            current.errorMsg = msg;
        }
    }
    result.append(current);
    return result;
}

bool CorruptedVideoScanner::parseSilenceDetect(const QString &line, double &start, double &end, double &duration, bool &isStart)
{
    QRegularExpressionMatch matchStart = s_silenceStartRegex.match(line);
    if (matchStart.hasMatch()) {
        isStart = true;
        start = matchStart.captured(1).toDouble();
        end = -1.0;
        duration = 0.0;
        return true;
    }

    QRegularExpressionMatch matchEnd = s_silenceEndRegex.match(line);
    if (matchEnd.hasMatch()) {
        isStart = false;
        end = matchEnd.captured(1).toDouble();
        if (matchEnd.lastCapturedIndex() >= 2 && !matchEnd.captured(2).isEmpty()) {
            duration = matchEnd.captured(2).toDouble();
            start = qMax(0.0, end - duration);
        } else {
            duration = 0.0;
            start = -1.0;
        }
        return true;
    }

    return false;
}

bool CorruptedVideoScanner::parseBlackDetect(const QString &line, double &start, double &end, double &duration)
{
    QRegularExpressionMatch match = s_blackRegex.match(line);
    if (match.hasMatch()) {
        start = match.captured(1).toDouble();
        end = match.captured(2).toDouble();
        if (match.lastCapturedIndex() >= 3 && !match.captured(3).isEmpty()) {
            duration = match.captured(3).toDouble();
        } else {
            duration = qMax(0.0, end - start);
        }
        return true;
    }
    return false;
}

bool CorruptedVideoScanner::parseFreezeDetect(const QString &line, double &start, double &end, double &duration, bool &isStart)
{
    QRegularExpressionMatch matchStart = s_freezeStartRegex.match(line);
    if (matchStart.hasMatch()) {
        isStart = true;
        start = matchStart.captured(1).toDouble();
        end = -1.0;
        duration = 0.0;
        return true;
    }

    QRegularExpressionMatch matchDur = s_freezeDurationRegex.match(line);
    if (matchDur.hasMatch()) {
        duration = matchDur.captured(1).toDouble();
    }

    QRegularExpressionMatch matchEnd = s_freezeEndRegex.match(line);
    if (matchEnd.hasMatch()) {
        isStart = false;
        end = matchEnd.captured(1).toDouble();
        start = qMax(0.0, end - duration);
        return true;
    }

    return false;
}

QString CorruptedVideoScanner::generateReport(const QString &filePath, double totalDuration, const QVector<CorruptedSegment> &segments)
{
    QString report;
    QTextStream out(&report);
    out << "=========================================================\n";
    out << "       FFmpeg Media Toolbox - Quality Control Report     \n";
    out << "=========================================================\n";
    out << "File: " << filePath << "\n";
    out << "Duration: " << QString::number(totalDuration, 'f', 2) << " s\n";
    out << "Total Issues Detected: " << segments.size() << "\n\n";

    int decodeCount = 0, silenceCount = 0, blackCount = 0, freezeCount = 0;
    for (const auto &seg : segments) {
        if (seg.type == QCIssueType::DecodeError) ++decodeCount;
        else if (seg.type == QCIssueType::Silence) ++silenceCount;
        else if (seg.type == QCIssueType::BlackFrame) ++blackCount;
        else if (seg.type == QCIssueType::FreezeFrame) ++freezeCount;
    }

    out << "Summary by Category:\n";
    out << "  - Corrupted/Decode Errors: " << decodeCount << "\n";
    out << "  - Silence Intervals:       " << silenceCount << "\n";
    out << "  - Black Frame Intervals:   " << blackCount << "\n";
    out << "  - Freeze Frame Intervals:  " << freezeCount << "\n";
    out << "---------------------------------------------------------\n";
    out << "Detailed Issues Timeline:\n";

    for (int i = 0; i < segments.size(); ++i) {
        const auto &seg = segments[i];
        out << QString("[%1] %2 | %3s - %4s (dur: %5s) | %6\n")
                   .arg(i + 1, 3)
                   .arg(seg.typeName(), -12)
                   .arg(seg.start, 7, 'f', 2)
                   .arg(seg.end, 7, 'f', 2)
                   .arg(seg.end - seg.start, 5, 'f', 2)
                   .arg(seg.errorMsg);
    }
    out << "=========================================================\n";
    return report;
}

bool CorruptedVideoScanner::startScan(const QString &filePath, double totalDuration)
{
    return startScan(filePath, totalDuration, QCOptions());
}

bool CorruptedVideoScanner::startScan(const QString &filePath, double totalDuration, const QCOptions &options)
{
    if (m_process->state() != QProcess::NotRunning) {
        return false;
    }

    m_totalDuration = totalDuration;
    m_options = options;
    m_progress = 0.0;
    m_currentDecodeTime = 0.0;
    m_stdoutBuffer.clear();
    m_stderrBuffer.clear();
    m_segments.clear();
    m_hasActiveSegment = false;
    m_activeSilenceStart = -1.0;
    m_activeFreezeStart = -1.0;
    m_activeFreezeDuration = 0.0;

    QStringList vfParts;
    if (m_options.hasVideo) {
        if (m_options.checkBlackFrames) {
            vfParts << QStringLiteral("blackdetect=d=%1:pic_th=%2:pix_th=%3")
                           .arg(QString::number(m_options.blackMinDuration, 'f', 2),
                                QString::number(m_options.blackPicTh, 'f', 2),
                                QString::number(m_options.blackPixTh, 'f', 2));
        }
        if (m_options.checkFreezeFrames) {
            vfParts << QStringLiteral("freezedetect=n=%1dB:d=%2")
                           .arg(QString::number(m_options.freezeNoiseDb, 'f', 1),
                                QString::number(m_options.freezeMinDuration, 'f', 1));
        }
    }

    QStringList afParts;
    if (m_options.hasAudio && m_options.checkSilence) {
        afParts << QStringLiteral("silencedetect=noise=%1dB:d=%2")
                       .arg(QString::number(m_options.silenceNoiseDb, 'f', 1),
                            QString::number(m_options.silenceMinDuration, 'f', 1));
    }

    const bool hasFilters = (!vfParts.isEmpty() || !afParts.isEmpty());

    // Use null decode: ffmpeg -nostats -progress pipe:1 -v info/error -hwaccel auto -i <file> [filters] -f null -
    QStringList args = {
        QStringLiteral("-nostats"),
        QStringLiteral("-progress"),
        QStringLiteral("pipe:1"),
        QStringLiteral("-v"),
        (hasFilters ? QStringLiteral("info") : QStringLiteral("error")),
        QStringLiteral("-hwaccel"),
        QStringLiteral("auto"),
        QStringLiteral("-i"),
        filePath
    };

    if (!vfParts.isEmpty()) {
        args << QStringLiteral("-vf") << vfParts.join(QLatin1Char(','));
    }
    if (!afParts.isEmpty()) {
        args << QStringLiteral("-af") << afParts.join(QLatin1Char(','));
    }

    args << QStringLiteral("-f") << QStringLiteral("null") << QStringLiteral("-");

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

QCOptions CorruptedVideoScanner::options() const
{
    return m_options;
}

void CorruptedVideoScanner::recordError(double timestamp, const QString &errorMsg)
{
    if (timestamp < 0.0) return;

    if (!m_hasActiveSegment) {
        m_activeSegment.type = QCIssueType::DecodeError;
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
        m_activeSegment.type = QCIssueType::DecodeError;
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

        // 1. Silence detection
        if (line.contains(QLatin1String("silencedetect"))) {
            double start = 0.0, end = 0.0, dur = 0.0;
            bool isStart = false;
            if (parseSilenceDetect(line, start, end, dur, isStart)) {
                if (isStart) {
                    m_activeSilenceStart = start;
                } else {
                    const double s = (m_activeSilenceStart >= 0.0) ? m_activeSilenceStart : qMax(0.0, end - dur);
                    m_activeSilenceStart = -1.0;
                    CorruptedSegment seg;
                    seg.type = QCIssueType::Silence;
                    seg.start = s;
                    seg.end = end;
                    seg.errorMsg = QStringLiteral("Silence detected (%1s)").arg(QString::number(end - s, 'f', 2));
                    m_segments.append(seg);
                    emit segmentDiscovered(seg);
                }
            }
            continue;
        }

        // 2. Black frame detection
        if (line.contains(QLatin1String("blackdetect"))) {
            double start = 0.0, end = 0.0, dur = 0.0;
            if (parseBlackDetect(line, start, end, dur)) {
                CorruptedSegment seg;
                seg.type = QCIssueType::BlackFrame;
                seg.start = start;
                seg.end = end;
                seg.errorMsg = QStringLiteral("Black frame detected (%1s)").arg(QString::number(end - start, 'f', 2));
                m_segments.append(seg);
                emit segmentDiscovered(seg);
            }
            continue;
        }

        // 3. Freeze frame detection
        if (line.contains(QLatin1String("freezedetect"))) {
            double start = 0.0, end = 0.0, dur = 0.0;
            bool isStart = false;
            if (parseFreezeDetect(line, start, end, dur, isStart)) {
                if (isStart) {
                    m_activeFreezeStart = start;
                } else if (end >= 0.0) {
                    const double s = (m_activeFreezeStart >= 0.0) ? m_activeFreezeStart : qMax(0.0, end - dur);
                    m_activeFreezeStart = -1.0;
                    CorruptedSegment seg;
                    seg.type = QCIssueType::FreezeFrame;
                    seg.start = s;
                    seg.end = end;
                    seg.errorMsg = QStringLiteral("Freeze frame detected (%1s)").arg(QString::number(end - s, 'f', 2));
                    m_segments.append(seg);
                    emit segmentDiscovered(seg);
                }
            }
            continue;
        }

        // 4. Decode errors (if enabled)
        if (m_options.checkDecodeErrors && s_errorKeywordRegex.match(line).hasMatch()) {
            if (!line.startsWith(QLatin1String("Input #")) &&
                !line.startsWith(QLatin1String("Output #")) &&
                !line.startsWith(QLatin1String("Stream #")) &&
                !line.startsWith(QLatin1String("configuration:")) &&
                !line.startsWith(QLatin1String("built with"))) {
                double t = extractTimestampFromLine(line);
                if (t < 0.0) {
                    t = m_currentDecodeTime;
                }
                recordError(t, line);
            }
        }
    }
}

void CorruptedVideoScanner::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    finalizeCurrentSegment();

    // Finalize pending silence if video ended while silent
    if (m_activeSilenceStart >= 0.0) {
        const double endTime = (m_totalDuration > 0.0) ? m_totalDuration : m_currentDecodeTime;
        if (endTime > m_activeSilenceStart) {
            CorruptedSegment seg;
            seg.type = QCIssueType::Silence;
            seg.start = m_activeSilenceStart;
            seg.end = endTime;
            seg.errorMsg = QStringLiteral("Silence detected (%1s)").arg(QString::number(endTime - m_activeSilenceStart, 'f', 2));
            m_segments.append(seg);
            emit segmentDiscovered(seg);
        }
        m_activeSilenceStart = -1.0;
    }

    // Finalize pending freeze if video ended while frozen
    if (m_activeFreezeStart >= 0.0) {
        const double endTime = (m_totalDuration > 0.0) ? m_totalDuration : m_currentDecodeTime;
        if (endTime > m_activeFreezeStart) {
            CorruptedSegment seg;
            seg.type = QCIssueType::FreezeFrame;
            seg.start = m_activeFreezeStart;
            seg.end = endTime;
            seg.errorMsg = QStringLiteral("Freeze frame detected (%1s)").arg(QString::number(endTime - m_activeFreezeStart, 'f', 2));
            m_segments.append(seg);
            emit segmentDiscovered(seg);
        }
        m_activeFreezeStart = -1.0;
    }

    // Sort segments chronologically by start timestamp
    std::sort(m_segments.begin(), m_segments.end(), [](const CorruptedSegment &a, const CorruptedSegment &b) {
        return a.start < b.start;
    });

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
