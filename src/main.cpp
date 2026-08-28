#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "ffmpeg/FFmpegLocator.h"

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle("Basic");

    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("FFMedia"));
    app.setApplicationName(QStringLiteral("FFmpeg Media Toolbox"));

    FFmpegLocator ffmpegLocator;

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty(QStringLiteral("ffmpegLocator"), &ffmpegLocator);

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
