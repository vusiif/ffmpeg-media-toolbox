#include <QTest>
#include <QSignalSpy>
#include "repair/UntruncLocator.h"
#include "repair/UntruncProcess.h"

class tst_UntruncProcess : public QObject
{
    Q_OBJECT

private slots:
    void testLocator();
    void testLocatorNonBlockingStartup();
    void testPredictFixedPath();
    void testProgressSignalEmission();
};

void tst_UntruncProcess::testProgressSignalEmission()
{
    UntruncProcess proc;
    QSignalSpy progressSpy(&proc, &UntruncProcess::progressChanged);
    QSignalSpy logSpy(&proc, &UntruncProcess::logMessage);

    // Simulate untrunc output with carriage return progress lines
    QString buffer = QStringLiteral("reading atoms...\r 12.5%  \r 45%  \r 98.2%  \nsaving fixed file\n");
    proc.processOutputBuffer(buffer);

    QCOMPARE(progressSpy.count(), 3);
    QCOMPARE(progressSpy.at(0).at(0).toDouble(), 0.125);
    QCOMPARE(progressSpy.at(1).at(0).toDouble(), 0.45);
    QCOMPARE(progressSpy.at(2).at(0).toDouble(), 0.982);

    QVERIFY(logSpy.count() >= 4);
    QVERIFY(logSpy.at(0).at(0).toString().contains(QStringLiteral("reading atoms...")));
    QVERIFY(logSpy.last().at(0).toString().contains(QStringLiteral("saving fixed file")));
}

void tst_UntruncProcess::testLocatorNonBlockingStartup()
{
    // Instantiation must be fast and non-blocking
    UntruncLocator locator;
    // Must complete and become available via waitForScan
    QVERIFY(locator.waitForScan(10000));
    QVERIFY(locator.isAvailable());
}

void tst_UntruncProcess::testLocator()
{
    UntruncLocator locator;
    locator.waitForScan();
    // Should locate untrunc from Others/untrunc/untrunc_x64/untrunc.exe or installed directory
    QVERIFY(locator.isAvailable());
    QVERIFY(!locator.untruncPath().isEmpty());
    QVERIFY(locator.untruncPath().contains(QStringLiteral("untrunc.exe")));
}

void tst_UntruncProcess::testPredictFixedPath()
{
    const QString ref = QStringLiteral("C:/test/sample.mp4");
    const QString corrupt = QStringLiteral("C:/test/broken.mp4");

    const QString predicted = UntruncProcess::predictFixedPath(ref, corrupt);
    QVERIFY(!predicted.isEmpty());
    QVERIFY(predicted.contains(QStringLiteral("_fixed")));
}

QTEST_MAIN(tst_UntruncProcess)
#include "tst_UntruncProcess.moc"
