#pragma once

#include <QString>

class OutputNaming
{
public:
    // Template tokens: {name} = input base name, {format} = output extension.
    // The result always gets "-out-<yyyyMMdd-HHmmss>" inserted before the
    // extension, so repeated runs from the same source never clobber each
    // other (or the source itself). Pass an explicit stamp for reproducible
    // names; empty uses the current time.
    static QString build(const QString &inputPath,
                         const QString &templ,
                         const QString &ext,
                         const QString &outputDir = QString(),
                         const QString &stamp = QString());
};
