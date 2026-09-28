#pragma once

#include <QString>

struct MediaFile;

namespace OutputEstimate {

// Rough output size in bytes; 0 when the inputs are not enough to be honest
// (unknown duration, missing video dimensions, copy with unknown bitrate).
// Stream copy estimates from the input size/bitrates; re-encodes use a
// documented CRF -> bits-per-pixel heuristic plus the audio bitrate.
qint64 estimateBytes(const MediaFile &input,
                     const QString &videoCodec,
                     const QString &audioCodec,
                     int crf,
                     bool copyStreams);

// "500 B", "1.5 MB", ...; empty for non-positive values.
QString humanizeBytes(qint64 bytes);

} // namespace OutputEstimate
