#include <QtTest>

#include "app/RecentFiles.h"

class RecentFilesTest : public QObject
{
    Q_OBJECT

private slots:
    void pushesToEmptyList();
    void movesExistingPathToFront();
    void keepsOrderOfOlderEntries();
    void ignoresEmptyPath();
    void ignoresNonPositiveCap();
    void capsAtMaxCount();
    void firstPositionStaysStable();
};

void RecentFilesTest::pushesToEmptyList()
{
    QCOMPARE(RecentFiles::pushFront({}, QStringLiteral("C:/media/a.mp4"), 10),
             QStringList{QStringLiteral("C:/media/a.mp4")});
}

void RecentFilesTest::movesExistingPathToFront()
{
    const QStringList current{QStringLiteral("C:/a.mp4"), QStringLiteral("C:/b.mp4"),
                              QStringLiteral("C:/c.mp4")};
    QCOMPARE(RecentFiles::pushFront(current, QStringLiteral("C:/b.mp4"), 10),
             (QStringList{QStringLiteral("C:/b.mp4"), QStringLiteral("C:/a.mp4"),
                          QStringLiteral("C:/c.mp4")}));
}

void RecentFilesTest::keepsOrderOfOlderEntries()
{
    const QStringList current{QStringLiteral("C:/b.mp4"), QStringLiteral("C:/c.mp4")};
    QCOMPARE(RecentFiles::pushFront(current, QStringLiteral("C:/d.mp4"), 10),
             (QStringList{QStringLiteral("C:/d.mp4"), QStringLiteral("C:/b.mp4"),
                          QStringLiteral("C:/c.mp4")}));
}

void RecentFilesTest::ignoresEmptyPath()
{
    const QStringList current{QStringLiteral("C:/a.mp4")};
    QCOMPARE(RecentFiles::pushFront(current, QString(), 10), current);
}

void RecentFilesTest::ignoresNonPositiveCap()
{
    const QStringList current{QStringLiteral("C:/a.mp4")};
    QCOMPARE(RecentFiles::pushFront(current, QStringLiteral("C:/b.mp4"), 0), current);
}

void RecentFilesTest::capsAtMaxCount()
{
    QStringList current;
    for (int i = 0; i < RecentFiles::kMaxCount; ++i) {
        current.append(QStringLiteral("C:/file%1.mp4").arg(i));
    }

    const QStringList result =
        RecentFiles::pushFront(current, QStringLiteral("C:/new.mp4"), RecentFiles::kMaxCount);

    QCOMPARE(result.size(), RecentFiles::kMaxCount);
    QCOMPARE(result.first(), QStringLiteral("C:/new.mp4"));
    QCOMPARE(result.last(), QStringLiteral("C:/file8.mp4"));
    QVERIFY(!result.contains(QStringLiteral("C:/file9.mp4")));
}

void RecentFilesTest::firstPositionStaysStable()
{
    const QStringList current{QStringLiteral("C:/a.mp4"), QStringLiteral("C:/b.mp4")};
    QCOMPARE(RecentFiles::pushFront(current, QStringLiteral("C:/a.mp4"), 10), current);
}

QTEST_GUILESS_MAIN(RecentFilesTest)
#include "tst_RecentFiles.moc"
