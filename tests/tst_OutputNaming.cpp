#include <QtTest>

#include "output/OutputNaming.h"

class OutputNamingTest : public QObject
{
    Q_OBJECT

private slots:
    void emptyTemplateUsesDefault();
    void explicitTokensAreReplaced();
    void repeatedTokensAreAllReplaced();
    void multiDotBaseNameIsKept();
    void customOutputDirectoryIsUsed();
    void blankTemplateFallsBackToDefault();
};

void OutputNamingTest::emptyTemplateUsesDefault()
{
    QCOMPARE(OutputNaming::build(QStringLiteral("C:/media/clip.mkv"), QString(),
                                 QStringLiteral("mp4")),
             QStringLiteral("C:/media/clip.mp4"));
}

void OutputNamingTest::explicitTokensAreReplaced()
{
    QCOMPARE(OutputNaming::build(QStringLiteral("C:/media/clip.mkv"),
                                 QStringLiteral("{name}.{format}"),
                                 QStringLiteral("mp4")),
             QStringLiteral("C:/media/clip.mp4"));
    QCOMPARE(OutputNaming::build(QStringLiteral("C:/media/clip.mkv"),
                                 QStringLiteral("{name}_{format}.{format}"),
                                 QStringLiteral("gif")),
             QStringLiteral("C:/media/clip_gif.gif"));
}

void OutputNamingTest::repeatedTokensAreAllReplaced()
{
    QCOMPARE(OutputNaming::build(QStringLiteral("C:/media/clip.mkv"),
                                 QStringLiteral("{name}-{name}.{format}"),
                                 QStringLiteral("mp4")),
             QStringLiteral("C:/media/clip-clip.mp4"));
}

void OutputNamingTest::multiDotBaseNameIsKept()
{
    QCOMPARE(OutputNaming::build(QStringLiteral("C:/media/a.b.mkv"), QString(),
                                 QStringLiteral("mp4")),
             QStringLiteral("C:/media/a.b.mp4"));
}

void OutputNamingTest::customOutputDirectoryIsUsed()
{
    QCOMPARE(OutputNaming::build(QStringLiteral("C:/media/clip.mkv"), QString(),
                                 QStringLiteral("mp4"), QStringLiteral("D:/out")),
             QStringLiteral("D:/out/clip.mp4"));
}

void OutputNamingTest::blankTemplateFallsBackToDefault()
{
    QCOMPARE(OutputNaming::build(QStringLiteral("C:/media/clip.mkv"),
                                 QStringLiteral("   "),
                                 QStringLiteral("mp4")),
             QStringLiteral("C:/media/clip.mp4"));
}

QTEST_GUILESS_MAIN(OutputNamingTest)
#include "tst_OutputNaming.moc"
