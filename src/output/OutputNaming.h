#pragma once

#include <QString>

class OutputNaming
{
public:
    // Template tokens: {name} = input base name, {format} = output extension.
    static QString build(const QString &inputPath,
                         const QString &templ,
                         const QString &ext,
                         const QString &outputDir = QString());
};
