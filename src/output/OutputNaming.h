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

    // Ensures that the proposed file path does not collide with an existing file on disk.
    // If targetPath exists, appends _1, _2, etc. (e.g. output_1.mp4) until a free filename is found.
    static QString ensureUniquePath(const QString &targetPath);
};
