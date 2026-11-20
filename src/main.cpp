#include <QApplication>

#include "ElaApplication.h"
#include "ElaTheme.h"
#include "app/AppSettings.h"
#include "app/MainWindow.h"
#include "i18n/LanguageManager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName(AppSettings::organization());
    app.setApplicationName(AppSettings::application());

    LanguageManager languageManager;

    eApp->init();
    eTheme->setThemeMode(ElaThemeType::Dark);

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(0x20, 0x20, 0x20));
    darkPalette.setColor(QPalette::WindowText, QColor(0xdf, 0xdf, 0xdf));
    darkPalette.setColor(QPalette::Base, QColor(0x2b, 0x2b, 0x2b));
    darkPalette.setColor(QPalette::AlternateBase, QColor(0x20, 0x20, 0x20));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, QColor(0xdf, 0xdf, 0xdf));
    darkPalette.setColor(QPalette::Button, QColor(0x2b, 0x2b, 0x2b));
    darkPalette.setColor(QPalette::ButtonText, QColor(0xdf, 0xdf, 0xdf));
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(0x00, 0x78, 0xd4));
    darkPalette.setColor(QPalette::Highlight, QColor(0x00, 0x78, 0xd4));
    darkPalette.setColor(QPalette::HighlightedText, Qt::white);
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(0x7f, 0x7f, 0x7f));
    darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0x7f, 0x7f, 0x7f));
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x7f, 0x7f, 0x7f));
    app.setPalette(darkPalette);

    MainWindow window(&languageManager);
    window.show();

    return app.exec();
}
