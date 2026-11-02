#include "PlayerNavigationHelper.h"

#include <QtGlobal>
#include <algorithm>

namespace Media {

double PlayerNavigationHelper::findNextKeyframe(const QVector<double> &keyframes, double curSec, double threshold)
{
    if (keyframes.isEmpty()) return -1.0;

    for (double kf : keyframes) {
        if (kf > curSec + threshold) {
            return kf;
        }
    }
    return -1.0;
}

double PlayerNavigationHelper::findPreviousKeyframe(const QVector<double> &keyframes, double curSec, double threshold)
{
    if (keyframes.isEmpty()) return -1.0;

    double candidate = -1.0;
    for (double kf : keyframes) {
        if (kf < curSec - threshold) {
            candidate = kf;
        } else {
            break;
        }
    }
    return candidate;
}

double PlayerNavigationHelper::stepForwardSec(double curSec, double durationSec, double stepSec)
{
    const double validStep = qMax(0.001, stepSec);
    const double target = curSec + validStep;
    if (durationSec > 0.0) {
        return qMin(durationSec, target);
    }
    return target;
}

double PlayerNavigationHelper::stepBackwardSec(double curSec, double stepSec)
{
    const double validStep = qMax(0.001, stepSec);
    return qMax(0.0, curSec - validStep);
}

QString PlayerNavigationHelper::formatTime(qint64 ms, bool showMs)
{
    if (ms < 0) ms = 0;
    const int totalSeconds = static_cast<int>(ms / 1000);
    const int milliseconds = static_cast<int>(ms % 1000);
    const int seconds = totalSeconds % 60;
    const int minutes = (totalSeconds / 60) % 60;
    const int hours = totalSeconds / 3600;

    if (showMs) {
        if (hours > 0) {
            return QStringLiteral("%1:%2:%3.%4")
                .arg(hours, 2, 10, QLatin1Char('0'))
                .arg(minutes, 2, 10, QLatin1Char('0'))
                .arg(seconds, 2, 10, QLatin1Char('0'))
                .arg(milliseconds, 3, 10, QLatin1Char('0'));
        }
        return QStringLiteral("%1:%2.%3")
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'))
            .arg(milliseconds, 3, 10, QLatin1Char('0'));
    }

    if (hours > 0) {
        return QStringLiteral("%1:%2:%3")
            .arg(hours, 2, 10, QLatin1Char('0'))
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    }
    return QStringLiteral("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}

QString PlayerNavigationHelper::formatTimeSec(double sec, bool showMs)
{
    return formatTime(static_cast<qint64>(qMax(0.0, sec) * 1000.0), showMs);
}

double PlayerNavigationHelper::speedIndexToRate(int index)
{
    switch (index) {
    case 0: return 0.5;
    case 1: return 0.75;
    case 2: return 1.0;
    case 3: return 1.25;
    case 4: return 1.5;
    case 5: return 2.0;
    default: return 1.0;
    }
}

int PlayerNavigationHelper::rateToSpeedIndex(double rate)
{
    if (qAbs(rate - 0.5) < 0.1) return 0;
    if (qAbs(rate - 0.75) < 0.1) return 1;
    if (qAbs(rate - 1.0) < 0.1) return 2;
    if (qAbs(rate - 1.25) < 0.1) return 3;
    if (qAbs(rate - 1.5) < 0.1) return 4;
    if (qAbs(rate - 2.0) < 0.1) return 5;
    return 2;
}

float PlayerNavigationHelper::volumeToScalar(int sliderValue)
{
    return qBound(0, sliderValue, 100) / 100.0f;
}

int PlayerNavigationHelper::scalarToVolume(float scalar)
{
    return static_cast<int>(qBound(0.0f, scalar, 1.0f) * 100.0f);
}

} // namespace Media
