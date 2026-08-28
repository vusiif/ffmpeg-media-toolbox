#pragma once

#include <QString>
#include <QStringList>

struct FFmpegCommand {
    QString program;
    QStringList arguments;

    bool isValid() const { return !program.isEmpty(); }

    QStringList toStringList() const {
        QStringList result;
        result << program;
        result << arguments;
        return result;
    }

    QString toDisplayString() const {
        QStringList parts;
        parts << program;
        for (const QString &arg : arguments) {
            if (arg.contains(QLatin1Char(' '))) {
                parts << QStringLiteral("\"%1\"").arg(arg);
            } else {
                parts << arg;
            }
        }
        return parts.join(QLatin1Char(' '));
    }
};
