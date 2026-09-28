#include "OutputNaming.h"

#include <QDir>
#include <QFileInfo>

QString OutputNaming::build(const QString &inputPath,
                            const QString &templ,
                            const QString &ext,
                            const QString &outputDir)
{
    const QFileInfo info(inputPath);
    QString name = templ.trimmed();
    if (name.isEmpty()) {
        name = QStringLiteral("{name}.{format}");
    }

    name.replace(QStringLiteral("{name}"), info.completeBaseName());
    name.replace(QStringLiteral("{format}"), ext);

    if (outputDir.isEmpty()) {
        return QDir(info.absolutePath()).filePath(name);
    }
    return QDir(outputDir).filePath(name);
}
