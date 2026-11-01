#include "FFmpegCommandBuilder.h"
#include "FFmpegCapabilities.h"
#include "conversion/RemuxPolicy.h"
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

FFmpegCommandBuilder::FFmpegCommandBuilder(QObject *parent)
    : QObject(parent)
{
}

bool FFmpegCommandBuilder::canFallbackToSoftware(const QStringList &arguments, QStringList *fallbackArgs)
{
    bool hasHw = false;
    QStringList result;
    result.reserve(arguments.size());

    for (int i = 0; i < arguments.size(); ++i) {
        const QString &arg = arguments.at(i);

        // Strip hardware acceleration flags
        if (arg == QLatin1String("-hwaccel") ||
            arg == QLatin1String("-hwaccel_device") ||
            arg == QLatin1String("-hwaccel_output_format")) {
            hasHw = true;
            if (i + 1 < arguments.size() && !arguments.at(i + 1).startsWith(QLatin1Char('-'))) {
                ++i;
            }
            continue;
        }

        // Replace hardware video encoder with software encoder
        if ((arg == QLatin1String("-c:v") || arg == QLatin1String("-vcodec")) && i + 1 < arguments.size()) {
            const QString &val = arguments.at(i + 1);
            if (FFmpegCapabilities::isHwEncoder(val)) {
                hasHw = true;
                const QString sw = FFmpegCapabilities::fallbackSoftwareEncoder(val);
                result << arg << (sw.isEmpty() ? QStringLiteral("libx264") : sw);
                ++i;
                continue;
            }
        }

        // Sanitize hardware-specific presets (e.g. NVENC p1-p7) to a standard libx264/libx265 preset
        if (arg == QLatin1String("-preset") && i + 1 < arguments.size()) {
            const QString &val = arguments.at(i + 1);
            if (val.startsWith(QLatin1Char('p')) && val.size() == 2 && val.at(1).isDigit()) {
                result << arg << QStringLiteral("medium");
                ++i;
                continue;
            }
        }

        result << arg;
    }

    if (hasHw && fallbackArgs) {
        *fallbackArgs = result;
    }
    return hasHw;
}

