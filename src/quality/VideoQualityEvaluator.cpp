#include "VideoQualityEvaluator.h"

#include "ffmpeg/FFmpegLocator.h"

#include <QRegularExpression>
#include <QTextStream>
#include <QtMath>

namespace {
static const QRegularExpression s_vmafRegex(
    QStringLiteral(R"(VMAF\s+score:\s*(\d+(?:\.\d+)?))"),
    QRegularExpression::CaseInsensitiveOption);

static const QRegularExpression s_ssimRegex(
    QStringLiteral(R"(SSIM\s+.*All:\s*(\d+(?:\.\d+)?))"),
    QRegularExpression::CaseInsensitiveOption);

static const QRegularExpression s_psnrRegex(
    QStringLiteral(R"(PSNR\s+.*average:\s*(\d+(?:\.\d+)?|inf))"),
    QRegularExpression::CaseInsensitiveOption);
} // namespace

QString VideoQualityResult::vmafRating() const
{
    if (vmaf < 0.0) return QString();
    if (vmaf >= 95.0) return QStringLiteral("Visually Lossless");
    if (vmaf >= 85.0) return QStringLiteral("High Quality");
    if (vmaf >= 75.0) return QStringLiteral("Good");
    return QStringLiteral("Noticeable Degradation");
}

QColor VideoQualityResult::ratingColor() const
{
    if (vmaf < 0.0) return QColor(128, 128, 128);
    if (vmaf >= 95.0) return QColor(16, 124, 65);   // Emerald Green
    if (vmaf >= 85.0) return QColor(40, 167, 69);   // Green
    if (vmaf >= 75.0) return QColor(230, 126, 34);  // Orange
    return QColor(231, 76, 60);                      // Red
}

QString VideoQualityResult::summaryText() const
{
    QStringList parts;
    if (vmaf >= 0.0) {
        parts << QStringLiteral("VMAF: %1 (%2)").arg(QString::number(vmaf, 'f', 1), vmafRating());
    }
    if (ssim >= 0.0) {
        parts << QStringLiteral("SSIM: %1").arg(QString::number(ssim, 'f', 4));
    }
    if (psnr >= 0.0) {
        parts << QStringLiteral("PSNR: %1 dB").arg(QString::number(psnr, 'f', 2));
    }
    return parts.join(QStringLiteral(" | "));
}

VideoQualityEvaluator::VideoQualityEvaluator(const FFmpegLocator *locator, QObject *parent)
    : QObject(parent)
    , m_locator(locator)
    , m_process(new QProcess(this))
{
    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &VideoQualityEvaluator::onReadyReadStdout);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &VideoQualityEvaluator::onReadyReadStderr);
    connect(m_process, &QProcess::finished,
            this, &VideoQualityEvaluator::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &VideoQualityEvaluator::onProcessError);
}

VideoQualityEvaluator::~VideoQualityEvaluator()
{
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }
}

bool VideoQualityEvaluator::parseVmaf(const QString &line, double &score)
{
    QRegularExpressionMatch m = s_vmafRegex.match(line);
    if (m.hasMatch()) {
        score = m.captured(1).toDouble();
        return true;
    }
    return false;
}

bool VideoQualityEvaluator::parseSsim(const QString &line, double &score)
{
    QRegularExpressionMatch m = s_ssimRegex.match(line);
    if (m.hasMatch()) {
        score = m.captured(1).toDouble();
        return true;
    }
    return false;
}

bool VideoQualityEvaluator::parsePsnr(const QString &line, double &score)
{
    QRegularExpressionMatch m = s_psnrRegex.match(line);
    if (m.hasMatch()) {
        const QString cap = m.captured(1).toLower();
        if (cap == QLatin1String("inf")) {
            score = 100.0;
        } else {
            score = cap.toDouble();
        }
        return true;
    }
    return false;
}

QString VideoQualityEvaluator::generateReport(const QString &referencePath,
                                             const QString &distortedPath,
                                             const VideoQualityResult &res,
                                             const VideoQualityOptions &opts)
{
    QString report;
    QTextStream out(&report);
    out << "=========================================================\n";
    out << "       FFmpeg Media Toolbox - Video Quality Report       \n";
    out << "=========================================================\n";
    out << "Reference File: " << referencePath << "\n";
    out << "Distorted File: " << distortedPath << "\n";
    if (opts.sampleDuration > 0.0) {
        out << "Sample Range:   " << QString::number(opts.sampleStart, 'f', 2) << "s - "
            << QString::number(opts.sampleStart + opts.sampleDuration, 'f', 2) << "s\n";
    } else {
        out << "Evaluation:     Full Video Stream\n";
    }
    out << "---------------------------------------------------------\n";
    out << "Objective Quality Metrics:\n";
    if (res.vmaf >= 0.0) {
        out << "  - VMAF Score: " << QString::number(res.vmaf, 'f', 2)
            << " / 100 (" << res.vmafRating() << ")\n";
    }
    if (res.ssim >= 0.0) {
        out << "  - SSIM:       " << QString::number(res.ssim, 'f', 4) << "\n";
    }
    if (res.psnr >= 0.0) {
        out << "  - PSNR:       " << QString::number(res.psnr, 'f', 2) << " dB\n";
    }
    out << "---------------------------------------------------------\n";
    out << "VMAF Rating Scale:\n";
    out << "  - 95 - 100: Visually Lossless (Indistinguishable from source)\n";
    out << "  - 85 - 94:  High Quality (Suitable for high-end web/streaming)\n";
    out << "  - 75 - 84:  Good Quality (Minor artifacts under close inspection)\n";
    out << "  - < 75:     Noticeable Degradation (Rate increase recommended)\n";
    out << "=========================================================\n";
    return report;
}

