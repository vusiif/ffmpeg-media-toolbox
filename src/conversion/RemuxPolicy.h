#pragma once

#include <QString>

struct MediaFile;

namespace RemuxPolicy {

// Conservative container support checks for stream copying. Unknown formats
// or codecs return false so callers fall back to re-encoding.
bool containerSupportsVideoCodec(const QString &format, const QString &codec);
bool containerSupportsAudioCodec(const QString &format, const QString &codec);

// True when every present stream of the input fits the target container and
// can therefore be copied (remux) instead of re-encoded.
bool canCopyAllStreams(const MediaFile &input, const QString &outputFormat);

// Fallback when the input codecs are unknown: only copy when the container
// stays the same.
bool canCopyByExtension(const QString &inputPath, const QString &outputFormat);

} // namespace RemuxPolicy
