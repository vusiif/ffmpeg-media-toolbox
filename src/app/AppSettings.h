#pragma once

#include <QString>

// Single source of truth for the QSettings scope shared by all modules.
// Every persistent setting in the app lives under (organization, application).
namespace AppSettings {

inline QString organization()
{
    return QStringLiteral("FFMedia");
}

inline QString application()
{
    return QStringLiteral("FFmpeg Media Toolbox");
}

} // namespace AppSettings