QStringList FFmpegCommandBuilder::buildConversion(const ConversionOptions &options) const
{
    QStringList args;

    if (options.inputPath.isEmpty() || options.outputPath.isEmpty()) {
        return args;
    }

    args << QStringLiteral("-y");
    if (!options.hwAccel.trimmed().isEmpty()) {
        args << QStringLiteral("-hwaccel") << options.hwAccel.trimmed();
    }
    if (options.seekSeconds > 0.0) {
        args << QStringLiteral("-ss") << formatTime(options.seekSeconds);
    }
    args << QStringLiteral("-i") << options.inputPath;
    if (options.durationLimit > 0.0) {
        args << QStringLiteral("-t") << formatTime(options.durationLimit);
    }

    // Stream mappings: explicit list takes precedence over preserveAllStreams.
    if (!options.streamMappings.isEmpty()) {
        for (const QString &m : options.streamMappings) {
            args << QStringLiteral("-map") << m;
        }
    } else if (options.preserveAllStreams) {
        const QString outExt = QFileInfo(options.outputPath).suffix().toLower();
        if (RemuxPolicy::isAudioContainer(outExt)) {
            args << QStringLiteral("-map") << QStringLiteral("0:a?");
        } else {
            args << QStringLiteral("-map") << QStringLiteral("0:v?")
                 << QStringLiteral("-map") << QStringLiteral("0:a?");
            if (RemuxPolicy::subtitleCodecForContainer(outExt).has_value()) {
                args << QStringLiteral("-map") << QStringLiteral("0:s?");
            }
        }
    }

    if (options.copyStreams) {
        args << QStringLiteral("-c") << QStringLiteral("copy");
        if (!options.subtitleCodec.isEmpty()) {
            args << QStringLiteral("-c:s") << options.subtitleCodec;
        }
    } else {
        if (!options.videoCodec.isEmpty()) {
            args << QStringLiteral("-c:v") << options.videoCodec;
        }
        if (!options.audioCodec.isEmpty() && !(options.twoPass && options.pass == 1)) {
            args << QStringLiteral("-c:a") << options.audioCodec;
        }
        if (!options.subtitleCodec.isEmpty()) {
            args << QStringLiteral("-c:s") << options.subtitleCodec;
        }

        // Determine effective video bitrate (explicit or derived from target file size)
        int effVideoBitrateKbps = options.videoBitrateKbps;
        if (effVideoBitrateKbps <= 0 && options.targetSizeMB > 0.0 && options.durationSeconds > 0.0) {
            const double margin = (options.containerOverheadMargin > 0.0) ? options.containerOverheadMargin : 0.0;
            const double usableMB = options.targetSizeMB * (1.0 - margin);
            const double totalKbps = (usableMB * 8192.0) / options.durationSeconds;
            const int aKbps = (options.audioBitrateKbps > 0) ? options.audioBitrateKbps : 128;
            effVideoBitrateKbps = qMax(50, static_cast<int>(totalKbps - aKbps));
        }

        if (effVideoBitrateKbps > 0) {
            args << QStringLiteral("-b:v") << QStringLiteral("%1k").arg(effVideoBitrateKbps);
            args << QStringLiteral("-maxrate") << QStringLiteral("%1k").arg(static_cast<int>(effVideoBitrateKbps * 1.5));
            args << QStringLiteral("-bufsize") << QStringLiteral("%1k").arg(effVideoBitrateKbps * 2);
        } else if (options.crf >= 0) {
            args << QStringLiteral("-crf") << QString::number(options.crf);
        }

        if (options.twoPass && (options.pass == 1 || options.pass == 2)) {
            args << QStringLiteral("-pass") << QString::number(options.pass);
            if (!options.passLogFile.trimmed().isEmpty()) {
                args << QStringLiteral("-passlogfile") << options.passLogFile.trimmed();
            }
        }

        if (!options.preset.isEmpty()) {
            args << QStringLiteral("-preset") << options.preset;
        }
        if (!options.pixelFormat.isEmpty()) {
            args << QStringLiteral("-pix_fmt") << options.pixelFormat;
        }

        QStringList vFilters;
        if (!options.hdrTonemap.trimmed().isEmpty()) {
            vFilters << QStringLiteral("tonemap=%1").arg(options.hdrTonemap.trimmed());
        }
        if (!options.videoFilter.trimmed().isEmpty()) {
            vFilters << options.videoFilter.trimmed();
        }
        if (!vFilters.isEmpty()) {
            args << QStringLiteral("-vf") << vFilters.join(QLatin1Char(','));
        }

        if (!options.colorSpace.isEmpty()) {
            args << QStringLiteral("-colorspace") << options.colorSpace;
        }
        if (!options.colorPrimaries.isEmpty()) {
            args << QStringLiteral("-color_primaries") << options.colorPrimaries;
        }
        if (!options.colorTrc.isEmpty()) {
            args << QStringLiteral("-color_trc") << options.colorTrc;
        }

        // Advanced audio options (pass 1 of 2-pass omits audio)
        if (options.twoPass && options.pass == 1) {
            args << QStringLiteral("-an");
        } else {
            int audioBitrate = options.audioBitrateKbps;
            if (audioBitrate <= 0 && effVideoBitrateKbps > 0) {
                audioBitrate = 128; // Default 128k audio when targeting specific file size/bitrate
            }
            if (audioBitrate > 0) {
                args << QStringLiteral("-b:a") << QStringLiteral("%1k").arg(audioBitrate);
            }
            if (options.audioSampleRate > 0) {
                args << QStringLiteral("-ar") << QString::number(options.audioSampleRate);
            }
            if (options.audioChannels > 0) {
                args << QStringLiteral("-ac") << QString::number(options.audioChannels);
            }

            QStringList aFilters;
            if (options.audioNormalize) {
                aFilters << QStringLiteral("loudnorm");
            }
            if (!options.audioFilter.trimmed().isEmpty()) {
                aFilters << options.audioFilter.trimmed();
            }
            if (!aFilters.isEmpty()) {
                args << QStringLiteral("-af") << aFilters.join(QLatin1Char(','));
            }
        }
    }

    if (options.stripMetadata) {
        args << QStringLiteral("-map_metadata") << QStringLiteral("-1");
    }
    if (options.stripChapters) {
        args << QStringLiteral("-map_chapters") << QStringLiteral("-1");
    }

    if (options.twoPass && options.pass == 1) {
#ifdef Q_OS_WIN
        args << QStringLiteral("-f") << QStringLiteral("null") << QStringLiteral("NUL");
#else
        args << QStringLiteral("-f") << QStringLiteral("null") << QStringLiteral("/dev/null");
#endif
    } else {
        args << options.outputPath;
    }

    return args;
}

