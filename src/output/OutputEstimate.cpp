#include "OutputEstimate.h"

#include "media/MediaFile.h"

#include <QtMath>

namespace {

// Bits per pixel per frame for the classic x264/x265 CRF ladder: CRF 23
// lands around 0.08 bpp, every 6 CRF steps halve/double the rate. Good to
// about +/-50% on real content - fine for a pre-run estimate.
double bitsPerPixelForCrf(int crf)
{
    const int effectiveCrf = crf >= 0 ? crf : 23;
    return 0.08 * qPow(2.0, (23 - effectiveCrf) / 6.0);
}

double audioBitsPerSecond(const AudioStream &audio, const QString &codec)
{
    const int rate = audio.sampleRate > 0 ? audio.sampleRate : 48000;
    const int channels = audio.channels > 0 ? audio.channels : 2;

    if (codec == QLatin1String("copy")) {
        return audio.bitrate > 0 ? static_cast<double>(audio.bitrate) : 128000.0;
    }
    if (codec == QLatin1String("pcm_s16le")) {
        return static_cast<double>(rate) * channels * 16.0;
    }
    if (codec == QLatin1String("flac")) {
        return static_cast<double>(rate) * channels * 16.0 * 0.6;
    }

    if (audio.bitrate > 0) {
        return static_cast<double>(audio.bitrate);
    }
    if (codec == QLatin1String("libopus")) {
        return 96000.0;
    }
    if (codec == QLatin1String("libmp3lame")) {
        return 192000.0;
    }
    return 128000.0;
}

} // namespace

namespace OutputEstimate {

qint64 estimateBytes(const MediaFile &input,
                     const QString &videoCodec,
                     const QString &audioCodec,
                     int crf,
                     bool copyStreams)
{
    if (!input.isValid() || input.duration <= 0) {
        return 0;
    }

    if (copyStreams && input.fileSize > 0) {
        return input.fileSize;
    }

    double bitsPerSecond = 0.0;

    if (input.hasVideo()) {
        const VideoStream &video = input.primaryVideo();
        if (video.width <= 0 || video.height <= 0) {
            return 0;
        }

        const bool copyVideo = copyStreams || videoCodec == QLatin1String("copy");
        if (copyVideo) {
            if (video.bitrate <= 0) {
                return 0;
            }
            bitsPerSecond += static_cast<double>(video.bitrate);
        } else {
            const double fps = video.frameRate > 0 ? video.frameRate : 25.0;
            bitsPerSecond += bitsPerPixelForCrf(crf) * video.width * video.height * fps;
        }
    }

    if (input.hasAudio()) {
        bitsPerSecond += audioBitsPerSecond(input.primaryAudio(), audioCodec);
    }

    if (bitsPerSecond <= 0) {
        return 0;
    }
    return static_cast<qint64>(bitsPerSecond * input.duration / 8.0);
}

QString humanizeBytes(qint64 bytes)
{
    if (bytes <= 0) {
        return {};
    }
    if (bytes < 1024) {
        return QStringLiteral("%1 B").arg(bytes);
    }
    if (bytes < 1024LL * 1024) {
        return QStringLiteral("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    }
    if (bytes < 1024LL * 1024 * 1024) {
        return QStringLiteral("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    }
    return QStringLiteral("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
}

} // namespace OutputEstimate
