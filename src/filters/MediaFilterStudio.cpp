#include "MediaFilterStudio.h"

#include <cmath>

bool ColorGradingOptions::isModified() const
{
    return preset != ColorPreset::Default
        || !lutFilePath.isEmpty()
        || std::abs(brightness) > 0.001
        || std::abs(contrast - 1.0) > 0.001
        || std::abs(saturation - 1.0) > 0.001
        || std::abs(gamma - 1.0) > 0.001
        || std::abs(temperature) > 0.001
        || std::abs(tint) > 0.001
        || enableDenoise
        || enableSharpen
        || enableVignette
        || enableDeinterlace;
}

void ColorGradingOptions::reset()
{
    preset = ColorPreset::Default;
    lutFilePath.clear();
    brightness = 0.0;
    contrast = 1.0;
    saturation = 1.0;
    gamma = 1.0;
    temperature = 0.0;
    tint = 0.0;
    enableDenoise = false;
    denoiseStrength = 4.0;
    enableSharpen = false;
    sharpenAmount = 0.8;
    enableVignette = false;
    vignetteStrength = 0.5;
    enableDeinterlace = false;
}

void ColorGradingOptions::applyPreset(ColorPreset p)
{
    reset();
    preset = p;
    switch (p) {
    case ColorPreset::Default:
    case ColorPreset::Custom:
        break;
    case ColorPreset::CinematicTealOrange:
        contrast = 1.15;
        saturation = 1.10;
        temperature = 0.22;
        tint = -0.05;
        enableSharpen = true;
        sharpenAmount = 0.6;
        enableVignette = true;
        vignetteStrength = 0.4;
        break;
    case ColorPreset::WarmVintage:
        contrast = 0.95;
        saturation = 0.85;
        brightness = 0.02;
        gamma = 1.10;
        temperature = 0.45;
        enableVignette = true;
        vignetteStrength = 0.6;
        break;
    case ColorPreset::CoolNight:
        contrast = 1.20;
        saturation = 1.15;
        gamma = 0.95;
        temperature = -0.40;
        tint = 0.08;
        break;
    case ColorPreset::BwHighContrast:
        saturation = 0.0;
        contrast = 1.40;
        brightness = -0.04;
        gamma = 0.95;
        enableSharpen = true;
        sharpenAmount = 0.8;
        break;
    case ColorPreset::LowlightBoost:
        brightness = 0.08;
        gamma = 1.30;
        contrast = 1.05;
        saturation = 1.05;
        enableDenoise = true;
        denoiseStrength = 4.5;
        break;
    case ColorPreset::VividPop:
        saturation = 1.35;
        contrast = 1.12;
        brightness = 0.02;
        enableSharpen = true;
        sharpenAmount = 0.7;
        break;
    }
}

bool AudioFxOptions::isModified() const
{
    return enableLoudnorm
        || enableDenoise
        || enableHighpass
        || enableLowpass
        || std::abs(volumeDb) > 0.01;
}

void AudioFxOptions::reset()
{
    enableLoudnorm = false;
    loudnessStandard = LoudnessStandard::YouTube;
    targetLufs = -14.0;
    enableDenoise = false;
    noiseReductionDb = 12.0;
    enableHighpass = false;
    highpassFreq = 80;
    enableLowpass = false;
    lowpassFreq = 12000;
    volumeDb = 0.0;
}

void AudioFxOptions::applyStandard(LoudnessStandard s)
{
    loudnessStandard = s;
    enableLoudnorm = true;
    switch (s) {
    case LoudnessStandard::YouTube:
        targetLufs = -14.0;
        break;
    case LoudnessStandard::ApplePodcast:
        targetLufs = -16.0;
        break;
    case LoudnessStandard::EbuR128:
        targetLufs = -23.0;
        break;
    case LoudnessStandard::AtscA85:
        targetLufs = -24.0;
        break;
    case LoudnessStandard::Custom:
        break;
    }
}

MediaFilterStudio::MediaFilterStudio(QObject *parent)
    : QObject(parent)
{
}

