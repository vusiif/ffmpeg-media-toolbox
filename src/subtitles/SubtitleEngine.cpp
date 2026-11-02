#include "SubtitleEngine.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>
#include <algorithm>

QString SubtitleItem::formatTime(qint64 ms, bool isVtt)
{
    if (ms < 0) ms = 0;
    const qint64 totalSeconds = ms / 1000;
    const qint64 millis = ms % 1000;
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds % 3600) / 60;
    const qint64 seconds = totalSeconds % 60;

    const QChar sep = isVtt ? QLatin1Char('.') : QLatin1Char(',');

    return QStringLiteral("%1:%2:%3%4%5")
        .arg(hours, 2, 10, QLatin1Char('0'))
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'))
        .arg(sep)
        .arg(millis, 3, 10, QLatin1Char('0'));
}

QString SubtitleItem::startFormatted(bool isVtt) const
{
    return formatTime(startTimeMs, isVtt);
}

QString SubtitleItem::endFormatted(bool isVtt) const
{
    return formatTime(endTimeMs, isVtt);
}

qint64 SubtitleItem::parseTime(const QString &timeStr)
{
    const QString s = timeStr.trimmed();
    if (s.isEmpty()) return 0;

    // Matches: (HH:)?MM:SS[,.]mmm or (H:)?M:S[,.]mmm
    static const QRegularExpression regex(
        QStringLiteral("^(?:(?:(\\d{1,2}):)?(\\d{1,2}):)?(\\d{1,2})[,.](\\d{1,3})$"));

    const auto match = regex.match(s);
    if (!match.hasMatch()) {
        // Fallback simple colon splitter
        const auto parts = s.split(QLatin1Char(':'));
        if (parts.size() == 3) {
            double h = parts[0].toDouble();
            double m = parts[1].toDouble();
            QString secStr = parts[2];
            secStr.replace(QLatin1Char(','), QLatin1Char('.'));
            double sec = secStr.toDouble();
            return static_cast<qint64>((h * 3600.0 + m * 60.0 + sec) * 1000.0);
        }
        return 0;
    }

    qint64 hours = match.captured(1).isEmpty() ? 0 : match.captured(1).toLongLong();
    qint64 minutes = match.captured(2).isEmpty() ? 0 : match.captured(2).toLongLong();
    qint64 seconds = match.captured(3).toLongLong();
    QString msecStr = match.captured(4);
    while (msecStr.length() < 3) {
        msecStr.append(QLatin1Char('0'));
    }
    if (msecStr.length() > 3) {
        msecStr = msecStr.left(3);
    }
    qint64 msec = msecStr.toLongLong();

    return (hours * 3600 + minutes * 60 + seconds) * 1000 + msec;
}

QList<SubtitleItem> SubtitleEngine::parseSrt(const QString &content)
{
    QList<SubtitleItem> items;
    const QString clean = content.trimmed();
    if (clean.isEmpty()) return items;

    const QStringList blocks = clean.split(QRegularExpression(QStringLiteral("\r?\n\r?\n+")),
                                           Qt::SkipEmptyParts);

    int autoId = 1;
    for (const QString &block : blocks) {
        const QStringList lines = block.split(QRegularExpression(QStringLiteral("\r?\n")),
                                              Qt::SkipEmptyParts);
        if (lines.isEmpty()) continue;

        int timeLineIdx = 0;
        int parsedId = autoId;

        // If first line is a numeric ID
        bool isNumeric = false;
        int candidateId = lines[0].trimmed().toInt(&isNumeric);
        if (isNumeric && lines.size() > 1) {
            parsedId = candidateId;
            timeLineIdx = 1;
        }

        if (timeLineIdx >= lines.size()) continue;

        const QString &timeLine = lines[timeLineIdx];
        const int arrowPos = timeLine.indexOf(QStringLiteral("-->"));
        if (arrowPos == -1) continue;

        const QString startStr = timeLine.left(arrowPos).trimmed();
        const QString endStr = timeLine.mid(arrowPos + 3).trimmed();

        const qint64 startMs = SubtitleItem::parseTime(startStr);
        const qint64 endMs = SubtitleItem::parseTime(endStr);

        QStringList textLines;
        for (int i = timeLineIdx + 1; i < lines.size(); ++i) {
            textLines.append(lines[i]);
        }

        SubtitleItem item;
        item.id = parsedId;
        item.startTimeMs = startMs;
        item.endTimeMs = endMs;
        item.text = textLines.join(QLatin1Char('\n')).trimmed();

        items.append(item);
        autoId = std::max(autoId, parsedId + 1);
    }

    return items;
}

