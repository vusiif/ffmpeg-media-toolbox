#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

struct BatchExportTarget {
    QString name;
    QString format = QStringLiteral("mp4");
    QString vcodec = QStringLiteral("libx264");
    QString acodec = QStringLiteral("aac");
    int quality = 23;
    QString resolution; // e.g. "1920x1080", "1280x720", or empty for original
    QString suffix = QStringLiteral("_1080p");
    bool enabled = true;

    QJsonObject toJson() const;
    static BatchExportTarget fromJson(const QJsonObject &obj);
};

struct BatchScannedFile {
    QString fullPath;
    QString relativeSubDir; // empty if directly in root directory, or "Season1/CD1"
    QString fileName;
    qint64 sizeBytes = 0;
};

class BatchExportMatrix
{
public:
    static QStringList defaultVideoFilters();
    static QStringList defaultAudioFilters();
    static QStringList defaultMediaFilters();

    static QList<BatchExportTarget> defaultTargets();

    static QList<BatchScannedFile> scanDirectory(const QString &rootDir, bool recursive,
                                                const QStringList &nameFilters = defaultMediaFilters());

    static QString resolveOutputPath(const QString &baseOutputDir, const QString &relativeSubDir,
                                    const QString &inputBaseName, const QString &extension,
                                    const QString &suffix = QString());

    static QString sanitizeSubPath(const QString &subPath);
};