QStringList FFmpegCommandBuilder::buildConversion(
    const QString &inputPath,
    const QString &outputPath,
    const QString &videoCodec,
    const QString &audioCodec,
    int crf,
    bool copyStreams,
    bool stripMetadata
) const
{
    ConversionOptions opts;
    opts.inputPath = inputPath;
    opts.outputPath = outputPath;
    opts.videoCodec = videoCodec;
    opts.audioCodec = audioCodec;
    opts.crf = crf;
    opts.copyStreams = copyStreams;
    opts.stripMetadata = stripMetadata;
    return buildConversion(opts);
}

QPair<QStringList, QStringList> FFmpegCommandBuilder::buildConversionTwoPass(
    const ConversionOptions &options, const QString &passLogFile) const
{
    ConversionOptions pass1Opts = options;
    pass1Opts.twoPass = true;
    pass1Opts.pass = 1;
    if (!passLogFile.isEmpty()) {
        pass1Opts.passLogFile = passLogFile;
    }

    ConversionOptions pass2Opts = options;
    pass2Opts.twoPass = true;
    pass2Opts.pass = 2;
    if (!passLogFile.isEmpty()) {
        pass2Opts.passLogFile = passLogFile;
    }

    return qMakePair(buildConversion(pass1Opts), buildConversion(pass2Opts));
}

QStringList FFmpegCommandBuilder::buildConversionSample(
    const ConversionOptions &options, double startTimeSec, double durationSec) const
{
    ConversionOptions sampleOpts = options;
    sampleOpts.seekSeconds = qMax(0.0, startTimeSec);
    sampleOpts.durationLimit = durationSec > 0.0 ? durationSec : 5.0;

    const QFileInfo fi(options.outputPath);
    const QString sampleName = QStringLiteral("%1_sample.%2").arg(fi.completeBaseName(), fi.suffix());
    sampleOpts.outputPath = fi.dir().filePath(sampleName);

    return buildConversion(sampleOpts);
}

QStringList FFmpegCommandBuilder::buildImageOperation(
    const QString &inputPath,
    const QString &outputPath,
    int cropX, int cropY, int cropW, int cropH,
    int resizeW, int resizeH,
    int rotate,
    bool flipH, bool flipV,
    const QString &outputFormat,
    int quality
) const
{
    QStringList args;

    if (inputPath.isEmpty() || outputPath.isEmpty()) {
        return args;
    }

    args << QStringLiteral("-y");
    args << QStringLiteral("-i") << inputPath;

    const QStringList filters =
        adjustmentFilters(cropX, cropY, cropW, cropH, resizeW, resizeH, rotate,
                          flipH, flipV);

    if (!filters.isEmpty()) {
        args << QStringLiteral("-vf") << filters.join(QLatin1Char(','));
    }

    if (!outputFormat.isEmpty()) {
        const QString muxer = normalizeMuxerFormat(outputFormat);
        if (!muxer.isEmpty()) {
            args << QStringLiteral("-f") << muxer;
        }
    }

    if (quality >= 0) {
        args << QStringLiteral("-q:v") << QString::number(quality);
    }

    args << outputPath;

    return args;
}

QStringList FFmpegCommandBuilder::adjustmentFilters(int cropX, int cropY, int cropW,
                                                    int cropH, int resizeW, int resizeH,
                                                    int rotate, bool flipH, bool flipV)
{
    QStringList filters;

    if (cropW > 0 && cropH > 0) {
        filters << QStringLiteral("crop=%1:%2:%3:%4")
                      .arg(cropW)
                      .arg(cropH)
                      .arg(cropX)
                      .arg(cropY);
    }

    if (resizeW > 0 && resizeH > 0) {
        filters << QStringLiteral("scale=%1:%2")
                      .arg(resizeW)
                      .arg(resizeH);
    }

    if (rotate == 90) {
        filters << QStringLiteral("transpose=1");
    } else if (rotate == 180) {
        filters << QStringLiteral("transpose=1,transpose=1");
    } else if (rotate == 270) {
        filters << QStringLiteral("transpose=2");
    }

    if (flipH) {
        filters << QStringLiteral("hflip");
    }
    if (flipV) {
        filters << QStringLiteral("vflip");
    }

    return filters;
}

