#include "SmartCutEngine.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>

SmartCutEngine::SmartCutEngine(QObject *parent)
    : QObject(parent)
{
}

QStringList SmartCutEngine::buildKeyframeProbeCommand(const QString &ffprobePath, const QString &inputPath)
{
    QStringList args;
    args << (ffprobePath.isEmpty() ? QStringLiteral("ffprobe") : ffprobePath);
    args << QStringLiteral("-v") << QStringLiteral("error");
    args << QStringLiteral("-select_streams") << QStringLiteral("v:0");
    args << QStringLiteral("-skip_frame") << QStringLiteral("nokey");
    args << QStringLiteral("-show_entries") << QStringLiteral("frame=pts_time");
    args << QStringLiteral("-of") << QStringLiteral("csv=p=0");
    args << inputPath;
    return args;
}

QVector<double> SmartCutEngine::parseKeyframeOutput(const QString &output)
{
    QVector<double> keyframes;
    const static QRegularExpression re(QStringLiteral(R"((\d+(?:\.\d+)?))"));
    QRegularExpressionMatchIterator it = re.globalMatch(output);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        bool ok = false;
        const double val = match.captured(1).toDouble(&ok);
        if (ok && val >= 0.0) {
            keyframes.append(val);
        }
    }

    std::sort(keyframes.begin(), keyframes.end());

    // Deduplicate within 0.005s tolerance
    QVector<double> deduped;
    for (double kf : keyframes) {
        if (deduped.isEmpty() || (kf - deduped.last() > 0.005)) {
            deduped.append(kf);
        }
    }
    return deduped;
}

double SmartCutEngine::findPrevKeyframe(double targetSec, const QVector<double> &keyframes)
{
    if (keyframes.isEmpty()) return 0.0;
    if (targetSec <= keyframes.first()) return keyframes.first();
    if (targetSec >= keyframes.last()) return keyframes.last();

    auto it = std::upper_bound(keyframes.begin(), keyframes.end(), targetSec);
    if (it != keyframes.begin()) {
        --it;
        return *it;
    }
    return keyframes.first();
}

double SmartCutEngine::findNextKeyframe(double targetSec, const QVector<double> &keyframes)
{
    if (keyframes.isEmpty()) return targetSec;
    if (targetSec >= keyframes.last()) return keyframes.last();

    auto it = std::lower_bound(keyframes.begin(), keyframes.end(), targetSec);
    if (it != keyframes.end()) {
        return *it;
    }
    return keyframes.last();
}

double SmartCutEngine::findNearestKeyframe(double targetSec, const QVector<double> &keyframes)
{
    if (keyframes.isEmpty()) return targetSec;
    const double prev = findPrevKeyframe(targetSec, keyframes);
    const double next = findNextKeyframe(targetSec, keyframes);
    return (std::abs(targetSec - prev) <= std::abs(next - targetSec)) ? prev : next;
}

bool SmartCutEngine::isKeyframeAligned(double targetSec, const QVector<double> &keyframes, double tolerance)
{
    if (keyframes.isEmpty()) return false;
    const double nearest = findNearestKeyframe(targetSec, keyframes);
    return std::abs(targetSec - nearest) <= tolerance;
}

QStringList SmartCutEngine::buildLosslessCut(const QString &inputPath, const QString &outputPath,
                                            double startSec, double endSec,
                                            const QString &outputFormat)
{
    QStringList args;
    if (inputPath.isEmpty() || outputPath.isEmpty()) return args;

    args << QStringLiteral("-y");
    if (startSec > 0.0) {
        args << QStringLiteral("-ss") << formatTime(startSec);
    }
    args << QStringLiteral("-i") << inputPath;
    if (endSec > startSec) {
        args << QStringLiteral("-t") << formatTime(endSec - startSec);
    }
    args << QStringLiteral("-c") << QStringLiteral("copy");
    args << QStringLiteral("-avoid_negative_ts") << QStringLiteral("make_zero");
    if (!outputFormat.isEmpty()) {
        args << QStringLiteral("-f") << outputFormat;
    }
    args << outputPath;
    return args;
}

