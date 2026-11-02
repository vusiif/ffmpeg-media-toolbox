#pragma once

#include <QList>
#include <QString>
#include <QStringList>

struct SubtitleItem {
    int id = 1;
    qint64 startTimeMs = 0;
    qint64 endTimeMs = 0;
    QString text;

    qint64 durationMs() const { return endTimeMs - startTimeMs; }
    QString startFormatted(bool isVtt = false) const;
    QString endFormatted(bool isVtt = false) const;

    static QString formatTime(qint64 ms, bool isVtt = false);
    static qint64 parseTime(const QString &timeStr);
};

class SubtitleEngine
{
public:
    // Parsing
    static QList<SubtitleItem> parseSrt(const QString &content);
    static QList<SubtitleItem> parseVtt(const QString &content);
    static QList<SubtitleItem> loadFromFile(const QString &filePath);

    // Serialization
    static QString serializeSrt(const QList<SubtitleItem> &items);
    static QString serializeVtt(const QList<SubtitleItem> &items);
    static QString serializeAss(const QList<SubtitleItem> &items,
                                const QString &title = QStringLiteral("Subtitles"));
    static QString serializePlainText(const QList<SubtitleItem> &items);
    static bool saveToFile(const QString &filePath, const QList<SubtitleItem> &items);

    // Timing adjustments
    static void shiftTime(QList<SubtitleItem> &items, qint64 offsetMs);
    static void scaleTime(QList<SubtitleItem> &items, double factor);
    static bool mergeItems(QList<SubtitleItem> &items, int indexA, int indexB);
    static bool splitItem(QList<SubtitleItem> &items, int index, qint64 splitTimeMs);

    // Lookup
    static int findItemAtTime(const QList<SubtitleItem> &items, qint64 timeMs);
};