QList<SubtitleItem> SubtitleEngine::parseVtt(const QString &content)
{
    QList<SubtitleItem> items;
    QString clean = content.trimmed();
    if (clean.startsWith(QStringLiteral("WEBVTT"), Qt::CaseInsensitive)) {
        int firstNl = clean.indexOf(QLatin1Char('\n'));
        if (firstNl >= 0) {
            clean = clean.mid(firstNl).trimmed();
        }
    }

    const QStringList blocks = clean.split(QRegularExpression(QStringLiteral("\r?\n\r?\n+")),
                                           Qt::SkipEmptyParts);

    int autoId = 1;
    for (const QString &block : blocks) {
        const QStringList lines = block.split(QRegularExpression(QStringLiteral("\r?\n")),
                                              Qt::SkipEmptyParts);
        if (lines.isEmpty()) continue;

        int timeLineIdx = 0;
        int parsedId = autoId;

        if (lines[0].indexOf(QStringLiteral("-->")) == -1 && lines.size() > 1) {
            // First line might be an identifier/cue name
            timeLineIdx = 1;
        }

        if (timeLineIdx >= lines.size()) continue;

        const QString &timeLine = lines[timeLineIdx];
        const int arrowPos = timeLine.indexOf(QStringLiteral("-->"));
        if (arrowPos == -1) continue;

        const QString startStr = timeLine.left(arrowPos).trimmed();
        // VTT can have cue settings after the end timestamp (e.g. "00:02.000 line:0 position:20%")
        QString endStr = timeLine.mid(arrowPos + 3).trimmed();
        int spaceIdx = endStr.indexOf(QLatin1Char(' '));
        if (spaceIdx >= 0) {
            endStr = endStr.left(spaceIdx);
        }

        const qint64 startMs = SubtitleItem::parseTime(startStr);
        const qint64 endMs = SubtitleItem::parseTime(endStr);

        QStringList textLines;
        for (int i = timeLineIdx + 1; i < lines.size(); ++i) {
            QString line = lines[i];
            // Remove basic VTT tags like <v Speaker>, <b>, </i>, <00:00:01.000>
            line.replace(QRegularExpression(QStringLiteral("<[^>]+>")), QString());
            textLines.append(line);
        }

        SubtitleItem item;
        item.id = parsedId++;
        item.startTimeMs = startMs;
        item.endTimeMs = endMs;
        item.text = textLines.join(QLatin1Char('\n')).trimmed();

        items.append(item);
    }

    return items;
}

QList<SubtitleItem> SubtitleEngine::loadFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    const QString content = QString::fromUtf8(file.readAll());
    file.close();

    const QString ext = QFileInfo(filePath).suffix().toLower();
    if (ext == QLatin1String("vtt")) {
        return parseVtt(content);
    }
    return parseSrt(content);
}

QString SubtitleEngine::serializeSrt(const QList<SubtitleItem> &items)
{
    QString out;
    QTextStream ts(&out);

    for (int i = 0; i < items.size(); ++i) {
        const auto &item = items.at(i);
        ts << (i + 1) << "\n";
        ts << SubtitleItem::formatTime(item.startTimeMs, false)
           << " --> "
           << SubtitleItem::formatTime(item.endTimeMs, false) << "\n";
        ts << item.text << "\n\n";
    }

    return out;
}

QString SubtitleEngine::serializeVtt(const QList<SubtitleItem> &items)
{
    QString out;
    QTextStream ts(&out);

    ts << "WEBVTT\n\n";
    for (int i = 0; i < items.size(); ++i) {
        const auto &item = items.at(i);
        ts << (i + 1) << "\n";
        ts << SubtitleItem::formatTime(item.startTimeMs, true)
           << " --> "
           << SubtitleItem::formatTime(item.endTimeMs, true) << "\n";
        ts << item.text << "\n\n";
    }

    return out;
}

QString SubtitleEngine::serializeAss(const QList<SubtitleItem> &items, const QString &title)
{
    QString out;
    QTextStream ts(&out);

    ts << "[Script Info]\n";
    ts << "Title: " << title << "\n";
    ts << "ScriptType: v4.00+\n";
    ts << "WrapStyle: 0\n";
    ts << "ScaledBorderAndShadow: yes\n";
    ts << "PlayResX: 1920\n";
    ts << "PlayResY: 1080\n\n";

    ts << "[V4+ Styles]\n";
    ts << "Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\n";
    ts << "Style: Default,Arial,54,&H00FFFFFF,&H000000FF,&H00000000,&H80000000,-1,0,0,0,100,100,0,0,1,3,2,2,30,30,40,1\n\n";

    ts << "[Events]\n";
    ts << "Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n";

    const auto formatAssTime = [](qint64 ms) -> QString {
        if (ms < 0) ms = 0;
        const qint64 totalSeconds = ms / 1000;
        const qint64 centis = (ms % 1000) / 10;
        const qint64 hours = totalSeconds / 3600;
        const qint64 minutes = (totalSeconds % 3600) / 60;
        const qint64 seconds = totalSeconds % 60;
        return QStringLiteral("%1:%2:%3.%4")
            .arg(hours)
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'))
            .arg(centis, 2, 10, QLatin1Char('0'));
    };

    for (const auto &item : items) {
        QString text = item.text;
        text.replace(QLatin1Char('\n'), QStringLiteral("\\N"));
        ts << "Dialogue: 0,"
           << formatAssTime(item.startTimeMs) << ","
           << formatAssTime(item.endTimeMs) << ","
           << "Default,,0,0,0,,"
           << text << "\n";
    }

    return out;
}