QStringList SmartCutEngine::buildAccurateReencode(const QString &inputPath, const QString &outputPath,
                                                 double startSec, double endSec,
                                                 const QString &videoCodec,
                                                 int crf,
                                                 const QString &outputFormat)
{
    QStringList args;
    if (inputPath.isEmpty() || outputPath.isEmpty()) return args;

    args << QStringLiteral("-y");
    if (startSec > 0.0) {
        args << QStringLiteral("-ss") << formatTime(startSec);
    }
    args << QStringLiteral("-i") << inputPath;
    if (endSec > startSec) {
        args << QStringLiteral("-t") << formatTime(endSec - startSec);
    }
    if (!videoCodec.isEmpty()) {
        args << QStringLiteral("-c:v") << videoCodec;
    }
    if (crf >= 0) {
        args << QStringLiteral("-crf") << QString::number(crf);
    }
    args << QStringLiteral("-c:a") << QStringLiteral("copy");
    if (!outputFormat.isEmpty()) {
        args << QStringLiteral("-f") << outputFormat;
    }
    args << outputPath;
    return args;
}

SmartCutPlan SmartCutEngine::planSmartCut(const QString &inputPath, const QString &outputPath,
                                         double startSec, double endSec,
                                         const QVector<double> &keyframes,
                                         const QString &tempDir)
{
    SmartCutPlan plan;
    plan.finalOutput = outputPath;
    if (endSec <= startSec) return plan;

    const QString baseDir = tempDir.isEmpty() ? QFileInfo(outputPath).absolutePath() : tempDir;
    const QString stem = QFileInfo(outputPath).completeBaseName();
    const QString ext = QFileInfo(outputPath).suffix().isEmpty() ? QStringLiteral("mp4") : QFileInfo(outputPath).suffix();

    // 1. If keyframes list is empty, or both start and end are already aligned to keyframes:
    if (keyframes.isEmpty() || (isKeyframeAligned(startSec, keyframes) && isKeyframeAligned(endSec, keyframes))) {
        SmartChunk chunk;
        chunk.type = SmartChunk::ChunkType::StreamCopy;
        chunk.startSec = startSec;
        chunk.endSec = endSec;
        chunk.tempOutput = outputPath;
        plan.chunks.append(chunk);
        plan.needsConcat = false;
        plan.isValid = true;
        return plan;
    }

    const double nextKeyAfterStart = findNextKeyframe(startSec, keyframes);
    const double prevKeyBeforeEnd = findPrevKeyframe(endSec, keyframes);

    // 2. If range is within a single GOP or too short to have intermediate keyframes:
    if (nextKeyAfterStart >= endSec || prevKeyBeforeEnd <= startSec || nextKeyAfterStart > prevKeyBeforeEnd) {
        SmartChunk chunk;
        chunk.type = SmartChunk::ChunkType::Reencode;
        chunk.startSec = startSec;
        chunk.endSec = endSec;
        chunk.tempOutput = outputPath;
        plan.chunks.append(chunk);
        plan.needsConcat = false;
        plan.isValid = true;
        return plan;
    }

    // 3. Multi-GOP Smart Cut:
    // Head: decode & re-encode from startSec up to the next keyframe
    if (!isKeyframeAligned(startSec, keyframes) && nextKeyAfterStart > startSec) {
        SmartChunk head;
        head.type = SmartChunk::ChunkType::Reencode;
        head.startSec = startSec;
        head.endSec = nextKeyAfterStart;
        head.tempOutput = QStringLiteral("%1/%2_part_head.%3").arg(baseDir, stem, ext);
        plan.chunks.append(head);
    }

    // Body: lossless stream copy from aligned keyframe to last aligned keyframe
    const double bodyStart = isKeyframeAligned(startSec, keyframes) ? startSec : nextKeyAfterStart;
    const double bodyEnd = isKeyframeAligned(endSec, keyframes) ? endSec : prevKeyBeforeEnd;
    if (bodyEnd > bodyStart) {
        SmartChunk body;
        body.type = SmartChunk::ChunkType::StreamCopy;
        body.startSec = bodyStart;
        body.endSec = bodyEnd;
        body.tempOutput = QStringLiteral("%1/%2_part_body.%3").arg(baseDir, stem, ext);
        plan.chunks.append(body);
    }

    // Tail: decode & re-encode from last keyframe up to endSec
    if (!isKeyframeAligned(endSec, keyframes) && endSec > prevKeyBeforeEnd) {
        SmartChunk tail;
        tail.type = SmartChunk::ChunkType::Reencode;
        tail.startSec = prevKeyBeforeEnd;
        tail.endSec = endSec;
        tail.tempOutput = QStringLiteral("%1/%2_part_tail.%3").arg(baseDir, stem, ext);
        plan.chunks.append(tail);
    }

    if (plan.chunks.size() == 1) {
        plan.chunks.first().tempOutput = outputPath;
        plan.needsConcat = false;
    } else if (plan.chunks.size() > 1) {
        plan.needsConcat = true;
        plan.concatListFile = QStringLiteral("%1/%2_concat.txt").arg(baseDir, stem);
    }
    plan.isValid = !plan.chunks.isEmpty();
    return plan;
}