QStringList VideoQualityEvaluator::buildCommand(const QString &referencePath,
                                               const QString &distortedPath,
                                               const VideoQualityOptions &options,
                                               int refWidth,
                                               int refHeight)
{
    QStringList args;
    args << QStringLiteral("-nostats")
         << QStringLiteral("-progress") << QStringLiteral("pipe:1");

    // Input 0: Reference
    if (options.sampleDuration > 0.0) {
        if (options.sampleStart > 0.0) {
            args << QStringLiteral("-ss") << QString::number(options.sampleStart, 'f', 2);
        }
        args << QStringLiteral("-t") << QString::number(options.sampleDuration, 'f', 2);
    }
    args << QStringLiteral("-i") << referencePath;

    // Input 1: Distorted
    if (options.sampleDuration > 0.0) {
        if (options.sampleStart > 0.0) {
            args << QStringLiteral("-ss") << QString::number(options.sampleStart, 'f', 2);
        }
        args << QStringLiteral("-t") << QString::number(options.sampleDuration, 'f', 2);
    }
    args << QStringLiteral("-i") << distortedPath;

    // Filter complex
    QStringList filterParts;
    QString distStream = QStringLiteral("[1:v]");

    if (refWidth > 0 && refHeight > 0) {
        filterParts << QStringLiteral("[1:v]scale=%1:%2:flags=bicubic[dist]")
                           .arg(refWidth).arg(refHeight);
        distStream = QStringLiteral("[dist]");
    }

    if (options.calculateVmaf) {
        QString vmafOpts;
        if (options.subsample > 1) {
            vmafOpts = QStringLiteral("n_subsample=%1").arg(options.subsample);
        }
        if (options.threads > 0) {
            if (!vmafOpts.isEmpty()) vmafOpts += QLatin1Char(':');
            vmafOpts += QStringLiteral("n_threads=%1").arg(options.threads);
        }
        if (vmafOpts.isEmpty()) {
            filterParts << QStringLiteral("%1[0:v]libvmaf").arg(distStream);
        } else {
            filterParts << QStringLiteral("%1[0:v]libvmaf=%2").arg(distStream, vmafOpts);
        }
    }

    if (options.calculateSsim) {
        filterParts << QStringLiteral("%1[0:v]ssim").arg(distStream);
    }

    if (options.calculatePsnr) {
        filterParts << QStringLiteral("%1[0:v]psnr").arg(distStream);
    }

    if (filterParts.isEmpty()) {
        filterParts << QStringLiteral("%1[0:v]libvmaf").arg(distStream);
    }

    args << QStringLiteral("-filter_complex") << filterParts.join(QLatin1Char(';'));
    args << QStringLiteral("-f") << QStringLiteral("null") << QStringLiteral("-");

    return args;
}

bool VideoQualityEvaluator::startEvaluation(const QString &referencePath,
                                           const QString &distortedPath,
                                           const VideoQualityOptions &options,
                                           double totalDuration,
                                           int refWidth,
                                           int refHeight)
{
    if (m_process->state() != QProcess::NotRunning) {
        return false;
    }

    m_totalDuration = (options.sampleDuration > 0.0) ? options.sampleDuration : totalDuration;
    m_options = options;
    m_progress = 0.0;
    m_currentDecodeTime = 0.0;
    m_stdoutBuffer.clear();
    m_stderrBuffer.clear();
    m_result = VideoQualityResult();

    const QStringList args = buildCommand(referencePath, distortedPath, options, refWidth, refHeight);

    m_running = true;
    m_process->start(m_locator->ffmpegPath(), args);
    if (!m_process->waitForStarted(5000)) {
        m_running = false;
        m_result.success = false;
        m_result.errorMessage = QStringLiteral("Failed to start FFmpeg process");
        emit finished(m_result);
        return false;
    }

    return true;
}

void VideoQualityEvaluator::cancel()
{
    if (m_process->state() == QProcess::NotRunning) {
        return;
    }
    m_process->kill();
}

bool VideoQualityEvaluator::isRunning() const
{
    return m_running;
}

double VideoQualityEvaluator::progress() const
{
    return m_progress;
}

VideoQualityResult VideoQualityEvaluator::result() const
{
    return m_result;
}

void VideoQualityEvaluator::onReadyReadStdout()
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

void VideoQualityEvaluator::onReadyReadStderr()
{
    m_stderrBuffer.append(QString::fromUtf8(m_process->readAllStandardError()));

    int newlineIdx;
    while ((newlineIdx = m_stderrBuffer.indexOf(QLatin1Char('\n'))) >= 0) {
        const QString line = m_stderrBuffer.left(newlineIdx).trimmed();
        m_stderrBuffer = m_stderrBuffer.mid(newlineIdx + 1);

        if (line.isEmpty()) continue;
        emit logMessage(line);

        double val = 0.0;
        if (parseVmaf(line, val)) {
            m_result.vmaf = val;
        } else if (parseSsim(line, val)) {
            m_result.ssim = val;
        } else if (parsePsnr(line, val)) {
            m_result.psnr = val;
        }
    }
}

void VideoQualityEvaluator::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    m_running = false;
    m_progress = 1.0;
    emit progressChanged(1.0);

    m_result.success = (exitStatus == QProcess::NormalExit &&
                        (m_result.vmaf >= 0.0 || m_result.ssim >= 0.0 || m_result.psnr >= 0.0));

    if (!m_result.success && m_result.errorMessage.isEmpty()) {
        m_result.errorMessage = QStringLiteral("Evaluation failed or no scores were produced");
    }

    emit finished(m_result);
}

void VideoQualityEvaluator::onProcessError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart) {
        m_running = false;
        m_result.success = false;
        m_result.errorMessage = QStringLiteral("FFmpeg process failed to start");
        emit finished(m_result);
    }
}
