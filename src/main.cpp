#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "ffmpeg/FFmpegLocator.h"
#include "ffmpeg/FFmpegCapabilities.h"
#include "media/MediaProbeService.h"
#include "app/TranslationManager.h"

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle("Basic");

    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("FFMedia"));
    app.setApplicationName(QStringLiteral("FFmpeg Media Toolbox"));

    FFmpegLocator ffmpegLocator;
    FFmpegCapabilities ffmpegCaps(&ffmpegLocator);
    MediaProbeService mediaProbe(&ffmpegLocator);
    TranslationManager i18n;

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty(QStringLiteral("ffmpegLocator"), &ffmpegLocator);
    engine.rootContext()->setContextProperty(QStringLiteral("ffmpegCaps"), &ffmpegCaps);
    engine.rootContext()->setContextProperty(QStringLiteral("mediaProbe"), &mediaProbe);
    engine.rootContext()->setContextProperty(QStringLiteral("i18n"), &i18n);

    const QUrl url(QStringLiteral("qrc:/qt/qml/ffmedia/qml/Main.qml"));

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection
    );

    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
