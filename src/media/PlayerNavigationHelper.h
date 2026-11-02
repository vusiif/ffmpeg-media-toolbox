#pragma once

#include <QString>
#include <QVector>

namespace Media {

class PlayerNavigationHelper
{
public:
    // Finds the next keyframe strictly greater than curSec + threshold
    static double findNextKeyframe(const QVector<double> &keyframes, double curSec, double threshold = 0.05);

    // Finds the previous keyframe strictly less than curSec - threshold
    static double findPreviousKeyframe(const QVector<double> &keyframes, double curSec, double threshold = 0.05);

    // Frame stepping calculations with boundary clamps
    static double stepForwardSec(double curSec, double durationSec, double stepSec = 0.04);
    static double stepBackwardSec(double curSec, double stepSec = 0.04);

    // Time formatting with optional milliseconds
    static QString formatTime(qint64 ms, bool showMs = false);
    static QString formatTimeSec(double sec, bool showMs = false);

    // Speed / Playback rate conversions
    static double speedIndexToRate(int index);
    static int rateToSpeedIndex(double rate);

    // Volume scalar conversions (0..100 <-> 0.0f..1.0f)
    static float volumeToScalar(int sliderValue);
    static int scalarToVolume(float scalar);
};

} // namespace Media