QString MediaFilterStudio::compileVideoFilters(const ColorGradingOptions &opts)
{
    QStringList filters;

    // 1. Deinterlace
    if (opts.enableDeinterlace) {
        filters << QStringLiteral("yadif=0:-1:0");
    }

    // 2. 3D LUT (.cube / .3dl)
    if (!opts.lutFilePath.isEmpty()) {
        QString cleanPath = opts.lutFilePath;
        cleanPath.replace(QLatin1Char('\\'), QLatin1Char('/'));
        cleanPath.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
        filters << QStringLiteral("lut3d=file='%1':interp=tetrahedral").arg(cleanPath);
    }

    // 3. Color Temperature & Tint (via colorbalance)
    if (std::abs(opts.temperature) > 0.005 || std::abs(opts.tint) > 0.005) {
        // Temperature > 0 (warm): boost red mid/highlights, cut blue
        // Temperature < 0 (cool): boost blue mid/highlights, cut red
        const double t = opts.temperature;
        const double tint = opts.tint;

        const double rm = std::clamp(t * 0.12 + tint * 0.06, -0.9, 0.9);
        const double gm = std::clamp(-tint * 0.12, -0.9, 0.9);
        const double bm = std::clamp(-t * 0.14 + tint * 0.06, -0.9, 0.9);

        const double rh = std::clamp(t * 0.15 + tint * 0.05, -0.9, 0.9);
        const double gh = std::clamp(-tint * 0.10, -0.9, 0.9);
        const double bh = std::clamp(-t * 0.18 + tint * 0.05, -0.9, 0.9);

        filters << QStringLiteral("colorbalance=rm=%1:gm=%2:bm=%3:rh=%4:gh=%5:bh=%6")
                       .arg(rm, 0, 'f', 2)
                       .arg(gm, 0, 'f', 2)
                       .arg(bm, 0, 'f', 2)
                       .arg(rh, 0, 'f', 2)
                       .arg(gh, 0, 'f', 2)
                       .arg(bh, 0, 'f', 2);
    }

    // 4. Basic Adjustments (eq)
    if (std::abs(opts.brightness) > 0.005
        || std::abs(opts.contrast - 1.0) > 0.005
        || std::abs(opts.saturation - 1.0) > 0.005
        || std::abs(opts.gamma - 1.0) > 0.005) {
        filters << QStringLiteral("eq=brightness=%1:contrast=%2:saturation=%3:gamma=%4")
                       .arg(opts.brightness, 0, 'f', 2)
                       .arg(opts.contrast, 0, 'f', 2)
                       .arg(opts.saturation, 0, 'f', 2)
                       .arg(opts.gamma, 0, 'f', 2);
    }

    // 5. Denoise (hqdn3d)
    if (opts.enableDenoise) {
        const double s = std::clamp(opts.denoiseStrength, 0.5, 10.0);
        filters << QStringLiteral("hqdn3d=%1:%2:%3:%4")
                       .arg(s, 0, 'f', 1)
                       .arg(s * 0.75, 0, 'f', 1)
                       .arg(s * 1.50, 0, 'f', 1)
                       .arg(s * 1.12, 0, 'f', 1);
    }

    // 6. Sharpen (unsharp)
    if (opts.enableSharpen) {
        const double a = std::clamp(opts.sharpenAmount, 0.1, 2.5);
        filters << QStringLiteral("unsharp=5:5:%1:5:5:0.0").arg(a, 0, 'f', 2);
    }

    // 7. Vignette
    if (opts.enableVignette) {
        const double v = std::clamp(opts.vignetteStrength, 0.1, 1.0);
        filters << QStringLiteral("vignette=PI*%1").arg(v * 0.35, 0, 'f', 2);
    }

    return filters.join(QLatin1Char(','));
}

QString MediaFilterStudio::compileAudioFilters(const AudioFxOptions &opts)
{
    QStringList filters;

    // 1. High-pass (remove rumble)
    if (opts.enableHighpass && opts.highpassFreq > 0) {
        filters << QStringLiteral("highpass=f=%1").arg(opts.highpassFreq);
    }

    // 2. Low-pass (remove hiss)
    if (opts.enableLowpass && opts.lowpassFreq > 0) {
        filters << QStringLiteral("lowpass=f=%1").arg(opts.lowpassFreq);
    }

    // 3. FFT Denoise (afftdn)
    if (opts.enableDenoise) {
        const double nr = std::clamp(opts.noiseReductionDb, 6.0, 25.0);
        filters << QStringLiteral("afftdn=nr=%1:nf=-50:tn=1").arg(nr, 0, 'f', 1);
    }

    // 4. Volume Gain (volume)
    if (std::abs(opts.volumeDb) > 0.05) {
        filters << QStringLiteral("volume=%1dB").arg(opts.volumeDb, 0, 'f', 1);
    }

    // 5. Loudness Normalization (loudnorm)
    if (opts.enableLoudnorm) {
        filters << QStringLiteral("loudnorm=I=%1:TP=-1.5:LRA=11").arg(opts.targetLufs, 0, 'f', 1);
    }

    return filters.join(QLatin1Char(','));
}

