#include "BatchExportMatrix.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

QJsonObject BatchExportTarget::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("name")] = name;
    obj[QStringLiteral("format")] = format;
    obj[QStringLiteral("vcodec")] = vcodec;
    obj[QStringLiteral("acodec")] = acodec;
    obj[QStringLiteral("quality")] = quality;
    obj[QStringLiteral("resolution")] = resolution;
    obj[QStringLiteral("suffix")] = suffix;
    obj[QStringLiteral("enabled")] = enabled;
    return obj;
}

BatchExportTarget BatchExportTarget::fromJson(const QJsonObject &obj)
{
    BatchExportTarget t;
    t.name = obj.value(QStringLiteral("name")).toString();
    t.format = obj.value(QStringLiteral("format")).toString(QStringLiteral("mp4"));
    t.vcodec = obj.value(QStringLiteral("vcodec")).toString(QStringLiteral("libx264"));
    t.acodec = obj.value(QStringLiteral("acodec")).toString(QStringLiteral("aac"));
    t.quality = obj.value(QStringLiteral("quality")).toInt(23);
    t.resolution = obj.value(QStringLiteral("resolution")).toString();
    t.suffix = obj.value(QStringLiteral("suffix")).toString(QStringLiteral("_1080p"));
    t.enabled = obj.value(QStringLiteral("enabled")).toBool(true);
    return t;
}

QStringList BatchExportMatrix::defaultVideoFilters()
{
    return {
        QStringLiteral("*.mp4"), QStringLiteral("*.mkv"), QStringLiteral("*.mov"),
        QStringLiteral("*.avi"), QStringLiteral("*.flv"), QStringLiteral("*.webm"),
        QStringLiteral("*.wmv"), QStringLiteral("*.ts"),  QStringLiteral("*.m4v"),
        QStringLiteral("*.mpg"), QStringLiteral("*.mpeg")
    };
}

QStringList BatchExportMatrix::defaultAudioFilters()
{
    return {
        QStringLiteral("*.mp3"), QStringLiteral("*.flac"), QStringLiteral("*.wav"),
        QStringLiteral("*.aac"), QStringLiteral("*.ogg"),  QStringLiteral("*.m4a"),
        QStringLiteral("*.wma"), QStringLiteral("*.opus")
    };
}

QStringList BatchExportMatrix::defaultMediaFilters()
{
    QStringList all = defaultVideoFilters();
    all.append(defaultAudioFilters());
    return all;
}

QList<BatchExportTarget> BatchExportMatrix::defaultTargets()
{
    QList<BatchExportTarget> targets;

    BatchExportTarget t1;
    t1.name = QStringLiteral("Web MP4 (1080p H.264/AAC)");
    t1.format = QStringLiteral("mp4");
    t1.vcodec = QStringLiteral("libx264");
    t1.acodec = QStringLiteral("aac");
    t1.quality = 23;
    t1.resolution = QStringLiteral("1920x1080");
    t1.suffix = QStringLiteral("_1080p");
    t1.enabled = true;
    targets.append(t1);

    BatchExportTarget t2;
    t2.name = QStringLiteral("Mobile MP4 (720p H.264/AAC)");
    t2.format = QStringLiteral("mp4");
    t2.vcodec = QStringLiteral("libx264");
    t2.acodec = QStringLiteral("aac");
    t2.quality = 24;
    t2.resolution = QStringLiteral("1280x720");
    t2.suffix = QStringLiteral("_720p");
    t2.enabled = false;
    targets.append(t2);

    BatchExportTarget t3;
    t3.name = QStringLiteral("High Efficiency (HEVC/H.265)");
    t3.format = QStringLiteral("mp4");
    t3.vcodec = QStringLiteral("libx265");
    t3.acodec = QStringLiteral("aac");
    t3.quality = 26;
    t3.resolution = QString();
    t3.suffix = QStringLiteral("_hevc");
    t3.enabled = false;
    targets.append(t3);

    BatchExportTarget t4;
    t4.name = QStringLiteral("Audio Extraction (MP3 320k)");
    t4.format = QStringLiteral("mp3");
    t4.vcodec = QString();
    t4.acodec = QStringLiteral("libmp3lame");
    t4.quality = 320;
    t4.resolution = QString();
    t4.suffix = QStringLiteral("_audio");
    t4.enabled = false;
    targets.append(t4);

    return targets;
}

QString BatchExportMatrix::sanitizeSubPath(const QString &subPath)
{
    QString res = subPath;
    res.replace(QLatin1Char('\\'), QLatin1Char('/'));
    while (res.startsWith(QLatin1Char('/'))) {
        res.remove(0, 1);
    }
    while (res.endsWith(QLatin1Char('/'))) {
        res.chop(1);
    }
    return res;
}

QList<BatchScannedFile> BatchExportMatrix::scanDirectory(const QString &rootDir, bool recursive,
                                                        const QStringList &nameFilters)
{
    QList<BatchScannedFile> files;
    const QDir root(rootDir);
    if (!root.exists()) {
        return files;
    }

    const QDirIterator::IteratorFlags flags = recursive ? QDirIterator::Subdirectories
                                                        : QDirIterator::NoIteratorFlags;

    QDirIterator it(rootDir, nameFilters, QDir::Files | QDir::NoSymLinks | QDir::Readable, flags);
    while (it.hasNext()) {
        const QString fullPath = it.next();
        const QFileInfo fi(fullPath);

        // Compute relative subfolder path
        QString relPath = root.relativeFilePath(fullPath);
        relPath.replace(QLatin1Char('\\'), QLatin1Char('/'));

        QString subDir;
        const int slashIdx = relPath.lastIndexOf(QLatin1Char('/'));
        if (slashIdx >= 0) {
            subDir = relPath.left(slashIdx);
        }

        BatchScannedFile item;
        item.fullPath = fullPath;
        item.relativeSubDir = sanitizeSubPath(subDir);
        item.fileName = fi.fileName();
        item.sizeBytes = fi.size();
        files.append(item);
    }

    return files;
}

QString BatchExportMatrix::resolveOutputPath(const QString &baseOutputDir, const QString &relativeSubDir,
                                            const QString &inputBaseName, const QString &extension,
                                            const QString &suffix)
{
    QString targetDir = baseOutputDir;
    const QString cleanSubDir = sanitizeSubPath(relativeSubDir);
    if (!cleanSubDir.isEmpty()) {
        targetDir = QDir(baseOutputDir).filePath(cleanSubDir);
    }

    QDir().mkpath(targetDir);

    QString outExt = extension;
    if (outExt.startsWith(QLatin1Char('.'))) {
        outExt.remove(0, 1);
    }

    const QString fileName = QStringLiteral("%1%2.%3").arg(inputBaseName, suffix, outExt);
    return QDir(targetDir).filePath(fileName);
}