QStringList FFmpegCommandBuilder::buildVideoAdjust(
    const QString &inputPath,
    const QString &outputPath,
    int cropX, int cropY, int cropW, int cropH,
    int resizeW, int resizeH,
    int rotate,
    int crf
) const
{
    if (inputPath.isEmpty() || outputPath.isEmpty()) {
        return {};
    }

    const QStringList filters = adjustmentFilters(cropX, cropY, cropW, cropH,
                                                  resizeW, resizeH, rotate);
    if (filters.isEmpty()) {
        // No operation selected: nothing worth queueing.
        return {};
    }

    QStringList args;
    args << QStringLiteral("-y");
    args << QStringLiteral("-i") << inputPath;
    args << QStringLiteral("-vf") << filters.join(QLatin1Char(','));
    if (crf >= 0) {
        args << QStringLiteral("-crf") << QString::number(crf);
    }
    args << outputPath;
    return args;
}

QStringList FFmpegCommandBuilder::buildTrim(
    const QString &inputPath,
    const QString &outputPath,
    double startSec, double endSec,
    const QString &outputFormat,
    const QString &videoCodec,
    const QString &audioCodec,
    int crf,
    bool copyStreams
) const
{
    QStringList args;
    if (inputPath.isEmpty() || outputPath.isEmpty()) return args;

    args << QStringLiteral("-y");
    if (startSec > 0) {
        args << QStringLiteral("-ss") << formatTime(startSec);
    }
    args << QStringLiteral("-i") << inputPath;
    if (endSec > 0) {
        args << QStringLiteral("-to") << formatTime(endSec);
    }

    if (copyStreams) {
        args << QStringLiteral("-c") << QStringLiteral("copy");
    } else {
        if (!videoCodec.isEmpty()) args << QStringLiteral("-c:v") << videoCodec;
        if (!audioCodec.isEmpty()) args << QStringLiteral("-c:a") << audioCodec;
        if (crf >= 0) args << QStringLiteral("-crf") << QString::number(crf);
    }
    if (!outputFormat.isEmpty()) {
        const QString muxer = normalizeMuxerFormat(outputFormat);
        if (!muxer.isEmpty()) {
            args << QStringLiteral("-f") << muxer;
        }
    }
    args << outputPath;
    return args;
}

QStringList FFmpegCommandBuilder::buildExtractAudio(
    const QString &inputPath,
    const QString &outputPath,
    const QString &audioFormat,
    const QString &audioCodec,
    int bitrateKbps
) const
{
    QStringList args;
    if (inputPath.isEmpty() || outputPath.isEmpty()) return args;

    args << QStringLiteral("-y");
    args << QStringLiteral("-i") << inputPath;
    args << QStringLiteral("-vn");
    if (!audioCodec.isEmpty()) {
        args << QStringLiteral("-c:a") << audioCodec;
    }
    if (bitrateKbps > 0) {
        args << QStringLiteral("-b:a") << QStringLiteral("%1k").arg(bitrateKbps);
    }
    if (!audioFormat.isEmpty()) {
        const QString muxer = normalizeMuxerFormat(audioFormat);
        if (!muxer.isEmpty()) {
            args << QStringLiteral("-f") << muxer;
        }
    }
    args << outputPath;
    return args;
}

