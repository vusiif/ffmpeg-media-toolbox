#include "MultiTrackCompositor.h"

#include <QtGlobal>
#include <QFileInfo>

namespace Compositor {

bool CompositorConfig::isModified() const
{
    if (layout != LayoutMode::PictureInPicture) return true;
    if (position != PipPosition::BottomRight) return true;
    if (qAbs(scalePercent - 0.25) > 0.001) return true;
    if (customWidth > 0 || customHeight > 0) return true;
    if (margin != 16) return true;
    if (drawBorder) return true;
    if (hasTimeRange) return true;
    if (audioPolicy != AudioMixPolicy::MixDual) return true;
    if (qAbs(mainVolume - 1.0) > 0.01) return true;
    if (qAbs(pipVolume - 0.8) > 0.01) return true;
    return false;
}

void CompositorConfig::reset()
{
    *this = CompositorConfig();
}

QString MultiTrackCompositor::overlayPositionExpression(PipPosition pos, int margin, int customX, int customY)
{
    const int m = qMax(0, margin);
    switch (pos) {
    case PipPosition::BottomLeft:
        return QStringLiteral("%1:main_h-overlay_h-%1").arg(m);
    case PipPosition::TopRight:
        return QStringLiteral("main_w-overlay_w-%1:%1").arg(m);
    case PipPosition::TopLeft:
        return QStringLiteral("%1:%1").arg(m);
    case PipPosition::Center:
        return QStringLiteral("(main_w-overlay_w)/2:(main_h-overlay_h)/2");
    case PipPosition::CustomCoordinate:
        return QStringLiteral("%1:%2").arg(qMax(0, customX)).arg(qMax(0, customY));
    case PipPosition::BottomRight:
    default:
        return QStringLiteral("main_w-overlay_w-%1:main_h-overlay_h-%1").arg(m);
    }
}

QString MultiTrackCompositor::buildVideoFilter(const CompositorConfig &config)
{
    if (config.layout == LayoutMode::SideBySide) {
        const int totalW = qMax(320, (config.splitWidth / 2) * 2);
        const int totalH = qMax(240, (config.splitHeight / 2) * 2);
        const double ratio = qBound(0.1, config.splitRatio, 0.9);
        const int w0 = qMax(160, (static_cast<int>(totalW * ratio) / 2) * 2);
        const int w1 = totalW - w0;

        return QStringLiteral("[0:v]scale=%1:%2:force_original_aspect_ratio=decrease,pad=%1:%2:(ow-iw)/2:(oh-ih)/2[v0];"
                              "[1:v]scale=%3:%2:force_original_aspect_ratio=decrease,pad=%3:%2:(ow-iw)/2:(oh-ih)/2[v1];"
                              "[v0][v1]hstack=inputs=2[outv]")
            .arg(w0).arg(totalH).arg(w1);
    }

    if (config.layout == LayoutMode::TopBottom) {
        const int totalW = qMax(320, (config.splitWidth / 2) * 2);
        const int totalH = qMax(240, (config.splitHeight / 2) * 2);
        const double ratio = qBound(0.1, config.splitRatio, 0.9);
        const int h0 = qMax(120, (static_cast<int>(totalH * ratio) / 2) * 2);
        const int h1 = totalH - h0;

        return QStringLiteral("[0:v]scale=%1:%2:force_original_aspect_ratio=decrease,pad=%1:%2:(ow-iw)/2:(oh-ih)/2[v0];"
                              "[1:v]scale=%1:%3:force_original_aspect_ratio=decrease,pad=%1:%3:(ow-iw)/2:(oh-ih)/2[v1];"
                              "[v0][v1]vstack=inputs=2[outv]")
            .arg(totalW).arg(h0).arg(h1);
    }

    // Default: PictureInPicture
    QString filter;
    QString baseLabel = QStringLiteral("[0:v]");
    QString overlayInput = QStringLiteral("[pip_scaled]");

    if (config.customWidth > 0 && config.customHeight > 0) {
        filter += QStringLiteral("[1:v]scale=%1:%2[pip_scaled];")
                      .arg(config.customWidth)
                      .arg(config.customHeight);
    } else if (config.customWidth > 0) {
        filter += QStringLiteral("[1:v]scale=%1:-2[pip_scaled];")
                      .arg(config.customWidth);
    } else if (config.customHeight > 0) {
        filter += QStringLiteral("[1:v]scale=-2:%1[pip_scaled];")
                      .arg(config.customHeight);
    } else {
        const double scaleRatio = qBound(0.05, config.scalePercent, 0.95);
        filter += QStringLiteral("[1:v][0:v]scale2ref=w=trunc(main_w*%1/2)*2:h=-2[pip_scaled][base_ref];")
                      .arg(scaleRatio, 0, 'f', 3);
        baseLabel = QStringLiteral("[base_ref]");
    }

    if (config.drawBorder && config.borderWidth > 0) {
        const int bw = qBound(1, config.borderWidth, 20);
        const QString color = config.borderColor.isEmpty() ? QStringLiteral("white") : config.borderColor;
        filter += QStringLiteral("[pip_scaled]drawbox=x=0:y=0:w=iw:h=ih:color=%1:t=%2[pip_bordered];")
                      .arg(color).arg(bw);
        overlayInput = QStringLiteral("[pip_bordered]");
    }

    QString overlayOpts = overlayPositionExpression(config.position, config.margin, config.customX, config.customY);

    if (config.hasTimeRange && config.startTimeSec >= 0) {
        if (config.durationSec > 0) {
            overlayOpts += QStringLiteral(":enable='between(t,%1,%2)'")
                               .arg(config.startTimeSec, 0, 'f', 2)
                               .arg(config.startTimeSec + config.durationSec, 0, 'f', 2);
        } else {
            overlayOpts += QStringLiteral(":enable='gte(t,%1)'")
                               .arg(config.startTimeSec, 0, 'f', 2);
        }
    }

    filter += QStringLiteral("%1%2overlay=%3[outv]").arg(baseLabel, overlayInput, overlayOpts);
    return filter;
}

QString MultiTrackCompositor::buildAudioFilter(const CompositorConfig &config)
{
    if (config.audioPolicy == AudioMixPolicy::MainOnly ||
        config.audioPolicy == AudioMixPolicy::PipOnly ||
        config.audioPolicy == AudioMixPolicy::MuteAll) {
        return QString();
    }

    double v0 = config.mainVolume;
    double v1 = config.pipVolume;

    if (config.audioPolicy == AudioMixPolicy::Ducking) {
        // Lower main audio to 25% of its volume to prioritize commentary/speech from secondary track
        v0 = config.mainVolume * 0.25;
        v1 = config.pipVolume * 1.0;
    }

    v0 = qBound(0.0, v0, 3.0);
    v1 = qBound(0.0, v1, 3.0);

    return QStringLiteral("[0:a]volume=%1[a0];[1:a]volume=%2[a1];[a0][a1]amix=inputs=2:duration=first:dropout_transition=2[outa]")
        .arg(v0, 0, 'f', 2)
        .arg(v1, 0, 'f', 2);
}

QString MultiTrackCompositor::buildFilterComplex(const CompositorConfig &config)
{
    const QString vf = buildVideoFilter(config);
    const QString af = buildAudioFilter(config);

    if (af.isEmpty()) {
        return vf;
    }
    return QStringLiteral("%1;%2").arg(vf, af);
}

QStringList MultiTrackCompositor::buildCommand(
    const QString &mainInput,
    const QString &pipInput,
    const QString &outputPath,
    const CompositorConfig &config)
{
    if (mainInput.isEmpty() || pipInput.isEmpty() || outputPath.isEmpty()) {
        return {};
    }

    QStringList args;
    args << QStringLiteral("-y");
    args << QStringLiteral("-i") << mainInput;
    args << QStringLiteral("-i") << pipInput;

    const QString filterComplex = buildFilterComplex(config);
    if (!filterComplex.isEmpty()) {
        args << QStringLiteral("-filter_complex") << filterComplex;
    }

    // Video stream mapping
    args << QStringLiteral("-map") << QStringLiteral("[outv]");

    // Audio stream mapping
    if (config.audioPolicy == AudioMixPolicy::MuteAll) {
        args << QStringLiteral("-an");
    } else if (config.audioPolicy == AudioMixPolicy::MainOnly) {
        args << QStringLiteral("-map") << QStringLiteral("0:a?");
    } else if (config.audioPolicy == AudioMixPolicy::PipOnly) {
        args << QStringLiteral("-map") << QStringLiteral("1:a?");
    } else {
        // MixDual or Ducking uses [outa]
        args << QStringLiteral("-map") << QStringLiteral("[outa]");
    }

    // Video codec & quality
    if (!config.videoCodec.isEmpty() && config.videoCodec != QLatin1String("copy")) {
        args << QStringLiteral("-c:v") << config.videoCodec;
        if (config.crf >= 0) {
            args << QStringLiteral("-crf") << QString::number(config.crf);
        }
        if (!config.preset.isEmpty()) {
            args << QStringLiteral("-preset") << config.preset;
        }
    } else {
        args << QStringLiteral("-c:v") << QStringLiteral("libx264");
    }

    // Audio codec
    if (config.audioPolicy != AudioMixPolicy::MuteAll) {
        args << QStringLiteral("-c:a")
             << (config.audioCodec.isEmpty() ? QStringLiteral("aac") : config.audioCodec);
    }

    if (config.shortest) {
        args << QStringLiteral("-shortest");
    }

    args << outputPath;
    return args;
}

} // namespace Compositor
