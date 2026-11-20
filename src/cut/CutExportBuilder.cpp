#include "CutExportBuilder.h"

#include "i18n/TrId.h"
#include "jobs/Job.h"
#include "conversion/RemuxPolicy.h"

#include <QDir>
#include <QFileInfo>
#include <cmath>
#include <memory>

Job *CutExportBuilder::createJob(const QString &ffmpeg, const MediaFile &media,
                                 const QString &output, const QVector<CutSegment> &segments,
                                 CutMode mode, const QVector<double> &keyframes,
                                 QString *error, QObject *parent)
{
    if (error) error->clear();
    const auto fail = [error](const char *key) -> Job * {
        if (error) *error = trId(key);
        return nullptr;
    };
    if (ffmpeg.isEmpty() || media.path.isEmpty() || !media.hasVideo() || segments.isEmpty() || output.isEmpty()) {
        return fail("cut.export.invalidInput");
    }
    if (QFileInfo(output).absoluteFilePath().compare(QFileInfo(media.path).absoluteFilePath(),
                                                    Qt::CaseInsensitive) == 0 || QFileInfo::exists(output)) {
        return fail("cut.export.outputExists");
    }
    const QString format = QFileInfo(output).suffix().toLower();
    const bool webm = format == QLatin1String("webm");
    if (!QStringList{QStringLiteral("mp4"), QStringLiteral("mkv"),
                     QStringLiteral("mov"), QStringLiteral("webm")}.contains(format)) {
        return fail("cut.export.unsupportedFormat");
    }
    if (mode != CutMode::SmartCut && mode != CutMode::AccurateReencode &&
        mode != CutMode::KeyframeLossless) {
        return fail("cut.export.invalidInput");
    }
    if (mode == CutMode::KeyframeLossless &&
        !RemuxPolicy::containerSupportsVideoCodec(format, media.primaryVideo().codec)) {
        return fail("cut.export.incompatibleCopy");
    }
    double totalDuration = 0;
    for (const auto &segment : segments) {
        if (!std::isfinite(segment.startSec) || !std::isfinite(segment.endSec) ||
            segment.startSec < 0 || !segment.isValid() ||
            (media.duration > 0 && segment.endSec > media.duration + 0.001)) {
            return fail("cut.export.invalidRange");
        }
        totalDuration += segment.duration();
    }

    const auto workspacePath = [](const QString &name) { return Job::workspaceToken() + QLatin1Char('/') + name; };
    const QString stagedName = QStringLiteral("result.") + format;
    const QString stagedOutput = workspacePath(stagedName);
    const QString videoEncoder = webm ? QStringLiteral("libvpx-vp9") : QStringLiteral("libx264");
    const QString audioEncoder = webm ? QStringLiteral("libopus") : QStringLiteral("aac");
    QList<JobStep> steps;
    QMap<QString, QString> files;
    QStringList segmentFiles;
    // A multi-range export must use a single codec configuration throughout.
    // Keep exact boundaries rather than concatenating heterogeneous SPS/audio.
    bool accurateFallback = mode == CutMode::SmartCut && segments.size() > 1;
    const CutMode effectiveMode = accurateFallback ? CutMode::AccurateReencode : mode;

    const auto addStep = [&steps](QStringList args, double duration) {
        // No part of an export may overwrite a pre-existing user file.
        for (QString &arg : args) if (arg == QLatin1String("-y")) arg = QStringLiteral("-n");
        steps.append({args, duration});
    };
    const auto accurate = [&](const CutSegment &segment, const QString &target, bool videoOnly) {
        QStringList args{QStringLiteral("-n"), QStringLiteral("-ss"), SmartCutEngine::formatTime(segment.startSec),
                         QStringLiteral("-i"), media.path, QStringLiteral("-t"),
                         SmartCutEngine::formatTime(segment.duration()), QStringLiteral("-map"), QStringLiteral("0:v:0")};
        if (videoOnly) {
            args << QStringLiteral("-an") << QStringLiteral("-c:v") << QStringLiteral("libx264")
                 << QStringLiteral("-bf") << QStringLiteral("0") << QStringLiteral("-pix_fmt") << QStringLiteral("yuv420p");
        } else {
            args << QStringLiteral("-map") << QStringLiteral("0:a?")
                 << QStringLiteral("-c:v") << videoEncoder << QStringLiteral("-c:a") << audioEncoder;
            if (std::abs(segment.audioOffsetSec) > 0.001) {
                if (segment.audioOffsetSec > 0) {
                    const int ms = static_cast<int>(std::round(segment.audioOffsetSec * 1000.0));
                    args << QStringLiteral("-af") << QStringLiteral("adelay=%1|%1").arg(ms);
                } else {
                    const double adv = -segment.audioOffsetSec;
                    args << QStringLiteral("-af") << QStringLiteral("atrim=start=%1,asetpts=PTS-STARTPTS").arg(adv, 0, 'f', 3);
                }
            }
        }
        args << QStringLiteral("-crf") << QStringLiteral("18");
        if (videoOnly) args << QStringLiteral("-muxdelay") << QStringLiteral("0") << QStringLiteral("-muxpreload") << QStringLiteral("0");
        args << target;
        addStep(args, segment.duration());
    };

    for (int i = 0; i < segments.size(); ++i) {
        const auto &segment = segments[i];
        const QString segmentName = QStringLiteral("segment-%1.mkv").arg(i);
        const QString target = segments.size() == 1 ? stagedOutput : workspacePath(segmentName);
        segmentFiles << segmentName;
        if (effectiveMode == CutMode::KeyframeLossless) {
            if (std::abs(segment.audioOffsetSec) > 0.001) {
                QStringList args{QStringLiteral("-n"), QStringLiteral("-ss"), SmartCutEngine::formatTime(segment.startSec),
                                 QStringLiteral("-i"), media.path, QStringLiteral("-t"),
                                 SmartCutEngine::formatTime(segment.duration()),
                                 QStringLiteral("-map"), QStringLiteral("0:v:0"),
                                 QStringLiteral("-map"), QStringLiteral("0:a?"),
                                 QStringLiteral("-c:v"), QStringLiteral("copy"),
                                 QStringLiteral("-c:a"), audioEncoder};
                if (segment.audioOffsetSec > 0) {
                    const int ms = static_cast<int>(std::round(segment.audioOffsetSec * 1000.0));
                    args << QStringLiteral("-af") << QStringLiteral("adelay=%1|%1").arg(ms);
                } else {
                    const double adv = -segment.audioOffsetSec;
                    args << QStringLiteral("-af") << QStringLiteral("atrim=start=%1,asetpts=PTS-STARTPTS").arg(adv, 0, 'f', 3);
                }
                args << target;
                addStep(args, segment.duration());
            } else {
                addStep(SmartCutEngine::buildLosslessCut(media.path, target, segment.startSec, segment.endSec), segment.duration());
            }
            continue;
        }
        if (effectiveMode == CutMode::AccurateReencode || std::abs(segment.audioOffsetSec) > 0.001) {
            accurate(segment, target, false);
            continue;
        }

        const auto plan = SmartCutEngine::planSmartCut(media.path, target, segment.startSec, segment.endSec, keyframes);
        const auto &video = media.primaryVideo();
        const auto onFrame = [&video](double time) {
            const double frame = time * video.frameRate;
            return video.frameRate > 0 && std::isfinite(frame) && qAbs(frame - std::round(frame)) < 0.00001;
        };
        // Packet-copy splicing is deliberately limited to a known compatible
        // stream layout. B-frame/VFR/other-codec boundaries require full decode;
        // do not silently substitute an inaccurate keyframe cut for them.
        const bool canSplice = !webm && video.codec == QLatin1String("h264") &&
            video.hasBFrames == 0 && video.constantFrameRate && video.zeroStartTime &&
            video.fieldOrder == QLatin1String("progressive") && video.pixelFormat == QLatin1String("yuv420p") &&
            onFrame(segment.startSec) && onFrame(segment.endSec);
        if (!plan.isValid || !canSplice || keyframes.isEmpty()) {
            accurateFallback = true;
            accurate(segment, target, false);
            continue;
        }
        if (!plan.needsConcat) {
            if (plan.chunks.first().type == SmartChunk::ChunkType::StreamCopy) {
                addStep(SmartCutEngine::buildLosslessCut(media.path, target, segment.startSec, segment.endSec), segment.duration());
            } else {
                accurate(segment, target, false);
            }
            continue;
        }

        QString listContent;
        for (int c = 0; c < plan.chunks.size(); ++c) {
            const auto &chunk = plan.chunks[c];
            const QString chunkName = QStringLiteral("chunk-%1-%2.ts").arg(i).arg(c);
            if (chunk.type == SmartChunk::ChunkType::Reencode) {
                accurate({chunk.startSec, chunk.endSec, {}}, workspacePath(chunkName), true);
            } else {
                auto args = SmartCutEngine::buildLosslessCut(media.path, workspacePath(chunkName), chunk.startSec, chunk.endSec);
                args.removeLast();
                args << QStringLiteral("-map") << QStringLiteral("0:v:0") << QStringLiteral("-an")
                     << QStringLiteral("-bsf:v") << QStringLiteral("h264_mp4toannexb")
                     << QStringLiteral("-muxdelay") << QStringLiteral("0") << QStringLiteral("-muxpreload") << QStringLiteral("0")
                     << workspacePath(chunkName);
                addStep(args, chunk.duration());
            }
            listContent += SmartCutEngine::generateConcatFileList({chunkName});
            listContent += QStringLiteral("duration %1\n").arg(chunk.duration(), 0, 'f', 6);
        }
        const QString listName = QStringLiteral("chunks-%1.ffconcat").arg(i);
        files.insert(listName, listContent);
        // Encode the selected audio only once, not separately at every GOP.
        QStringList mux{QStringLiteral("-n"), QStringLiteral("-f"), QStringLiteral("concat"),
                        QStringLiteral("-safe"), QStringLiteral("0"), QStringLiteral("-i"), workspacePath(listName),
                        QStringLiteral("-ss"), SmartCutEngine::formatTime(segment.startSec),
                        QStringLiteral("-i"), media.path, QStringLiteral("-map"), QStringLiteral("0:v:0"),
                        QStringLiteral("-map"), QStringLiteral("1:a?"), QStringLiteral("-c:v"), QStringLiteral("copy"),
                        QStringLiteral("-c:a"), audioEncoder, QStringLiteral("-t"), SmartCutEngine::formatTime(segment.duration()), target};
        addStep(mux, segment.duration());
    }
    if (segments.size() > 1) {
        QString list;
        for (int i = 0; i < segmentFiles.size(); ++i) {
            list += SmartCutEngine::generateConcatFileList({segmentFiles[i]});
            if (effectiveMode != CutMode::KeyframeLossless) {
                list += QStringLiteral("duration %1\n").arg(segments[i].duration(), 0, 'f', 6);
            }
        }
        files.insert(QStringLiteral("segments.ffconcat"), list);
        addStep(SmartCutEngine::buildConcatDemuxer(workspacePath(QStringLiteral("segments.ffconcat")), stagedOutput), totalDuration);
    }

    auto job = std::make_unique<Job>(parent);
    job->setType(JobType::VideoOperation);
    job->setCommandSequence(ffmpeg, steps, files);
    job->setInputFiles({media.path});
    job->setOutputFile(QFileInfo(output).absoluteFilePath());
    job->setStagedOutputFile(stagedName);
    job->setTotalDuration(totalDuration);
    job->setAutoSoftwareFallback(false);
    if (accurateFallback) {
        job->addLog(trId("cut.export.accurateFallback"));
        job->setName(trId("cut.export.accurateFallbackLabel"));
    }
    return job.release();
}
