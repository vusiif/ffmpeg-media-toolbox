#pragma once

#include <QString>
#include <QStringList>

struct WhisperOptions {
    QString language = QStringLiteral("auto");
    QString modelName = QStringLiteral("base");
    QString modelPath;
    QString whisperBinaryPath;
    int threadCount = 4;
    bool translateToEnglish = false;
    int maxLineLength = 42;
};

class WhisperPipeline
{
public:
    static QStringList supportedLanguages();
    static QStringList modelTiers();
    static QString defaultModelDirectory();

    static bool isWhisperAvailable(const QString &customPath = QString());
    static QString findWhisperBinary(const QString &customPath = QString());

    static QStringList buildAudioExtractionCommand(const QString &inputMedia,
                                                   const QString &outputWav16k);

    static QStringList buildWhisperCommand(const WhisperOptions &options,
                                           const QString &inputWav16k,
                                           const QString &outputBaseName);
};
