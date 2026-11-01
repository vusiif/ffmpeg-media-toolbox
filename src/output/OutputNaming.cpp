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

    QString targetPath;
    if (outputDir.isEmpty()) {
        targetPath = QDir(info.absolutePath()).filePath(name);
    } else {
        targetPath = QDir(outputDir).filePath(name);
    }
    return ensureUniquePath(targetPath);
}

QString OutputNaming::ensureUniquePath(const QString &targetPath)
{
    if (targetPath.isEmpty() || !QFileInfo::exists(targetPath)) {
        return targetPath;
    }

    const QFileInfo fi(targetPath);
    const QString dir = fi.absolutePath();
    const QString base = fi.completeBaseName();
    const QString ext = fi.suffix();

    int count = 1;
    while (count < 10000) {
        QString candidateName = ext.isEmpty()
            ? QStringLiteral("%1_%2").arg(base).arg(count)
            : QStringLiteral("%1_%2.%3").arg(base).arg(count).arg(ext);
        QString candidate = QDir(dir).filePath(candidateName);
        if (!QFileInfo::exists(candidate)) {
            return candidate;
        }
        ++count;
    }
    return targetPath;
}
