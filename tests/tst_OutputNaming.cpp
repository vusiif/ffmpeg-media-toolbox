#include <QRegularExpression>
#include <QtTest>

#include "output/OutputNaming.h"

namespace {
const QString kStamp = QStringLiteral("20260102-030405");
} // namespace

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
    void nameWithoutExtensionGetsSuffixAtEnd();
    void stampIsUnconditional();
    void defaultStampIsGenerated();
};

void OutputNamingTest::emptyTemplateUsesDefault()
{
    QCOMPARE(OutputNaming::build(QStringLiteral("C:/media/clip.mkv"), QString(),
                                 QStringLiteral("mp4"), QString(), kStamp),
             QStringLiteral("C:/media/clip-out-20260102-030405.mp4"));
}

void OutputNamingTest::explicitTokensAreReplaced()
{
    QCOMPARE(OutputNaming::build(QStringLiteral("C:/media/clip.mkv"),
                                 QStringLiteral("{name}.{format}"),
                                 QStringLiteral("mp4"), QString(), kStamp),
             QStringLiteral("C:/media/clip-out-20260102-030405.mp4"));
    QCOMPARE(OutputNaming::build(QStringLiteral("C:/media/clip.mkv"),
                                 QStringLiteral("{name}_{format}.{format}"),
                                 QStringLiteral("gif"), QString(), kStamp),
             QStringLiteral("C:/media/clip_gif-out-20260102-030405.gif"));
}

void OutputNamingTest::repeatedTokensAreAllReplaced()
{
    QCOMPARE(OutputNaming::build(QStringLiteral("C:/media/clip.mkv"),
                                 QStringLiteral("{name}-{name}.{format}"),
                                 QStringLiteral("mp4"), QString(), kStamp),
             QStringLiteral("C:/media/clip-clip-out-20260102-030405.mp4"));
}

void OutputNamingTest::multiDotBaseNameIsKept()
{
    QCOMPARE(OutputNaming::build(QStringLiteral("C:/media/a.b.mkv"), QString(),
                                 QStringLiteral("mp4"), QString(), kStamp),
             QStringLiteral("C:/media/a.b-out-20260102-030405.mp4"));
}

void OutputNamingTest::customOutputDirectoryIsUsed()
{
    QCOMPARE(OutputNaming::build(QStringLiteral("C:/media/clip.mkv"), QString(),
                                 QStringLiteral("mp4"), QStringLiteral("D:/out"), kStamp),
             QStringLiteral("D:/out/clip-out-20260102-030405.mp4"));
}

void OutputNamingTest::blankTemplateFallsBackToDefault()
{
    QCOMPARE(OutputNaming::build(QStringLiteral("C:/media/clip.mkv"),
                                 QStringLiteral("   "),
                                 QStringLiteral("mp4"), QString(), kStamp),
             QStringLiteral("C:/media/clip-out-20260102-030405.mp4"));
}

void OutputNamingTest::nameWithoutExtensionGetsSuffixAtEnd()
{
    QCOMPARE(OutputNaming::build(QStringLiteral("C:/media/clip.mkv"),
                                 QStringLiteral("{name}"),
                                 QStringLiteral("mp4"), QString(), kStamp),
             QStringLiteral("C:/media/clip-out-20260102-030405"));
}

void OutputNamingTest::stampIsUnconditional()
{
    // Even when the container changes and no overwrite is possible, the stamp
    // is applied so re-runs with different parameters never clobber each other.
    const QString path = OutputNaming::build(QStringLiteral("C:/media/clip.mp4"), QString(),
                                             QStringLiteral("mkv"), QString(), kStamp);
    QCOMPARE(path, QStringLiteral("C:/media/clip-out-20260102-030405.mkv"));
}

void OutputNamingTest::defaultStampIsGenerated()
{
    const QString path = OutputNaming::build(QStringLiteral("C:/media/clip.mkv"), QString(),
                                             QStringLiteral("mp4"));
    const QRegularExpression re(
        QStringLiteral("^C:/media/clip-out-\\d{8}-\\d{6}\\.mp4$"));
    const QRegularExpressionMatch match = re.match(path);
    QVERIFY2(match.hasMatch(), qPrintable(path));
}

QTEST_GUILESS_MAIN(OutputNamingTest)
#include "tst_OutputNaming.moc"
