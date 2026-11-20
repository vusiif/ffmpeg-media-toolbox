#pragma once

#include "SmartCutEngine.h"
#include "media/MediaFile.h"

class Job;

// Builds one ordered job per output. Workspaces are allocated by JobQueue at
// execution time, so persisted jobs and retries never depend on old temp files.
// This builder does no I/O. The caller owns the returned job until enqueueing.
// Partial-GOP copy is limited to compatible progressive H.264/CFR/no-B-frame
// streams; other precise cuts use an explicitly labeled re-encode fallback.
class CutExportBuilder
{
public:
    static Job *createJob(const QString &ffmpeg, const MediaFile &media,
                          const QString &output, const QVector<CutSegment> &segments,
                          CutMode mode, const QVector<double> &keyframes,
                          QString *error = nullptr, QObject *parent = nullptr);
};
