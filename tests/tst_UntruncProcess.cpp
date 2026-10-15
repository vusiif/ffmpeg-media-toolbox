#include <QTest>
#include "repair/UntruncLocator.h"
#include "repair/UntruncProcess.h"

class tst_UntruncProcess : public QObject
{
    Q_OBJECT

private slots:
    void testLocator();
    void testLocatorNonBlockingStartup();
    void testPredictFixedPath();
};

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