QStringList FFmpegCommandBuilder::buildAudioProcess(const AudioProcessOptions &options) const
{
    QStringList args;
    if (options.inputPath.isEmpty() || options.outputPath.isEmpty()) {
        return args;
    }

    args << QStringLiteral("-y");
    args << QStringLiteral("-i") << options.inputPath;
    args << QStringLiteral("-vn");

    if (!options.audioCodec.isEmpty()) {
        args << QStringLiteral("-c:a") << options.audioCodec;
    }
    if (options.bitrateKbps > 0) {
        args << QStringLiteral("-b:a") << QStringLiteral("%1k").arg(options.bitrateKbps);
    }
    if (options.sampleRate > 0) {
        args << QStringLiteral("-ar") << QString::number(options.sampleRate);
    }
    if (options.channels > 0) {
        args << QStringLiteral("-ac") << QString::number(options.channels);
    }

    QStringList filters;
    if (options.denoise) {
        const double nr = qBound(6.0, options.denoiseNoiseReduction, 25.0);
        filters << QStringLiteral("afftdn=nr=%1").arg(nr);
    }
    if (options.highpassFreq > 0) {
        filters << QStringLiteral("highpass=f=%1").arg(options.highpassFreq);
    }
    if (options.lowpassFreq > 0) {
        filters << QStringLiteral("lowpass=f=%1").arg(options.lowpassFreq);
    }
    if (options.removeSilence) {
        filters << QStringLiteral("silenceremove=start_periods=1:start_duration=0.5:start_threshold=-50dB:detection=peak");
    }
    if (qAbs(options.volumeFactor - 1.0) > 0.01) {
        filters << QStringLiteral("volume=%1").arg(options.volumeFactor);
    }
    if (options.loudnorm) {
        filters << QStringLiteral("loudnorm=I=-16:TP=-1.5:LRA=11");
    }
    if (qAbs(options.tempo - 1.0) > 0.01) {
        const double t = qBound(0.5, options.tempo, 2.0);
        filters << QStringLiteral("atempo=%1").arg(t);
    }
    if (options.reverse) {
        filters << QStringLiteral("areverse");
    }

    if (!filters.isEmpty()) {
        args << QStringLiteral("-af") << filters.join(QLatin1Char(','));
    }

    if (!options.format.isEmpty()) {
        const QString muxer = normalizeMuxerFormat(options.format);
        if (!muxer.isEmpty()) {
            args << QStringLiteral("-f") << muxer;
        }
    }

    args << options.outputPath;
    return args;
}

QStringList FFmpegCommandBuilder::buildMute(
    const QString &inputPath,
    const QString &outputPath,
    const QString &outputFormat,
    const QString &videoCodec,
    int crf,
    bool copyStreams
) const
{
    QStringList args;
    if (inputPath.isEmpty() || outputPath.isEmpty()) return args;

    args << QStringLiteral("-y");
    args << QStringLiteral("-i") << inputPath;
    args << QStringLiteral("-an");
    if (copyStreams) {
        args << QStringLiteral("-c:v") << QStringLiteral("copy");
    } else {
        if (!videoCodec.isEmpty()) args << QStringLiteral("-c:v") << videoCodec;
        if (crf >= 0) args << QStringLiteral("-crf") << QString::number(crf);
    }
    if (!outputFormat.isEmpty()) {
        const QString muxer = normalizeMuxerFormat(outputFormat);
        if (!muxer.isEmpty()) {
            args << QStringLiteral("-f") << muxer;
        }
    }
    args << outputPath;
    return args;
}

QStringList FFmpegCommandBuilder::buildScreenshot(
    const QString &inputPath,
    const QString &outputPath,
    double timeSec,
    const QString &imageFormat
) const
{
    QStringList args;
    if (inputPath.isEmpty() || outputPath.isEmpty()) return args;

    args << QStringLiteral("-y");
    if (timeSec > 0) {
        args << QStringLiteral("-ss") << formatTime(timeSec);
    }
    args << QStringLiteral("-i") << inputPath;
    args << QStringLiteral("-frames:v") << QStringLiteral("1");
    if (!imageFormat.isEmpty()) {
        args << QStringLiteral("-f") << imageFormat;
    }
    args << outputPath;
    return args;
}

QStringList FFmpegCommandBuilder::buildGif(
    const QString &inputPath,
    const QString &outputPath,
    int fps, int width,
    double startSec, double endSec
) const
{
    QStringList args;
    if (inputPath.isEmpty() || outputPath.isEmpty()) return args;

    args << QStringLiteral("-y");
    if (startSec > 0) {
        args << QStringLiteral("-ss") << formatTime(startSec);
    }
    args << QStringLiteral("-i") << inputPath;
    if (endSec > 0) {
        args << QStringLiteral("-to") << formatTime(endSec);
    }

    QString filter = QStringLiteral("fps=%1").arg(fps > 0 ? fps : 12);
    if (width > 0) {
        filter += QStringLiteral(",scale=%1:-1:flags=lanczos").arg(width);
    }
    args << QStringLiteral("-vf") << filter;
    args << QStringLiteral("-loop") << QStringLiteral("0");
    args << outputPath;
    return args;
}

