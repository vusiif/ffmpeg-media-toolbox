#include <QApplication>

#include "ElaApplication.h"
#include "ElaTheme.h"
#include "app/MainWindow.h"
#include "i18n/LanguageManager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("FFMedia"));
    app.setApplicationName(QStringLiteral("FFmpeg Media Toolbox"));

    LanguageManager languageManager;

    eApp->init();
    eTheme->setThemeMode(ElaThemeType::Dark);

    MainWindow window(&languageManager);
    window.show();

    return app.exec();
}
