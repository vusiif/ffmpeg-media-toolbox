#include "OutputNaming.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>

QString OutputNaming::build(const QString &inputPath,
                            const QString &templ,
                            const QString &ext,
                            const QString &outputDir,
                            const QString &stamp)
{
    const QFileInfo info(inputPath);
    QString name = templ.trimmed();
    if (name.isEmpty()) {
        name = QStringLiteral("{name}.{format}");
    }

    name.replace(QStringLiteral("{name}"), info.completeBaseName());
    name.replace(QStringLiteral("{format}"), ext);

    QString finalStamp = stamp;
    if (finalStamp.isEmpty()) {
        finalStamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    }
    const QString suffix = QStringLiteral("-out-") + finalStamp;
    const int dot = name.lastIndexOf(QLatin1Char('.'));
    if (dot > 0) {
        name.insert(dot, suffix);
    } else {
        name.append(suffix);
    }

    if (outputDir.isEmpty()) {
        return QDir(info.absolutePath()).filePath(name);
    }
    return QDir(outputDir).filePath(name);
}