QStringList MediaFilterStudio::buildSamplePreviewCommand(const QString &inputPath, const QString &outputPath,
                                                         const ColorGradingOptions &videoOpts,
                                                         const AudioFxOptions &audioOpts,
                                                         double startSec, double durationSec)
{
    QStringList args;
    args << QStringLiteral("-y");
    if (startSec > 0.0) {
        args << QStringLiteral("-ss") << QString::number(startSec, 'f', 2);
    }
    if (durationSec > 0.0) {
        args << QStringLiteral("-t") << QString::number(durationSec, 'f', 2);
    }
    args << QStringLiteral("-i") << inputPath;

    const QString vf = compileVideoFilters(videoOpts);
    if (!vf.isEmpty()) {
        args << QStringLiteral("-vf") << vf;
    }

    const QString af = compileAudioFilters(audioOpts);
    if (!af.isEmpty()) {
        args << QStringLiteral("-af") << af;
    }

    args << QStringLiteral("-c:v") << QStringLiteral("libx264")
         << QStringLiteral("-preset") << QStringLiteral("ultrafast")
         << QStringLiteral("-crf") << QStringLiteral("18")
         << QStringLiteral("-c:a") << QStringLiteral("aac")
         << outputPath;

    return args;
}

QStringList MediaFilterStudio::buildFilterExportCommand(const QString &inputPath, const QString &outputPath,
                                                        const ColorGradingOptions &videoOpts,
                                                        const AudioFxOptions &audioOpts,
                                                        const QString &videoCodec,
                                                        const QString &audioCodec,
                                                        int crf)
{
    QStringList args;
    args << QStringLiteral("-y") << QStringLiteral("-i") << inputPath;

    const QString vf = compileVideoFilters(videoOpts);
    if (!vf.isEmpty()) {
        args << QStringLiteral("-vf") << vf;
    }

    const QString af = compileAudioFilters(audioOpts);
    if (!af.isEmpty()) {
        args << QStringLiteral("-af") << af;
    }

    if (!videoCodec.isEmpty()) {
        args << QStringLiteral("-c:v") << videoCodec;
    }
    if (crf >= 0) {
        args << QStringLiteral("-crf") << QString::number(crf);
    }
    if (!audioCodec.isEmpty()) {
        args << QStringLiteral("-c:a") << audioCodec;
    }

    args << outputPath;
    return args;
}

QString MediaFilterStudio::presetName(ColorPreset preset)
{
    switch (preset) {
    case ColorPreset::Default: return QStringLiteral("filter.preset.default");
    case ColorPreset::CinematicTealOrange: return QStringLiteral("filter.preset.cinematic");
    case ColorPreset::WarmVintage: return QStringLiteral("filter.preset.warmVintage");
    case ColorPreset::CoolNight: return QStringLiteral("filter.preset.coolNight");
    case ColorPreset::BwHighContrast: return QStringLiteral("filter.preset.bw");
    case ColorPreset::LowlightBoost: return QStringLiteral("filter.preset.lowlight");
    case ColorPreset::VividPop: return QStringLiteral("filter.preset.vivid");
    case ColorPreset::Custom: return QStringLiteral("filter.preset.custom");
    }
    return QString();
}

QString MediaFilterStudio::standardName(LoudnessStandard standard)
{
    switch (standard) {
    case LoudnessStandard::YouTube: return QStringLiteral("filter.audio.standard.youtube");
    case LoudnessStandard::ApplePodcast: return QStringLiteral("filter.audio.standard.podcast");
    case LoudnessStandard::EbuR128: return QStringLiteral("filter.audio.standard.ebur128");
    case LoudnessStandard::AtscA85: return QStringLiteral("filter.audio.standard.atsc");
    case LoudnessStandard::Custom: return QStringLiteral("filter.audio.standard.custom");
    }
    return QString();
}