QStringList SmartCutEngine::buildChunkCommand(const QString &inputPath, const SmartChunk &chunk,
                                             const QString &videoCodec, int crf)
{
    if (chunk.type == SmartChunk::ChunkType::StreamCopy) {
        return buildLosslessCut(inputPath, chunk.tempOutput, chunk.startSec, chunk.endSec);
    }
    return buildAccurateReencode(inputPath, chunk.tempOutput, chunk.startSec, chunk.endSec, videoCodec, crf);
}

QStringList SmartCutEngine::buildConcatDemuxer(const QString &concatListPath, const QString &outputPath)
{
    QStringList args;
    args << QStringLiteral("-y");
    args << QStringLiteral("-f") << QStringLiteral("concat");
    args << QStringLiteral("-safe") << QStringLiteral("0");
    args << QStringLiteral("-i") << concatListPath;
    args << QStringLiteral("-c") << QStringLiteral("copy");
    args << outputPath;
    return args;
}

QString SmartCutEngine::generateConcatFileList(const QStringList &filePaths)
{
    QString result;
    for (const QString &path : filePaths) {
        // Replace single quotes for safety
        QString sanitized = path;
        sanitized.replace(QLatin1Char('\\'), QLatin1Char('/'));
        sanitized.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
        result += QStringLiteral("file '%1'\n").arg(sanitized);
    }
    return result;
}

QStringList SmartCutEngine::buildFilterConcat(const QStringList &inputPaths, const QString &outputPath)
{
    QStringList args;
    if (inputPaths.size() < 2 || outputPath.isEmpty()) return args;

    args << QStringLiteral("-y");
    for (const QString &file : inputPaths) {
        args << QStringLiteral("-i") << file;
    }

    QString filter;
    for (int i = 0; i < inputPaths.size(); ++i) {
        filter += QStringLiteral("[%1:v][%1:a]").arg(i);
    }
    filter += QStringLiteral("concat=n=%1:v=1:a=1[outv][outa]").arg(inputPaths.size());

    args << QStringLiteral("-filter_complex") << filter;
    args << QStringLiteral("-map") << QStringLiteral("[outv]");
    args << QStringLiteral("-map") << QStringLiteral("[outa]");
    args << outputPath;
    return args;
}

QString SmartCutEngine::formatTime(double seconds)
{
    if (seconds < 0.0) seconds = 0.0;
    const int totalMs = static_cast<int>(std::round(seconds * 1000.0));
    const int ms = totalMs % 1000;
    const int totalSec = totalMs / 1000;
    const int s = totalSec % 60;
    const int totalMin = totalSec / 60;
    const int m = totalMin % 60;
    const int h = totalMin / 60;

    return QStringLiteral("%1:%2:%3.%4")
        .arg(h, 2, 10, QLatin1Char('0'))
        .arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0'))
        .arg(ms, 3, 10, QLatin1Char('0'));
}

QString SmartCutEngine::formatDuration(double seconds)
{
    if (seconds < 0.0) seconds = 0.0;
    const int totalSec = static_cast<int>(std::round(seconds));
    const int s = totalSec % 60;
    const int totalMin = totalSec / 60;
    const int m = totalMin % 60;
    const int h = totalMin / 60;

    if (h > 0) {
        return QStringLiteral("%1:%2:%3")
            .arg(h, 2, 10, QLatin1Char('0'))
            .arg(m, 2, 10, QLatin1Char('0'))
            .arg(s, 2, 10, QLatin1Char('0'));
    }
    return QStringLiteral("%1:%2")
        .arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0'));
}