QString SubtitleEngine::serializePlainText(const QList<SubtitleItem> &items)
{
    QStringList lines;
    for (const auto &item : items) {
        if (!item.text.trimmed().isEmpty()) {
            lines.append(item.text.trimmed());
        }
    }
    return lines.join(QLatin1Char('\n'));
}

bool SubtitleEngine::saveToFile(const QString &filePath, const QList<SubtitleItem> &items)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    const QString ext = QFileInfo(filePath).suffix().toLower();
    QString data;
    if (ext == QLatin1String("vtt")) {
        data = serializeVtt(items);
    } else if (ext == QLatin1String("ass") || ext == QLatin1String("ssa")) {
        data = serializeAss(items, QFileInfo(filePath).baseName());
    } else if (ext == QLatin1String("txt")) {
        data = serializePlainText(items);
    } else {
        data = serializeSrt(items);
    }

    const QByteArray bytes = data.toUtf8();
    file.write(bytes);
    file.close();
    return true;
}

void SubtitleEngine::shiftTime(QList<SubtitleItem> &items, qint64 offsetMs)
{
    if (offsetMs == 0) return;
    for (auto &item : items) {
        item.startTimeMs = std::max<qint64>(0, item.startTimeMs + offsetMs);
        item.endTimeMs = std::max<qint64>(item.startTimeMs, item.endTimeMs + offsetMs);
    }
}

void SubtitleEngine::scaleTime(QList<SubtitleItem> &items, double factor)
{
    if (factor <= 0.0 || factor == 1.0) return;
    for (auto &item : items) {
        item.startTimeMs = static_cast<qint64>(item.startTimeMs * factor);
        item.endTimeMs = static_cast<qint64>(item.endTimeMs * factor);
    }
}

bool SubtitleEngine::mergeItems(QList<SubtitleItem> &items, int indexA, int indexB)
{
    if (indexA < 0 || indexA >= items.size() || indexB < 0 || indexB >= items.size()) {
        return false;
    }
    if (indexA == indexB) return false;

    int first = std::min(indexA, indexB);
    int second = std::max(indexA, indexB);

    SubtitleItem &itemA = items[first];
    const SubtitleItem &itemB = items[second];

    itemA.endTimeMs = std::max(itemA.endTimeMs, itemB.endTimeMs);
    itemA.text = itemA.text.trimmed() + QStringLiteral(" ") + itemB.text.trimmed();

    items.removeAt(second);

    // Re-index
    for (int i = 0; i < items.size(); ++i) {
        items[i].id = i + 1;
    }
    return true;
}

bool SubtitleEngine::splitItem(QList<SubtitleItem> &items, int index, qint64 splitTimeMs)
{
    if (index < 0 || index >= items.size()) return false;
    SubtitleItem &orig = items[index];
    if (splitTimeMs <= orig.startTimeMs || splitTimeMs >= orig.endTimeMs) {
        return false;
    }

    SubtitleItem second;
    second.startTimeMs = splitTimeMs;
    second.endTimeMs = orig.endTimeMs;

    orig.endTimeMs = splitTimeMs;

    const QString text = orig.text.trimmed();
    int mid = text.length() / 2;
    int space = text.indexOf(QLatin1Char(' '), mid);
    if (space == -1) {
        space = text.lastIndexOf(QLatin1Char(' '), mid);
    }

    if (space > 0 && space < text.length() - 1) {
        orig.text = text.left(space).trimmed();
        second.text = text.mid(space + 1).trimmed();
    } else {
        second.text = text;
    }

    items.insert(index + 1, second);

    for (int i = 0; i < items.size(); ++i) {
        items[i].id = i + 1;
    }
    return true;
}

int SubtitleEngine::findItemAtTime(const QList<SubtitleItem> &items, qint64 timeMs)
{
    for (int i = 0; i < items.size(); ++i) {
        if (timeMs >= items.at(i).startTimeMs && timeMs <= items.at(i).endTimeMs) {
            return i;
        }
    }
    return -1;
}