QStringList FFmpegCommandBuilder::buildVideoJoin(
    const QStringList &inputPaths,
    const QString &outputPath
) const
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

QStringList FFmpegCommandBuilder::buildGridJoin(
    const QStringList &inputPaths,
    const QString &outputPath,
    int cols,
    int spacing,
    const QString &backgroundColor,
    int padding
) const
{
    QStringList args;
    if (inputPaths.isEmpty() || outputPath.isEmpty()) return args;
    if (cols < 1) cols = 1;

    args << QStringLiteral("-y");
    for (const QString &file : inputPaths) {
        args << QStringLiteral("-i") << file;
    }

    QString filter;
    if (spacing > 0) {
        for (int i = 0; i < inputPaths.size(); ++i) {
            filter += QStringLiteral("[%1:v]pad=iw+%2:ih+%2:%2:%2:color=%3[p%1];")
                          .arg(i)
                          .arg(spacing)
                          .arg(backgroundColor.isEmpty() ? QStringLiteral("black") : backgroundColor);
        }
    }
    for (int i = 0; i < inputPaths.size(); ++i) {
        filter += (spacing > 0 ? QStringLiteral("[p%1]").arg(i)
                               : QStringLiteral("[%1:v]").arg(i));
    }
    filter += QStringLiteral("xstack=inputs=%1:layout=").arg(inputPaths.size());

    QStringList layout;
    for (int i = 0; i < inputPaths.size(); ++i) {
        const int r = i / cols;
        const int c = i % cols;
        const QString x = c == 0 ? QStringLiteral("0") : QStringLiteral("%1*w0").arg(c);
        const QString y = r == 0 ? QStringLiteral("0") : QStringLiteral("%1*h0").arg(r);
        layout << x + QLatin1Char('_') + y;
    }
    filter += layout.join(QLatin1Char('|'));
    filter += QStringLiteral("[outv]");

    QString mapTarget = QStringLiteral("[outv]");
    if (padding > 0) {
        filter += QStringLiteral(";[outv]pad=iw+%1:ih+%1:%2:%2:color=%3[outp]")
                      .arg(padding * 2)
                      .arg(padding)
                      .arg(backgroundColor);
        mapTarget = QStringLiteral("[outp]");
    }

    args << QStringLiteral("-filter_complex") << filter;
    args << QStringLiteral("-map") << mapTarget;
    args << outputPath;
    return args;
}

QStringList FFmpegCommandBuilder::buildImageSequenceToVideo(
    const QString &inputPattern,
    const QString &outputPath,
    int fps
) const
{
    QStringList args;
    if (inputPattern.isEmpty() || outputPath.isEmpty()) return args;

    args << QStringLiteral("-y");
    args << QStringLiteral("-framerate") << QString::number(fps > 0 ? fps : 25);
    args << QStringLiteral("-i") << inputPattern;
    args << QStringLiteral("-pix_fmt") << QStringLiteral("yuv420p");
    args << outputPath;
    return args;
}

QStringList FFmpegCommandBuilder::buildVideoToImageSequence(
    const QString &inputPath,
    const QString &outputPattern,
    double fps
) const
{
    QStringList args;
    if (inputPath.isEmpty() || outputPattern.isEmpty()) return args;

    args << QStringLiteral("-y");
    args << QStringLiteral("-i") << inputPath;
    if (fps > 0) {
        args << QStringLiteral("-vf") << QStringLiteral("fps=%1").arg(fps);
    }
    args << outputPattern;
    return args;
}

QStringList FFmpegCommandBuilder::buildSubtitleMux(
    const QString &inputPath,
    const QString &subtitlePath,
    const QString &outputPath,
    const QString &subtitleCodec
) const
{
    if (inputPath.isEmpty() || subtitlePath.isEmpty() || outputPath.isEmpty()) {
        return {};
    }

    QStringList args;
    args << QStringLiteral("-y");
    args << QStringLiteral("-i") << inputPath;
    args << QStringLiteral("-i") << subtitlePath;
    // Keep every stream of the source plus the new subtitle stream.
    args << QStringLiteral("-map") << QStringLiteral("0");
    args << QStringLiteral("-map") << QStringLiteral("1");
    args << QStringLiteral("-c") << QStringLiteral("copy");
    if (!subtitleCodec.isEmpty()) {
        args << QStringLiteral("-c:s") << subtitleCodec;
    }
    args << outputPath;
    return args;
}

