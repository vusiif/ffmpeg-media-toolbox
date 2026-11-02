#include "WhisperPipeline.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

QStringList WhisperPipeline::supportedLanguages()
{
    return {
        QStringLiteral("auto"),
        QStringLiteral("zh"),
        QStringLiteral("en"),
        QStringLiteral("ja"),
        QStringLiteral("ko"),
        QStringLiteral("es"),
        QStringLiteral("fr"),
        QStringLiteral("de"),
        QStringLiteral("ru"),
        QStringLiteral("it")
    };
}

QStringList WhisperPipeline::modelTiers()
{
    return {
        QStringLiteral("tiny"),
        QStringLiteral("base"),
        QStringLiteral("small"),
        QStringLiteral("medium"),
        QStringLiteral("large")
    };
}

QString WhisperPipeline::defaultModelDirectory()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!base.isEmpty()) {
        const QString dir = QDir(base).filePath(QStringLiteral("models"));
        QDir().mkpath(dir);
        return dir;
    }
    return QDir::current().filePath(QStringLiteral("models"));
}

QString WhisperPipeline::findWhisperBinary(const QString &customPath)
{
    if (!customPath.isEmpty() && QFile::exists(customPath)) {
        return QDir::cleanPath(customPath);
    }

    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        QDir(appDir).filePath(QStringLiteral("whisper-cli.exe")),
        QDir(appDir).filePath(QStringLiteral("whisper.exe")),
        QDir(appDir).filePath(QStringLiteral("main.exe")),
        QDir(appDir).filePath(QStringLiteral("whisper-cpp/main.exe")),
        QStringLiteral("whisper-cli.exe"),
        QStringLiteral("whisper.exe")
    };

    for (const QString &cand : candidates) {
        if (QFile::exists(cand)) {
            return QDir::cleanPath(cand);
        }
    }

    const QString inPath = QStandardPaths::findExecutable(QStringLiteral("whisper-cli"));
    if (!inPath.isEmpty()) return inPath;

    return QString();
}

bool WhisperPipeline::isWhisperAvailable(const QString &customPath)
{
    return !findWhisperBinary(customPath).isEmpty();
}

QStringList WhisperPipeline::buildAudioExtractionCommand(const QString &inputMedia,
                                                         const QString &outputWav16k)
{
    return {
        QStringLiteral("-hide_banner"),
        QStringLiteral("-y"),
        QStringLiteral("-i"), inputMedia,
        QStringLiteral("-vn"),
        QStringLiteral("-ar"), QStringLiteral("16000"),
        QStringLiteral("-ac"), QStringLiteral("1"),
        QStringLiteral("-c:a"), QStringLiteral("pcm_s16le"),
        outputWav16k
    };
}

QStringList WhisperPipeline::buildWhisperCommand(const WhisperOptions &options,
                                                 const QString &inputWav16k,
                                                 const QString &outputBaseName)
{
    QStringList args;
    if (!options.modelPath.isEmpty()) {
        args << QStringLiteral("-m") << options.modelPath;
    }
    args << QStringLiteral("-f") << inputWav16k;

    if (!options.language.isEmpty() && options.language != QLatin1String("auto")) {
        args << QStringLiteral("-l") << options.language;
    } else {
        args << QStringLiteral("-l") << QStringLiteral("auto");
    }

    if (options.translateToEnglish) {
        args << QStringLiteral("-tr");
    }

    if (options.threadCount > 0) {
        args << QStringLiteral("-t") << QString::number(options.threadCount);
    }

    if (options.maxLineLength > 0) {
        args << QStringLiteral("-ml") << QString::number(options.maxLineLength);
    }

    args << QStringLiteral("-osrt") << QStringLiteral("-ovtt");
    args << QStringLiteral("-of") << outputBaseName;
    return args;
}
