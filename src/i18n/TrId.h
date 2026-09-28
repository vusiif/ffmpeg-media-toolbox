#pragma once

#include <QCoreApplication>
#include <QString>

// Id-based translation lookup.
// lrelease -idbased stores messages under the "Main" context keyed by id;
// qtTrId() does not find them, so go through translate() directly.
inline QString trId(const char *id)
{
    return QCoreApplication::translate("Main", id);
}