QStringList FFmpegCommandBuilder::buildSubtitleBurnin(
    const QString &inputPath,
    const QString &subtitlePath,
    const QString &outputPath,
    int crf
) const
{
    if (inputPath.isEmpty() || subtitlePath.isEmpty() || outputPath.isEmpty()) {
        return {};
    }

    QStringList args;
    args << QStringLiteral("-y");
    args << QStringLiteral("-i") << inputPath;
    args << QStringLiteral("-i") << subtitlePath;
    args << QStringLiteral("-filter_complex")
         << QStringLiteral("[0:v][1:s]subtitles=filename=%1[v]")
                .arg(escapeFilterPath(subtitlePath));
    args << QStringLiteral("-map") << QStringLiteral("[v]");
    args << QStringLiteral("-map") << QStringLiteral("0:a?");
    args << QStringLiteral("-c:a") << QStringLiteral("copy");
    if (crf >= 0) {
        args << QStringLiteral("-crf") << QString::number(crf);
    }
    args << outputPath;
    return args;
}

QStringList FFmpegCommandBuilder::buildWatermark(
    const QString &inputPath,
    const QString &watermarkPath,
    const QString &outputPath,
    const QString &position,
    int opacityPercent,
    const QString &outputFormat
) const
{
    QStringList args;
    if (inputPath.isEmpty() || watermarkPath.isEmpty() || outputPath.isEmpty()) return args;

    const double opacity = qBound(0, opacityPercent, 100) / 100.0;

    args << QStringLiteral("-y");
    args << QStringLiteral("-i") << inputPath;
    args << QStringLiteral("-i") << watermarkPath;

    QString overlay;
    if (position == QLatin1String("top-left")) {
        overlay = QStringLiteral("10:10");
    } else if (position == QLatin1String("top-right")) {
        overlay = QStringLiteral("main_w-overlay_w-10:10");
    } else if (position == QLatin1String("bottom-left")) {
        overlay = QStringLiteral("10:main_h-overlay_h-10");
    } else if (position == QLatin1String("center")) {
        overlay = QStringLiteral("(main_w-overlay_w)/2:(main_h-overlay_h)/2");
    } else {
        overlay = QStringLiteral("main_w-overlay_w-10:main_h-overlay_h-10");
    }

    const QString filter = QStringLiteral("[1:v]format=rgba,colorchannelmixer=aa=%1[wm];[0:v][wm]overlay=%2")
                               .arg(opacity)
                               .arg(overlay);

    args << QStringLiteral("-filter_complex") << filter;
    args << QStringLiteral("-map") << QStringLiteral("0:a?");
    if (!outputFormat.isEmpty()) {
        const QString muxer = normalizeMuxerFormat(outputFormat);
        if (!muxer.isEmpty()) {
            args << QStringLiteral("-f") << muxer;
        }
    }
    args << outputPath;
    return args;
}

