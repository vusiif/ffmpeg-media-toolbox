#include <QTest>
#include <QVector>

#include "media/PlayerNavigationHelper.h"

using namespace Media;

class tst_PlayerNavigation : public QObject
{
    Q_OBJECT

private slots:
    void testKeyframeSearch();
    void testStepForwardAndBackward();
    void testTimeFormatting();
    void testSpeedAndRateConversion();
    void testVolumeAndScalarConversion();
};

void tst_PlayerNavigation::testKeyframeSearch()
{
    const QVector<double> emptyKfs;
    QCOMPARE(PlayerNavigationHelper::findNextKeyframe(emptyKfs, 1.0), -1.0);
    QCOMPARE(PlayerNavigationHelper::findPreviousKeyframe(emptyKfs, 1.0), -1.0);

    const QVector<double> kfs = { 0.0, 2.5, 5.0, 7.8, 12.0 };

    // Next keyframe
    QCOMPARE(PlayerNavigationHelper::findNextKeyframe(kfs, 0.0), 2.5);
    QCOMPARE(PlayerNavigationHelper::findNextKeyframe(kfs, 1.2), 2.5);
    QCOMPARE(PlayerNavigationHelper::findNextKeyframe(kfs, 2.5), 5.0);
    QCOMPARE(PlayerNavigationHelper::findNextKeyframe(kfs, 7.0), 7.8);
    QCOMPARE(PlayerNavigationHelper::findNextKeyframe(kfs, 7.8), 12.0);
    QCOMPARE(PlayerNavigationHelper::findNextKeyframe(kfs, 12.0), -1.0);
    QCOMPARE(PlayerNavigationHelper::findNextKeyframe(kfs, 15.0), -1.0);

    // Previous keyframe
    QCOMPARE(PlayerNavigationHelper::findPreviousKeyframe(kfs, 15.0), 12.0);
    QCOMPARE(PlayerNavigationHelper::findPreviousKeyframe(kfs, 12.0), 7.8);
    QCOMPARE(PlayerNavigationHelper::findPreviousKeyframe(kfs, 9.0), 7.8);
    QCOMPARE(PlayerNavigationHelper::findPreviousKeyframe(kfs, 7.8), 5.0);
    QCOMPARE(PlayerNavigationHelper::findPreviousKeyframe(kfs, 5.0), 2.5);
    QCOMPARE(PlayerNavigationHelper::findPreviousKeyframe(kfs, 2.5), 0.0);
    QCOMPARE(PlayerNavigationHelper::findPreviousKeyframe(kfs, 0.0), -1.0);
}

void tst_PlayerNavigation::testStepForwardAndBackward()
{
    // Step forward
    QCOMPARE(PlayerNavigationHelper::stepForwardSec(0.0, 10.0, 0.04), 0.04);
    QCOMPARE(PlayerNavigationHelper::stepForwardSec(5.0, 10.0, 0.04), 5.04);
    // Boundary clamp at duration
    QCOMPARE(PlayerNavigationHelper::stepForwardSec(9.98, 10.0, 0.04), 10.0);
    QCOMPARE(PlayerNavigationHelper::stepForwardSec(10.0, 10.0, 0.04), 10.0);

    // Step backward
    QCOMPARE(PlayerNavigationHelper::stepBackwardSec(5.0, 0.04), 4.96);
    QCOMPARE(PlayerNavigationHelper::stepBackwardSec(0.05, 0.04), 0.01);
    // Boundary clamp at 0
    QCOMPARE(PlayerNavigationHelper::stepBackwardSec(0.02, 0.04), 0.0);
    QCOMPARE(PlayerNavigationHelper::stepBackwardSec(0.0, 0.04), 0.0);
}

void tst_PlayerNavigation::testTimeFormatting()
{
    // Standard MM:SS
    QCOMPARE(PlayerNavigationHelper::formatTime(0), QStringLiteral("00:00"));
    QCOMPARE(PlayerNavigationHelper::formatTime(5000), QStringLiteral("00:05"));
    QCOMPARE(PlayerNavigationHelper::formatTime(65000), QStringLiteral("01:05"));

    // HH:MM:SS
    QCOMPARE(PlayerNavigationHelper::formatTime(3665000), QStringLiteral("01:01:05"));

    // With milliseconds
    QCOMPARE(PlayerNavigationHelper::formatTime(65123, true), QStringLiteral("01:05.123"));
    QCOMPARE(PlayerNavigationHelper::formatTime(3665042, true), QStringLiteral("01:01:05.042"));

    // formatTimeSec
    QCOMPARE(PlayerNavigationHelper::formatTimeSec(65.123, true), QStringLiteral("01:05.123"));
}

void tst_PlayerNavigation::testSpeedAndRateConversion()
{
    QCOMPARE(PlayerNavigationHelper::speedIndexToRate(0), 0.5);
    QCOMPARE(PlayerNavigationHelper::speedIndexToRate(1), 0.75);
    QCOMPARE(PlayerNavigationHelper::speedIndexToRate(2), 1.0);
    QCOMPARE(PlayerNavigationHelper::speedIndexToRate(3), 1.25);
    QCOMPARE(PlayerNavigationHelper::speedIndexToRate(4), 1.5);
    QCOMPARE(PlayerNavigationHelper::speedIndexToRate(5), 2.0);

    QCOMPARE(PlayerNavigationHelper::rateToSpeedIndex(0.5), 0);
    QCOMPARE(PlayerNavigationHelper::rateToSpeedIndex(1.0), 2);
    QCOMPARE(PlayerNavigationHelper::rateToSpeedIndex(1.5), 4);
    QCOMPARE(PlayerNavigationHelper::rateToSpeedIndex(2.0), 5);
}

void tst_PlayerNavigation::testVolumeAndScalarConversion()
{
    QCOMPARE(PlayerNavigationHelper::volumeToScalar(0), 0.0f);
    QCOMPARE(PlayerNavigationHelper::volumeToScalar(100), 1.0f);
    QCOMPARE(PlayerNavigationHelper::volumeToScalar(50), 0.5f);
    QCOMPARE(PlayerNavigationHelper::volumeToScalar(-10), 0.0f);
    QCOMPARE(PlayerNavigationHelper::volumeToScalar(150), 1.0f);

    QCOMPARE(PlayerNavigationHelper::scalarToVolume(0.0f), 0);
    QCOMPARE(PlayerNavigationHelper::scalarToVolume(1.0f), 100);
    QCOMPARE(PlayerNavigationHelper::scalarToVolume(0.5f), 50);
}

QTEST_MAIN(tst_PlayerNavigation)
#include "tst_PlayerNavigation.moc"