QStringList FFmpegCommandBuilder::buildPip(
    const QString &mainPath,
    const QString &pipPath,
    const QString &outputPath,
    int pipWidth,
    int pipHeight,
    const QString &position,
    int margin,
    int crf
) const
{
    if (mainPath.isEmpty() || pipPath.isEmpty() || outputPath.isEmpty()) {
        return {};
    }

    QString scaleStr;
    if (pipWidth > 0 && pipHeight > 0) {
        scaleStr = QStringLiteral("%1:%2").arg(pipWidth).arg(pipHeight);
    } else if (pipWidth > 0) {
        scaleStr = QStringLiteral("%1:-1").arg(pipWidth);
    } else if (pipHeight > 0) {
        scaleStr = QStringLiteral("-1:%1").arg(pipHeight);
    } else {
        scaleStr = QStringLiteral("iw/3:-1");
    }

    const int m = qMax(0, margin);
    QString overlay;
    if (position == QLatin1String("top-left")) {
        overlay = QStringLiteral("%1:%1").arg(m);
    } else if (position == QLatin1String("top-right")) {
        overlay = QStringLiteral("main_w-overlay_w-%1:%1").arg(m);
    } else if (position == QLatin1String("bottom-left")) {
        overlay = QStringLiteral("%1:main_h-overlay_h-%1").arg(m);
    } else if (position == QLatin1String("center")) {
        overlay = QStringLiteral("(main_w-overlay_w)/2:(main_h-overlay_h)/2");
    } else {
        overlay = QStringLiteral("main_w-overlay_w-%1:main_h-overlay_h-%1").arg(m);
    }

    const QString filter = QStringLiteral("[1:v]scale=%1[pip];[0:v][pip]overlay=%2")
                               .arg(scaleStr, overlay);

    QStringList args;
    args << QStringLiteral("-y");
    args << QStringLiteral("-i") << mainPath;
    args << QStringLiteral("-i") << pipPath;
    args << QStringLiteral("-filter_complex") << filter;
    args << QStringLiteral("-map") << QStringLiteral("0:a?");
    if (crf >= 0) {
        args << QStringLiteral("-crf") << QString::number(crf);
    }
    args << outputPath;
    return args;
}

QString FFmpegCommandBuilder::escapeFilterPath(const QString &path)
{
    QString result = path;
    // Forward slashes work in ffmpeg on Windows and dodge the backslash
    // escaping maze; the colon still has to be escaped for the option parser.
    result.replace(QLatin1Char('\\'), QLatin1Char('/'));
    result.replace(QLatin1Char(':'), QStringLiteral("\\:"));
    result.replace(QLatin1Char('\''), QStringLiteral("\\'"));
    return QStringLiteral("'") + result + QStringLiteral("'");
}

QString FFmpegCommandBuilder::formatTime(double seconds)
{
    const int total = static_cast<int>(seconds);
    const int h = total / 3600;
    const int m = (total % 3600) / 60;
    const int s = total % 60;
    return QStringLiteral("%1:%2:%3")
        .arg(h, 2, 10, QLatin1Char('0'))
        .arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0'));
}

QString FFmpegCommandBuilder::escapeFilterValue(const QString &value)
{
    QString result = value;
    result.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    result.replace(QLatin1Char('\''), QStringLiteral("\\'"));
    result.replace(QLatin1Char(';'), QStringLiteral("\\;"));
    return result;
}

QString FFmpegCommandBuilder::normalizeMuxerFormat(const QString &format)
{
    const QString f = format.trimmed().toLower();
    if (f.isEmpty()) {
        return QString();
    }
    if (f == QLatin1String("aac")) {
        return QStringLiteral("adts");
    }
    if (f == QLatin1String("mkv")) {
        return QStringLiteral("matroska");
    }
    if (f == QLatin1String("m4a")) {
        return QStringLiteral("ipod");
    }
    return f;
}

QStringList FFmpegCommandBuilder::buildCropdetect(const QString &inputPath, double seekSec, int frames) const
{
    QStringList args;
    if (inputPath.isEmpty()) {
        return args;
    }
    args << QStringLiteral("-y");
    if (seekSec > 0.0) {
        args << QStringLiteral("-ss") << formatTime(seekSec);
    }
    args << QStringLiteral("-i") << inputPath;
    args << QStringLiteral("-vf") << QStringLiteral("cropdetect=24:16:0");
    args << QStringLiteral("-vframes") << QString::number(frames > 0 ? frames : 20);
    args << QStringLiteral("-f") << QStringLiteral("null") << QStringLiteral("-");
    return args;
}

FFmpegCommandBuilder::CropArea FFmpegCommandBuilder::parseCropdetectOutput(const QString &ffmpegOutput)
{
    CropArea result;
    // Format: crop=W:H:X:Y
    static const QRegularExpression re(QStringLiteral(R"(crop=(\d+):(\d+):(\d+):(\d+))"));
    auto it = re.globalMatch(ffmpegOutput);
    while (it.hasNext()) {
        const auto match = it.next();
        result.w = match.captured(1).toInt();
        result.h = match.captured(2).toInt();
        result.x = match.captured(3).toInt();
        result.y = match.captured(4).toInt();
    }
    return result;
}
